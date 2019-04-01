// V_SYNC.CPP
// Copyright (c) A.Sobolev 2001-2003, 2006, 2007, 2008, 2009, 2014, 2015, 2016, 2017, 2018, 2019
//
#include <pp.h>
#pragma hdrstop
//
// @ModuleDef(PPViewObjSync)
//
IMPLEMENT_PPFILT_FACTORY(ObjSync); SLAPI ObjSyncFilt::ObjSyncFilt() : PPBaseFilt(PPFILT_OBJSYNC, 0, 0)
{
	SetFlatChunk(offsetof(ObjSyncFilt, ReserveStart), offsetof(ObjSyncFilt, ObjIDList) - offsetof(ObjSyncFilt, ReserveStart));
	Init(1, 0);
}

int SLAPI ObjSyncFilt::Init(int fullyDestroy, long extraData)
{
	PPBaseFilt::Init(fullyDestroy, extraData);
	ObjIDList.freeAll();
	CommIDList.freeAll();
	return 1;
}

int SLAPI ObjSyncFilt::Write(SBuffer & rBuf, long f) const
{
	return (PPBaseFilt::Write(rBuf, f) && rBuf.Write(&ObjIDList, 0) && rBuf.Write(&CommIDList, 0)) ? 1 : 0;
}

int SLAPI ObjSyncFilt::Read(SBuffer & rBuf, long f)
{
	return (PPBaseFilt::Read(rBuf, f) && rBuf.Read(&ObjIDList, 0) && rBuf.Read(&CommIDList, 0)) ? 1 : 0;
}

int SLAPI ObjSyncFilt::Copy(const PPBaseFilt * pS, int deep)
{
	int    ok = 1;
	if(PPBaseFilt::Copy(pS, deep)) {
		ObjIDList  = static_cast<const ObjSyncFilt *>(pS)->ObjIDList;
		CommIDList = static_cast<const ObjSyncFilt *>(pS)->CommIDList;
	}
	else
		ok = 0;
	return ok;
}

int SLAPI ObjSyncFilt::IsEqual(const PPBaseFilt * pS, int deep) const
{
	return BIN(PPBaseFilt::IsEqual(pS, deep) && 
		ObjIDList.IsEqual(&static_cast<const ObjSyncFilt *>(pS)->ObjIDList) && CommIDList.IsEqual(static_cast<const ObjSyncFilt *>(pS)->CommIDList));
}
//
//
//
SLAPI PPViewObjSync::PPViewObjSync() : PPView(0, &Filt, PPVIEW_OBJSYNC), P_Tbl(DS.GetTLA().P_ObjSync), P_TmpTbl(0)
{
}

SLAPI PPViewObjSync::~PPViewObjSync()
{
	delete P_TmpTbl;
}
//
// ObjSyncFiltDialog
//
class ObjSyncFiltDialog : public TDialog {
public:
	ObjSyncFiltDialog() : TDialog(DLG_SYNCFILT)
	{
	}
	int    setDTS(const ObjSyncFilt *);
	int    getDTS(ObjSyncFilt *);
private:
	ObjSyncFilt Data;
};

int ObjSyncFiltDialog::setDTS(const ObjSyncFilt * pFilt)
{
	int    ok = -1;
	if(pFilt) {
		Data = *pFilt;
		SetupObjListCombo(this, CTLSEL_SYNCFILT_OBJ, Data.ObjType);
		SetupPPObjCombo(this, CTLSEL_SYNCFILT_DIV, PPOBJ_DBDIV, Data.DbDivID, 0, 0);
		uint      pos;
		SString   buf;
		StringSet ids_set(',', 0);
		for(pos = 0; pos < Data.ObjIDList.getCount(); pos++)
			ids_set.add(buf.Z().Cat(Data.ObjIDList.at(pos)));
		setCtrlString(CTL_SYNCFILT_OBJIDS, ids_set.getBuf());
		ids_set.clear();
		for(pos = 0; pos < Data.CommIDList.getCount(); pos++) {
			LAssoc  comm_id = Data.CommIDList.at(pos);
			ids_set.add(buf.Z().Cat(comm_id.Key).CatDiv('-', 0).Cat(comm_id.Val));
		}
		setCtrlString(CTL_SYNCFILT_COMMIDS, ids_set.getBuf());
		ok = 1;
	}
	return ok;
}

int ObjSyncFiltDialog::getDTS(ObjSyncFilt * pFilt)
{
	int    ok = 1;
	uint   sel = 0, pos;
	SString   buf;
	StringSet ids_set(',', 0);
	getCtrlData(CTLSEL_SYNCFILT_OBJ, &Data.ObjType);
	getCtrlData(CTLSEL_SYNCFILT_DIV, &Data.DbDivID);
	getCtrlString(sel = CTL_SYNCFILT_OBJIDS, buf);
	if(buf.NotEmptyS()) {
		PPID  id;
		PPIDArray  objid_list;
		ids_set.setBuf(buf.cptr(), buf.Len() + 1);
		for(pos = 0; ids_set.get(&pos, buf);) {
			THROW(buf.Strip().IsDigit() && (id = buf.ToLong()) > 0);
			objid_list.addUnique(id);
		}
		objid_list.sort();
		Data.ObjIDList = objid_list;
	}
	else
		Data.ObjIDList.freeAll();
	getCtrlString(sel = CTL_SYNCFILT_COMMIDS, buf);
	if(buf.NotEmptyS()) {
		LAssoc  comm_id;
		LAssocArray  commid_list;
		SString left, right;
		ids_set.setBuf(buf.cptr(), buf.Len() + 1);
		for(pos = 0; ids_set.get(&pos, buf);) {
			THROW(buf.Strip().Divide('-', left, right) > 0 && left.Strip().IsDigit() && (comm_id.Key = left.ToLong()) > 0 &&
				right.Strip().IsDigit() && (comm_id.Val = right.ToLong()) > 0);
			if(!commid_list.bsearch(&comm_id, 0, PTR_CMPFUNC(_2long)))
				commid_list.ordInsert(&comm_id, 0, PTR_CMPFUNC(_2long));
		}
		Data.CommIDList = commid_list;
	}
	else
		Data.CommIDList.freeAll();
	CATCH
		ok = PPErrorByDialog(this, sel, PPERR_USERINPUT);
	ENDCATCH
	ASSIGN_PTR(pFilt, Data);
	return ok;
}
//
//
//
int SLAPI PPViewObjSync::EditBaseFilt(PPBaseFilt * pFilt)
{
	if(!Filt.IsA(pFilt))
		return PPErrorZ();
	else {
		DIALOG_PROC_BODY(ObjSyncFiltDialog, static_cast<ObjSyncFilt *>(pFilt));
	}
}

