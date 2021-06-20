// CSHMCHN.CPP
// Copyright (c) A.Sobolev 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021
// @codepage UTF-8
//
// Механизм работы с кассовыми машинами
//
#include <pp.h>
#pragma hdrstop

#define DECLARE_CMT(t)  void _Register_##t(); struct Registrar_##t { Registrar_##t(); };
#define LINK_CMT(t)     Registrar_##t R_##t
#define DYN_LINK_CMT(t) _Register_##t()

DECLARE_CMT(EL92AQ);
// @v10.4.5 DECLARE_CMT(AMC100);
DECLARE_CMT(SMUKM30);
// @v10.4.5 DECLARE_CMT(AMC200);
DECLARE_CMT(SYNCSYM);
DECLARE_CMT(CRUKM);
DECLARE_CMT(CRCSHSRV);
DECLARE_CMT(SHTRIHFRF);
DECLARE_CMT(ATOL);
DECLARE_CMT(PAPYRUS);
DECLARE_CMT(ATOLWOATOLCARD);
DECLARE_CMT(SHTRIHMFRK);
DECLARE_CMT(FRONTOL);
DECLARE_CMT(ATOLDRV);
DECLARE_CMT(SYNCCASH); // @vmiller
DECLARE_CMT(SETSTART);
DECLARE_CMT(DREAMKAS); // @v10.0.04
//
// Регистрация интерфейсов кассовых аппаратов
//
void * PPCashMachine::P_RegData = 0; // @global @threadsafe

struct CMRegEntry { // @flat
	enum {
		sync  = 1,
		async = 2
	};
	PPID   cmt;
	int    flags;
	RegCashMachineFunc func;
};

typedef TSVector <CMRegEntry> CMRegArray; // @v10.0.04 TSArray-->TSVector

/*static*/int PPCashMachine::RegisterMachine(PPID cmt, RegCashMachineFunc func, int _sync, int _async)
{
	ENTER_CRITICAL_SECTION
	SETIFZ(P_RegData, new CMRegArray);
	CMRegArray * p_ary = static_cast<CMRegArray *>(P_RegData);
	CMRegEntry entry;
	entry.cmt = cmt;
	entry.func = func;
	entry.flags = 0;
	if(_sync)
		entry.flags |= entry.sync;
	if(_async)
		entry.flags |= entry.async;
	p_ary->insert(&entry);
	LEAVE_CRITICAL_SECTION
	return 1;
}

// @vmiller {
int PPCashMachine::SyncOrAsync(int cashID, int & rS)
{
	rS = 0;
	int    ok = 1;
	int    local_pos_id = 0;
	uint   sync_count = 0;
	SString path_str;
	PPGetFilePath(PPPATH_BIN, "ppdrv.ini", path_str);
	PPIniFile ini_file(path_str);
	local_pos_id = cashID - PPCMT_FIRST_DYN_DVC/*idx*/;
	if(local_pos_id < 0) // @v10.0.04 (<=)-->(<)
		ok = -1;
	else {
		StringSet set;
		THROW(ini_file.GetEntryList(PPINISECT_DRV_SYNCPOS, &set, 1));
		sync_count = set.getCount();
		THROW(ini_file.GetEntryList(PPINISECT_DRV_ASYNCPOS, &set, 1));
		if(local_pos_id < (int)sync_count) // @v10.0.04 (<=)-->(<)
			rS = 1;
		else if(local_pos_id > (int)set.getCount())
			ok = -1;
	}
	CATCHZOK
	return ok;
}
// } @vmiller

/*static*/int PPCashMachine::GetRegEntry(PPID cmtID, uint * pPos)
{
	int    idx = 0, s = 0; // @vmiller
	if(P_RegData == 0) {
		DYN_LINK_CMT(EL92AQ);
		// @v10.4.5 DYN_LINK_CMT(AMC100);
		DYN_LINK_CMT(SMUKM30);
		// @v10.4.5 DYN_LINK_CMT(AMC200);
		DYN_LINK_CMT(SYNCSYM);
		DYN_LINK_CMT(CRUKM);
		DYN_LINK_CMT(CRCSHSRV);
		DYN_LINK_CMT(SHTRIHFRF);
		DYN_LINK_CMT(ATOL);
		DYN_LINK_CMT(PAPYRUS);
		DYN_LINK_CMT(ATOLWOATOLCARD);
		DYN_LINK_CMT(SHTRIHMFRK);
		DYN_LINK_CMT(FRONTOL);
		DYN_LINK_CMT(ATOLDRV);
		DYN_LINK_CMT(SYNCCASH); // @vmiller
		DYN_LINK_CMT(SETSTART);
		DYN_LINK_CMT(DREAMKAS); // @v10.0.04
	}
	CMRegArray * p_ary = static_cast<CMRegArray *>(P_RegData);
	if(p_ary) {
		// @vmiller {
		if(PPCashMachine::SyncOrAsync(cmtID, s) > 0) {
			if(s == 1) {
				ASSIGN_PTR(pPos, 12); // Соответствует типу SYNCCASH // @v10.4.5 14-->12
				return 1;
			}
		}
		// } @vmiller
		for(uint i = 0; i < p_ary->getCount(); i++) {
			if(p_ary->at(i).cmt == cmtID) {
				ASSIGN_PTR(pPos, i);
				return 1;
			}
		}
	}
	ASSIGN_PTR(pPos, 0);
	return PPSetError(PPERR_INVCMT);
}

/*static*/PPCashMachine * PPCashMachine::CreateInstance(PPID cashID)
{
	PPCashMachine * p_cm = 0;
	uint   p = 0;
	PPCashNode cn_rec;
	PPObjCashNode cn_obj;
	if(cn_obj.Fetch(cashID, &cn_rec) > 0) {
		PPID   _cn_type = 0;
		if(cn_rec.CashType == PPCMT_CASHNGROUP) {
			PPIDArray child_list;
			cn_obj.GetListByGroup(cashID, child_list);
			for(uint i = 0; i < child_list.getCount(); i++) {
				const PPID child_id = child_list.get(i);
				PPCashNode child_cn_rec;
				if(cn_obj.Fetch(child_id, &child_cn_rec) > 0) {
					if(!_cn_type)
						_cn_type = child_cn_rec.CashType;
					else if(_cn_type != child_cn_rec.CashType) {
						_cn_type = 0;
						break;
					}
				}
			}
		}
		else
			_cn_type = cn_rec.CashType;
		if(_cn_type && GetRegEntry(_cn_type, &p))
			p_cm = static_cast<CMRegArray *>(P_RegData)->at(p).func(cashID);
	}
	return p_cm;
}

/*static*/int PPCashMachine::IsSyncCMT(PPID cmtID)
{
	uint p;
	return BIN(GetRegEntry(cmtID, &p) && static_cast<CMRegArray *>(P_RegData)->at(p).flags & CMRegEntry::sync);
}

/*static*/int PPCashMachine::IsAsyncCMT(PPID cmtID)
{
	uint p;
	return BIN(GetRegEntry(cmtID, &p) && static_cast<CMRegArray *>(P_RegData)->at(p).flags & CMRegEntry::async);
}

PPCashMachine::FiscalCorrection::FiscalCorrection() : AmtCash(0.0), AmtBank(0.0), AmtPrepay(0.0), AmtPostpay(0.0),
	AmtVat20(0.0), AmtVat18(0.0), AmtVat10(0.0), AmtVat00(0.0), AmtNoVat(0.0), VatRate(0.0), Dt(ZERODATE), Flags(0)
{
}

PPCashMachine::PPCashMachine(PPID cashID) : P_SyncIntrf(0), P_AsyncIntrf(0), SyncPrintErrCode(0), NodeID(cashID), ParentNodeID(0)
{
	Valid = (GetNode() > 0);
}

PPCashMachine::~PPCashMachine()
{
	delete P_SyncIntrf;
	delete P_AsyncIntrf;
}

/*virtual*/int PPCashMachine::ValidateCheck(CCheckTbl::Rec *, int /*correct*/, char * /*pErrBuf*/, size_t /*errBufLen*/)
{
	return 2; // not supported
}

PPID PPCashMachine::GetCurSessID()
{
	PPCashNode cn_rec;
	return (PPRef->GetItem(PPOBJ_CASHNODE, NodeRec.ID, &cn_rec) > 0) ? cn_rec.CurSessID : 0;
}

const PPCashNode & PPCashMachine::GetNodeData() const { return NodeRec; }
int PPCashMachine::IsValid() const { return Valid; }
void PPCashMachine::SetParentNode(PPID parentNodeID) { ParentNodeID = parentNodeID; }

int PPCashMachine::GetSyncIntrf()
{
	int    ok = 1;
	if(!P_SyncIntrf) {
		P_SyncIntrf = SyncInterface();
		if(!P_SyncIntrf)
			ok = PPSetError(PPERR_SYNCCSNOTSUPP);
		else if(P_SyncIntrf->IsError())
			ok = 0;
	}
	return ok;
}

int PPCashMachine::GetAsyncIntrf()
{
	return SETIFZ(P_AsyncIntrf, AsyncInterface()) ? 1 : PPSetError(PPERR_ASYNCCSNOTSUPP);
}
//
// Синхронные методы
//
int PPCashMachine::NewDayDialog(LDATE * pDt)
{
	int    ok = 0;
	TDialog * dlg = new TDialog(SLS.CheckUiFlag(sluifUseLargeDialogs) ? DLG_NEWDAY_L : DLG_NEWDAY);
	if(CheckDialogPtr(&dlg)) {
		const LDATE dt_now = getcurdate_();
		if(*pDt == 0)
			*pDt = dt_now;
		dlg->SetupCalDate(CTLCAL_NEWDAY_DATE, CTL_NEWDAY_DATE);
		dlg->setCtrlData(CTL_NEWDAY_DATE, pDt);
		while(ok != cmOK && ExecView(dlg) == cmOK) {
			LDATE   dt = dlg->getCtrlDate(CTL_NEWDAY_DATE);
			if(checkdate(dt)) {
				if(dt == dt_now || CONFIRM(PPCFM_POSDATNEQCURRENT)) {
					ok = cmOK;
					*pDt = dt;
				}
				else
					ok = cmCancel;
			}
			else {
				PPError(PPERR_SLIB);
				ok = 0;
			}
		}
	}
	delete dlg;
	return ok;
}

