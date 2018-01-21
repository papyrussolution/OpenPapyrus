// DGACC.CPP
// Copyright (c) A.Sobolev 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2005, 2006, 2007, 2008, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018
// @codepage windows-1251
// Диалоговая группа ввода счета и аналитической статьи
//
#include <pp.h>
#pragma hdrstop

AcctCtrlGroup::AcctCtrlGroup(uint _ctl_acc, uint _ctl_art, uint _ctlsel_accnam, uint _ctlsel_artnam) : CtrlGroup(),
	ctl_acc(_ctl_acc), ctl_art(_ctl_art), ctlsel_accname(_ctlsel_accnam), ctlsel_artname(_ctlsel_artnam),
	AccSheetID(0), CurID(0), AccSelParam(0)
{
	AcctId.Clear();
	ppobj = new PPObjAccTurn(0);
}

AcctCtrlGroup::~AcctCtrlGroup()
{
	delete ppobj;
}

int AcctCtrlGroup::setData(TDialog * dlg, void * data)
{
	int    acc_sel_param_changed = 0;
	Rec  * p_rec = (Rec*)data;
	Acct   acct;
	MEMSZERO(acct);
	AcctId     = p_rec->AcctId;
	AccSheetID = p_rec->AccSheetID;
	if(AccSelParam != p_rec->AccSelParam)
		acc_sel_param_changed = 1;
	AccSelParam = p_rec->AccSelParam;
	ppobj->ConvertAcctID(&AcctId, &acct, &CurID, 1 /* useCache */);
	setup(dlg, &acct, 1, acc_sel_param_changed);
	return 1;
}

int AcctCtrlGroup::getData(TDialog * dlg, void * pData)
{
	int    ok = 1;
	Rec  * p_rec = (Rec*)pData;
	char   b[48];
	Acct   acct;
	PPAccount acc_rec;
	p_rec->AccType = -1;
	if(dlg->getCtrlData(ctl_acc, b) && *strip(b)) {
		THROW(ppobj->ConvertStr(b, CurID, &acct, &AcctId, &AccSheetID));
		THROW(ObjRts.CheckAccID(AcctId.ac, PPR_READ));
		if(ppobj->P_Tbl->AccObj.Search(AcctId.ac, &acc_rec) > 0)
			p_rec->AccType = acc_rec.Type;
		if(AcctId.ar == 0 && dlg->getCtrlData(ctl_art, b) && *strip(b)) {
			THROW_PP((acct.ar = atol(b)) > 0, PPERR_USERINPUT);
			THROW(ppobj->P_Tbl->Art.SearchNum(AccSheetID, acct.ar) > 0);
			AcctId.ar = ppobj->P_Tbl->Art.data.ID;
		}
	}
	else
		AcctId.ac = AcctId.ar = AccSheetID = 0;
	p_rec->AcctId = AcctId;
	p_rec->AccSheetID = AccSheetID;
	CATCHZOK
	return ok;
}

void AcctCtrlGroup::setup(TDialog * dlg, Acct * pAcct, int sheetChanged, int accSelParamChanged)
{
	char   b[32];
	dlg->setCtrlData(ctl_acc, pAcct->ToStr(ACCF_DEFAULT|ACCF_BAL, b));
	if(AccSheetID && pAcct->ar)
		ltoa(pAcct->ar, b, 10);
	else
		b[0] = 0;
	if(ctl_art) {
		dlg->setCtrlData(ctl_art, b);
		dlg->disableCtrl(ctl_art, !AccSheetID);
		dlg->disableCtrl(ctlsel_artname, !AccSheetID);
		if(AccSheetID) {
			TView * p = dlg->GetCurrentView();
			if(p) {
				do {
					p = p->P_Next;
				} while(p != dlg->GetCurrentView() && !p->TestId(ctl_art));
				if(p->TestId(ctl_art))
					p->select();
			}
		}
	}
	ComboBox * p_combo = (ComboBox *)dlg->getCtrlView(ctlsel_accname);
	if(p_combo) {
		if(p_combo->listDef() && !accSelParamChanged) {
			if(AcctId.ac)
				p_combo->TransmitData(+1, &AcctId.ac);
			else
				p_combo->setInputLineText(0);
		}
		else
			SetupPPObjCombo(p_combo, PPOBJ_ACCOUNT2, AcctId.ac, OLW_CANINSERT, (void *)AccSelParam);
	}
	if(sheetChanged) {
		if(AccSheetID)
			SetupArCombo(dlg, ctlsel_artname, AcctId.ar, OLW_LOADDEFONOPEN|OLW_CANINSERT, AccSheetID);
		else {
			p_combo = (ComboBox *)dlg->getCtrlView(ctlsel_artname);
			if(p_combo) {
				p_combo->TransmitData(+1, &AccSheetID); // @# AccSheetID == 0
				p_combo->setInputLineText(0);
			}
		}
	}
	TView::messageCommand(dlg, cmPPAccSelected, this);
}

