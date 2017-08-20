// V_CRATE.CPP
// Copyright (c) A.Sobolev 2000, 2003, 2004, 2005, 2006, 2007, 2008, 2015, 2016, 2017
//
#include <pp.h>
#pragma hdrstop
//
//
//
IMPLEMENT_PPFILT_FACTORY(CurRate); SLAPI CurRateFilt::CurRateFilt() : PPBaseFilt(PPFILT_CURRATE, 0, 0)
{
	SetFlatChunk(offsetof(CurRateFilt, ReserveStart),
		offsetof(CurRateFilt, Reserve)-offsetof(CurRateFilt, ReserveStart)+sizeof(Reserve));
	Init(1, 0);
}
//
//
//
SLAPI PPViewCurRate::PPViewCurRate() : PPView(0, &Filt)
{
}

PPBaseFilt * SLAPI PPViewCurRate::CreateFilt(void * extraPtr) const
{
	PPBaseFilt * p_base_filt = 0;
	if(PPView::CreateFiltInstance(PPFILT_CURRATE, &p_base_filt)) {
		const PPConfig & r_cfg = LConfig;
		((CurRateFilt *)p_base_filt)->CurID = (long)extraPtr;
		((CurRateFilt *)p_base_filt)->Period.upp = r_cfg.OperDate;
		((CurRateFilt *)p_base_filt)->BaseCurID  = r_cfg.BaseCurID;
		((CurRateFilt *)p_base_filt)->RateTypeID = r_cfg.BaseRateTypeID;
	}
	return p_base_filt;
}

int SLAPI PPViewCurRate::EditBaseFilt(PPBaseFilt * pBaseFilt)
{
	int    ok = -1, valid_data = 0;
	CurRateFilt * p_filt = 0;
	TDialog * dlg = 0;
	THROW(Filt.IsA(pBaseFilt));
	p_filt = (CurRateFilt *)pBaseFilt;
	THROW(CheckDialogPtr(&(dlg = new TDialog(DLG_CRATEFLT))));
	dlg->SetupCalPeriod(CTLCAL_CRATEFLT_PERIOD, CTL_CRATEFLT_PERIOD);
	SetPeriodInput(dlg, CTL_CRATEFLT_PERIOD, &p_filt->Period);
	SetupPPObjCombo(dlg, CTLSEL_CRATEFLT_CUR, PPOBJ_CURRENCY, p_filt->CurID, 0, 0);
	SetupPPObjCombo(dlg, CTLSEL_CRATEFLT_RELCUR, PPOBJ_CURRENCY, p_filt->BaseCurID, 0, 0);
	SetupPPObjCombo(dlg, CTLSEL_CRATEFLT_RATETYPE, PPOBJ_CURRATETYPE, p_filt->RateTypeID, 0, 0);
	while(!valid_data && ExecView(dlg) == cmOK) {
		GetPeriodInput(dlg, CTL_CRATEFLT_PERIOD, &p_filt->Period);
		dlg->getCtrlData(CTLSEL_CRATEFLT_CUR, &p_filt->CurID);
		dlg->getCtrlData(CTLSEL_CRATEFLT_RELCUR, &p_filt->BaseCurID);
		dlg->getCtrlData(CTLSEL_CRATEFLT_RATETYPE, &p_filt->RateTypeID);
		ok = valid_data = 1;
	}
	CATCHZOKPPERR
	delete dlg;
	return ok;
}

int SLAPI PPViewCurRate::Init_(const PPBaseFilt * pBaseFilt)
{
	int    ok = 1;
	if(Helper_InitBaseFilt(pBaseFilt)) {
		BExtQuery::ZDelete(&P_IterQuery);
	}
	else
		ok = 0;
	return ok;
}

