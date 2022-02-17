// V_QCERT.CPP
// Copyright (c) A.Sobolev 1996, 1997, 1998-2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2015, 2016, 2017, 2018, 2020
// @codepage windows-1251
//
#include <pp.h>
#pragma hdrstop

IMPLEMENT_PPFILT_FACTORY(QCert); QCertFilt::QCertFilt() : PPBaseFilt(PPFILT_QCERT, 0, 0)
{
	SetFlatChunk(offsetof(QCertFilt, ReserveStart),
		offsetof(QCertFilt, Reserve)-offsetof(QCertFilt, ReserveStart)+sizeof(Reserve));
	Init(1, 0);
}

PPViewQCert::PPViewQCert() : PPView(&QcObj, &Filt, 0, 0, 0), P_TempTbl(0), P_RcptT(&BillObj->trfr->Rcpt)
{
}

PPViewQCert::~PPViewQCert()
{
	delete P_TempTbl;
}

int PPViewQCert::EditBaseFilt(PPBaseFilt * pFilt)
{
	int    ok = -1, valid_data = 0;
	QCertFilt f;
	f.Copy(pFilt, 1);
	TDialog * dlg = 0;
	THROW(Filt.IsA(pFilt));
	THROW(CheckDialogPtr(&(dlg = new TDialog(DLG_QCERTFLT))));
	dlg->SetupCalPeriod(CTLCAL_QCERTFLT_EXPIRY,   CTL_QCERTFLT_EXPIRY);
	dlg->SetupCalPeriod(CTLCAL_QCERTFLT_INITDATE, CTL_QCERTFLT_INITDATE);
	SetPeriodInput(dlg, CTL_QCERTFLT_EXPIRY,   &f.ExpiryPeriod);
	SetPeriodInput(dlg, CTL_QCERTFLT_INITDATE, &f.InitPeriod);
	SetupPersonCombo(dlg, CTLSEL_QCERTFLT_ORGAN, f.RegOrgan, 0, PPPRK_BUSADMIN, 0);
	dlg->setCtrlData(CTL_QCERTFLT_CODESTR,  f.CodeStr);
	dlg->setCtrlData(CTL_QCERTFLT_INNERNUM, f.InnerCode);
	dlg->AddClusterAssoc(CTL_QCERTFLT_FLAGS, 0, QCertFilt::fHasRest);
	dlg->AddClusterAssoc(CTL_QCERTFLT_FLAGS, 1, QCertFilt::fShowPassive);
	dlg->SetClusterData(CTL_QCERTFLT_FLAGS, f.Flags);
	while(!valid_data && ExecView(dlg) == cmOK) {
		if(!GetPeriodInput(dlg, CTL_QCERTFLT_EXPIRY, &f.ExpiryPeriod))
			PPErrorByDialog(dlg, CTL_QCERTFLT_EXPIRY);
		else if(!GetPeriodInput(dlg, CTL_QCERTFLT_INITDATE, &f.InitPeriod))
			PPErrorByDialog(dlg, CTL_QCERTFLT_INITDATE);
		else {
			dlg->getCtrlData(CTLSEL_QCERTFLT_ORGAN, &f.RegOrgan);
			dlg->getCtrlData(CTL_QCERTFLT_CODESTR,  f.CodeStr);
			dlg->getCtrlData(CTL_QCERTFLT_INNERNUM, f.InnerCode);
			dlg->GetClusterData(CTL_QCERTFLT_FLAGS, &f.Flags);
			ok = valid_data = 1;
			THROW(pFilt->Copy(&f, 1));
		}
	}
	CATCHZOKPPERR
	delete dlg;
	return ok;
}

PP_CREATE_TEMP_FILE_PROC(CreateTempFile, QualityCert);