int AcctCtrlGroup::processAccInput(TDialog * dlg)
{
	char   b[32];
	Acct   acct;
	AcctID save_acctid  = AcctId;
	PPID   save_sheetid = AccSheetID;
	dlg->getCtrlData(ctl_acc, b);
	if(*strip(b) == 0) {
		AcctId.Clear();
		acct.Clear();
		AccSheetID = 0;
	}
	else {
		if(!ppobj->ConvertStr(b, CurID, &acct, &AcctId, &AccSheetID)) {
			PPError();
			b[0] = 0;
			dlg->setCtrlData(ctl_acc, b);
			dlg->selectCtrl(ctl_acc);
			return 0;
		}
		if(AcctId.ar == 0 && save_acctid.ar && save_sheetid == AccSheetID)
			AcctId.ar = save_acctid.ar;
	}
	if(save_acctid != AcctId)
		setup(dlg, &acct, 1);
	return 1;
}

void AcctCtrlGroup::processAccCombo(TDialog * dlg)
{
	Acct   acct;
	PPID   bal;
	dlg->getCtrlData(ctlsel_accname, &bal);
	if(bal != AcctId.ac) {
		PPAccount acc_rec;
		if(ppobj->P_Tbl->AccObj.Search(bal, &acc_rec) > 0) {
			AccSheetID = acc_rec.AccSheetID;
			acct.ac = acc_rec.A.Ac;
			acct.sb = acc_rec.A.Sb;
		}
		else {
			AccSheetID = 0;
			acct.Clear();
			PPError();
		}
		AcctId.ac = bal;
		AcctId.ar = acct.ar = 0;
		setup(dlg, &acct, 1);
	}
}

void AcctCtrlGroup::processArtCombo(TDialog * dlg)
{
	char   b[32];
	PPID   ar;
	if(ctlsel_artname) {
		dlg->getCtrlData(ctlsel_artname, &ar);
		if(ar != AcctId.ar) {
			b[0] = 0;
			if(ppobj->P_Tbl->Art.Search(ar) <= 0)
				PPError();
			else if(ar)
				ltoa(ppobj->P_Tbl->Art.data.Article, b, 10);
			dlg->setCtrlData(ctl_art, b);
			AcctId.ar = ar;
			TView::messageCommand(dlg, cmPPArSelected, this);
		}
	}
}

int AcctCtrlGroup::processArtInput(TDialog * dlg)
{
	int    r;
	long   ar, prev_ar = AcctId.ar;
	char   b[32];
	b[0] = 0;
	dlg->getCtrlData(ctl_art, b);
	if((ar = AccSheetID ? atol(b) : 0) != 0)
		if((r = ppobj->P_Tbl->Art.SearchNum(AccSheetID, ar)) <= 0) {
			PPErrorByDialog(dlg, ctl_art, r ? (PPErrCode = PPERR_ARTICLENFOUND) : PPErrCode);
			ar = 0;
		}
		else
			AcctId.ar = ppobj->P_Tbl->Art.data.ID;
	if(ar == 0) {
		AcctId.ar = 0;
		dlg->setCtrlData(ctl_art, &(b[0] = 0));
	}
	{
		ComboBox * p_combo = (ComboBox *)dlg->getCtrlView(ctlsel_artname);
		if(p_combo) {
			ListWindow * p_listwin = p_combo->getListWindow();
			if(p_listwin) {
				TView::messageCommand(p_listwin, cmLBLoadDef);
				dlg->setCtrlLong(ctlsel_artname, AcctId.ar);
			}
		}
	}
	if(prev_ar != AcctId.ar)
		TView::messageCommand(dlg, cmPPArSelected, this);
	return 1;
}