int SLAPI PPViewCurRate::InitIteration()
{
	int    idx = 0;
	DBQ  * dbq = 0;
	union {
		CurrencyRateTbl::Key0 k0;
		CurrencyRateTbl::Key1 k1;
	} k, k_;
	BExtQuery::ZDelete(&P_IterQuery);
	if(Filt.RateTypeID) {
		idx = 0;
		k.k0.RateTypeID = Filt.RateTypeID;
		k.k0.CurID = Filt.CurID;
		k.k0.RelCurID = Filt.BaseCurID;
		k.k0.Dt = Filt.Period.low;
	}
	else {
		idx = 1;
		k.k1.CurID = Filt.CurID;
		k.k1.RateTypeID = Filt.RateTypeID;
		k.k1.RelCurID = Filt.BaseCurID;
		k.k1.Dt = Filt.Period.low;
	}
	P_IterQuery = new BExtQuery(&Tbl, idx);
	dbq = ppcheckfiltid(dbq, Tbl.RateTypeID, Filt.RateTypeID);
	dbq = ppcheckfiltid(dbq, Tbl.CurID, Filt.CurID);
	dbq = ppcheckfiltid(dbq, Tbl.RelCurID, Filt.BaseCurID);
	dbq = & (*dbq && daterange(Tbl.Dt, &Filt.Period));
	P_IterQuery->where(*dbq);
	k_ = k;
	Counter.Init(P_IterQuery->countIterations(0, &k_, spGe));
	P_IterQuery->initIteration(0, &k, spGe);
	return 1;
}

int FASTCALL PPViewCurRate::NextIteration(CurRateViewItem * pItem)
{
	if(P_IterQuery) {
		if(P_IterQuery->nextIteration() > 0) {
			ASSIGN_PTR(pItem, Tbl.data);
			return 1;
		}
		return -1;
	}
	return PPSetError(PPERR_ITERNOTINITED);
}

int SLAPI PPViewCurRate::GetRate(const CurRateIdent * pIdent, double * pRate)
{
	CurrencyRateTbl::Rec rec;
	if(Tbl.SearchRate(pIdent, &rec) > 0) {
		ASSIGN_PTR(pRate, rec.Rate);
		return 1;
	}
	return -1;
}

int SLAPI PPViewCurRate::AddItem(CurRateIdent * pIdent)
{
	int    ok = -1;
	CurrencyRateTbl::Rec rec;
	MEMSZERO(rec);
	rec.Dt = LConfig.OperDate;
	rec.CurID = Filt.CurID;
	rec.RelCurID = Filt.BaseCurID;
	rec.RateTypeID = Filt.RateTypeID;
	if(EditRecord(&rec, 1) > 0) {
		pIdent->CurID = rec.CurID;
		pIdent->BaseCurID = rec.RelCurID;
		pIdent->RateTypeID = rec.RateTypeID;
		pIdent->Dt = rec.Dt;
		ok = BIN(Tbl.UpdateRate(pIdent, rec.Rate, 1));
	}
	return ok;
}

int SLAPI PPViewCurRate::EditItem(const CurRateIdent * pIdent)
{
	CurrencyRateTbl::Rec rec;
	return (Tbl.SearchRate(pIdent, &rec) > 0 && EditRecord(&rec, 0) > 0) ? BIN(Tbl.UpdateRate(pIdent, rec.Rate, 1)) : -1;
}

int SLAPI PPViewCurRate::DeleteItem(const CurRateIdent * pIdent)
{
	return CONFIRM(PPCFM_DELETE) ? (Tbl.RemoveRate(pIdent, 1) ? 1 : PPErrorZ()) : -1;
}

