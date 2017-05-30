// V_CHKOPJ.CPP
// Copyright (c) A.Starodub 2010, 2013, 2015, 2016
//
#include <pp.h>
#pragma hdrstop
//
// Журнал чековых операций
//
SLAPI CheckOpJrnl::CheckOpJrnl(CCheckCore * pCc) : CheckOpJrnlTbl()
{
	P_Cc = pCc;
}

SLAPI CheckOpJrnl::~CheckOpJrnl()
{
	P_Cc = 0; // @notowned
}

int SLAPI CheckOpJrnl::LogEvent(int16 action, const CCheckPacket * pPack, const CCheckLineTbl::Rec * pLineRec, int useTa /*= 0*/)
{
	int    ok = 1;
	CheckOpJrnlTbl::Rec log_rec;
	PPID   node_id = 0; // @vmiller
	{
		PPTransaction tra(useTa);
		THROW(tra);
		MEMSZERO(log_rec);
		log_rec.UserID    = LConfig.User;
		log_rec.Action    = action + 1;
		getcurdatetime(&log_rec.Dt, &log_rec.Tm);
		log_rec.CheckNum  = pPack->Rec.Code;
		log_rec.CheckID   = pPack->Rec.ID;
		log_rec.Price     = pLineRec ? pLineRec->Price : 0.0f;
		log_rec.Summ      = (float)MONEYTOLDBL(pPack->Rec.Amount);
		log_rec.GoodsID   = (pLineRec) ? pLineRec->GoodsID : 0;
		log_rec.PrinterID = 0;
		log_rec.AgentID   = pPack->Ext.SalerID; // @vmiller
		// @vmiller {
		if(P_Cc && P_Cc->GetNodeID(log_rec.CheckID, &node_id) > 0)
			log_rec.PosNodeID = node_id;
		// } @vmiller
		while(Search(log_rec.Dt, log_rec.Tm, 0) > 0)
			log_rec.Tm.addhs(1);
		THROW_DB(insertRecBuf(&log_rec));
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int SLAPI CheckOpJrnl::Search(LDATE dt, LTIME tm, CheckOpJrnlTbl::Rec * pRec)
{
	int    ok = -1;
	CheckOpJrnlTbl::Key0 k0;
	MEMSZERO(k0);
	k0.Dt = dt;
	k0.Tm = tm;
	if(search(0, &k0, spEq) > 0) {
		ok = 1;
		ASSIGN_PTR(pRec, data);
	}
	return ok;
}

IMPLEMENT_PPFILT_FACTORY(CheckOpJrnl); SLAPI CheckOpJrnlFilt::CheckOpJrnlFilt() : PPBaseFilt(PPFILT_CHECKOPJRNL, 0, 1)
{
	SetFlatChunk(offsetof(CheckOpJrnlFilt, ReserveStart),
		offsetof(CheckOpJrnlFilt, ActionIDList)-offsetof(CheckOpJrnlFilt, ReserveStart));
	SetBranchSArray(offsetof(CheckOpJrnlFilt, ActionIDList));
	Init(1, 0);
}

int SLAPI CheckOpJrnlFilt::IsEmpty() const
{
	if(!Period.IsZero())
		return 0;
	else if(UserID)
		return 0;
	else if(ActionIDList.getCount())
		return 0;
	else
		return 1;
}
//
//
//
class CheckOpJFiltDialog : public TDialog {
public:
	CheckOpJFiltDialog();
	int    setDTS(const CheckOpJrnlFilt *);
	int    getDTS(CheckOpJrnlFilt *);
private:
	DECL_HANDLE_EVENT;

	StrAssocArray ActionList;
	CheckOpJrnlFilt   Filt;
};

CheckOpJFiltDialog::CheckOpJFiltDialog() : TDialog(DLG_CHKOPJFILT)
{
	SetupCalPeriod(CTLCAL_CHKOPJFILT_PERIOD, CTL_CHKOPJFILT_PERIOD);
	{
		SString actions_buf, temp_buf;
		PPLoadText(PPTXT_CHKOPACTIONLIST, actions_buf);
		StringSet ss(';', actions_buf);
		for(uint p = 0, j = 0; ss.get(&p, temp_buf) > 0;j++)
			ActionList.Add((long)(j + 1), temp_buf);
	}
}

IMPL_HANDLE_EVENT(CheckOpJFiltDialog)
{
	TDialog::handleEvent(event);
	if(event.isCmd(cmChkOpJActionList)) {
		ListToListData data(&ActionList, 0, &Filt.ActionIDList);
		data.TitleStrID = PPTXT_TITLE_CHKOPACTIONLIST;
		if(ListToListDialog(&data) > 0) {
			if(Filt.ActionIDList.isList()) {
				SetComboBoxListText(this, CTLSEL_CHKOPJFILT_ACTION);
				disableCtrl(CTLSEL_CHKOPJFILT_ACTION, 1);
			}
			else {
				setCtrlLong(CTLSEL_CHKOPJFILT_ACTION, Filt.ActionIDList.getSingle());
				disableCtrl(CTLSEL_CHKOPJFILT_ACTION, 0);
			}
		}
	}
	else if(event.isCbSelected(CTLSEL_CHKOPJFILT_ACTION))
		Filt.ActionIDList.setSingleNZ(getCtrlLong(CTLSEL_CHKOPJFILT_ACTION));
	else
		return;
	clearEvent(event);
}

int CheckOpJFiltDialog::setDTS(const CheckOpJrnlFilt * pFilt)
{
	Filt = *pFilt;
	SetPeriodInput(this, CTL_CHKOPJFILT_PERIOD, &Filt.Period);
	SetupPPObjCombo(this, CTLSEL_CHKOPJFILT_USER, PPOBJ_USR, Filt.UserID, 0, 0);
	SetupStrAssocCombo(this, CTLSEL_CHKOPJFILT_ACTION, &ActionList, 0, 0);
	if(Filt.ActionIDList.isList()) {
		SetComboBoxListText(this, CTLSEL_CHKOPJFILT_ACTION);
		disableCtrl(CTLSEL_CHKOPJFILT_ACTION, 1);
	}
	else {
		setCtrlLong(CTLSEL_CHKOPJFILT_ACTION, Filt.ActionIDList.getSingle());
		disableCtrl(CTLSEL_CHKOPJFILT_ACTION, 0);
	}
	SetupArCombo(this, CTLSEL_CHKOPJFILT_AGENT, Filt.AgentID, OLW_LOADDEFONOPEN, GetAgentAccSheet(), sacfDisableIfZeroSheet);
	return 1;
}

int CheckOpJFiltDialog::getDTS(CheckOpJrnlFilt * pFilt)
{
	int    ok = 1;
	if(!GetPeriodInput(this, CTL_CHKOPJFILT_PERIOD, &Filt.Period))
		ok = PPErrorByDialog(this, CTL_CHKOPJFILT_PERIOD);
	else {
		getCtrlData(CTLSEL_CHKOPJFILT_USER, &Filt.UserID);
		if(!Filt.ActionIDList.isList())
			Filt.ActionIDList.setSingleNZ(getCtrlLong(CTLSEL_CHKOPJFILT_ACTION));
		getCtrlData(CTLSEL_CHKOPJFILT_AGENT, &Filt.AgentID); // @vmiller
		*pFilt = Filt;
	}
	return ok;
}
//
//
//
PPBaseFilt * SLAPI PPViewCheckOpJrnl::CreateFilt(void * extraPtr) const
{
	CheckOpJrnlFilt * p_filt = new CheckOpJrnlFilt;
	p_filt->Period.SetDate(getcurdate_());
	return p_filt;
}

int SLAPI PPViewCheckOpJrnl::EditBaseFilt(PPBaseFilt * pBaseFilt)
{
	if(!Filt.IsA(pBaseFilt))
		return 0;
	CheckOpJrnlFilt * p_filt = (CheckOpJrnlFilt *)pBaseFilt;
	DIALOG_PROC_BODYERR(CheckOpJFiltDialog, p_filt);
}

SLAPI PPViewCheckOpJrnl::PPViewCheckOpJrnl() : PPView(0, &Filt, PPVIEW_CHECKOPJRNL)
{
	P_Tbl = new CheckOpJrnl(0);
}

SLAPI PPViewCheckOpJrnl::~PPViewCheckOpJrnl()
{
	delete P_Tbl;
}

PP_CREATE_TEMP_FILE_PROC(CreateTempFile, SysJournal);

int FASTCALL PPViewCheckOpJrnl::CheckRecForFilt(const CheckOpJrnlTbl::Rec * pRec)
{
	int    ok = 1;
	if(pRec) {
		if(Filt.UserID && pRec->UserID != Filt.UserID)
			return 0;
		if(!Filt.Period.CheckDate(pRec->Dt))
			return 0;
		if(Filt.ActionIDList.getCount() && !Filt.ActionIDList.lsearch(pRec->Action))
			return 0;
	}
	else
		ok = 0;
	return ok;
}

int SLAPI PPViewCheckOpJrnl::Init_(const PPBaseFilt * pFilt)
{
	int    ok = 1;
	SString temp_buf;     // @vmiller
	PPObjCashNode cn_obj; // @vmiller
	Counter.Init();
	THROW(Helper_InitBaseFilt(pFilt));
	Filt.Period.Actualize(ZERODATE);
	CATCHZOK
	ZDELETE(P_IterQuery);
	return ok;
}

int SLAPI PPViewCheckOpJrnl::InitIteration()
{
	int    ok = 1;
	DBQ  * dbq = 0;
	CheckOpJrnlTbl::Key0 k0, ks;

	MEMSZERO(k0);
	ZDELETE(P_IterQuery);
	{
		P_IterQuery = new BExtQuery(P_Tbl, 0);
		P_IterQuery->selectAll();
		dbq = & daterange(P_Tbl->Dt, &Filt.Period);
		dbq = ppcheckfiltid(dbq, P_Tbl->UserID, Filt.UserID);
		dbq = ppcheckfiltidlist(dbq, P_Tbl->Action, &Filt.ActionIDList);
		P_IterQuery->where(*dbq);
		k0.Dt = Filt.Period.low;
		k0.Tm = Filt.BegTm;
	}
	ks = k0;
	Counter.Init(P_IterQuery->countIterations(0, &ks, spGe));
	P_IterQuery->initIteration(0, &k0, spGe);
	return ok;
}

int FASTCALL PPViewCheckOpJrnl::NextIteration(CheckOpJrnlViewItem * pItem)
{
	while(pItem && P_IterQuery && P_IterQuery->nextIteration() > 0) {
		P_Tbl->copyBufTo(pItem);
		if(!Filt.BegTm || P_Tbl->data.Dt > Filt.Period.low || P_Tbl->data.Tm >= Filt.BegTm) {
			Counter.Increment();
			PPWaitPercent(Counter);
			return 1;
		}
	}
	return -1;
}

DBQuery * SLAPI PPViewCheckOpJrnl::CreateBrowserQuery(uint * pBrwId, SString *)
{
	int    add_dbe = 0;
	uint   brw_id = BROWSER_CHKOPJ;
	DBQuery * q = 0;
	DBE    dbe_user;
	DBE    dbe_goods;
	DBE    dbe_action;
	DBE    dbe_saler; // @vmiller
	DBQ  * dbq = 0;
	CheckOpJrnlTbl * p_chkop_j  = new CheckOpJrnlTbl(0);
	CCheckExtTbl   * p_ext = 0;

	THROW(CheckTblPtr(p_chkop_j));
	PPDbqFuncPool::InitObjNameFunc(dbe_user,   PPDbqFuncPool::IdObjNameUser, p_chkop_j->UserID);
	PPDbqFuncPool::InitObjNameFunc(dbe_action, PPDbqFuncPool::IdChkOpJActionName, p_chkop_j->Action);
	PPDbqFuncPool::InitObjNameFunc(dbe_goods,  PPDbqFuncPool::IdObjNameGoods, p_chkop_j->GoodsID);
	PPDbqFuncPool::InitObjNameFunc(dbe_saler,  PPDbqFuncPool::IdObjNameAr, p_chkop_j->AgentID); // @vmiller

	dbq = & daterange(p_chkop_j->Dt, &Filt.Period);
	dbq = ppcheckfiltid(dbq, p_chkop_j->UserID, Filt.UserID);
	dbq = ppcheckfiltidlist(dbq, p_chkop_j->Action, &Filt.ActionIDList);
	dbq = ppcheckfiltid(dbq, p_chkop_j->AgentID, Filt.AgentID); // @vmiller
	if(Filt.BegTm)
		dbq = & (*dbq && p_chkop_j->Tm >= (long)Filt.BegTm);
	q = & select(
		p_chkop_j->Dt,       // #1
		p_chkop_j->Tm,       // #2
		p_chkop_j->Action,   // #3
		dbe_user,            // #4
		dbe_action,          // #5
		p_chkop_j->CheckNum, // #6
		dbe_goods,           // #7
		p_chkop_j->Price,    // #8
		p_chkop_j->Summ,     // #9
		dbe_saler,  // @vmiller
		p_chkop_j->PosNodeID,// @vmiller
		0L).from(p_chkop_j, 0L);
	q->where(*dbq);
	q->orderBy(p_chkop_j->Dt, p_chkop_j->Tm, 0L);
	THROW(CheckQueryPtr(q));
	CATCH
		if(q)
			ZDELETE(q);
		else {
			delete p_chkop_j;
		}
	ENDCATCH
	ASSIGN_PTR(pBrwId, brw_id);
	return q;
}

int SLAPI PPViewCheckOpJrnl::ProcessCommand(uint ppvCmd, const void * pHdr, PPViewBrowser * pBrw)
{
	int    ok = PPView::ProcessCommand(ppvCmd, pHdr, pBrw);
	if(ok == -2) {
		BrwHdr hdr;
		if(pHdr)
			hdr = *(PPViewCheckOpJrnl::BrwHdr *)pHdr;
		else
			MEMSZERO(hdr);
		switch(ppvCmd) {
			case PPVCMD_VIEWGOODS:
				if(P_Tbl) {
					CheckOpJrnlTbl::Rec rec;
					MEMSZERO(rec);
					if(P_Tbl->Search(hdr.Dt, hdr.Tm, &rec) > 0 && rec.GoodsID) {
						GObj.Edit(&rec.GoodsID, 0);
					}
				}
				break;
			case PPVCMD_VIEWCHECK:
				if(P_Tbl) {
					CheckOpJrnlTbl::Rec rec;
					MEMSZERO(rec);
					if(P_Tbl->Search(hdr.Dt, hdr.Tm, &rec) > 0 && rec.CheckID) {
						PPID   cn_id = 0;
						CC.GetNodeID(rec.CheckID, &cn_id);
						CCheckPane(cn_id, rec.CheckID);
					}
				}
				break;
		}
	}
	return ok;
}

int SLAPI PPViewCheckOpJrnl::Print(const void *)
{
	return Helper_Print(/*REPORT_CHECKOPJ*/0, 0);
}
//
// Implementation of PPALDD_CheckOpJrnl
//
PPALDD_CONSTRUCTOR(CheckOpJrnl)
{
	if(Valid) {
		AssignHeadData(&H, sizeof(H));
		AssignIterData(1, &I, sizeof(I));
	}
}

PPALDD_DESTRUCTOR(CheckOpJrnl)
{
	Destroy();
}

int PPALDD_CheckOpJrnl::InitData(PPFilt & rFilt, long rsrv)
{
	INIT_PPVIEW_ALDD_DATA_U(CheckOpJrnl, rsrv);
	H.FltBeg  = p_filt->Period.low;
	H.FltEnd  = p_filt->Period.upp;
	H.FltUserID = p_filt->UserID;
	H.FltAction  = p_filt->ActionIDList.getSingle();
	return DlRtm::InitData(rFilt, rsrv);
}

int PPALDD_CheckOpJrnl::InitIteration(PPIterID iterId, int sortId, long /*rsrv*/)
{
	INIT_PPVIEW_ALDD_ITER(CheckOpJrnl);
}

int PPALDD_CheckOpJrnl::NextIteration(PPIterID iterId)
{
	START_PPVIEW_ALDD_ITER(CheckOpJrnl);
	I.UserID   = item.UserID;
	I.ActionID = item.Action;
	I.Dt       = item.Dt;
	I.Tm       = item.Tm;
	SString temp_buf;
	if(PPLoadString(PPTXT_CHKOPACTIONLIST, I.ActionID - 1, temp_buf))
		temp_buf.CopyTo(I.ActionName, sizeof(I.ActionName));
	else
		ltoa(item.Action, I.ActionName, 10);
	I.GoodsID  = item.GoodsID;
	I.CheckID  = item.CheckID;
	I.CheckNum = item.CheckNum;
	// I.PrinterName;
	FINISH_PPVIEW_ALDD_ITER();
}

void PPALDD_CheckOpJrnl::Destroy()
{
	DESTROY_PPVIEW_ALDD(CheckOpJrnl);
}