PP_CREATE_TEMP_FILE_PROC(CreateTempFile, ObjSync);

int SLAPI PPViewObjSync::Init_(const PPBaseFilt * pFilt)
{
	int    ok = 1;
	BExtQuery::ZDelete(&P_IterQuery);
	ZDELETE(P_TmpTbl);
	THROW(Helper_InitBaseFilt(pFilt));
	if(Filt.ObjIDList.getCount() > 0 || Filt.CommIDList.getCount() > 0) {
		PPWait(1);
		THROW(P_TmpTbl = CreateTempFile());
		{
			IterCounter cntr;
			ObjSyncTbl::Key1  k1, k1_;
			BExtInsert bei(P_TmpTbl);
			PPTransaction tra(ppDbDependTransaction, 1);
			THROW(tra);
			if(Filt.ObjType) {
				if(Filt.ObjIDList.getCount()) {
					for(uint i = 0; i < Filt.ObjIDList.getCount(); i++) {
						const PPID obj_id = Filt.ObjIDList.get(i);
						MEMSZERO(k1);
						k1.ObjType = (int16)Filt.ObjType;
						k1.ObjID = obj_id;
						if(P_Tbl->search(1, &k1, spGe) && P_Tbl->data.ObjType == Filt.ObjType && P_Tbl->data.ObjID == obj_id) do {
							ObjSyncTbl::Rec  sync_rec;
							P_Tbl->copyBufTo(&sync_rec);
							if(!Filt.DbDivID || sync_rec.DBID == Filt.DbDivID) {
								if(!Filt.CommIDList.getCount() || Filt.CommIDList.SearchPair(sync_rec.CommIdPfx, sync_rec.CommID, 0)) {
									THROW_DB(bei.insert(&sync_rec));
								}
							}
						} while(P_Tbl->search(1, &k1, spNext) && P_Tbl->data.ObjType == Filt.ObjType && P_Tbl->data.ObjID == obj_id);
					}
				}
				else if(Filt.CommIDList.getCount()) {
					for(uint i = 0; i < Filt.CommIDList.getCount(); i++) {
						const LAssoc & r_comm_id = Filt.CommIDList.at(i);
						ObjSyncTbl::Key0 k0;
						MEMSZERO(k0);
						k0.ObjType = (int16)Filt.ObjType;
						k0.CommIdPfx = (int16)r_comm_id.Key;
						k0.CommID = r_comm_id.Val;
						if(P_Tbl->search(0, &k0, spGe) && P_Tbl->data.ObjType == Filt.ObjType && P_Tbl->data.CommIdPfx == r_comm_id.Key && P_Tbl->data.CommID == r_comm_id.Val) do {
							ObjSyncTbl::Rec  sync_rec;
							P_Tbl->copyBufTo(&sync_rec);
							if(!Filt.DbDivID || sync_rec.DBID == Filt.DbDivID) {
								THROW_DB(bei.insert(&sync_rec));
							}
						} while(P_Tbl->search(0, &k0, spNext) && P_Tbl->data.ObjType == Filt.ObjType && P_Tbl->data.CommIdPfx == r_comm_id.Key && P_Tbl->data.CommID == r_comm_id.Val);
					}
				}
			}
			else {
				DBQ  * dbq = 0;
				BExtQuery q(P_Tbl, 1);
				dbq = ppcheckfiltid(dbq, P_Tbl->ObjType, Filt.ObjType);
				dbq = ppcheckfiltid(dbq, P_Tbl->DBID, Filt.DbDivID);
				q.selectAll().where(*dbq);
				MEMSZERO(k1);
				k1.ObjType = (int16)Filt.ObjType;
				k1_ = k1;
				cntr.Init(q.countIterations(0, &k1_, spGe));
				for(q.initIteration(0, &k1, spGe); q.nextIteration() > 0;) {
					int  is_suitable = 1;
					ObjSyncTbl::Rec  sync_rec;
					P_Tbl->copyBufTo(&sync_rec);
					if(Filt.ObjIDList.getCount() > 0)
						is_suitable = Filt.ObjIDList.bsearch(sync_rec.ObjID);
					if(is_suitable && Filt.CommIDList.getCount() > 0)
						is_suitable = Filt.CommIDList.SearchPair(sync_rec.CommIdPfx, sync_rec.CommID, 0);
					if(is_suitable) {
						THROW_DB(bei.insert(&sync_rec));
					}
					PPWaitPercent(cntr.Increment());
				}
			}
			THROW_DB(bei.flash());
			THROW(tra.Commit());
		}
		PPWait(0);
	}
	CATCH
		ZDELETE(P_TmpTbl);
		ok = 0;
	ENDCATCH
	return ok;
}

