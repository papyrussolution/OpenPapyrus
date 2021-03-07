// OBJATURN.CPP
// Copyright (c) A.Sobolev 1996-2000, 2001, 2002, 2003, 2004, 2005, 2006, 2009, 2010, 2013, 2015, 2016, 2017, 2019
//
#include <pp.h>
#pragma hdrstop

// Prototype
long FASTCALL ATTF_TO_ATDF(long attf); // Defined in objbill.cpp

TLP_IMPL(PPObjAccTurn, AccTurnCore, P_Tbl);

PPObjAccTurn::PPObjAccTurn(void * extraPtr) : PPObject(PPOBJ_ACCTURN), ExtraPtr(extraPtr)
{
	TLP_OPEN(P_Tbl);
}

PPObjAccTurn::~PPObjAccTurn()
{
	TLP_CLOSE(P_Tbl);
}

int PPObjAccTurn::ConvertStr(const char * pStr, PPID curID, Acct * pAcct, AcctID * pAcctId, PPID * pSheetID)
	{ return P_Tbl->ConvertStr(pStr, curID, pAcct, pAcctId, pSheetID); }
int PPObjAccTurn::ConvertAcct(const Acct * pAcct, PPID curID, AcctID * pAcctId, PPID * pAccSheetID)
	{ return P_Tbl->ConvertAcct(pAcct, curID, pAcctId, pAccSheetID); }
int PPObjAccTurn::ConvertAcctID(const AcctID & rAci, Acct * acct, PPID * pCurID, int useCache)
	{ return P_Tbl->ConvertAcctID(rAci, acct, pCurID, useCache); }

int PPObjAccTurn::VerifyRevokingCurFromAccount(PPID accID, PPID curID)
{
	PPID   cur_acc_id = 0;
	if(P_Tbl->AccObj.SearchCur(accID, curID, &cur_acc_id, 0) > 0)
		if(ReplyAccDel(cur_acc_id) != DBRPL_OK)
			return 0;
	return 1;
}

int PPObjAccTurn::VerifyChangingAccsheetOfAccount(PPID accID)
{
	int    ok = 1;
	PPAccount acc_rec;
	if(accID && P_Tbl->AccObj.Search(accID, &acc_rec) > 0 && acc_rec.AccSheetID) {
		AcctRelTbl::Rec acr_rec;
		for(SEnum en = P_Tbl->AccRel.EnumByAcc(accID); en.Next(&acr_rec) > 0;) {
			if(!acr_rec.Closed) {
				AccTurnTbl::Key1 atk;
				MEMSZERO(atk);
				atk.Acc = acr_rec.ID;
				if(P_Tbl->search(1, &atk, spGt) && atk.Acc == acr_rec.ID) {
					ok = 0;
					break;
				}
			}
		}
	}
	return ok;
}

int PPObjAccTurn::CreateBlankAccTurn(PPID opID, PPBillPacket * pPack, long * pFlags, int use_ta)
{
	int       ok = 1;
	long      f = 0;
	PPAccTurnTemplArray att_list;
	PPAccTurn at;
	THROW(pPack->CreateBlank(opID, 0, 0, use_ta));
	pPack->CreateAccTurn(&at);
	THROW(PPObjOprKind::GetATTemplList(opID, &att_list));
	if(att_list.getCount()) {
		PPAccTurnTempl & r_tmpl = att_list.at(0);
		f = ATTF_TO_ATDF(r_tmpl.Flags);
		at.DbtID = r_tmpl.DbtID;
		at.CrdID = r_tmpl.CrdID;
		THROW(P_Tbl->AccObj.InitAccSheetForAcctID(&at.DbtID, &at.DbtSheet));
		THROW(P_Tbl->AccObj.InitAccSheetForAcctID(&at.CrdID, &at.CrdSheet));
	}
	THROW_SL(pPack->Turns.insert(&at));
	CATCHZOK
	ASSIGN_PTR(pFlags, f);
	return ok;
}

