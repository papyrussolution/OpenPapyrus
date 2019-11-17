// C_BILL.CPP
// Copyright (c) A.Sobolev 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2005, 2006, 2007, 2009, 2010, 2011, 2015, 2016, 2017, 2018, 2019
// @codepage UTF-8
// Корректировка документов
//
#include <pp.h>
#pragma hdrstop

// @v9.4.0 @construction {
//
// Descr: Унифицированная структура для проверки и корректировки документов
//
struct BillRecoverParam {
    enum {
		fRecalcAccturns  = 0x0001,
		fCheckAmounts    = 0x0002, // Проверять суммы по документам
		fCheckPayments   = 0x0004, // Проверять оплаты
		fNoRecalcAmounts = 0x0008, // BillRecalcParam
		fRecalcTrfrs     = 0x0010, // BillRecalcParam
		fSearchAbsence   = 0x0020, // Искать потерянные документы
		fCheckOrderLinks = 0x0040  // Проверять привязку строк документов к заказам
    };
    long   Flags;
    SString LogFileName;
};
// } @v9.4.0

struct BillRecalcParam {
	SLAPI  BillRecalcParam() : OpID(0), Flags(0)
	{
		Period.Z();
	}
	enum {
		fNoRecalcAmounts = 0x0001,
		fRecalcTrfrs     = 0x0002
	};
	DateRange Period;
	PPID   OpID;
	long   Flags;
	SString LogFileName;
};

class PrcssrAbsentBill {
public:
	struct AbsentEntry {
		AbsentEntry()
		{
			THISZERO();
		}
		enum {
			fIntrexpnd = 0x0001,
			fRetOnRcpt = 0x0002,
			fMaybeRet  = 0x0004,
			fPlus      = 0x0008,
			fMinus     = 0x0010,
			fByAccTurn = 0x0020  // Отсутствие документа обнаружено по бух проводке
		};
		PPID   ID;
		PPID   OpTypeID;
		PPID   ObjectID;
		PPID   LocID;
		LDATE  Dt;
		long   Flags;
	};
	struct Param {
		Param()
		{
			Period.Z();
		}
		DateRange Period;
	};
	SLAPI  PrcssrAbsentBill();
	SLAPI ~PrcssrAbsentBill();
	int    SLAPI InitParam(Param *);
	int    SLAPI EditParam(Param *);
	int    SLAPI Init(const Param *);
	int    SLAPI Run();
private:
	int    SLAPI ScanTransfer(SArray * pList);
	int    SLAPI ScanAccturn(SArray * pList);
	int    SLAPI Repair(const AbsentEntry * pEntry);
	Param  P;
	SArray * P_List;
	PPObjBill * P_BObj;
};

struct RecoverAbsBillData {
	RecoverAbsBillData()
	{
		THISZERO();
	}
	PPID   BillID;
	PPID   OpTypeID;
	PPID   OpID;
	PPID   LocID;
	PPID   ObjectID;
	LDATE  Dt;
	uint   Flags; //  PrcssrAbcentBill::AbsentEntry::fXXX
	int    Stop;
	double Cost;
	double Price;
	double Discount;
	long   LinesCount;
};