int PPCashMachine::SyncOpenSession(LDATE * pDate)
{
	int    ok = 1, r;
	LDATETIME dtm;
	dtm.Set(DEREFPTROR(pDate, ZERODATE), ZEROTIME);
	CSessionCore csess_core;
	THROW(PPObjCashNode::IsLocked(NodeID) <= 0);
	THROW(GetSyncIntrf());
	if(dtm.d == ZERODATE) {
		THROW(r = NewDayDialog(&dtm.d));
		if(r != cmOK || dtm.d == ZERODATE)
			return -1;
	}
	{
		PPTransaction tra(1);
		THROW(tra);
		THROW(GetNode() > 0);
		THROW_PP((NodeRec.Flags & CASHF_DAYCLOSED) || !NodeRec.CurDate || (NodeRec.Flags & CASHF_CHKPAN && !NodeRec.CurSessID), PPERR_DAYNOTCLOSED);
		THROW_PP(NodeRec.CurDate <= dtm.d, PPERR_STARTBACKDATE);
		NodeRec.Flags &= ~CASHF_DAYCLOSED;
		if(NodeRec.Flags & CASHF_CHKPAN) {
			long   last_sess_number = 0;
			THROW(csess_core.GetLastNumber(NodeRec.ID, NodeRec.ID, &last_sess_number));
			last_sess_number++;
			dtm.t = getcurtime_();
			THROW(csess_core.CreateSess(&NodeRec.CurSessID, NodeRec.ID, NodeRec.ID, last_sess_number, dtm, 0));
		}
		NodeRec.CurDate = dtm.d;
		THROW(UpdateNode(0));
		THROW(tra.Commit());
	}
	DS.SetOperDate(dtm);
	CATCHZOK
	ASSIGN_PTR(pDate, dtm.d);
	return ok;
}