int PPViewQCert::Init_(const PPBaseFilt * pBaseFilt)
{
	int    ok = 1;
	THROW(Helper_InitBaseFilt(pBaseFilt));
	ZDELETE(P_TempTbl);
	Counter.Init();
	if(Filt.CodeStr[0] || Filt.InnerCode[0] || (Filt.Flags & QCertFilt::fHasRest)) {
		DBQ  * dbq = 0;
		QualityCertTbl * t = QcObj.P_Tbl;
		QualityCertTbl::Key0 k;
		THROW(P_TempTbl = CreateTempFile());
		{
			long   c = 0;
			BExtInsert bei(P_TempTbl);
			PPTransaction tra(ppDbDependTransaction, 1);
			THROW(tra);
			{
				IterCounter cntr;
				BExtQuery q(t, 0, 16);
				dbq = & (*dbq && daterange(t->Expiry, &Filt.ExpiryPeriod) && daterange(t->InitDate, &Filt.InitPeriod));
				dbq = ppcheckfiltid(dbq, t->RegOrgan, Filt.RegOrgan);
				if(!(Filt.Flags & QCertFilt::fShowPassive))
					dbq = & (*dbq && t->Passive == 0L);
				q.selectAll().where(*dbq);
				MEMSZERO(k);
				cntr.Init(q.countIterations(0, &k, spGe));
				MEMSZERO(k);
				for(q.initIteration(false, &k, spGe); q.nextIteration() > 0;) {
					int    found = 0;
					QualityCertTbl::Rec rec;
					t->copyBufTo(&rec);
					if(Filt.Flags & QCertFilt::fHasRest && P_RcptT) {
						if(P_RcptT->IsThereOpenedLotForQCert(rec.ID) > 0)
							found = 1;
					}
					else
						found = 1;
					if(found && Filt.CodeStr[0] && !ExtStrSrch(rec.Code, Filt.CodeStr, 0))
						found = 0;
					if(found && Filt.InnerCode[0] && stricmp866(strip(rec.InnerCode), Filt.InnerCode) != 0)
						found = 0;
					if(found) {
						THROW_DB(bei.insert(&rec));
						c++;
					}
					PPWaitPercent(cntr.Increment());
				}
			}
			THROW_DB(bei.flash());
			THROW(tra.Commit());
			Counter.Init(c);
		}
	}
	CATCH
		BExtQuery::ZDelete(&P_IterQuery);
		ZDELETE(P_TempTbl);
		ok = 0;
	ENDCATCH
	return ok;
}

int PPViewQCert::InitIteration()
{
	BExtQuery::ZDelete(&P_IterQuery);
	Counter.Init();
	int    ok  = 1;
	DBQ  * dbq = 0;
	QualityCertTbl * t = P_TempTbl ? P_TempTbl : QcObj.P_Tbl;
	QualityCertTbl::Key0 k;
	THROW_MEM(P_IterQuery = new BExtQuery(t, 0, 16));
	P_IterQuery->selectAll();
	if(!P_TempTbl) {
		dbq = & (*dbq && daterange(t->Expiry, &Filt.ExpiryPeriod) && daterange(t->InitDate, &Filt.InitPeriod));
		dbq = ppcheckfiltid(dbq, t->RegOrgan, Filt.RegOrgan);
		if(!(Filt.Flags & QCertFilt::fShowPassive))
			dbq = & (*dbq && t->Passive == 0L);
		P_IterQuery->where(*dbq);
	}
	Counter.Init(P_IterQuery->countIterations(0, MEMSZERO(k), spGe));
	P_IterQuery->initIteration(0, MEMSZERO(k), spGe);
	CATCH
		ok = 0;
		BExtQuery::ZDelete(&P_IterQuery);
	ENDCATCH
	return ok;
}

int FASTCALL PPViewQCert::NextIteration(QCertViewItem * pItem)
{
	if(P_IterQuery) {
		while(P_IterQuery->nextIteration() > 0) {
			int    found = 0;
			if(!P_TempTbl && Filt.Flags & QCertFilt::fHasRest && P_RcptT) {
				if(P_RcptT->IsThereOpenedLotForQCert(QcObj.P_Tbl->data.ID) > 0)
					found = 1;
			}
			else
				found = 1;
			if(found) {
				Counter.Increment();
				if(P_TempTbl)
					P_TempTbl->copyBufTo(pItem);
				else
					QcObj.P_Tbl->copyBufTo(pItem);
				return 1;
			}
		}
	}
	return -1;
}

void PPViewQCert::ViewTotal()
{
	TDialog * dlg = new TDialog(DLG_QCERTTOTAL);
	if(CheckDialogPtrErr(&dlg)) {
		long   count = 0;
		for(InitIteration(); NextIteration(0) > 0; count++);
		dlg->setCtrlLong(CTL_QCERTTOTAL_COUNT, count);
		ExecViewAndDestroy(dlg);
	}
}

int PPViewQCert::Transmit()
{
	int    ok = -1;
	ObjTransmitParam param;
	if(ObjTransmDialog(DLG_OBJTRANSM, &param) > 0) {
		QCertViewItem item;
		const PPIDArray & rary = param.DestDBDivList.Get();
		PPObjIDArray objid_ary;
		PPWaitStart();
		for(InitIteration(); NextIteration(&item) > 0; PPWaitPercent(GetCounter()))
			objid_ary.Add(PPOBJ_QCERT, item.ID);
		THROW(PPObjectTransmit::Transmit(&rary, &objid_ary, &param));
		ok = 1;
	}
	CATCHZOKPPERR
	PPWaitStop();
	return ok;
}