int SLAPI PPObjBill::GatherPayments()
{
	int    ok = 1;
	long   f, f1;
	PPID   bill_id = 0;
	SString bill_name, fmt_buf, msg_buf;
	PPLogger logger;
	PPWait(1);
	IterCounter cntr;
	PPInitIterCounter(cntr, P_Tbl);
	if(P_Tbl->search(0, &bill_id, spFirst)) do {
		BillTbl::Rec rec;
		P_Tbl->copyBufTo(&rec);
		if(CheckOpFlags(rec.OpID, OPKF_NEEDPAYMENT) || CheckOpFlags(rec.OpID, OPKF_RECKON)) {
			DBRowId _preserve_pos;
			PPID   cur_id = rec.CurID;
			double paym = 0.0, real_paym = 0.0;
			const  double amt  = BR2(rec.Amount);
			const  double omt_paymamt = BR2(rec.PaymAmount);
			f = f1 = rec.Flags;
			{
				THROW_DB(P_Tbl->getPosition(&_preserve_pos));
				THROW(P_Tbl->GetAmount(bill_id, PPAMT_PAYMENT, cur_id, &paym));
				THROW(P_Tbl->CalcPayment(bill_id, 1, 0, cur_id, &real_paym));
				real_paym = R6(real_paym);
				// @v10.4.9 {
				if(amt < 0.0) {
					real_paym = -real_paym;
				}
				// } @v10.4.9 
				SETFLAG(f, BILLF_PAYOUT, R2(real_paym - amt) >= 0);
				if(paym != real_paym || f != f1 || real_paym != omt_paymamt) {
					PPObjBill::MakeCodeString(&rec, 1, bill_name);
					PPTransaction tra(1);
					THROW(tra);
					if(paym != real_paym) {
						//"Неверная сумма оплаты по документу '%s' (факт=%.2lf, правильно=%.2lf)"
						msg_buf.Printf(PPLoadTextS(PPTXT_LOG_INVBILLPAYMAMOUNT, fmt_buf), bill_name.cptr(), paym, real_paym);
						logger.Log(msg_buf);
						AmtEntry ae(PPAMT_PAYMENT, cur_id, real_paym);
						THROW(P_Tbl->UpdateAmount(bill_id, &ae, 1));
					}
					if(f != f1 || real_paym != omt_paymamt) {
						if(f != f1) {
							//"Неверно установлен признак 'оплачено' по документу '%s' (факт=%d, правильно=%d)"
							msg_buf.Printf(PPLoadTextS(PPTXT_LOG_INVBILLPAYMFLAG, fmt_buf), bill_name.cptr(), BIN(f1 & BILLF_PAYOUT), BIN(f & BILLF_PAYOUT));
							logger.Log(msg_buf);
						}
						if(real_paym != omt_paymamt) {
							//"Неверная включенная сумма оплаты по документу '%s' (факт=%.2lf, правильно=%.2lf)"
							msg_buf.Printf(PPLoadTextS(PPTXT_LOG_INVBILLOMTPAYMAMT, fmt_buf), bill_name.cptr(), omt_paymamt, real_paym);
							logger.Log(msg_buf);
						}
						THROW_DB(updateFor(P_Tbl, 0, (P_Tbl->ID == bill_id), set(P_Tbl->Flags, dbconst(f)).set(P_Tbl->PaymAmount, dbconst(real_paym))));
					}
					THROW(tra.Commit());
				}
				THROW_DB(P_Tbl->getDirect(0, &bill_id, _preserve_pos));
			}
		}
		PPWaitPercent(cntr.Increment());
	} while(P_Tbl->search(0, &bill_id, spNext));
	PPWait(0);
	CATCHZOKPPERR
	logger.Save(PPFILNAM_ERR_LOG, 0);
	return ok;
}

int SLAPI PPObjBill::CheckAmounts(PPID id, PPLogger & rLogger)
{
	int    ok = 1;
	double bamt, ramt;
	AmtList al;
	PPBillPacket pack;
	THROW(ExtractPacketWithFlags(id, &pack, BPLD_SKIPTRFR) > 0);
	if(pack.Rec.OpID && !oneof3(pack.OpTypeID, PPOPT_ACCTURN, PPOPT_PAYMENT, PPOPT_INVENTORY)) {
		SString log_buf, fmt_buf, bill_buf, amt_buf;
		pack.Rec.Flags |= BILLF_NOLOADTRFR;
		THROW(pack.SumAmounts(&al, 1));
		for(uint i = 0; i < al.getCount(); i++) {
			const PPID t = al.at(i).AmtTypeID;
			bamt = R2(pack.Amounts.Get(t, 0L /* @curID */));
			ramt = R2(al.at(i).Amt);
			if(bamt != ramt) {
				if(PPLoadString(PPMSG_ERROR, PPERR_NMATCHBILLAMT, fmt_buf)) {
					MakeCodeString(&pack.Rec, 1, bill_buf);
					GetObjectName(PPOBJ_AMOUNTTYPE, t, amt_buf);
					log_buf.Printf(fmt_buf, bill_buf.cptr(), t, amt_buf.cptr(), bamt, ramt, bamt-ramt);
					rLogger.Log(log_buf);
				}
			}
		}
		bamt = R2(pack.GetAmount());
		ramt = R2(al.Get(PPAMT_MAIN, pack.Rec.CurID));
		if(bamt != ramt) {
			if(PPLoadString(PPMSG_ERROR, PPERR_NMATCHBILLMAINAMT, fmt_buf)) {
				MakeCodeString(&pack.Rec, 1, bill_buf);
				log_buf.Printf(fmt_buf, bill_buf.cptr(), bamt, ramt, bamt-ramt);
				rLogger.Log(log_buf);
			}
		}
	}
	CATCHZOK
	return ok;
}