int SLAPI PPViewCurRate::EditRecord(CurrencyRateTbl::Rec * pRec, int isNew)
{
	int    ok = -1, valid_data = 0;
	TDialog * dlg = new TDialog(DLG_CURRATE);
	if(CheckDialogPtrErr(&dlg)) {
		uint   sel = 0;
		dlg->SetupCalDate(CTLCAL_CURRATE_DATE, CTL_CURRATE_DATE);
		dlg->setCtrlData(CTL_CURRATE_DATE, &pRec->Dt);
		SetupPPObjCombo(dlg, CTLSEL_CURRATE_CUR, PPOBJ_CURRENCY, pRec->CurID, OLW_CANINSERT, 0);
		SetupPPObjCombo(dlg, CTLSEL_CURRATE_RELCUR, PPOBJ_CURRENCY, pRec->RelCurID, OLW_CANINSERT, 0);
		SetupPPObjCombo(dlg, CTLSEL_CURRATE_RATETYPE, PPOBJ_CURRATETYPE, pRec->RateTypeID, OLW_CANINSERT, 0);
		dlg->setCtrlData(CTL_CURRATE_RATE, &pRec->Rate);
		if(!isNew) {
			dlg->disableCtrls(1, CTL_CURRATE_DATE, CTLCAL_CURRATE_DATE, CTLSEL_CURRATE_CUR,
				CTLSEL_CURRATE_RELCUR, CTLSEL_CURRATE_RATETYPE, 0);
			sel = CTL_CURRATE_RATE;
		}
		else if(!pRec->CurID)
			sel = CTL_CURRATE_CUR;
		else if(!pRec->RelCurID)
			sel = CTL_CURRATE_RELCUR;
		else if(!pRec->RateTypeID)
			sel = CTL_CURRATE_RATETYPE;
		else if(!pRec->Dt)
			sel = CTL_CURRATE_DATE;
		else
			sel = CTL_CURRATE_RATE;
		dlg->selectCtrl(sel);
		while(!valid_data && ExecView(dlg) == cmOK) {
			dlg->getCtrlData(CTL_CURRATE_DATE, &pRec->Dt);
			if(!checkdate(pRec->Dt, 0))
				PPErrorByDialog(dlg, CTL_CURRATE_DATE, PPERR_SLIB);
			else {
				dlg->getCtrlData(CTLSEL_CURRATE_CUR,      &pRec->CurID);
				dlg->getCtrlData(CTLSEL_CURRATE_RELCUR,   &pRec->RelCurID);
				dlg->getCtrlData(CTLSEL_CURRATE_RATETYPE, &pRec->RateTypeID);
				dlg->getCtrlData(CTL_CURRATE_RATE,        &pRec->Rate);
				if(pRec->RelCurID == pRec->CurID)
					PPErrorByDialog(dlg, CTL_CURRATE_RELCUR, PPERR_CURRATEBASE);
				else
					ok = valid_data = 1;
			}
		}
	}
	else
		ok = 0;
	delete dlg;
	return ok;
}

DBQuery * SLAPI PPViewCurRate::CreateBrowserQuery(uint * pBrwId, SString * pSubTitle)
{
	uint   brw_id = BROWSER_CURRATE;
	CurrencyRateTbl * crt = 0;
	ReferenceTbl * rtt = 0;
	DBQuery * q = 0;
	DBQ  * dbq = 0;
	DBE    dbe_cur;
	DBE    dbe_basecur;
	crt = new CurrencyRateTbl;
	rtt = new ReferenceTbl;
	PPDbqFuncPool::InitObjNameFunc(dbe_cur, PPDbqFuncPool::IdObjSymbCurrency, crt->CurID);
	PPDbqFuncPool::InitObjNameFunc(dbe_basecur, PPDbqFuncPool::IdObjSymbCurrency, crt->RelCurID);
	q = & select(
		crt->RateTypeID,  // #0
		crt->CurID,       // #1
		crt->RelCurID,    // #2
		crt->Dt,          // #3
		crt->Rate,        // #4
		dbe_cur,          // #5
		dbe_basecur,      // #6
		rtt->ObjName,     // #7
		0);
	dbq = & daterange(crt->Dt, &Filt.Period);
	dbq = ppcheckfiltid(dbq, crt->CurID, Filt.CurID);
	dbq = ppcheckfiltid(dbq, crt->RelCurID, Filt.BaseCurID);
	dbq = ppcheckfiltid(dbq, crt->RateTypeID, Filt.RateTypeID);
	dbq = & (*dbq && rtt->ObjType == PPOBJ_CURRATETYPE && rtt->ObjID == crt->RateTypeID);
	q->from(crt, rtt, 0L).where(*dbq);
	ASSIGN_PTR(pBrwId, brw_id);
	return q;
}

