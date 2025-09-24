// OBJCSESS.CPP
// Copyright (c) A.Sobolev 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021, 2023, 2024, 2025
// @codepage UTF-8
//
#include <pp.h>
#pragma hdrstop
// @v12.4.1 #include <crpe.h>
//
// PPObjCSession
//
static const struct __RtToS {
	long   R;
	int    IsOpr;
	const char * S;
} RtToS[] = {
	{ CSESSRT_OPEN,               0, "O" },
	{ CSESSRT_CLOSE,              0, "Z" }, // .
	{ CSESSRT_ADDCHECK,           0, "A" },
	{ CSESSRT_RMVCHECK,           0, "D" },
	{ CSESSRT_CHECKINFO,          0, "E" },
	{ CSESSRT_ESCCHECK,           0, "C" }, // .
	{ CSESSRT_SYSINFO,            0, "N" },
	{ CSESSRT_CORRECT,            0, "Y" },
	{ CSESSOPRT_RETCHECK,         1, "R" }, // .
	{ CSESSOPRT_BANKING,          1, "B" }, // .
	{ CSESSOPRT_ESCCLINE,         1, "L" }, // .
	{ CSESSOPRT_PREPRT,           1, "P" },
	{ CSESSOPRT_SUSPCHECK,        1, "S" },
	{ CSESSOPRT_COPYCHECK,        1, "1" }, // .
	{ CSESSOPRT_COPYZREPT,        1, "2" }, // .
	{ CSESSOPRT_SELNOTOWNSUSPCHK, 1, "T" },
	{ CSESSOPRT_ROWDISCOUNT,      1, "I" }, // .
	{ CSESSOPRT_XREP,             1, "X" }, // .
	{ CSESSOPRT_CTBLORD,          1, "K" }, // .
	{ CSESSOPRT_SPLITCHK,         1, "F" },
	{ CSESSOPRT_CHGPRINTEDCHK,    1, "G" },
	{ CSESSOPRT_CHGCCAGENT,       1, "J" },
	{ CSESSOPRT_MERGECHK,         1, "M" },
	{ CSESSOPRT_ESCCLINEBORD,     1, "Q" },
	{ CSESSOPRT_REPRNUNFCC,       1, "3" },
	{ CSESSOPRT_ARBITRARYDISC,    1, "4" }, // @v11.0.9
};

/*static*/void PPObjCSession::RightsToString(long rt, long opRt, SString & rBuf)
{
	rBuf.Z();
	for(uint i = 0; i < SIZEOFARRAY(RtToS); i++) {
		if((rt & RtToS[i].R && !RtToS[i].IsOpr) || (opRt & RtToS[i].R && RtToS[i].IsOpr))
			rBuf.Cat(RtToS[i].S);
	}
}

/*static*/void PPObjCSession::StringToRights(const char * pBuf, long * pRt, long * pOpRt)
{
	long   rt = 0;
	long   ort = 0;
	const  size_t len = sstrlen(pBuf);
	for(size_t c = 0; c < len; c++) {
		for(uint i = 0; i < SIZEOFARRAY(RtToS); i++) {
			const size_t cl = sstrlen(RtToS[i].S);
			if(strnicmp(RtToS[i].S, pBuf+c, cl) == 0) {
				if(RtToS[i].IsOpr)
					ort |= RtToS[i].R;
				else
					rt |= RtToS[i].R;
				c += (cl-1);
			}
		}
	}
	ASSIGN_PTR(pRt, rt);
	ASSIGN_PTR(pOpRt, ort);
}

TLP_IMPL(PPObjCSession, CSessionCore, P_Tbl);
TLP_IMPL(PPObjCSession, CCheckCore, P_Cc);

PPObjCSession::PPObjCSession(void * extraPtr) : PPObject(PPOBJ_CSESSION), P_EqCfg(0), ExtraPtr(extraPtr)
{
	TLP_OPEN(P_Tbl);
	TLP_OPEN(P_Cc);
}

PPObjCSession::~PPObjCSession()
{
	TLP_CLOSE(P_Tbl);
	TLP_CLOSE(P_Cc);
	delete P_EqCfg;
}

const PPEquipConfig & PPObjCSession::GetEqCfg()
{
	if(P_EqCfg == 0) {
		P_EqCfg = new PPEquipConfig;
		ReadEquipConfig(P_EqCfg);
	}
	return *P_EqCfg;
}

int PPObjCSession::Search(PPID id, void * b) { return SearchByID(P_Tbl, Obj, id, b); }
const char * PPObjCSession::GetNamePtr() { return MakeCodeString(&P_Tbl->data, NameBuf).cptr(); }

/*static*/SString & FASTCALL PPObjCSession::MakeCodeString(const CSessionTbl::Rec * pRec, SString & rBuf)
{
	rBuf.Z().Cat(pRec->Dt).CatDiv('-', 1).Cat(pRec->SessNumber).CatDiv('-', 1).Cat(pRec->CashNumber).CatDiv('-', 1);
	CatObjectName(PPOBJ_CASHNODE, pRec->CashNodeID, rBuf);
	return rBuf;
}

int PPObjCSession::Edit(PPID * pID, void * extraPtr)
{
	class CSessDialog : public PPListDialog {
	public:
		CSessDialog(const CSessTotal & rTotal) : PPListDialog(DLG_CSESS, CTL_CSESS_AMTLIST, 0), R_Total(rTotal)
		{
			updateList(-1);
		}
	private:
		virtual int  setupList()
		{
			int    ok = 1;
			StringSet ss(SLBColumnDelim);
			SString sub;
			struct Entry {
				double Value;
				const char * P_TextSymb;
			};
			const Entry data_list[] = {
				{ static_cast<double>(R_Total.CheckCount), "csesstotal_checkcount"      }, // "Количество чеков"
				{ R_Total.Amount, "csesstotal_amount"          }, // "Сумма по чекам"
				{ R_Total.Discount, "csesstotal_discount"        }, // "Скидка по чекам"
				{ static_cast<double>(R_Total.AggrCount), "csesstotal_aggrcount"       }, // "Количество агрегирующих строк"
				{ R_Total.AggrAmount, "csesstotal_aggramount"      }, // "Сумма по агрегирующим строкам"
				{ R_Total.AggrRest, "csesstotal_aggrrest"        }, // "Сумма дефицита по агрегирующим строкам"
				{ static_cast<double>(R_Total.WrOffBillCount), "csesstotal_wroffbillcount"  }, // "Количество документов списания"
				{ R_Total.WrOffAmount, "csesstotal_wroffbillamount" }, // "Сумма документов списания"
				{ R_Total.WrOffCost, "csesstotal_wroffcost"       }, // "Сумма документов списания в ценах поступления"
				{ R_Total.Income, "csesstotal_income"          }, // "Доход"
				{ R_Total.BnkAmount, "csesstotal_bnkamount"       }, // "Сумма безналичных оплат"
				{ R_Total.RetAmount, "csesstotal_retamount"       }, // "Сумма возвратов"
				{ R_Total.RetBnkAmount, "csesstotal_retbnkamount"    }, // "Сумма возвратов проведенных через банк"
				{ R_Total.WORetAmount, "csesstotal_woretamount"     }, // "Сумма чеков без учета возвратов"
				{ R_Total.WORetBnkAmount, "csesstotal_woretbnkamount"  }, // "Сумма безналичных оплат без учета возвратов"
				{ R_Total.BnkDiscount, "csesstotal_bnkdiscount"     }, // "Сумма скидок по чекам оплаченных через банк"
				{ R_Total.CSCardAmount, "csesstotal_cscardamount"    }, // "Сумма оплат по корпоративным картам"
				{ R_Total.FiscalAmount, "csesstotal_fiscalamount"    }, // "Фискальная сумма чеков"
				{ R_Total.AltRegAmount, "csesstotal_altregamount"    }, // "Сумма чеков проведенных через альтернативный регистратор"
				{ static_cast<double>(R_Total.SaleCheckCount), "csesstotal_salecheckcount"  }, // "Количество чеков продаж"
				{ static_cast<double>(R_Total.SaleBnkCount), "csesstotal_salebnkcount"    }, // "Количество чеков продаж оплаченных безналично"
				{ static_cast<double>(R_Total.RetCheckCount), "csesstotal_retcheckcount"   }, // "Количество чеков возвратов"
				{ static_cast<double>(R_Total.RetBnkCount), "csesstotal_retbnkcount"     }, // "Количество чеков возвратов оплаченных безналично"
				{ static_cast<double>(R_Total.AltRegCount), "csesstotal_altregcount"     }, // "Количество чеков проведенных через альтернативный регистратор"
			};
			for(uint i = 0; i < SIZEOFARRAY(data_list); i++) {
				const Entry & r_entry = data_list[i];
				if(r_entry.Value != 0.0) {
					ss.Z();
					PPLoadString(r_entry.P_TextSymb, sub);
					ss.add(sub);
					long fmt = (ffrac(r_entry.Value) == 0.0) ? MKSFMTD(0, 0, 0) : MKSFMTD_020;
					sub.Z().Cat(r_entry.Value, fmt);
					ss.add(sub);
					addStringToList(i+1, ss.getBuf());
				}
			}
			return ok;
		}
		const CSessTotal & R_Total;
	};
	int    ok = cmCancel;
	int    valid_data = 0;
	SString super_sess_text;
	CSessDialog * dlg = 0;
	CSessionTbl::Rec rec;
	CSessTotal total_blk;
	if(*pID) {
		PPIDArray sub_list;
		THROW(Search(*pID, &rec) > 0);
		P_Tbl->GetSubSessList(rec.ID, &sub_list);
		sub_list.add(rec.ID);
		sub_list.sortAndUndup();
		for(uint i = 0; i < sub_list.getCount(); i++) {
			const  PPID sess_id = sub_list.at(i);
			THROW(P_Cc->GetSessTotal(sess_id, 0, &total_blk, 0));
		}
	}
	THROW(CheckDialogPtr(&(dlg = new CSessDialog(total_blk))));
	dlg->setCtrlData(CTL_CSESS_ID, &rec.ID);
	dlg->setCtrlData(CTL_CSESS_SUPERSESSID, &rec.SuperSessID);
	GetObjectName(PPOBJ_CSESSION, rec.SuperSessID, super_sess_text);
	dlg->setCtrlString(CTL_CSESS_SUPERSESSTEXT, super_sess_text);
	dlg->disableCtrl(CTL_CSESS_SUPERSESSTEXT, true);
	dlg->setCtrlData(CTL_CSESS_NUMBER, &rec.SessNumber);
	dlg->SetupCalDate(CTLCAL_CSESS_DATE, CTL_CSESS_DATE);
	dlg->setCtrlData(CTL_CSESS_DATE, &rec.Dt);
	dlg->setCtrlData(CTL_CSESS_TIME, &rec.Tm);
	SetupPPObjCombo(dlg, CTLSEL_CSESS_CASHNODE, PPOBJ_CASHNODE, rec.CashNodeID, 0, 0);
	dlg->AddClusterAssoc(CTL_CSESS_INCOMPL,  0, CSESSINCMPL_COMPLETE);
	dlg->AddClusterAssoc(CTL_CSESS_INCOMPL,  1, CSESSINCMPL_GLINES);
	dlg->AddClusterAssocDef(CTL_CSESS_INCOMPL,  2, CSESSINCMPL_CHECKS);
	dlg->SetClusterData(CTL_CSESS_INCOMPL, rec.Incomplete);
	dlg->setCtrlUInt16(CTL_CSESS_TEMP, BIN(rec.Temporary));
	dlg->disableCtrl(CTL_CSESS_ID, true);
	for(valid_data = 0; !valid_data && ExecView(dlg) == cmOK;) {
		dlg->getCtrlData(CTL_CSESS_ID, &rec.ID);
		dlg->getCtrlData(CTL_CSESS_SUPERSESSID, &rec.SuperSessID);
		dlg->getCtrlData(CTL_CSESS_NUMBER, &rec.SessNumber);
		dlg->getCtrlData(CTL_CSESS_DATE, &rec.Dt);
		dlg->getCtrlData(CTL_CSESS_TIME, &rec.Tm);
		dlg->getCtrlData(CTLSEL_CSESS_CASHNODE, &rec.CashNodeID);
		dlg->GetClusterData(CTL_CSESS_INCOMPL, &rec.Incomplete);
		rec.Temporary = BIN(dlg->getCtrlUInt16(CTL_CSESS_TEMP));
		if(!checkdate(rec.Dt))
			PPErrorByDialog(dlg, CTL_CSESS_DATE, PPERR_SLIB);
		else if(!checktime(rec.Tm))
			PPErrorByDialog(dlg, CTL_CSESS_TIME, PPERR_SLIB);
		else if(rec.CashNodeID == 0)
			PPErrorByDialog(dlg, CTLSEL_CSESS_CASHNODE, PPERR_CASHNODENEEDED);
		else {
			if(CheckRights(CSESSRT_SYSINFO)) {
				int    r = (*pID) ? UpdateByID(P_Tbl, Obj, *pID, &rec, 1) : AddObjRecByID(P_Tbl, Obj, pID, &rec, 1);
				if(!r)
					PPError();
			}
			else
				PPError();
			valid_data = 1;
			ok = cmOK;
		}
	}
	CATCHZOKPPERR
	delete dlg;
	return ok;
}