static int SLAPI RecalcBillDialog(uint rezID, BillRecalcParam * pFilt)
{
	int    r = -1;
	PPIDArray types;
	TDialog * dlg = 0;
	if(CheckDialogPtr(&(dlg = new TDialog(rezID)))) {
		FileBrowseCtrlGroup::Setup(dlg, CTLBRW_BILLFLT_LOGFILE, CTL_BILLFLT_LOGFILE, 1, 0, 0, FileBrowseCtrlGroup::fbcgfLogFile);
		dlg->SetupCalPeriod(CTLCAL_BILLFLT_PERIOD, CTL_BILLFLT_PERIOD);
		pFilt->Period.SetDate(LConfig.OperDate);
		SetPeriodInput(dlg, CTL_BILLFLT_PERIOD, &pFilt->Period);
		dlg->setCtrlString(CTL_BILLFLT_LOGFILE, pFilt->LogFileName);
		types.addzlist(PPOPT_ACCTURN, PPOPT_GOODSRECEIPT, PPOPT_GOODSEXPEND, PPOPT_GOODSORDER,
			PPOPT_PAYMENT, PPOPT_GOODSRETURN, PPOPT_GOODSREVAL, PPOPT_GOODSACK, PPOPT_GOODSMODIF, PPOPT_GENERIC, PPOPT_CORRECTION, 0L); // @v10.3.6 PPOPT_CORRECTION
		SetupOprKindCombo(dlg, CTLSEL_BILLFLT_OPRKIND, pFilt->OpID, 0, &types, 0);
		dlg->AddClusterAssoc(CTL_BILLFLT_FLAGS, 0, BillRecalcParam::fNoRecalcAmounts);
		dlg->AddClusterAssoc(CTL_BILLFLT_FLAGS, 1, BillRecalcParam::fRecalcTrfrs);
		dlg->SetClusterData(CTL_BILLFLT_FLAGS, pFilt->Flags);
		while(r == -1 && (r = ExecView(dlg)) == cmOK) {
			if(!GetPeriodInput(dlg, CTL_BILLFLT_PERIOD, &pFilt->Period)) {
				PPErrorByDialog(dlg, CTL_BILLFLT_PERIOD);
				r = -1;
			}
			else {
				dlg->getCtrlData(CTLSEL_BILLFLT_OPRKIND, &pFilt->OpID);
				dlg->getCtrlString(CTL_BILLFLT_LOGFILE, pFilt->LogFileName);
				pFilt->LogFileName.Strip();
				dlg->GetClusterData(CTL_BILLFLT_FLAGS, &pFilt->Flags);
			}
		}
	}
	else
		r = 0;
	delete dlg;
	return r;
}

int SLAPI RecalcBillTurns(int checkAmounts)
{
	int    ok = 1;
	int    ta = 0;
	PPObjBill * p_bobj = BillObj;
	int    frrl_tag = 0, r;
	PPLogger logger;
	BillRecalcParam flt;
	flt.Period.SetDate(LConfig.OperDate);
	THROW(r = RecalcBillDialog((checkAmounts ? DLG_CHKBAMT : DLG_RECLCTRN), &flt));
	if(r == cmOK) {
		DateIter diter(&flt.Period);
		{
			PPViewBill bill_view;
			BillFilt bill_flt;
			BillViewItem view_item;
			bill_flt.Bbt = bbtRealTypes;
			bill_flt.Period = flt.Period;
			bill_flt.OpID = flt.OpID;
			PPWait(1);
			bill_view.Init_(&bill_flt);
			if(!checkAmounts) {
				THROW(PPStartTransaction(&ta, 1));
				THROW(p_bobj->atobj->P_Tbl->LockingFRR(1, &frrl_tag, 0));
			}
			for(bill_view.InitIteration(PPViewBill::OrdByDefault); bill_view.NextIteration(&view_item) > 0;) {
				THROW(PPCheckUserBreak());
				if(checkAmounts) {
					THROW(p_bobj->CheckAmounts(view_item.ID, logger));
				}
				else {
					long   flags = 0;
				    if(flt.Flags & BillRecalcParam::fNoRecalcAmounts)
				    	flags |= BORTF_NORECALCAMOUNTS;
				    if(flt.Flags & BillRecalcParam::fRecalcTrfrs) {
				    	flags |= BORTF_RECALCTRFRS;
						flags &= ~BORTF_NORECALCAMOUNTS;
				    }
					THROW(p_bobj->RecalcTurns(view_item.ID, flags, 0));
				}
				PPWaitPercent(bill_view.GetCounter());
			}
			if(!checkAmounts) {
				THROW(p_bobj->atobj->P_Tbl->LockingFRR(0, &frrl_tag, 0));
				THROW(PPCommitWork(&ta));
			}
			PPWait(0);
		}
	}
	CATCH
		p_bobj->atobj->P_Tbl->LockingFRR(-1, &frrl_tag, 0);
		PPRollbackWork(&ta);
		ok = PPErrorZ();
	ENDCATCH
	logger.Save(flt.LogFileName, 0);
	return ok;
}

SLAPI PrcssrAbsentBill::PrcssrAbsentBill() : P_BObj(BillObj), P_List(0)
{
}

SLAPI PrcssrAbsentBill::~PrcssrAbsentBill()
{
}