int PPCashMachine::_SyncCloseBill(int ret, int use_ta)
{
	int    ok = 1;
	PPID   op = ret ? GetCashRetOp() : GetCashOp();
	if(op > 0) {
		PPObjBill * p_bobj = BillObj;
		PPID   id;
		int    count = 0;
		double sum = 0.0;
		PPBillPacket pack;
		BillCore & r_tbl = *p_bobj->P_Tbl;
		BillTbl::Key2 k;
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW(GetNode() > 0);
		if(NodeRec.DownBill) {
			THROW(p_bobj->ExtractPacket(NodeRec.DownBill, &pack));
			if(pack.Rec.OpID != op)
				return -1;
		}
		else {
			THROW(pack.CreateBlank(op, 0, 0, 0));
			pack.Rec.Dt = NodeRec.CurDate;
		}
		k.OpID   = op;
		k.Dt     = NodeRec.CurDate;
		k.BillNo = MAXLONG;
		while(r_tbl.search(2, &k, spLt) && k.Dt == NodeRec.CurDate) {
			BillTbl::Rec bill_rec;
			r_tbl.copyBufTo(&bill_rec);
			if(bill_rec.UserID == NodeID && bill_rec.Flags & BILLF_CASH) {
				if(ret)
					sum -= BR2(bill_rec.Amount);
				else
					sum += BR2(bill_rec.Amount);
				if(count == 0 && NodeRec.DownBill == 0) {
					THROW(p_bobj->TurnPacket(&pack, 0));
					//
					// Засекаем ид. результирующего документа
					//
					NodeRec.DownBill = pack.Rec.ID;
					THROW(UpdateNode(0));
				}
				THROW(p_bobj->UniteGoodsBill(&pack, bill_rec.ID, 0));
				PPWaitLong(++count);
			}
		}
		THROW_DB(BTROKORNFOUND);
		id = pack.Rec.ID;
		pack.destroy();
		if(count)
			THROW(p_bobj->RecalcTurns(id, 0, 0));
		NodeRec.DownBill = 0;
		THROW(UpdateNode(0));
		THROW(tra.Commit());
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

/*static*/long CSessionCore::GetUnitingPeriodSec()
{
	const PPCommConfig & r_ccfg = CConfig;
	return ((r_ccfg.CSessUnitingPeriod > 0) ? r_ccfg.CSessUnitingPeriod : (60*12)) * 60;
}

int CSessionCore::SearchSuperCandidate(PPID nodeID, const LDATETIME & rDtm, PPIDArray * pList)
{
	int    ok = -1;
	const long uniting_period_sec = CSessionCore::GetUnitingPeriodSec();
	CSessionTbl::Key1 k1;
	LDATETIME start = rDtm;
	LDATETIME end = rDtm;
	start.addsec(-uniting_period_sec);
	end.addsec(uniting_period_sec);
	k1.CashNodeID = nodeID;
	k1.Dt = start.d;
	k1.Tm = start.t;
	CALLPTRMEMB(pList, clear());
	while(search(1, &k1, spGt) && data.CashNodeID == nodeID && cmp(end, data.Dt, data.Tm) > 0) {
		if(data.SuperSessID == 0) {
			if(pList) {
				THROW_SL(pList->addUnique(data.ID));
			}
			ok = 1;
		}
	}
	THROW_DB(BTROKORNFOUND);
	CATCHZOK
	return ok;
}

int PPCashMachine::SyncDiagnose(StringSet * pSs)
{
	return GetSyncIntrf() ? P_SyncIntrf->Diagnose(pSs) : 0;
}

int PPCashMachine::SyncCloseSession()
{
	int    ok = -1;
	int    r;
	int    is_locked = 0;
	int    my_lock = 0;
	int    close_sess = 1;
	const  PPConfig & r_cfg = LConfig;
	const  LDATE  save_date   = r_cfg.OperDate;
	const  PPID   save_loc_id = r_cfg.Location;
	const  PPID   save_cash_id = r_cfg.Cash;
	CCheckCore  chk_core;
	THROW_PP(BillObj->CheckRights(BILLRT_CLOSECASH), PPERR_NORTCLOSECASH);
	THROW((my_lock = PPObjCashNode::IsLocked(NodeID)) <= 0);
	if(my_lock == 0) {
		THROW(r = PPObjCashNode::Lock(NodeID));
		if(r > 0)
			is_locked = 1;
	}
	THROW(GetSyncIntrf());
	THROW(GetNode() > 0);
	if(!(NodeRec.Flags & CASHF_CHKPAN) && NodeRec.DownBill) {
		r = PPMessage(mfConf|mfYesNoCancel, PPCFM_CONTDAYCLOSING);
		if(r != cmYes)
			if(r == cmNo) {
				NodeRec.DownBill = 0;
				THROW(UpdateNode(1));
			}
			else
				return -1;
	}
	THROW_PP((NodeRec.Flags & CASHF_DAYCLOSED) == 0, PPERR_DAYCLOSED);
	if(chk_core.IsCheckWFlags(NodeRec.CurSessID, CCHKF_SUSPENDED) && PPMessage(mfConf|mfYesNo, PPCFM_CLOSESESSWSUSPCHKS) == cmNo)
		close_sess = 0;
	if(close_sess) {
		DS.SetOperDate(NodeRec.CurDate);
		DS.SetLocation(NodeRec.LocID);
		if(SyncViewSessionStat(PPCashMachine::cssoCloseSession) == cmOK) {
			PPObjSecur::Exclusion ose(PPEXCLRT_CSESSWROFF);
			PPID   sess_id = 0;
			CSessGrouping csg;
			CSessTotal   total;
			PPTransaction tra(1);
			THROW(tra);
			PPWaitStart();
			if(NodeRec.Flags & CASHF_CHKPAN) {
				DS.SetCurCashNodeID(0);
				PPID   super_id = 0;
				THROW(r = csg.AttachSessToSupersess(NodeID, NodeRec.CurSessID, &super_id, 0));
				if(r < 0) {
					//
					// Группируем и списываем сессию только в том случае, если она не вошла в суперсессию
					//
					THROW(csg.Grouping(NodeRec.CurSessID, &total, 0, 0));
					THROW(csg.ConvertToBills(NodeRec.CurSessID, NodeRec.LocID, 0, 1, 1, 0));
				}
				DS.SetCurCashNodeID(save_cash_id);
			}
			else {
				THROW(_SyncCloseBill(0, 0));
				THROW(_SyncCloseBill(1, 0));
			}
			sess_id = NodeRec.CurSessID;
			NodeRec.DownBill  = 0;
			NodeRec.CurSessID = 0;
			NodeRec.Flags   |= CASHF_DAYCLOSED;
			NodeRec.Flags   &= ~CASHF_DAYSTARTED;
			THROW(UpdateNode(0));
			DS.LogAction(PPACN_CSESSCLOSED, PPOBJ_CSESSION, sess_id, 0, 0);
			THROW(tra.Commit());
			//
			THROW(P_SyncIntrf->CloseSession(sess_id));
			{
				ObjTagItem tag_item;
				if(PPRef->Ot.GetTag(PPOBJ_CASHNODE, NodeRec.ID, PPTAG_POSNODE_HOSTUUID, &tag_item) > 0) {
					PPPosProtocol::RouteBlock dest_rb;
					if(tag_item.GetGuid(&dest_rb.Uuid) > 0) {
						PPIDArray sess_list;
						sess_list.add(sess_id);
						PPPosProtocol pp;
						THROW(pp.ExportPosSession(sess_list, NodeRec.ID, 0, &dest_rb));
					}
				}
			}
			PPWaitStop();
			ok = 1;
		}
	}
	CATCHZOK
	if(is_locked)
		PPObjCashNode::Unlock(NodeID);
	DS.SetCurCashNodeID(save_cash_id);
	DS.SetLocation(save_loc_id);
	DS.SetOperDate(save_date);
	return ok;
}

/* @v10.5.5 Перенесено в архивных целях из ppw.rc поскольку не используется //
DLG_CASHDISP DIALOGEX 98, 8, 321, 242
STYLE DS_SETFONT | DS_MODALFRAME | WS_POPUP | WS_CAPTION | WS_SYSMENU
CAPTION "@cashmachinestat"
FONT 8, "MS Sans Serif", 0, 0, 0x0
BEGIN
    GROUPBOX        "",IDC_STATIC,5,0,150,90
    LTEXT           "@cashmachinestat_begsum",4001,15,10,125,8
    EDITTEXT        CTL_CASHDISP_BEGSUM,15,20,65,13,ES_AUTOHSCROLL
    LTEXT           "@cashmachinestat_income",4002,15,35,130,8
    EDITTEXT        CTL_CASHDISP_INCOME,15,45,65,13,ES_AUTOHSCROLL
    GROUPBOX        "",IDC_STATIC,160,0,150,90
    LTEXT           "@cashmachinestat_currentsum",4003,170,10,130,8
    EDITTEXT        CTL_CASHDISP_CURSUM,170,20,65,13,ES_AUTOHSCROLL
    LTEXT           "@cashmachinestat_cash",4009,170,35,125,8
    EDITTEXT        CTL_CASHDISP_CASHSUM,170,45,65,13,ES_AUTOHSCROLL
    LTEXT           "@difference",4010,170,60,125,8
    EDITTEXT        CTL_CASHDISP_DIFF,170,70,65,13,ES_AUTOHSCROLL
    GROUPBOX        "",IDC_STATIC,5,95,305,90
    LTEXT           "Сумма наличных оплат",4004,10,105,140,8
    EDITTEXT        CTL_CASHDISP_CASHAMT,10,115,65,13,ES_AUTOHSCROLL
    LTEXT           "Сумма безнал оплат",4005,170,105,130,8
    EDITTEXT        CTL_CASHDISP_BANKAMT,170,115,65,13,ES_AUTOHSCROLL
    LTEXT           "@refundamount",4006,10,130,130,8
    EDITTEXT        CTL_CASHDISP_RETAMT,10,140,65,13,ES_AUTOHSCROLL
    LTEXT           "@discountamount",4007,170,130,130,8
    EDITTEXT        CTL_CASHDISP_DSCNTAMT,170,140,65,13,ES_AUTOHSCROLL
    LTEXT           "Сумма по кред картам",4008,10,155,135,8
    EDITTEXT        CTL_CASHDISP_SCARDAMT,10,165,65,13,ES_AUTOHSCROLL
    DEFPUSHBUTTON   "@but_ok",STDCTL_OKBUTTON,175,220,60,13
    PUSHBUTTON      "@but_cancel",STDCTL_CANCELBUTTON,250,220,60,13
    LTEXT           "@cashmachinestat_warn",IDC_STATIC,5,190,305,20
END
*/

struct CashStat {
	CashStat()
	{
		THISZERO();
	}
	PPID   CashID;
	double BegSum;
	double Income;
	double Cash;
	double AmtCash;       // Сумма наличных оплат
	double AmtBank;       // Сумма безналичных оплат
	double AmtReturn;     // Сумма возвратов
	double Discount;      // Сумма скидок
	double AmtSCard;      // Сумма по кредитным картам
};

class CashStatDialog : public TDialog {
public:
	CashStatDialog(uint dlgID, PPCashMachine * pCm, CashStat * pStat) : TDialog(dlgID), machine(pCm)
	{
		if(!RVALUEPTR(stat, pStat))
			MEMSZERO(stat);
		setup();
	}
	void   setup();
	// @v10.3.9 void   correct();
private:
	DECL_HANDLE_EVENT;
	PPCashMachine * machine;
	CashStat stat;
};

void CashStatDialog::setup()
{
	setCtrlReal(CTL_CASHDISP_BEGSUM, stat.BegSum);
	setCtrlReal(CTL_CASHDISP_INCOME, stat.Income);
	double v = stat.BegSum + stat.Income;
	setCtrlReal(CTL_CASHDISP_CURSUM, v);
	setCtrlReal(CTL_CASHDISP_CASHAMT,  stat.AmtCash);
	setCtrlReal(CTL_CASHDISP_BANKAMT,  stat.AmtBank);
	setCtrlReal(CTL_CASHDISP_SCARDAMT, stat.AmtSCard);
	setCtrlReal(CTL_CASHDISP_DSCNTAMT, stat.Discount);
	setCtrlReal(CTL_CASHDISP_RETAMT,   stat.AmtReturn);
	if(stat.Cash >= 0) {
		setCtrlData(CTL_CASHDISP_CASHSUM, &stat.Cash);
		v -= stat.Cash;
	}
	else {
		TInputLine * il = static_cast<TInputLine *>(getCtrlView(CTL_CASHDISP_CASHSUM));
		CALLPTRMEMB(il, setText("Error"));
	}
	setCtrlData(CTL_CASHDISP_DIFF, &v);
	setCtrlReadOnly(CTL_CASHDISP_BEGSUM, 1);
	setCtrlReadOnly(CTL_CASHDISP_INCOME, 1);
	setCtrlReadOnly(CTL_CASHDISP_CURSUM, 1);
	setCtrlReadOnly(CTL_CASHDISP_CASHSUM, 1);
	setCtrlReadOnly(CTL_CASHDISP_DIFF, 1);
	setCtrlReadOnly(CTL_CASHDISP_CASHAMT, 1);
	setCtrlReadOnly(CTL_CASHDISP_BANKAMT, 1);
	setCtrlReadOnly(CTL_CASHDISP_SCARDAMT, 1);
	setCtrlReadOnly(CTL_CASHDISP_DSCNTAMT, 1);
	setCtrlReadOnly(CTL_CASHDISP_RETAMT, 1);
}

/* @v10.3.9 void CashStatDialog::correct()
{
	double add = stat.BegSum + stat.Income - stat.Cash;
	TDialog * dlg = new TDialog(DLG_CASHCORR);
	THROW(CheckDialogPtr(&dlg));
	dlg->setCtrlReal(CTL_CASHCORR_CURVAL, stat.Cash);
	dlg->disableCtrl(CTL_CASHCORR_CURVAL, 1);
	dlg->setCtrlReal(CTL_CASHCORR_ADD, add);
	if(ExecView(dlg) == cmOK) {
		dlg->getCtrlData(CTL_CASHCORR_ADD, &add);
		if(add > 0.0) {
			THROW(machine->SyncAddSummator(add));
			stat.Cash += add;
			setup();
		}
	}
	CATCH
		PPError();
	ENDCATCH
	delete dlg;
}*/

IMPL_HANDLE_EVENT(CashStatDialog)
{
	TDialog::handleEvent(event);
	/* @v10.3.9 if(event.isKeyDown(kbF7) && stat.Cash >= 0) {
		correct();
		clearEvent(event);
	}*/
}

int PPCashMachine::SyncViewSessionStat(long options)
{
	int    ok = cmCancel;
	CashStat stat;
	double sum = 0.0;
	const  PPConfig & r_cfg = LConfig;
	PPID   save_cash_id = r_cfg.Cash;
	PPID   save_loc_id  = r_cfg.Location;
	SString font_face, msg_buf;
	TDialog * dlg = 0;
	THROW(GetSyncIntrf());
	THROW(GetNode() > 0);
	DS.SetCurCashNodeID(NodeID);
	DS.SetLocation(NodeRec.LocID);
	if(NodeRec.Flags & CASHF_CHKPAN) {
		PPObjCSession cs_obj;
		CSessionTbl::Rec cs_rec;
		if(NodeRec.CurSessID && cs_obj.P_Tbl->Search(NodeRec.CurSessID, &cs_rec) > 0) {
			// @v10.4.2 CCheckCore cc;
			CSessTotal cs_total;
			long   _f = cs_obj.P_Tbl->GetCcGroupingFlags(cs_rec, 0);
			_f |= CCheckCore::gglfUseFullCcPackets;
			cs_obj.P_Cc->GetSessTotal(NodeRec.CurSessID, _f, &cs_total, 0);
			stat.AmtCash   = cs_total.Amount - cs_total.BnkAmount - cs_total.CSCardAmount;
			stat.AmtBank   = cs_total.BnkAmount;
			stat.AmtSCard  = cs_total.CSCardAmount;
			stat.Discount  = cs_total.Discount;
			stat.AmtReturn = cs_total.RetAmount;
			sum = cs_total.Amount;
		}
		if(sum == 0.0)  {
			SyncGetSummator(&sum);
			SETMAX(sum, 0.0);
		}
	}
	else {
		BillTotal  btotal;
		PPViewBill bview;
		BillFilt   flt;
		flt.Flags |= BillFilt::fCashOnly;
		bview.Init_(&flt);
		bview.CalcTotal(&btotal);
		sum = btotal.Sum;
	}
	DS.SetCurCashNodeID(save_cash_id);
	stat.Income  = sum;
	stat.CashID  = NodeID;
	THROW(SyncGetSummator(&stat.Cash));
	THROW(CheckDialogPtr(&(dlg = new CashStatDialog(DLG_CASHDISP_WCFM, this, &stat))));
	if(options & cssoCloseSession) {
		PPGetSubStr(PPTXT_FONTFACE, PPFONT_IMPACT, font_face);
		dlg->SetCtrlFont(CTL_CASHDISP_ST_TITLE, font_face, 20);
		PPLoadString("zreport", msg_buf);
		dlg->setStaticText(CTL_CASHDISP_ST_TITLE, msg_buf);
		//
		// @v10.5.5 PPLoadText(PPTXT_CSESSTOTAL_ZREP, msg_buf);
		PPLoadString("cashmachinestat_warn", msg_buf); // @v10.5.5
		dlg->setStaticText(CTL_CASHDISP_ST_INFO, msg_buf);
	}
	else if(options & cssoXReport) {
		PPGetSubStr(PPTXT_FONTFACE, PPFONT_IMPACT, font_face);
		dlg->SetCtrlFont(CTL_CASHDISP_ST_TITLE, font_face, 20);
		PPLoadString("xreport", msg_buf);
		dlg->setStaticText(CTL_CASHDISP_ST_TITLE, msg_buf);
	}
	ok = ExecViewAndDestroy(dlg);
	CATCHZOKPPERR
	DS.SetLocation(save_loc_id);
	DS.SetCurCashNodeID(save_cash_id);
	return ok;
}

int PPCashMachine::SyncBrowseCheckList(const char * pCheckPanInitStr, long checkPanFlags)
{
	int    ok = 1;
	PPObjBill * p_bobj = BillObj;
	const  PPConfig & r_cfg = LConfig;
	int    r = cmOK;
	int    deny_ops = 0;
	LDATE  save_date = ZERODATE;
	LDATE  cur_date = ZERODATE;
	PPSyncCashNode  scn;
	PPObjCashNode   cn_obj;
	PPID   ext_cash_node_id = 0;  // ИД спаренного узла
	PPCashMachine * p_cm_ext = 0; // Движок спаренного узла
	CmLocking * p_cml_x = 0;  // Блокировка для спаренного узла
	THROW(p_bobj->CheckRights(BILLRT_CASH));
	THROW(GetSyncIntrf());
	THROW(GetNode() > 0);
	{
		const long cnf = NodeRec.Flags; // Флаги основного кассового узла
		CmLocking cml(NodeRec);
		if(!cml) {
			if(pCheckPanInitStr) {
				CALLEXCEPT();
			}
			else {
				THROW(cml.IsForeignLocking());
				deny_ops = 1;
			}
		}
		else {
			if(cnf & CASHF_DAYCLOSED || !NodeRec.CurDate || (cnf & CASHF_CHKPAN && !NodeRec.CurSessID)) {
				THROW_PP(p_bobj->CheckRights(BILLRT_OPENCASH), PPERR_NORTOPENCASH);
				if((r = PPMessage(mfConf|mfYesNo, PPCFM_PREVCASHDAYCLOSED)) == cmYes) {
					THROW(r = SyncOpenSession(&cur_date));
					if(r > 0)
						r = cmOK;
				}
			}
			else {
				save_date = r_cfg.OperDate;
				DS.SetOperDate(NodeRec.CurDate);
			}
		}
		THROW(cn_obj.GetSync(NodeID, &scn) > 0);
		ext_cash_node_id = scn.ExtCashNodeID;
		if(ext_cash_node_id) {
			PPCashNode cn_ext;
			THROW(p_cm_ext = PPCashMachine::CreateInstance(ext_cash_node_id));
			THROW(p_cm_ext->GetNode() > 0);
			cn_ext = p_cm_ext->GetNodeData();
			const long cnxf = cn_ext.Flags; // Флаги спаренного кассового узла
			if(cn_ext.CashType == PPCMT_PAPYRUS) {
				p_cm_ext->SetParentNode(NodeID);
				p_cm_ext->AsyncOpenSession(0, 0);
			}
			else {
				THROW_MEM(p_cml_x = new CmLocking(cn_ext));
				if(!(*p_cml_x)) {
					if(pCheckPanInitStr) {
						CALLEXCEPT();
					}
					else {
						THROW(p_cml_x->IsForeignLocking());
					}
				}
				else {
					if(cnxf & CASHF_DAYCLOSED || !cn_ext.CurDate || (cnxf & CASHF_CHKPAN && !cn_ext.CurSessID)) {
						THROW_PP(p_bobj->CheckRights(BILLRT_OPENCASH), PPERR_NORTOPENCASH);
						if(cur_date || (r = PPMessage(mfConf|mfYesNo, PPCFM_PREVCASHDAYCLOSED)) == cmYes) {
							THROW(r = p_cm_ext->SyncOpenSession(&cur_date));
							if(r > 0)
								r = cmOK;
						}
					}
				}
			}
			ZDELETE(p_cm_ext);
		}
		if(r == cmOK) {
			DS.SetCurCashNodeID(NodeID);
			if(cnf & CASHF_CHKPAN) {
				if(pCheckPanInitStr) {
					CCheckPane(NodeID, 0, pCheckPanInitStr, checkPanFlags);
				}
				else {
					CCheckFilt ccflt;
					//ccflt.Period.SetDate(r_cfg.OperDate);
					ccflt.NodeList.Add(NodeID);
					ccflt.CashNumber = NodeID;
					if(!!cml)
						ccflt.Flags = CCheckFilt::fActiveSess | CCheckFilt::fImmOpenPanel;
					ccflt.SessIDList.add(NodeRec.CurSessID);
					ViewCCheck(&ccflt, PPView::exefDisable3Tier);
				}
			}
			else {
				BillFilt flt;
				flt.Period.SetDate(r_cfg.OperDate);
				flt.LocList.Add(NodeRec.LocID ? NodeRec.LocID : r_cfg.Location);
				flt.Flags |= BillFilt::fCashOnly;
				if(deny_ops)
					flt.Flags |= (BillFilt::fDenyAdd|BillFilt::fDenyUpdate|BillFilt::fDenyRemove);
				THROW(ViewGoodsBills(&flt, 0));
			}
		}
	}
	CATCHZOK
	DS.SetCurCashNodeID(0);
	if(save_date)
		DS.SetOperDate(save_date);
	delete p_cml_x;
	delete p_cm_ext;
	return ok;
}

int PPCashMachine::SyncGetSummator(double * pSum)
	{ return (GetSyncIntrf() && P_SyncIntrf->GetSummator(pSum)); }

int PPCashMachine::SyncGetDeviceTime(LDATETIME * pDtm)
	{ return (GetSyncIntrf() && P_SyncIntrf->GetDeviceTime(pDtm)); }

int PPCashMachine::SyncSetPrintErrCode(int retCode, int updateNode)
{
	SyncPrintErrCode = 0;
	int    ok = retCode;
	int    err_code = (ok == 0) ? P_SyncIntrf->GetPrintErrCode() : SYNCPRN_NO_ERROR;
	if(updateNode && !oneof2(err_code, SYNCPRN_ERROR_WHILE_PRINT, SYNCPRN_CANCEL_WHILE_PRINT)) {
		NodeRec.Flags &= ~CASHF_LASTCHKCANCELLED;
		if(!UpdateNode(1)) {
			if(err_code == SYNCPRN_NO_ERROR)
				err_code = SYNCPRN_ERROR_AFTER_PRINT;
			else if(err_code == SYNCPRN_CANCEL)
				err_code = SYNCPRN_ERROR;
			ok = 0;
		}
	}
	if(!ok) {
		if(err_code == SYNCPRN_ERROR_AFTER_PRINT)
			SyncPrintErrCode = 1;
		else if(oneof2(err_code, SYNCPRN_ERROR, SYNCPRN_ERROR_WHILE_PRINT))
			SyncPrintErrCode = 2;
		else if(oneof2(err_code, SYNCPRN_CANCEL, SYNCPRN_CANCEL_WHILE_PRINT))
			SyncPrintErrCode = 3;
	}
	return ok;
}
//
//
//
CmLocking::CmLocking(const PPCashNode & rNodeRec) : R_NodeRec(rNodeRec), Flags(0)
{
	int    r = PPObjCashNode::IsLocked(R_NodeRec.ID);
	if(r == 0) {
		r = PPObjCashNode::Lock(R_NodeRec.ID);
		if(!r)
			Flags |= fError;
		else if(r > 0)
			Flags |= fLocking;
	}
	else if(r > 0) {
		Flags |= fForeignLocking;
		if(!(R_NodeRec.Flags & CASHF_WORKWHENLOCK))
			Flags |= fError;
		r = -1;
	}
}

CmLocking::~CmLocking()
{
	if(Flags & fLocking)
		PPObjCashNode::Unlock(R_NodeRec.ID);
}

int CmLocking::operator !() const { return BIN(Flags & fError); }
int CmLocking::IsForeignLocking() const { return BIN(Flags & fForeignLocking); }
//
//
//
int PPCashMachine::TestPrintCheck(CCheckPacket * pPack)
{
	int     ok = 1, chk_no = 0, is_format = 0;
	THROW_INVARG(pPack);
	if(pPack->GetCount() == 0)
		ok = -1;
	else if(GetSyncIntrf()) {
		PPSlipFormatter * p_formatter = P_SyncIntrf->GetSlipFormatter();
		if(p_formatter) {
			SlipDocCommonParam sdc_param;
			int    prn_total_sale = 1, r = 0;
			SString line_buf, serial;
			const SString format_name("CCheck");
			SString out_buf;
			SlipLineParam sl_param;
			double amt = fabs(R2(MONEYTOLDBL(pPack->Rec.Amount)));
			double sum = fabs(pPack->_Cash) + 0.001;
			double running_total = 0.0;
			double cc_amount = 0.0;
			double fiscal = 0.0, nonfiscal = 0.0;
			out_buf.Z().Cat("CCHECK").Space().CatChar('{').CR();
			{
				const long _mfmt = MKSFMTD(0, 12, NMBF_NOTRAILZ);
				{
					CCheckLineTbl::Rec line_rec;
					out_buf.Tab().CatEq("ID", pPack->Rec.ID).CatDiv(';', 2).CatEq("Code", pPack->Rec.Code).CatDiv(';', 2).
						CatEq("Date", pPack->Rec.Dt).CatDiv(';', 2).CatEq("Time", pPack->Rec.Tm).CatDiv(';', 2).
						Cat("Flags").Eq().Cat("0x").CatHex(pPack->Rec.Flags).CatDiv(';', 2).
						CatEq("Amount", MONEYTOLDBL(pPack->Rec.Amount)).CatDiv(';', 2).
						CatEq("Discount", MONEYTOLDBL(pPack->Rec.Discount), _mfmt).CR();
					out_buf.Tab().Cat("Lines").Space().CatChar('{').CR();
					for(uint i = 0; pPack->EnumLines(&i, &line_rec, &serial) > 0;) {
						double amt = pPack->GetItemAmount(line_rec);
						running_total += amt;
						{
							const double s  = intmnytodbl(line_rec.Price) * line_rec.Quantity;
							const double ds = line_rec.Dscnt * line_rec.Quantity;
							cc_amount = R2(cc_amount + s - ds);
						}
						out_buf.Tab(2).CatLongZ(line_rec.GoodsID, 5).Tab().
							Cat(line_rec.Quantity, _mfmt).Tab().
							Cat(intmnytodbl(line_rec.Price), _mfmt).Tab().
							Cat(line_rec.Dscnt, _mfmt).Tab().
							Cat(amt, _mfmt).Tab().
							Cat(running_total, _mfmt).Tab().
							Cat(cc_amount, _mfmt).CR();
					}
					out_buf.Tab().CatChar('}').CR();
				}
				out_buf.CR();
				pPack->HasNonFiscalAmount(&fiscal, &nonfiscal);
				if(p_formatter->Init(format_name, &sdc_param) > 0) {
					running_total = 0.0;
					out_buf.Tab().CatEq("format", format_name).CatChar('{').CR();
					if(sdc_param.PageWidth > 36)
						out_buf.Tab(2).Cat("PageWidthOverflow").CatDiv(':', 2).Cat(sdc_param.PageWidth).CatChar('>').Cat(36).CR();
					for(p_formatter->InitIteration(pPack); p_formatter->NextIteration(line_buf, &sl_param) > 0;) {
						line_buf.Transf(CTRANSF_OUTER_TO_INNER);
						if(sl_param.Flags & SlipLineParam::fRegFiscal) {
							double _q = sl_param.Qtty;
							double _p = sl_param.Price;
							running_total += (_q * _p);
							prn_total_sale = 0;
							out_buf.Tab(2).CatEq("Quantity", _q, _mfmt).Tab().
								CatEq("Price", _p, _mfmt).Tab().
								CatEq("RunningTotal", running_total, _mfmt).Tab().
								CatEq("Line", line_buf).CR();
						}
						else {
							out_buf.Tab(2).CatEq("Line", line_buf).CR();
						}
					}
					if(prn_total_sale) {
						if(fiscal != 0.0) {
							if(!pPack->GetCount()) {
								// @v10.7.3 double _q = 1.0;
								double _p = amt;
								running_total += amt;
							}
							else /*if(fiscal != 0.0)*/ {
								// @v10.7.3 double _q = 1.0;
								double _p = fiscal;
								running_total += fiscal;
							}
						}
					}
					else if(running_total > amt) {
						SString fmt_buf, msg_buf, added_buf;
						added_buf.Z().Cat(running_total, _mfmt).CatChar('>').Cat(amt, _mfmt);
						msg_buf.Printf(PPLoadTextS(PPTXT_SHTRIH_RUNNGTOTALGTAMT, fmt_buf), added_buf.cptr());
						PPLogMessage(PPFILNAM_SHTRIH_LOG, msg_buf, LOGMSGF_TIME|LOGMSGF_USER);
					}
					out_buf.Tab().CatChar('}').CR();
				}
				else {
					// Ошибка инициализации формата чека
					out_buf.Tab(2).Cat("Error in format").Space().Cat(format_name).CR();
				}
				if(running_total > sum)
					sum = running_total;
				if(nonfiscal > 0.0) {
					if(fiscal > 0.0) {
						;
					}
				}
				/*
				else if(flags & PRNCHK_BANKING) {
					double  add_paym = intmnytodbl(pPack->Ext.AddPaym);
					if(add_paym) {
						;
					}
					else {
						;
					}
				}
				else {
					;
				}
				*/
				if(fiscal != 0.0) {
					;
				}
				else {
					;
				}
			}
			out_buf.CatChar('}').CR();
			PPLogMessage(PPFILNAM_DEBUG_LOG, out_buf, 0);
		}
	}
	CATCHZOK
	return ok;
}

int PPCashMachine::SyncPrintCheck(CCheckPacket * pCcPack, int addSummator)
{
	int    ok = 1;
	SyncPrintErrCode = 0;
	CmLocking cml(NodeRec);
	THROW(cml);
	THROW(GetSyncIntrf());
	if(!(pCcPack->Rec.Flags & CCHKF_PRINTED) || PPMessage(mfConf|mfYesNo, PPCFM_BILLCHECKED) == cmYes) {
		uint   flags = 0;
		THROW(GetNode() > 0);
		if(NodeRec.Flags & CASHF_OPENBOX)
			flags |= PRNCHK_OPENBOX;
		if(NodeRec.Flags & CASHF_ROUNDINT)
			flags |= PRNCHK_ROUNDINT;
		if(NodeRec.Flags & CASHF_LASTCHKCANCELLED)
			flags |= PRNCHK_LASTCHKANNUL;
		if(addSummator)
			flags |= PRNCHK_ADDFISCCNTR;
		else
			flags |= PRNCHK_REPRINT;
		if(pCcPack->Rec.Flags & CCHKF_RETURN)
			flags |= PRNCHK_RETURN;
		if(pCcPack->Rec.Flags & CCHKF_BANKING)
			flags |= PRNCHK_BANKING;
		NodeRec.Flags |= CASHF_LASTCHKCANCELLED;
		THROW(UpdateNode(1));
		ok = SyncSetPrintErrCode(P_SyncIntrf->PrintCheck(pCcPack, flags), 1);
		if(ok > 0 && NodeRec.Flags & CASHF_AUTO_PRINTCOPY)
			ok = SyncPrintCheckCopy(pCcPack, 0);
	}
	CATCHZOK
	return ok;
}

int PPCashMachine::SyncOpenBox()
{
	int    ok = -1;
	THROW(GetNode() > 0);
	{
		CmLocking cml(NodeRec);
		THROW(cml);
		if(NodeRec.Flags & CASHF_OPENBOX) {
			THROW(GetSyncIntrf());
			THROW(ok = P_SyncIntrf->OpenBox());
		}
	}
	CATCHZOK
	return ok;
}

int PPCashMachine::SyncCheckForSessionOver()
{
	int    ok = 1, is_over = 0;
	CmLocking cml(NodeRec);
	THROW(cml);
	THROW(GetSyncIntrf());
	THROW(is_over = P_SyncIntrf->CheckForSessionOver());
	if(is_over > 0) {
		if(PPMessage(mfConf|mfYesNo, PPCFM_CLOSESESSWHICHOVER) == cmYes) {
			LDATE  cur_date = getcurdate_();
			THROW(SyncCloseSession());
			THROW(SyncOpenSession(&cur_date));
		}
		else
			ok = -1;
	}
	CATCHZOK
	return ok;
}

int PPCashMachine::SyncPrintCheckCopy(CCheckPacket * pPack, const char * pFormatName)
{
	int    ok = 0;
	uint   flags = 0;
	THROW(GetSyncIntrf());
	THROW(GetNode() > 0);
	{
		CmLocking cml(NodeRec);
		THROW(cml);
		SETFLAG(flags, PRNCHK_ROUNDINT, NodeRec.Flags & CASHF_ROUNDINT);
		SETFLAG(flags, PRNCHK_RETURN, pPack->Rec.Flags & CCHKF_RETURN);
		ok = SyncSetPrintErrCode(P_SyncIntrf->PrintCheckCopy(pPack, pFormatName, flags), 0);
	}
	CATCHZOK
	return ok;
}

int PPCashMachine::GetSlipFormatList(StrAssocArray * pList, int getSlipDocForms)
{
	int    ok = 1;
	THROW(GetSyncIntrf());
	{
		PPSlipFormatter * p_sf = P_SyncIntrf->GetSlipFormatter();
		THROW(p_sf);
		ok = p_sf->GetFormList(pList, getSlipDocForms);
	}
	CATCHZOK
	return ok;
}

int PPCashMachine::SyncPrintSlipDocument(CCheckPacket * pPack, const char * pFormatName)
{
	int    ok = 0;
	uint   flags = 0;
	THROW(GetSyncIntrf());
	THROW(GetNode() > 0);
	{
		CmLocking cml(NodeRec);
		THROW(cml);
		SETFLAG(flags, PRNCHK_RETURN, pPack->Rec.Flags & CCHKF_RETURN);
		SETFLAG(flags, PRNCHK_ROUNDINT, NodeRec.Flags & CASHF_ROUNDINT);
		ok = SyncSetPrintErrCode(P_SyncIntrf->PrintSlipDoc(pPack, pFormatName, flags), 0);
	}
	CATCHZOK
	return ok;
}

int PPCashMachine::SyncPrintBnkTermReport(const char * pSlipText)
{
	int    ok = 0;
	THROW(GetSyncIntrf());
	THROW(GetNode() > 0);
	{
		CmLocking cml(NodeRec);
		THROW(cml);
		ok = SyncSetPrintErrCode(P_SyncIntrf->PrintBnkTermReport(pSlipText), 0);
	}
	CATCHZOK
	return ok;
}

int PPCashMachine::SyncPrintFiscalCorrection(const PPCashMachine::FiscalCorrection * pFc)
{
	int    ok = 0;
	THROW(GetSyncIntrf());
	THROW(GetNode() > 0);
	{
		CmLocking cml(NodeRec);
		THROW(cml);
		ok = SyncSetPrintErrCode(P_SyncIntrf->PrintFiscalCorrection(pFc), 0);
	}
	CATCHZOK
	return ok;
}

int PPCashMachine::SyncPrintXReport()
{
	int    ok = 0, r = 0;
	PPObjCSession cs_obj;
	//
	// Проверка права должна осуществляться в вызывающей функции.
	// Причина в том, что регламент CSESSOPRT_XREP может быть переопределен в кассовой панели.
	// THROW(cs_obj.CheckRights(CSESSOPRT_XREP, 1));
	//
	{
		CmLocking cml(NodeRec);
		CSessInfo cs_info;
		THROW(cml);
		THROW(GetSyncIntrf());
		if(SyncViewSessionStat(PPCashMachine::cssoXReport) == cmOK) {
			PPWaitStart();
			if(cs_obj.P_Tbl->Search(NodeRec.CurSessID, &cs_info.Rec) > 0) {
				// @v10.4.2 CCheckCore cc;
				long   _f = cs_obj.P_Tbl->GetCcGroupingFlags(cs_info.Rec, 0);
				_f |= CCheckCore::gglfUseFullCcPackets;
				r = cs_obj.P_Cc->GetSessTotal(NodeRec.CurSessID, _f, &cs_info.Total, 0);
			}
			ok = SyncSetPrintErrCode(P_SyncIntrf->PrintXReport((r > 0) ? &cs_info : 0), 0);
		}
		else
			ok = -1;
		PPWaitStop();
	}
	CATCHZOK
	return ok;
}

int PPCashMachine::SyncPrintZReportCopy(const CSessInfo * pInfo)
{
	int    ok = 0;
	CmLocking cml(NodeRec);
	THROW(cml);
	THROW(GetSyncIntrf());
	PPWaitStart();
	ok = SyncSetPrintErrCode(P_SyncIntrf->PrintZReportCopy(pInfo), 0);
	PPWaitStop();
	CATCHZOK
	return ok;
}

int PPCashMachine::SyncPrintIncasso()
{
	int    ok = -1;
	ushort v = 0;
	double sum = 0.0;
	TDialog * dlg = 0;
	CmLocking cml(NodeRec);
	THROW(cml);
	THROW(CheckDialogPtrErr(&(dlg = new TDialog(SLS.CheckUiFlag(sluifUseLargeDialogs) ? DLG_INCASSO_L : DLG_INCASSO))));
	dlg->setCtrlReal(CTL_INCASSO_SUM,  sum);
	dlg->setCtrlUInt16(CTL_INCASSO_TYPE, v);
	while(ok <= 0 && ExecView(dlg) == cmOK) {
		sum = dlg->getCtrlReal(CTL_INCASSO_SUM);
		v = dlg->getCtrlUInt16(CTL_INCASSO_TYPE);
		if(sum > 0.0) {
			THROW(GetSyncIntrf());
			if(SyncSetPrintErrCode(P_SyncIntrf->PrintIncasso(sum, BIN(v)), 0))
				ok = 1;
			else
				PPError();
		}
		else
			PPErrorByDialog(dlg, CTL_INCASSO_SUM, PPERR_USERINPUT);
	}
	CATCHZOK
	delete dlg;
	return ok;
}

int PPCashMachine::SyncAllowPrint()
{
	int    ok = 1;
	PPObjBill * p_bobj = BillObj;
	THROW(p_bobj->CheckRights(BILLRT_CASH));
	THROW(GetNode() > 0);
	if((NodeRec.Flags & CASHF_DAYCLOSED) || !NodeRec.CurDate || (NodeRec.Flags & CASHF_CHKPAN && !NodeRec.CurSessID)) {
		LDATE  cur_date = getcurdate_();
		THROW_PP(p_bobj->CheckRights(BILLRT_OPENCASH), PPERR_NORTOPENCASH);
		THROW(SyncOpenSession(&cur_date));
	}
	CATCHZOK
	return ok;
}

int PPCashMachine::SyncLockCashKeyb() { return (GetSyncIntrf() && P_SyncIntrf->EnableCashKeyb(0)); }
int PPCashMachine::SyncUnlockCashKeyb() { return (GetSyncIntrf() && P_SyncIntrf->EnableCashKeyb(1)); }
int PPCashMachine::SyncPrintBnkTermReport(long pinpad, const char * pZCheck)
	{ return (pinpad & PPSyncCashNode::btfPinpad) ? (GetSyncIntrf() && P_SyncIntrf->PrintBnkTermReport(pZCheck)) : 1; }
//
// Асинхронные методы
//
int PPCashMachine::AsyncUpdateSession()
{
	return PPSetError(PPERR_CASHFUNCNSUPP);
}

int PPCashMachine::AsyncBrowseCheckList()
{
	ViewCCheck(0, 0);
	return 1;
}

int PPCashMachine::AsyncOpenSession(int update, PPID sinceDlsID)
{
	int    ok = 1;
	if(!update) {
		PPObjCSession cs_obj;
		THROW(cs_obj.CheckRights(CSESSRT_OPEN));
	}
	{
		CmLocking cml(NodeRec);
		THROW(cml);
		THROW(GetAsyncIntrf());
		THROW(P_AsyncIntrf->OpenSession(update, sinceDlsID));
	}
	CATCHZOK
	PPSaveErrContext();
	PPRestoreErrContext();
	return ok;
}

int PPCashMachine::AsyncCloseSession(int asTempSess, DateRange * pPrd /*=0*/)
{
	int    ok = 1;
	CmLocking cml(NodeRec);
	THROW(cml);
	THROW(GetAsyncIntrf());
	THROW(P_AsyncIntrf->CloseSession(asTempSess, pPrd));
	CATCHZOK
	return ok;
}

/*static*/int PPCashMachine::AsyncCloseSession2(PPID posNodeID, DateRange * pPeriod)
{
	int    ok = 1;
	PPCashNode cn_rec;
	PPObjCashNode cn_obj;
	PPIDArray cn_list;
	PPIDArray loc_list;
	SString temp_buf;
	THROW(cn_obj.Fetch(posNodeID, &cn_rec) > 0);
	const int is_group = BIN(cn_rec.CashType == PPCMT_CASHNGROUP);
	if(is_group) {
		PPIDArray child_list;
		cn_obj.GetListByGroup(posNodeID, child_list);
		for(uint i = 0; i < child_list.getCount(); i++) {
			const PPID child_cn_id = child_list.get(i);
			PPCashNode child_node;
			if(cn_obj.Fetch(child_cn_id, &child_node) > 0) {
				// @v10.1.1 THROW_PP_S(PPCashMachine::IsAsyncCMT(child_node.CashType), PPERR_FUNCNACCEPTEDFORPOSNODE, cn_rec.Name);
				if(PPCashMachine::IsAsyncCMT(child_node.CashType)) { // @v10.1.1
					loc_list.add(child_node.LocID);
					cn_list.add(child_cn_id);
				}
			}
		}
	}
	else {
		THROW_PP_S(PPCashMachine::IsAsyncCMT(cn_rec.CashType), PPERR_FUNCNACCEPTEDFORPOSNODE, cn_rec.Name);
		cn_list.add(cn_rec.ID);
		loc_list.add(cn_rec.LocID);
	}
	{
		TSCollection <PPCashMachine> cm_list;
		cn_list.sortAndUndup();
		for(uint i = 0; i < cn_list.getCount(); i++) {
			const PPID cn_id = cn_list.get(i);
			PPCashMachine * p_cm = 0;
			THROW(p_cm = PPCashMachine::CreateInstance(cn_id));
			cm_list.insert(p_cm);
			{
				CmLocking cml(p_cm->NodeRec);
				THROW(cml);
				THROW(p_cm->GetAsyncIntrf());
				THROW(p_cm->P_AsyncIntrf->CloseSession(0, pPeriod));
			}
		}
		for(uint cmidx = 0; cmidx < cm_list.getCount(); cmidx++) {
			PPCashMachine * p_cm = cm_list.at(cmidx);
			if(p_cm && p_cm->GetAsyncIntrf()) {
				p_cm->P_AsyncIntrf->CleanUpSession();
			}
		}
	}
	CATCHZOK
	//delete p_cm;
	return ok;
}

int PPCashMachine::AsyncInteractiveQuery()
{
	int    ok = 1;
	CmLocking cml(NodeRec);
	THROW(cml);
	THROW(GetAsyncIntrf());
	THROW(P_AsyncIntrf->InteractiveQuery());
	CATCHZOK
	return ok;
}

int PPCashMachine::AsyncBrowseExcess()
{
	CSessExcFilt flt;
	flt.CashNodeID = GetNodeData().ID;
	return PPView::Execute(PPVIEW_CSESSEXC, &flt, 0, 0);
}
//
// Общие методы
//
int PPCashMachine::GetNode()
{
	int    r = PPRef->GetItem(PPOBJ_CASHNODE, NodeID, &NodeRec);
	return (r > 0) ? 1 : ((r == 0) ? 0 : (PPErrCode = PPERR_INVCASHNODEID, -1));
}

int PPCashMachine::UpdateNode(int use_ta)
{
	return PPRef->UpdateItem(PPOBJ_CASHNODE, NodeID, &NodeRec, 1, use_ta);
}
//
// Панель операций с кассовыми сессиями
//
CSPanel::CSPanel(uint rezID, PPID aNodeID, char * nodeName, int fZReport) : TDialog(rezID)
{
	PPCashNode cn_rec;
	PPObjCashNode cn_obj;
	cn_obj.Search(aNodeID, &cn_rec);
	UseKeyboardWKey = BIN(fZReport >= 0 && (cn_rec.Flags & CASHF_KEYBOARDWKEY));
	DefInputLine = CTL_CSPANEL_INPUT;
	showCtrl(CTL_CSPANEL_INPUT, 0);
	enableCommand(cmCSClose, BIN(fZReport > 0 && (fZReport & fcspZReport)));
	enableCommand(cmSCSZReportCopy, BIN(fZReport > 0 && (fZReport & fcspZRepCopy)));
	enableCommand(cmSCSXReport, BIN(fZReport & fcspXReport));
	enableCommand(cmSCSPrintCheckCopy, BIN(fZReport & fcspCheckCopy)); // @v10.9.6
	setCtrlData(CTL_CSPANEL_NODE, nodeName);
	setCtrlReadOnly(CTL_CSPANEL_NODE, 1);
}

void CSPanel::ResetButtons()
{
	SString  input;
	if(UseKeyboardWKey && getCtrlString(CTL_CSPANEL_INPUT, input) && input.IsDigit()) {
		long  key_pos = input.ToLong();
		PPIDArray oper_rights_ary;
		if(GetOperRightsByKeyPos(key_pos, &oper_rights_ary) > 0) {
			enableCommand(cmCSClose, oper_rights_ary.lsearch(CSESSRT_CLOSE));
			enableCommand(cmSCSZReportCopy, oper_rights_ary.lsearch(CSESSOPRT_COPYZREPT));
			enableCommand(cmSCSXReport,     oper_rights_ary.lsearch(CSESSOPRT_XREP << 16));
		}
	}
	setCtrlString(CTL_CSPANEL_INPUT, input.Z());
}

IMPL_HANDLE_EVENT(CSPanel)
{
	TDialog::handleEvent(event);
	if(TVCOMMAND) {
		ushort c = TVCMD;
		if(oneof12(c, cmCSOpen, cmCSClose, cmACSUpdate, cmCSViewCheckList, cmACSViewExcess, cmACSGetChecks,
			cmSCSLock, cmSCSUnlock, cmSCSXReport, cmSCSIncasso, cmSCSZReportCopy, cmPosNodeQuery) ||
			c == cmSCSPrintCheckCopy) { // @v10.9.6 cmSCSPrintCheckCopy
			if(IsInState(sfModal)) {
				endModal(c);
				return; // После endModal не следует обращаться к this
			}
		}
	}
	else if(event.isKeyDown(kbF11)) {
		ResetButtons();
		clearEvent(event);
	}
}

int AsyncCashnPrepDialog(AsyncPosPrepParam * pParam)
{
	int    ok = -1, valid_data = 0;
	PPObjCashNode::SelFilt f;
	AsyncPosPrepParam param;
	if(!RVALUEPTR(param, pParam))
		MEMSZERO(param);
	TDialog * dlg = new TDialog(DLG_CASHNPAR);
	THROW_MEM(dlg);
	THROW(CheckDialogPtr(&dlg));
	dlg->AddClusterAssoc(CTL_CASHNPAR_FLAGS, 0, AsyncPosPrepParam::fUpdateOnly);
	dlg->SetClusterData(CTL_CASHNPAR_FLAGS, param.Flags);
	f.LocID = 0;
	f.SyncGroup = 2; // only async nodes
	SetupPPObjCombo(dlg, CTLSEL_CASHNPAR_NODE, PPOBJ_CASHNODE, param.CashNodeID, 0, &f);
	while(!valid_data && ExecView(dlg) == cmOK) {
		dlg->getCtrlData(CTLSEL_CASHNPAR_NODE, &param.CashNodeID);
		dlg->GetClusterData(CTL_CASHNPAR_FLAGS, &param.Flags);
		if(param.CashNodeID) {
			ASSIGN_PTR(pParam, param);
			valid_data = ok = 1;
		}
		else
			PPError(PPERR_INVCASHNODEID);
	}
	CATCHZOK
	delete dlg;
	return ok;
}

int SelectCSession(PPID cashNodeID, PPID extCashNodeID, CSessInfo * pInfo)
{
	int    ok = -1, valid_data = 0;
	LDATE  dt = getcurdate_();
	PPObjCSession cs_obj; // @v10.4.2
	// @v10.4.2 CCheckCore    cc_core;
	// @v10.4.2 CSessionCore  cs_core;
	CSessInfo     cs_info;
	MEMSZERO(cs_info);
	TDialog * dlg = new TDialog(SLS.CheckUiFlag(sluifUseLargeDialogs) ? DLG_SELSESNO_L : DLG_SELSESNO);
	THROW(CheckDialogPtr(&dlg));
	dlg->SetupCalDate(CTLCAL_SELSESNO_DATE, CTL_SELSESNO_DATE);
	dlg->disableCtrl(CTL_SELSESNO_EXTCASHNODE, extCashNodeID ? 0 : 1);
	dlg->setCtrlData(CTL_SELSESNO_DATE, &dt);
	while(!valid_data && ExecView(dlg) == cmOK) {
		long   sess_no = dlg->getCtrlLong(CTL_SELSESNO_NUMBER);
		dlg->getCtrlData(CTL_SELSESNO_DATE, &dt);
		ushort v = dlg->getCtrlUInt16(CTL_SELSESNO_EXTCASHNODE);
		PPID   cn_id = v ? extCashNodeID : cashNodeID;
		if(checkdate(dt, 1)) {
			if(!sess_no)
				cs_obj.P_Tbl->GetLastNumber(cn_id, cn_id, &sess_no);
			PPID   sess_id = 0;
			if(sess_no > 0 && cs_obj.P_Tbl->SearchByNumber(&sess_id, cn_id, cn_id, sess_no, dt) > 0 && cs_obj.P_Tbl->Search(sess_id, &cs_info.Rec) > 0) {
				long   _f = cs_obj.P_Tbl->GetCcGroupingFlags(cs_info.Rec, 0);
				_f |= CCheckCore::gglfUseFullCcPackets;
				cs_obj.P_Cc->GetSessTotal(sess_id, _f, &cs_info.Total, 0);
				valid_data = ok = v ? 2 : 1;
			}
		}
		if(!valid_data)
			PPError(PPERR_CSESS_NOTFOUND);
	}
	CATCHZOK
	delete dlg;
	ASSIGN_PTR(pInfo, cs_info);
	return ok;
}

int ExecCSPanel(const CashNodePaneFilt * pFilt)
{
	MemLeakTracer mlt;
	int    ok = -1;
	int    is_single = 0;
	PPID   ext_cash_node_id = 0;
	PPCashMachine * p_cm = 0;
	PPCashMachine * p_cm_ext = 0;
	CSPanel  * dlg  = 0;
	PPBnkTerminal * p_bnk_term = 0;
	PPID   cn_id = (pFilt && pFilt->CashNodeID) ? (is_single = 1, pFilt->CashNodeID) : PPObjCashNode::Select(0, 0, &is_single);
	if(cn_id > 0) {
		PPObjCSession cs_obj;
		int    sync = 0;
		int    async = 0;
		int    is_locked = 0;
		int    csp_flags = 0;
		int    cmd_from_filt = BIN(pFilt && pFilt->CommandID);
		PPIDArray child_list;
		PPIDArray loc_list;
		PPObjCashNode cn_obj;
		PPCashNode    cn_rec;
		THROW(cn_obj.Search(cn_id, &cn_rec) > 0);
		const int is_group = BIN(cn_rec.CashType == PPCMT_CASHNGROUP);
		if(is_group) {
			cn_obj.GetListByGroup(cn_id, child_list);
			for(uint i = 0; i < child_list.getCount(); i++) {
				const PPID child_cn_id = child_list.get(i);
				PPCashNode child_node;
				if(cn_obj.Fetch(child_cn_id, &child_node) > 0) {
					loc_list.add(child_node.LocID);
					const int child_sync = PPCashMachine::IsSyncCMT(child_node.CashType);
					const int child_async = PPCashMachine::IsAsyncCMT(child_node.CashType);
					SETIFZ(sync, child_sync);
					SETIFZ(async, child_async);
				}
			}
		}
		else {
			sync  = PPCashMachine::IsSyncCMT(cn_rec.CashType);
			async = PPCashMachine::IsAsyncCMT(cn_rec.CashType);
			loc_list.add(cn_rec.LocID);
		}
		THROW_PP_S((sync && !async) || (!sync && async), PPERR_CNUNDEFSYNC, cn_rec.Name);
		THROW_PP_S(loc_list.getCount(), PPERR_UNDEFCASHNODELOC, cn_rec.Name);
		{
			PPIDArray rt_loc_list;
			PPObjLocation loc_obj;
			bool   is_loc_restictions = false;
			THROW(loc_obj.GetWarehouseList(&rt_loc_list, &is_loc_restictions));
			if(is_loc_restictions) {
				SString  loc_name;
				for(uint i = 0; i < loc_list.getCount(); i++) {
					const PPID loc_id = loc_list.get(i);
					GetLocationName(loc_id, loc_name);
					THROW_PP_S(rt_loc_list.lsearch(loc_id), PPERR_NORTTOLOC, loc_name);
				}
			}
		}
		is_locked = PPObjCashNode::IsLocked(cn_id);
		if(is_locked <= 0 || (cn_rec.Flags & CASHF_WORKWHENLOCK || CONFIRM(PPCFM_VIEWLOCKEDCASHNODE))) {
			int    c = cmCancel;
			int    csp_flags = 0;
			if(is_locked < 0)
				is_locked = 0;
			SETFLAG(csp_flags, CSPanel::fcspZReport,  cs_obj.CheckRights(CSESSRT_CLOSE, 0));
			SETFLAG(csp_flags, CSPanel::fcspZRepCopy, cs_obj.CheckRights(CSESSOPRT_COPYZREPT, 1));
			SETFLAG(csp_flags, CSPanel::fcspXReport,  cs_obj.CheckRights(CSESSOPRT_XREP, 1));
			dlg = new CSPanel((sync ? DLG_SCSPANEL : DLG_ACSPANEL), cn_id, cn_rec.Name, is_locked ? -1 : csp_flags);
			THROW(CheckDialogPtr(&dlg));
			THROW(p_cm = PPCashMachine::CreateInstance(cn_id));
			if(sync) {
				PPSyncCashNode  scn;
				if(cn_obj.GetSync(cn_id, &scn) > 0) {
					ext_cash_node_id = scn.ExtCashNodeID;
					if(ext_cash_node_id) {
						THROW(p_cm_ext = PPCashMachine::CreateInstance(ext_cash_node_id));
						p_cm_ext->SetParentNode(cn_id);
					}
				}
				if(is_locked) {
					dlg->enableCommand(cmCSOpen,     0);
					dlg->enableCommand(cmCSClose,    0);
					dlg->enableCommand(cmSCSXReport, 0);
					dlg->enableCommand(cmSCSZReportCopy, 0);
					dlg->enableCommand(cmSCSLock,    0);
					dlg->enableCommand(cmSCSUnlock,  0);
					dlg->enableCommand(cmSCSIncasso, 0);
				}
			}
			else {
				if(is_locked) {
					dlg->enableCommand(cmCSOpen,       0);
					dlg->enableCommand(cmCSClose,      0);
					dlg->enableCommand(cmACSUpdate,    0);
					dlg->enableCommand(cmACSGetChecks, 0);
				}
				else if(!cs_obj.CheckRights(CSESSRT_OPEN))
					dlg->enableCommand(cmCSOpen,       0);
				dlg->showCtrl(CTL_CSPANEL_GETCHECKS, cn_rec.CashType == PPCMT_CRCSHSRV);
			}
			dlg->selectButton(cmCSViewCheckList);
			while(cmd_from_filt || (dlg && (c = ExecView(dlg)) != cmCancel)) {
				int    do_exit_loop = 0;
				if(cmd_from_filt) {
					c = (is_locked && pFilt->CommandID != cmCSViewCheckList) ? 0 : pFilt->CommandID;
					do_exit_loop = 1;
				}
				if(sync) {
					LDATE  dt = ZERODATE;
					switch(c) {
						case cmCSOpen:
							ok = p_cm->SyncOpenSession(&dt);
							if(p_cm_ext) {
								if(p_cm_ext->GetNodeData().CashType == PPCMT_PAPYRUS) {
									if(ok > 0)
										ok = p_cm_ext->AsyncOpenSession(0, 0);
								}
								else if(ok>= 0) {
									if(ok == 0)
										PPError();
									PPObjCashNode::Lock(ext_cash_node_id);
									ok = p_cm_ext->SyncOpenSession(&dt);
									PPObjCashNode::Unlock(ext_cash_node_id);
								}
							}
							break;
						case cmCSClose:
							{
								SString zcheck;
								PPSyncCashNode scn;
								if(cn_obj.GetSync(cn_id, &scn) > 0 && GetBnkTerm(scn.BnkTermType, scn.BnkTermLogNum, scn.BnkTermPort, scn.BnkTermPath, &p_bnk_term) > 0) {
									assert(p_bnk_term);
								}
								// @vmiller {
								/* @v11.0.9 Экспериментально перемещаем печать сверки по банку после печати z-отчета
								if(p_bnk_term) {
									if(p_bnk_term->GetSessReport(zcheck))
										p_cm->SyncPrintBnkTermReport(scn.BnkTermFlags, zcheck);
									else
										PPError();
								}
								*/
								// } @vmiller
								ok = p_cm->SyncCloseSession();
								if(p_cm_ext) {
									if(p_cm_ext->GetNodeData().CashType == PPCMT_PAPYRUS) {
										if(ok > 0)
											ok = p_cm_ext->AsyncCloseSession(0, 0);
									}
									else if(ok >= 0) {
										if(ok == 0)
											PPError();
										PPObjCashNode::Lock(ext_cash_node_id);
										ok = p_cm_ext->SyncCloseSession();
										PPObjCashNode::Unlock(ext_cash_node_id);
									}
								}
								// @v11.0.9 Экспериментально перемещаем печать сверки по банку после печати z-отчета
								if(p_bnk_term) {
									if(p_bnk_term->GetSessReport(zcheck)) {
										if(CConfig.Flags & CCFLG_DEBUG) {
											//
											// В одном из магазинов при снятии Z-отчета после печати банковского слипа загибается касса viki-print
											// Данный дамп призван помочь идентифицировать проблему.
											//
											SString temp_buf;
											PPGetFilePath(PPPATH_LOG, "bnkterm_zrep_dump.txt", temp_buf);
											SFile f_debug(temp_buf, SFile::mAppend|SFile::mBinary);
											if(f_debug.IsValid()) {
												temp_buf.Z().CR().Cat(getcurdatetime_(), DATF_ISO8601, 0).CR();
												f_debug.WriteLine(temp_buf);
												f_debug.Write(zcheck.cptr(), zcheck.Len());
											}
										}
										p_cm->SyncPrintBnkTermReport(scn.BnkTermFlags, zcheck);
									}
									else
										PPError();
								}
								//
							}
							break;
						case cmCSViewCheckList:
							ZDELETE(dlg);
							do_exit_loop = 1;
							ok =  p_cm->SyncBrowseCheckList(0, 0);
							break;
						case cmSCSLock:
							ok =  p_cm->SyncLockCashKeyb();
							break;
						case cmSCSUnlock:
							ok =  p_cm->SyncUnlockCashKeyb();
							break;
						case cmSCSXReport:
							if(cs_obj.CheckRights(CSESSOPRT_XREP, 1)) {
								ok = p_cm->SyncPrintXReport();
								if(p_cm_ext) {
									if(ok == 0)
										PPError();
									PPObjCashNode::Lock(ext_cash_node_id);
									ok = p_cm_ext->SyncPrintXReport();
									PPObjCashNode::Unlock(ext_cash_node_id);
								}
							}
							else
								ok = 0;
							break;
						case cmSCSZReportCopy:
							{
								CSessInfo  cs_info;
								ok = SelectCSession(cn_id, ext_cash_node_id, &cs_info);
								if(ok == 1)
									ok = p_cm->SyncPrintZReportCopy(&cs_info);
								else if(ok == 2)
									if(p_cm_ext) {
										PPObjCashNode::Lock(ext_cash_node_id);
										ok = p_cm_ext->SyncPrintZReportCopy(&cs_info);
										PPObjCashNode::Unlock(ext_cash_node_id);
									}
									else
										ok = -1;
								break;
							}
						case cmSCSIncasso:      ok =  p_cm->SyncPrintIncasso();      break;
						default:                ok = -1; break;
					}
				}
				else if(async) {
					switch(c) {
						case cmCSOpen:          ok =  p_cm->AsyncOpenSession(0, 0);  break;
						case cmACSUpdate:       ok =  p_cm->AsyncOpenSession(1, 0);  break;
						case cmCSClose:
							// @v10.1.1 ok =  p_cm->AsyncCloseSession(0, 0);
							ok = PPCashMachine::AsyncCloseSession2(cn_id, 0); // @v10.1.1
							break;
						case cmACSGetChecks:    ok =  p_cm->AsyncCloseSession(1, 0); break;
						case cmCSViewCheckList: ok =  p_cm->AsyncBrowseCheckList();  break;
						case cmACSViewExcess:   ok =  p_cm->AsyncBrowseExcess();     break;
						case cmPosNodeQuery:    ok =  p_cm->AsyncInteractiveQuery(); break;
						default:                ok = -1; break;
					}
				}
				else {
					do_exit_loop = 1;
				}
				if(ok == 0)
					PPError();
				if(do_exit_loop)
					break;
				ok = 1;
			}
			ZDELETE(p_cm);
			ZDELETE(p_cm_ext);
			if(is_locked && dlg) {
				dlg->enableCommand(cmCSOpen,  1);
				dlg->enableCommand(cmCSClose, 1);
				if(sync) {
					dlg->enableCommand(cmSCSXReport, 1);
					dlg->enableCommand(cmSCSLock,    1);
					dlg->enableCommand(cmSCSUnlock,  1);
					dlg->enableCommand(cmSCSIncasso,  1);
				}
				else {
					dlg->enableCommand(cmACSUpdate,    1);
					dlg->enableCommand(cmACSGetChecks, 1);
				}
			}
			delete dlg;
			dlg = 0;
		}
	}
	CATCHZOKPPERR
	delete p_cm;
	delete p_cm_ext;
	ZDELETE(p_bnk_term);
	delete dlg;
	return ok;
}

int ViewCashBills(PPID cashNodeID)
{
	int    ok = 1;
	PPCashMachine * p_cm = 0;
	if(cashNodeID == 0) {
		THROW(cashNodeID = PPObjCashNode::Select(LConfig.Location, 1));
	}
	if(cashNodeID > 0) {
		THROW(p_cm = PPCashMachine::CreateInstance(cashNodeID));
		THROW(p_cm->SyncBrowseCheckList(0, 0));
	}
	CATCHZOKPPERR
	delete p_cm;
	return ok;
}

int CloseCashDay()
{
	int    ok = 1;
	PPID   cash_node_id = 0;
	PPCashMachine * p_cm = 0;
	THROW_PP(BillObj->CheckRights(BILLRT_CLOSECASH), PPERR_NORTCLOSECASH);
	THROW(cash_node_id = PPObjCashNode::Select(LConfig.Location, 1));
	if(cash_node_id > 0) {
		THROW(p_cm = PPCashMachine::CreateInstance(cash_node_id));
		THROW(p_cm->SyncCloseSession());
	}
	CATCHZOKPPERR
	delete p_cm;
	return ok;
}
