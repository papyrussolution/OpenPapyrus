// CREVAL.CPP
// Copyright (c) A.Sobolev 2000-2002, 2003, 2004, 2006, 2007, 2008, 2015, 2016, 2017
// @codepage windows-1251
// Валютная переоценка
//
#include <pp.h>
#pragma hdrstop
//
//
//
class CRevalDialog : public PPListDialog {
public:
	CRevalDialog() : PPListDialog(DLG_CREVAL, CTL_CREVAL_ACCLIST)
	{
		SetupCalDate(CTLCAL_CREVAL_DT, CTL_CREVAL_DT);
		SmartListBox * p_list = (SmartListBox*)getCtrlView(CTL_CREVAL_CRATELIST);
		if(!SetupStrListBox(p_list))
			PPError();
		updateList(-1);
		updateCRateList();
	}
	int    setDTS(const CurRevalParam *);
	int    getDTS(CurRevalParam *);
private:
	DECL_HANDLE_EVENT;
	virtual int  setupList();
	virtual int  addItem(long * pos, long * id);
	virtual int  delItem(long pos, long id);
	int    getAcc(uint ctlID, Acct *);
	void   editCRate();
	void   updateCRateList();
	int    setupCRateList(PPID accID, int * pIsCurAcc);

	CurRevalParam Data;
	PPObjAccount AccObj;
};

int CRevalDialog::setDTS(const CurRevalParam * pData)
{
	int    ok = 1;
	Data = *pData;
	setCtrlData(CTL_CREVAL_DT, &Data.Dt);
	setCtrlData(CTL_CREVAL_CORRACC,    &Data.CorrAcc);
	setCtrlData(CTL_CREVAL_NEGCORRACC, &Data.NegCorrAcc);
	SetupPPObjCombo(this, CTLSEL_CREVAL_LOC, PPOBJ_LOCATION, Data.LocID, 0, 0);
	for(uint i = 0; i < Data.AccList.getCount(); i++)
		setupCRateList(Data.AccList.at(i), 0);
	updateList(-1);
	updateCRateList();
	return ok;
}

int CRevalDialog::getAcc(uint ctl, Acct * pAcc)
{
	AcctID acctid;
	getCtrlData(ctl, pAcc);
	//pAcc->ar = 0;
	return (pAcc->ac == 0 && pAcc->sb == 0) ? -1 : ((BillObj->atobj->ConvertAcct(pAcc, 0L, &acctid, 0) > 0) ? 1 : (selectCtrl(ctl), 0));
}

int CRevalDialog::getDTS(CurRevalParam * pData)
{
	int    ok = 1, r;
	uint   i, sel = 0;
	PPID * p_acc_id = 0;
	getCtrlData(CTL_CREVAL_DT, &Data.Dt);
	THROW_SL(checkdate(Data.Dt, 0));
	getCtrlData(sel = CTLSEL_CREVAL_LOC, &Data.LocID);
	THROW_PP(Data.LocID, PPERR_LOCNEEDED);
	THROW(r = getAcc(sel = CTL_CREVAL_CORRACC, &Data.CorrAcc));
	THROW_PP(r > 0, PPERR_ACCNEEDED);
	THROW(r = getAcc(sel = CTL_CREVAL_NEGCORRACC, &Data.NegCorrAcc));
	sel = CTL_CREVAL_CRATELIST;
	for(i = 0; Data.AccList.enumItems(&i, (void**)&p_acc_id);) {
		PPIDArray cur_list;
		if(AccObj.GetCurList(*p_acc_id, 0, &cur_list) > 0) {
			PPID * p_cur_id = 0;
	   	    for(uint j = 0; cur_list.enumItems(&j, (void**)&p_cur_id);) {
		   	    if(*p_cur_id) {
					const double crate = Data.CRateList.Get(PPAMT_CRATE, *p_cur_id);
					THROW_PP(crate > 0.0, PPERR_INVCRATE);
				}
			}
		}
	}
	*pData = Data;
	CATCH
		ok = PPErrorByDialog(this, sel);
	ENDCATCH
	return ok;
}