int PPObjCSession::Recalc(PPID sessID, int use_ta)
{
	MemLeakTracer mlt;
	int    ok = 1;
	PPObjBill * p_bobj = BillObj;
	uint   i;
	const  PPID ret_op_id = GetCashRetOp();
	const  PPID wroff_acc_op_id = GetEqCfg().WrOffAccOpID;
	CGoodsLine cgl;
	PPIDArray sub_list;
	CSessionTbl::Rec sess_rec;
	CSessTotal cs_total;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW(Search(sessID, &sess_rec) > 0);
		P_Tbl->GetSubSessList(sessID, &sub_list);
		sub_list.add(sessID);
		for(i = 0; i < sub_list.getCount(); i++) {
			PPID   bill_id = 0, sess_id = sub_list.at(i);
			BillTbl::Rec bill_rec;
			while(p_bobj->P_Tbl->EnumMembersOfPool(PPASS_CSESSBILLPOOL, sess_id, &bill_id) > 0) {
				if(p_bobj->Search(bill_id, &bill_rec) > 0 && bill_rec.OpID != wroff_acc_op_id) {
					double amt = BR2(bill_rec.Amount);
					if(bill_rec.OpID == ret_op_id)
						amt = -amt;
					cs_total.WrOffAmount += amt;
				}
			}
			THROW(cgl.CalcSessTotal(sess_id, &cs_total));
			THROW(P_Cc->GetSessTotal(sess_id, P_Tbl->GetCcGroupingFlags(sess_rec, sess_id), &cs_total, 0));
		}
		THROW_DB(updateFor(P_Tbl, 0, (P_Tbl->ID == sessID),
			set(P_Tbl->Amount, dbconst(cs_total.Amount)).
			set(P_Tbl->Discount, dbconst(cs_total.Discount)).
			set(P_Tbl->AggrAmount, dbconst(cs_total.AggrAmount)).
			set(P_Tbl->AggrRest, dbconst(cs_total.AggrRest)).
			set(P_Tbl->WrOffAmount, dbconst(cs_total.WrOffAmount)).
			set(P_Tbl->BnkAmount, dbconst(cs_total.BnkAmount)).         // @CSCardAmount
			set(P_Tbl->CSCardAmount, dbconst(cs_total.CSCardAmount))));
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int PPObjCSession::VerifyAmounts(PPID sessID, const CSessTotal & rTotal, PPLogger & rLogger)
{
	int    ok = 1;
	double delta;
	SString added_msg_buf;
	CSessionTbl::Rec rec;
	THROW(Search(sessID, &rec) > 0);
	//
	delta = R6(rTotal.Amount - rec.Amount);
	if(delta != 0) {
		MakeCodeString(&rec, added_msg_buf).Space().CatEq("delta", delta);
		rLogger.LogMsgCode(mfError, PPERR_CSES_CHKAMOUNT, added_msg_buf);
		ok = -1;
	}
	//
	delta = R6(rTotal.Discount - rec.Discount);
	if(delta != 0) {
		MakeCodeString(&rec, added_msg_buf).Space().CatEq("delta", delta);
		rLogger.LogMsgCode(mfError, PPERR_CSES_CHKDISCOUNT, added_msg_buf);
		ok = -1;
	}
	//
	if(rTotal.AggrAmount != 0) {
		//
		delta = R6(rTotal.AggrAmount - rec.AggrAmount);
		if(delta != 0) {
			MakeCodeString(&rec, added_msg_buf).Space().CatEq("delta", delta);
			rLogger.LogMsgCode(mfError, PPERR_CSES_WROFFAMOUNT, added_msg_buf);
			ok = -1;
		}
		//
		delta = R0(rec.Amount - rec.WrOffAmount - rec.AggrRest);
		if(delta != 0) {
			MakeCodeString(&rec, added_msg_buf).Space().CatEq("delta", delta);
			rLogger.LogMsgCode(mfError, PPERR_CSES_WROFFAMOUNT, added_msg_buf);
			ok = -1;
		}
	}
	CATCHZOK
	return ok;
}

int PPObjCSession::Recover(const PPIDArray & rSessList)
{
	int    ok = 1;
	PPObjBill * p_bobj = BillObj;
	PPWaitStart();
	CGoodsLine cgl;
	SString added_msg_buf, temp_buf;
	CSessionTbl::Rec sess_rec, sub_rec;
	PPLogger logger;
	const  PPID ret_op_id = GetCashRetOp();
	const  PPID wroff_acc_op_id = GetEqCfg().WrOffAccOpID;
	{
		PPTransaction tra(1);
		THROW(tra);
		for(uint j = 0; j < rSessList.getCount(); j++) {
			int    is_super = 0, r;
			const  PPID outer_sess_id = rSessList.get(j);
			PPIDArray sub_list;
			CSessTotal cs_total;
			THROW(Search(outer_sess_id, &sess_rec) > 0);
			if(P_Tbl->GetSubSessList(outer_sess_id, &sub_list) > 0)
				is_super = 1;
			sub_list.add(outer_sess_id);
			for(uint i = 0; i < sub_list.getCount(); i++) {
				PPID   bill_id = 0, sess_id = sub_list.at(i);
				CSessTotal sub_total;
				BillTbl::Rec bill_rec;
				while(p_bobj->P_Tbl->EnumMembersOfPool(PPASS_CSESSBILLPOOL, sess_id, &bill_id) > 0) {
					if(p_bobj->Search(bill_id, &bill_rec) > 0 && bill_rec.OpID != wroff_acc_op_id)
						cs_total.WrOffAmount += BR2((bill_rec.OpID == ret_op_id) ? -bill_rec.Amount : bill_rec.Amount);
				}
				THROW(cgl.CalcSessTotal(sess_id, &cs_total));
				THROW(P_Cc->GetSessTotal(sess_id, P_Tbl->GetCcGroupingFlags(sess_rec, sess_id), &sub_total, 0));
				cs_total.Add(&sub_total);
				if(sess_id != outer_sess_id) {
					THROW(r = VerifyAmounts(sess_id, sub_total, logger));
					if(r < 0)
						ok = -1;
					THROW(Search(sess_id, &sub_rec) > 0);
					if(sub_rec.WrOffAmount > 0.0 || P_Tbl->HasChildren(sess_id) > 0) {
						MakeCodeString(&sub_rec, temp_buf);
						MakeCodeString(&sess_rec, added_msg_buf).CatChar('-').CatChar('>').Cat(temp_buf);
						logger.LogMsgCode(mfError, PPERR_CSES_INNERSUPER, added_msg_buf);
						ok = -1;
					}
				}
			}
			THROW(r = VerifyAmounts(outer_sess_id, cs_total, logger));
			if(r < 0)
				ok = -1;
			PPWaitPercent(j+1, rSessList.getCount());
		}
		THROW(tra.Commit());
	}
	CATCH
		ok = 0;
		logger.LogLastError();
	ENDCATCH
	PPWaitStop();
	return ok;
}

int PPObjCSession::ReWriteOff(PPID sessID, int level /* @#[0,5,10] */, int use_ta)
{
	int    ok = 1;
	LAssocArray dfct_subst_list;
	PPObjCashNode cn_obj;
	PPCashNode cn_rec;
	CSessGrouping csg;
	CSessionTbl::Rec sess_rec;
	CSessTotal   total;
	THROW(CheckRights(CSESSRT_CORRECT));
	PPWaitStart();
	{
		PPObjSecur::Exclusion ose(PPEXCLRT_CSESSWROFF);
		PPTransaction tra(use_ta);
		THROW(tra);
		if(level == 0) {
			THROW(Recalc(sessID, 0));
		}
		else if(level == 5) {
			THROW(csg.GetSess(sessID, &sess_rec) > 0);
			THROW(cn_obj.Search(sess_rec.CashNodeID, &cn_rec) > 0);
			THROW(UndoWritingOff(sessID, 0));
			if(sess_rec.Incomplete == CSESSINCMPL_CHECKS)
				THROW(csg.Grouping(sessID, &total, 0, 0));
			THROW(csg.ConvertToBills(sessID, cn_rec.LocID, 0, 1, 1, 0));
		}
		else if(level == 10) {
			THROW(csg.GetSess(sessID, &sess_rec) > 0);
			THROW(cn_obj.Search(sess_rec.CashNodeID, &cn_rec) > 0);
			THROW(UndoWritingOff(sessID, 0));
			THROW(csg.UndoGrouping(sessID, &dfct_subst_list, 0));
			THROW(csg.GetSess(sessID, &sess_rec) > 0);
			if(sess_rec.Incomplete == CSESSINCMPL_CHECKS)
				THROW(csg.Grouping(sessID, &total, &dfct_subst_list, 0));
			THROW(csg.ConvertToBills(sessID, cn_rec.LocID, 0, 1, 1, 0));
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	PPWaitStop();
	return ok;
}

int PPObjCSession::UndoWritingOff(PPID sessID, int use_ta)
{
	int    ok = 1;
	CGoodsLine cgl;
	CSessTotal cs_total;
	{
		PPObjSecur::Exclusion ose(PPEXCLRT_CSESSWROFFROLLBACK);
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW(RemoveWrOffBills(sessID, 0));
		THROW(cgl.UndoWritingOff(sessID));
		THROW(cgl.CalcSessTotal(sessID, &cs_total));
		THROW_DB(updateFor(P_Tbl, 0, (P_Tbl->ID == sessID),
			set(P_Tbl->WrOffAmount, dbconst(0.0)).
			set(P_Tbl->AggrAmount, dbconst(cs_total.AggrAmount)).
			set(P_Tbl->AggrRest, dbconst(cs_total.AggrRest)).
			set(P_Tbl->Incomplete, dbconst((long)CSESSINCMPL_GLINES))));
		DS.LogAction(PPACN_UNDOCSESSWROFF, PPOBJ_CSESSION, sessID, 0, 0);
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int PPObjCSession::RemoveWrOffBills(PPID sessID, int use_ta)
{
	int    ok = 1;
	uint   i;
	PPID   bill_id = 0;
	BillTbl::Rec bill_rec;
	PPIDArray modif_bill_ids, ret_bill_ids, dfct_bill_list;
	PPObjBill * p_bobj = BillObj;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		for(bill_id = 0; p_bobj->P_Tbl->EnumMembersOfPool(PPASS_CSESSBILLPOOL, sessID, &bill_id) > 0;) {
			if(p_bobj->Search(bill_id, &bill_rec) > 0) {
				if(GetOpType(bill_rec.OpID) == PPOPT_GOODSMODIF) {
					THROW_SL(modif_bill_ids.add(bill_id));
				}
				else if(GetOpType(bill_rec.OpID) == PPOPT_GOODSRETURN) {
					THROW_SL(ret_bill_ids.add(bill_id));
				}
				else
					THROW(p_bobj->RemovePacket(bill_id, 0));
			}
		}
		for(bill_id = 0; p_bobj->P_Tbl->EnumMembersOfPool(PPASS_CSDBILLPOOL, sessID, &bill_id) > 0;)
			if(p_bobj->Search(bill_id, &bill_rec) > 0)
				THROW(p_bobj->RemovePacket(bill_id, 0));
		for(i = 0; i < modif_bill_ids.getCount(); i++)
			THROW(p_bobj->RemovePacket(modif_bill_ids.at(i), 0));
		for(i = 0; i < ret_bill_ids.getCount(); i++)
			THROW(p_bobj->RemovePacket(ret_bill_ids.at(i), 0));
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

/*virtual*/int PPObjCSession::RemoveObjV(PPID sessID, ObjCollection * pObjColl, uint options/* = rmv_default*/, void * pExtraParam)
{
	int    ok = 1;
	CGoodsLine cgl;
	PPIDArray sub_list;
	CSessionTbl::Rec rec;
	{
		PPTransaction tra(BIN(options & use_transaction));
		THROW(tra);
		THROW(Search(sessID, &rec) > 0);
		THROW_PP(rec.SuperSessID == 0, PPERR_CANTRMVSUBSESS);
		P_Tbl->GetSubSessList(sessID, &sub_list);
		sub_list.add(sessID);
		for(uint i = 0; i < sub_list.getCount(); i++) {
			PPID   sess_id = sub_list.get(i);
			THROW(RemoveWrOffBills(sess_id, 0)); // AHTOXA
			THROW(cgl.RemoveSess(sess_id));
			THROW(P_Cc->RemoveSess(sess_id, 0));
			THROW_DB(deleteFrom(P_Tbl, 0, P_Tbl->ID == sess_id));
			THROW(RemoveSync(sess_id));
		}
		DS.LogAction(PPACN_OBJRMV, Obj, sessID, 0, 0);
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int PPObjCSession::HandleMsg(int msg, PPID _obj, PPID _id, void * extraPtr)
{
	if(msg == DBMSG_OBJDELETE) {
		if(_obj == PPOBJ_CASHNODE) {
			CSessionTbl::Key1 k1;
			MEMSZERO(k1);
			k1.CashNodeID = _id;
			if(P_Tbl->search(1, &k1, spGe) && k1.CashNodeID == _id)
				return RetRefsExistsErr(Obj, P_Tbl->data.ID);
		}
	}
	return DBRPL_OK;
}

int PPObjCSession::EditRights(uint bufSize, ObjRights * rt, EmbedDialog * pDlg)
{
	class CSessRightsDlg : public TDialog {
	public:
		CSessRightsDlg() : TDialog(DLG_RTCSESS)
		{
		}
		virtual int TransmitData(int dir, void * pData)
		{
			int    s = 1;
			if(dir > 0)
				setDTS(static_cast<ObjRights *>(pData));
			else if(dir < 0)
				getDTS(static_cast<ObjRights *>(pData));
			else
				s = TDialog::TransmitData(dir, pData);
			return s;
		}
		int setDTS(const ObjRights * pData)
		{
			if(pData) {
				ushort comm_rt = pData->Flags;
				setCtrlData(CTL_RTCSESS_FLAGS,   &comm_rt);
				AddClusterAssoc(CTL_RTCSESS_SFLAGS,  0, CSESSRT_OPEN);
				AddClusterAssoc(CTL_RTCSESS_SFLAGS,  1, CSESSRT_CLOSE);
				AddClusterAssoc(CTL_RTCSESS_SFLAGS,  2, CSESSRT_ADDCHECK);
				AddClusterAssoc(CTL_RTCSESS_SFLAGS,  3, CSESSRT_RMVCHECK);
				AddClusterAssoc(CTL_RTCSESS_SFLAGS,  4, CSESSRT_CHECKINFO);
				AddClusterAssoc(CTL_RTCSESS_SFLAGS,  5, CSESSRT_ESCCHECK);
				AddClusterAssoc(CTL_RTCSESS_SFLAGS,  6, CSESSRT_SYSINFO);
				AddClusterAssoc(CTL_RTCSESS_SFLAGS,  7, CSESSRT_CORRECT);
				SetClusterData(CTL_RTCSESS_SFLAGS, pData->Flags);
				AddClusterAssoc(CTL_RTCSESS_SFLAGS2,  0, CSESSOPRT_RETCHECK);
				AddClusterAssoc(CTL_RTCSESS_SFLAGS2,  1, CSESSOPRT_BANKING);
				AddClusterAssoc(CTL_RTCSESS_SFLAGS2,  2, CSESSOPRT_ESCCLINE);
				AddClusterAssoc(CTL_RTCSESS_SFLAGS2,  3, CSESSOPRT_PREPRT);
				AddClusterAssoc(CTL_RTCSESS_SFLAGS2,  4, CSESSOPRT_SUSPCHECK);
				AddClusterAssoc(CTL_RTCSESS_SFLAGS2,  5, CSESSOPRT_COPYCHECK);
				AddClusterAssoc(CTL_RTCSESS_SFLAGS2,  6, CSESSOPRT_COPYZREPT);
				AddClusterAssoc(CTL_RTCSESS_SFLAGS2,  7, CSESSOPRT_ROWDISCOUNT);
				AddClusterAssoc(CTL_RTCSESS_SFLAGS2,  8, CSESSOPRT_XREP);
				AddClusterAssoc(CTL_RTCSESS_SFLAGS2,  9, CSESSOPRT_CTBLORD);
				AddClusterAssoc(CTL_RTCSESS_SFLAGS2, 10, CSESSOPRT_SPLITCHK);
				AddClusterAssoc(CTL_RTCSESS_SFLAGS2, 11, CSESSOPRT_MERGECHK);
				AddClusterAssoc(CTL_RTCSESS_SFLAGS2, 12, CSESSOPRT_CHGPRINTEDCHK);
				AddClusterAssoc(CTL_RTCSESS_SFLAGS2, 13, CSESSOPRT_RESTORESUSPWOA);
				SetClusterData(CTL_RTCSESS_SFLAGS2, pData->OprFlags);
				AddClusterAssoc(CTL_RTCSESS_SFLAGS3,  0, CSESSOPRT_CHGCCAGENT);
				AddClusterAssoc(CTL_RTCSESS_SFLAGS3,  1, CSESSOPRT_ESCCLINEBORD);
				AddClusterAssoc(CTL_RTCSESS_SFLAGS3,  2, CSESSOPRT_REPRNUNFCC);
				AddClusterAssoc(CTL_RTCSESS_SFLAGS3,  3, CSESSOPRT_ARBITRARYDISC); // @v11.0.9
				SetClusterData(CTL_RTCSESS_SFLAGS3, pData->OprFlags);
			}
			return 1;
		}
		int getDTS(ObjRights * pData)
		{
			if(pData) {
				ushort comm_rt = getCtrlUInt16(CTL_RTCSESS_FLAGS);
				ushort v = static_cast<ushort>(GetClusterData(CTL_RTCSESS_SFLAGS));
				pData->Flags = ((comm_rt & 0x00ff) | v);
				GetClusterData(CTL_RTCSESS_SFLAGS2, &pData->OprFlags);
				GetClusterData(CTL_RTCSESS_SFLAGS3, &pData->OprFlags);
			}
			return 1;
		}
	};
	int    r = 1;
	CSessRightsDlg * dlg = 0;
	THROW(CheckDialogPtr(&(dlg = new CSessRightsDlg())));
	if(pDlg)
		pDlg->Embed(dlg);
	else {
		THROW_PP(bufSize >= sizeof(ObjRights), PPERR_OBJRTBUFSIZ);
		dlg->setDTS(rt);
		if((r = ExecView(dlg)) == cmOK)
			dlg->getDTS(rt);
		else
			r = -1;
	}
	CATCH
		r = 0;
	ENDCATCH
	if(!pDlg)
		delete dlg;
	return r;
}

/*static*/int PPObjCSession::ValidateCcDate2MaxIdIndex(const LAssocArray & rIndex)
{
	int    ok = 1;
	uint   _i = 0;
	if(!rIndex.getCount())
		ok = -1;
	else {
		LDATE prev_dt = ZERODATE;
		long prev_max_id = 0;
		for(_i = 0; _i < rIndex.getCount(); _i++) {
			const LAssoc & r_entry = rIndex.at(_i);
			LDATE _dt;
			_dt.v = static_cast<ulong>(r_entry.Key);
			const long max_id = r_entry.Val;
			THROW_PP(checkdate(_dt), PPERR_INVALIDCCDATE2MAXIDINDEX);
			THROW_PP(_dt > prev_dt, PPERR_INVALIDCCDATE2MAXIDINDEX);
			THROW_PP(max_id >= prev_max_id, PPERR_INVALIDCCDATE2MAXIDINDEX);
			prev_dt = _dt;
			prev_max_id = max_id;
		}
	}
	CATCHZOK
	return ok;
}

static int StoreCcDate2MaxIdIndex(LAssocArray & rIndex)
{
	int    ok = 1;
	Reference * p_ref = PPRef;
	SBuffer cbuf;
	if(rIndex.getCount()) {
		SBuffer _sbuf;
		SSerializeContext sctx;
		THROW(PPObjCSession::ValidateCcDate2MaxIdIndex(rIndex));
		sctx.Serialize(+1, &rIndex, _sbuf);
		if(_sbuf.GetAvailableSize() > 128) {
			uint8 cs[32];
			size_t cs_size = SSerializeContext::GetCompressPrefix(cs);
			SCompressor compr(SCompressor::tZLib);
			THROW_SL(cbuf.Write(cs, cs_size));
			THROW_SL(compr.CompressBlock(_sbuf.GetBuf(0), _sbuf.GetAvailableSize(), cbuf, 0, 0));
		}
		else {
			cbuf = _sbuf;
		}
		THROW(p_ref->PutPropSBuffer(PPOBJ_CONFIG, PPCFG_MAIN, PPPRP_CCDATETOMAXIDINDEX, cbuf, 1));
	}
	else {
		THROW(p_ref->PutPropSBuffer(PPOBJ_CONFIG, PPCFG_MAIN, PPPRP_CCDATETOMAXIDINDEX, cbuf, 1));
		ok = -1;
	}
	CATCHZOK
	return ok;
}

static int RecallCcDate2MaxIdIndex(LAssocArray & rIndex)
{
	rIndex.clear();
	int    ok = 1;
	SBuffer cbuf;
	Reference * p_ref = PPRef;
	if(p_ref->GetPropSBuffer(PPOBJ_CONFIG, PPCFG_MAIN, PPPRP_CCDATETOMAXIDINDEX, cbuf) > 0) {
		SSerializeContext sctx;
		const size_t actual_size = cbuf.GetAvailableSize();
		const size_t cs_size = SSerializeContext::GetCompressPrefix(0);
		if(actual_size > cs_size && SSerializeContext::IsCompressPrefix(cbuf.GetBuf(cbuf.GetRdOffs()))) {
			SCompressor compr(SCompressor::tZLib);
			SBuffer dbuf;
			int  inflr = compr.DecompressBlock(cbuf.GetBuf(cbuf.GetRdOffs()+cs_size), actual_size-cs_size, dbuf);
			THROW_SL(inflr);
			THROW_SL(sctx.Serialize(-1, &rIndex, dbuf));
		}
		else {
			THROW_SL(sctx.Serialize(-1, &rIndex, cbuf));
		}
		THROW(PPObjCSession::ValidateCcDate2MaxIdIndex(rIndex));
	}
	else
		ok = -1;
	CATCH
		rIndex.clear();
		ok = 0;
	ENDCATCH
	return ok;
}

int PPObjCSession::BuildCcDate2MaxIdIndex(int mode)
{
	constexpr long max_date_diff = 3;
	int    ok = -1;
	//buildccdate2maxidindexMode_Force = 0,    // Безусловно перестроить индекс
	//buildccdate2maxidindexMode_SkipIfCached, // Ничего не делать если индекс уже кэширован
	//buildccdate2maxidindexMode_SkipIfActual, // Ничего не делать если последний элемент индекса не старше чем дата последнего чека минус 3 дня //
	bool do_rebuild = false;
	if(mode == buildccdate2maxidindexMode_Force)
		do_rebuild = true;
	else {
		if(mode == buildccdate2maxidindexMode_SkipIfCached && IsCcDate2MaxIdIndexLoaded()) {
			do_rebuild = false;
		}
		else {
			LAssocArray index;
			assert(oneof2(mode, buildccdate2maxidindexMode_SkipIfCached, buildccdate2maxidindexMode_SkipIfActual));
			THROW(RecallCcDate2MaxIdIndex(index));
			{
				LDATE last_date = ZERODATE;
				if(index.getCount()) {
					last_date.v = static_cast<ulong>(index.at(index.getCount()-1).Key);
				}
				if(!last_date)
					do_rebuild = true;
				else {
					CCheckTbl::Rec last_cc_rec;
					if(P_Cc->GetLastCheck(&last_cc_rec) > 0 && diffdate(last_cc_rec.Dt, last_date) > max_date_diff)
						do_rebuild = true;
				}
			}
		}
	}
	if(do_rebuild) {
		LAssocArray index;
		THROW(P_Cc->MakeDate2MaxIdIndex(index));
		THROW(StoreCcDate2MaxIdIndex(index));
		ok = 1;
	}
	CATCHZOK
	return ok;
}

int PPObjCSession::GetCcDate2MaxIdIndex(LAssocArray & rIndex)
{
	return RecallCcDate2MaxIdIndex(rIndex);
}

int PPObjCSession::NeedTransmit(PPID id, const DBDivPack & rDestDbDivPack, ObjTransmContext * pCtx)
{
	int    ok = 1;
	SString fmt_buf;
	CSessionTbl::Rec rec;
	if(Search(id, &rec) > 0) {
		if(rec.Temporary) {
			// Кассовая сессия '%s' не передана поскольку является временной
			PPLoadText(PPTXT_LOG_NTRANS_CSESTEMP, fmt_buf);
			ok = -1;
		}
		else if(rec.CashNodeID) {
			PPObjCashNode cn_obj;
			PPCashNode cn_rec;
			if(cn_obj.Search(rec.CashNodeID, &cn_rec) > 0) {
				if(cn_rec.CurSessID == id) {
					// Кассовая сессия '%s' не передана поскольку не закрыта
					PPLoadText(PPTXT_LOG_NTRANS_CSESNCL, fmt_buf);
					ok = -1;
				}
			}
			else {
				// Кассовая сессия '%s' не передана поскольку не найден узел, которому она принадлежит
				PPLoadText(PPTXT_LOG_NTRANS_CSESNFN, fmt_buf);
				ok = -1;
			}
		}
		if(fmt_buf.NotEmpty()) {
			if(pCtx) {
				SString msg_buf, sess_buf;
				PPObjCSession::MakeCodeString(&rec, sess_buf);
				pCtx->Output(msg_buf.Printf(fmt_buf, sess_buf.cptr()));
			}
		}
	}
	else
		ok = -1;
	return ok;
}
/*
	Формат представления чеков в потоке передачи данных между разделами

	Header:
		int8   HdrByte;    // 0x01 - Discount != 0; 0x02 - SCardID != 0
		int16  ItemsCount;
		long   CashID;
		date   Dt;
		time   Tm;
		long   Flags;      // CCHKF_XXX
		money  Amount[8];
		money  Discount[8]; // if HdrByte & 0x01
		long   SCardID;     // if HdrByte & 0x02

	Items: // Size <= 30
		int8   HdrByte;      // 0x01 - Quantity != 1; 0x02 - Discount != 0
		uint16 GoodsIdx;     // Индекс в таблице идентификаторов товаров
		money  Price[8];     // Цена
		double Quantity;     // Количество товара if !(HdrByte & 0x01) then implicit Quantity == 1
		money  Discount[8];  // if HdrByte & 0x02

	@todo В некоторых случаях uint16 для индекса в таблице товаров может быть мало.
	  Следует предусмотреть дополнительный бит в HdrByte для индикации использования uint32-индекса
*/

struct CChkTransm {
	int8   HdrByte;    // 0x01 - Discount != 0; 0x02 - SCardID != 0; 0x04 - UserIdx != 0;
		// 0x08 - SalerID != 0, 0x10 - TableNo != 0, 0x20 - AddPaym
	int16  ItemsCount;
	long   CashID;
	long   Code;
	LDATE  Dt;
	LTIME  Tm;
	long   Flags;      // CCHKF_XXX
	char   Amount[8];
	//char   Discount[8]; // if HdrByte & 0x01
	//uint16 SCardIdx;    // if HdrByte & 0x02
	//uint16 CashierIdx;  // if HdrByte & 0x04 @v5.4.4
	//uint16 SalerIdx;    // if HdrByte & 0x08 @v5.4.4
	//uint16 TableNo;     // if HdrByte & 0x10 @v5.4.4
};

struct CChkItemTransm {
	int8   HdrByte;      // 0x01 - Quantity != 1; 0x02 - Discount != 0; 0x04 - Serial
	uint16 GoodsIdx;     // Индекс в таблице идентификаторов товаров
	long   Price;        // Цена
	//double Quantity;     // Количество товара if !(ItmByte & 0x01) then implicit Quantity == 1
	//long   Discount;     // if ItmByte & 0x02
	//char   Serial[]      // Серийный номер
};

class CSessTransmitPacket {
public:
	CSessTransmitPacket() : UseCclExt(BIN(CConfig.Flags & CCFLG_USECCHECKLINEEXT)), Method_700(1), Valid(1), ChecksCount(0)
	{
	}
	int    IsValid() const { return Valid; }
	int    LoadSession(PPID sessID, ObjTransmContext * pCtx);
	int    ProcessRefs(PPObjIDArray *, int replace, ObjTransmContext * pCtx);
	int    PutToStream(FILE * fStream);
	int    GetFromStream(FILE * fStream, ObjTransmContext * pCtx);
	int    Restore(PPID * pID, ObjTransmContext * pCtx);
	CSessionTbl::Rec Rec;
	uint32  ChecksCount; // @!CSessTransmitPacket::LoadSession
private:
	PPObjCSession SessObj;
	PPIDArray ChildIdList; // Подчиненные сессии (используется только для //
		// форсирования передачи этих сессий вместе с суперсессией:
		// 1. Функция LoadSession загружает этот список из БД
		// 2. Функция ProcessRefs передает модулю обмена данными этот список
		// 3. Модуль обмена данными форсирует передачу подчиненных сессий
		//
	SBuffer Bs;
	int    Valid;
	const  int UseCclExt;
	const  int Method_700;
};

int CSessTransmitPacket::Restore(PPID * pID, ObjTransmContext * pCtx)
{
	int    ok = -1;
	uint   i;
	PPID   sess_id = 0;
	SString msg_tmpl_buf, msg_buf, err_msg_fmt, err_msg_text, ccheck_code;
	if(*pID == 0) {
		PPUserFuncProfiler ufp(PPUPRF_CSESSTXMRESTORE);
		PPLoadText(PPTXT_RESTORECSESS, msg_tmpl_buf);
		PPIniFile ini_file;
		int    accept_dup_time = 0;
		ini_file.GetInt(PPINISECT_CONFIG, PPINIPARAM_ACCEPTDUPTIMECHECK, &accept_dup_time);
		{
			PPTransaction tra(1);
			THROW(tra);
			Bs.SetRdOffs(0);
			THROW_SL(SessObj.P_Tbl->SerializeRecord(-1, &Rec, Bs, &pCtx->SCtx));
			THROW_SL(pCtx->SCtx.Serialize(-1, ChecksCount, Bs));
			Rec.ID = 0;
			if(!Rec.SuperSessID)
				Rec.Incomplete = CSESSINCMPL_CHECKS;
			//
			// Очень редко, но бывает, что суперсессия не принимается из другого раздела по
			// причине того, что суперсессия по другому разделу имеет ту же временную метку.
			//
			if(Rec.SuperSessID == 0 && Rec.CashNumber == 0)
				SessObj.P_Tbl->CheckUniqueDateTime(Rec.SuperSessID, Rec.CashNumber, &Rec.Dt, &Rec.Tm);
			//
			THROW(AddObjRecByID(SessObj.P_Tbl, SessObj.Obj, &sess_id, &Rec, 0));
			for(i = 0; i < ChecksCount; i++) {
				CCheckPacket pack;
				THROW(SessObj.P_Cc->SerializePacket(-1, &pack, Bs, &pCtx->SCtx));
				pack.Rec.ID = 0;
				pack.Rec.SessID = sess_id;
				pack.Rec.Flags |= CCHKF_NOTUSED;
				if(accept_dup_time) {
					CCheckTbl::Rec rec;
					while(SessObj.P_Cc->Search(pack.Rec.PosNodeID, pack.Rec.Dt, pack.Rec.Tm, &rec) > 0) {
						if(pack.Rec.Code != rec.Code)
							pack.Rec.Tm.addhs(10);
						else
							break;
					}
				}
				pack.UpdFlags |= CCheckPacket::ufCheckInvariant;
				pack.UpdFlags |= CCheckPacket::ufSkipScSpcTrt;
				if(!SessObj.P_Cc->TurnCheck(&pack, 0)) {
					PPLoadText(PPTXT_LOG_ERRACCEPTCCHECK, err_msg_fmt);
					CCheckCore::MakeCodeString(&pack.Rec, CCheckCore::mcsID, ccheck_code);
					PPGetLastErrorMessage(1, err_msg_text);
					msg_buf.Printf(err_msg_fmt, ccheck_code.cptr(), err_msg_text.cptr());
					pCtx->Output(msg_buf);
				}
				PPWaitMsg(msg_buf.Printf(msg_tmpl_buf, (long)i+1, (long)ChecksCount));
			}
			THROW(SessObj.Recalc(sess_id, 0));
			if(Rec.SuperSessID && SessObj.Search(Rec.SuperSessID) > 0)
				THROW(SessObj.Recalc(Rec.SuperSessID, 0));
			THROW(tra.Commit());
		}
		ufp.SetFactor(0, static_cast<double>(ChecksCount));
		ufp.Commit();
		ASSIGN_PTR(pID, sess_id);
		ok = 1;
	}
	else if(pCtx->Flags & ObjTransmContext::fRecover) {
		CSessionTbl::Rec cs_rec;
		if(SessObj.Search(*pID, &cs_rec) > 0) {
			PPTransaction tra(1);
			THROW(tra);
			Bs.SetRdOffs(0);
			THROW_SL(SessObj.P_Tbl->SerializeRecord(-1, &Rec, Bs, &pCtx->SCtx));
			THROW_SL(pCtx->SCtx.Serialize(-1, ChecksCount, Bs));
			for(i = 0; i < ChecksCount; i++) {
				CCheckPacket pack;
				THROW(SessObj.P_Cc->SerializePacket(-1, &pack, Bs, &pCtx->SCtx));
				{
					const CcAmountList & r_al = pack.AL_Const();
					if(r_al.getCount()) {
						int    do_recover = 0;
						for(uint j = 0; !do_recover && j < r_al.getCount(); j++) {
							if(r_al.at(j).AddedID)
								do_recover = 1;
						}
						if(do_recover) {
							CCheckCore * p_cc = SessObj.P_Cc;
							CCheckTbl::Key3 k3;
							MEMSZERO(k3);
							k3.SessID = *pID;
							k3.PosNodeID = pack.Rec.PosNodeID;
							k3.Dt = pack.Rec.Dt;
							k3.Tm = pack.Rec.Tm;
							PPID   cc_id = 0;
							if(p_cc->search(3, &k3, spGe)) do {
								if(labs(DiffTime(p_cc->data.Tm, pack.Rec.Tm, 3)) < 10 && p_cc->data.Code == pack.Rec.Code) {
									cc_id = p_cc->data.ID;
								}
							} while(!cc_id && p_cc->search(3, &k3, spGt));
							if(cc_id) {
								const double epsilon = 1E-6;
								CCheckPacket cc_pack;
								if(p_cc->LoadPacket(cc_id, 0, &cc_pack) > 0) {
									SCardTbl::Rec sc_rec;
									CCheckPaymTbl::Key0 cpk0;
									MEMSZERO(cpk0);
									cpk0.CheckID = cc_id;
									if(p_cc->PaymT.search(0, &cpk0, spGe) && p_cc->PaymT.data.CheckID == cc_id) {
										uint k = 0;
										do {
											CcAmountEntry cc_ae;
											const int16 rbc = p_cc->PaymT.data.RByCheck;
											cc_ae.Type = p_cc->PaymT.data.PaymType;
											cc_ae.Amount = intmnytodbl(p_cc->PaymT.data.Amount);
											cc_ae.AddedID = p_cc->PaymT.data.SCardID;
											if(cc_ae.AddedID && k < r_al.getCount()) {
												const CcAmountEntry & r_ae = r_al.at(k);
												if(r_ae.Type == cc_ae.Type && fabs(r_ae.Amount - cc_ae.Amount) < epsilon && r_ae.AddedID != cc_ae.AddedID) {
													if(p_cc->Cards.Search(r_ae.AddedID, &sc_rec) > 0) {
														cc_ae.AddedID = r_ae.AddedID;
														{
															p_cc->PaymT.data.SCardID = r_ae.AddedID;
															THROW_DB(p_cc->PaymT.updateRec());
															//
															// PPTXT_CCPAYMSCARDCORRECTED  "Скорректирована карта оплаты в чеке %s"
															PPLoadText(PPTXT_CCPAYMSCARDCORRECTED, err_msg_fmt);
															CCheckCore::MakeCodeString(&pack.Rec, CCheckCore::mcsID, ccheck_code);
															pCtx->Output(msg_buf.Printf(err_msg_fmt, ccheck_code.cptr()));
														}
													}
												}
											}
											k++;
										} while(p_cc->PaymT.search(0, &cpk0, spNext) && p_cc->PaymT.data.CheckID == cc_id);
									}
								}
							}
							else {
								pack.UpdFlags |= (CCheckPacket::ufCheckInvariant|CCheckPacket::ufSkipScSpcTrt);
								CCheckCore::MakeCodeString(&pack.Rec, CCheckCore::mcsID, ccheck_code);
								if(!p_cc->TurnCheck(&pack, 0)) {
									PPLoadText(PPTXT_LOG_ERRACCEPTCCHECK, err_msg_fmt);
									PPGetLastErrorMessage(1, err_msg_text);
									msg_buf.Printf(err_msg_fmt, ccheck_code.cptr(), err_msg_text.cptr());
								}
								else {
									PPLoadText(PPTXT_CCABSRESTORED, err_msg_fmt);
									msg_buf.Printf(err_msg_fmt, ccheck_code.cptr());
								}
								pCtx->Output(msg_buf);
							}
						}
					}
				}
			}
			THROW(tra.Commit());
		}
	}
	CATCH
		pCtx->OutputLastError();
		ok = 0;
	ENDCATCH
	return ok;
}

int CSessTransmitPacket::ProcessRefs(PPObjIDArray * ary, int replace, ObjTransmContext * pCtx)
{
	int    ok = 1;
	SBuffer temp_buf;
	CCheckCore * p_cc = SessObj.P_Cc;
	THROW(PPObject::ProcessObjListRefInArray(PPOBJ_CSESSION, ChildIdList, ary, replace));
	PROFILE_START
	Bs.SetRdOffs(0);
	THROW_SL(SessObj.P_Tbl->SerializeRecord(-1, &Rec, Bs, &pCtx->SCtx));
	THROW_SL(pCtx->SCtx.Serialize(-1, ChecksCount, Bs));
	THROW(PPObject::ProcessObjRefInArray(PPOBJ_CSESSION, &Rec.SuperSessID, ary, replace));
	THROW(PPObject::ProcessObjRefInArray(PPOBJ_CASHNODE, &Rec.CashNodeID,  ary, replace));
	if(replace) {
		THROW_SL(SessObj.P_Tbl->SerializeRecord(+1, &Rec, temp_buf, &pCtx->SCtx));
		THROW_SL(pCtx->SCtx.Serialize(+1, ChecksCount, temp_buf));
	}
	for(uint i = 0; i < ChecksCount; i++) {
		CCheckPacket pack;
		THROW(p_cc->SerializePacket(-1, &pack, Bs, &pCtx->SCtx));
		THROW(PPObject::ProcessObjRefInArray(PPOBJ_SCARD,   &pack.Rec.SCardID, ary, replace));
		THROW(PPObject::ProcessObjRefInArray(PPOBJ_PERSON,  &pack.Rec.UserID, ary, replace));
		THROW(PPObject::ProcessObjRefInArray(PPOBJ_ARTICLE, &pack.Ext.SalerID,  ary, replace));
		THROW(PPObject::ProcessObjRefInArray(PPOBJ_LOCATION, &pack.Ext.AddrID,  ary, replace));
		for(uint j = 0; j < pack.Items_.getCount(); j++) {
			CCheckLineTbl::Rec & r_line = pack.Items_.at(j);
			THROW(PPObject::ProcessObjRefInArray(PPOBJ_GOODS, &r_line.GoodsID, ary, replace));
		}
		for(uint k = 0; k < pack.AL().getCount(); k++) {
			CcAmountEntry & r_al_entry = pack.AL().at(k);
			if(r_al_entry.Type == CCAMTTYP_CRDCARD) {
				THROW(PPObject::ProcessObjRefInArray(PPOBJ_SCARD, &r_al_entry.AddedID, ary, replace));
				THROW(PPObject::ProcessObjRefInArray(PPOBJ_CURRENCY, &r_al_entry.CurID, ary, replace));
			}
		}
		if(replace) {
			THROW(p_cc->SerializePacket(+1, &pack, temp_buf, &pCtx->SCtx));
		}
	}
	if(replace)
		Bs = temp_buf;
	PROFILE_END
	CATCHZOK
	return ok;
}
//
// Извлекает сессию из потока передачи данных
//
int CSessTransmitPacket::GetFromStream(FILE * stream, ObjTransmContext * pCtx)
{
	int    ok = 1;
	Bs.Z();
	THROW_SL(Bs.ReadFromFile(stream, 0))
	CATCHZOK
	return ok;
}
//
// Сохраняет сессию в потоке передачи данных
//
int CSessTransmitPacket::PutToStream(FILE * stream)
{
	int    ok = 1;
	Bs.SetRdOffs(0);
	THROW_SL(Bs.WriteToFile(stream, 0, 0))
	CATCHZOK
	return ok;
}
//
// Извлекает сессию из базы данных
//
int CSessTransmitPacket::LoadSession(PPID sessID, ObjTransmContext * pCtx)
{
	int    ok = 1;
	ChecksCount = 0;
	Bs.Z();
	if(SessObj.Search(sessID, &Rec) > 0) {
		PPIDArray id_list;
		CCheckCore * p_cc = SessObj.P_Cc;
		CCheckTbl::Key3 k3;
		MEMSZERO(k3);
		k3.SessID = sessID;
		while(p_cc->search(3, &k3, spGt) && k3.SessID == sessID) {
			id_list.add(p_cc->data.ID);
		}
		ChecksCount = id_list.getCount();
		THROW_SL(SessObj.P_Tbl->SerializeRecord(+1, &Rec, Bs, &pCtx->SCtx));
		THROW_SL(pCtx->SCtx.Serialize(+1, ChecksCount, Bs));
		for(uint i = 0; i < id_list.getCount(); i++) {
			CCheckPacket cc_pack;
			THROW(p_cc->LoadPacket(id_list.get(i), 0, &cc_pack) > 0);
			THROW(p_cc->SerializePacket(+1, &cc_pack, Bs, &pCtx->SCtx));
		}
		SessObj.P_Tbl->GetSubSessList(sessID, &ChildIdList);
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

int PPObjCSession::Read(PPObjPack * p, PPID id, void * stream, ObjTransmContext * pCtx)
{
	int    ok = 1;
	CSessTransmitPacket * p_pack = new CSessTransmitPacket;
	THROW_MEM(p_pack);
	THROW(p_pack->IsValid());
	if(stream == 0) {
		THROW(p_pack->LoadSession(id, pCtx) > 0);
	}
	else {
		THROW(p_pack->GetFromStream(static_cast<FILE *>(stream), pCtx));
	}
	p->Data = p_pack;
	CATCHZOK
	return ok;
}

int PPObjCSession::Write(PPObjPack * p, PPID * pID, void * stream, ObjTransmContext * pCtx)
{
	int    ok = 1;
	CSessTransmitPacket * p_pack = p ? static_cast<CSessTransmitPacket *>(p->Data) : 0;
	if(p_pack)
		if(stream == 0) {
			if(!p_pack->Restore(pID, pCtx))
				ok = -1;
		}
		else {
			THROW(p_pack->PutToStream(static_cast<FILE *>(stream)));
		}
	CATCHZOK
	return ok;
}

IMPL_DESTROY_OBJ_PACK(PPObjCSession, CSessTransmitPacket);

int PPObjCSession::ProcessObjRefs(PPObjPack * p, PPObjIDArray * ary, int replace, ObjTransmContext * pCtx)
{
	return (p && p->Data) ? static_cast<CSessTransmitPacket *>(p->Data)->ProcessRefs(ary, replace, pCtx) : -1;
}
//
//  @VADIM Регламентация операционных прав кассира с помощью клавиатуры с ключом
//
struct _RightNKeyPosEntry { // @persistent @size=8
	long   OperRightFlag; // CSESSRT_XXX || CSESSOPRT_XXX (к счастью, они не пересекаются)
	long   KeyPos;
};

struct _PPKeybordWKeyCfg { // @persistent @store(PropertyTbl) @size=84
	PPID   Tag;            // Const=PPOBJ_CONFIG
	PPID   ID;             // Const=PPCFG_MAIN
	PPID   Prop;           // Const=PPPRP_KEYBWKEYCFG
	_RightNKeyPosEntry OperRights[14]; // Соответствие операционных прав положениям ключа
};

int GetOperRightsByKeyPos(int keyPos, PPIDArray * pOperRightsAry)
{
	int    ok = -1;
	if(pOperRightsAry) {
		_PPKeybordWKeyCfg  kwk_cfg;
		pOperRightsAry->freeAll();
		if(keyPos > 0 && PPRef->GetPropMainConfig(PPPRP_KEYBWKEYCFG, &kwk_cfg, sizeof(kwk_cfg)) > 0) {
			for(int i = 0; i < SIZEOFARRAY(kwk_cfg.OperRights); i++) {
				SBitArray  key_pos_ary;
				key_pos_ary.Init(&kwk_cfg.OperRights[i].KeyPos, 32);
				if(key_pos_ary.get(keyPos - 1) > 0) {
					long rt = kwk_cfg.OperRights[i].OperRightFlag;
					pOperRightsAry->add(rt);
				}
			}
			ok = 1;
		}
	}
	return ok;
}

int EditDueToKeyboardRights()
{
	class KeybWKeyCfgDlg : public TDialog {
		DECL_DIALOG_DATA(_PPKeybordWKeyCfg);
	public:
		KeybWKeyCfgDlg() : TDialog(DLG_CFGKBDWKEY)
		{
		}
		DECL_DIALOG_SETDTS()
		{
			if(!RVALUEPTR(Data, pData))
				MEMSZERO(Data);
			SString  key_pos;
			SString temp_buf;
			for(int i = 0; i < SIZEOFARRAY(Data.OperRights); i++) {
				StringSet key_pos_str(',', 0);
				SBitArray  key_pos_ary;
				key_pos_ary.Init(&Data.OperRights[i].KeyPos, 32);
				for(size_t pos = 0; pos < 32; pos++)
					if(key_pos_ary.get(pos))
						key_pos_str.add(key_pos.Z().Cat((long)(pos + 1)));
				temp_buf = key_pos_str.getBuf();
				setCtrlString(CTL_CFGKBDWKEY1 + i, temp_buf);
			}
			return 1;
		}
		DECL_DIALOG_GETDTS()
		{
			int    ok = 1;
			uint   sel = 0;
			for(int i = 0; i < SIZEOFARRAY(Data.OperRights); i++) {
				long   l_init = 0;
				char   buf[64];
				SString  key_pos;
				SBitArray key_pos_ary;
				key_pos_ary.Init(&l_init, 32);
				getCtrlData(sel = CTL_CFGKBDWKEY1 + i, buf);
				StringSet  key_pos_str(',', buf);
				for(uint pos = 0; key_pos_str.get(&pos, key_pos);) {
					long   l_key_pos;
					THROW_PP((l_key_pos = key_pos.ToLong()) > 0 && l_key_pos < 33, PPERR_USERINPUT);
					key_pos_ary.set((size_t)(l_key_pos - 1), 1);
				}
				key_pos_ary.getBuf(&Data.OperRights[i].KeyPos, sizeof(long));
			}
			Data.OperRights[0].OperRightFlag = CSESSOPRT_RETCHECK;
			Data.OperRights[1].OperRightFlag = CSESSOPRT_ESCCLINE;
			Data.OperRights[2].OperRightFlag = CSESSOPRT_BANKING;
			Data.OperRights[3].OperRightFlag = CSESSRT_CLOSE;
			Data.OperRights[4].OperRightFlag = CSESSRT_ESCCHECK;
			Data.OperRights[5].OperRightFlag = CSESSOPRT_COPYCHECK;
			Data.OperRights[6].OperRightFlag = CSESSOPRT_COPYZREPT;
			Data.OperRights[7].OperRightFlag = CSESSOPRT_ROWDISCOUNT;
			Data.OperRights[8].OperRightFlag = (CSESSOPRT_XREP << 16);   // CSESSOPRT_XREP пересекается с CSESSRT_CLOSE
			Data.OperRights[9].OperRightFlag = CSESSOPRT_SPLITCHK;
			Data.OperRights[10].OperRightFlag = CSESSOPRT_MERGECHK;
			Data.OperRights[11].OperRightFlag = CSESSOPRT_CHGPRINTEDCHK;
			Data.OperRights[12].OperRightFlag = CSESSOPRT_CHGCCAGENT;
			Data.OperRights[13].OperRightFlag = CSESSOPRT_ESCCLINEBORD;
			ASSIGN_PTR(pData, Data);
			CATCHZOKPPERRBYDLG
			return ok;
		}
	};
	int    ok = -1;
	int    valid_data = 0;
	_PPKeybordWKeyCfg  kwk_cfg;
	KeybWKeyCfgDlg * p_dlg = 0;
	THROW(CheckCfgRights(PPCFGOBJ_KEYBWKEYCFG, PPR_READ, 0));
	THROW(CheckDialogPtr(&(p_dlg = new KeybWKeyCfgDlg())));
	if(PPRef->GetPropMainConfig(PPPRP_KEYBWKEYCFG, &kwk_cfg, sizeof(kwk_cfg)) <= 0)
		MEMSZERO(kwk_cfg);
	p_dlg->setDTS(&kwk_cfg);
	while(!valid_data && ExecView(p_dlg) == cmOK) {
		if(p_dlg->getDTS(&kwk_cfg)) {
			int  is_key_pos = 0;
			for(int i = 0; i < SIZEOFARRAY(kwk_cfg.OperRights); i++)
				if(kwk_cfg.OperRights[i].KeyPos) {
					is_key_pos = 1;
					break;
				}
			THROW(PPRef->PutProp(PPOBJ_CONFIG, PPCFG_MAIN, PPPRP_KEYBWKEYCFG, is_key_pos ? &kwk_cfg : 0, sizeof(kwk_cfg), 1));
			ok = valid_data = 1;
		}
	}
	CATCHZOKPPERR
	delete p_dlg;
	return ok;
}
//
//
//
CTableOrder::Packet::Packet() : SCardID(0)
{
	Init(0, ZERODATETIME, 0);
}

void CTableOrder::Packet::Init(PPID posNodeID, LDATETIME initDtm, long initDuration)
{
	PosNodeID = posNodeID;
	ChkID = 0;
	CcNo = 0;
	CcDtm.Z();
	TableNo = 0;
	Status = 0;
	PrepayAmount = 0.0;
	Memo.Z();
	if(checkdate(initDtm.d))
		Chunk.Init(initDtm, initDuration);
	else {
		LDATETIME dtm = getcurdatetime_();
		dtm.t = encodetime(dtm.t.hour()+1, 0, 0, 0);
		Chunk.Init(dtm, (initDuration > 0) ? initDuration : (60*60));
	}
}
//
//
//
CTableOrder::Param::Param() : Ver(1), PosNodeID(0), StartTime(ZERODATETIME), InitDuration(0), Flags(0), TableNo(0)
{
	MEMSZERO(Reserve);
}

int CTableOrder::Param::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx)
{
	int    ok = 1;
	THROW_SL(pCtx->Serialize(dir, Ver, rBuf));
	THROW_SL(pCtx->Serialize(dir, PosNodeID, rBuf));
	THROW_SL(pCtx->Serialize(dir, StartTime, rBuf));
	THROW_SL(pCtx->Serialize(dir, InitDuration, rBuf));
	THROW_SL(pCtx->Serialize(dir, Flags, rBuf));
	THROW_SL(pCtx->Serialize(dir, TableNo, rBuf));
	THROW_SL(pCtx->SerializeBlock(dir, sizeof(Reserve), Reserve, rBuf, 1));
	CATCHZOK
	return ok;
}

CTableOrder::CTableOrder() : P_Grid(0), State(stRtUndef), P_ScObj(new PPObjSCard), P_CnObj(new PPObjCashNode)
{
}

CTableOrder::~CTableOrder()
{
	delete P_ScObj;
	delete P_CnObj;
	delete P_Grid;
}

int CTableOrder::HasRight(long rt)
{
	if(State & stRtUndef) {
		PPObjCSession cs_obj;
		if(cs_obj.CheckRights(CSESSOPRT_CTBLORD, 1))
			State |= stRtAll;
		State &= ~stRtUndef;
	}
	int ok = oneof3(rt, PPR_INS, PPR_MOD, PPR_DEL) ? BIN(State & stRtAll) : 0;
	if(!ok)
		PPSetError(PPERR_NORIGHTS);
	return ok;
}

int CTableOrder::EditParam(Param * pParam)
{
	int    ok = -1;
	Param  param;
	PPObjCashNode::SelFilt sf;
	sf.SyncGroup = 1;
	TDialog * dlg = new TDialog(DLG_CTBLORDPAR);
	THROW(CheckDialogPtr(&dlg));
	RVALUEPTR(param, pParam);
	SetupPPObjCombo(dlg, CTLSEL_CTBLORD_POSNODE, PPOBJ_CASHNODE, param.PosNodeID, 0, &sf);
	dlg->setCtrlTime(CTL_CTBLORD_STTM, param.StartTime.t);
	dlg->setCtrlLong(CTL_CTBLORD_CONT, param.InitDuration / 60);
	dlg->AddClusterAssoc(CTL_CTBLORD_FLAGS, 0, Param::fShowTimeGraph);
	dlg->SetClusterData(CTL_CTBLORD_FLAGS, param.Flags);
	while(ok < 0 && ExecView(dlg) == cmOK) {
		dlg->getCtrlData(CTLSEL_CTBLORD_POSNODE, &param.PosNodeID);
		param.StartTime.d = ZERODATE;
		dlg->getCtrlData(CTL_CTBLORD_STTM, &param.StartTime.t);
		dlg->GetClusterData(CTL_CTBLORD_FLAGS, &param.Flags);
		param.InitDuration = dlg->getCtrlLong(CTL_CTBLORD_CONT) * 60;
		ASSIGN_PTR(pParam, param);
		ok = 1;
	}
	CATCHZOK
	delete dlg;
	return ok;
}

class CTableOrderDialog : public TDialog {
	DECL_DIALOG_DATA(CTableOrder::Packet);
public:
	CTableOrderDialog(CTableOrder * pTo) : TDialog(DLG_CTBLORD), P_To(pTo)
	{
		SetupCalDate(CTLCAL_CTBLORD_STDT, CTL_CTBLORD_STDT);
		SetupCalDate(CTLCAL_CTBLORD_FNDT, CTL_CTBLORD_FNDT);
		SetupTimePicker(this, CTL_CTBLORD_STTM, CTLTM_CTBLORD_STTM);
		SetupTimePicker(this, CTL_CTBLORD_FNTM, CTLTM_CTBLORD_FNTM);
		enableCommand(cmCreateSCard, !Data.SCardID && ScObj.CheckRights(PPR_INS) && ScObj.GetConfig().DefCreditSerID);
	}
	DECL_DIALOG_SETDTS()
	{
		int    ok = 1;
		RVALUEPTR(Data, pData);
		PPObjCashNode::SelFilt sf;
		sf.SyncGroup = 1;
		SetupPPObjCombo(this, CTLSEL_CTBLORD_POSNODE, PPOBJ_CASHNODE, Data.PosNodeID, 0, &sf);
		setCtrlData(CTL_CTBLORD_TBL, &Data.TableNo);
		setCtrlData(CTL_CTBLORD_CCN, &Data.CcNo);
		setCtrlDatetime(CTL_CTBLORD_CCDT, CTL_CTBLORD_CCTM, Data.CcDtm);
		if(Data.SCardID) {
			SCardTbl::Rec sc_rec;
			if(ScObj.Search(Data.SCardID, &sc_rec) > 0) {
				setCtrlData(CTL_CTBLORD_SCARD, sc_rec.Code);
			}
			else
				Data.SCardID = 0;
		}
		setCtrlReal(CTL_CTBLORD_PREPAY, Data.PrepayAmount);
		setCtrlDatetime(CTL_CTBLORD_STDT, CTL_CTBLORD_STTM, Data.Chunk.Start);
		setCtrlDatetime(CTL_CTBLORD_FNDT, CTL_CTBLORD_FNTM, Data.Chunk.Finish);
		long   cont = Data.Chunk.GetDuration();
		setCtrlLong(CTL_CTBLORD_CONT, cont / 60);
		setCtrlString(CTL_CTBLORD_MEMO, Data.Memo);
		enableCommand(cmCreateSCard, !Data.SCardID && ScObj.CheckRights(PPR_INS) && ScObj.GetConfig().DefCreditSerID);
		disableCtrls((Data.ChkID != 0), CTLSEL_CTBLORD_POSNODE, CTL_CTBLORD_CCN, CTL_CTBLORD_SCARD,
			CTL_CTBLORD_PREPAY, 0);
		return ok;
	}
	DECL_DIALOG_GETDTS()
	{
		int    ok = 1;
		uint   sel = 0;
		getCtrlData(sel = CTLSEL_CTBLORD_POSNODE, &Data.PosNodeID);
		THROW_PP(Data.PosNodeID, PPERR_CASHNODENEEDED);
		getCtrlData(sel = CTL_CTBLORD_TBL, &Data.TableNo);
		THROW_PP(Data.TableNo > 0, PPERR_CTBLNEEDED);
		getCtrlDatetime(CTL_CTBLORD_STDT, CTL_CTBLORD_STTM, Data.Chunk.Start);
		getCtrlDatetime(CTL_CTBLORD_FNDT, CTL_CTBLORD_FNTM, Data.Chunk.Finish);
		getCtrlData(sel = CTL_CTBLORD_PREPAY, &Data.PrepayAmount);
		THROW_PP(Data.PrepayAmount >= 0.0, PPERR_INVAMOUNT);
		THROW_PP(Data.PrepayAmount == 0.0 || Data.SCardID, PPERR_CCORDPREPAYREQSCARD);
		getCtrlString(CTL_CTBLORD_MEMO, Data.Memo);
		ASSIGN_PTR(pData, Data);
		CATCHZOKPPERRBYDLG
		return ok;
	}
private:
	DECL_HANDLE_EVENT
	{
		TDialog::handleEvent(event);
		if(event.isCmd(cmCreateSCard)) {
			PPSCardConfig sc_cfg;
			PPObjSCardSeries scs_obj;
			PPSCardSeries scs_rec;
			PPObjPerson psn_obj;
			PPObjPerson::EditBlock peb;
			ScObj.FetchConfig(&sc_cfg);
			if(scs_obj.Fetch(sc_cfg.DefCreditSerID, &scs_rec) > 0) {
				psn_obj.InitEditBlock(NZOR(scs_rec.PersonKindID, NZOR(sc_cfg.PersonKindID, PPPRK_CLIENT)), peb);
				peb.SCardSeriesID = sc_cfg.DefCreditSerID;
				peb.ShortDialog = 1;
				PPID   psn_id = 0;
				if(psn_obj.Edit_(&psn_id, peb) == cmOK)
					SetupSCard(peb.RetSCardID, 0);
			}
		}
		else if(event.isCmd(cmPrint)) {
			CALLPTRMEMB(P_To, Print(&Data));
		}
		else if(event.isCmd(cmInputUpdated)) {
			static int __lock = 0;
			if(!__lock) {
				__lock = 1;
				int    do_ret = 0;
				if(event.isCtlEvent(CTL_CTBLORD_SCARD)) {
					SString temp_buf;
					getCtrlString(CTL_CTBLORD_SCARD, temp_buf);
					SetupSCard(0, temp_buf);
				}
				else if(event.isCtlEvent(CTL_CTBLORD_STDT)) {
					const LDATE dt = getCtrlDate(CTL_CTBLORD_STDT);
					if(checkdate(dt)) {
						Data.Chunk.Start.d = dt;
						if(!Data.Chunk.Finish.d || diffdate(Data.Chunk.Finish.d, dt) != 1) {
							Data.Chunk.Finish.d = dt;
							setCtrlDate(CTL_CTBLORD_FNDT, dt);
						}
					}
				}
				else if(event.isCtlEvent(CTL_CTBLORD_FNDT)) {
					const LDATE dt = getCtrlDate(CTL_CTBLORD_FNDT);
					if(checkdate(dt)) {
						Data.Chunk.Finish.d = dt;
						if(!Data.Chunk.Start.d || diffdate(dt, Data.Chunk.Start.d) != 1) {
							Data.Chunk.Start.d = dt;
							setCtrlDate(CTL_CTBLORD_STDT, dt);
						}
					}
				}
				else if(event.isCtlEvent(CTL_CTBLORD_STTM)) {
					LDATETIME dtm;
					dtm.t = getCtrlTime(CTL_CTBLORD_STTM);
					if(checktime(dtm.t)) {
						Data.Chunk.Start.t = dtm.t;
						dtm.d = getCtrlDate(CTL_CTBLORD_STDT);
						if(checkdate(dtm.d)) {
							long cont = getCtrlLong(CTL_CTBLORD_CONT);
							if(cont > 0) {
								Data.Chunk.Init(dtm, cont * 60);
								setCtrlDatetime(CTL_CTBLORD_FNDT, CTL_CTBLORD_FNTM, Data.Chunk.Finish);
							}
						}
					}
				}
				else if(event.isCtlEvent(CTL_CTBLORD_FNTM)) {
					LDATETIME dtm;
					dtm.t = getCtrlTime(CTL_CTBLORD_FNTM);
					if(checktime(dtm.t)) {
						Data.Chunk.Finish.t = dtm.t;
						dtm.d = getCtrlDate(CTL_CTBLORD_FNDT);
						if(checkdate(dtm.d)) {
							//STimeChunk checkdatetime
							Data.Chunk.Finish = dtm;
							getCtrlDatetime(CTL_CTBLORD_STDT, CTL_CTBLORD_STTM, Data.Chunk.Start);
							long cont = Data.Chunk.GetDuration() / 60;
							setCtrlLong(CTL_CTBLORD_CONT, cont);
						}
					}
				}
				else if(event.isCtlEvent(CTL_CTBLORD_CONT)) {
					LDATETIME dtm;
					getCtrlDatetime(CTL_CTBLORD_STDT, CTL_CTBLORD_STTM, dtm);
					long cont = getCtrlLong(CTL_CTBLORD_CONT);
					if(checkdate(dtm.d) && checktime(dtm.t)) {
						Data.Chunk.Init(dtm, cont * 60);
						setCtrlDatetime(CTL_CTBLORD_FNDT, CTL_CTBLORD_FNTM, Data.Chunk.Finish);
					}
				}
				else
					do_ret = 1;
				__lock = 0;
				if(do_ret)
					return;
			}
			else
				return;
		}
		else
			return;
		clearEvent(event);
	}
	void   SetupSCard(PPID cardID, const char * pScCode)
	{
		SString scard_no(pScCode);
		SString msg_buf, temp_buf;
		SCardTbl::Rec scard_rec;
		double rest = 0.0;
		Data.SCardID = 0;
		if(cardID && ScObj.Search(cardID, &scard_rec) > 0) {
			if(ScObj.IsCreditSeries(scard_rec.SeriesID)) {
				scard_no = scard_rec.Code;
				Data.SCardID = scard_rec.ID;
				ScObj.P_Tbl->GetRest(Data.SCardID, MAXDATE, &rest);
				if(rest != 0.0) {
					PPLoadString("rest", temp_buf);
					msg_buf.Cat(temp_buf).CatDiv(':', 2).Cat(rest, MKSFMTD(0, 2, NMBF_NOZERO));
				}
				if(scard_rec.PersonID) {
					GetPersonName(scard_rec.PersonID, temp_buf);
					msg_buf.CatCharN(' ', 3).Cat(temp_buf);
				}
			}
			else {
				PPGetMessage(mfError, PPERR_SCARDISNTCREDIT, scard_rec.Code, 1, msg_buf.Z());
				scard_no = 0;
			}
			setCtrlString(CTL_CTBLORD_SCARD, scard_no);
		}
		else if(scard_no.NotEmptyS()) {
			if(ScObj.SearchCode(0, scard_no, &scard_rec) > 0) {
				if(ScObj.IsCreditSeries(scard_rec.SeriesID)) {
					Data.SCardID = scard_rec.ID;
					ScObj.P_Tbl->GetRest(Data.SCardID, MAXDATE, &rest);
					if(rest != 0.0) {
						PPLoadString("rest", temp_buf);
						msg_buf.Cat(temp_buf).CatDiv(':', 2).Cat(rest, MKSFMTD(0, 2, NMBF_NOZERO));
					}
					if(scard_rec.PersonID) {
						GetPersonName(scard_rec.PersonID, temp_buf);
						msg_buf.CatCharN(' ', 3).Cat(temp_buf);
					}
				}
				else {
					PPGetMessage(mfError, PPERR_SCARDISNTCREDIT, scard_rec.Code, 1, msg_buf.Z());
					scard_no = 0;
				}
			}
			else {
				PPGetLastErrorMessage(1, msg_buf.Z());
				scard_no.Z();
			}
		}
		setStaticText(CTL_CTBLORD_ST_SCREST, msg_buf);
		disableCtrl(CTL_CTBLORD_PREPAY, !Data.SCardID);
		enableCommand(cmCreateSCard, !Data.SCardID && ScObj.CheckRights(PPR_INS) && ScObj.GetConfig().DefCreditSerID);
	}
	PPObjSCard ScObj;
	CTableOrder * P_To;
};

int CTableOrder::Edit(Packet * pData)
{
	Packet temp_data;
	if(!pData) {
		temp_data.Init(0, getcurdatetime_(), 60*60);
		pData = &temp_data;
	}
	DIALOG_PROC_BODY_P1(CTableOrderDialog, this, pData);
}

int CTableOrder::CheckTableBusyStatus(long tableNo, const STimeChunk & rChunk)
{
	return 1;
}
//
// Может изменить: номер стола, время начала и время завершения, примечание.
//
int CTableOrder::Update(const Packet * pPack, int use_ta)
{
	int    ok = -1;
	SString cc_text;
	CCheckTbl::Rec cc_rec;
	CCheckExtTbl::Rec ccext_rec;
	THROW(HasRight(PPR_MOD));
	THROW_INVARG(pPack);
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW(P_ScObj->P_CcTbl->Search(pPack->ChkID, &cc_rec) > 0);
		CCheckCore::MakeCodeString(&cc_rec, CCheckCore::mcsDefault, cc_text);
		THROW_PP_S(!(cc_rec.Flags & CCHKF_SKIP), PPERR_CCHKORDCANCELED, cc_text);
		THROW_PP_S(!(cc_rec.Flags & CCHKF_CLOSEDORDER), PPERR_CCHKORDCLOSED, cc_text);
		THROW_PP_S(P_ScObj->P_CcTbl->GetExt(pPack->ChkID, &ccext_rec) > 0, PPERR_CTBLORDCHKHASNTEXT, cc_text);
		ccext_rec.TableNo = pPack->TableNo;
		ccext_rec.StartOrdDtm = pPack->Chunk.Start;
		ccext_rec.EndOrdDtm = pPack->Chunk.Finish;
		pPack->Memo.CopyTo(ccext_rec.Memo, sizeof(ccext_rec.Memo));
		ok = P_ScObj->P_CcTbl->UpdateExt(pPack->ChkID, &ccext_rec, 0);
		THROW(ok);
		THROW(tra.Commit());
		if(ok > 0)
			UpdateGridItem(pPack->TableNo, pPack->ChkID, pPack->Chunk);
	}
	CATCHZOK
	return ok;
}

int CTableOrder::Print(const Packet * pPack)
{
	int    ok = 1;
	const  uint rpt_id = REPORT_CCHECKDETAILORD;
	CCheckPacket cc_pack;
	THROW(MakeCCheckPacket(pPack, &cc_pack));
	{
		PPReportEnv env(0, 0);
		PPAlddPrint(rpt_id, PView(&cc_pack), &env);
	}
	CATCHZOK
	return ok;
}

int CTableOrder::MakeCCheckPacket(const Packet * pPack, CCheckPacket * pCcPack)
{
	int    ok = 1;
	PPCashNode cn_rec;
	CCheckPacket cc_pack;
	assert(pPack && pCcPack);
	THROW_INVARG(pPack && pCcPack);
	THROW(P_CnObj->Search(pPack->PosNodeID, &cn_rec) > 0);
	THROW_PP_S(cn_rec.CurSessID, PPERR_CSESSNOPENED, cn_rec.Name);
	pCcPack->Z();
	pCcPack->Rec.Flags |= CCHKF_ORDER;
	pCcPack->Rec.PosNodeID = pPack->PosNodeID;
	pCcPack->Rec.SessID = cn_rec.CurSessID;
	pCcPack->Rec.SCardID = pPack->SCardID;
	pCcPack->Ext.TableNo = pPack->TableNo;
	pCcPack->Ext.StartOrdDtm = pPack->Chunk.Start;
	pCcPack->Ext.EndOrdDtm = pPack->Chunk.Finish;
	pPack->Memo.CopyTo(pCcPack->Ext.Memo, sizeof(pCcPack->Ext.Memo));
	getcurdatetime(&pCcPack->Rec.Dt, &pCcPack->Rec.Tm);
	if(pPack->PrepayAmount > 0.0 && pPack->SCardID) {
		const  PPID chg_goods_id = P_ScObj->GetConfig().ChargeGoodsID;
		THROW_PP(chg_goods_id, PPERR_UNDEFSCCHGGOODS);
		THROW(pCcPack->InsertItem(chg_goods_id, 1.0, pPack->PrepayAmount, 0.0));
		pCcPack->SetupAmount(0, 0);
	}
	CATCHZOK
	return ok;
}

int CTableOrder::Create(const Param * pParam)
{
	int    ok = -1;
	PPCashMachine * p_cm = 0;
	Packet ord;
	LDATETIME dtm;
	const LDATE now_date = getcurdate_();
	THROW(HasRight(PPR_INS));
	if(pParam) {
		PPSyncCashNode acn_pack;
		dtm.Set(NZOR(pParam->StartTime.d, now_date), pParam->StartTime.t);
		ord.Init(pParam->PosNodeID, dtm, pParam->InitDuration);
		if(pParam->TableNo > 0)
			ord.TableNo = static_cast<int16>(pParam->TableNo);
		else if(pParam->PosNodeID && P_CnObj->GetSync(pParam->PosNodeID, &acn_pack) > 0) {
			if(acn_pack.TableSelWhatman.NotEmpty() && fileExists(acn_pack.TableSelWhatman)) {
				TWhatmanObject::SelectObjRetBlock sel_blk;
				if(PPWhatmanWindow::Launch(acn_pack.TableSelWhatman, 0, &sel_blk) > 0) {
					if(sel_blk.Val1 == PPOBJ_CAFETABLE && sel_blk.Val2 > 0)
						ord.TableNo = static_cast<int16>(sel_blk.Val2);
				}
			}
		}
	}
	else {
		dtm.Set(now_date, encodetime(15, 0, 0, 0));
		ord.Init(pParam->PosNodeID, dtm, 60*60);
	}
	if(Edit(&ord) > 0) {
		CCheckPacket cc_pack;
		TSVector <SCardCore::UpdateRestNotifyEntry> urn_list;
		PPTransaction tra(1);
		THROW(tra);
		THROW(MakeCCheckPacket(&ord, &cc_pack));
		{
			int    r = 0, sync_prn_err = 0;
			THROW(p_cm = PPCashMachine::CreateInstance(ord.PosNodeID));
			if((r = p_cm->SyncPrintCheck(&cc_pack, 1)) == 0)
				sync_prn_err = p_cm->SyncGetPrintErrCode();
			if(r || sync_prn_err == 1) {
				cc_pack.Rec.Flags |= CCHKF_PRINTED;
				P_ScObj->P_CcTbl->WriteCCheckLogFile(&cc_pack, 0, CCheckCore::logPrinted, 0);
			}
			/*
			if(r == 0 && sync_prn_err != 3)
				PPError();
			*/
		}
		THROW(ok = P_ScObj->P_CcTbl->TurnCheck(&cc_pack, 0));
		if(ord.PrepayAmount > 0.0 && ord.SCardID && P_ScObj->IsCreditCard(ord.SCardID) > 0) {
			SCardOpTbl::Rec scop_rec;
			MEMSZERO(scop_rec);
			scop_rec.SCardID = cc_pack.Rec.SCardID;
			scop_rec.Dt      = cc_pack.Rec.Dt;
			scop_rec.Tm      = cc_pack.Rec.Tm;
			scop_rec.LinkObjType = PPOBJ_CCHECK;
			scop_rec.LinkObjID   = cc_pack.Rec.ID;
			scop_rec.UserID  = cc_pack.Rec.UserID;
			scop_rec.Amount  = ord.PrepayAmount;
			THROW(P_ScObj->P_Tbl->PutOpRec(&scop_rec, &urn_list, 0));
		}
		THROW(tra.Commit());
		P_ScObj->FinishSCardUpdNotifyList(urn_list);
		UpdateGridItem(cc_pack.Ext.TableNo, cc_pack.Rec.ID, ord.Chunk);
	}
	CATCHZOKPPERR
	delete p_cm;
	return ok;
}

int CTableOrder::Cancel(PPID ordCheckID)
{
	int    ok = -1;
	SString cc_text;
	CCheckTbl::Rec rec;
	THROW(HasRight(PPR_DEL));
	THROW(P_ScObj->P_CcTbl->Search(ordCheckID, &rec) > 0);
	CCheckCore::MakeCodeString(&rec, CCheckCore::mcsDefault, cc_text);
	THROW_PP_S(rec.Flags & CCHKF_ORDER, PPERR_CCHKNORDER, cc_text);
	THROW_PP_S(!(rec.Flags & CCHKF_SKIP), PPERR_CCHKORDCANCELED, cc_text);
	THROW(P_ScObj->P_CcTbl->UpdateFlags(ordCheckID, rec.Flags | CCHKF_SKIP, 1));
	CATCHZOK
	return ok;
}

int CTableOrder::GetCheck(const CCheckTbl::Rec * pCcRec, const CCheckExtTbl::Rec * pCcExtRec, Packet * pPack)
{
	int    ok = -1;
	if(pCcRec && pCcRec->Flags & CCHKF_ORDER) {
		ok = 1;
		if(pPack) {
			LDATETIME zero_dtm = ZERODATETIME;
			pPack->Init(0, zero_dtm, 0);
			pPack->ChkID = pCcRec->ID;
			pPack->CcNo = pCcRec->Code;
			pPack->CcDtm.Set(pCcRec->Dt, pCcRec->Tm);
			pPack->SCardID = pCcRec->SCardID;
			if(pCcRec->Flags & CCHKF_CLOSEDORDER)
				pPack->Status = 1;
			else if(pCcRec->Flags & CCHKF_SKIP)
				pPack->Status = -1;
			else
				pPack->Status = 0;
			if(pCcRec->SessID) {
				PPObjCSession cs_obj;
				CSessionTbl::Rec cs_rec;
				if(cs_obj.Search(pCcRec->SessID, &cs_rec) > 0)
					pPack->PosNodeID = cs_rec.CashNodeID;
			}
			if(pCcRec->SCardID) {
				SCardOpTbl::Rec scop_rec;
				if(P_ScObj->P_Tbl->SearchOpByLinkObj(PPOBJ_CCHECK, pCcRec->ID, &scop_rec) > 0) {
					if(scop_rec.Amount > 0.0) {
						pPack->PrepayAmount = scop_rec.Amount;
					}
				}
			}
			if(pCcExtRec) {
				pPack->TableNo = (int16)pCcExtRec->TableNo;
				pPack->Chunk.Init(pCcExtRec->StartOrdDtm, pCcExtRec->EndOrdDtm);
				pPack->Memo = pCcExtRec->Memo;
			}
		}
	}
	return ok;
}

int CTableOrder::GetCheck(PPID chkID, Packet * pPack)
{
	int    ok = -1;
	CCheckTbl::Rec cc_rec;
	if(P_ScObj->P_CcTbl->Search(chkID, &cc_rec) > 0 && cc_rec.Flags & CCHKF_ORDER) {
		CCheckExtTbl::Rec ccext_rec;
		CCheckExtTbl::Rec * p_ccext_rec = 0;
		if(P_ScObj->P_CcTbl->GetExt(chkID, &ccext_rec) > 0)
			p_ccext_rec = &ccext_rec;
		ok = GetCheck(&cc_rec, p_ccext_rec, pPack);
	}
	return ok;
}

int CTableOrder::GetSCard(PPID scardID, SCardTbl::Rec * pScRec)
{
	return P_ScObj->Search(scardID, pScRec);
}

class CTableTimeChunkGrid : public STimeChunkGrid {
public:
	CTableTimeChunkGrid(PPID posNodeID, CTableOrder * pTo);
	virtual int GetText(int item, long id, SString & rBuf);
	virtual int Edit(int item, long rowID, const LDATETIME & rTm, long * pID);
	virtual int MoveChunk(int mode, long id, long rowId, const STimeChunk & rNewChunk);
	virtual int GetColor(long id, STimeChunkGrid::Color * pClr);
private:
	PPObjSCard ScObj;
	PPID   PosNodeID;
	CTableOrder * P_To; // @notowned
};

CTableTimeChunkGrid::CTableTimeChunkGrid(PPID posNodeID, CTableOrder * pTo) : STimeChunkGrid(), PosNodeID(posNodeID), P_To(pTo)
{
}

int CTableTimeChunkGrid::GetText(int item, long id, SString & rBuf)
{
	int    ok = -1;
	rBuf.Z();
	if(item == iTitle) {
		rBuf = "TEST";
		ok = 1;
	}
	else if(item == iRow) {
		if(id < 0)
			ok = 1;
		else
			ok = STimeChunkGrid::GetText(item, id, rBuf);
	}
	else if(item == iChunk) {
		ok = STimeChunkGrid::GetText(item, id, rBuf);
	}
	else if(item == iChunkBallon) {
		CTableOrder::Packet ord;
		SString temp_buf;
		if(P_To->GetCheck(id, &ord) > 0) {
			SCardTbl::Rec sc_rec;
			PPLoadString("booking", temp_buf);
			rBuf.Cat(temp_buf).CR();
			ord.Chunk.ToStr(STimeChunk::fmtOmitSec, temp_buf.Z());
			rBuf.Cat(temp_buf).CR();
			temp_buf.Z();
			if(ord.SCardID && P_To->GetSCard(ord.SCardID, &sc_rec) > 0) {
				rBuf.Cat(sc_rec.Code).Space();
				if(sc_rec.PersonID) {
					GetPersonName(sc_rec.PersonID, temp_buf);
					rBuf.Cat(temp_buf);
				}

			}
			if(ord.PrepayAmount != 0.0) {
				PPLoadString("prepay", temp_buf);
				rBuf.CR().Cat(temp_buf).CatDiv(':', 2).Cat(ord.PrepayAmount, SFMT_MONEY);
			}
			ok = 1;
		}
	}
	return ok;
}

int CTableTimeChunkGrid::Edit(int item, long rowID, const LDATETIME & rTm, long * pID)
{
	int    ok = -1;
	if(item == iChunk) {
		CTableOrder::Packet pack;
		if(*pID) {
			if(P_To->HasRight(PPR_MOD) && P_To->GetCheck(*pID, &pack) > 0) {
				if(P_To->Edit(&pack) > 0) {
					ok = P_To->Update(&pack, 1);
					if(!ok)
						PPError();
				}
			}
		}
		else {
			if(P_To->HasRight(PPR_INS)) {
				CTableOrder::Param param;
				param.PosNodeID = PosNodeID;
				param.TableNo = rowID;
				param.StartTime = rTm;
				param.InitDuration = 60*60;
				if(P_To->Create(&param) > 0) {
					ok = 1;
				}
			}
		}
	}
	else if(item == iRow) {
		ok = STimeChunkGrid::Edit(item, rowID, rTm, pID);
	}
	return ok;
}

int CTableTimeChunkGrid::MoveChunk(int mode, long id, long rowId, const STimeChunk & rNewChunk)
{
	int    ok = -1;
	if(mode == mmCanResizeLeft || mode == mmCanResizeRight)
		ok = 1;
	else if(mode == mmCanMove) {
		ok = P_To->HasRight(PPR_MOD) ? 1 : 0;
	}
	else if(mode == mmCommit) {
		CTableOrder::Packet pack;
		if(P_To->GetCheck(id, &pack) > 0) {
			pack.TableNo = (int16)rowId;
			pack.Chunk = rNewChunk;
			ok = P_To->Update(&pack, 1);
			if(!ok)
				PPError();
		}
	}
	return ok;
}

int CTableTimeChunkGrid::GetColor(long id, STimeChunkGrid::Color * pClr)
{
	return STimeChunkGrid::GetColor(id, pClr);
}

int CTableOrder::UpdateGridItem(long tableNo, PPID checkID, const STimeChunk & rNewChunk)
{
	int    ok = -1;
	if(P_Grid) {
		int    is_order = 0;
		long   ex_table_no = 0;
		uint pos = 0;
		CCheckTbl::Rec cc_rec;
		STimeChunkAssoc ca;
		if(P_Grid->GetChunk(checkID, &ex_table_no, &ca)) {
			STimeChunkAssocArray * p_ex_row = P_Grid->Get(ex_table_no, &(pos = 0));
			if(p_ex_row) {
				p_ex_row->Remove(checkID);
				ok = 1;
			}
		}
		if(P_ScObj->P_CcTbl->Search(checkID, &cc_rec) > 0) {
			if(cc_rec.Flags & CCHKF_ORDER && cc_rec.Flags & CCHKF_EXT) {
				CCheckExtTbl::Rec ccext_rec;
				if(P_ScObj->P_CcTbl->GetExt(checkID, &ccext_rec) > 0) {
					STimeChunk chunk;
					chunk.Init(ccext_rec.StartOrdDtm, ccext_rec.EndOrdDtm);
					STimeChunkAssocArray * p_row = P_Grid->Get(ccext_rec.TableNo, &(pos = 0));
					if(p_row) {
						p_row->Add(checkID, 0, &chunk, 0);
					}
					else {
						SString row_text;
						p_row = new STimeChunkAssocArray(ccext_rec.TableNo);
						p_row->Add(checkID, 0, &chunk, 0);
						PPLoadString("ftable", row_text);
						row_text.Space().Cat(ccext_rec.TableNo);
						P_Grid->SetRow(p_row, row_text);
					}
					ok = 1;
				}
			}
		}
	}
	return ok;
}

int CTableOrder::SetupGrid(PPID posNodeID)
{
	int    ok = 1;
	SETIFZ(P_Grid, new CTableTimeChunkGrid(posNodeID, this));
	THROW_MEM(P_Grid);
	{
		TSVector <CTableStatus> status_list;
		CCheckCore::FetchCTableOrderList(status_list);
		LongArray table_list;
		uint i, j;
		SString row_text;
		for(j = 0; j < status_list.getCount(); j++) {
			table_list.addUnique(status_list.at(j).TableNo);
		}
		if(posNodeID) {
			PPObjCashNode cn_obj;
			PPCashNode cn_rec;
			if(cn_obj.Search(posNodeID, &cn_rec) > 0 && cn_rec.Flags & CASHF_SYNC) {
				PPSyncCashNode scn;
				if(cn_obj.GetSync(posNodeID, &scn) > 0)
					table_list.addUnique(&scn.CTblList);
			}
		}
		table_list.sort();
		for(i = 0; i < table_list.getCount(); i++) {
			const long table_no = table_list.get(i);
			STimeChunkAssocArray * p_row = new STimeChunkAssocArray(table_no);
			for(j = 0; j < status_list.getCount(); j++) {
				const CTableStatus & r_st = status_list.at(j);
				if(r_st.TableNo == table_no) {
					p_row->Add(r_st.CheckID, 0, &r_st.OrderTime, 1);
				}
			}
			PPLoadString("ftable", row_text);
			row_text.Space().Cat(table_no);
			P_Grid->SetRow(p_row, row_text);
		}
	}
	CATCHZOK
	return ok;
}

/*static*/int CTableOrder::ShowTimeGraph(PPID posNodeID, int modeless)
{
	class CTableTimeChunkBrowser : public PPTimeChunkBrowser {
	public:
		CTableTimeChunkBrowser() : PPTimeChunkBrowser()
		{
			SetBmpId(STimeChunkBrowser::bmpModeGantt, BM_TIMEGRAPH_GANTT);
			SetBmpId(STimeChunkBrowser::bmpModeHourDay, BM_TIMEGRAPH_HOURDAY);
			SetBmpId(STimeChunkBrowser::bmpBack, BM_BACK);
		}
		CTableOrder To;
	};
	int    ok = 1;
	CTableTimeChunkBrowser * p_brw = new CTableTimeChunkBrowser;
	STimeChunkBrowser::Param p, saved_params;
	InitSTimeChunkBrowserParam("CTableOrder", &p);
	saved_params.RegSaveParam = p.RegSaveParam;
	p.Quant = 15 * 60;
	SETFLAG(p.Flags, STimeChunkBrowser::Param::fSnapToQuant, 1);
	p.PixQuant = 20;
	p.PixRow = 20;
	p.PixRowMargin = 5;
	p.HdrLevelHeight = 20;
	const LDATE curdt = getcurdate_();
	p.DefBounds.Set(plusdate(curdt, -1), plusdate(curdt, 90));
	if(p_brw->RestoreParameters(saved_params) > 0) {
		p.PixQuant = saved_params.PixQuant;
		p.PixRow   = saved_params.PixRow;
		p.PixRowMargin = saved_params.PixRowMargin;
		p.TextZonePart = saved_params.TextZonePart;
	}
	p_brw->SetParam(&p);
	THROW(p_brw->To.SetupGrid(posNodeID));
	p_brw->SetData(p_brw->To.P_Grid, 0);
	{
		SString title_buf;
		p_brw->setTitle(PPLoadTextS(PPTXT_PPVTTL_CTABLEORDER, title_buf));
	}
	if(modeless) {
		InsertView(p_brw);
	}
	else {
		ExecViewAndDestroy(p_brw);
	}
	CATCHZOKPPERR
	return ok;
}
//
//
//
class CSessCache : public ObjCacheHash {
public:
	struct Data : public ObjCacheEntry { // size=28+16
		long   SuperSessID;
		long   CashNodeID;
		long   CashNumber;
		long   SessNumber;
		LDATE  Dt;
		LTIME  Tm;
		int16  Incomplete;
		int16  Temporary;
	};
	CSessCache();
	int    GetCcDate2MaxIdIndex(LAssocArray & rIndex)
	{
		int    ok = 1;
		CcD2MiiBlk.Lck.ReadLock_();
		if(!CcD2MiiBlk.IsLoaded) {
			CcD2MiiBlk.Lck.Unlock_();
			CcD2MiiBlk.Lck.WriteLock_();
			{
				PPObjCSession cs_obj;
				int r = cs_obj.GetCcDate2MaxIdIndex(CcD2MiiBlk.Index);
				if(!r) {
					; // @todo @err
				}
				CcD2MiiBlk.IsLoaded = true;
			}
		}
		rIndex = CcD2MiiBlk.Index;
		CcD2MiiBlk.Lck.Unlock_();
		return ok;
	}
	bool   IsCcDate2MaxIdIndexLoaded() 
	{
		bool  result = false;
		CcD2MiiBlk.Lck.ReadLock_();
		result = CcD2MiiBlk.IsLoaded;
		CcD2MiiBlk.Lck.Unlock_();
		return result;
	}
	void   DirtyCcDate2MaxIdIndex()
	{
		CcD2MiiBlk.Lck.WriteLock_();
		CcD2MiiBlk.IsLoaded = false;
		CcD2MiiBlk.Lck.Unlock_();
	}
private:
	virtual int  FetchEntry(PPID id, ObjCacheEntry * pEntry, void * /*extraData*/);
	virtual void EntryToData(const ObjCacheEntry * pEntry, void * pDataRec) const;

	struct CcDate2MaxIdIndex_Block { // @v11.7.4
		CcDate2MaxIdIndex_Block() : IsLoaded(0)
		{
		}
		ReadWriteLock Lck; // Блокировка списка CcDate2MaxIdIndex
		LAssocArray Index; // 
		bool  IsLoaded;    //
		uint8 Reserve[3];  // @alignment
	};
	CcDate2MaxIdIndex_Block CcD2MiiBlk; // @v11.7.4
};

CSessCache::CSessCache() : ObjCacheHash(PPOBJ_CSESSION, sizeof(Data), (1024*1024), 8)
{
}

int CSessCache::FetchEntry(PPID id, ObjCacheEntry * pEntry, void * /*extraData*/)
{
	int    ok = -1;
	PPObjCSession cs_obj;
	Data * p_cache_rec = static_cast<Data *>(pEntry);
	CSessionTbl::Rec rec;
	if(id && cs_obj.Search(id, &rec) > 0) {
		#define FLD(f) p_cache_rec->f = rec.f
		FLD(SuperSessID);
		FLD(CashNodeID);
		FLD(CashNumber);
		FLD(SessNumber);
		FLD(Dt);
		FLD(Tm);
		FLD(Incomplete);
		FLD(Temporary);
		#undef FLD
		ok = 1;
	}
	return ok;
}

void CSessCache::EntryToData(const ObjCacheEntry * pEntry, void * pDataRec) const
{
	CSessionTbl::Rec * p_data_rec = static_cast<CSessionTbl::Rec *>(pDataRec);
	const Data * p_cache_rec = static_cast<const Data *>(pEntry);
	memzero(p_data_rec, sizeof(*p_data_rec));
	#define FLD(f) p_data_rec->f = p_cache_rec->f
	FLD(ID);
	FLD(SuperSessID);
	FLD(CashNodeID);
	FLD(CashNumber);
	FLD(SessNumber);
	FLD(Dt);
	FLD(Tm);
	FLD(Incomplete);
	FLD(Temporary);
	#undef FLD
}

int PPObjCSession::Fetch(PPID id, CSessionTbl::Rec * pRec)
{
	CSessCache * p_cache = GetDbLocalCachePtr <CSessCache> (PPOBJ_CSESSION);
	return p_cache ? p_cache->Get(id, pRec, 0) : Search(id, pRec);
}

int PPObjCSession::FetchCcDate2MaxIdIndex(LAssocArray & rIndex)
{
	CSessCache * p_cache = GetDbLocalCachePtr <CSessCache> (PPOBJ_CSESSION);
	return p_cache ? p_cache->GetCcDate2MaxIdIndex(rIndex) : GetCcDate2MaxIdIndex(rIndex);
}

bool PPObjCSession::IsCcDate2MaxIdIndexLoaded()
{
	CSessCache * p_cache = GetDbLocalCachePtr <CSessCache> (PPOBJ_CSESSION);
	return p_cache ? p_cache->IsCcDate2MaxIdIndexLoaded() : false;
}

IMPL_OBJ_DIRTY(PPObjCSession, CSessCache);

/*static*/uint PPObjCSession::GetCcListByMarkBackDays(const TSCollection <CCheckCore::ListByMarkEntry> & rList)
{
	int16 r = CConfig.CcListByMarkBackDays;
	uint  result = (r > 0 && r <= (10 * 365)) ? static_cast<uint>(r) : 14;
	const LDATE now_dt = getcurdate_();
	for(uint i = 0; i < rList.getCount(); i++) {
		const CCheckCore::ListByMarkEntry * p_entry = rList.at(i);
		if(p_entry && checkdate(p_entry->OrgLotDate)) {
			const long d = diffdate(now_dt, p_entry->OrgLotDate);
			if(d > 0) {
				SETMAX(result, static_cast<uint>(d));
			}
		}
	}
	return result;
}

int PPObjCSession::GetListByEgaisMark(TSCollection <CCheckCore::ListByMarkEntry> & rList)
{
	// @v12.0.5 const uint back_days = 14; // @v11.8.3 90-->14
	const uint back_days = PPObjCSession::GetCcListByMarkBackDays(rList); // @v12.0.5
	LAssocArray index;
	LAssocArray * p_index = FetchCcDate2MaxIdIndex(index) ? &index : 0;
	return P_Cc ? P_Cc->Helper_GetListByMark2(rList, CCheckPacket::lnextEgaisMark, p_index, back_days, CCheckPacket::extssEgaisUrl) : 0;
}

int PPObjCSession::GetListByChZnMark(TSCollection <CCheckCore::ListByMarkEntry> & rList)
{
	// @v12.0.5 const uint back_days = 14; // @v11.8.3 90-->14
	const uint back_days = PPObjCSession::GetCcListByMarkBackDays(rList); // @v12.0.5
	LAssocArray index;
	LAssocArray * p_index = FetchCcDate2MaxIdIndex(index) ? &index : 0;
	return P_Cc ? P_Cc->Helper_GetListByMark2(rList, CCheckPacket::lnextChZnMark, p_index, back_days, 0) : 0;
}

int PPObjCSession::GetListByUuid(const S_GUID & rUuid, uint backDays, PPIDArray & rCcList)
{
	LAssocArray index;
	LAssocArray * p_index = (backDays && FetchCcDate2MaxIdIndex(index)) ? &index : 0;
	return P_Cc ? P_Cc->GetListByUuid(rUuid, p_index, backDays, rCcList) : 0;
}
//
//
//
IMPLEMENT_IMPEXP_HDL_FACTORY(CCHECK2, PPCCheckImpExpParam);


PPCCheckImpExpParam::PPCCheckImpExpParam(uint recId, long flags) : PPImpExpParam(recId, flags), Flags(0), PredefFormat(0), CcByUuidSearchMaxDays(0)
{
}

/*virtual*/int PPCCheckImpExpParam::SerializeConfig(int dir, PPConfigDatabase::CObjHeader & rHdr, SBuffer & rTail, SSerializeContext * pSCtx)
{
	int    ok = 1;
	SString temp_buf;
	StrAssocArray param_list;
	THROW(PPImpExpParam::SerializeConfig(dir, rHdr, rTail, pSCtx));
	if(dir > 0) {
		if(PredefFormat)
			param_list.Add(IMPEXPPARAM_CC_PREDEFFMT, temp_buf.Z().Cat(PredefFormat));
	}
	THROW_SL(pSCtx->Serialize(dir, param_list, rTail));
	if(dir < 0) {
		PredefFormat = piefUndef;
		for(uint i = 0; i < param_list.getCount(); i++) {
			StrAssocArray::Item item = param_list.at_WithoutParent(i);
			temp_buf = item.Txt;
			switch(item.Id) {
				case IMPEXPPARAM_CC_PREDEFFMT: PredefFormat = temp_buf.ToLong(); break;
			}
		}
	}
	CATCHZOK
	return ok;
}

int PPCCheckImpExpParam::WriteIni(PPIniFile * pFile, const char * pSect) const
{
	int    ok = 1;
	SString params;
	SString fld_name;
	SString param_val;
	THROW(PPImpExpParam::WriteIni(pFile, pSect));
	THROW(PPLoadText(PPTXT_IMPEXPPARAM_CC, params));
	if(Direction != 0) { // import
		;
	}
	else { // export
	}
	PPGetSubStr(params, IMPEXPPARAM_CC_PREDEFFMT, fld_name);
	pFile->AppendParam(pSect, fld_name, param_val.Z().Cat(PredefFormat), 1);
	PPGetSubStr(params, IMPEXPPARAM_CC_FLAGS, fld_name);
	pFile->AppendParam(pSect, fld_name, param_val.Z().Cat(Flags), 1);
	PPGetSubStr(params, IMPEXPPARAM_CC_POSNODE, fld_name);
	pFile->AppendParam(pSect, fld_name, param_val.Z().Cat(PosNodeID), 1);
	CATCHZOK
	return ok;
}

int PPCCheckImpExpParam::ReadIni(PPIniFile * pFile, const char * pSect, const StringSet * pExclParamList)
{
	PredefFormat = piefUndef;
	Flags = 0;
	PosNodeID = 0;
	int    ok = 1;
	SString params;
	SString fld_name;
	SString param_val;
	StringSet excl;
	RVALUEPTR(excl, pExclParamList);
	THROW(PPLoadText(PPTXT_IMPEXPPARAM_CC, params));
	if(PPGetSubStr(params, IMPEXPPARAM_CC_PREDEFFMT, fld_name)) {
		excl.add(fld_name);
		if(pFile->GetParam(pSect, fld_name, param_val) > 0)
			PredefFormat = param_val.ToLong();
	}
	if(PPGetSubStr(params, IMPEXPPARAM_CC_FLAGS, fld_name)) {
		excl.add(fld_name);
		if(pFile->GetParam(pSect, fld_name, param_val) > 0)
			Flags = param_val.ToLong();
	}
	if(PPGetSubStr(params, IMPEXPPARAM_CC_POSNODE, fld_name)) {
		excl.add(fld_name);
		if(pFile->GetParam(pSect, fld_name, param_val) > 0)
			PosNodeID = param_val.ToLong();
	}
	THROW(PPImpExpParam::ReadIni(pFile, pSect, &excl));
	CATCHZOK
	return ok;
}

CCheckImpExpDialog::CCheckImpExpDialog() : ImpExpParamDialog(DLG_IMPEXPCC)
{
}

IMPL_HANDLE_EVENT(CCheckImpExpDialog)
{
	ImpExpParamDialog::handleEvent(event);
	if(event.isClusterClk(CTL_IMPEXP_DIR))
		SetupCtrls(GetClusterData(CTL_IMPEXP_DIR));
	else if(event.isClusterClk(CTL_IMPEXPCC_FLAGS))
		GetClusterData(CTL_IMPEXPCC_FLAGS, &Data.Flags);
	else if(event.isCbSelected(CTLSEL_IMPEXPCC_PDFMT))
		SetupCtrls(GetClusterData(CTL_IMPEXP_DIR));
	else
		return;
	clearEvent(event);
}

void CCheckImpExpDialog::SetupCtrls(long direction /* 0 - import, 1 - export */)
{
	//DisableClusterItem(CTL_IMPEXPCC_FLAGS, 0, direction == 0);
	//DisableClusterItem(CTL_IMPEXPCC_FLAGS, 3, direction);
}

int CCheckImpExpDialog::setDTS(const PPCCheckImpExpParam * pData)
{
	RVALUEPTR(Data, pData);
	ImpExpParamDialog::setDTS(&Data);
	SetupStringCombo(this, CTLSEL_IMPEXPCC_PDFMT, PPTXT_PREDEFIMPEXPBILLFMT, Data.PredefFormat);
	SetupPPObjCombo(this, CTLSEL_IMPEXPCC_POSNODE, PPOBJ_CASHNODE, Data.PosNodeID, 0);
	AddClusterAssoc(CTL_IMPEXPCC_FLAGS, 0, PPCCheckImpExpParam::fPrintAsseblyOrders);
	SetClusterData(CTL_IMPEXPCC_FLAGS, Data.Flags);
	SetupCtrls(Data.Direction);
	return 1;
}

int CCheckImpExpDialog::getDTS(PPCCheckImpExpParam * pData)
{
	int    ok = 1;
	THROW(ImpExpParamDialog::getDTS(&Data));
	getCtrlData(CTLSEL_IMPEXPCC_PDFMT, &Data.PredefFormat);
	getCtrlData(CTLSEL_IMPEXPCC_POSNODE, &Data.PosNodeID);
	GetClusterData(CTL_IMPEXPCC_FLAGS, &Data.Flags);
	ASSIGN_PTR(pData, Data);
	CATCH
		ok = PPErrorByDialog(this, 0);
	ENDCATCH
	return ok;
}

PPCCheckImporter::PPCCheckImporter()
{
}

PPCCheckImporter::~PPCCheckImporter()
{
}

int PPCCheckImporter::Init(const PPCCheckImpExpParam * pParam, bool nonInteractive)
{
	int    ok = 1;
	if(!RVALUEPTR(Param, pParam)) {
		THROW(LoadSdRecord(PPREC_CCHECK2, &Param.InrRec));
		ok = Select(&Param, 1, nonInteractive);
	}
	else {
		
	}
	CATCHZOK
	return ok;
}

int PPCCheckImporter::LoadConfig(int isImport)
{
	int    ok = 1;
	SString ini_file_name;
	THROW(PPGetFilePath(PPPATH_BIN, PPFILNAM_IMPEXP_INI, ini_file_name));
	{
		PPIniFile ini_file(ini_file_name, 0, 1, 1);
		Param.Init(isImport);
		THROW(LoadSdRecord(PPREC_CCHECK2, &Param.InrRec));
		{
			SString name(CfgName);
			Param.ProcessName(2, name);
			Param.ProcessName(1, name);
			THROW(Param.ReadIni(&ini_file, name, 0));
		}
	}
	CATCHZOK
	return ok;
}

/*
<?xml version="1.0"?>
<Root>
  <OrderID>4</OrderID>
  <CreateDate>28.06.2023</CreateDate>
  <CreateTime>21:55:09</CreateTime>
  <Customer>
    <Name>Test TESTOV</Name>
    <Phone>0000000000</Phone>
    <Email>dimian@artleks.ru</Email>
  </Customer>
  <Products>
    <Product>
      <GoodsID>3884</GoodsID>
      <Name>Драники со сметаной</Name>
      <Cost>180</Cost>
      <Count>1</Count>
      <Amount>180</Amount>
    </Product>
    <Product>
      <GoodsID>4278</GoodsID>
      <Name>Потрошки  по-деревенски</Name>
      <Cost>320</Cost>
      <Count>1</Count>
      <Amount>320</Amount>
    </Product>
    <Product>
      <GoodsID>4575</GoodsID>
      <Name>Паста с соусом «Карбонара»</Name>
      <Cost>350</Cost>
      <Count>1</Count>
      <Amount>350</Amount>
    </Product>
    <Product>
      <GoodsID>4879</GoodsID>
      <Name>Каша овсянная на молоке с маслом</Name>
      <Cost>50</Cost>
      <Count>1</Count>
      <Amount>50</Amount>
    </Product></Products>
  <Comment></Comment>
  <FlagPaid>true</FlagPaid>
  <AmountTotal>900.00</AmountTotal>
  <StoreID>2</StoreID>
  <StoreTitle>Нойбрандербург на Калинина</StoreTitle>
  <TableID>1</TableID>
  <TableName>№1 (1 зал)</TableName>
  <TypeOrder>Самовывоз</TypeOrder>
  <ReadyTime></ReadyTime>
</Root>
*/

int PPCCheckImporter::Read_Predef_Contract01(xmlParserCtxt * pCtx, const SString & rFileName, CCheckPacket & rPack)
{
	int    ok = 1;
	SString temp_buf;
	SString name_buf;
	SString code_buf;
	Goods2Tbl::Rec goods_rec;
	xmlDoc * p_doc = 0;
	const xmlNode * p_root = 0;
	PPCashNode cn_rec;
	THROW_LXML((p_doc = xmlCtxtReadFile(pCtx, rFileName, 0, XML_PARSE_NOENT)), pCtx);
	if(p_doc) {
		S_GUID doc_guid;
		{
			//
			// Создаем суррогатный GUID документа для того, чтобы избежать дублирования при повторном импорте
			//
			xmlBuffer * p_xml_buf = xmlBufferCreate();
			if(p_xml_buf) {
				xmlSaveCtxt * p_sv_ctx = xmlSaveToBuffer(p_xml_buf, "UTF-8"/*encoding*/, XML_SAVE_AS_XML);
				if(p_sv_ctx) {
					if(xmlSaveDoc(p_sv_ctx, p_doc) == 0) {
						xmlSaveClose(p_sv_ctx);
						const uchar * p_buf_data = xmlBufferContent(p_xml_buf);
						if(p_buf_data) {
							temp_buf.Z().Cat(reinterpret_cast<const char *>(p_buf_data));
						}
					}
				}
			}
			xmlBufferFree(p_xml_buf);
			if(temp_buf.IsEmpty()) { 
				// Если вдруг, что невероятно, не удалось вывести xml в буфер, то используем в качестве "сырья" для GUID'а имя файла (без пути)
				SFsPath ps(rFileName);
				ps.Merge(SFsPath::fNam|SFsPath::fExt, temp_buf);
			}
			binary128 hash_md5 = SlHash::Md5(0, temp_buf.cptr(), temp_buf.Len());
			memcpy(doc_guid.Data, &hash_md5, sizeof(doc_guid));
			rPack.SetGuid(&doc_guid);
		}
		double amount_total = 0.0;
		THROW(p_root = xmlDocGetRootElement(p_doc));
		if(SXml::IsName(p_root, "Root")) {
			LDATETIME cr_dtm(ZERODATETIME);
			for(const xmlNode * p_n = p_root->children; p_n; p_n = p_n->next) {
				if(SXml::GetContentByName(p_n, "OrderID", temp_buf) > 0) {
					temp_buf.Strip();
					rPack.PutExtStrData(CCheckPacket::extssOuterIdent, temp_buf);
				}
				else if(SXml::GetContentByName(p_n, "CreateDate", temp_buf)) {
					cr_dtm.d = strtodate_(temp_buf, DATF_GERMAN);
				}
				else if(SXml::GetContentByName(p_n, "CreateTime", temp_buf)) {
					strtotime(temp_buf, TIMF_HMS, &cr_dtm.t);
				}
				else if(SXml::IsName(p_n, "Customer")) {
					for(const xmlNode * p_c = p_n->children; p_c; p_c = p_c->next) {
						if(SXml::GetContentByName(p_c, "Name", temp_buf)) {
							temp_buf.Strip().Transf(CTRANSF_UTF8_TO_INNER);
							rPack.PutExtStrData(CCheckPacket::extssBuyerName, temp_buf);
						}
						else if(SXml::GetContentByName(p_c, "Phone", temp_buf)) {
							temp_buf.Strip().Transf(CTRANSF_UTF8_TO_INNER);
							rPack.PutExtStrData(CCheckPacket::extssBuyerPhone, temp_buf);
						}
						else if(SXml::GetContentByName(p_c, "Email", temp_buf)) {
							temp_buf.Strip().Transf(CTRANSF_UTF8_TO_INNER);
							rPack.PutExtStrData(CCheckPacket::extssBuyerEMail, temp_buf);
						}
					}
				}
				else if(SXml::IsName(p_n, "Products")) {
					for(const xmlNode * p_c = p_n->children; p_c; p_c = p_c->next) {
						if(SXml::IsName(p_c, "Product")) {
							CCheckLineTbl::Rec ln_rec;
							name_buf.Z();
							PPID   outer_goods_id = 0;
							double amount = 0.0;
							for(const xmlNode * p_w = p_c->children; p_w; p_w = p_w->next) {
								if(SXml::GetContentByName(p_w, "GoodsID", temp_buf)) {
									outer_goods_id = temp_buf.ToLong();
								}
								else if(SXml::GetContentByName(p_w, "Barcode", temp_buf)) {
									code_buf = temp_buf;
								}
								else if(SXml::GetContentByName(p_w, "Name", temp_buf)) {
									name_buf = temp_buf;
								}
								else if(SXml::GetContentByName(p_w, "Cost", temp_buf)) {
									ln_rec.Price = dbltointmny(temp_buf.ToReal());
								}
								else if(SXml::GetContentByName(p_w, "Count", temp_buf)) {
									ln_rec.Quantity = temp_buf.ToReal();
								}
								else if(SXml::GetContentByName(p_w, "Amount", temp_buf)) {
									amount = temp_buf.ToReal();
								}
							}
							PPID   goods_id = 0;
							if(code_buf.NotEmpty()) {
								BarcodeTbl::Rec bc_rec;
								THROW_PP_S(GObj.SearchByBarcode(code_buf, &bc_rec, &goods_rec, 0) > 0, PPERR_CCIMP_WAREBYCCLINENFOUND, code_buf);
								goods_id = goods_rec.ID;
							}
							else {
								THROW_PP_S(GObj.Fetch(ln_rec.GoodsID, &goods_rec) > 0, PPERR_CCIMP_WAREBYCCLINENFOUND, ln_rec.GoodsID);
								goods_id = goods_rec.ID;
							}
							assert(goods_id != 0); // Выше мы нашли товар, а если нет, то выскочили по ошибке.
							ln_rec.GoodsID = goods_id;
							THROW_PP_S(ln_rec.Quantity > 0.0, PPERR_CCIMP_INVQTTY, goods_rec.Name);
							THROW(rPack.InsertItem_(&ln_rec, 0, 0));
						}
					}
				}
				else if(SXml::GetContentByName(p_n, "Comment", temp_buf)) {
					temp_buf.Strip().Transf(CTRANSF_UTF8_TO_INNER);
					rPack.PutExtStrData(CCheckPacket::extssMemo, temp_buf);
				}
				else if(SXml::GetContentByName(p_n, "FlagPaid", temp_buf)) {
				}
				else if(SXml::GetContentByName(p_n, "AmountTotal", temp_buf)) {
					amount_total = temp_buf.ToReal();
				}
				else if(SXml::GetContentByName(p_n, "StoreID", temp_buf)) {
				}
				else if(SXml::GetContentByName(p_n, "StoreTitle", temp_buf)) {
				}
				else if(SXml::GetContentByName(p_n, "TableID", temp_buf)) {
					rPack.Ext.TableNo = temp_buf.ToLong();
				}
				else if(SXml::GetContentByName(p_n, "TableName", temp_buf)) {
				}
				else if(SXml::GetContentByName(p_n, "TypeOrder", temp_buf)) {
					temp_buf.Strip().Transf(CTRANSF_UTF8_TO_INNER);
					rPack.PutExtStrData(CCheckPacket::extssOuterExtTag, temp_buf);
				}
				else if(SXml::GetContentByName(p_n, "ReadyTime", temp_buf)) {
					LDATETIME dtm;
					if(strtodatetime(temp_buf, &dtm, DATF_ISO8601CENT, TIMF_HMS)) {
						rPack.Ext.StartOrdDtm = dtm;
					}
					else if(strtotime(temp_buf, TIMF_HMS, &dtm.t) && dtm.t != ZEROTIME) {
						dtm.d = getcurdate_();
						rPack.Ext.StartOrdDtm = dtm;
					}
				}
			}
			rPack.Rec.Flags |= (CCHKF_IMPORTED|CCHKF_SUSPENDED|CCHKF_SYNC);
			if(checkdate(cr_dtm.d)) {
				rPack.Ext.CreationDtm = cr_dtm;
			}
		}
	}
	CATCHZOK
	return ok;
}

int PPCCheckImporter::SerializeParam(int dir, SBuffer & rBuf, SSerializeContext * pCtx)
{
	int    ok = 1;
	bool   is_ver_ok = false;
	PPVersionInfo vi = DS.GetVersionInfo();
	SVerT cur_ver = vi.GetVersion();
	SVerT ser_ver;
	if(dir > 0) {
		if(Param.Name.NotEmpty())
			CfgName = Param.Name;
		THROW_SL(cur_ver.Serialize(dir, rBuf, pCtx));
		is_ver_ok = true;
	}
	else if(dir < 0) {
		const size_t preserve_offs = rBuf.GetRdOffs();
		int    mj = 0, mn = 0, rz = 0;
		THROW_SL(ser_ver.Serialize(dir, rBuf, pCtx));
		ser_ver.Get(&mj, &mn, &rz);
	}
	THROW_SL(pCtx->Serialize(dir, CfgName, rBuf));
	if(dir < 0) {
		;
	}
	CATCHZOK
	return ok;
}

int PPCCheckImporter::Select(PPCCheckImpExpParam * pParam, int isImport, bool nonInteractive)
{
	int    ok = -1;
	int    valid_data = 0;
	TDialog * dlg = 0;
	uint   p = 0;
	long   id = 0;
	//PPID   loc_id = 0;
	SString ini_file_name;
	StrAssocArray list;
	PPCCheckImpExpParam param;
	THROW_INVARG(pParam);
	THROW(PPGetFilePath(PPPATH_BIN, PPFILNAM_IMPEXP_INI, ini_file_name));
	{
		PPIniFile ini_file(ini_file_name, 0, 1, 1);
		pParam->Direction = BIN(isImport);
		THROW(GetImpExpSections(ini_file, PPREC_CCHECK2, &param, &list, isImport ? 2 : 1));
		if(pParam->Name.NotEmpty())
			id = (list.SearchByTextNc(pParam->Name, &p) > 0) ? static_cast<uint>(list.Get(p).Id) : 0;
		else if(CfgName.NotEmpty()) 
			id = (list.SearchByTextNc(CfgName, &p) > 0) ? static_cast<uint>(list.Get(p).Id) : 0;
		if(nonInteractive) {
			if(id > 0) {
				pParam->ProcessName(1, CfgName);
				ok = pParam->ReadIni(&ini_file, CfgName, 0);
				CfgName = pParam->Name;
				pParam->ProcessName(2, CfgName);
				THROW(ok);
			}
		}
		else {
			SString sect;
			THROW(CheckDialogPtrErr(&(dlg = new TDialog(DLG_RUNIE_CC_IMP))));
			SetupStrAssocCombo(dlg, CTLSEL_RUNIECC_CFG, list, id, 0, 0, 0);
			//SetupPPObjCombo(dlg, CTLSEL_IEGOODS_LOC, PPOBJ_LOCATION, loc_id, 0, 0);
			while(ok < 0 && ExecView(dlg) == cmOK) {
				id = dlg->getCtrlLong(CTLSEL_RUNIECC_CFG);
				if(id) {
					list.GetText(id, sect);
					pParam->ProcessName(1, sect);
					pParam->ReadIni(&ini_file, sect, 0);
					CfgName = pParam->Name;
					pParam->ProcessName(2, CfgName);
					ok = 1;
				}
				else
					PPError(PPERR_INVGOODSIMPEXPCFG);
			}
		}
	}
	CATCHZOK
	delete dlg;
	return ok;
}

int PPCCheckImpExpParam::PreprocessImportFileSpec(StringSet & rList)
{
	// mailfrom:coke@gmail.com?subj=orders
	int    ok = -1;
	SString _file_spec;
	(_file_spec = FileName).Transf(CTRANSF_INNER_TO_UTF8);
	{
		InetUrl url;
		const int urlpr = url.Parse(_file_spec);
		const int url_prot = (urlpr > 0) ? url.GetProtocol() : 0;
		{
			SString temp_buf;
			SFsPath ps;
			if(url_prot > 0) {
				url.GetComponent(InetUrl::cPath, 0, temp_buf);
				ps.Split(temp_buf);
			}
			else
				ps.Split(_file_spec);
			{
				ps.Nam = ps.Nam;
				ps.Merge(temp_buf);
				if(url_prot > 0) {
					SString url_path;
					url_path.EncodeUrl(temp_buf, 0);
					url.SetComponent(InetUrl::cPath, temp_buf);
					url.Compose(0, _file_spec);
				}
				else
					_file_spec = temp_buf;
			}
		}
		THROW(GetFilesFromSource(_file_spec, rList, 0));
	}
    if(rList.getCount())
		ok = 1;
	CATCHZOK
	return ok;
}

int PPCCheckImporter::Run()
{
	int    ok = -1;
	SString temp_buf;
	xmlParserCtxt * p_ctx = 0;
	PPSyncCashNode cn_rec;
	bool need_to_print = LOGIC(Param.Flags & PPCCheckImpExpParam::fPrintAsseblyOrders);
	CPosProcessor * p_posprc = 0;
	THROW_PP(Param.PosNodeID && CnObj.GetSync(Param.PosNodeID, &cn_rec) > 0, PPERR_CCIMP_UNDEFPOSNODE);
	if(Param.PredefFormat == PredefinedImpExpFormat::piefCCheck_Contract01) {
		StringSet ss_files;
		SString filename;
		SString msg_buf;
		SString fmt_buf;
		PPImpExpParam::PtTokenList fn_fld_list;
		CCheckCore * p_cc = CsObj.P_Cc;
		PPIDArray cc_list_by_guid;
		THROW(Param.PreprocessImportFileSpec(ss_files));
		THROW(p_ctx = xmlNewParserCtxt());
		for(uint ssp = 0; ss_files.get(&ssp, filename);) {
			Param.PreprocessImportFileName(filename, fn_fld_list); // isn't used
			CCheckPacket cc_pack;
			const int r = Read_Predef_Contract01(p_ctx, filename, cc_pack);
			if(r > 0) {
				cc_pack.Rec.PosNodeID = Param.PosNodeID;
				// отложенный чек не должен быть привязан к сессии: cc_pack.Rec.SessID == 0
				PPObjPerson::GetCurUserPerson(&cc_pack.Rec.UserID, 0);
				cc_pack.Rec.Flags |= (CCHKF_SYNC|CCHKF_SUSPENDED|CCHKF_IMPORTED);
				{
					CCheckTbl::Rec last_chk_rec;
					if(p_cc->GetLastCheckByCode(Param.PosNodeID, &last_chk_rec) > 0)
						cc_pack.Rec.Code = last_chk_rec.Code + 1;
				}
				cc_pack.SetupAmount(0, 0);
				//p_cc->SearchByDateAndCode()
				{
					const LDATETIME now_dtm = getcurdatetime_();
					cc_pack.Rec.Dt = now_dtm.d;
					cc_pack.Rec.Tm = now_dtm.t;
					p_cc->AdjustRecTime(cc_pack.Rec);
				}
				S_GUID doc_guid;
				const uint back_days = (Param.CcByUuidSearchMaxDays > 0) ? Param.CcByUuidSearchMaxDays : 30;
				PPID   last_found_cc_id = 0;
				if(cc_pack.GetGuid(doc_guid) && CsObj.GetListByUuid(doc_guid, back_days, cc_list_by_guid) > 0) {
					cc_list_by_guid.sort();
					CCheckTbl::Rec last_founc_cc_rec;
					uint   fccidx = cc_list_by_guid.getCount();
					if(fccidx) do {
						const PPID fcc_id = cc_list_by_guid.get(--fccidx);
						if(p_cc->Search(fcc_id, &last_founc_cc_rec) > 0) {
							last_found_cc_id = fcc_id;
							break;
						}
					} while(fccidx);
					if(last_found_cc_id) {
						if(PPLoadText(PPTXT_CCIMP_ALREADYIMPORTED, fmt_buf)) {
							CCheckCore::MakeCodeString(&last_founc_cc_rec, CCheckCore::mcsID, temp_buf);
							Logger.Log(msg_buf.Printf(fmt_buf, temp_buf.cptr()));
						}
					}
				}
				if(!last_found_cc_id) {
					// Каждый чек проводим отдельной транзакцией поскольку нам в дальнейшем понадобяться дополнительные,
					// возможно, весьма длительные действия.
					if(p_cc->TurnCheck(&cc_pack, 1)) {
						if(PPLoadText(PPTXT_CCIMP_ACCEPTED, fmt_buf)) {
							CCheckCore::MakeCodeString(&cc_pack.Rec, CCheckCore::mcsID, temp_buf);
							Logger.Log(msg_buf.Printf(fmt_buf, temp_buf.cptr()));
						}
						if(need_to_print) {
							if(!p_posprc) {
								p_posprc = new CPosProcessor(Param.PosNodeID, 0, 0, 0, 0);
							}
							if(p_posprc) {
								const  bool zero_agent_restriction = p_posprc->Backend_SetZeroAgentRestriction(false);
								PPID   cc_id = cc_pack.Rec.ID; // @note: non-const because func AcceptCheck will modify it
								double cc_amt = 0.0;
								double cc_discount = 0.0;
								cc_pack.CalcAmount(&cc_amt, &cc_discount);
								if(p_posprc->RestoreSuspendedCheck(cc_id, 0/*pPack*/, 0/*unfinishedForReprinting*/)) {
									if(DS.IsThreadInteractive()) {
										p_posprc->PrintToLocalPrinters(-1, true/*ignoreNonZeroAgentReq*/);
									}
									else {
										if(OpenCrrEngine()) {
											p_posprc->PrintToLocalPrinters(-1, true/*ignoreNonZeroAgentReq*/);
											CloseCrrEngine();
										}
										else {
											// @todo @err
										}
									}
									if(!p_posprc->AcceptCheck(&cc_id, 0, 0, cc_amt, CPosProcessor::accmSuspended))
										Logger.LogLastError();
								}
								else
									Logger.LogLastError();
								p_posprc->Backend_SetZeroAgentRestriction(zero_agent_restriction);
							}
						}
						ok = 1;
					}
					else
						Logger.LogLastError();
				}
			}
			else if(r == 0) {
				Logger.LogLastError();
			}
		}
	}
	else if(Param.PredefFormat) {
		CALLEXCEPT_PP_S(PPERR_UNSUPPORTEDPREDEFFMT, Param.PredefFormat);
	}
	else {
		// @todo реализация настраиваемого формата данных
	}
	CATCH
		ok = 0;
		Logger.LogLastError();
	ENDCATCH
	ZDELETE(p_posprc);
	Logger.Save(PPGetFilePathS(PPPATH_LOG, PPFILNAM_IMPEXP_LOG, temp_buf), 0);
	xmlFreeParserCtxt(p_ctx);
	return ok;
}

int ImportCChecks(PPCCheckImpExpParam * pParam)
{
	int    ok = -1;
	int    r = 1;
	PPCCheckImporter importer;
	THROW(r = importer.Init(pParam, false/*interactive*/));
	if(r > 0) {
		THROW(importer.Run());
		ok = 1;
	}
	CATCHZOKPPERR
	return ok;
}