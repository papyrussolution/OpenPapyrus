// V_CASHN.CPP
// Copyright (c) A.Starodub 2008, 2009, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018
// @codepage UTF-8
//
// Кассовые узлы
//
#include <pp.h>
#pragma hdrstop

IMPLEMENT_PPFILT_FACTORY(CashNode); SLAPI CashNodeFilt::CashNodeFilt() : PPBaseFilt(PPFILT_CASHNODE, 0, 0)
{
	SetFlatChunk(offsetof(CashNodeFilt, ReserveStart),	offsetof(CashNodeFilt, ReserveEnd) - offsetof(CashNodeFilt, ReserveStart));
	Init(1, 0);
}

CashNodeFilt & FASTCALL CashNodeFilt::operator = (const CashNodeFilt & s)
{
	Copy(&s, 0);
	return *this;
}

SLAPI PPViewCashNode::PPViewCashNode() : PPView(&ObjCashN, &Filt, PPVIEW_CASHNODE), P_TempTbl(0)
{
	ImplementFlags |= PPView::implDontEditNullFilter; // @v9.6.1
	PPLoadText(PPTXT_CMT, CashTypeNames);
	// @vmiller {
	int    s = 0, a = 0, idx = 0;
	SString line_buf;
	SString symbol, drv_name, drv_path;
	while(PPGetSubStr(PPTXT_CMT, idx, line_buf) > 0)
		idx++;
	for(long i = idx; GetStrFromDrvIni(PPINISECT_DRV_SYNCPOS, i, idx, line_buf) > 0; i++) {
		int    drv_impl = 0;
		if(PPAbstractDevice::ParseRegEntry(line_buf, symbol = 0, drv_name = 0, drv_path = 0, &drv_impl)) {
			if(CashTypeNames.Empty())
				CashTypeNames.Cat(i).Comma().Cat(drv_name.ToOem());
			else
				CashTypeNames.Semicol().Cat(i).Comma().Cat(drv_name.ToOem());
		}
	}
	// } @vmiller
}

SLAPI PPViewCashNode::~PPViewCashNode()
{
	ZDELETE(P_TempTbl);
}

int SLAPI PPViewCashNode::CheckForFilt(const PPCashNode * pRec) const
{
	if(pRec) {
		if(Filt.CashTypeID && Filt.CashTypeID != pRec->CashType)
			return 0;
		if(Filt.LocID && Filt.LocID != pRec->LocID)
			return 0;
		if((Filt.SyncType == CashNodeFilt::sOnlySync && (pRec->Flags & CASHF_ASYNC)) || (Filt.SyncType == CashNodeFilt::sOnlyASync && (pRec->Flags & CASHF_SYNC)))
			return 0;
	}
	return 1;
}

int SLAPI PPViewCashNode::MakeTempEntry(const PPCashNode * pRec, TempCashNodeTbl::Rec * pTempRec)
{
	int    ok = -1;
	if(pRec && pTempRec) {
		memzero(pTempRec, sizeof(*pTempRec)); // @fix @v8.2.11
		pTempRec->ID = pRec->ID;
		STRNSCPY(pTempRec->Name, pRec->Name);
		STRNSCPY(pTempRec->Symb, pRec->Symb);
		STRNSCPY(pTempRec->Port, pRec->Port);
		{
			pTempRec->ParentID = pRec->ParentID;
			PPCashNode parent_cn_rec;
			if(ObjCashN.Fetch(pRec->ParentID, &parent_cn_rec) > 0) {
				STRNSCPY(pTempRec->ParentName, parent_cn_rec.Name);
			}
		}
		pTempRec->LocID = pRec->LocID;
		pTempRec->CashTypeID = pRec->CashType;
		pTempRec->Flags = pRec->Flags;
		/* @v9.7.11
		{
			SString buf, buf1, buf2;
			PPGetSubStr(CashTypeNames, pTempRec->CashTypeID - 1, buf);
			buf.Divide(',', buf1, buf2);
			buf2.CopyTo(pTempRec->CashTypeName, sizeof(pTempRec->CashTypeName));
		}	
		*/
		// @v9.7.11 {
		{
			SString temp_buf;
			GetDeviceTypeName(DVCCLS_SYNCPOS, pTempRec->CashTypeID, temp_buf);
			STRNSCPY(pTempRec->CashTypeName, temp_buf);
		}
		// } @v9.7.11 
		ok = 1;
	}
	return ok;
}