IMPL_HANDLE_EVENT(CRevalDialog)
{
	if(TVCOMMAND && (TVCMD == cmaInsert || TVCMD == cmaDelete)) {
		TView * p_list = getCtrlView(CTL_CREVAL_CRATELIST);
		if(p_list && p_list->IsInState(sfSelected))
			clearEvent(event);
	}
	PPListDialog::handleEvent(event);
	if(TVCOMMAND) {
		if(TVCMD == cmCRevalUpdateCRate)
			editCRate();
		else if(TVCMD == cmLBDblClk) {
			if(event.isCtlEvent(CTL_CREVAL_CRATELIST))
				editCRate();
		}
		else
			return;
	}
	else if(TVBROADCAST) {
		if(event.isCtlEvent(CTL_CREVAL_CRATELIST)) {
			enum { ok_button, edit_button } toggle;
			if(TVCMD == cmReceivedFocus)
				toggle = edit_button;
			else if(TVCMD == cmReleasedFocus)
				toggle = ok_button;
			else
				return;
			SetDefaultButton(STDCTL_OKBUTTON, toggle == ok_button);
			SetDefaultButton(CTL_CREVAL_UPDCRBUTTON, toggle == edit_button);
			return;
		}
		else if(TVCMD == cmReleasedFocus || TVCMD == cmCommitInput)
			if(TVINFOVIEW->TestId(CTL_CREVAL_DT)) {
				const LDATE dt = getCtrlDate(CTL_CREVAL_DT);
				if(checkdate(dt, 0)) {
					Data.Dt = dt;
					setupCRateList(0, 0);
				}
			}
			else
				return;
	}
	else
		return;
	clearEvent(event);
}

void CRevalDialog::editCRate()
{
	SmartListBox * p_list = (SmartListBox*)getCtrlView(CTL_CREVAL_CRATELIST);
	if(p_list) {
		const long pos = p_list->def->_curItem();
		if(pos >= 0 && pos < (long)Data.CRateList.getCount()) {
			AmtEntry * p_entry = &Data.CRateList.at((uint)pos);
			const LDATE dt = getCtrlDate(CTL_CREVAL_DT);
			double rate = p_entry->Amt;
			if(SelectCurRate(p_entry->CurID, LConfig.BaseRateTypeID, &rate) > 0) {
				p_entry->AmtTypeID = PPAMT_CRATE;
				p_entry->Amt       = rate;
				updateCRateList();
			}
		}
	}
}

void CRevalDialog::updateCRateList()
{
	AmtEntry * p_entry = 0;
	SmartListBox * p_list = (SmartListBox*)getCtrlView(CTL_CREVAL_CRATELIST);
	if(p_list) {
		lock();
		const int sav_pos = (int)p_list->def->_curItem();
		StringSet ss(SLBColumnDelim);
		p_list->freeAll();
		for(uint i = 0; Data.CRateList.enumItems(&i, (void**)&p_entry);) {
			char sub[64];
			PPCurrency cur_rec;
			ss.clear(1);
			if(p_entry->CurID && PPRef->GetItem(PPOBJ_CURRENCY, p_entry->CurID, &cur_rec) > 0)
				ss.add(cur_rec.Symb);
			else
				ss.add(intfmt(p_entry->CurID, MKSFMT(0, NMBF_NOZERO), sub));
			ss.add(realfmt(p_entry->Amt, MKSFMTD(0, 6, 0), sub));
			if(!p_list->addItem(p_entry->CurID, ss.getBuf())) {
				PPError(PPERR_SLIB, 0);
				break;
			}
		}
		p_list->focusItem(sav_pos);
		p_list->Draw_();
		unlock();
	}
}

int CRevalDialog::setupList()
{
	int    ok = 1;
	PPID * p_acc_id = 0;
	for(uint i = 0; ok && Data.AccList.enumItems(&i, (void**)&p_acc_id);) {
		PPAccount acc_rec;
		if(AccObj.Search(*p_acc_id, &acc_rec) > 0) {
			char   sub[64];
			StringSet ss(SLBColumnDelim);
			ss.add(((const Acct*)&acc_rec.A.Ac)->ToStr(ACCF_BAL|ACCF_DEFAULT, sub));
			ss.add(acc_rec.Name);
			if(!addStringToList(acc_rec.ID, ss.getBuf()))
				return 0;
		}
	}
	return ok;
}