int SLAPI PPViewObjSync::InitIteration()
{
	int    ok = 1;
	DBQ  * dbq = 0;
	ObjSyncTbl * p_sync_tbl = P_TmpTbl ? P_TmpTbl : P_Tbl;
	ObjSyncTbl::Key0 k;
	MEMSZERO(k);
	BExtQuery::ZDelete(&P_IterQuery);
	THROW_MEM(P_IterQuery = new BExtQuery(p_sync_tbl, 0));
	P_IterQuery->selectAll();
	if(Filt.ObjType) {
		dbq = & (*dbq && p_sync_tbl->ObjType == Filt.ObjType);
		k.ObjType = (short)Filt.ObjType;
	}
	if(Filt.DbDivID)
		dbq = & (*dbq && p_sync_tbl->DBID == Filt.DbDivID);
	P_IterQuery->where(*dbq);
	P_IterQuery->initIteration(0, &k, spGe);
	CATCHZOK
	return ok;
}

int FASTCALL PPViewObjSync::NextIteration(ObjSyncViewItem * pItem)
{
	ObjSyncTbl * p_sync_tbl = P_TmpTbl ? P_TmpTbl : P_Tbl;
	if(P_IterQuery && P_IterQuery->nextIteration() > 0) {
		p_sync_tbl->copyBufTo(pItem);
		return 1;
	}
	else
		return -1;
}

int SLAPI PPViewObjSync::EditRecord(ObjSyncTbl::Rec * pRec)
{
	int    ok = -1;
	TDialog * dlg = new TDialog(DLG_OBJSYNC);
	if(CheckDialogPtrErr(&dlg)) {
		PPCommSyncID comm_id;
		SString comm_id_buf;
		comm_id = *pRec;
		SetupObjListCombo(dlg, CTLSEL_OBJSYNC_OBJ, pRec->ObjType);
		SetupPPObjCombo(dlg, CTLSEL_OBJSYNC_DIV, PPOBJ_DBDIV, pRec->DBID, 0, 0);
		dlg->setCtrlData(CTL_OBJSYNC_ID,     &pRec->ObjID);
		dlg->setCtrlString(CTL_OBJSYNC_COMMID, comm_id.ToStr(0, comm_id_buf));
		dlg->SetupCalDate(CTLCAL_OBJSYNC_DT, CTL_OBJSYNC_DT);
		dlg->setCtrlData(CTL_OBJSYNC_DT,     &pRec->Dt);
		dlg->setCtrlData(CTL_OBJSYNC_TM,     &pRec->Tm);
		//dlg->disableCtrls(1, CTLSEL_OBJSYNC_OBJ, CTLSEL_OBJSYNC_DIV, 0);
		for(int valid_data = 0; !valid_data && ExecView(dlg) == cmOK;) {
			dlg->getCtrlData(CTLSEL_OBJSYNC_OBJ, &pRec->ObjType);
			dlg->getCtrlData(CTLSEL_OBJSYNC_DIV, &pRec->DBID);
			dlg->getCtrlData(CTL_OBJSYNC_ID,     &pRec->ObjID);
			dlg->getCtrlString(CTL_OBJSYNC_COMMID, comm_id_buf);
			comm_id.FromStr(comm_id_buf);
			comm_id.Get(pRec);
			dlg->getCtrlData(CTL_OBJSYNC_DT,     &pRec->Dt);
			dlg->getCtrlData(CTL_OBJSYNC_TM,     &pRec->Tm);
			ok = valid_data = 1;
		}
	}
	else
		ok = 0;
	delete dlg;
	return ok;
}

int SLAPI PPViewObjSync::AddItem()
{
	int    ok = -1;
	ObjSyncTbl::Rec rec;
	MEMSZERO(rec);
	while(ok <= 0 && EditRecord(&rec) > 0) {
		if(P_Tbl->AddRawRecord(&rec, 1))
			ok = 1;
		else
			ok = (PPError(PPERR_DBENGINE, 0), 0);
	}
	return ok;
}

int SLAPI PPViewObjSync::EditItem(const ObjSyncIdent * pIdent)
{
	int    ok = -1;
	ObjSyncTbl::Rec rec, temp_rec;
	if(P_Tbl->Search(pIdent, &rec) > 0) {
		while(ok <= 0 && EditRecord(&rec) > 0) {
			if(P_Tbl->Search(pIdent, &temp_rec) > 0) {
				temp_rec.ObjID = rec.ObjID;
				temp_rec.CommIdPfx = rec.CommIdPfx;
				temp_rec.CommID = rec.CommID;
				temp_rec.Dt = rec.Dt;
				temp_rec.Tm = rec.Tm;
				ok = P_Tbl->Update(pIdent, &temp_rec, 1) ? 1 : PPErrorZ();
			}
		}
	}
	return ok;
}