int SLAPI PrcssrAbsentBill::InitParam(Param * pParam)
{
	CALLPTRMEMB(pParam, Period.Z());
	return 1;
}

int SLAPI PrcssrAbsentBill::EditParam(Param * pParam)
{
	return DateRangeDialog(0, 0, &pParam->Period);
}

int SLAPI PrcssrAbsentBill::Init(const Param * pParam)
{
	RVALUEPTR(P, pParam);
	return 1;
}

int SLAPI PrcssrAbsentBill::ScanAccturn(SArray * pList)
{
	int    ok = 1;
	IterCounter counter;
	AccTurnCore * p_at = P_BObj->atobj->P_Tbl;
	AccTurnTbl::Key2 k2, k2_;
	BExtQuery q(p_at, 2);
	q.selectAll().where(daterange(p_at->Dt, &P.Period));
	MEMSZERO(k2);
	k2.Dt = P.Period.low;
	PPWait(1);
	k2_ = k2;
	counter.Init(q.countIterations(0, &k2_, spGe));
	for(q.initIteration(0, &k2, spGe); q.nextIteration() > 0;) {
		AccTurnTbl::Rec rec;
		p_at->copyBufTo(&rec);
		if(P_BObj->Search(rec.Bill) <= 0) {
			AbsentEntry entry;
			entry.ID    = rec.Bill;
			entry.Dt    = rec.Dt;
			entry.Flags |= AbsentEntry::fByAccTurn;
			uint pos = 0;
			if(pList->lsearch(&entry, &pos, CMPF_LONG)) {
				;
			}
			else {
				THROW_SL(pList->insert(&entry));
			}
		}
		PPWaitPercent(counter.Increment());
	}
	CATCHZOK
	PPWait(0);
	return ok;
}

int SLAPI PrcssrAbsentBill::ScanTransfer(SArray * pList)
{
	int    ok = 1;
	IterCounter counter;
	Transfer * trfr = P_BObj->trfr;
	TransferTbl::Key1 k1, k1_;
	BExtQuery q(trfr, 1);
	q.selectAll().where(daterange(trfr->Dt, &P.Period));
	MEMSZERO(k1);
	k1.Dt = P.Period.low;
	PPWait(1);
	k1_ = k1;
	counter.Init(q.countIterations(0, &k1_, spGe));
	for(q.initIteration(0, &k1, spGe); q.nextIteration() > 0;) {
		TransferTbl::Rec rec;
		ReceiptTbl::Rec lot_rec;
		trfr->copyBufTo(&rec);
		if(P_BObj->Search(rec.BillID) <= 0) {
			AbsentEntry entry;
			entry.ID    = rec.BillID;
			entry.Dt    = rec.Dt;
			entry.LocID = rec.LocID;
			if(rec.Flags & PPTFR_PLUS)
				entry.Flags |= AbsentEntry::fPlus;
			else if(rec.Flags & PPTFR_MINUS)
				entry.Flags |= AbsentEntry::fMinus;
			if(rec.Flags & PPTFR_DRAFT) {
				if(rec.Flags & PPTFR_PLUS)
					entry.OpTypeID = PPOPT_DRAFTRECEIPT;
				else if(rec.Flags & PPTFR_MINUS)
					entry.OpTypeID = PPOPT_DRAFTEXPEND;
			}
			else if(rec.Flags & PPTFR_ORDER)
				entry.OpTypeID = PPOPT_GOODSORDER;
			else if(rec.Flags & PPTFR_REVAL)
				entry.OpTypeID = PPOPT_GOODSREVAL;
			else if(rec.Flags & PPTFR_RECEIPT) {
				if(rec.Reverse == 0) {
					if(TR5(rec.Discount) != 0) {
						entry.OpTypeID = PPOPT_GOODSRETURN;
						entry.Flags |= AbsentEntry::fRetOnRcpt;
					}
					else {
						entry.OpTypeID = PPOPT_GOODSRECEIPT;
						if(trfr->Rcpt.Search(rec.LotID, &lot_rec) > 0)
							entry.ObjectID = lot_rec.SupplID;
					}
				}
				else {
					entry.OpTypeID = PPOPT_GOODSEXPEND;
					entry.Flags |= AbsentEntry::fIntrexpnd;
				}
			}
			else if(rec.Flags & PPTFR_PLUS)
				entry.OpTypeID = PPOPT_GOODSRETURN;
			else if(rec.Flags & PPTFR_MINUS)
				entry.OpTypeID = PPOPT_GOODSEXPEND;
			if(!pList->lsearch(&entry, 0, CMPF_LONG))
				THROW_SL(pList->insert(&entry));
		}
		PPWaitPercent(counter.Increment());
	}
	CATCHZOK
	PPWait(0);
	return ok;
}

