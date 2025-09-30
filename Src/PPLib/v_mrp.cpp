// V_MRP.CPP
// Copyright (c) A.Sobolev 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2015, 2016, 2017, 2018, 2019, 2020, 2021, 2022, 2024, 2025
// @codepage UTF-8
//
#include <pp.h>
#pragma hdrstop
//
// @ModuleDef(PPViewMrpTab)
//
int ViewMrpTab(const MrpTabFilt * pFilt);
int ViewMrpLine(const MrpLineFilt * pFilt, PPID mrpTabID);

IMPLEMENT_PPFILT_FACTORY(MrpTab); MrpTabFilt::MrpTabFilt() : PPBaseFilt(PPFILT_MRPTAB, 0, 0)
{
	SetFlatChunk(offsetof(MrpTabFilt, ReserveStart),
		offsetof(MrpTabFilt, Reserve)-offsetof(MrpTabFilt, ReserveStart)+sizeof(Reserve));
	Init(1, 0);
}

PPViewMrpTab::PPViewMrpTab() : PPView(&MrpObj, &Filt, PPVIEW_MRPTAB, 0, 0), P_TempOrd(0)
{
}

PPViewMrpTab::~PPViewMrpTab()
{
	delete P_TempOrd;
}

int PPViewMrpTab::EditBaseFilt(PPBaseFilt * pBaseFilt)
{
	int    ok = -1;
	TDialog * dlg = 0;
	if(!Filt.IsA(pBaseFilt))
		ok = 0;
	else if(CheckDialogPtrErr(&(dlg = new TDialog(DLG_MRPTABFLT)))) {
		MrpTabFilt * p_filt = static_cast<MrpTabFilt *>(pBaseFilt);
		PPObjMrpTab::SetupLinkObjTypeCombo(dlg, CTLSEL_MRPTABFLT_OBJTYPE, p_filt->LinkObjType);
		SetupPPObjCombo(dlg, CTLSEL_MRPTABFLT_LOC, PPOBJ_LOCATION, p_filt->LocID, 0, 0);
		dlg->SetupCalPeriod(CTLCAL_MRPTABFLT_PERIOD, CTL_MRPTABFLT_PERIOD);
		SetPeriodInput(dlg, CTL_MRPTABFLT_PERIOD, p_filt->Period);
		dlg->AddClusterAssoc(CTL_MRPTABFLT_FLAGS, 0, MrpTabFilt::fSkipChilds);
		dlg->SetClusterData(CTL_MRPTABFLT_FLAGS, p_filt->Flags);
		for(int valid_data = 0; !valid_data && ExecView(dlg) == cmOK;) {
			if(!GetPeriodInput(dlg, CTL_MRPTABFLT_PERIOD, &p_filt->Period))
				PPError();
			else {
				dlg->getCtrlData(CTLSEL_MRPTABFLT_OBJTYPE, &p_filt->LinkObjType);
				dlg->getCtrlData(CTLSEL_MRPTABFLT_LOC, &p_filt->LocID);
				dlg->GetClusterData(CTL_MRPTABFLT_FLAGS, &p_filt->Flags);
				ok = valid_data = 1;
			}
		}
	}
	else
		ok = 0;
	delete dlg;
	return ok;
}