int PPObjAccTurn::CreateBlankAccTurnBySample(PPBillPacket * pPack, const PPBillPacket * pSamplePack, long * pFlags)
{
	int       ok = 1;
	long      f = 0;
	PPAccTurnTemplArray att_list;
	PPAccTurn at;
	const PPAccTurn * p_sample_at = 0;
	THROW(pPack->CreateBlankBySample(pSamplePack->Rec.ID, 1));
	pPack->CreateAccTurn(&at);
	if(pSamplePack->Turns.getCount() > 0)
		p_sample_at = & pSamplePack->Turns.at(0);
	THROW(PPObjOprKind::GetATTemplList(pSamplePack->Rec.OpID, &att_list));
	if(att_list.getCount()) {
		PPAccTurnTempl & r_tmpl = att_list.at(0);
		f = ATTF_TO_ATDF(r_tmpl.Flags);
		at.DbtID = r_tmpl.DbtID;
		at.CrdID = r_tmpl.CrdID;
	}
	if(p_sample_at) {
		at.DbtID  = p_sample_at->DbtID;
		at.CrdID  = p_sample_at->CrdID;
		at.CurID  = p_sample_at->CurID;
		at.CRate  = p_sample_at->CRate;
		at.Amount = p_sample_at->Amount;
	}
	THROW(P_Tbl->AccObj.InitAccSheetForAcctID(&at.DbtID, &at.DbtSheet));
	THROW(P_Tbl->AccObj.InitAccSheetForAcctID(&at.CrdID, &at.CrdSheet));
	THROW_SL(pPack->Turns.insert(&at));
	CATCHZOK
	ASSIGN_PTR(pFlags, f);
	return ok;
}

int PPObjAccTurn::SearchAccturnInPacketByCorrAcc(const PPBillPacket * pPack, int side, int ac, Acct * pCorrAcc, uint * pPos)
{
	for(uint i = 0; i < pPack->Turns.getCount(); i++) {
		Acct   dbt, crd;
		PPID   cur_id = 0;
		ConvertAcctID(pPack->Turns.at(i).DbtID, &dbt, &cur_id, 1 /* useCache */);
		ConvertAcctID(pPack->Turns.at(i).CrdID, &crd, &cur_id, 1 /* useCache */);
		if(side == PPDEBIT && dbt.ac == ac) {
			ASSIGN_PTR(pCorrAcc, crd);
			ASSIGN_PTR(pPos, i);
			return 1;
		}
		else if(side == PPCREDIT && crd.ac == ac) {
			ASSIGN_PTR(pCorrAcc, dbt);
			ASSIGN_PTR(pPos, i);
			return 1;
		}
	}
	return 0;
}

int PPObjAccTurn::HandleMsg(int msg, PPID _obj, PPID _id, void * extraPtr)
{
	if(msg == DBMSG_OBJDELETE)
		switch(_obj) {
			case PPOBJ_ACCOUNT2: return ReplyAccDel(_id);
			case PPOBJ_ARTICLE: return ReplyArticleDel(_id);
		}
	else if(msg == DBMSG_OBJREPLACE)
		if(_obj == PPOBJ_ARTICLE)
			return P_Tbl->ReplaceArticle(_id, reinterpret_cast<long>(extraPtr)) ? DBRPL_OK : DBRPL_ERROR;
	return DBRPL_OK;
}

int PPObjAccTurn::ReplyAccDel(PPID _id)
{
	int    ok = DBRPL_OK;
	int    r = P_Tbl->SearchAccRef(_id, 1);
	if(r > 0)
		ok = RetRefsExistsErr(Obj, 0);
	else if(r == 0)
		ok = DBRPL_ERROR;
	return ok;
}