int CRevalDialog::setupCRateList(PPID accID, int * pIsCurAcc)
{
	int    ok = 1;
	PPObjBill * p_bobj = BillObj;
	uint   i;
	double crate;
	PPIDArray cur_list;
	int    is_cur_acc = 0;
	if(!accID) {
		AmtEntry * p_entry;
		for(i = 0; Data.CRateList.enumItems(&i, (void **)&p_entry);) {
			p_bobj->GetCurRate(p_entry->CurID, &Data.Dt, &(crate = 0.0));
			p_entry->Amt = crate;
		}
	}
	else if(AccObj.GetCurList(accID, 0, &cur_list) > 0) {
		PPID * p_cur_id = 0;
   	    for(i = 0; cur_list.enumItems(&i, (void**)&p_cur_id);) {
	   	    if(*p_cur_id) {
				is_cur_acc = 1;
				if(Data.CRateList.Search(PPAMT_CRATE, *p_cur_id, 0) <= 0) {
					p_bobj->GetCurRate(*p_cur_id, &Data.Dt, &(crate = 0.0));
					Data.CRateList.Add(PPAMT_CRATE, *p_cur_id, crate, 0);
   	        	}
			}
	   	}
	}
	updateCRateList();
	ASSIGN_PTR(pIsCurAcc, is_cur_acc);
	return ok;
}

int CRevalDialog::addItem(long * pPos, long * pID)
{
	int    ok = -1;
	PPID   id = 0;
	if(PPSelectObject(PPOBJ_ACCOUNT2, &id, 0, (void *)-1L) > 0) {
		int    is_cur_acc = 0;
		setupCRateList(id, &is_cur_acc);
		if(is_cur_acc) {
			Data.AccList.add(id);
			ASSIGN_PTR(pPos, Data.AccList.getCount()-1);
			ASSIGN_PTR(pID, id);
			ok = 1;
		}
	}
	return ok;
}

int CRevalDialog::delItem(long pos, long /*id*/)
{
	return Data.AccList.atFree((uint)pos) ? 1 : -1;
}

int SLAPI EditCRevalParam(CurRevalParam * pData)
{
	DIALOG_PROC_BODY(CRevalDialog, pData);
}

#define __HDR_DW_COUNT 9

int SLAPI PutCurRevalConfig(CurRevalParam * pData, int use_ta)
{
	int    ok = 1;
	PPIDArray temp;
	if(pData) {
		uint i;
		union {
			long lval[2];
			Acct accval;
		} temp_val;
		temp_val.accval = pData->CorrAcc;
		THROW_SL(temp.add(temp_val.lval[0]));
		THROW_SL(temp.add(temp_val.lval[1]));
		temp_val.accval = pData->NegCorrAcc;
		THROW_SL(temp.add(temp_val.lval[0]));
		THROW_SL(temp.add(temp_val.lval[1]));
		THROW_SL(temp.add(pData->Flags));
		for(i = 0; i < 4; i++)
			THROW_SL(temp.add(0L));
		for(i = 0; i < pData->AccList.getCount(); i++)
			THROW_SL(temp.add(pData->AccList.at(i)));
	}
	THROW(PPRef->PutPropArray(PPOBJ_CONFIG, PPCFG_MAIN, PPPRP_CURREVALCFG, (pData ? &temp : (PPIDArray*)0), use_ta));
	CATCHZOK
	return ok;
}

int SLAPI GetCurRevalConfig(CurRevalParam * pData)
{
	uint   i;
	PPIDArray temp;
	if(PPRef->GetPropArray(PPOBJ_CONFIG, PPCFG_MAIN, PPPRP_CURREVALCFG, &temp) > 0) {
		if(temp.getCount() < __HDR_DW_COUNT) {
			CALLPTRMEMB(pData, Init());
	   	    return -1;
		}
		if(pData) {
			union {
				long lval[2];
				Acct accval;
			} temp_val;
			temp_val.lval[0] = temp.at(0);
			temp_val.lval[1] = temp.at(1);
			pData->CorrAcc   = temp_val.accval;
			temp_val.lval[0] = temp.at(2);
			temp_val.lval[1] = temp.at(3);
			pData->NegCorrAcc = temp_val.accval;
			pData->Flags = temp.at(4);
			for(i = __HDR_DW_COUNT; i < temp.getCount(); i++)
				if(!pData->AccList.add(temp.at(i))) {
					pData->Init();
					return PPSetErrorSLib();
				}
		}
		return 1;
	}
	return -1;
}

int SLAPI CurReval()
{
	int    ok = -1;
	CurRevalParam param;
	GetCurRevalConfig(&param);
	param.Dt = LConfig.OperDate;
	param.LocID = LConfig.Location;
	while(ok <= 0 && EditCRevalParam(&param) > 0) {
		PutCurRevalConfig(&param, 1);
		if(BillObj->atobj->P_Tbl->RevalCurRests(&param))
			ok = 1;
		else
			ok = PPErrorZ();
	}
	return ok;
}