DBQuery * SLAPI PPViewObjSync::CreateBrowserQuery(uint * pBrwId, SString * pSubTitle)
{
	DBQuery * q = 0;
	uint   brw_id = BROWSER_OBJSYNC;
	DBQ  * dbq = 0;
	DBE    dbe_commid;
	DBE    dbe_objtitle;
	ObjSyncTbl * t = P_TmpTbl ? new ObjSyncTbl(P_TmpTbl->GetName()) : new ObjSyncTbl;
	{
		dbe_commid.init();
		dbe_commid.push(t->CommIdPfx);
		dbe_commid.push(t->CommID);
		dbe_commid.push((DBFunc)PPDbqFuncPool::IdCommSyncId);
	}
	PPDbqFuncPool::InitLongFunc(dbe_objtitle, PPDbqFuncPool::IdObjTitle, t->ObjType);
	q = & select(
		t->ObjType,    // #0
		t->ObjID,      // #1
		t->CommIdPfx,  // #2
		t->CommID,     // #3
		t->DBID,       // #4
		t->Dt,         // #5
		t->Tm,         // #6
		dbe_commid,    // #7
		dbe_objtitle,  // #8
		t->Flags,      // #9
		0L).from(t, 0L);
	dbq = ppcheckfiltid(dbq, t->ObjType, Filt.ObjType);
	dbq = ppcheckfiltid(dbq, t->DBID, Filt.DbDivID);
	q->where(*dbq).orderBy(t->ObjType, t->CommIdPfx, t->CommID, 0L);
	if(pSubTitle) {
		*pSubTitle = 0;
		SString temp_buf;
		if(Filt.ObjType) {
			pSubTitle->CatDivIfNotEmpty('-', 1).Cat(GetObjectTitle(Filt.ObjType, temp_buf));
		}
		if(Filt.DbDivID) {
			GetObjectName(PPOBJ_DBDIV, Filt.DbDivID, temp_buf);
			pSubTitle->CatDivIfNotEmpty('-', 1).Cat(temp_buf);
		}
	}
	ASSIGN_PTR(pBrwId, brw_id);
	return q;
}

int SLAPI PPViewObjSync::ProcessCommand(uint ppvCmd, const void * pHdr, PPViewBrowser * pBrw)
{
	int    ok = PPView::ProcessCommand(ppvCmd, pHdr, pBrw);
	if(ok == -2) {
		const struct _ident {
			int16 ObjType;
			PPID  ObjID;
			int16 CommIdxPfx;
			PPID  CommID;
			int16 DBID;
		} * p_ident = pHdr ? static_cast<const _ident *>(pHdr) : 0;
		ObjSyncIdent ident;
		if(p_ident) {
			ident.fComm = 0;
			ident.ObjType = p_ident->ObjType;
			ident.ObjID   = p_ident->ObjID;
			ident.DBID   = p_ident->DBID;
		}
		else
			MEMSZERO(ident);
		switch(ppvCmd) {
			case PPVCMD_EDITITEM:
				ok = -1;
				if(p_ident && EditItem(&ident) > 0)
					ok = 1;
				break;
			case PPVCMD_ADDITEM:
				ok = -1;
				if(AddItem() > 0)
					ok = 1;
				break;
			case PPVCMD_DELETEITEM:
				ok = -1;
				if(p_ident) {
					if(CONFIRM(PPCFM_DELETE) && P_Tbl->Search(&ident, 0) > 0)
						ok = P_Tbl->Remove_S(&ident, 1) ? 1 : PPErrorZ();
				}
				break;
			case PPVCMD_EDITOBJ:
				ok = -1;
				if(p_ident && ident.DBID == LConfig.DBDiv)
					ok = EditPPObj(ident.ObjType, ident.ObjID);
				break;
		}
	}
	return ok;
}
//
// @ModuleDef(PPViewObjSyncCmp)
//
IMPLEMENT_PPFILT_FACTORY(ObjSyncCmp); SLAPI ObjSyncCmpFilt::ObjSyncCmpFilt() : PPBaseFilt(PPFILT_OBJSYNCCMP, 0, 0)
{
	SetFlatChunk(offsetof(ObjSyncCmpFilt, ReserveStart),
		offsetof(ObjSyncCmpFilt, Reserve)-offsetof(ObjSyncCmpFilt, ReserveStart)+sizeof(Reserve));
	Init(1, 0);
}
//
//
//
SLAPI PPViewObjSyncCmp::PPViewObjSyncCmp() : PPView(0, &Filt, PPVIEW_OBJSYNCCMP), P_TmpTbl(0), P_Ot(0)
{
	DefReportId = REPORT_OBJSYNCCMP;
}

SLAPI PPViewObjSyncCmp::~PPViewObjSyncCmp()
{
	delete P_TmpTbl;
	delete P_Ot;
}

PPBaseFilt * SLAPI PPViewObjSyncCmp::CreateFilt(void * extraPtr) const
{
	ObjSyncCmpFilt * p_filt = new ObjSyncCmpFilt;
	if((reinterpret_cast<long>(extraPtr)) == 1)
		p_filt->Flags |= ObjSyncCmpFilt::fInitialize;
	return p_filt;
}