void AcctCtrlGroup::handleEvent(TDialog * dlg, TEvent & event)
{
	int    rcv;
	if(TVBROADCAST && TVCMD == cmChangedFocus && TVINFOVIEW && dlg->P_Owner) {
		if(event.isCtlEvent(ctl_acc))
			processAccInput(dlg);
		else if(event.isCtlEvent(ctl_art))
			processArtInput(dlg);
		else
			return;
	}
	else if(event.isCbSelected(ctlsel_accname))
		processAccCombo(dlg);
	else if(event.isCbSelected(ctlsel_artname))
		processArtCombo(dlg);
	else if(event.isKeyDown(DEFAULT_CBX_KEY)) {
		if(dlg->isCurrCtlID(ctl_acc))
			rcv = ctlsel_accname;
		else if(dlg->isCurrCtlID(ctl_art))
			rcv = ctlsel_artname;
		else
			return;
		dlg->messageToCtrl(rcv, cmCBActivate, 0);
	}
	else
		return;
	dlg->clearEvent(event);
}
//
//
//
SLAPI ArticleCtrlGroup::Rec::Rec() : AcsID(0), OpID(0)
{
}

SLAPI ArticleCtrlGroup::Rec::Rec(PPID acsID, PPID opID, const ObjIdListFilt * pArList) : AcsID(acsID), OpID(opID)
{
	RVALUEPTR(ArList, pArList);
}

SLAPI ArticleCtrlGroup::Rec::Rec(PPID acsID, PPID opID, PPID arID) : AcsID(acsID), OpID(opID)
{
	if(arID)
		ArList.Add(arID);
}

ArticleCtrlGroup::ArticleCtrlGroup(uint ctlselAcs, uint ctlselOp, uint ctlselAr, uint cmEditList, PPID accSheetID, long flags) :
	CtrlGroup(), Data(accSheetID, 0, 0L), Flags(flags), CtlselAcs(ctlselAcs), CtlselOp(ctlselOp), CtlselAr(ctlselAr), CmEditList(cmEditList), AccSheetID(accSheetID)
{
}

void ArticleCtrlGroup::SetAccSheet(long accSheetID)
{
	AccSheetID = accSheetID;
	Data.AcsID = accSheetID;
}

void ArticleCtrlGroup::SetupAccSheet(TDialog * pDlg)
{
	PPID   single_id = Data.ArList.GetSingle();
	long   sacf = sacfDisableIfZeroSheet;
	if(Flags & fNonEmptyExchageParam)
		sacf |= sacfNonEmptyExchageParam;
	SetupArCombo(pDlg, CtlselAr, single_id, 0, Data.AcsID, sacf);
	if(Data.ArList.GetCount() > 1) {
		SetComboBoxListText(pDlg, CtlselAr);
		pDlg->disableCtrl(CtlselAr, 1);
	}
	else {
		pDlg->setCtrlLong(CtlselAr, single_id);
		pDlg->disableCtrl(CtlselAr, 0);
	}
}

void ArticleCtrlGroup::SetupOp(TDialog * pDlg)
{
	PPID   acs_id = 0, acs2_id = 0;
	if(GetOpCommonAccSheet(Data.OpID, &acs_id, &acs2_id) > 0)
		Data.AcsID = (Flags & fByOpAccSheet2) ? acs2_id : acs_id;
	// @v8.4.1 {
	else if(CtlselAcs)
		Data.AcsID = 0;
	if(CtlselAcs) {
		pDlg->setCtrlLong(CtlselAcs, Data.AcsID);
		pDlg->disableCtrl(CtlselAcs, BIN(Data.AcsID));
	}
	// } @v8.4.1
	SetupAccSheet(pDlg);
}

int ArticleCtrlGroup::setData(TDialog * pDlg, void * pData)
{
	Data = *(Rec*)pData;
	SETIFZ(Data.AcsID, AccSheetID);
	if(CtlselAcs)
		SetupPPObjCombo(pDlg, CtlselAcs, PPOBJ_ACCSHEET, Data.AcsID, 0, 0);
	SetupOp(pDlg);
	return 1;
}

int ArticleCtrlGroup::getData(TDialog * pDlg, void * pData)
{
	Rec * p_rec = (Rec*)pData;
	if(CtlselAcs) {
		pDlg->getCtrlData(CtlselAcs, &Data.AcsID);
	}
	if(CtlselOp) {
		pDlg->getCtrlData(CtlselOp, &Data.OpID);
	}
	if(Data.ArList.GetCount() <= 1) {
		PPID   temp_id = pDlg->getCtrlLong(CtlselAr);
		Data.ArList.Set(0);
        if(temp_id)
			Data.ArList.Add(temp_id);
	}
	*p_rec = Data;
	return 1;
}