int SLAPI PPViewCurRate::ProcessCommand(uint ppvCmd, const void * pHdr, PPViewBrowser * pBrw)
{
	int    ok = PPView::ProcessCommand(ppvCmd, pHdr, pBrw);
	if(ok == -2) {
		CurRateIdent ident;
		MEMSZERO(ident);
		struct _H {
			PPID   RateTypeID;
			PPID   CurID;
			PPID   BaseCurID;
			LDATE  Dt;
		} * p_h = (_H *)pHdr;
		if(p_h) {
			ident.CurID      = p_h->CurID;
			ident.BaseCurID  = p_h->BaseCurID;
			ident.RateTypeID = p_h->RateTypeID;
			ident.Dt         = p_h->Dt;
		}
		switch(ppvCmd) {
			case PPVCMD_ADDITEM:
				ok = AddItem(&ident);
				break;
			case PPVCMD_EDITITEM:
				ok = pHdr ? EditItem(&ident) : -1;
				break;
			case PPVCMD_DELETEITEM:
				ok = pHdr ? DeleteItem(&ident) : -1;
				break;
		}
	}
	return ok;
}

int SLAPI PPViewCurRate::Print(const void *)
{
	uint   rpt_id = 0;
	if(Filt.BaseCurID && Filt.CurID && Filt.RateTypeID)
		rpt_id = REPORT_CURRATEBCT;
	else if(Filt.BaseCurID && Filt.RateTypeID)
		rpt_id = REPORT_CURRATEBT;
	else
		rpt_id = REPORT_CURRATE;
	return Helper_Print(rpt_id);
}
//
//
//
class SelCRateDialog : public TDialog {
public:
	SelCRateDialog(CurRateFilt * pFilt, int enableCurID, int enableRateType) : TDialog(DLG_SELCRATE)
	{
		Filt = *pFilt;
		EnableCurID    = enableCurID;
		EnableRateType = enableRateType;
		MEMSZERO(SelIdent);
		SelRate = 0;
		P_List = (SmartListBox*)getCtrlView(CTL_SELCRATE_LIST);
		if(!SetupStrListBox(P_List))
			PPError();
		::SetupCurrencyCombo(this, CTLSEL_SELCRATE_CUR, Filt.CurID, 0, 1, 0);
		SetupPPObjCombo(this, CTLSEL_SELCRATE_RATE, PPOBJ_CURRATETYPE, Filt.RateTypeID, 0, 0);
		disableCtrl(CTLSEL_SELCRATE_CUR, !EnableCurID);
		disableCtrl(CTLSEL_SELCRATE_RATE, !EnableRateType);
		updateList();
	}
	int    getSelectedRate(CurRateIdent *, double * pRate);
private:
	DECL_HANDLE_EVENT;
	void   updateList();
	void   addRate();
	void   editRate();
	void   delRate();
	int    getIdent(CurRateIdent *);

	CurRateFilt    Filt;
	PPViewCurRate  CRV;
	SmartListBox * P_List;

	int EnableCurID;
	int EnableRateType;

	CurRateIdent SelIdent;
	double SelRate;
};