class AbsBillDialog : public TDialog {
public:
	AbsBillDialog() : TDialog(DLG_ABSBILL)
	{
	}
	int    setDTS(const RecoverAbsBillData *);
	int    getDTS(RecoverAbsBillData *);
private:
	DECL_HANDLE_EVENT;
	RecoverAbsBillData Data;
};

IMPL_HANDLE_EVENT(AbsBillDialog)
{
	TDialog::handleEvent(event);
	if(event.isCbSelected(CTLSEL_ABSBILL_OP)) {
		if(getCtrlView(CTLSEL_ABSBILL_OBJECT)) {
			PPID   acc_sheet_id = 0, acc_sheet2_id = 0;
			getCtrlData(CTLSEL_ABSBILL_OP, &Data.OpID);
			GetOpCommonAccSheet(Data.OpID, &acc_sheet_id, &acc_sheet2_id);
			SetupArCombo(this, CTLSEL_ABSBILL_OBJECT, Data.ObjectID, 0, acc_sheet_id, sacfDisableIfZeroSheet|sacfNonGeneric);
		}
	}
	else if(event.isCmd(cmViewAccturns))
		BillObj->ViewAccturns(Data.BillID);
	else
		return;
	clearEvent(event);
}

int AbsBillDialog::setDTS(const RecoverAbsBillData * pData)
{
	Data = *pData;
	SString src_text;
	PPOprKind op_rec;
	SString loc_name;
	PPIDArray op_list;
	PPGetSubStr(PPTXT_ABSBILL_SRC, (Data.Flags & PrcssrAbsentBill::AbsentEntry::fByAccTurn) ? 1 : 0, src_text);
	setStaticText(CTL_ABSBILL_ST_SRC, src_text);
	setCtrlData(CTL_ABSBILL_ID, &Data.BillID);
	setCtrlData(CTL_ABSBILL_DT, &Data.Dt);
	setCtrlData(CTL_ABSBILL_LINESCOUNT, &Data.LinesCount);
	setCtrlData(CTL_ABSBILL_COST, &Data.Cost);
	setCtrlData(CTL_ABSBILL_PRICE, &Data.Price);
	GetLocationName(Data.LocID, loc_name);
	setCtrlString(CTL_ABSBILL_LOCNAME, loc_name);
	if(Data.Flags & PrcssrAbsentBill::AbsentEntry::fByAccTurn) {
		SetupOprKindCombo(this, CTLSEL_ABSBILL_OP, Data.OpID, 0, 0, 0);
	}
	else {
		for(PPID op_id = 0; EnumOperations(0, &op_id, &op_rec) > 0;) {
			if(!Data.OpTypeID) {
				if(Data.Flags & PrcssrAbsentBill::AbsentEntry::fPlus) {
					if(IsExpendOp(op_id) == 0)
						op_list.add(op_id);
				}
				else if(Data.Flags & PrcssrAbsentBill::AbsentEntry::fMinus) {
					if(IsExpendOp(op_id) > 0)
						op_list.add(op_id);
				}
			}
			else if(op_rec.OpTypeID == Data.OpTypeID ||
				(op_rec.OpTypeID == PPOPT_GOODSRETURN && Data.Flags & PrcssrAbsentBill::AbsentEntry::fMaybeRet)) {
				if(op_rec.OpTypeID == PPOPT_GOODSRETURN) {
					if(Data.Flags & PrcssrAbsentBill::AbsentEntry::fPlus) {
						if(IsExpendOp(op_id) == 0)
							op_list.add(op_id);
					}
					else if(Data.Flags & PrcssrAbsentBill::AbsentEntry::fMinus) {
						if(IsExpendOp(op_id) > 0)
							op_list.add(op_id);
					}
				}
				else if(Data.Flags & PrcssrAbsentBill::AbsentEntry::fIntrexpnd) {
					if(IsIntrExpndOp(op_id))
						op_list.add(op_id);
				}
				else
					op_list.add(op_id);
			}
		}
		SETIFZ(Data.OpID, op_list.getSingle());
		SetupOprKindCombo(this, CTLSEL_ABSBILL_OP, Data.OpID, 0, &op_list, OPKLF_OPLIST);
	}
	SetupArCombo(this, CTLSEL_ABSBILL_OBJECT, Data.ObjectID, 0, 0, sacfNonGeneric);
	return 1;
}

int AbsBillDialog::getDTS(RecoverAbsBillData * pData)
{
	getCtrlData(CTLSEL_ABSBILL_OP, &Data.OpID);
	if(!Data.OpID)
		return PPErrorByDialog(this, CTLSEL_ABSBILL_OP, PPERR_OPRKINDNEEDED);
	else {
		getCtrlData(CTLSEL_ABSBILL_OBJECT, &Data.ObjectID);
		Data.Stop = BIN(getCtrlUInt16(CTL_ABSBILL_STOP));
		ASSIGN_PTR(pData, Data);
		return 1;
	}
}