int SLAPI PPViewCashNode::EditBaseFilt(PPBaseFilt * pFilt)
{
	int    ok = -1;
	CashNodeFilt filt;
	TDialog * p_dlg = new TDialog(DLG_CASHNFLT);
	filt.Copy(pFilt, 0);
	THROW(CheckDialogPtr(&p_dlg));
	SetupStringCombo(p_dlg, CTLSEL_CASHNFLT_TYPE, PPTXT_CMT, filt.CashTypeID);
	SetupPPObjCombo(p_dlg,  CTLSEL_CASHNFLT_LOC, PPOBJ_LOCATION, filt.LocID, 0, 0);
	p_dlg->AddClusterAssocDef(CTL_CASHNFLT_SYNCTYPE,  0, CashNodeFilt::sAll);
	p_dlg->AddClusterAssoc(CTL_CASHNFLT_SYNCTYPE,  1, CashNodeFilt::sOnlySync);
	p_dlg->AddClusterAssoc(CTL_CASHNFLT_SYNCTYPE,  2, CashNodeFilt::sOnlyASync);
	p_dlg->SetClusterData(CTL_CASHNFLT_SYNCTYPE, filt.SyncType);
	if(ExecView(p_dlg) == cmOK) {
		long sync_type = 0;
		filt.LocID      = p_dlg->getCtrlLong(CTLSEL_CASHNFLT_LOC);
		filt.CashTypeID = p_dlg->getCtrlLong(CTLSEL_CASHNFLT_TYPE);
		p_dlg->GetClusterData(CTL_CASHNFLT_SYNCTYPE, &sync_type);
		if(filt.CashTypeID)
			filt.SyncType = CashNodeFilt::sAll;
		else
			filt.SyncType = (int16)sync_type;
		if(pFilt)
			pFilt->Copy(&filt, 0);
		ok = 1;
	}
	CATCHZOKPPERR
	delete p_dlg;
	return ok;
}

// @v8.6.6 PP_CREATE_TEMP_FILE_PROC(CreateTempFile, TempCashNode);

int SLAPI PPViewCashNode::Init_(const PPBaseFilt * pFilt)
{
	int    ok = 1;
	THROW(Helper_InitBaseFilt(pFilt));
	BExtQuery::ZDelete(&P_IterQuery);
	ZDELETE(P_TempTbl);
	THROW(P_TempTbl = CreateTempFile <TempCashNodeTbl> ());
	{
		PPCashNode rec;
		BExtInsert bei(P_TempTbl);
		PPTransaction tra(ppDbDependTransaction, 1);
		THROW(tra);
		for(PPID id = 0; ObjCashN.EnumItems(&id, &rec) > 0;) {
			if(CheckForFilt(&rec) > 0) {
				TempCashNodeTbl::Rec temp_rec;
				MakeTempEntry(&rec, &temp_rec);
				THROW_DB(bei.insert(&temp_rec));
			}
		}
		THROW_DB(bei.flash());
		THROW(tra.Commit());
	}
	CATCH
		ZDELETE(P_TempTbl);
		ok = 0;
	ENDCATCH
	return ok;
}