void SelCRateDialog::updateList()
{
	PPID   cur_id = Filt.CurID;
	PPID   rate_type_id = Filt.RateTypeID;
	StringSet ss(SLBColumnDelim);
	CurRateViewItem item;
	P_List->freeAll();
	getCtrlData(CTLSEL_SELCRATE_CUR, &cur_id);
	getCtrlData(CTLSEL_SELCRATE_RATE, &rate_type_id);
	Filt.CurID = cur_id;
	Filt.RateTypeID = rate_type_id;
	CRV.Init_(&Filt);
	for(CRV.InitIteration(); CRV.NextIteration(&item) > 0;) {
		char   sub[64];
		ss.clear(1);
		ss.add(datefmt(&item.Dt, MKSFMT(0, DATF_DMY|DATF_CENTURY), sub));
		ss.add(realfmt(item.Rate, MKSFMTD(0, 4, 0), sub));
		if(!P_List->addItem(item.Dt, ss.getBuf())) {
			PPError();
			return;
		}
	}
	P_List->Draw_();
}

int SelCRateDialog::getIdent(CurRateIdent * pIdent)
{
	if(P_List && P_List->def) {
		long i = 0;
		P_List->getCurID(&i);

		CurRateIdent ident;
		ident.CurID = Filt.CurID;
		ident.BaseCurID = Filt.BaseCurID;
		ident.RateTypeID = Filt.RateTypeID;
		ident.Dt.v = (ulong)i;
		ASSIGN_PTR(pIdent, ident);
		return 1;
	}
	return 0;
}

void SelCRateDialog::addRate()
{
	CurRateIdent ident;
	if(CRV.AddItem(&ident) > 0)
		updateList();
}

void SelCRateDialog::editRate()
{
	CurRateIdent ident;
	if(getIdent(&ident))
		if(CRV.EditItem(&ident) > 0)
			updateList();
}

void SelCRateDialog::delRate()
{
	CurRateIdent ident;
	if(getIdent(&ident))
		if(CRV.DeleteItem(&ident) > 0)
			updateList();
}

int SelCRateDialog::getSelectedRate(CurRateIdent * pIdent, double * pRate)
{
	if(SelIdent.CurID > 0) {
		ASSIGN_PTR(pIdent, SelIdent);
		ASSIGN_PTR(pRate, SelRate);
		return 1;
	}
	return 0;
}

IMPL_HANDLE_EVENT(SelCRateDialog)
{
	if(TVCOMMAND && TVCMD == cmOK && IsInState(sfModal)) {
		if(!getIdent(&SelIdent) || CRV.GetRate(&SelIdent, &SelRate) <= 0)
			SelIdent.CurID = 0;
		endModal(cmOK);
	}
	else {
		TDialog::handleEvent(event);
		if(TVCOMMAND)
			switch(TVCMD) {
				case cmaInsert: addRate();  break;
				case cmaDelete: delRate();  break;
				case cmLBDblClk:
				case cmaEdit:   editRate(); break;
				case cmCBSelected: updateList(); break;
				default:        return;
			}
		else if(event.isKeyDown(KB_CTRLENTER)) {
			if(IsInState(sfModal)) {
				if(!getIdent(&SelIdent) || CRV.GetRate(&SelIdent, &SelRate) <= 0)
					SelIdent.CurID = 0;
				endModal(cmOK);
			}
		}
		else
			return;
	}
	clearEvent(event);
}

int SLAPI SelectCurRate(PPID curID, PPID rateTypeID, double * pRate)
{
	int    ok = -1;
	CurRateFilt filt;
	filt.CurID = curID;
	filt.RateTypeID = rateTypeID;
	filt.BaseCurID = LConfig.BaseCurID;
	ASSIGN_PTR(pRate, 0);
	SelCRateDialog * dlg = new SelCRateDialog(&filt, 0, 0);
	if(CheckDialogPtrErr(&dlg)) {
		if(ExecView(dlg) == cmOK)
			if(dlg->getSelectedRate(0, pRate) > 0)
				ok = 1;
		delete dlg;
	}
	else
		ok = 0;
	return ok;
}