int ArticleCtrlGroup::selectByCode(TDialog * pDlg)
{
	int    ok = -1;
	ComboBox * p_combo = (ComboBox *)pDlg->getCtrlView(CtlselAr);
	if(Data.AcsID && p_combo && p_combo->link()) {
		PPID   reg_type_id = 0, ar_id = 0;
		if(PPObjArticle::GetSearchingRegTypeID(Data.AcsID, 0, 1, &reg_type_id) > 0) {
			PPObjArticle ar_obj;
			SString code, title;
			PPRegisterType reg_type_rec;
			SearchObject(PPOBJ_REGISTERTYPE, reg_type_id, &reg_type_rec);
			PPLoadText(PPTXT_SEARCHARTICLE, title);
			PPInputStringDialogParam isd_param(title, reg_type_rec.Name);
			while(ok < 0 && InputStringDialog(&isd_param, code) > 0) {
				if(ar_obj.SearchByRegCode(Data.AcsID, reg_type_id, code, &ar_id, 0) > 0) {
					ComboBox * p_combo = (ComboBox *)pDlg->getCtrlView(CtlselAr);
					if(p_combo) {
						ListWindow * p_listwin = p_combo->getListWindow();
						if(p_listwin) {
							TView::messageCommand(p_listwin, cmLBLoadDef);
							pDlg->setCtrlLong(CtlselAr, ar_id);
							TView::messageCommand(pDlg, cmCBSelected, p_combo);
						}
					}
					ok = 1;
				}
				else
					PPError();
			}
		}
	}
	return ok;
}

static int SLAPI EditArList(TDialog * pDlg, uint ctlID, PPID accSheetID, ObjIdListFilt * pArList)
{
	int    ok = -1;
	if(pArList && pDlg) {
		PPIDArray ary;
		if(pArList->IsExists())
			ary = pArList->Get();
		if(!ary.getCount())
			ary.setSingleNZ(pDlg->getCtrlLong(ctlID));
		ArticleFilt ar_filt;
		ar_filt.AccSheetID = accSheetID;
		ar_filt.Ft_Closed = -1;
		ListToListData lst(PPOBJ_ARTICLE, &ar_filt, &ary);
		lst.TitleStrID = 0; // PPTXT_XXX;
		if(ListToListDialog(&lst) > 0) {
			pArList->Set(ary.getCount() ? &ary : 0);
			if(pArList->GetCount() > 1) {
				SetComboBoxListText(pDlg, ctlID);
				pDlg->disableCtrl(ctlID, 1);
			}
			else {
				pDlg->setCtrlLong(ctlID, pArList->GetSingle());
				pDlg->disableCtrl(ctlID, 0);
			}
			ok = 1;
		}
	}
	else
		ok = 0;
	return ok;
}

void ArticleCtrlGroup::handleEvent(TDialog * pDlg, TEvent & event)
{
	if(CtlselAcs && event.isCbSelected(CtlselAcs)) {
		const PPID preserve_acs_id = Data.AcsID;
		pDlg->getCtrlData(CtlselAcs, &Data.AcsID);
		if(Data.AcsID != preserve_acs_id) {
			Data.ArList.Set(0);
			SetupAccSheet(pDlg);
		}
	}
	else if(CtlselOp && event.isCbSelected(CtlselOp)) {
		const PPID preserve_op_id = Data.OpID;
		pDlg->getCtrlData(CtlselOp, &Data.OpID);
		if(Data.OpID != preserve_op_id) {
			Data.ArList.Set(0);
			SetupOp(pDlg);
		}
	}
	else if(CtlselAr && event.isCbSelected(CtlselAr)) {
		PPID   ar_id = pDlg->getCtrlLong(CtlselAr);
		if(ar_id)
			MessagePersonBirthDay(pDlg, ObjectToPerson(ar_id, 0));
	}
	else if(CmEditList && event.isCmd(CmEditList)) {
		EditArList(pDlg, CtlselAr, Data.AcsID, &Data.ArList);
		pDlg->clearEvent(event);
	}
	else if(event.isKeyDown(kbF2)) {
		ComboBox * p_combo = (ComboBox *)pDlg->getCtrlView(CtlselAr);
		if(p_combo && pDlg->IsCurrentView(p_combo->link())) {
			selectByCode(pDlg);
			pDlg->clearEvent(event);
		}
	}
}
//
//
//
SLAPI CurAmtCtrlGroup::Rec::Rec()
{
	THISZERO();
}

CurAmtCtrlGroup::CurAmtCtrlGroup(uint amtCID, uint curSelCID, uint crateCID, uint baseAmtCID, uint dateCID, uint selCRateCmd, AmtList * pAmtList) : 
	CtrlGroup(), AmtCID(amtCID), CurSelCID(curSelCID), CRateCID(crateCID), BaseAmtCID(baseAmtCID), DateCID(dateCID), SelCRateCmd(selCRateCmd), P_AL(pAmtList)
{
	MEMSZERO(Data);
}

CurAmtCtrlGroup::~CurAmtCtrlGroup()
{
}