int SLAPI PPViewObjSyncCmp::EditBaseFilt(PPBaseFilt * pFilt)
{
	int    ok = -1;
	ushort v = 0;
	ObjSyncCmpFilt * p_filt = 0;
	TDialog * dlg = 0;
	THROW(Filt.IsA(pFilt));
	p_filt = (ObjSyncCmpFilt *)pFilt;
	THROW(CheckDialogPtr(&(dlg = new TDialog(DLG_SYNCCMPFLT))));
	SetupObjListCombo(dlg, CTLSEL_SYNCCMPFLT_OBJ, p_filt->ObjType);
	switch(p_filt->ErrCode) {
		case 0: v = BIN(p_filt->Flags & ObjSyncCmpFilt::fErrOnly); break;
		case 1: v = 2; break;
		case 2: v = 3; break;
		case 3: v = 4; break;
		case 4: v = 5; break;
		default: v = 0; break;
	}
	dlg->setCtrlData(CTL_SYNCCMPFLT_ERR, &v);
	dlg->AddClusterAssocDef(CTL_SYNCCMPFLT_ORDER, 0, ObjSyncCmpFilt::ordByError);
	dlg->AddClusterAssoc(CTL_SYNCCMPFLT_ORDER, 1, ObjSyncCmpFilt::ordByCommID);
	dlg->AddClusterAssoc(CTL_SYNCCMPFLT_ORDER, 2, ObjSyncCmpFilt::ordBySrcID);
	dlg->AddClusterAssoc(CTL_SYNCCMPFLT_ORDER, 3, ObjSyncCmpFilt::ordBySrcName);
	dlg->AddClusterAssoc(CTL_SYNCCMPFLT_ORDER, 4, ObjSyncCmpFilt::ordByDestID);
	dlg->AddClusterAssoc(CTL_SYNCCMPFLT_ORDER, 5, ObjSyncCmpFilt::ordByDestName);
	dlg->SetClusterData(CTL_SYNCCMPFLT_ORDER, p_filt->Order);
	SetRealRangeInput(dlg, CTL_SYNCCMPFLT_SCORE, &p_filt->Score);
	if(ExecView(dlg) == cmOK) {
		dlg->getCtrlData(CTLSEL_SYNCCMPFLT_OBJ, &p_filt->ObjType);
		dlg->getCtrlData(CTL_SYNCCMPFLT_ERR, &(v = 0));
		p_filt->Flags &= ~ObjSyncCmpFilt::fErrOnly;
		switch(v) {
			case 0: p_filt->ErrCode = 0; break;
			case 1: p_filt->ErrCode = 0; p_filt->Flags |= ObjSyncCmpFilt::fErrOnly; break;
			case 2: p_filt->ErrCode = 1; break;
			case 3: p_filt->ErrCode = 2; break;
			case 4: p_filt->ErrCode = 3; break;
			case 5: p_filt->ErrCode = 4; break;
			default: p_filt->ErrCode = 0; break;
		}
		dlg->GetClusterData(CTL_SYNCCMPFLT_ORDER, &p_filt->Order);
		GetRealRangeInput(dlg, CTL_SYNCCMPFLT_SCORE, &p_filt->Score);
		ok = 1;
	}
	CATCHZOKPPERR
	delete dlg;
	return ok;
}

PP_CREATE_TEMP_FILE_PROC(CreateTempSyncCmpFile, TempSyncCmp);

int SLAPI PPViewObjSyncCmp::Init_(const PPBaseFilt * pFilt)
{
	int    ok = 1;
	ObjReceiveParam param;
	BExtQuery::ZDelete(&P_IterQuery);
	THROW(Helper_InitBaseFilt(pFilt));
	if(Filt.Flags & ObjSyncCmpFilt::fInitialize) {
		ZDELETE(P_TmpTbl);
		THROW(P_TmpTbl = CreateTempSyncCmpFile());
		param.Flags |= ObjReceiveParam::fSyncCmp;
		param.P_SyncCmpTbl = P_TmpTbl;
		THROW(PPObjectTransmit::ReceivePackets(&param));
	}
	CATCHZOK
	Filt.Flags &= ~ObjSyncCmpFilt::fInitialize;
	return ok;
}

int SLAPI PPViewObjSyncCmp::InitIteration()
{
	if(P_TmpTbl) {
		DBQ * dbq = 0;
		union {
			TempSyncCmpTbl::Key0 k0;
			TempSyncCmpTbl::Key1 k1;
			TempSyncCmpTbl::Key2 k2;
			TempSyncCmpTbl::Key3 k3;
			TempSyncCmpTbl::Key4 k4;
			TempSyncCmpTbl::Key5 k5;
		} k;
		int    idx = 0;
		if(Filt.Order == ObjSyncCmpFilt::ordByError)
			idx = 5;
		else if(Filt.Order == ObjSyncCmpFilt::ordByCommID)
			idx = 0;
		else if(Filt.Order == ObjSyncCmpFilt::ordBySrcID)
			idx = 1;
		else if(Filt.Order == ObjSyncCmpFilt::ordBySrcName)
			idx = 3;
		else if(Filt.Order == ObjSyncCmpFilt::ordByDestID)
			idx = 2;
		else if(Filt.Order == ObjSyncCmpFilt::ordByDestName)
			idx = 4;
		else
			idx = 5;
		MEMSZERO(k);
		if(Filt.ObjType) {
			dbq = & (*dbq && P_TmpTbl->ObjType == Filt.ObjType);
			k.k0.ObjType = Filt.ObjType;
		}
		if(Filt.ErrCode == 0) {
			if(Filt.Flags & ObjSyncCmpFilt::fErrOnly)
				dbq = & (*dbq && P_TmpTbl->ObjType > 0L);
		}
		else
			dbq = & (*dbq && P_TmpTbl->ErrCode == (long)Filt.ErrCode);
		dbq = & (*dbq && realrange(P_TmpTbl->CmpScore, Filt.Score.low, Filt.Score.upp));
		P_IterQuery = new BExtQuery(P_TmpTbl, idx, 16);
		if(P_IterQuery) {
			P_IterQuery->selectAll().where(*dbq);
			P_IterQuery->initIteration(0, &k, spGe);
			return 1;
		}
		else
			return PPSetErrorNoMem();
	}
	return 0;
}

int FASTCALL PPViewObjSyncCmp::NextIteration(ObjSyncCmpViewItem * pItem)
{
	if(P_IterQuery && P_IterQuery->nextIteration() > 0) {
		P_TmpTbl->copyBufTo(pItem);
		return 1;
	}
	return -1;
}

int SLAPI PPViewObjSyncCmp::CalcTotal(ObjSyncCmpTotal * pTotal)
{
	memzero(pTotal, sizeof(*pTotal));
	ObjSyncCmpViewItem item;
	PPWait(1);
	for(InitIteration(); NextIteration(&item) > 0;) {
		pTotal->Count++;
		switch(item.ErrCode) {
			case 0: pTotal->NeCount++; break;
			case 1: pTotal->N1Count++; break;
			case 2: pTotal->N2Count++; break;
			case 3: pTotal->N3Count++; break;
			case 4: pTotal->N4Count++; break;
		}
	}
	PPWait(0);
	return 1;
}