int PPViewQCert::SetPassiveTag(int set)
{
	int    ok = -1, r;
	QCertViewItem item;
	if(CONFIRM(PPCFM_QCSETPASSIVETAG)) {
		PPWaitStart();
		PPTransaction tra(1);
		THROW(tra);
		for(InitIteration(); NextIteration(&item) > 0;) {
			THROW(r = QcObj.SetPassiveTag(item.ID, set, 0));
			if(r > 0)
				ok = 1;
			PPWaitPercent(GetCounter());
		}
		THROW(tra.Commit());
		PPWaitStop();
	}
	CATCHZOKPPERR
	return ok;
}

DBQuery * PPViewQCert::CreateBrowserQuery(uint * pBrwId, SString * pSubTitle)
{
	uint   brw_id = BROWSER_QCERT;
	DBQuery * q = 0;
	DBQ  * dbq = 0;
	DBE    dbe_psn;
	QualityCertTbl * qc = 0;
	ReceiptTbl * rcpt = 0;
	if(P_TempTbl) {
		THROW(CheckTblPtr(qc = new QualityCertTbl(P_TempTbl->GetName())));
		PPDbqFuncPool::InitObjNameFunc(dbe_psn, PPDbqFuncPool::IdObjNamePerson, qc->RegOrgan);
		q = & select(
			qc->ID,         // #00
			qc->Code,       // #01
			qc->BlankCode,  // #02
			qc->InitDate,   // #03
			qc->Expiry,     // #04
			dbe_psn,        // #05
			qc->GoodsName,  // #06
			qc->Etc,        // #07
			0L).from(qc, 0L);
		THROW(CheckQueryPtr(q));
	}
	else {
		THROW(CheckTblPtr(qc = new QualityCertTbl));
		PPDbqFuncPool::InitObjNameFunc(dbe_psn, PPDbqFuncPool::IdObjNamePerson, qc->RegOrgan);
		if(Filt.Flags & QCertFilt::fHasRest)
			THROW(CheckTblPtr(rcpt = new ReceiptTbl));
		q = & select(
			qc->ID,         // #00
			qc->Code,       // #01
			qc->BlankCode,  // #02
			qc->InitDate,   // #03
			qc->Expiry,     // #04
			dbe_psn,        // #05
			qc->GoodsName,  // #06
			qc->Etc,        // #07
			0L).from(qc, rcpt, 0L);
		THROW(CheckQueryPtr(q));
		if(!(Filt.Flags & QCertFilt::fShowPassive))
			dbq = & (*dbq && qc->Passive == 0L);
		dbq = & (*dbq && daterange(qc->Expiry, &Filt.ExpiryPeriod));
		dbq = ppcheckfiltid(dbq, qc->RegOrgan, Filt.RegOrgan);
		dbq = & (*dbq && daterange(qc->InitDate, &Filt.InitPeriod));
		if(Filt.Flags & QCertFilt::fHasRest)
			dbq = & (*dbq && rcpt->QCertID == qc->ID && rcpt->Closed == 0L);
		q->where(*dbq);
	}
	if(!(Filt.Flags & QCertFilt::fShowPassive))
		q->orderBy(qc->Passive, qc->Code, 0L);
	else
		q->orderBy(qc->Code, 0L);
	THROW(CheckQueryPtr(q));
	CATCH
		if(q == 0)
			if(dbq)
				delete dbq;
			else {
				delete qc;
				delete rcpt;
			}
		else
			ZDELETE(q);
	ENDCATCH
	ASSIGN_PTR(pBrwId, brw_id);
	return q;
}

int PPViewQCert::ProcessCommand(uint ppvCmd, const void * pHdr, PPViewBrowser * pBrw)
{
	int    ok = PPView::ProcessCommand(ppvCmd, pHdr, pBrw);
	if(ok == -2) {
		PPID   id = pHdr ? *static_cast<const PPID *>(pHdr) : 0;
		switch(ppvCmd) {
			case PPVCMD_ADDBYSAMPLE:
				{
					PPID   temp_id = 0;
					ok = (id && QcObj.Edit(&temp_id, reinterpret_cast<void *>(id)) == cmOK) ? 1 : -1;
				}
				break;
			case PPVCMD_VIEWLOTS:
				ok = -1;
				if(id) {
					LotFilt lot_flt;
					lot_flt.QCertID = id;
					if(Filt.Flags & QCertFilt::fHasRest)
						lot_flt.ClosedTag = 1;
					::ViewLots(&lot_flt, 0, 0);
				}
				break;
			case PPVCMD_SETPASSIVTAG:
				ok = SetPassiveTag(1);
				break;
			case PPVCMD_TRANSMIT:
				ok = Transmit();
				break;
		}
	}
	return ok;
}