void CurAmtCtrlGroup::setupCurrencyCombo(TDialog * pDlg, PPIDArray * pCurList)
{
	PPID   cur_id = pDlg->getCtrlLong(CurSelCID);
	SETIFZ(cur_id, Data.CurID);
	if(pCurList && !pCurList->lsearch(cur_id))
		cur_id = 0;
	::SetupCurrencyCombo(pDlg, CurSelCID, cur_id, 0, 1, pCurList);
	if(cur_id == 0)
		setupCurRate(pDlg, 0);
}

void CurAmtCtrlGroup::setupCurRate(TDialog * pDlg, int fromBase)
{
	double crate = 0.0, base_amount = 0.0;
	PPID   cur_id = pDlg->getCtrlLong(CurSelCID);
	double amount = pDlg->getCtrlReal(AmtCID);
	if(DateCID)
		pDlg->getCtrlData(DateCID, &Data.CRateDate);
	if(cur_id) {
		pDlg->disableCtrl(CRateCID, 0);
		if(fromBase) {
			if(!P_AL || P_AL->Get(PPAMT_CRATE, cur_id, &crate) <= 0)
				BillObj->GetCurRate(cur_id, &Data.CRateDate, &crate);
			pDlg->setCtrlReal(CRateCID, crate);
		}
		else
			crate = pDlg->getCtrlReal(CRateCID);
		base_amount = R2(amount * crate);
	}
	else {
		pDlg->setCtrlReal(CRateCID, crate);
		pDlg->disableCtrl(CRateCID, 1);
	}
	if(SelCRateCmd)
		pDlg->enableCommand(SelCRateCmd, BIN(cur_id));
	pDlg->setCtrlReal(BaseAmtCID, base_amount);
}

int CurAmtCtrlGroup::setData(TDialog * pDlg, void * pData)
{
	int    use_same_crate = 0;
	Data = *(Rec *)pData;
	pDlg->disableCtrl(BaseAmtCID, 1);
	SetupCurrencyCombo(pDlg, CurSelCID, Data.CurID, 0, 1, 0);
	pDlg->setCtrlData(AmtCID, &Data.Amount);
	if(Data.CRate > 0.0) {
		pDlg->setCtrlReal(CRateCID, Data.CRate);
		use_same_crate = 1;
	}
	setupCurRate(pDlg, use_same_crate ? 0 : 1);
	return 1;
}

int CurAmtCtrlGroup::getData(TDialog * pDlg, void * pData)
{
	int    ok = 1;
	pDlg->getCtrlData(CurSelCID,  &Data.CurID);
	pDlg->getCtrlData(AmtCID,     &Data.Amount);
	pDlg->getCtrlData(CRateCID,   &Data.CRate);
	pDlg->getCtrlData(BaseAmtCID, &Data.BaseAmount);
	if(pData)
		*(Rec*)pData = Data;
	return ok;
}

void CurAmtCtrlGroup::handleEvent(TDialog * pDlg, TEvent & event)
{
	if(event.isCbSelected(CurSelCID))
		setupCurRate(pDlg, 1);
	else if(event.isCmd(cmCurAmtGrpSetupCurrencyCombo)) {
		PPIDArray * p_cur_list = (PPIDArray *)event.message.infoPtr;
		setupCurrencyCombo(pDlg, p_cur_list);
	}
	else if(SelCRateCmd && event.isCmd(SelCRateCmd)) {
		double rate = 0.0;
		PPID   cur_id = pDlg->getCtrlLong(CurSelCID);
		if(cur_id && SelectCurRate(cur_id, LConfig.BaseRateTypeID, &rate) > 0) {
			pDlg->setCtrlData(CRateCID, &rate);
			setupCurRate(pDlg, 0);
		}
	}
	else if(TVBROADCAST && TVCMD == cmChangedFocus && TVINFOVIEW) {
		if(event.isCtlEvent(AmtCID) || event.isCtlEvent(CRateCID))
			setupCurRate(pDlg, 0);
		else if(event.isCtlEvent(DateCID))
			setupCurRate(pDlg, 1);
		return;
	}
	else if(event.isKeyDown(kbF2) && pDlg->isCurrCtlID(CRateCID)) {
		double rate = 0.0;
		PPID   cur_id = pDlg->getCtrlLong(CurSelCID);
		if(SelectCurRate(cur_id, LConfig.BaseRateTypeID, &rate) > 0) {
			pDlg->setCtrlData(CRateCID, &rate);
			setupCurRate(pDlg, 0);
		}
	}
	else
		return;
	pDlg->clearEvent(event);
}