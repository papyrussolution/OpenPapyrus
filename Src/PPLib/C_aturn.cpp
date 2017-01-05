// C_ATURN.CPP
// Copyright (c) A.Sobolev 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2005, 2006, 2007, 2016
//
// Процедуры корректировки бух. проводок
//
#include <pp.h>
#pragma hdrstop

struct CorrectAccturnRestParam {
	PPLogger Logger;
	int    Correct;
};

static int ReplyProc(PPID accRelID, LDATE dt, long oprno, double delta, void * paramPtr)
{
	CorrectAccturnRestParam * p = (CorrectAccturnRestParam *)paramPtr;
	if(p) {
		SString buf, buf1, buf2;
		PPLoadString(PPMSG_INFORMATION, PPINF_INVATURNREST, buf2);
		buf1.Cat(accRelID).Space().Cat(oprno).Space().Cat(dt).Space().
			CatChar('(').Cat(delta, MKSFMTD(0, 12, NMBF_NOTRAILZ)).CatChar(')');
		buf.Printf(buf2, (const char *)buf1);
		p->Logger.Log(buf.Printf(buf2, (const char *)buf1));
		return p->Correct;
	}
	else
		return -1;
}

int SLAPI CorrectAccTurnRest()
{
	int    ok = -1;
	ushort v;
	SString log_file_name, acc_buf;
	CorrectAccturnRestParam param;
	param.Correct = 1;
	TDialog * dlg = new TDialog(DLG_CORCREST);
	THROW(CheckDialogPtr(&dlg, 0));
	FileBrowseCtrlGroup::Setup(dlg, CTLBRW_CORCREST_LOG, CTL_CORCREST_LOG, 1, 0, 0, FileBrowseCtrlGroup::fbcgfLogFile);
	PPLoadText(PPTXT_CORRECTACCREST, log_file_name);
	dlg->setTitle(log_file_name);
	log_file_name = 0;
	dlg->setCtrlString(CTL_CORCREST_LOG, log_file_name);
	dlg->setCtrlUInt16(CTL_CORCREST_FLAGS, BIN(param.Correct > 0));
	if(ExecView(dlg) == cmOK) {
		dlg->getCtrlString(CTL_CORCREST_LOG, log_file_name);
		dlg->getCtrlData(CTL_CORCREST_FLAGS, &v);
		log_file_name.Strip();
		param.Correct = v ? 1 : -1;

		PPObjAccTurn atobj;
		AcctRel * p_arel = &atobj.P_Tbl->AccRel;
		PPID   acc = 0;
		long   count = 0;
		PPWait(1);
		if(p_arel->search(0, &acc, spFirst))
			do {
				count++;
				PPWaitMsg(((const Acct *)&p_arel->data.Ac)->ToStr(ACCF_DEFAULT, acc_buf));
				THROW(atobj.P_Tbl->RecalcRest(acc, ZERODATE, ReplyProc, &param, 1));
			} while(p_arel->search(&acc, spNext));
		THROW_DB(BTROKORNFOUND);
		param.Logger.Save(log_file_name, 0);
		PPWait(0);
	}
	CATCHZOKPPERR
	delete dlg;
	return ok;
}

int SLAPI RemoveEmptyAcctRels()
{
	PPWait(1);
	PPObjAccTurn atobj;
	int    ok = atobj.P_Tbl->RemoveEmptyAcctRels();
	if(!ok)
		PPError();
	PPWait(0);
	return ok;
}
//
//
//
struct CorrectAccturnParam {
	PPLogger Logger;
};

static int CorrectAccturnMsgProc(int msgCode, PPID accID, PPID billID, LDATE dt, long oprno, void * paramPtr)
{
	int    ok = 1;
	if(PPCheckUserBreak()) {
		CorrectAccturnParam * p = (CorrectAccturnParam *)paramPtr;
		if(p) {
			SString buf, buf1, buf2;
			PPLoadString(PPMSG_ERROR, msgCode, buf1);
			buf2.Cat(dt).Space().CatEq("OprNo", oprno).Space().CatEq("Acc", accID).Space().CatEq("BillID", billID);
			p->Logger.Log(buf.Printf(buf1, (const char *)buf2));
		}
	}
	else
		ok = -1;
	return ok;
}

int SLAPI CorrectAccturn()
{
	int    ok = -1;
	long   flags = 0;
	SString log_fname;
	CorrectAccturnParam param;
	TDialog * dlg = new TDialog(DLG_CORATURN);
	THROW(CheckDialogPtr(&dlg, 0));
	FileBrowseCtrlGroup::Setup(dlg, CTLBRW_CORATURN_LOG, CTL_CORATURN_LOG, 1, 0, 0, FileBrowseCtrlGroup::fbcgfLogFile);
	PPGetFileName(PPFILNAM_ACCTURN_LOG, log_fname);
	dlg->setCtrlString(CTL_CORATURN_LOG, log_fname);
	dlg->AddClusterAssoc(CTL_CORATURN_FLAGS, 0, ATRPRF_CHECKBILLLINK);
	dlg->AddClusterAssoc(CTL_CORATURN_FLAGS, 1, ATRPRF_CHECKACCLINK);
	dlg->AddClusterAssoc(CTL_CORATURN_FLAGS, 2, ATRPRF_REPAIRMIRROR);
	dlg->AddClusterAssoc(CTL_CORATURN_FLAGS, 3, ATRPRF_RMVZEROBILLLINK);
	dlg->AddClusterAssoc(CTL_CORATURN_FLAGS, 4, ATRPRF_RMVZEROACCLINK);
	dlg->SetClusterData(CTL_CORATURN_FLAGS, flags);
	if(ExecView(dlg) == cmOK) {
		dlg->getCtrlString(CTL_CORATURN_LOG, log_fname);
		dlg->GetClusterData(CTL_CORATURN_FLAGS, &flags);
		ok = 1;
	}
	if(ok > 0) {
		PPObjAccTurn atobj;
		PPWait(1);
		THROW(atobj.P_Tbl->Repair(flags, CorrectAccturnMsgProc, &param));
		PPWait(0);
	}
	CATCHZOKPPERR
	param.Logger.Save(log_fname, 0);
	return ok;
}