int PPViewMrpTab::UpdateTempTable(PPID id)
{
	int    ok = 1;
	if(P_TempOrd) {
		SString temp_buf;
		MrpTabTbl::Rec rec;
		PPTransaction tra(ppDbDependTransaction, 1);
		THROW(tra);
		if(MrpObj.Search(id, &rec) > 0) {
			TempOrderTbl::Key0 k0;
			TempOrderTbl::Rec ord_rec;
			ord_rec.ID = id;
			if(rec.LinkObjType) {
				GetObjectName(rec.LinkObjType, rec.LinkObjID, temp_buf.Z());
				STRNSCPY(ord_rec.Name, temp_buf);
			}
			else
				ord_rec.Name[0] = 0;
			k0.ID = id;
			if(P_TempOrd->searchForUpdate(0, &k0, spEq)) {
				STRNSCPY(P_TempOrd->data.Name, ord_rec.Name);
				THROW_DB(P_TempOrd->updateRec()); // @sfu
			}
			else {
				THROW_DB(P_TempOrd->insertRecBuf(&ord_rec));
			}
		}
		else
			THROW_DB(deleteFrom(P_TempOrd, 0, P_TempOrd->ID == id));
		THROW(tra.Commit());
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

int PPViewMrpTab::Init_(const PPBaseFilt * pBaseFilt)
{
	int    ok = 1;
	SString temp_buf;
	MrpTabTbl * t = MrpObj.P_Tbl;
	MrpTabViewItem item;
	THROW(Helper_InitBaseFilt(pBaseFilt));
	ZDELETE(P_TempOrd);
	THROW(P_TempOrd = CreateTempOrderFile());
	for(InitIteration(); NextIteration(&item) > 0;) {
		TempOrderTbl::Rec temp_ord_rec;
		temp_ord_rec.ID = t->data.ID;
		if(t->data.LinkObjType) {
			GetObjectName(t->data.LinkObjType, t->data.LinkObjID, temp_buf.Z());
			STRNSCPY(temp_ord_rec.Name, temp_buf);
		}
		THROW_DB(P_TempOrd->insertRecBuf(&temp_ord_rec));
	}
	CATCHZOK
	return ok;
}

int PPViewMrpTab::InitIteration()
{
	BExtQuery::ZDelete(&P_IterQuery);
	Counter.Init();

	int    ok = 1;
	int    idx = 0;
	MrpTabTbl * t = MrpObj.P_Tbl;
	union {
		MrpTabTbl::Key0 k0;
		MrpTabTbl::Key1 k1;
		MrpTabTbl::Key2 k2;
		MrpTabTbl::Key3 k3;
	} k, k_;
	DBQ * dbq = 0;

	MEMSZERO(k);
	if(Filt.SingleID) {
		idx = 0;
		k.k0.ID = Filt.SingleID;
	}
	else if(Filt.ParentID) {
		idx = 3;
		dbq = &(*dbq && t->ParentID == Filt.ParentID);
		k.k3.ParentID = Filt.ParentID;
	}
	else {
		if(Filt.LinkObjType) {
			idx = 1;
			dbq = &(*dbq && t->LinkObjType == Filt.LinkObjType);
			dbq = ppcheckfiltid(dbq, t->LinkObjID, Filt.LinkObjID);
			k.k1.LinkObjType = Filt.LinkObjType;
			k.k1.LinkObjID = Filt.LinkObjID;
		}
		else {
			idx = 2;
			k.k2.Dt = Filt.Period.low;
		}
		dbq = &(*dbq && daterange(t->Dt, &Filt.Period));
		dbq = ppcheckfiltid(dbq, t->LocID, Filt.LocID);
		if(Filt.Flags & MrpTabFilt::fSkipChilds)
			dbq = &(*dbq && t->ParentID == 0L);
	}
	THROW_MEM(P_IterQuery = new BExtQuery(t, idx));
	P_IterQuery->selectAll().where(*dbq);
	k_ = k;
	Counter.Init(P_IterQuery->countIterations(0, &k_, spGe));
	P_IterQuery->initIteration(false, &k, spGe);
	CATCH
		ok = 0;
		BExtQuery::ZDelete(&P_IterQuery);
	ENDCATCH
	return ok;
}

int FASTCALL PPViewMrpTab::NextIteration(MrpTabViewItem * pItem)
{
	while(P_IterQuery && P_IterQuery->nextIteration() > 0) {
		Counter.Increment();
		if(Filt.SingleID || MrpObj.CheckForFilt(&Filt, MrpObj.P_Tbl->data.ID, &MrpObj.P_Tbl->data)) {
			MrpObj.P_Tbl->CopyBufTo(pItem);
			return 1;
		}
	}
	return -1;
}

int PPViewMrpTab::Detail(const void * pHdr, PPViewBrowser * pBrw)
{
	if(pHdr)
		ViewMrpLine(0, *static_cast<const  PPID *>(pHdr));
	return -1;
}

int PPViewMrpTab::ProcessCommand(uint ppvCmd, const void * pHdr, PPViewBrowser * pBrw)
{
	PPID   id = pHdr ? *static_cast<const  PPID *>(pHdr) : 0;
	//
	// По умолчанию двойной щелчок мыши и Enter генерируют команду PPVCMD_EDITITEM
	//
	if(ppvCmd == PPVCMD_EDITITEM)
		ppvCmd = PPVCMD_DETAIL;
	else if(ppvCmd == PPVCMD_EDITITEM2)
		ppvCmd = PPVCMD_EDITITEM;
	int    ok = PPView::ProcessCommand(ppvCmd, pHdr, pBrw);
	if(ok == -2) {
		switch(ppvCmd) {
			case PPVCMD_VIEWCHILDS:
				ok = -1;
				if(id && MrpObj.P_Tbl->GetSubList(id, 0) > 0) {
					MrpTabFilt filt;
					filt.ParentID = id;
					ViewMrpTab(&filt);
				}
				break;
		}
	}
	else if(ok > 0) {
		if(oneof3(ppvCmd, PPVCMD_ADDITEM, PPVCMD_EDITITEM, PPVCMD_DELETEITEM))
			ok = UpdateTempTable(id) ? 1 : PPErrorZ();
	}
	return ok;
}

/*
int PPViewMrpTab::ChangeFilt(int refreshOnly, BrowserWindow * pW)
{
	int    ok = -1;
	MrpTabFilt filt = Filt;
	if(refreshOnly || EditFilt(&filt) > 0) {
		uint   brw_id = 0;
		DBQuery * p_q = 0;
		DBQBrowserDef * p_def = (DBQBrowserDef*)pW->view->getDef();
		PPWaitStart();
		SString sub_title;
		if(Init(&filt) && (p_q = CreateBrowserQuery(&brw_id, &sub_title)) != 0) {
			p_def->setQuery(*p_q);
			pW->setSubTitle(sub_title);
			pW->refresh();
			ok = 1;
		}
		else
			ok = PPErrorZ();
		PPWaitStop();
	}
	return ok;
}
*/

DBQuery * PPViewMrpTab::CreateBrowserQuery(uint * pBrwId, SString * pSubTitle)
{
	DBQuery * q = 0;
	MrpTabTbl * t = 0;
	TempOrderTbl * ot = 0;
	LocationTbl * lt = 0;
	DBQ * dbq = 0;
	THROW(CheckTblPtr(t = new MrpTabTbl));
	THROW(CheckTblPtr(lt = new LocationTbl));
	THROW(CheckTblPtr(ot  = new TempOrderTbl(P_TempOrd->GetName())));
	q = & select(
		t->ID,           // #00
		t->Name,         // #01
		t->LinkObjType,  // #02
		ot->Name,        // #03
		lt->Name,        // #04
		t->Dt,           // #05
		0L).from(t, ot, lt, 0L);
	if(Filt.SingleID)
		dbq = &(*dbq && t->ID == Filt.SingleID);
	else if(Filt.ParentID) {
		dbq = &(*dbq && t->ParentID == Filt.ParentID);
	}
	else {
		if(Filt.LinkObjType) {
			dbq = &(*dbq && t->LinkObjType == Filt.LinkObjType);
			dbq = ppcheckfiltid(dbq, t->LinkObjID, Filt.LinkObjID);
		}
		dbq = &(*dbq && daterange(t->Dt, &Filt.Period));
		dbq = ppcheckfiltid(dbq, t->LocID, Filt.LocID);
		if(Filt.Flags & MrpTabFilt::fSkipChilds)
			dbq = &(*dbq && t->ParentID == 0L);
	}
	dbq = &(*dbq && ot->ID == t->ID && (lt->ID += t->LocID));
	q->where(*dbq);
	if(Filt.SingleID)
		q->orderBy(t->ID, 0L);
	else if(Filt.ParentID)
		q->orderBy(t->ParentID, 0L);
	else if(Filt.LinkObjType)
		q->orderBy(t->LinkObjType, t->LinkObjID, 0L);
	else
		q->orderBy(t->Dt, 0L);
	if(pSubTitle) {
		if(Filt.ParentID) {
			CatObjectName(PPOBJ_MRPTAB, Filt.ParentID, *pSubTitle);
		}
		else if(Filt.LinkObjType)
			GetObjectTitle(Filt.LinkObjType, *pSubTitle);
	}
	THROW(CheckQueryPtr(q));
	CATCH
		if(q) {
			ZDELETE(q);
		}
		else {
			delete lt;
			delete t;
			delete ot;
		}
	ENDCATCH
	ASSIGN_PTR(pBrwId, BROWSER_MRPTAB);
	return q;
}
//
//
//
int ViewMrpTab(const MrpTabFilt * pFilt) { return PPView::Execute(PPVIEW_MRPTAB, pFilt, PPView::exefModeless, 0); }
//
//
//
IMPLEMENT_PPFILT_FACTORY(MrpLine); MrpLineFilt::MrpLineFilt() : PPBaseFilt(PPFILT_MRPLINE, 0, 0)
{
	SetFlatChunk(offsetof(MrpLineFilt, ReserveStart),
		offsetof(MrpLineFilt, Reserve)-offsetof(MrpLineFilt, ReserveStart)+sizeof(Reserve));
	Init(1, 0);
}

PPViewMrpLine::PPViewMrpLine() : PPView(0, &Filt, PPVIEW_MRPLINE, 0, REPORT_MRPLINES), P_TempOrd(0)
{
}

PPViewMrpLine::~PPViewMrpLine()
{
	delete P_TempOrd;
}

PPBaseFilt * PPViewMrpLine::CreateFilt(const void * extraPtr) const
{
	MrpLineFilt * p_filt = 0;
	if(PPView::CreateFiltInstance(PPFILT_MRPLINE, reinterpret_cast<PPBaseFilt **>(&p_filt)))
		p_filt->TabID = (reinterpret_cast<long>(extraPtr));
	return static_cast<PPBaseFilt *>(p_filt);
}

int PPViewMrpLine::EditBaseFilt(PPBaseFilt * pBaseFilt)
{
	int    ok = -1;
	TDialog * dlg = 0;
	if(!Filt.IsA(pBaseFilt))
		ok = 0;
	else if(CheckDialogPtrErr(&(dlg = new TDialog(DLG_MRPLINEFLT)))) {
		MrpLineFilt * p_filt = static_cast<MrpLineFilt *>(pBaseFilt);
		{
			ushort v = 0;
			if(p_filt->SrcGoodsID == MRPSRCV_TOTAL && p_filt->Flags & MrpLineFilt::fShowTotalReq)
				v = 0;
			else if(p_filt->SrcGoodsID == MRPSRCV_DEP)
				v = 1;
			else if(p_filt->SrcGoodsID == MRPSRCV_INDEP)
				v = 2;
			else
				v = 0;
			dlg->setCtrlData(CTL_MRPLINEFLT_KIND, &v);
		}
		// @v11.5.6 {
		{
			ushort v = 0;
			if(p_filt->Ft_Terminal == 0)
				v = 0;
			else if(p_filt->Ft_Terminal > 0)
				v = 1;
			else if(p_filt->Ft_Terminal < 0)
				v = 2;
			dlg->setCtrlData(CTL_MRPLINEFLT_TERM, &v);
		}
		// } @v11.5.6 
		dlg->AddClusterAssoc(CTL_MRPLINEFLT_FLAGS, 0, MrpLineFilt::fShowDeficitOnly);
		// @v11.5.6 dlg->AddClusterAssoc(CTL_MRPLINEFLT_FLAGS, 1, MrpLineFilt::fShowTerminalOnly);
		dlg->SetClusterData(CTL_MRPLINEFLT_FLAGS, p_filt->Flags);
		for(int valid_data = 0; !valid_data && ExecView(dlg) == cmOK;) {
			{
				ushort v = 0;
				dlg->getCtrlData(CTL_MRPLINEFLT_KIND, &v);
				p_filt->Flags &= ~MrpLineFilt::fShowTotalReq;
				if(v == 0) {
					p_filt->SrcGoodsID = MRPSRCV_TOTAL;
					p_filt->Flags |= MrpLineFilt::fShowTotalReq;
				}
				else if(v == 1)
					p_filt->SrcGoodsID = MRPSRCV_DEP;
				else if(v == 2)
					p_filt->SrcGoodsID = MRPSRCV_INDEP;
			}
			// @v11.5.6 {
			{
				ushort v = 0;
				dlg->getCtrlData(CTL_MRPLINEFLT_TERM, &v);
				if(v == 0)
					p_filt->Ft_Terminal = 0;
				else if(v == 1)
					p_filt->Ft_Terminal = +1;
				else if(v == 2)
					p_filt->Ft_Terminal = -1;
				else
					p_filt->Ft_Terminal = 0;
			}
			// } @v11.5.6 
			dlg->GetClusterData(CTL_MRPLINEFLT_FLAGS, &p_filt->Flags);
			ok = valid_data = 1;
		}
	}
	else
		ok = 0;
	delete dlg;
	return ok;
}

int PPViewMrpLine::Init_(const PPBaseFilt * pBaseFilt)
{
	int    ok = 1;
	THROW(Helper_InitBaseFilt(pBaseFilt));
	BExtQuery::ZDelete(&P_IterQuery);
	ZDELETE(P_TempOrd);
	Counter.Init();
	CATCHZOK
	return ok;
}

int PPViewMrpLine::CreateOrderTable(IterOrder ord, TempOrderTbl ** ppTbl)
{
	int    ok = -1;
	TempOrderTbl * p_o = 0;
	BExtInsert * p_bei = 0;
	*ppTbl = 0;
	if(oneof3(ord, OrdByGoodsName, OrdByReq, OrdByDeficit)) {
		MrpLineViewItem item;
		SString goods_name;
		THROW(p_o = CreateTempOrderFile());
		THROW_MEM(p_bei = new BExtInsert(p_o));
		for(InitIteration(); NextIteration(&item) > 0;) {
			TempOrderTbl::Rec ord_rec;
			ord_rec.ID = item.ID;
			if(ord == OrdByGoodsName) {
				STRNSCPY(ord_rec.Name, GetGoodsName(Filt.DestGoodsID ? item.SrcID : item.DestID, goods_name));
			}
			else if(ord == OrdByReq) {
				sprintf(ord_rec.Name, "%055.8lf", Filt.DestGoodsID ? item.SrcReqQtty : item.DestReqQtty);
			}
			else if(ord == OrdByDeficit) {
				sprintf(ord_rec.Name, "%055.8lf", item.DestDfct);
			}
			THROW_DB(p_bei->insert(&ord_rec));
		}
		THROW_DB(p_bei->flash());
		*ppTbl = p_o;
		p_o = 0;
		ok = 1;
	}
	CATCHZOK
	delete p_o;
	delete p_bei;
	return ok;
}

int PPViewMrpLine::InitIteration()
{
	int    ok = 1, idx = 0;
	union {
		MrpLineTbl::Key1 k1;
		MrpLineTbl::Key2 k2;
	} k, k_;
	BExtQuery::ZDelete(&P_IterQuery);
	DBQ * dbq = 0;
	MrpLineTbl * t = &MrpObj.P_Tbl->Lines;
	MEMSZERO(k);
	k.k1.TabID = Filt.TabID;
	if(Filt.DestGoodsID) {
		idx = 1;
		k.k1.DestID = Filt.DestGoodsID;
		k.k1.SrcID = -MAXLONG;
	}
	else {
		idx = 2;
		k.k2.SrcID = Filt.SrcGoodsID;
	}
	P_IterQuery = new BExtQuery(t, idx);
	dbq = & (*dbq && t->TabID == Filt.TabID);
	if(Filt.DestGoodsID)
		dbq = & (*dbq && t->DestID == Filt.DestGoodsID && t->SrcID > 0L);
	else
		dbq = &(*dbq && t->SrcID == Filt.SrcGoodsID);
	P_IterQuery->where(*dbq);
	k_ = k;
	Counter.Init(P_IterQuery->countIterations(0, &k_, spGe));
	P_IterQuery->initIteration(false, &k, spGe);
	return 1;
}

int FASTCALL PPViewMrpLine::NextIteration(MrpLineViewItem * pItem)
{
	MrpLineTbl & r_lt = MrpObj.P_Tbl->Lines;
	while(P_IterQuery && P_IterQuery->nextIteration() > 0) {
		// @v11.5.6 if((!(Filt.Flags & MrpLineFilt::fShowSubst) || r_lt.data.Flags & MRPLF_SUBST) && (!(Filt.Flags & MrpLineFilt::fShowTerminalOnly) || r_lt.data.Flags & MRPLF_TERMINAL)) {
		if(!(Filt.Flags & MrpLineFilt::fShowSubst) || r_lt.data.Flags & MRPLF_SUBST) { // @v11.5.6
			if(Filt.Ft_Terminal == 0 || (Filt.Ft_Terminal > 0 && r_lt.data.Flags & MRPLF_TERMINAL) || (Filt.Ft_Terminal < 0 && !(r_lt.data.Flags & MRPLF_TERMINAL))) { // @v11.5.6
				MrpLineTbl::Rec rec;
				MrpLineTbl::Rec total;
				r_lt.CopyBufTo(&rec);
				MrpObj.P_Tbl->GetTotalLine(Filt.TabID, Filt.DestGoodsID ? rec.SrcID : rec.DestID, &total);
				rec.DestRest = total.DestRest;
				rec.DestDfct = total.DestDfct;
				if(Filt.Flags & MrpLineFilt::fShowDeficitOnly && total.DestDfct <= 0)
					continue;
				ASSIGN_PTR(pItem, rec);
				return 1;
			}
		}
	}
	return -1;
}

void PPViewMrpLine::ViewTotal()
{
	TDialog * dlg = new TDialog(DLG_MRPLNTOTAL);
	if(CheckDialogPtrErr(&dlg)) {
		MrpLineTotal total;
		CalcTotal(&total);
		dlg->setCtrlData(CTL_MRPLNTOTAL_COUNT, &total.Count);
		dlg->setCtrlData(CTL_MRPLNTOTAL_TERMCNT, &total.TermCount);
		dlg->setCtrlData(CTL_MRPLNTOTAL_DFCTCNT, &total.DfctCount);
		ExecViewAndDestroy(dlg);
	}
}

int PPViewMrpLine::CalcTotal(MrpLineTotal * pTotal)
{
	MrpLineViewItem item;
	MrpLineTotal total;
	MEMSZERO(total);
	for(InitIteration(); NextIteration(&item) > 0;) {
		PPID   leader_id = 0;
		MrpLineTbl::Rec total_rec;
		total.Count++;
		if(item.Flags & MRPLF_TERMINAL)
			total.TermCount++;
		if(Filt.DestGoodsID) {
			leader_id = item.SrcID;
			total.ReqQtty += item.SrcReqQtty;
		}
		else {
			leader_id = item.DestID;
			total.ReqQtty += item.DestReqQtty;
		}
		if(MrpObj.P_Tbl->GetTotalLine(Filt.TabID, leader_id, &total_rec) > 0)
			if(total_rec.DestDfct > 0)
				total.DfctCount++;
		total.Cost  += item.Cost;
		total.Price += item.Price;
	}
	ASSIGN_PTR(pTotal, total);
	return 1;
}

int PPViewMrpLine::GetItem(PPID lineID, MrpLineViewItem * pItem)
{
	return MrpObj.P_Tbl->SearchLineByID(lineID, pItem);
}

int PPViewMrpLine::Detail(const void * pHdr, PPViewBrowser * pBrw)
{
	int    ok = -1;
	PPID   line_id = pHdr ? *static_cast<const  PPID *>(pHdr) : 0;
	MrpLineViewItem item;
	if(GetItem(line_id, &item) > 0) {
		PPID   goods_id = Filt.DestGoodsID ? item.SrcID : item.DestID;
		MrpLineFilt flt;
		flt.TabID = Filt.TabID;
		if(Filt.DestGoodsID) {
			if(MrpObj.P_Tbl->IsTerminalGoods(Filt.TabID, item.SrcID) > 0)
				flt.SrcGoodsID = item.SrcID;  // Покажем позиции, от которых зависит спрос на item.SrcID
			else
				flt.DestGoodsID = item.SrcID; // Покажем позиции, спрос на которые зависит от item.DestID
		}
		else if(item.SrcID == MRPSRCV_DEP)
			flt.SrcGoodsID = item.DestID;     // Покажем позиции, от которых зависит спрос на item.DestID
		else {
			if(MrpObj.P_Tbl->IsTerminalGoods(Filt.TabID, item.DestID) > 0)
				flt.SrcGoodsID = item.DestID;  // Покажем позиции, от которых зависит спрос на item.SrcID
			else
				flt.DestGoodsID = item.DestID; // Покажем позиции, спрос на которые зависит от item.DestID
		}
		ViewMrpLine(&flt, 0);
	}
	return ok;
}

int PPViewMrpLine::ProcessCommand(uint ppvCmd, const void * pHdr, PPViewBrowser * pBrw)
{
	int    ok = PPView::ProcessCommand(ppvCmd, pHdr, pBrw);
	if(ok == -2) {
		PPID   line_id = pHdr ? *static_cast<const  PPID *>(pHdr) : 0;
		switch(ppvCmd) {
			case PPVCMD_EDITGOODS:
				ok = -1;
				{
					MrpLineViewItem item;
					if(GetItem(line_id, &item) > 0) {
						PPID   _id_to_edit = Filt.DestGoodsID ? item.SrcID : item.DestID;
						PPObjGoods goods_obj;
						if(goods_obj.Edit(&_id_to_edit, 0) == cmOK)
							ok = 1;
					}
				}
				break;
			case PPVCMD_VIEWLOTS:
				ok = -1;
				{
					MrpLineViewItem item;
					if(GetItem(line_id, &item) > 0) {
						PPID   goods_id = Filt.DestGoodsID ? item.SrcID : item.DestID;
						MrpTabTbl::Rec tab_rec;
						if(MrpObj.Search(item.TabID, &tab_rec) > 0)
							::ViewLots(goods_id, tab_rec.LocID, 0, 0, 1);
					}
				}
				break;
			case PPVCMD_REPLACEMENT:
				ok = -1;
				{
					MrpLineViewItem item;
					if(GetItem(line_id, &item) > 0) {
						PPID   goods_id = Filt.DestGoodsID ? item.SrcID : item.DestID;
						if(MrpObj.P_Tbl->IsReplacedGoods(Filt.TabID, goods_id) > 0) {
							MrpLineFilt flt;
							flt.TabID = Filt.TabID;
							flt.DestGoodsID = goods_id;
							flt.Flags |= MrpLineFilt::fShowSubst;
							ViewMrpLine(&flt, 0);
						}
					}
				}
				break;
			case PPVCMD_PUTTOBASKET:
				ok = -1;
				{
					MrpLineViewItem item;
					if(GetItem(line_id, &item) > 0) {
						PPID   goods_id = 0;
						double qtty = 1.0;
						if(Filt.DestGoodsID) {
							goods_id = item.SrcID;
							qtty = item.SrcReqQtty;
						}
						else {
							goods_id = item.DestID;
							qtty = item.DestReqQtty;
						}
						if(goods_id) {
							AddGoodsToBasket(goods_id, 0, qtty, item.Price);
						}
					}
				}
				break;
			case PPVCMD_PUTTOBASKETALL:
				ok = -1;
				{
					SelBasketParam param;
					if(GetBasketByDialog(&param, GetSymb()) > 0) {
						PPObjGoods goods_obj;
						MrpLineViewItem item;
						for(InitIteration(); NextIteration(&item) > 0;) {
							ILTI   i_i;
							PPID   goods_id = 0;
							double qtty = 1.0;
							if(Filt.DestGoodsID) {
								i_i.GoodsID = item.SrcID;
								i_i.Quantity = item.SrcReqQtty;
							}
							else {
								i_i.GoodsID = item.DestID;
								i_i.Quantity = item.DestReqQtty;
							}
							if(i_i.Quantity != 0.0 && goods_obj.Fetch(i_i.GoodsID, 0) > 0) {
								i_i.Price = item.Price;
								param.Pack.AddItem(&i_i, 0, param.SelReplace);
							}
						}
						GoodsBasketDialog(param, 1);
					}
				}
				break;
		}
	}
	return ok;
}

DBQuery * PPViewMrpLine::CreateBrowserQuery(uint * pBrwId, SString * pSubTitle)
{
	static DbqStringSubst flag_subst(2);  // @global @threadsafe

	DBQuery * q = 0;
	MrpLineTbl * t = 0, * t2 = 0;
	Goods2Tbl  * gt = 0;
	TempOrderTbl * ot = 0;
	DBQ  * dbq = 0;
	DBE  * dbe_term = 0;
	DBE  * dbe_replaced = 0;
	uint   brw_id = 0;

	THROW(CheckTblPtr(t = new MrpLineTbl));
	if(!(Filt.Flags & MrpLineFilt::fShowSubst))
		THROW(CheckTblPtr(t2 = new MrpLineTbl));
	THROW(CheckTblPtr(gt = new Goods2Tbl));
	THROW(CreateOrderTable(OrdByGoodsName, &ot));

	dbq = & (*dbq && t->TabID == Filt.TabID);
	if(Filt.Flags & MrpLineFilt::fShowSubst) {
		brw_id = BROWSER_MRPLINE_SUBST;
		q = & select(
			t->ID,               // #00
			gt->Name,            // #01
			t->DestReqQtty,      // #02
			t->SrcReqQtty,       // #03
			0L);
		dbq = & (*dbq && t->DestID == Filt.DestGoodsID && t->SrcID > 0L);
	}
	else {
		brw_id = BROWSER_MRPLINE;
		dbe_term     = & flagtoa(t2->Flags, MRPLF_TERMINAL, flag_subst.Get(PPTXT_FLAG_YES));
		dbe_replaced = & flagtoa(t2->Flags, MRPLF_REPLACED, flag_subst.Get(PPTXT_FLAG_YES));
		if(Filt.DestGoodsID) {
			q = & select(
				t->ID,           // #0
				t->Flags,        // #1
				gt->Name,        // #2
				t->SrcReqQtty,   // #3
				t2->DestRest,    // #4
				t2->DestDfct,    // #5
				*dbe_term,       // #6
				*dbe_replaced,   // #7
				t2->Cost,        // #8
				t2->Price,       // #9
				0L);
			dbq = & (*dbq && t->DestID == Filt.DestGoodsID && t->SrcID > 0L);
		}
		else {
			q = & select(
				t->ID,           // #0
				t->Flags,        // #1
				gt->Name,        // #2
				t->DestReqQtty,  // #3
				t2->DestRest,    // #4
				t2->DestDfct,    // #5
				*dbe_term,       // #6
				*dbe_replaced,   // #7
				t2->Cost,        // #8
				t2->Price,       // #9
				0L);
			dbq = &(*dbq && t->SrcID == Filt.SrcGoodsID);
		}
	}
	// @v11.5.6 dbq = ppcheckflag(dbq, t->Flags, MRPLF_TERMINAL, (Filt.Flags & MrpLineFilt::fShowTerminalOnly) ? 1 : 0);
	dbq = ppcheckflag(dbq, t->Flags, MRPLF_TERMINAL, Filt.Ft_Terminal); // @v11.5.6
	dbq = ppcheckflag(dbq, t->Flags, MRPLF_SUBST, (Filt.Flags & MrpLineFilt::fShowSubst) ? 1 : 0);
	if(Filt.Flags & MrpLineFilt::fShowSubst) {
		dbq = &(*dbq && gt->ID == t->SrcID);
		if(ot) {
			dbq = &(*dbq && t->ID == ot->ID);
			q->from(ot, t, gt, 0L).where(*dbq).orderBy(ot->Name, 0L);
		}
		else
			q->from(t, gt, 0L).where(*dbq);
	}
	else {
		if(Filt.DestGoodsID) {
			dbq = &(*dbq && gt->ID == t->SrcID &&
				t2->DestID == t->SrcID && t2->TabID == Filt.TabID && t2->SrcID == (long)MRPSRCV_TOTAL);
		}
		else {
			dbq = &(*dbq && gt->ID == t->DestID &&
				t2->DestID == t->DestID && t2->TabID == Filt.TabID && t2->SrcID == (long)MRPSRCV_TOTAL);
		}
		if(Filt.Flags & MrpLineFilt::fShowDeficitOnly)
			dbq = &(*dbq && t2->DestDfct > 0.0);
		if(ot) {
			dbq = &(*dbq && t->ID == ot->ID);
			q->from(ot, t, t2, gt, 0L).where(*dbq).orderBy(ot->Name, 0L);
		}
		else
			q->from(t, t2, gt, 0L).where(*dbq);
	}
	THROW(CheckQueryPtr(q));
	if(pSubTitle) {
		SString temp_buf, goods_name;
		*pSubTitle = 0;
		if(Filt.Flags & MrpLineFilt::fShowSubst) {
			PPGetSubStr(PPTXT_ST_MRPLINE, 2, temp_buf);
			pSubTitle->Printf(temp_buf, GetGoodsName(Filt.DestGoodsID, goods_name).cptr());
		}
		else if(Filt.DestGoodsID) {
			PPGetSubStr(PPTXT_ST_MRPLINE, 0, temp_buf);
			pSubTitle->Printf(temp_buf, GetGoodsName(Filt.DestGoodsID, goods_name).cptr());
		}
		else if(Filt.SrcGoodsID) {
			PPGetSubStr(PPTXT_ST_MRPLINE, 1, temp_buf);
			pSubTitle->Printf(temp_buf, GetGoodsName(Filt.SrcGoodsID, goods_name).cptr());
		}
		else if(Filt.TabID)
			GetObjectName(PPOBJ_MRPTAB, Filt.TabID, *pSubTitle);
	}
	CATCH
		if(q) {
			ZDELETE(q);
		}
		else {
			delete t;
			delete gt;
			delete ot;
		}
	ENDCATCH
	delete dbe_term;
	delete dbe_replaced;
	ASSIGN_PTR(pBrwId, brw_id);
	return q;
}
//
//
//
int ViewMrpLine(const MrpLineFilt * pFilt, PPID tabID) { return PPView::Execute(PPVIEW_MRPLINE, pFilt, 1, reinterpret_cast<void *>(tabID)); }
//
// Implementation of PPALDD_MrpTab
//
PPALDD_CONSTRUCTOR(MrpTab)
{
	if(Valid) {
		AssignHeadData(&H, sizeof(H));
		Extra[0].Ptr = new PPObjMrpTab;
	}
}

PPALDD_DESTRUCTOR(MrpTab)
{
	Destroy();
	delete static_cast<PPObjMrpTab *>(Extra[0].Ptr);
}

int PPALDD_MrpTab::InitData(PPFilt & rFilt, long rsrv)
{
	int    ok = -1;
	if(rFilt.ID == H.ID)
		ok = DlRtm::InitData(rFilt, rsrv);
	else {
		MEMSZERO(H);
		H.ID = rFilt.ID;
		PPObjMrpTab * p_obj = static_cast<PPObjMrpTab *>(Extra[0].Ptr);
		MrpTabTbl::Rec rec;
		if(p_obj->Search(rFilt.ID, &rec) > 0) {
			H.ID = rec.ID;
			STRNSCPY(H.Name, rec.Name);
			H.LinkObjType = rec.LinkObjType;
			H.LinkObjID = rec.LinkObjID;
			H.LocID = rec.LocID;
			H.Dt = rec.Dt;
			H.Flags = rec.Flags;
			if(rec.LinkObjType && rec.LinkObjID) {
				SString temp_buf;
				GetObjectName(rec.LinkObjType, rec.LinkObjID, temp_buf.Z());
				STRNSCPY(H.LinkObjName, temp_buf);
			}
			else
				H.LinkObjName[0] = 0;
			ok = DlRtm::InitData(rFilt, rsrv);
		}
	}
	return ok;
}
//
// Implementation of PPALDD_MrpLines
//
PPALDD_CONSTRUCTOR(MrpLines)
{
	if(Valid) {
		AssignHeadData(&H, sizeof(H));
		AssignDefIterData(&I, sizeof(I));
	}
}

PPALDD_DESTRUCTOR(MrpLines) { Destroy(); }

int PPALDD_MrpLines::InitData(PPFilt & rFilt, long rsrv)
{
	INIT_PPVIEW_ALDD_DATA_U(MrpLine, rsrv);
	H.TabID  = p_filt->TabID;
	H.FltRowKind = (int16)p_filt->SrcGoodsID;
	H.FltShowDfctOnly = BIN(p_filt->Flags & MrpLineFilt::fShowDeficitOnly);
	// @v11.5.6 H.FltShowTermOnly = BIN(p_filt->Flags & MrpLineFilt::fShowTerminalOnly);
	H.FltShowTermOnly = (p_filt->Ft_Terminal > 0); // @v11.5.6 @todo необходимо выровнять структуру MrpLine для отражения новой семантики фильтрации по критерию терминальности
	return DlRtm::InitData(rFilt, rsrv);
}

void PPALDD_MrpLines::Destroy() { DESTROY_PPVIEW_ALDD(MrpLine); }
int  PPALDD_MrpLines::InitIteration(PPIterID iterId, int sortId, long /*rsrv*/) { INIT_PPVIEW_ALDD_ITER(MrpLine); }

int PPALDD_MrpLines::NextIteration(PPIterID iterId)
{
	START_PPVIEW_ALDD_ITER(MrpLine);
	const MrpLineFilt * p_filt = (const MrpLineFilt *)p_v->GetBaseFilt();
	I.ID = item.ID;
	if(p_filt) {
		I.GoodsID = p_filt->DestGoodsID ? item.SrcID : item.DestID;
		I.ReqQtty = p_filt->DestGoodsID ? item.SrcReqQtty : item.DestReqQtty;
	}
	I.Rest = item.DestRest;
	I.Deficit = item.DestDfct;
	FINISH_PPVIEW_ALDD_ITER();
}