int SLAPI PPViewCashNode::UpdateTempTable(const PPIDArray * pIdList)
{
	int    ok = -1;
	if(pIdList && P_TempTbl) {
		PPTransaction tra(ppDbDependTransaction, 1);
		THROW(tra);
		for(uint i = 0; i < pIdList->getCount(); i++) {
			PPID   id = pIdList->at(i);
			PPCashNode rec;
			TempCashNodeTbl::Rec temp_rec;
			if(ObjCashN.Search(id, &rec) > 0 && CheckForFilt(&rec)) {
				ok = 1;
				MakeTempEntry(&rec, &temp_rec);
				if(SearchByID_ForUpdate(P_TempTbl, 0,  id, 0) > 0) {
					THROW_DB(P_TempTbl->updateRecBuf(&temp_rec)); // @sfu
				}
				else {
					THROW_DB(P_TempTbl->insertRecBuf(&temp_rec));
				}
			}
			else {
				THROW_DB(deleteFrom(P_TempTbl, 0, P_TempTbl->ID == id));
				ok = 1;
			}
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int SLAPI PPViewCashNode::InitIteration()
{
	int    ok = 1;
	TempCashNodeTbl::Key0 k, k_;
	BExtQuery::ZDelete(&P_IterQuery);
	P_IterQuery = new BExtQuery(P_TempTbl, 0, 128);
	P_IterQuery->selectAll();
	MEMSZERO(k);
	k_ = k;
	Counter.Init(P_IterQuery->countIterations(0, &k_, spGe));
	P_IterQuery->initIteration(0, &k, spGe);
	return ok;
}

int FASTCALL PPViewCashNode::NextIteration(CashNodeViewItem * pItem)
{
	while(pItem && P_IterQuery && P_IterQuery->nextIteration() > 0) {
		PPCashNode rec;
		TempCashNodeTbl * p_t = P_TempTbl;
		if(ObjCashN.Search(p_t->data.ID, &rec) > 0) {
			ASSIGN_PTR(pItem, rec);
			return 1;
		}
	}
	return -1;
}

DBQuery * SLAPI PPViewCashNode::CreateBrowserQuery(uint * pBrwId, SString * pSubTitle)
{
	DBQuery * q  = 0;
	TempCashNodeTbl * t = 0;
	uint   brw_id = BROWSER_CASHNODE;
	DBQ  * dbq = 0;
	DBE dbe_loc;
	THROW(CheckTblPtr(t = new TempCashNodeTbl(P_TempTbl->GetName())));
	PPDbqFuncPool::InitObjNameFunc(dbe_loc,  PPDbqFuncPool::IdObjNameLoc, t->LocID);
	q = & select(
		t->ID,            // #0
		t->Name,          // #1
		t->CashTypeName,  // #2
		dbe_loc,          // #3
		t->Port,          // #5
		t->Symb,          // #6
		t->ParentName,    // #7
		0L).from(t, 0L).where(*dbq).orderBy(t->Name, 0L);
	THROW(CheckQueryPtr(q));
	CATCH
		if(q)
			ZDELETE(q);
		else
			delete t;
	ENDCATCH
	ASSIGN_PTR(pBrwId, brw_id);
	return q;
}

int SLAPI PPViewCashNode::ExecCPanel(uint ppvCmd, PPID cashID)
{
	CashNodePaneFilt filt;
	filt.CashNodeID = cashID;
	filt.CommandID  = (ppvCmd == PPVCMD_OPPANEL) ? 0 : cmCSOpen;
	return ::ExecCSPanel(&filt);
}

int SLAPI PPViewCashNode::ProcessCommand(uint ppvCmd, const void * pHdr, PPViewBrowser * pBrw)
{
	int    ok = (ppvCmd != PPVCMD_ADDITEM) ? PPView::ProcessCommand(ppvCmd, pHdr, pBrw) : -2;
	PPIDArray  id_list;
	PPID   id = (pHdr) ? *(PPID *)pHdr : 0;
	if(ok == -2) {
		switch(ppvCmd) {
			case PPVCMD_ADDITEM:
				ok = (ObjCashN.Edit(&(id = 0), 0) == cmOK) ? 1 : -1;
				break;
			case PPVCMD_CSESSIONS:
				if(id) {
					CSessFilt filt;
					PPViewCSess v;
					filt.NodeList_.Add(id);
					// @v9.1.7 if(v.EditBaseFilt(&filt) > 0)
					ok = PPView::Execute(PPVIEW_CSESS, &filt, PPView::exefModeless, 0);
				}
				break;
			case PPVCMD_LOADDATA:
			case PPVCMD_OPPANEL:
				{
					MemLeakTracer mlt; // @debug
					int    exec_panel = 0;
					PPIDArray cash_list;
					PPCashNode rec;
					if(ppvCmd == PPVCMD_LOADDATA && ObjCashN.Search(id, &rec) > 0 && rec.CashType == PPCMT_CASHNGROUP) {
						PPIDArray _cash_list;
						ObjCashN.GetListByGroup(rec.ID, _cash_list);
						for(uint i = 0; i < _cash_list.getCount(); i++) {
							PPID child_id = _cash_list.at(i);
							if(ObjCashN.Search(child_id, &rec) > 0 && (rec.Flags & CASHF_ASYNC))
								cash_list.add(child_id);
						}
						exec_panel = BIN(cash_list.getCount() > 0);
					}
					else if(ppvCmd == PPVCMD_OPPANEL || (ppvCmd == PPVCMD_LOADDATA && ObjCashN.Search(id, &rec) > 0 && (rec.Flags & CASHF_ASYNC))) {
						cash_list.add(id);
						exec_panel = 1;
					}
					if(exec_panel)
						for(uint i = 0; i < cash_list.getCount(); i++)
							ok = ExecCPanel(ppvCmd, cash_list.at(i));
				}
				break;
			case PPVCMD_LOADSTAT:
				ok = -1;
				{
					DvcLoadingStatFilt filt;
					filt.DvcID   = id;
					filt.DvcType = 1;
					PPView::Execute(PPVIEW_DVCLOADINGSTAT, &filt, 1, 0);
				}
				break;
			case PPVCMD_PRINTZEROCHECK: // @v10.0.0
				ok = -1;
				if(id) {
					BillObj->PrintCheck(0, id, 0);
					ok = 1;
				}
				break;
		}
	}
	if(ok > 0 && oneof3(ppvCmd, PPVCMD_ADDITEM, PPVCMD_EDITITEM, PPVCMD_DELETEITEM)) {
		id_list.add(id);
		ok = UpdateTempTable(&id_list);
	}
	return ok;
}