int SLAPI PPViewObjSyncCmp::ViewTotal()
{
	int    ok = -1;
	ObjSyncCmpTotal total;
	CalcTotal(&total);
	TDialog * dlg = new TDialog(DLG_SYNCCMPTOTAL);
	if(CheckDialogPtrErr(&dlg)) {
		dlg->setCtrlLong(CTL_SYNCCMPTOTAL_NECOUNT, total.NeCount);
		dlg->setCtrlLong(CTL_SYNCCMPTOTAL_N1COUNT, total.N1Count);
		dlg->setCtrlLong(CTL_SYNCCMPTOTAL_N2COUNT, total.N2Count);
		dlg->setCtrlLong(CTL_SYNCCMPTOTAL_N3COUNT, total.N3Count);
		dlg->setCtrlLong(CTL_SYNCCMPTOTAL_N4COUNT, total.N4Count);
		dlg->setCtrlLong(CTL_SYNCCMPTOTAL_COUNT,   total.Count);
		ExecViewAndDestroy(dlg);
	}
	else
		ok = 0;
	return ok;
}

int SLAPI PPViewObjSyncCmp::RemoveAllSync()
{
	int    ok = -1;
	if(!P_Ot)
		P_Ot = new PPObjectTransmit(PPObjectTransmit::tmUndef, 1, 0);
	if(P_Ot && CONFIRMCRIT(PPCFM_DELETE)) {
		ObjSyncCmpViewItem item;
		PPWait(1);
		PPTransaction tra(1);
		THROW(tra);
		for(InitIteration(); NextIteration(&item) > 0;) {
			int    r;
			PPCommSyncID comm_id;
			comm_id = item;
			THROW(r = P_Ot->SyncTbl.RemoveByCommID(item.ObjType, comm_id, 0, 0));
			if(r > 0) {
				THROW(P_Ot->UpdateSyncCmpItem(P_TmpTbl, item.ObjType, comm_id));
				ok = 1;
			}
		}
		THROW(tra.Commit());
		PPWait(0);
	}
	CATCHZOKPPERR
	return ok;
}

DBQuery * SLAPI PPViewObjSyncCmp::CreateBrowserQuery(uint * pBrwId, SString * pSubTitle)
{
	DBQuery * q = 0;
	uint   brw_id = BROWSER_SYNCCMP;
	DBQ  * dbq = 0;
	DBE    dbe_commid;
	TempSyncCmpTbl * p_t = 0;
	THROW_PP(P_TmpTbl, PPERR_PPVIEWNOTINITED);
	THROW_MEM(p_t = new TempSyncCmpTbl(P_TmpTbl->GetName()));
	{
		dbe_commid.init();
		dbe_commid.push(p_t->CommIdPfx);
		dbe_commid.push(p_t->CommID);
		dbe_commid.push((DBFunc)PPDbqFuncPool::IdCommSyncId);
	}
	q = & select(
		p_t->ObjType,   // #0
		p_t->DestID,    // #1
		p_t->CommIdPfx, // #2
		p_t->CommID,    // #3
		p_t->SrcID,     // #4
		p_t->SrcName,   // #5
		p_t->DestName,  // #6
		p_t->ErrCode,   // #7
		p_t->CmpScore,  // #8
		dbe_commid,     // #9
		p_t->SrcModDt,  // #10
		p_t->SrcModTm,  // #11
		p_t->DestModDt, // #12
		p_t->DestModTm, // #13
		0L).from(p_t, 0L);
	if(Filt.ObjType)
		dbq = &(*dbq && p_t->ObjType == Filt.ObjType);
	if(Filt.ErrCode == 0) {
		if(Filt.Flags & ObjSyncCmpFilt::fErrOnly)
			dbq = & (*dbq && p_t->ErrCode > 0L);
	}
	else
		dbq = & (*dbq && p_t->ErrCode == (long)Filt.ErrCode);
	dbq = & (*dbq && realrange(P_TmpTbl->CmpScore, Filt.Score.low, Filt.Score.upp));
	q->where(*dbq);
	switch(Filt.Order) {
		case ObjSyncCmpFilt::ordByError:    q->orderBy(p_t->ObjType, p_t->ErrCode, p_t->CmpScore, 0L); break;
		case ObjSyncCmpFilt::ordByCommID:   q->orderBy(p_t->ObjType, p_t->CommIdPfx, p_t->CommID, 0L); break;
		case ObjSyncCmpFilt::ordBySrcID:    q->orderBy(p_t->ObjType, p_t->SrcID, 0L);    break;
		case ObjSyncCmpFilt::ordBySrcName:  q->orderBy(p_t->ObjType, p_t->SrcName, 0L);  break;
		case ObjSyncCmpFilt::ordByDestID:   q->orderBy(p_t->ObjType, p_t->DestID, 0L);   break;
		case ObjSyncCmpFilt::ordByDestName: q->orderBy(p_t->ObjType, p_t->DestName, 0L); break;
	}
	THROW(CheckQueryPtr(q));
	CATCH
		if(q)
			ZDELETE(q);
		else
			delete p_t;
	ENDCATCH
	ASSIGN_PTR(pBrwId, brw_id);
	return q;
}