int PPObjAccTurn::ReplyArticleDel(PPID _id)
{
	int    ok = DBRPL_OK;
	PPID   bill_id = 0;
	int    r = P_Tbl->SearchArticleRef(_id, 1, &bill_id);
	if(r > 0) {
		ok = RetRefsExistsErr(PPOBJ_BILL, bill_id);
	}
	else if(r == 0)
		ok = DBRPL_ERROR;
	return ok;
}

int PPObjAccTurn::EditRecoverBalanceParam(RecoverBalanceParam * pParam)
{
	int    ok = -1, valid_data = 0;
	Acct   acct;
	PPID   cur_id = 0;
	AcctID acct_id;
	TDialog * dlg = 0;
	acct_id.ac = pParam->BalAccID;
	acct_id.ar = 0;
	MEMSZERO(acct);
	THROW(CheckDialogPtr(&(dlg = new TDialog(DLG_CBAL))));
	FileBrowseCtrlGroup::Setup(dlg, CTLBRW_CBAL_LOG, CTL_CBAL_LOG, 1, 0, 0, FileBrowseCtrlGroup::fbcgfLogFile);
	dlg->SetupCalPeriod(CTLCAL_CBAL_PERIOD, CTL_CBAL_PERIOD);
	SetPeriodInput(dlg, CTL_CBAL_PERIOD, &pParam->Period);
	ConvertAcctID(acct_id, &acct, &cur_id, 1 /* useCache */);
	dlg->setCtrlData(CTL_CBAL_BAL, &acct);
	dlg->setCtrlString(CTL_CBAL_LOG, pParam->LogFileName);
	dlg->AddClusterAssoc(CTL_CBAL_FLAGS, 0, RecoverBalanceParam::fCorrect);
	dlg->SetClusterData(CTL_CBAL_FLAGS, pParam->Flags);
	while(!valid_data && ExecView(dlg) == cmOK) {
		if(!GetPeriodInput(dlg, CTL_CBAL_PERIOD, &pParam->Period))
			PPErrorByDialog(dlg, CTL_CBAL_PERIOD);
		else {
			PPAccount acc_rec;
			dlg->getCtrlData(CTL_CBAL_BAL, &acct);
			acct.ar = 0;
			if((acct.ac || acct.sb) && P_Tbl->AccObj.SearchNum(acct.ac, acct.sb, 0L/*@curID*/, &acc_rec) <= 0)
				PPErrorByDialog(dlg, CTL_CBAL_BAL);
			else {
				pParam->BalAccID = (acct.ac || acct.sb) ? acc_rec.ID : 0;
				dlg->GetClusterData(CTL_CBAL_FLAGS, &pParam->Flags);
				dlg->getCtrlString(CTL_CBAL_LOG, pParam->LogFileName);
				ok = valid_data = 1;
			}
		}
	}
	CATCHZOKPPERR
	delete dlg;
	return ok;
}

int PPObjAccTurn::CorrectBalance()
{
	int    ok = -1;
	PPLogger logger;
	RecoverBalanceParam param;
	PPGetFileName(PPFILNAM_BALANCE_LOG, param.LogFileName);
	param.Flags |= RecoverBalanceParam::fCorrect;
	if(EditRecoverBalanceParam(&param) > 0) {
		PPWaitStart();
		THROW(P_Tbl->RecalcBalance(&param, logger));
		PPWaitStop();
		ok = 1;
	}
	CATCHZOKPPERR
	logger.Save(param.LogFileName, 0);
	return ok;
}

int PPObjAccTurn::CorrectRelsArRefs()
{
	PPID   k = 0;
	IterCounter cntr;
	PPWaitStart();
	ArticleCore & r_arc = P_Tbl->Art;
	for(PPInitIterCounter(cntr, &r_arc); r_arc.search(0, &k, spGt);) {
		if(!P_Tbl->UpdateRelsArRef(r_arc.data.ID, r_arc.data.Article, 1)) {
			PPError();
			PPWaitStart();
		}
		PPWaitPercent(cntr.Increment());
	}
	PPWaitStop();
	return 1;
}
