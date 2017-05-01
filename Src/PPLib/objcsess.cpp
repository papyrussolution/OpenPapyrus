// OBJCSESS.CPP
// Copyright (c) A.Sobolev 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016
//
#include <pp.h>
#pragma hdrstop
//
// PPObjCSession
//
static struct __RtToS {
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
	{ CSESSOPRT_CHGCCAGENT,       1, "J" }, // @v8.1.1
	{ CSESSOPRT_MERGECHK,         1, "M" }, // @v8.5.5
	{ CSESSOPRT_ESCCLINEBORD,     1, "Q" }  // @v8.7.3
};

//static
int PPObjCSession::RightsToString(long rt, long opRt, SString & rBuf)
{
	rBuf = 0;
	for(uint i = 0; i < SIZEOFARRAY(RtToS); i++) {
		if((rt & RtToS[i].R && !RtToS[i].IsOpr) || (opRt & RtToS[i].R && RtToS[i].IsOpr)) {
			rBuf.Cat(RtToS[i].S);
		}
	}
	return 1;
}

//static
int PPObjCSession::StringToRights(const char * pBuf, long * pRt, long * pOpRt)
{
	long   rt = 0;
	long   ort = 0;
	const  size_t len = sstrlen(pBuf);
	for(size_t c = 0; c < len; c++) {
		for(uint i = 0; i < SIZEOFARRAY(RtToS); i++) {
			const size_t cl = strlen(RtToS[i].S);
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
	return 1;
}

TLP_IMPL(PPObjCSession, CSessionCore, P_Tbl);

SLAPI PPObjCSession::PPObjCSession(void * extraPtr) : PPObject(PPOBJ_CSESSION)
{
	TLP_OPEN(P_Tbl);
	ExtraPtr = extraPtr;
	P_EqCfg = 0;
}

SLAPI PPObjCSession::~PPObjCSession()
{
	TLP_CLOSE(P_Tbl);
	delete P_EqCfg;
}

const PPEquipConfig & SLAPI PPObjCSession::GetEqCfg()
{
	if(P_EqCfg == 0) {
		P_EqCfg = new PPEquipConfig;
		ReadEquipConfig(P_EqCfg);
	}
	return *P_EqCfg;
}

int SLAPI PPObjCSession::Search(PPID id, void * b)
{
	return SearchByID(P_Tbl, Obj, id, b);
}

//static
SString & SLAPI PPObjCSession::MakeCodeString(const CSessionTbl::Rec * pRec, SString & rBuf)
{
	rBuf = 0;
	rBuf.Cat(pRec->Dt).CatDiv('-', 1).Cat(pRec->SessNumber).CatDiv('-', 1).Cat(pRec->CashNumber).CatDiv('-', 1);
	GetObjectName(PPOBJ_CASHNODE, pRec->CashNodeID, rBuf, 1);
	return rBuf;
}

const char * SLAPI PPObjCSession::GetNamePtr()
{
	return MakeCodeString(&P_Tbl->data, NameBuf).cptr();
}

int SLAPI PPObjCSession::Edit(PPID * pID, void * extraPtr)
{
	int    ok = cmCancel, valid_data = 0;
	SString super_sess_text;
	TDialog * dlg = 0;
	CSessionTbl::Rec rec;
	if(*pID) {
		THROW(Search(*pID, &rec) > 0);
	}
	else
		MEMSZERO(rec);
	THROW(CheckDialogPtr(&(dlg = new TDialog(DLG_CSESS))));
	dlg->setCtrlData(CTL_CSESS_ID, &rec.ID);
	dlg->setCtrlData(CTL_CSESS_SUPERSESSID, &rec.SuperSessID);
	GetObjectName(PPOBJ_CSESSION, rec.SuperSessID, super_sess_text);
	dlg->setCtrlString(CTL_CSESS_SUPERSESSTEXT, super_sess_text);
	dlg->disableCtrl(CTL_CSESS_SUPERSESSTEXT, 1);
	dlg->setCtrlData(CTL_CSESS_NUMBER, &rec.SessNumber);
	dlg->SetupCalDate(CTLCAL_CSESS_DATE, CTL_CSESS_DATE);
	dlg->setCtrlData(CTL_CSESS_DATE, &rec.Dt);
	dlg->setCtrlData(CTL_CSESS_TIME, &rec.Tm);
	SetupPPObjCombo(dlg, CTLSEL_CSESS_CASHNODE, PPOBJ_CASHNODE, rec.CashNodeID, 0, 0);
	dlg->AddClusterAssoc(CTL_CSESS_INCOMPL,  0, CSESSINCMPL_COMPLETE);
	dlg->AddClusterAssoc(CTL_CSESS_INCOMPL,  1, CSESSINCMPL_GLINES);
	dlg->AddClusterAssoc(CTL_CSESS_INCOMPL,  2, CSESSINCMPL_CHECKS);
	dlg->AddClusterAssoc(CTL_CSESS_INCOMPL, -1, CSESSINCMPL_CHECKS);
	dlg->SetClusterData(CTL_CSESS_INCOMPL, rec.Incomplete);
	dlg->setCtrlUInt16(CTL_CSESS_TEMP, BIN(rec.Temporary));
	dlg->disableCtrl(CTL_CSESS_ID, 1);
	for(valid_data = 0; !valid_data && ExecView(dlg) == cmOK;) {
		dlg->getCtrlData(CTL_CSESS_ID, &rec.ID);
		dlg->getCtrlData(CTL_CSESS_SUPERSESSID, &rec.SuperSessID);
		dlg->getCtrlData(CTL_CSESS_NUMBER, &rec.SessNumber);
		dlg->getCtrlData(CTL_CSESS_DATE, &rec.Dt);
		dlg->getCtrlData(CTL_CSESS_TIME, &rec.Tm);
		dlg->getCtrlData(CTLSEL_CSESS_CASHNODE, &rec.CashNodeID);
		dlg->GetClusterData(CTL_CSESS_INCOMPL, &rec.Incomplete);
		rec.Temporary = BIN(dlg->getCtrlUInt16(CTL_CSESS_TEMP));
		if(!checkdate(rec.Dt, 0))
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
	return ok;
}

int SLAPI PPObjCSession::Recalc(PPID sessID, int use_ta)
{
	MemLeakTracer mlt;
	int    ok = 1;
	PPObjBill * p_bobj = BillObj;
	uint   i;
	const  PPID ret_op_id = GetCashRetOp();
	const  PPID wroff_acc_op_id = GetEqCfg().WrOffAccOpID;
	CGoodsLine cgl;
	CCheckCore cc;
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
				if(p_bobj->Search(bill_id, &bill_rec) > 0 && bill_rec.OpID != wroff_acc_op_id) { // @v8.6.6 (bill_rec.OpID != wroff_acc_op_id))
					double amt = BR2(bill_rec.Amount);
					if(bill_rec.OpID == ret_op_id)
						amt = -amt;
					cs_total.WrOffAmount += amt;
				}
			}
			THROW(cgl.CalcSessTotal(sess_id, &cs_total));
			THROW(cc.GetSessTotal(sess_id, P_Tbl->GetCcGroupingFlags(sess_rec, sess_id), &cs_total, 0));
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

int SLAPI PPObjCSession::VerifyAmounts(PPID sessID, const CSessTotal & rTotal, PPLogger & rLogger)
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

int SLAPI PPObjCSession::Recover(const PPIDArray & rSessList)
{
	int    ok = 1;
	PPObjBill * p_bobj = BillObj;
	PPWait(1);
	CGoodsLine cgl;
	CCheckCore cc;
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
					if(p_bobj->Search(bill_id, &bill_rec) > 0 && bill_rec.OpID != wroff_acc_op_id) // @v8.6.6 (bill_rec.OpID != wroff_acc_op_id)
						cs_total.WrOffAmount += BR2((bill_rec.OpID == ret_op_id) ? -bill_rec.Amount : bill_rec.Amount);
				}
				THROW(cgl.CalcSessTotal(sess_id, &cs_total));
				THROW(cc.GetSessTotal(sess_id, P_Tbl->GetCcGroupingFlags(sess_rec, sess_id), &sub_total, 0));
				cs_total.Add(&sub_total);
				if(sess_id != outer_sess_id) {
					THROW(r = VerifyAmounts(sess_id, sub_total, logger));
					if(r < 0)
						ok = -1;
					THROW(Search(sess_id, &sub_rec) > 0);
					if(sub_rec.WrOffAmount > 0.0 || P_Tbl->HasChild(sess_id) > 0) {
						MakeCodeString(&sub_rec, temp_buf = 0);
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
	PPWait(0);
	return ok;
}

int SLAPI PPObjCSession::ReWriteOff(PPID sessID, int level /* @#[0,5,10] */, int use_ta)
{
	int    ok = 1;
	LAssocArray dfct_subst_list;
	PPObjCashNode cn_obj;
	PPCashNode cn_rec;
	CSessGrouping csg;
	CSessionTbl::Rec sess_rec;
	CSessTotal   total;
	THROW(CheckRights(CSESSRT_CORRECT));
	PPWait(1);
	{
		PPObjSecur::Exclusion ose(PPEXCLRT_CSESSWROFF); // @v8.6.1
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
	PPWait(0);
	return ok;
}

int SLAPI PPObjCSession::UndoWritingOff(PPID sessID, int use_ta)
{
	int    ok = 1;
	CGoodsLine cgl;
	CSessTotal cs_total;
	{
		PPObjSecur::Exclusion ose(PPEXCLRT_CSESSWROFFROLLBACK); // @v8.6.1
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
		DS.LogAction(PPACN_UNDOCSESSWROFF, PPOBJ_CSESSION, sessID, 0, 0); // @v7.1.4
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int SLAPI PPObjCSession::RemoveWrOffBills(PPID sessID, int use_ta)
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

//int SLAPI PPObjCSession::Remove(PPID sessID, long, uint options)
//virtual
int  SLAPI PPObjCSession::RemoveObjV(PPID sessID, ObjCollection * pObjColl, uint options/* = rmv_default*/, void * pExtraParam)
{
	int    ok = 1;
	CGoodsLine cgl;
	CCheckCore cc;
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
			THROW(cc.RemoveSess(sess_id, 0));
			THROW_DB(deleteFrom(P_Tbl, 0, P_Tbl->ID == sess_id));
			THROW(RemoveSync(sess_id));
		}
		DS.LogAction(PPACN_OBJRMV, Obj, sessID, 0, 0);
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int SLAPI PPObjCSession::HandleMsg(int msg, PPID _obj, PPID _id, void * extraPtr)
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

int SLAPI PPObjCSession::EditRights(uint bufSize, ObjRights * rt, EmbedDialog * pDlg)
{
	class CSessRightsDlg : public TDialog {
	public:
		CSessRightsDlg::CSessRightsDlg() : TDialog(DLG_RTCSESS)
		{
		}
		virtual int TransmitData(int dir, void * pData)
		{
			int    s = 1;
			if(dir > 0)
				setDTS((ObjRights*)pData);
			else if(dir < 0)
				getDTS((ObjRights*)pData);
			else
				s = TDialog::TransmitData(dir, pData);
			return s;
		}
		int setDTS(ObjRights * pData)
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
				SetClusterData(CTL_RTCSESS_SFLAGS3, pData->OprFlags);
			}
			return 1;
		}
		int getDTS(ObjRights * pData)
		{
			if(pData) {
				ushort comm_rt = getCtrlUInt16(CTL_RTCSESS_FLAGS);
				ushort v = (ushort)GetClusterData(CTL_RTCSESS_SFLAGS);
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

int SLAPI PPObjCSession::NeedTransmit(PPID id, const DBDivPack & rDestDbDivPack, ObjTransmContext * pCtx)
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
				pCtx->Output(msg_buf.Printf(fmt_buf, (const char *)sess_buf));
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
	SLAPI  CSessTransmitPacket() : UseCclExt(BIN(CConfig.Flags & CCFLG_USECCHECKLINEEXT)), Method_700(1)
	{
		Valid = 1;
		ChecksCount = 0;
	}
	int    SLAPI IsValid() const { return Valid; }
	int    SLAPI LoadSession(PPID sessID, ObjTransmContext * pCtx);
	int    SLAPI ProcessRefs(PPObjIDArray *, int replace, ObjTransmContext * pCtx);
	int    SLAPI PutToStream(FILE * fStream);
	int    SLAPI GetFromStream(FILE * fStream, ObjTransmContext * pCtx);
	int    SLAPI Restore(PPID * pID, ObjTransmContext * pCtx);
	CSessionTbl::Rec Rec;
	uint32  ChecksCount; // @!CSessTransmitPacket::LoadSession
private:
	PPObjCSession SessObj;
	CCheckCore Cc;
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

int SLAPI CSessTransmitPacket::Restore(PPID * pID, ObjTransmContext * pCtx)
{
	int    ok = -1;
	uint   i;
	PPID   sess_id = 0;
	SString msg_tmpl_buf, msg_buf, err_msg_fmt, err_msg_text, ccheck_code;
	if(*pID == 0) {
		PPUserFuncProfiler ufp(PPUPRF_CSESSTXMRESTORE); // @v8.1.3
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
			// @v7.1.6 {
			// Очень редко, но бывает, что суперсессия не принимается из другого раздела по
			// причине того, что суперсессия по другому разделу имеет ту же временную метку.
			//
			if(Rec.SuperSessID == 0 && Rec.CashNumber == 0)
				SessObj.P_Tbl->CheckUniqueDateTime(Rec.SuperSessID, Rec.CashNumber, &Rec.Dt, &Rec.Tm);
			// } @v7.1.6
			THROW(AddObjRecByID(SessObj.P_Tbl, SessObj.Obj, &sess_id, &Rec, 0));
			for(i = 0; i < ChecksCount; i++) {
				CCheckPacket pack;
				THROW(Cc.SerializePacket(-1, &pack, Bs, &pCtx->SCtx));
				pack.Rec.ID = 0;
				pack.Rec.SessID = sess_id;
				pack.Rec.Flags |= CCHKF_NOTUSED;
				if(accept_dup_time) {
					CCheckTbl::Rec rec;
					while(Cc.Search(pack.Rec.CashID, pack.Rec.Dt, pack.Rec.Tm, &rec) > 0) {
						if(pack.Rec.Code != rec.Code)
							pack.Rec.Tm.addhs(10);
						else
							break;
					}
				}
				pack.UpdFlags |= CCheckPacket::ufCheckInvariant; // @v7.0.0
				pack.UpdFlags |= CCheckPacket::ufSkipUhtt; // @v8.4.2
				if(!Cc.TurnCheck(&pack, 0)) {
					PPLoadText(PPTXT_LOG_ERRACCEPTCCHECK, err_msg_fmt);
					CCheckCore::MakeCodeString(&pack.Rec, ccheck_code);
					PPGetLastErrorMessage(1, err_msg_text);
					msg_buf.Printf(err_msg_fmt, (const char *)ccheck_code, (const char *)err_msg_text);
					pCtx->Output(msg_buf);
				}
				PPWaitMsg(msg_buf.Printf(msg_tmpl_buf, (long)i+1, (long)ChecksCount));
			}
			THROW(SessObj.Recalc(sess_id, 0));
			if(Rec.SuperSessID && SessObj.Search(Rec.SuperSessID) > 0)
				THROW(SessObj.Recalc(Rec.SuperSessID, 0));
			THROW(tra.Commit());
		}
		ufp.SetFactor(0, (double)ChecksCount); // @v8.1.3
		ufp.Commit();                          // @v8.1.3
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
				THROW(Cc.SerializePacket(-1, &pack, Bs, &pCtx->SCtx));
				{
					const CcAmountList & r_al = pack.AL_Const();
					if(r_al.getCount()) {
						int    do_recover = 0;
						for(uint j = 0; !do_recover && j < r_al.getCount(); j++) {
							if(r_al.at(j).AddedID)
								do_recover = 1;
						}
						if(do_recover) {
							CCheckTbl::Key3 k3;
							MEMSZERO(k3);
							k3.SessID = *pID;
							k3.CashID = pack.Rec.CashID;
							k3.Dt = pack.Rec.Dt;
							k3.Tm = pack.Rec.Tm;
							PPID   cc_id = 0;
							if(Cc.search(3, &k3, spGe)) do {
								if(labs(DiffTime(Cc.data.Tm, pack.Rec.Tm, 3)) < 10 && Cc.data.Code == pack.Rec.Code) {
									cc_id = Cc.data.ID;
								}
							} while(!cc_id && Cc.search(3, &k3, spGt));
							if(cc_id) {
								const double epsilon = 1E-6;
								CCheckPacket cc_pack;
								if(Cc.LoadPacket(cc_id, 0, &cc_pack) > 0) {
									SCardTbl::Rec sc_rec;
									CCheckPaymTbl::Key0 cpk0;
									MEMSZERO(cpk0);
									cpk0.CheckID = cc_id;
									if(Cc.PaymT.search(0, &cpk0, spGe) && Cc.PaymT.data.CheckID == cc_id) {
										uint k = 0;
										do {
											CcAmountEntry cc_ae;
											const int16 rbc = Cc.PaymT.data.RByCheck;
											cc_ae.Type = Cc.PaymT.data.PaymType;
											cc_ae.Amount = intmnytodbl(Cc.PaymT.data.Amount);
											cc_ae.AddedID = Cc.PaymT.data.SCardID;
											if(cc_ae.AddedID && k < r_al.getCount()) {
												const CcAmountEntry & r_ae = r_al.at(k);
												if(r_ae.Type == cc_ae.Type && fabs(r_ae.Amount - cc_ae.Amount) < epsilon && r_ae.AddedID != cc_ae.AddedID) {
													if(Cc.Cards.Search(r_ae.AddedID, &sc_rec) > 0) {
														cc_ae.AddedID = r_ae.AddedID;
														{
															Cc.PaymT.data.SCardID = r_ae.AddedID;
															THROW_DB(Cc.PaymT.updateRec());
															//
															// PPTXT_CCPAYMSCARDCORRECTED  "Скорректирована карта оплаты в чеке %s"
															PPLoadText(PPTXT_CCPAYMSCARDCORRECTED, err_msg_fmt);
															CCheckCore::MakeCodeString(&pack.Rec, ccheck_code);
															pCtx->Output(msg_buf.Printf(err_msg_fmt, (const char *)ccheck_code));
														}
													}
												}
											}
											k++;
										} while(Cc.PaymT.search(0, &cpk0, spNext) && Cc.PaymT.data.CheckID == cc_id);
									}
								}
							}
							// @v8.4.2 {
							else {
								pack.UpdFlags |= CCheckPacket::ufCheckInvariant; // @v7.0.0
								pack.UpdFlags |= CCheckPacket::ufSkipUhtt; // @v8.4.2
								CCheckCore::MakeCodeString(&pack.Rec, ccheck_code);
								if(!Cc.TurnCheck(&pack, 0)) {
									PPLoadText(PPTXT_LOG_ERRACCEPTCCHECK, err_msg_fmt);
									PPGetLastErrorMessage(1, err_msg_text);
									msg_buf.Printf(err_msg_fmt, (const char *)ccheck_code, (const char *)err_msg_text);
								}
								else {
									PPLoadText(PPTXT_CCABSRESTORED, err_msg_fmt);
									msg_buf.Printf(err_msg_fmt, (const char *)ccheck_code);
								}
								pCtx->Output(msg_buf);
							}
							// } @v8.4.2
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

int SLAPI CSessTransmitPacket::ProcessRefs(PPObjIDArray * ary, int replace, ObjTransmContext * pCtx)
{
	int    ok = 1;
	SBuffer temp_buf;
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
		THROW(Cc.SerializePacket(-1, &pack, Bs, &pCtx->SCtx));
		THROW(PPObject::ProcessObjRefInArray(PPOBJ_SCARD,   &pack.Rec.SCardID, ary, replace));
		THROW(PPObject::ProcessObjRefInArray(PPOBJ_PERSON,  &pack.Rec.UserID, ary, replace));
		THROW(PPObject::ProcessObjRefInArray(PPOBJ_ARTICLE, &pack.Ext.SalerID,  ary, replace));
		THROW(PPObject::ProcessObjRefInArray(PPOBJ_LOCATION, &pack.Ext.AddrID,  ary, replace)); // @v9.0.4
		// @v9.0.4 THROW(PPObject::ProcessObjRefInArray(PPOBJ_SCARD,   &pack.Ext.AddCrdCardID,  ary, replace));
		for(uint j = 0; j < pack.Items_.getCount(); j++) {
			CCheckLineTbl::Rec & r_line = pack.Items_.at(j);
			THROW(PPObject::ProcessObjRefInArray(PPOBJ_GOODS, &r_line.GoodsID, ary, replace));
		}
		// @v8.2.3 {
		for(uint k = 0; k < pack.AL().getCount(); k++) {
			CcAmountEntry & r_al_entry = pack.AL().at(k);
			if(r_al_entry.Type == CCAMTTYP_CRDCARD) {
				THROW(PPObject::ProcessObjRefInArray(PPOBJ_SCARD, &r_al_entry.AddedID, ary, replace));
				THROW(PPObject::ProcessObjRefInArray(PPOBJ_CURRENCY, &r_al_entry.CurID, ary, replace)); // @v9.0.4
			}
		}
		// } @v8.2.3
		if(replace) {
			THROW(Cc.SerializePacket(+1, &pack, temp_buf, &pCtx->SCtx));
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
int SLAPI CSessTransmitPacket::GetFromStream(FILE * stream, ObjTransmContext * pCtx)
{
	int    ok = 1;
	Bs.Clear();
	THROW_SL(Bs.ReadFromFile(stream, 0))
	CATCHZOK
	return ok;
}
//
// Сохраняет сессию в потоке передачи данных
//
int SLAPI CSessTransmitPacket::PutToStream(FILE * stream)
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
int SLAPI CSessTransmitPacket::LoadSession(PPID sessID, ObjTransmContext * pCtx)
{
	int    ok = 1;
	ChecksCount = 0;
	Bs.Clear();
	if(SessObj.Search(sessID, &Rec) > 0) {
		PPIDArray id_list;
		CCheckTbl::Key3 k3;
		MEMSZERO(k3);
		k3.SessID = sessID;
		while(Cc.search(3, &k3, spGt) && k3.SessID == sessID) {
			id_list.add(Cc.data.ID);
		}
		ChecksCount = id_list.getCount();
		THROW_SL(SessObj.P_Tbl->SerializeRecord(+1, &Rec, Bs, &pCtx->SCtx));
		THROW_SL(pCtx->SCtx.Serialize(+1, ChecksCount, Bs));
		for(uint i = 0; i < id_list.getCount(); i++) {
			CCheckPacket cc_pack;
			THROW(Cc.LoadPacket(id_list.get(i), 0, &cc_pack) > 0);
			THROW(Cc.SerializePacket(+1, &cc_pack, Bs, &pCtx->SCtx));
		}
		SessObj.P_Tbl->GetSubSessList(sessID, &ChildIdList);
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

int SLAPI PPObjCSession::Read(PPObjPack * p, PPID id, void * stream, ObjTransmContext * pCtx)
{
	int    ok = 1;
	CSessTransmitPacket * p_pack = new CSessTransmitPacket;
	THROW_MEM(p_pack);
	THROW(p_pack->IsValid());
	if(stream == 0) {
		THROW(p_pack->LoadSession(id, pCtx) > 0);
	}
	else {
		THROW(p_pack->GetFromStream((FILE *)stream, pCtx));
	}
	p->Data = p_pack;
	CATCHZOK
	return ok;
}

int SLAPI PPObjCSession::Write(PPObjPack * p, PPID * pID, void * stream, ObjTransmContext * pCtx)
{
	int    ok = 1;
	CSessTransmitPacket * p_pack = p ? (CSessTransmitPacket *)p->Data : 0;
	if(p_pack)
		if(stream == 0) {
			if(!p_pack->Restore(pID, pCtx))
				ok = -1;
		}
		else {
			THROW(p_pack->PutToStream((FILE *)stream));
		}
	CATCHZOK
	return ok;
}

IMPL_DESTROY_OBJ_PACK(PPObjCSession, CSessTransmitPacket);

int SLAPI PPObjCSession::ProcessObjRefs(PPObjPack * p, PPObjIDArray * ary, int replace, ObjTransmContext * pCtx)
{
	return (p && p->Data) ? ((CSessTransmitPacket *)p->Data)->ProcessRefs(ary, replace, pCtx) : -1;
}
//
//  @VADIM Регламентация операционных прав кассира с помощью клавиатуры с ключом
//
struct _RightNKeyPosEntry { // @persistent @size=8
	long   OperRightFlag; // CSESSRT_XXX || CSESSOPRT_XXX (к счастью, они не пересекаются)
	long   KeyPos;
};

#define WKEYRTCOUNT 13 // @v7.0.5 [9]-->[11] // @v8.5.5 [11]-->[13]

struct _PPKeybordWKeyCfg { // @persistent @store(PropertyTbl) @size=84
	PPID   Tag;            // Const=PPOBJ_CONFIG
	PPID   ID;             // Const=PPCFG_MAIN
	PPID   Prop;           // Const=PPPRP_KEYBWKEYCFG
	_RightNKeyPosEntry OperRights[WKEYRTCOUNT]; // Соответствие операционных прав положениям ключа
	// @v7.0.5 char   Reserve[16];    // @reserve
};

int SLAPI GetOperRightsByKeyPos(int keyPos, PPIDArray * pOperRightsAry)
{
	int    ok = -1;
	if(pOperRightsAry) {
		_PPKeybordWKeyCfg  kwk_cfg;
		pOperRightsAry->freeAll();
		if(keyPos > 0 && PPRef->GetProp(PPOBJ_CONFIG, PPCFG_MAIN, PPPRP_KEYBWKEYCFG, &kwk_cfg, sizeof(kwk_cfg)) > 0) {
			for(int i = 0; i < WKEYRTCOUNT; i++) {
				BitArray  key_pos_ary;
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

int SLAPI EditDueToKeyboardRights()
{
	class KeybWKeyCfgDlg : public TDialog {
	public:
		KeybWKeyCfgDlg() : TDialog(DLG_CFGKBDWKEY)
		{
		}
		int    setDTS(_PPKeybordWKeyCfg * pCfg)
		{
			if(!RVALUEPTR(KWKCfg, pCfg))
				MEMSZERO(KWKCfg);
			SString  key_pos;
			for(int i = 0; i < WKEYRTCOUNT; i++) {
				StringSet key_pos_str(',', 0);
				BitArray  key_pos_ary;
				key_pos_ary.Init(&KWKCfg.OperRights[i].KeyPos, 32);
				for(size_t pos = 0; pos < 32; pos++)
					if(key_pos_ary.get(pos))
						key_pos_str.add((key_pos = 0).Cat((long)(pos + 1)));
				setCtrlData(CTL_CFGKBDWKEY1 + i, key_pos_str.getBuf());
			}
			return 1;
		}
		int    getDTS(_PPKeybordWKeyCfg * pCfg)
		{
			int    ok = 1;
			uint   sel = 0;
			for(int i = 0; i < WKEYRTCOUNT; i++) {
				long   l_init = 0;
				char   buf[64];
				SString  key_pos;
				BitArray key_pos_ary;
				key_pos_ary.Init(&l_init, 32);
				getCtrlData(sel = CTL_CFGKBDWKEY1 + i, buf);
				StringSet  key_pos_str(',', buf);
				for(uint pos = 0; key_pos_str.get(&pos, key_pos);) {
					long   l_key_pos;
					THROW_PP((l_key_pos = key_pos.ToLong()) > 0 && l_key_pos < 33, PPERR_USERINPUT);
					key_pos_ary.set((size_t)(l_key_pos - 1), 1);
				}
				key_pos_ary.getBuf(&KWKCfg.OperRights[i].KeyPos, sizeof(long));
			}
			KWKCfg.OperRights[0].OperRightFlag = CSESSOPRT_RETCHECK;
			KWKCfg.OperRights[1].OperRightFlag = CSESSOPRT_ESCCLINE;
			KWKCfg.OperRights[2].OperRightFlag = CSESSOPRT_BANKING;
			KWKCfg.OperRights[3].OperRightFlag = CSESSRT_CLOSE;
			KWKCfg.OperRights[4].OperRightFlag = CSESSRT_ESCCHECK;
			KWKCfg.OperRights[5].OperRightFlag = CSESSOPRT_COPYCHECK;
			KWKCfg.OperRights[6].OperRightFlag = CSESSOPRT_COPYZREPT;
			KWKCfg.OperRights[7].OperRightFlag = CSESSOPRT_ROWDISCOUNT;
			KWKCfg.OperRights[8].OperRightFlag = (CSESSOPRT_XREP << 16);   // CSESSOPRT_XREP пересекается с CSESSRT_CLOSE
			KWKCfg.OperRights[9].OperRightFlag = CSESSOPRT_SPLITCHK;
			KWKCfg.OperRights[10].OperRightFlag = CSESSOPRT_MERGECHK;      // @v8.5.5
			KWKCfg.OperRights[11].OperRightFlag = CSESSOPRT_CHGPRINTEDCHK;
			KWKCfg.OperRights[12].OperRightFlag = CSESSOPRT_CHGCCAGENT;    // @v8.5.5
			KWKCfg.OperRights[13].OperRightFlag = CSESSOPRT_ESCCLINEBORD;  // @v8.7.3
			ASSIGN_PTR(pCfg, KWKCfg);
			CATCH
				ok = PPErrorByDialog(this, sel);
			ENDCATCH
			return ok;
		}
		_PPKeybordWKeyCfg  KWKCfg;
	};
	int    ok = -1, valid_data = 0;
	_PPKeybordWKeyCfg  kwk_cfg;
	KeybWKeyCfgDlg * p_dlg = 0;
	THROW(CheckCfgRights(PPCFGOBJ_KEYBWKEYCFG, PPR_READ, 0));
	THROW(CheckDialogPtr(&(p_dlg = new KeybWKeyCfgDlg())));
	if(PPRef->GetProp(PPOBJ_CONFIG, PPCFG_MAIN, PPPRP_KEYBWKEYCFG, &kwk_cfg, sizeof(kwk_cfg)) <= 0)
		MEMSZERO(kwk_cfg);
	p_dlg->setDTS(&kwk_cfg);
	while(!valid_data && ExecView(p_dlg) == cmOK) {
		if(p_dlg->getDTS(&kwk_cfg)) {
			int  is_key_pos = 0;
			for(int i = 0; i < WKEYRTCOUNT; i++)
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
CTableOrder::Packet::Packet()
{
	LDATETIME dtm;
	dtm.SetZero();
	SCardID = 0;
	Init(0, dtm, 0);
}

int CTableOrder::Packet::Init(PPID posNodeID, LDATETIME initDtm, long initDuration)
{
	PosNodeID = posNodeID;
	ChkID = 0;
	CcNo = 0;
	CcDtm.SetZero();
	TableNo = 0;
	Status = 0;
	PrepayAmount = 0.0;
	Memo = 0;
	if(checkdate(initDtm.d, 0))
		Chunk.Init(initDtm, initDuration);
	else {
		LDATETIME dtm = getcurdatetime_();
		dtm.t = encodetime(dtm.t.hour()+1, 0, 0, 0);
		Chunk.Init(dtm, (initDuration > 0) ? initDuration : (60*60));
	}
	return 1;
}
//
//
//
CTableOrder::Param::Param()
{
	Ver = 1;
	PosNodeID = 0;
	StartTime = ZEROTIME;
	InitDuration = 0;
	Flags = 0;
	TableNo = 0;
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

CTableOrder::CTableOrder()
{
	P_ScObj = new PPObjSCard;
	P_CnObj = new PPObjCashNode;
	P_Grid = 0;
	State = 0;
	State |= stRtUndef;
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
public:
	CTableOrderDialog(CTableOrder * pTo) : TDialog(DLG_CTBLORD)
	{
		P_To = pTo;
		SetupCalDate(CTLCAL_CTBLORD_STDT, CTL_CTBLORD_STDT);
		SetupCalDate(CTLCAL_CTBLORD_FNDT, CTL_CTBLORD_FNDT);
		SetupTimePicker(this, CTL_CTBLORD_STTM, CTLTM_CTBLORD_STTM); // @v6.7.9
		SetupTimePicker(this, CTL_CTBLORD_FNTM, CTLTM_CTBLORD_FNTM); // @v6.7.9
		enableCommand(cmCreateSCard, !Data.SCardID && ScObj.CheckRights(PPR_INS) && ScObj.GetConfig().DefCreditSerID);
	}
	int    setDTS(const CTableOrder::Packet * pData)
	{
		int    ok = 1;
		Data = *pData;
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
		setCtrlString(CTL_CTBLORD_MEMO, Data.Memo); // @v7.1.3
		enableCommand(cmCreateSCard, !Data.SCardID && ScObj.CheckRights(PPR_INS) && ScObj.GetConfig().DefCreditSerID); // @v6.8.0
		disableCtrls((Data.ChkID != 0), CTLSEL_CTBLORD_POSNODE, CTL_CTBLORD_CCN, CTL_CTBLORD_SCARD,
			CTL_CTBLORD_PREPAY, 0);
		return ok;
	}
	int    getDTS(CTableOrder::Packet * pData)
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
		CATCH
			ok = PPErrorByDialog(this, sel);
		ENDCATCH
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
					if(checkdate(dt, 0)) {
						Data.Chunk.Start.d = dt;
						if(!Data.Chunk.Finish.d || diffdate(Data.Chunk.Finish.d, dt) != 1) {
							Data.Chunk.Finish.d = dt;
							setCtrlDate(CTL_CTBLORD_FNDT, dt);
						}
					}
				}
				else if(event.isCtlEvent(CTL_CTBLORD_FNDT)) {
					const LDATE dt = getCtrlDate(CTL_CTBLORD_FNDT);
					if(checkdate(dt, 0)) {
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
						if(checkdate(dtm.d, 0)) {
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
						if(checkdate(dtm.d, 0)) {
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
					if(checkdate(dtm.d, 0) && checktime(dtm.t)) {
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
		SString scard_no = pScCode;
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
				PPGetMessage(mfError, PPERR_SCARDISNTCREDIT, scard_rec.Code, 1, msg_buf = 0);
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
					PPGetMessage(mfError, PPERR_SCARDISNTCREDIT, scard_rec.Code, 1, msg_buf = 0);
					scard_no = 0;
				}
			}
			else {
				PPGetLastErrorMessage(1, msg_buf = 0);
				scard_no = 0;
			}
		}
		setStaticText(CTL_CTBLORD_ST_SCREST, msg_buf);
		disableCtrl(CTL_CTBLORD_PREPAY, !Data.SCardID);
		enableCommand(cmCreateSCard, !Data.SCardID && ScObj.CheckRights(PPR_INS) && ScObj.GetConfig().DefCreditSerID); // @v6.8.0
	}
	CTableOrder::Packet Data;
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
		CCheckCore::MakeCodeString(&cc_rec, cc_text);
		THROW_PP_S(!(cc_rec.Flags & CCHKF_SKIP), PPERR_CCHKORDCANCELED, cc_text);
		THROW_PP_S(!(cc_rec.Flags & CCHKF_CLOSEDORDER), PPERR_CCHKORDCLOSED, cc_text);
		THROW_PP_S(P_ScObj->P_CcTbl->GetExt(pPack->ChkID, &ccext_rec) > 0, PPERR_CTBLORDCHKHASNTEXT, cc_text);
		ccext_rec.TableNo = pPack->TableNo;
		ccext_rec.StartOrdDtm = pPack->Chunk.Start;
		ccext_rec.EndOrdDtm = pPack->Chunk.Finish;
		pPack->Memo.CopyTo(ccext_rec.Memo, sizeof(ccext_rec.Memo)); // @v7.1.3
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
	uint   rpt_id = REPORT_CCHECKDETAILORD;
	CCheckPacket cc_pack;
	THROW(MakeCCheckPacket(pPack, &cc_pack));
	{
		PView  pv(&cc_pack);
		PPReportEnv env;
		//env.PrnFlags = noAsk ? SReport::PrintingNoAsk : 0;
		PPAlddPrint(rpt_id, &pv, &env);
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
	pCcPack->Init();
	pCcPack->Rec.Flags |= CCHKF_ORDER;
	pCcPack->Rec.CashID = pPack->PosNodeID;
	pCcPack->Rec.SessID = cn_rec.CurSessID;
	pCcPack->Rec.SCardID = pPack->SCardID;
	pCcPack->Ext.TableNo = pPack->TableNo;
	pCcPack->Ext.StartOrdDtm = pPack->Chunk.Start;
	pCcPack->Ext.EndOrdDtm = pPack->Chunk.Finish;
	pPack->Memo.CopyTo(pCcPack->Ext.Memo, sizeof(pCcPack->Ext.Memo)); // @v7.1.3
	getcurdatetime(&pCcPack->Rec.Dt, &pCcPack->Rec.Tm);
	if(pPack->PrepayAmount > 0.0 && pPack->SCardID) {
		const PPID chg_goods_id = P_ScObj->GetConfig().ChargeGoodsID;
		THROW_PP(chg_goods_id, PPERR_UNDEFSCCHGGOODS);
		THROW(pCcPack->InsertItem(chg_goods_id, 1.0, pPack->PrepayAmount, 0.0));
		THROW(pCcPack->SetupAmount(0, 0));
	}
	CATCHZOK
	return ok;
}

int CTableOrder::Create(Param * pParam)
{
	int    ok = -1;
	PPCashMachine * p_cm = 0;
	Packet ord;
	LDATETIME dtm;
	THROW(HasRight(PPR_INS));
	if(pParam) {
		PPSyncCashNode acn_pack;
		dtm.Set(NZOR(pParam->StartTime.d, getcurdate_()), pParam->StartTime.t);
		ord.Init(pParam->PosNodeID, dtm, pParam->InitDuration);
		if(pParam->TableNo > 0)
			ord.TableNo = (int16)pParam->TableNo;
		else if(pParam->PosNodeID && P_CnObj->GetSync(pParam->PosNodeID, &acn_pack) > 0) {
			if(acn_pack.TableSelWhatman.NotEmpty() && fileExists(acn_pack.TableSelWhatman)) {
				TWhatmanObject::SelectObjRetBlock sel_blk;
				if(PPWhatmanWindow::Launch(acn_pack.TableSelWhatman, 0, &sel_blk) > 0) {
					if(sel_blk.Val1 == PPOBJ_CAFETABLE && sel_blk.Val2 > 0)
						ord.TableNo = (int16)sel_blk.Val2;
				}
			}
		}
	}
	else {
		dtm.Set(getcurdate_(), encodetime(15, 0, 0, 0));
		ord.Init(pParam->PosNodeID, dtm, 60*60);
	}
	if(Edit(&ord) > 0) {
		CCheckPacket cc_pack;
		TSArray <SCardCore::UpdateRestNotifyEntry> urn_list;
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
	CCheckCore::MakeCodeString(&rec, cc_text);
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
			LDATETIME zero_dtm;
			zero_dtm.SetZero();
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
				if(P_ScObj->P_Tbl->SearchOpByCheck(pCcRec->ID, &scop_rec) > 0) {
					if(scop_rec.Amount > 0.0) {
						pPack->PrepayAmount = scop_rec.Amount;
					}
				}
			}
			if(pCcExtRec) {
				pPack->TableNo = (int16)pCcExtRec->TableNo;
				pPack->Chunk.Init(pCcExtRec->StartOrdDtm, pCcExtRec->EndOrdDtm);
				pPack->Memo = pCcExtRec->Memo; // @v7.1.3
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
	// @v9.4.5 ~CTableTimeChunkGrid();
	virtual int GetText(int item, long id, SString & rBuf);
	virtual int Edit(int item, long rowID, const LDATETIME & rTm, long * pID);
	virtual int MoveChunk(int mode, long id, long rowId, const STimeChunk & rNewChunk);
	virtual int GetColor(long id, STimeChunkGrid::Color * pClr);

	PPObjSCard ScObj;
	PPID   PosNodeID;
	CTableOrder * P_To; // @notowned
};

CTableTimeChunkGrid::CTableTimeChunkGrid(PPID posNodeID, CTableOrder * pTo) : STimeChunkGrid()
{
	PosNodeID = posNodeID;
	P_To = pTo;
}

/* @v9.4.5 CTableTimeChunkGrid::~CTableTimeChunkGrid()
{
}*/

int CTableTimeChunkGrid::GetText(int item, long id, SString & rBuf)
{
	int    ok = -1;
	rBuf = 0;
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
			ord.Chunk.ToStr(temp_buf = 0, STimeChunk::fmtOmitSec);
			rBuf.Cat(temp_buf).CR();
			temp_buf = 0;
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
		TSArray <CTableStatus> status_list;
		CCheckCore::FetchCTableOrderList(status_list);
		LongArray table_list;
		uint i, j;
		SString row_text;
		for(j = 0; j < status_list.getCount(); j++) {
			table_list.addUnique(status_list.at(j).TableNo);
		}
		// @v7.6.11 {
		if(posNodeID) {
			PPObjCashNode cn_obj;
			PPCashNode cn_rec;
			if(cn_obj.Search(posNodeID, &cn_rec) > 0 && cn_rec.Flags & CASHF_SYNC) {
				PPSyncCashNode scn;
				if(cn_obj.GetSync(posNodeID, &scn) > 0)
					table_list.addUnique(&scn.CTblList);
			}
		}
		// } @v7.6.11
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

// static
int CTableOrder::ShowTimeGraph(PPID posNodeID, int modeless)
{
	class CTableTimeChunkBrowser : public STimeChunkBrowser {
	public:
		CTableTimeChunkBrowser() : STimeChunkBrowser()
		{
			//P_To = new CTableOrder;
			SetBmpId(STimeChunkBrowser::bmpModeGantt, BM_TIMEGRAPH_GANTT);
			SetBmpId(STimeChunkBrowser::bmpModeHourDay, BM_TIMEGRAPH_HOURDAY);
			SetBmpId(STimeChunkBrowser::bmpBack, BM_BACK);
		}
		/*~CTableTimeChunkBrowser()
		{
			delete P_To;
		}*/
		//CTableOrder * P_To;
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
		PPLoadText(PPTXT_PPVTTL_CTABLEORDER, title_buf);
		p_brw->setTitle(title_buf);
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
	SLAPI  CSessCache();
private:
	virtual int SLAPI FetchEntry(PPID, ObjCacheEntry * pEntry, long extraData);
	virtual int SLAPI EntryToData(const ObjCacheEntry * pEntry, void * pDataRec) const;
};

SLAPI CSessCache::CSessCache() : ObjCacheHash(PPOBJ_CSESSION, sizeof(Data), (1024*1024), 8)
{
}

int SLAPI CSessCache::FetchEntry(PPID id, ObjCacheEntry * pEntry, long extraData)
{
	int    ok = -1;
	PPObjCSession cs_obj;
	Data * p_cache_rec = (Data *)pEntry;
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

int SLAPI CSessCache::EntryToData(const ObjCacheEntry * pEntry, void * pDataRec) const
{
	CSessionTbl::Rec * p_data_rec = (CSessionTbl::Rec *)pDataRec;
	const Data * p_cache_rec = (const Data *)pEntry;
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
	return 1;
}

int SLAPI PPObjCSession::Fetch(PPID id, CSessionTbl::Rec * pRec)
{
	CSessCache * p_cache = GetDbLocalCachePtr <CSessCache> (PPOBJ_CSESSION);
	return p_cache ? p_cache->Get(id, pRec, 0) : Search(id, pRec);
}

IMPL_OBJ_DIRTY(PPObjCSession, CSessCache);
//
//
//
#if 0 // {

class PPCSessComplexImpExpParam : public PPImpExpParam {
public:
	SLAPI  PPCSessComplexImpExpParam(uint recId = 0, long flags = 0);
	int    SLAPI Clear();
	virtual int WriteIni(PPIniFile * pFile, const char * pSect) const;
	virtual int ReadIni(PPIniFile * pFile, const char * pSect, const StringSet * pExclParamList);
	virtual int SerializeConfig(int dir, PPConfigDatabase::CObjHeader & rHdr, SBuffer & rTail, SSerializeContext * pSCtx);

	enum {
		fSkinCSess  = 0x0001,
		fSkipCCheck = 0x0002,
		fSkipCCLine = 0x0004,
		fSkipCCPaym = 0x0008
	};
	long   Flags;
	SString CSessTag;
	SString CCheckTag;
	SString CCLineTag;
	SString CCPaymTag;
};
#endif // } 0

#if 0 // @v9.1.3 Подход провальный - полностью заменяется на PPPosProtocol {

IMPLEMENT_IMPEXP_HDL_FACTORY(CSESSCOMPLEX, PPCSessComplexImpExpParam);

SLAPI PPCSessComplexImpExpParam::PPCSessComplexImpExpParam(uint recId, long flags) : PPImpExpParam(recId, flags)
{
	Flags = 0;
}

int SLAPI PPCSessComplexImpExpParam::Clear()
{
	Flags = 0;
	CSessTag = 0;
	CCheckTag = 0;
	CCLineTag = 0;
	CCPaymTag = 0;
	return 1;
}

int PPCSessComplexImpExpParam::WriteIni(PPIniFile * pFile, const char * pSect) const
{
	int    ok = 1;
	long   flags = 0;
	SString params, fld_name, param_val;
	THROW(PPImpExpParam::WriteIni(pFile, pSect));
	{
		THROW(PPLoadText(PPTXT_CSESSCPARAMS, params));
		if(Flags != 0) {
			PPGetSubStr(params, CSESSCPARAMS_FLAGS, fld_name);
			pFile->AppendParam(pSect, fld_name, (param_val = 0).Cat(Flags), 1);
		}
		{
			struct S {
				int    ID;
				const  SString & R_Val;
			} s_items[] = {
				{CSESSCPARAMS_CSESSTAG,  CSessTag},
				{CSESSCPARAMS_CCHECKTAG, CCheckTag},
				{CSESSCPARAMS_CCLINETAG, CCLineTag},
				{CSESSCPARAMS_CCPAYMTAG, CCPaymTag}
			};
			for(uint i = 0; i < SIZEOFARRAY(s_items); i++) {
				if((param_val = s_items[i].R_Val).NotEmptyS()) {
					PPGetSubStr(params, s_items[i].ID, fld_name);
					pFile->AppendParam(pSect, fld_name, param_val, 1);
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

int PPCSessComplexImpExpParam::ReadIni(PPIniFile * pFile, const char * pSect, const StringSet * pExclParamList)
{
	int    ok = 1;
	SString params, fld_name, param_val;
	StringSet excl;
	if(pExclParamList)
		excl = *pExclParamList;
	THROW(PPLoadText(PPTXT_CSESSCPARAMS, params));
	{
		PPGetSubStr(params, CSESSCPARAMS_FLAGS, fld_name);
		excl.add(fld_name);
		pFile->GetParam(pSect, fld_name, param_val);
		Flags = param_val.ToLong();
	}
	{
		struct S {
			int    ID;
			SString & R_Val;
		} s_items[] = {
			{CSESSCPARAMS_CSESSTAG,  CSessTag},
			{CSESSCPARAMS_CCHECKTAG, CCheckTag},
			{CSESSCPARAMS_CCLINETAG, CCLineTag},
			{CSESSCPARAMS_CCPAYMTAG, CCPaymTag}
		};
		for(uint i = 0; i < SIZEOFARRAY(s_items); i++) {
			s_items[i].R_Val = 0;
			if(PPGetSubStr(params, s_items[i].ID, fld_name)) {
				excl.add(fld_name);
				pFile->GetParam(pSect, fld_name, param_val);
				s_items[i].R_Val = param_val.Strip();
			}
		}
	}
	THROW(PPImpExpParam::ReadIni(pFile, pSect, &excl));
	CATCHZOK
	return ok;
}

int PPCSessComplexImpExpParam::SerializeConfig(int dir, PPConfigDatabase::CObjHeader & rHdr, SBuffer & rTail, SSerializeContext * pSCtx)
{
	int    ok = 1;
	SString temp_buf;
	StrAssocArray param_list;
	struct S {
		int    ID;
		SString & R_Val;
	} s_items[] = {
		{CSESSCPARAMS_CSESSTAG,  CSessTag},
		{CSESSCPARAMS_CCHECKTAG, CCheckTag},
		{CSESSCPARAMS_CCLINETAG, CCLineTag},
		{CSESSCPARAMS_CCPAYMTAG, CCPaymTag}
	};
	THROW(PPImpExpParam::SerializeConfig(dir, rHdr, rTail, pSCtx));
	if(dir > 0) {
		if(Flags) {
			param_list.Add(CSESSCPARAMS_FLAGS, (temp_buf = 0).Cat(Flags));
		}
		for(uint i = 0; i < SIZEOFARRAY(s_items); i++) {
			if((temp_buf = s_items[i].R_Val).NotEmptyS()) {
				param_list.Add(s_items[i].ID, temp_buf);
			}
		}
	}
	THROW_SL(pSCtx->Serialize(dir, param_list, rTail));
	if(dir < 0) {
		Clear();
		for(uint i = 0; i < param_list.getCount(); i++) {
			StrAssocArray::Item item = param_list.at_WithoutParent(i);
			temp_buf = item.Txt;
			if(item.Id == CSESSCPARAMS_FLAGS) {
				Flags = temp_buf.ToLong();
			}
			else {
				for(uint j = 0; j < SIZEOFARRAY(s_items); j++) {
					if(item.Id == s_items[j].ID) {
						s_items[j].R_Val = temp_buf.Strip();
						break;
					}
				}
			}
		}
	}
	CATCHZOK
	return ok;
}
//
//
//
CSessComplexImpExpDialog::CSessComplexImpExpDialog() : ImpExpParamDialog(DLG_IMPEXPCSC, 0)
{
}

int CSessComplexImpExpDialog::setDTS(const PPCSessComplexImpExpParam * pData)
{
	Data = *pData;
	ImpExpParamDialog::setDTS(&Data);
	{
		setCtrlString(CTL_IMPEXPCSC_CSESSTAG,  Data.CSessTag);
		setCtrlString(CTL_IMPEXPCSC_CCHECKTAG, Data.CCheckTag);
		setCtrlString(CTL_IMPEXPCSC_CCLINETAG, Data.CCLineTag);
		setCtrlString(CTL_IMPEXPCSC_CCPAYMTAG, Data.CCPaymTag);
	}
	return 1;
}

int CSessComplexImpExpDialog::getDTS(PPCSessComplexImpExpParam * pData)
{
	int    ok = 1;
	uint   sel = 0;
	THROW(ImpExpParamDialog::getDTS(&Data));
	{
		getCtrlString(CTL_IMPEXPCSC_CSESSTAG,  Data.CSessTag);
		getCtrlString(CTL_IMPEXPCSC_CCHECKTAG, Data.CCheckTag);
		getCtrlString(CTL_IMPEXPCSC_CCLINETAG, Data.CCLineTag);
		getCtrlString(CTL_IMPEXPCSC_CCPAYMTAG, Data.CCPaymTag);
	}
	ASSIGN_PTR(pData, Data);
	CATCH
		ok = PPErrorByDialog(this, sel);
	ENDCATCH
	return ok;
}

int SLAPI EditCSessComplexImpExpParam(const char * pIniSection)
{
	int    ok = -1;
	CSessComplexImpExpDialog * dlg = 0;
	PPCSessComplexImpExpParam param;
	SString ini_file_name, sect;
   	THROW(PPGetFilePath(PPPATH_BIN, PPFILNAM_IMPEXP_INI, ini_file_name));
   	{
   		int    direction = 0;
   		PPIniFile ini_file(ini_file_name, 0, 1, 1);
   		THROW(CheckDialogPtr(&(dlg = new CSessComplexImpExpDialog())));
   		THROW(LoadSdRecord(PPREC_CSESSCOMPLEX, &param.InrRec));
   		direction = param.Direction;
   		if(!isempty(pIniSection))
   			THROW(param.ReadIni(&ini_file, pIniSection, 0));
   		dlg->setDTS(&param);
   		while(ok <= 0 && ExecView(dlg) == cmOK)
   			if(dlg->getDTS(&param)) {
   				int is_new = (pIniSection && *pIniSection && param.Direction == direction) ? 0 : 1;
   				if(!isempty(pIniSection))
   					if(is_new)
   						ini_file.RemoveSection(pIniSection);
   					else
   						ini_file.ClearSection(pIniSection);
   				PPErrCode = PPERR_DUPOBJNAME;
   				if((!is_new || ini_file.IsSectExists(param.Name) == 0) && param.WriteIni(&ini_file, param.Name) && ini_file.FlashIniBuf())
   					ok = 1;
   				else
   					PPError();
   			}
   			else
   				PPError();
   	}
	CATCHZOKPPERR
   	delete dlg;
   	return ok;
}

int EditCSessComplexImpExpParams()
{
	int    ok = -1;
	PPCSessComplexImpExpParam param;
	CSessComplexImpExpDialog * p_dlg = new CSessComplexImpExpDialog();
	THROW(CheckDialogPtr(&p_dlg));
	THROW(ok = EditImpExpParams(PPFILNAM_IMPEXP_INI, PPREC_CSESSCOMPLEX, &param, p_dlg));
	CATCHZOK
	delete p_dlg;
	return ok;
}

class PrcssrCSessComplexExport {
public:
	SLAPI  PrcssrCSessComplexExport();
	SLAPI ~PrcssrCSessComplexExport();
	int    SLAPI Init(const PPCSessComplexImpExpParam * pParam);
	int    SLAPI ExportSession(PPID sessID);
private:
	PPCSessComplexImpExpParam Param;
	PPImpExp * P_IeCSess;
	PPImpExp * P_IeCCheck;
	PPImpExp * P_IeCCLine;
	PPImpExp * P_IeCCPaym;
	int    OneXmlOut;
	PPObjCSession CsObj;
	PPObjGoods GObj;
	CCheckCore Cc;
};

SLAPI PrcssrCSessComplexExport::PrcssrCSessComplexExport()
{
	P_IeCSess = 0;
	P_IeCCheck = 0;
	P_IeCCLine = 0;
	P_IeCCPaym = 0;
	OneXmlOut = 0;
}

SLAPI PrcssrCSessComplexExport::~PrcssrCSessComplexExport()
{
	delete P_IeCSess;
	delete P_IeCCheck;
	delete P_IeCCLine;
	delete P_IeCCPaym;
}

int SLAPI PrcssrCSessComplexExport::Init(const PPCSessComplexImpExpParam * pParam)
{
	int    ok = 1;
	if(pParam)
		Param = *pParam;
	else {
		THROW(LoadSdRecord(PPREC_CSESSCOMPLEX, &Param.InrRec));
		//ok = SelectGoodsImportCfgs(&Param, 0);
	}
	if(ok > 0) {
		if(Param.DataFormat == Param.dfXml)
			OneXmlOut = 1;
		if(Param.CSessTag.NotEmpty()) {
			THROW_MEM(P_IeCSess = new PPImpExp(&Param, 0));
			THROW(P_IeCSess->OpenFileForWriting(0, 1));
		}
		if(Param.CCheckTag.NotEmpty()) {
			if(!(OneXmlOut && P_IeCSess)) {
				THROW_MEM(P_IeCCheck = new PPImpExp(&Param, 0));
				THROW(P_IeCCheck->OpenFileForWriting(0, 1));
			}
		}
		if(Param.CCLineTag.NotEmpty()) {
			if(!(OneXmlOut && (P_IeCSess || P_IeCCheck))) {
				THROW_MEM(P_IeCCLine = new PPImpExp(&Param, 0));
				THROW(P_IeCCLine->OpenFileForWriting(0, 1));
			}
		}
		if(Param.CCPaymTag.NotEmpty()) {
			if(!(OneXmlOut && (P_IeCSess || P_IeCCheck))) {
				THROW_MEM(P_IeCCPaym = new PPImpExp(&Param, 0));
				THROW(P_IeCCPaym->OpenFileForWriting(0, 1));
			}
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI PrcssrCSessComplexExport::ExportSession(PPID sessID)
{
	int    ok = 1;
	SString temp_buf;
	CSessionTbl::Rec csess_rec;
	Sdr_CSessComplex sdr_csess, sdr_ccheck, sdr_paym, sdr_ccline;
	if(CsObj.Search(sessID, &csess_rec) > 0) {
		MEMSZERO(sdr_csess);
		sdr_csess.PosNodeID = csess_rec.CashNodeID;
		sdr_csess.PosN = csess_rec.CashNumber;
		sdr_csess.SessN = csess_rec.SessNumber;
		sdr_csess.SessID = csess_rec.ID;
		sdr_csess.SessDate = csess_rec.Dt;
		sdr_csess.SessTime = csess_rec.Tm;
		sdr_csess.SessAmt  = csess_rec.Amount;
		sdr_csess.SessDscnt = csess_rec.Discount;
		sdr_csess.BnkAmt = csess_rec.BnkAmount;

		if(P_IeCSess) {
			THROW(P_IeCSess->AppendRecord(&sdr_csess, sizeof(sdr_csess)));
		}
		{
			CCheckTbl::Key3 k3;
			BExtQuery q(&Cc, 3);
			MEMSZERO(k3);
			k3.SessID = sessID;
			q.selectAll().where(Cc.SessID == sessID);
			for(q.initIteration(0, &k3, spGe); q.nextIteration();) {
				CCheckPacket cc_pack;
				if(Cc.LoadPacket(Cc.data.ID, 0, &cc_pack) > 0) {
					MEMSZERO(sdr_ccheck);
					sdr_ccheck = sdr_csess;
					sdr_ccheck.CheckID = cc_pack.Rec.ID;
					sdr_ccheck.CheckN  = cc_pack.Rec.Code;
					sdr_ccheck.CheckDate = cc_pack.Rec.Dt;
					sdr_ccheck.CheckTime = cc_pack.Rec.Tm;
					sdr_ccheck.CcAmt     = MONEYTOLDBL(cc_pack.Rec.Amount);
					sdr_ccheck.CcDscnt   = MONEYTOLDBL(cc_pack.Rec.Discount);
					//sdr_ccheck.CcBnkAmt     double       format(13.2) "Сумма оплаты чека по банковской карте";
					//sdr_ccheck.CcCrdAmt     double       format(13.2) "Сумма оплаты чека по корпоративной карте";
					if(cc_pack.Rec.SCardID) {
						SCardTbl::Rec sc_rec;
						if(Cc.Cards.Search(cc_pack.Rec.SCardID, &sc_rec) > 0) {
							STRNSCPY(sdr_ccheck.SCardN, sc_rec.Code);
						}
						else {
							ideqvalstr(cc_pack.Rec.SCardID, temp_buf = 0).CopyTo(sdr_ccheck.SCardN, sizeof(sdr_ccheck.SCardN));
						}
					}
					sdr_ccheck.CcFlags = cc_pack.Rec.Flags;
					sdr_ccheck.IsRet = BIN(cc_pack.Rec.Flags & CCHKF_RETURN);
					sdr_ccheck.IsBnk = BIN(cc_pack.Rec.Flags & CCHKF_BANKING);
					if(P_IeCCheck) {
						THROW(P_IeCCheck->AppendRecord(&sdr_ccheck, sizeof(sdr_ccheck)));
					}
					else if(OneXmlOut && P_IeCSess) {
						THROW(P_IeCSess->AppendRecord(&sdr_ccheck, sizeof(sdr_ccheck)));
					}
					{
						CCheckItem ccl;
						for(uint lp = 0; cc_pack.EnumLines(&lp, &ccl);) {
							MEMSZERO(sdr_ccline);
							sdr_ccline = sdr_ccheck;
							sdr_ccline.GoodsID = ccl.GoodsID;
							STRNSCPY(sdr_ccline.Barcode, ccl.BarCode);
							STRNSCPY(sdr_ccline.Serial, ccl.Serial);
							sdr_ccline.LnQtty  = ccl.Quantity;
							sdr_ccline.LnPrice = ccl.Price;
							sdr_ccline.LnDscnt = ccl.Discount;
							if(P_IeCCLine) {
								THROW(P_IeCCLine->AppendRecord(&sdr_ccline, sizeof(sdr_ccline)));
							}
							else if(OneXmlOut) {
								if(P_IeCCheck) {
									THROW(P_IeCCheck->AppendRecord(&sdr_ccline, sizeof(sdr_ccline)));
								}
								else if(P_IeCSess) {
									THROW(P_IeCSess->AppendRecord(&sdr_ccline, sizeof(sdr_ccline)));
								}
							}
						}
					}
					{
						CcAmountList temp_al_list = cc_pack.AL_Const();
						if(!temp_al_list.getCount()) {
							if(cc_pack.Rec.Flags & CCHKF_BANKING) {
								temp_al_list.Add(CCAMTTYP_BANK, sdr_ccheck.CcAmt);
							}
							else if(cc_pack.Rec.Flags & CCHKF_INCORPCRD && cc_pack.Rec.SCardID) {
								temp_al_list.Add(CCAMTTYP_CRDCARD, sdr_ccheck.CcAmt, cc_pack.Rec.SCardID);
							}
							else {
								temp_al_list.Add(CCAMTTYP_CASH, sdr_ccheck.CcAmt);
							}
						}
						if(temp_al_list.getCount()) {
							for(uint j = 0; j < temp_al_list.getCount(); j++) {
								const CcAmountEntry & r_al_entry = temp_al_list.at(j);

								MEMSZERO(sdr_paym);
								sdr_paym = sdr_ccheck;

								sdr_paym.PaymType = (int16)r_al_entry.Type;
								sdr_paym.PaymAmt  = r_al_entry.Amount;
								if(r_al_entry.AddedID) {
									SCardTbl::Rec sc_rec;
									if(Cc.Cards.Search(r_al_entry.AddedID, &sc_rec) > 0) {
										STRNSCPY(sdr_paym.PaymCardN, sc_rec.Code);
									}
									else {
										ideqvalstr(cc_pack.Rec.SCardID, temp_buf = 0).CopyTo(sdr_paym.PaymCardN, sizeof(sdr_paym.PaymCardN));
									}
								}
								if(P_IeCCPaym) {
									THROW(P_IeCCLine->AppendRecord(&sdr_paym, sizeof(sdr_paym)));
								}
								else if(OneXmlOut) {
									if(P_IeCCheck) {
										THROW(P_IeCCheck->AppendRecord(&sdr_paym, sizeof(sdr_paym)));
									}
									else if(P_IeCSess) {
										THROW(P_IeCSess->AppendRecord(&sdr_paym, sizeof(sdr_paym)));
									}
								}
							}
						}
					}
				}
			}
		}
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

class PrcssrCSessComplexImport {
};

#endif // } 0 @v9.1.3 Подход провальный - полностью заменяется на PPPosProtocol