int SLAPI PPViewObjSyncCmp::ProcessCommand(uint ppvCmd, const void * pHdr, PPViewBrowser * pBrw)
{
	int    ok = PPView::ProcessCommand(ppvCmd, pHdr, pBrw);
	if(ok == -2) {
		struct Hdr {
			PPID   ObjType;
			PPID   DestID;
			PPCommSyncID CommID;
		};
		const Hdr * p_hdr = pHdr ? static_cast<const Hdr *>(pHdr) : 0;
		switch(ppvCmd) {
			case PPVCMD_EDITOBJ:
				ok = p_hdr ? EditPPObj(p_hdr->ObjType, p_hdr->DestID) : -1;
				break;
			case PPVCMD_DELETEITEM:
				ok = -1;
				if(p_hdr && p_hdr->ObjType && !p_hdr->CommID.IsZero()) {
					if(!P_Ot)
						P_Ot = new PPObjectTransmit(PPObjectTransmit::tmUndef, 1, 0);
					if(P_Ot && CONFIRM(PPCFM_DELETE)) {
						int r = P_Ot->SyncTbl.RemoveByCommID(p_hdr->ObjType, p_hdr->CommID, 0, 1);
						if(r > 0) {
							P_Ot->UpdateSyncCmpItem(P_TmpTbl, p_hdr->ObjType, p_hdr->CommID);
							ok = 1;
						}
						else if(r == 0)
							ok = PPErrorZ();
					}
				}
				break;
			case PPVCMD_DELETEALL:
				ok = RemoveAllSync();
				break;
			case PPVCMD_PUTTOBASKET:
				ok = -1;
				if(p_hdr && p_hdr->ObjType == PPOBJ_GOODS && p_hdr->DestID)
					AddGoodsToBasket(p_hdr->DestID, 0, 1, 0);
				break;
		}
	}
	return ok;
}

int SLAPI AnalyzeObjSyncCmp(int asModeless)
{
	ObjSyncCmpFilt init_filt;
	init_filt.Flags |= ObjSyncCmpFilt::fInitialize;
	return PPView::Execute(PPVIEW_OBJSYNCCMP, &init_filt, asModeless, reinterpret_cast<void *>(1));
}
//
// Implementation of PPALDD_ObjSyncCmp
//
PPALDD_CONSTRUCTOR(ObjSyncCmp)
{
	if(Valid) {
		AssignHeadData(&H, sizeof(H));
		AssignDefIterData(&I, sizeof(I));
	}
}

PPALDD_DESTRUCTOR(ObjSyncCmp) { Destroy(); }

int PPALDD_ObjSyncCmp::InitData(PPFilt & rFilt, long rsrv)
{
	INIT_PPVIEW_ALDD_DATA_U(ObjSyncCmp, rsrv);
	H.FltObjType = p_filt->ObjType;
	H.DestDbDiv  = LConfig.DBDiv;
	H.FltErrCode = p_filt->ErrCode;
	H.fErrOnly   = BIN(p_filt->Flags & ObjSyncCmpFilt::fErrOnly);
	H.Order      = p_filt->Order;
	return DlRtm::InitData(rFilt, rsrv);
}

int PPALDD_ObjSyncCmp::InitIteration(PPIterID iterId, int sortId, long /*rsrv*/)
{
	INIT_PPVIEW_ALDD_ITER(ObjSyncCmp);
}

int PPALDD_ObjSyncCmp::NextIteration(PPIterID iterId)
{
	START_PPVIEW_ALDD_ITER(ObjSyncCmp);
	I.ObjType   = item.ObjType;
	I.CommIdPfx = item.CommIdPfx;
	I.CommID    = item.CommID;
	I.SrcID     = item.SrcID;
	I.DestID    = item.DestID;
	I.ErrCode   = item.ErrCode;
	I.CmpScore  = item.CmpScore;
	I.SrcModDt  = item.SrcModDt;
	I.SrcModTm  = item.SrcModTm;
	I.DestModDt = item.DestModDt;
	I.DestModTm = item.DestModTm;
	{
		timefmt(I.SrcModTm,  TIMF_HMS, I.SrcTmTxt);
		timefmt(I.DestModTm, TIMF_HMS, I.DestTmTxt);

		PPCommSyncID comm_id;
		comm_id = item;
		SString temp_buf;
		comm_id.ToStr(0, temp_buf).CopyTo(I.CommIdTxt, sizeof(I.CommIdTxt));
	}
	STRNSCPY(I.SrcName, item.SrcName);
	STRNSCPY(I.DestName, item.DestName);
	FINISH_PPVIEW_ALDD_ITER();
}

void PPALDD_ObjSyncCmp::Destroy()
{
	DESTROY_PPVIEW_ALDD(ObjSyncCmp);
}
//
// @ModuleDecl(PPViewObjSyncQueue)
//
IMPLEMENT_PPFILT_FACTORY(ObjSyncQueue); SLAPI ObjSyncQueueFilt::ObjSyncQueueFilt() : PPBaseFilt(PPFILT_OBJSYNCQUEUE, 0, 0)
{
	SetFlatChunk(offsetof(ObjSyncQueueFilt, ReserveStart),
		offsetof(ObjSyncQueueFilt, Reserve)-offsetof(ObjSyncQueueFilt, ReserveStart)+sizeof(Reserve));
	Init(1, 0);
}
//
//
//
SLAPI PPViewObjSyncQueue::PPViewObjSyncQueue() : PPView(0, &Filt, PPVIEW_OBJSYNCQUEUE)
{
}

SLAPI PPViewObjSyncQueue::~PPViewObjSyncQueue()
{
}

int SLAPI PPViewObjSyncQueue::EditBaseFilt(PPBaseFilt * pFilt)
{
	return Filt.IsA(pFilt) ? 1 : PPErrorZ();
}

int SLAPI PPViewObjSyncQueue::Init_(const PPBaseFilt * pFilt)
{
	BExtQuery::ZDelete(&P_IterQuery);
	return Helper_InitBaseFilt(pFilt);
}