int SLAPI PrcssrAbsentBill::Repair(const AbsentEntry * pEntry)
{
	int    ok = -1, valid_data = 0, stop = 0;
	AbsBillDialog * dlg = 0;
	PPTransferItem ti;
	int    rbybill = 0;
	RecoverAbsBillData rabd;
	while(P_BObj->trfr->EnumItems(pEntry->ID, &rbybill, &ti) > 0) {
		rabd.LinesCount++;
		rabd.Cost += ti.Cost * ti.Qtty();
		rabd.Price += ti.Price * ti.Qtty();
		rabd.Discount += ti.Discount * ti.Qtty();
	}
	rabd.Price -= rabd.Discount;
	rabd.Flags = pEntry->Flags;
	rabd.BillID = pEntry->ID;
	rabd.LocID = pEntry->LocID;
	rabd.ObjectID = pEntry->ObjectID;
	rabd.Dt = pEntry->Dt;
	rabd.OpTypeID = pEntry->OpTypeID;
	rabd.OpID = 0;

	THROW(CheckDialogPtr(&(dlg = new AbsBillDialog)));
	dlg->setDTS(&rabd);
	for(valid_data = 0; !valid_data && ExecView(dlg) == cmOK;)
		if(dlg->getDTS(&rabd))
			valid_data = 1;
	stop = dlg->getCtrlUInt16(CTL_ABSBILL_STOP) ? 1 : 0;
	delete dlg;
	dlg = 0;
	if(valid_data) {
		if(rabd.Stop) {
			ok = -2;
		}
		else {
			BillTbl::Rec bill_rec;
			PPBillPacket temp_pack;
			PPTransaction tra(1);
			THROW(tra);
			THROW(temp_pack.CreateBlank(rabd.OpID, 0, rabd.LocID, 0));
			MEMSZERO(bill_rec);
			bill_rec.ID = rabd.BillID;
			bill_rec.Dt = rabd.Dt;
			bill_rec.LocID = NZOR(rabd.LocID, LConfig.Location);
			bill_rec.OpID  = rabd.OpID;
			bill_rec.Object = rabd.ObjectID;
			bill_rec.Flags = temp_pack.Rec.Flags;
			STRNSCPY(bill_rec.Code, "ABSRCVR");
			STRNSCPY(bill_rec.Memo, "Absent recovered");
			THROW(P_BObj->P_Tbl->_GetBillNo(bill_rec.Dt, &bill_rec.BillNo));
			THROW_DB(P_BObj->P_Tbl->insertRecBuf(&bill_rec));
			THROW(tra.Commit());
			ok = 1;
		}
	}
	else if(stop)
		ok = -2;
	CATCHZOK
	delete dlg;
	return ok;
}

int SLAPI PrcssrAbsentBill::Run()
{
	int    ok = 1;
	SArray bill_list(sizeof(AbsentEntry));
	THROW(ScanTransfer(&bill_list));
	THROW(ScanAccturn(&bill_list));
	for(uint i = 0; i < bill_list.getCount(); i++) {
		int r = Repair(static_cast<AbsentEntry *>(bill_list.at(i)));
		if(r == 0)
			PPError();
		else if(r == -2) {
			ok = -1;
			break;
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI RecoverAbsenceBills()
{
	int    ok = -1;
	PrcssrAbsentBill prcssr;
	PrcssrAbsentBill::Param param;
	prcssr.InitParam(&param);
	if(prcssr.EditParam(&param) > 0)
		if(prcssr.Init(&param) && prcssr.Run())
			ok = 1;
		else
			ok = PPErrorZ();
	return ok;
}

int SLAPI RemoveBadReckons()
{
	int    ok = 1;
	PPObjBill * p_bobj = BillObj;
	IterCounter cntr;
	PPObjOprKind op_obj;
	ObjAssoc & a = PPRef->Assc;
	ObjAssocTbl::Key1 k1;
	MEMSZERO(k1);
	k1.AsscType = PPASS_PAYMBILLPOOL;
	{
		long num_iters = 0;
		while(a.search(1, &k1, spGt) && k1.AsscType == PPASS_PAYMBILLPOOL)
			num_iters++;
		cntr.Init(num_iters);
		MEMSZERO(k1);
		k1.AsscType = PPASS_PAYMBILLPOOL;
	}
	PPWait(1);
	{
		PPTransaction tra(1);
		THROW(tra);
		while(a.search(1, &k1, spGt) && k1.AsscType == PPASS_PAYMBILLPOOL) {
			BillTbl::Rec prmr_bill_rec, scnd_bill_rec;
			int    r1 = p_bobj->Search(k1.PrmrObjID, &prmr_bill_rec);
			int    r2 = p_bobj->Search(k1.ScndObjID, &scnd_bill_rec);
			if(r1 <= 0)
				MEMSZERO(prmr_bill_rec);
			if(r2 <= 0)
				MEMSZERO(scnd_bill_rec);
			int    is_removed = 0;
			SString msg_buf, code1, code2;
			PPObjBill::MakeCodeString(&scnd_bill_rec, 1, code1);
			PPObjBill::MakeCodeString(&prmr_bill_rec, 1, code2);
			msg_buf.Cat(code1).Space().CatChar('-').CatChar('>').Space().Cat(code2).Quot('[', ']').Space().Cat("removed");
			if(r1 > 0 && r2 > 0) {
				PPOprKind op_rec;
				PPID   op_type_id = GetOpType(scnd_bill_rec.OpID, &op_rec);
				if(op_type_id != PPOPT_PAYMENT) {
					THROW_DB(a.deleteRec());
					is_removed = 1;
				}
				else {
					PPReckonOpEx rox;
					if(op_obj.GetReckonExData(prmr_bill_rec.OpID, &rox) <= 0 || !rox.OpList.lsearch(scnd_bill_rec.OpID)) {
						THROW_DB(a.deleteRec());
						is_removed = 1;
					}
				}
			}
			else {
				THROW_DB(a.deleteRec());
				is_removed = 1;
			}
			if(is_removed)
				PPLogMessage(PPFILNAM_CBADRECKONS_LOG, msg_buf, 0);
			PPWaitPercent(cntr.Increment());
		}
		THROW(tra.Commit());
	}
	PPWait(0);
	CATCHZOKPPERR
	return ok;
}

int SLAPI PPObjBill::SearchPaymWOLinkBill()
{
	int    ok = 1;
	PPID   op_id;
	PPOprKind  opk;
	PPIDArray  op_ary;
	StrAssocArray op_name_ary;
	PPObjArticle  ar_obj;
	BillTbl::Key2 bk2, bk2_;
	BExtQuery  bq(P_Tbl, 2, 1);
	DBQ * dbq = 0;
	IterCounter cntr;
	PPLogger logger;
	int    repare = 0;
	SString log_filename, wait_msg_buf, msg_buf, code;
	{
		TDialog * dlg = new TDialog(DLG_C_LINKBILL);
		THROW(CheckDialogPtr(&dlg));
		FileBrowseCtrlGroup::Setup(dlg, CTLBRW_C_LINKBILL_LOG, CTL_C_LINKBILL_LOG, 1, 0, 0, FileBrowseCtrlGroup::fbcgfLogFile);
		PPGetFileName(PPFILNAM_PAYMLINK_LOG, log_filename);
		dlg->setCtrlString(CTL_C_LINKBILL_LOG, log_filename);
		dlg->setCtrlUInt16(CTL_C_LINKBILL_FLAGS, BIN(repare));
		if(ExecView(dlg) == cmOK) {
			dlg->getCtrlString(CTL_C_LINKBILL_LOG, log_filename);
			repare = BIN(dlg->getCtrlUInt16(CTL_C_LINKBILL_FLAGS));
		}
		else
			repare = -1;
		delete dlg;
	}
	if(repare >= 0) {
		PPWait(1);
		PPWaitMsg(PPLoadTextS(PPTXT_WAIT_SEARCHUNLINKEDPAYMS, wait_msg_buf));
		PPTransaction tra(BIN(repare > 0));
		THROW(tra);
		for(op_id = 0; EnumOperations(PPOPT_PAYMENT, &op_id, &opk) > 0;) {
			THROW(op_ary.add(op_id));
			THROW_SL(op_name_ary.Add(op_id, opk.Name));
		}
		for(op_id = 0; EnumOperations(PPOPT_GOODSRETURN, &op_id, &opk) > 0;) {
			THROW(op_ary.add(op_id));
			THROW_SL(op_name_ary.Add(op_id, opk.Name));
		}
		op_name_ary.SortByID();
		dbq = &(ppidlist(P_Tbl->OpID, &op_ary));
		bq.selectAll().where(*dbq);
		MEMSZERO(bk2);
		bk2_ = bk2;
		cntr.Init(bq.countIterations(0, &bk2_, spGe));
		for(bq.initIteration(0, &bk2, spGe); bq.nextIteration() > 0; cntr.Increment()) {
			BillTbl::Rec b_rec;
			P_Tbl->copyBufTo(&b_rec);
			if(b_rec.LinkBillID == 0 || P_Tbl->Search(b_rec.LinkBillID) <= 0) {
				ArticleTbl::Rec ar_rec;
				MakeCodeString(&b_rec, 0, code);
				msg_buf.Z();
				op_name_ary.GetText(b_rec.OpID, msg_buf);
				msg_buf.Space().Cat(code);
				if(b_rec.Object && ar_obj.Fetch(b_rec.Object, &ar_rec) && ar_rec.Name[0])
					msg_buf.Space().Cat(ar_rec.Name);
				if(repare > 0) {
					if(b_rec.LinkBillID) {
						THROW_DB(updateFor(P_Tbl, 0, (P_Tbl->ID == b_rec.ID), set(P_Tbl->LinkBillID, dbconst(0L))));
						DS.LogAction(PPACN_BILLCORRECTED, Obj, b_rec.ID, PPACN_EXT_BILLCORRECTED_LINKRESET, 0);
						// @v10.0.0 PPGetWord(PPWORD_CORRECTED, 0, code);
						msg_buf.CatDiv(';', 2).Cat(PPLoadStringS("corrected", code));
					}
				}
				logger.Log(msg_buf);
			}
			PPWaitPercent(cntr, wait_msg_buf);
		}
		THROW(tra.Commit());
	}
	CATCHZOKPPERR
	logger.Save(log_filename, 0);
	PPWait(0);
	return ok;
}

int SLAPI PPObjBill::RecoverUnitedFreightPorts()
{
	int    ok = 1;
	SysJournal * p_sj = DS.GetTLA().P_SysJ;
	if(p_sj) {
		IterCounter cntr;
		PPObjWorld w_obj;
		WorldTbl::Rec w_rec;
		SString temp_buf, native_country_name;
		PropertyTbl * p_prop = &PPRef->Prop;
		PropertyTbl::Key0 k0, k0_;
		BExtQuery q(p_prop, 0, 256);
		DBQ * dbq = &(p_prop->ObjType == PPOBJ_BILL && p_prop->Prop == BILLPRP_FREIGHT);
		q.select(p_prop->ObjType, p_prop->ObjID, p_prop->Prop, p_prop->Text, p_prop->Val1, p_prop->Val2, 0L).where(*dbq);
		MEMSZERO(k0);
		k0.ObjType = PPOBJ_BILL;
		PPWait(1);
		{
			PPID   native_country_id = 0;
			if(w_obj.GetNativeCountry(&native_country_id) > 0 && w_obj.Search(native_country_id, &w_rec) > 0)
				native_country_name = w_rec.Name;
		}
		PPTransaction tra(1);
		THROW(tra);
		k0_ = k0;
		cntr.Init(q.countIterations(0, &k0_, spGe));
		for(q.initIteration(0, &k0, spGe); q.nextIteration() > 0;) {
			int do_update = 0;
			PPFreight fr = *reinterpret_cast<const PPFreight *>(&p_prop->data);
			if(fr.PortOfLoading && w_obj.Search(fr.PortOfLoading, &w_rec) < 0) {
				PPID   subst_id = 0;
				if(p_sj->GetLastObjUnifyEvent(PPOBJ_WORLD, fr.PortOfLoading, &subst_id, 0) > 0) {
					if(w_obj.Search(fr.PortOfLoading, &w_rec) > 0) {
						fr.PortOfLoading = subst_id;
						do_update = 1;
					}
				}
				if(!do_update) {
					subst_id = 0;
					(temp_buf = "City").Space().CatChar('#').Cat(fr.PortOfLoading);
					THROW(w_obj.AddSimple(&subst_id, WORLDOBJ_CITY, temp_buf, native_country_name, 0));
					fr.PortOfLoading = subst_id;
					do_update = 1;
				}
			}
			if(fr.PortOfDischarge && w_obj.Search(fr.PortOfDischarge, &w_rec) < 0) {
				PPID   subst_id = 0;
				if(p_sj->GetLastObjUnifyEvent(PPOBJ_WORLD, fr.PortOfDischarge, &subst_id, 0) > 0) {
					if(w_obj.Search(fr.PortOfDischarge, &w_rec) > 0) {
						fr.PortOfDischarge = subst_id;
						do_update = 1;
					}
				}
				if(!do_update) {
					subst_id = 0;
					(temp_buf = "City").Space().CatChar('#').Cat(fr.PortOfDischarge);
					THROW(w_obj.AddSimple(&subst_id, WORLDOBJ_CITY, temp_buf, native_country_name, 0));
					fr.PortOfDischarge = subst_id;
					do_update = 1;
				}
			}
			if(do_update) {
				THROW(P_Tbl->SetFreight(fr.ID, &fr, 0));
				DS.LogAction(PPACN_RCVRFREIGHTCITY, PPOBJ_BILL, fr.ID, 0, 0);
			}
			PPWaitPercent(cntr.Increment());
		}
		THROW(tra.Commit());
		PPWait(0);
	}
	CATCHZOKPPERR
	return ok;
}