DBQuery * SLAPI PPViewObjSyncQueue::CreateBrowserQuery(uint * pBrwId, SString * pSubTitle)
{
	DBQuery * q = 0;
	uint   brw_id = BROWSER_OBJSYNCQUEUE;
	DBQ  * dbq = 0;
	DBE    dbe_commid;
	DBE    dbe_objtitle;
	ObjSyncQueueTbl * p_t = new ObjSyncQueueTbl;
	{
		dbe_commid.init();
		dbe_commid.push(p_t->CommIdPfx);
		dbe_commid.push(p_t->CommID);
		dbe_commid.push((DBFunc)PPDbqFuncPool::IdCommSyncId);
	}
	PPDbqFuncPool::InitLongFunc(dbe_objtitle, PPDbqFuncPool::IdObjTitle, p_t->ObjType);
	q = &select(
	 	p_t->ID,         // #00
		p_t->DBID,       // #01
		p_t->ObjType,    // #02
		p_t->ObjID,      // #03
		p_t->CommIdPfx,  // #04
		p_t->CommID,     // #05
		p_t->PrimObjID,  // #06
		p_t->ModDt,      // #07
		p_t->ModTm,      // #08
		p_t->Priority,   // #09
		p_t->Flags,      // #10
		p_t->ObjName,    // #11
		dbe_commid,      // #12
		dbe_objtitle,    // #13
		0L).from(p_t, 0L).orderBy(p_t->Priority, 0L);
	ASSIGN_PTR(pBrwId, brw_id);
	return q;
}

int SLAPI PPViewObjSyncQueue::ProcessCommand(uint ppvCmd, const void * pHdr, PPViewBrowser * pBrw)
{
	int    ok = PPView::ProcessCommand(ppvCmd, pHdr, pBrw);
	if(ok == -2) {
		switch(ppvCmd) {
			case PPVCMD_RECEIVE:
				ok = -1;
				if(PPObjectTransmit::ReceivePackets(0) > 0)
					ok = 1;
				break;
			case PPVCMD_DELETEALL:
				ok = -1;
				if(CONFIRM(PPCFM_OBJSYNCQUEUECLEAR)) {
					if(Tbl.Clear())
						ok = 1;
					else
						ok = PPErrorZ();
				}
				break;
		}
	}
	return ok;
}
//
//
//
struct _ObjSyncEntry {
	PPCommSyncID CommID;
	int16  Flags;
	PPID   DBDivID;
	PPID   PrivID;
	LDATETIME ModDtm;
};

int SLAPI ViewObjSyncTab(PPObjID oid)
{
	class ObjSyncTabDialog : public PPListDialog {
	public:
		ObjSyncTabDialog() : PPListDialog(DLG_OBJSYNCTAB, CTL_OBJSYNCTAB_LIST)
		{
			updateList(-1);
		}
		int    setDTS(const TSVector <_ObjSyncEntry> * pData)
		{
			RVALUEPTR(Data, pData);
			updateList(-1);
			return 1;
		}
	private:
		virtual int setupList()
		{
			int    ok = 1;
			StringSet ss(SLBColumnDelim);
			SString temp_buf;
			for(uint i = 0; i < Data.getCount(); i++) {
				// @lbt_objsynctab        "20,L,Раздел;8,R,Общий ИД;8,R,Собственный ИД;12,L,Дата модификации"
				const _ObjSyncEntry & r_entry = Data.at(i);
				ss.clear();
				GetObjectName(PPOBJ_DBDIV, r_entry.DBDivID, temp_buf.Z());
				ss.add(temp_buf);
				ss.add(r_entry.CommID.ToStr(0, temp_buf.Z()));
				ss.add(temp_buf.Z().Cat(r_entry.PrivID));
				ss.add(temp_buf.Z().Cat(r_entry.ModDtm));
				addStringToList(i+1, ss.getBuf());
			}
			return ok;
		}
		TSVector <_ObjSyncEntry> Data;
	};

	int    ok = -1;
	ObjSyncTabDialog * dlg = 0;
	SString temp_buf;
	TSVector <_ObjSyncEntry> list;
	ObjSyncCore * p_tbl = DS.GetTLA().P_ObjSync;
	ObjSyncTbl::Rec hdr_rec, rec;
	if(p_tbl->SearchPrivate(oid.Obj, oid.Id, 0, &hdr_rec) > 0) {
		// ObjType, CommIdPfx, CommID, DBID (unique mod); // #0
		ObjSyncTbl::Key0 k0;
		MEMSZERO(k0);
		k0.ObjType = (int16)oid.Obj;
		k0.CommIdPfx = hdr_rec.CommIdPfx;
		k0.CommID = hdr_rec.CommID;
		while(p_tbl->search(0, &k0, spGt) && p_tbl->data.ObjType == oid.Obj &&
			p_tbl->data.CommIdPfx == hdr_rec.CommIdPfx && p_tbl->data.CommID == hdr_rec.CommID) {
			p_tbl->copyBufTo(&rec);
			_ObjSyncEntry entry;
			MEMSZERO(entry);
			entry.CommID = rec;
			entry.Flags = rec.Flags;
			entry.DBDivID = rec.DBID;
			entry.PrivID = rec.ObjID;
			entry.ModDtm.Set(rec.Dt, rec.Tm);
			list.insert(&entry);
		}
		GetObjectName(oid.Obj, oid.Id, temp_buf);
		{
			THROW(CheckDialogPtr(&(dlg = new ObjSyncTabDialog)));
			dlg->setStaticText(CTL_OBJSYNCTAB_ST_INFO, temp_buf);
			dlg->setDTS(&list);
			ExecViewAndDestroy(dlg);
			ok = -1;
		}
	}
	CATCHZOKPPERR
	return ok;
}
