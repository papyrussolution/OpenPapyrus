// V_PSNEV.CPP
// Copyright (c) A.Sobolev, A.Starodub 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021
// @ModuleDef(PPViewPersonEvent)
//
#include <pp.h>
#pragma hdrstop
//
//
//
long AverageEventTimePrcssr::Item::GetAverage() const
{
	return Count ? diffdatetimesec(LastEv, FirstEv) / Count : 0;
}

AverageEventTimePrcssr::AverageEventTimePrcssr()
{
}

int AverageEventTimePrcssr::Add(long id1, long id2, LDATE dt, LTIME tm)
{
	uint   pos = 0;
	LAssoc srch_ids(id1, id2);
	if(List.lsearch(&srch_ids, &pos, PTR_CMPFUNC(_2long))) {
		Item & r_item = List.at(pos);
		if(r_item.FirstEv.d >= dt && r_item.FirstEv.t >= tm)
			r_item.FirstEv.Set(dt, tm);
		if(r_item.LastEv.d <= dt && r_item.LastEv.t <= tm)
			r_item.LastEv.Set(dt, tm);
		r_item.Count++;
	}
	else {
		Item item;
		item.ID1 = id1;
		item.ID2 = id2;
		item.FirstEv.Set(dt, tm);
		item.LastEv.Set(dt, tm);
		item.Count = 0;
		List.insert(&item);
	}
	return 1;
}

int AverageEventTimePrcssr::Enum(uint * pPos, Item ** pItem)
{
	return List.enumItems(pPos, (void **)pItem);
}
//
//
//
PersonEventViewItem::PersonEventViewItem() : GrpCount(0)
{
}

PersonEventViewItem & PersonEventViewItem::Z()
{
	memzero(this, sizeof(PersonEventTbl::Rec));
	SMemo.Z(); // @v11.1.12
	GrpText1.Z();
	GrpText2.Z();
	AvgEvTime.Z();
	GrpCount  = 0;
	return *this;
}
//
//
//
IMPLEMENT_PPFILT_FACTORY(PersonEvent); PersonEventFilt::PersonEventFilt() : PPBaseFilt(PPFILT_PERSONEVENT, 0, 2)
{
	SetFlatChunk(offsetof(PersonEventFilt, ReserveStart),
		offsetof(PersonEventFilt, PsnOpList) - offsetof(PersonEventFilt, ReserveStart));
	SetBranchObjIdListFilt(offsetof(PersonEventFilt, PsnOpList));
	Init(1, 0);
}
//
//
//
PPViewPersonEvent::PPViewPersonEvent() : PPView(&PsnEvObj, &Filt, PPVIEW_PERSONEVENT, 0, 0), P_TempGrpTbl(0), P_TempTbl(0)
{
}

PPViewPersonEvent::~PPViewPersonEvent()
{
	delete P_TempGrpTbl;
	delete P_TempTbl;
}

int PPViewPersonEvent::EditBaseFilt(PPBaseFilt * pBaseFilt)
{
	class PersonEventFiltDialog : public TDialog {
		DECL_DIALOG_DATA(PersonEventFilt);
		enum {
			ctlgroupPsnOp = 1
		};
	public:
		PersonEventFiltDialog() : TDialog(DLG_PSNEVFLT)
		{
			SetupCalPeriod(CTLCAL_PSNEVFLT_PERIOD, CTL_PSNEVFLT_PERIOD);
			addGroup(ctlgroupPsnOp, new PersonOpCtrlGroup(CTLSEL_PSNEVFLT_OP, CTLSEL_PSNEVFLT_PRMR, CTLSEL_PSNEVFLT_SCND, cmOpList));
		}
		DECL_DIALOG_SETDTS()
		{
			RVALUEPTR(Data, pData);
			PPID   kind = 0;
			PPPsnOpKindPacket pok_pack;
			PPObjPsnOpKind    pok_obj;
			SetPeriodInput(this, CTL_PSNEVFLT_PERIOD, &Data.Period);
			{
				PersonOpCtrlGroup::Rec op_rec(&Data.PsnOpList, Data.PrmrID, Data.ScndID);
				// @v10.5.8 PersonOpCtrlGroup * p_grp = static_cast<PersonOpCtrlGroup *>(getGroup(GRP_PSNOP));
				// @v10.5.8 CALLPTRMEMB(p_grp, setData(this, &op_rec));
				setGroupData(ctlgroupPsnOp, &op_rec); // @v10.5.8
			}
			SetupStringCombo(this, CTLSEL_PSNEVFLT_SUBST, PPTXT_SUBSTPSNEVLIST, Data.Sgpe);
			SetupSubstDateCombo(this, CTLSEL_PSNEVFLT_SUBSTDT, Data.Sgd);
			AddClusterAssoc(CTL_PSNEVFLT_FLAGS, 0, Data.fWithoutPair);
			SetClusterData(CTL_PSNEVFLT_FLAGS, Data.Flags);
			// @v10.8.12 {
			SetupPPObjCombo(this, CTLSEL_PSNEVFLT_EPRMRREG, PPOBJ_REGISTERTYPE, Data.ExtPrmrPersonRegID, 0, 0);
			{
				ObjTagFilt ot_filt(PPOBJ_PERSON, ObjTagFilt::fOnlyTags);
				SetupPPObjCombo(this, CTLSEL_PSNEVFLT_EPRMRTAG, PPOBJ_TAG, Data.ExtPrmrPersonTagID, 0, &ot_filt);
			}
			// } @v10.8.12
			return 1;
		}
		DECL_DIALOG_GETDTS()
		{
			GetPeriodInput(this, CTL_PSNEVFLT_PERIOD, &Data.Period);
			getCtrlData(CTLSEL_PSNEVFLT_PRMR, &Data.PrmrID);
			getCtrlData(CTLSEL_PSNEVFLT_SCND, &Data.ScndID);
			{
				PersonOpCtrlGroup::Rec op_rec;
				PersonOpCtrlGroup * p_grp = static_cast<PersonOpCtrlGroup *>(getGroup(ctlgroupPsnOp));
				CALLPTRMEMB(p_grp, getData(this, &op_rec));
				Data.PsnOpList = op_rec.PsnOpList;
				if(Data.PsnOpList.GetCount() == 0)
					Data.PsnOpList.Set(0);
			}
			getCtrlData(CTLSEL_PSNEVFLT_SUBST,   &Data.Sgpe);
			getCtrlData(CTLSEL_PSNEVFLT_SUBSTDT, &Data.Sgd);
			GetClusterData(CTL_PSNEVFLT_FLAGS, &Data.Flags);
			getCtrlData(CTLSEL_PSNEVFLT_EPRMRREG, &Data.ExtPrmrPersonRegID); // @v10.8.12
			getCtrlData(CTLSEL_PSNEVFLT_EPRMRTAG, &Data.ExtPrmrPersonTagID); // @v10.8.12
			ASSIGN_PTR(pData, Data);
			return 1;
		}
	};
	if(!Filt.IsA(pBaseFilt))
		return 0;
	PersonEventFilt * p_filt = static_cast<PersonEventFilt *>(pBaseFilt);
	DIALOG_PROC_BODY(PersonEventFiltDialog, p_filt);
}

PP_CREATE_TEMP_FILE_PROC(CreateTempGrpFile, TempPersonEvent);
PP_CREATE_TEMP_FILE_PROC(CreateTempFile_, PersonEvent);

int PPViewPersonEvent::Init_(const PPBaseFilt * pFilt)
{
	int    ok = 1, filt_saved = 0;
	PersonEventFilt save_filt;
	AverageEventTimePrcssr * p_avg_ev_prcssr = 0;
	TempPersonEventTbl * p_temp_grp_tbl = 0;
	PersonEventTbl * p_temp_tbl = 0;
	BExtQuery::ZDelete(&P_IterQuery);
	THROW(Helper_InitBaseFilt(pFilt));
	save_filt = Filt;
	filt_saved = 1;
	Filt.Period.Actualize(ZERODATE);
	Counter.Init();
	ZDELETE(P_TempGrpTbl);
	ZDELETE(P_TempTbl);
	if(Filt.Sgpe != sgpeNone || Filt.Sgd != sgdNone) {
		PersonEventViewItem item;
		TempPersonEventTbl::Key0 k0;
		THROW(p_temp_grp_tbl = CreateTempGrpFile());
		Filt.Sgpe = sgpeNone;
		Filt.Sgd  = sgdNone;
		THROW_MEM(p_avg_ev_prcssr = new AverageEventTimePrcssr);
		{
			SString temp_buf;
			PPTransaction tra(ppDbDependTransaction, 1);
			THROW(tra);
			for(InitIteration(); NextIteration(&item) > 0;) {
				TempPersonEventTbl::Rec temp_rec;
				MEMSZERO(k0);
				// @v10.7.5 @ctr MEMSZERO(temp_rec);
				temp_rec.Count = 1;
				if(save_filt.Sgd != sgdNone) {
					LTIME tm = ZEROTIME;
					LDATE dt = ZERODATE;
					ShrinkSubstDateExt(save_filt.Sgd, item.Dt, item.Tm, &dt, &tm);
					FormatSubstDateExt(save_filt.Sgd, dt, tm, temp_buf);
					temp_buf.CopyTo(temp_rec.DtSubst, sizeof(temp_rec.DtSubst));
					dt.v = (save_filt.Sgd == sgdHour) ? tm.v : dt.v;
					temp_rec.Dt  = dt;
					temp_rec.ID2 = dt.v;
				}
				else {
					datefmt(&temp_rec.Dt, DATF_DMY, temp_rec.DtSubst);
					temp_rec.Dt = item.Dt;
				}
				PsnEvObj.Subst(save_filt.Sgpe, item.OpID, item.PersonID, item.SecondID, &temp_rec.ID);
				PsnEvObj.GetSubstName(save_filt.Sgpe, temp_rec.ID, temp_rec.Name, sizeof(temp_rec.Name));
				k0.ID  = temp_rec.ID;
				k0.ID2 = temp_rec.ID2;
				if(p_temp_grp_tbl->searchForUpdate(0, &k0, spEq) > 0) {
					p_temp_grp_tbl->data.Count++;
					THROW_DB(p_temp_grp_tbl->updateRec()); // @sfu
				}
				else
					THROW_DB(p_temp_grp_tbl->insertRecBuf(&temp_rec));
				CALLPTRMEMB(p_avg_ev_prcssr, Add(temp_rec.ID, temp_rec.ID2, item.Dt, item.Tm));
			}
			if(p_avg_ev_prcssr) {
				AverageEventTimePrcssr::Item * p_avg_item = 0;
				for(uint pos = 0; p_avg_ev_prcssr->Enum(&pos, &p_avg_item) > 0;) {
					TempPersonEventTbl::Key0 k0;
					MEMSZERO(k0);
					k0.ID  = p_avg_item->ID1;
					k0.ID2 = p_avg_item->ID2;
					if(p_temp_grp_tbl->searchForUpdate(0, &k0, spEq) > 0) {
						p_temp_grp_tbl->data.AvgEvTime = p_avg_item->GetAverage();
						THROW_DB(p_temp_grp_tbl->updateRec()); // @sfu
					}
				}
			}
			THROW(tra.Commit());
			Filt = save_filt;
			P_TempGrpTbl = p_temp_grp_tbl;
			p_temp_grp_tbl = 0;
		}
	}
	else if(Filt.Flags & PersonEventFilt::fWithoutPair) {
		PersonEventViewItem item;
		THROW(p_temp_tbl = CreateTempFile_());
		{
			PPTransaction tra(ppDbDependTransaction, 1);
			THROW(tra);
			for(InitIteration(); NextIteration(&item) > 0;) {
				PersonEventTbl::Rec temp_rec;
				temp_rec = *static_cast<const PersonEventTbl::Rec *>(&item);
				THROW_DB(p_temp_tbl->insertRecBuf(&temp_rec));
			}
			THROW(tra.Commit());
			P_TempTbl = p_temp_tbl;
			p_temp_tbl = 0;
		}
	}
	filt_saved = 0;
	CATCH
		ZDELETE(P_TempTbl);
		ZDELETE(P_TempGrpTbl);
		ZDELETE(p_temp_grp_tbl);
		ZDELETE(p_temp_tbl);
		ok = 0;
	ENDCATCH
	ZDELETE(p_avg_ev_prcssr);
	if(filt_saved)
		Filt = save_filt;
	return ok;
}

int PPViewPersonEvent::InitIteration()
{
	BExtQuery::ZDelete(&P_IterQuery);
	int    ok = 1;
	int    idx = 0;
	DBQ  * dbq = 0;
	union {
		PersonEventTbl::Key1     k1;
		PersonEventTbl::Key2     k2;
		PersonEventTbl::Key3     k3;
		TempPersonEventTbl::Key0 tk0;
		TempPersonEventTbl::Key1 tk1;
	} k, k_;
	MEMSZERO(k);
	if(P_TempGrpTbl) {
		P_IterQuery = new BExtQuery(P_TempGrpTbl, 1);
		P_IterQuery->selectAll();
	}
	else {
		PersonEventTbl * pe = NZOR(P_TempTbl, PsnEvObj.P_Tbl);
		if(Filt.PrmrID) {
			k.k3.PersonID = Filt.PrmrID;
			k.k3.Dt = Filt.Period.low;
			idx = 3;
		}
		else if(Filt.PsnOpList.GetSingle()) {
			k.k2.OpID = Filt.PsnOpList.GetSingle();
			k.k2.Dt   = Filt.Period.low;
			idx = 2;
		}
		else {
			k.k1.Dt = Filt.Period.low;
			idx = 1;
		}
		P_IterQuery = new BExtQuery(pe, idx);
		dbq = & daterange(pe->Dt, &Filt.Period);
		dbq = & (*dbq && timerange(pe->Tm, &Filt.TmPeriod));
		dbq = ppcheckfiltid(dbq, pe->PersonID, Filt.PrmrID);
		if(Filt.PsnOpList.IsExists())
			dbq = ppcheckfiltidlist(dbq, pe->OpID, &Filt.PsnOpList.Get());
		dbq = ppcheckfiltid(dbq, pe->SecondID, Filt.ScndID);
		P_IterQuery->select(pe->ID, 0L);
	}
	k_ = k;
	P_IterQuery->where(*dbq);
	Counter.Init(P_IterQuery->countIterations(0, &k_, spGe));
	P_IterQuery->initIteration(false, &k, spGe);
	return ok;
}

int FASTCALL PPViewPersonEvent::NextIteration(PersonEventViewItem * pItem)
{
	SString buf;
	while(P_IterQuery && P_IterQuery->nextIteration() > 0) {
		PPWaitPercent(Counter.Increment());
		if(P_TempGrpTbl) {
			if(pItem) {
				TempPersonEventTbl::Rec & r_rec = P_TempGrpTbl->data;
				pItem->Z();
				pItem->ID       = r_rec.ID;
				pItem->Dt       = r_rec.Dt;
				pItem->GrpCount = r_rec.Count;
				pItem->GrpText1 = r_rec.Name;
				pItem->GrpText2 = r_rec.DtSubst;
				// @v11.1.12 (buf = r_rec.Name).CR().Cat(r_rec.DtSubst).CopyTo(pItem->Memo, sizeof(pItem->Memo));
				(pItem->SMemo = r_rec.Name).CR().Cat(r_rec.DtSubst); // @v11.1.12
				{
					LDATETIME dtm = ZERODATETIME;
					const long days = dtm.settotalsec(r_rec.AvgEvTime);
					if(days)
						pItem->AvgEvTime.Cat(days).CatChar('d').Space();
					pItem->AvgEvTime.Cat(dtm.t, TIMF_HMS);
				}
			}
			return 1;
		}
		else if(P_TempTbl) {
			PersonEventTbl::Rec rec;
			if(PsnEvObj.Search(P_TempTbl->data.ID, &rec) > 0) {
				ASSIGN_PTR(static_cast<PersonEventTbl::Rec *>(pItem), rec);
				return 1;
			}
		}
		else {
			int    skip = 0;
			PersonEventTbl::Rec rec;
			if(PsnEvObj.Search(PsnEvObj.P_Tbl->data.ID, &rec) > 0) {
				if(oneof2(Filt.DayOfWeek, 0, dayofweek(&rec.Dt, 1))) {
					if(Filt.Flags & PersonEventFilt::fWithoutPair) {
						PPPsnOpKind pok_rec;
						if(PokObj.Fetch(rec.OpID, &pok_rec) > 0 && pok_rec.PairOp) {
							PersonEventTbl::Rec pair_rec;
							if(PsnEvObj.SearchPairEvent(rec.ID, 0, 0, &pair_rec) > 0)
								skip = 1;
						}
						else
							skip = 1;
					}
				}
				else
					skip = 1;
			}
			else
				skip = 1;
			if(!skip) {
				ASSIGN_PTR(static_cast<PersonEventTbl::Rec *>(pItem), rec);
				return 1;
			}
		}
	}
	return -1;
}

DBQuery * PPViewPersonEvent::CreateBrowserQuery(uint * pBrwId, SString * pSubTitle)
{
	uint   brw_id = 0;
	PersonEventTbl * pe = 0;
	TempPersonEventTbl * t = 0;
	DBQuery * q = 0;
	DBQ  * dbq = 0;
	DBE    dbe_psn_prmr;
	DBE    dbe_psn_scnd;
	DBE    dbe_op;
	DBE    dbe_avg_tm;
	DBE    dbe_extreg; // @v10.8.12
	DBE    dbe_exttag; // @v10.8.12
	DBE    dbe_memo;   // @v11.1.12
	if(P_TempGrpTbl) {
		brw_id = BROWSER_PSNEVSUBST;
		THROW(CheckTblPtr(P_TempGrpTbl));
		THROW(CheckTblPtr(t = new TempPersonEventTbl(P_TempGrpTbl->GetName())));
		PPDbqFuncPool::InitObjNameFunc(dbe_avg_tm, PPDbqFuncPool::IdDurationToTime, t->AvgEvTime);
		q = & select(
			t->ID,       // #0
			t->ID2,      // #1
			t->Dt,       // #2
			t->DtSubst,  // #3
			t->Name,     // #4
			t->Count,    // #5
			dbe_avg_tm,  // #6
			0L).from(t, 0L);
		if(Filt.Sgd != sgdNone)
			q->orderBy(t->Dt, t->ID, t->ID2, 0);
	}
	else {
		brw_id = Filt.PrmrID ? BROWSER_PSNEVBYPSN : BROWSER_PSNEV;
		if(P_TempTbl) {
			THROW(CheckTblPtr(pe = new PersonEventTbl(P_TempTbl->GetName())));
		}
		else {
			THROW(CheckTblPtr(pe = new PersonEventTbl));
		}
		PPDbqFuncPool::InitObjNameFunc(dbe_psn_prmr, PPDbqFuncPool::IdObjNamePerson, pe->PersonID);
		PPDbqFuncPool::InitObjNameFunc(dbe_psn_scnd, PPDbqFuncPool::IdObjNamePerson, pe->SecondID);
		PPDbqFuncPool::InitObjNameFunc(dbe_op, PPDbqFuncPool::IdObjNamePsnOpKind, pe->OpID);
		PPDbqFuncPool::InitObjNameFunc(dbe_memo, PPDbqFuncPool::IdObjMemoPersonEvent, pe->ID); // @v11.1.12
		q = & select(
			pe->ID,       // #0
			pe->Flags,    // #1 
			pe->Dt,       // #2
			pe->Tm,       // #3
			dbe_psn_prmr, // #4
			dbe_op,       // #5
			dbe_psn_scnd, // #6
			dbe_memo,     // #7 // @v11.1.12 pe->Memo-->dbe_memo
			0L).from(pe, 0L);
		{
			dbe_extreg.init();
			if(Filt.ExtPrmrPersonRegID) {
				dbe_extreg.push(dbconst(Filt.ExtPrmrPersonRegID));
				dbe_extreg.push(dbconst(PPOBJ_PERSON));
				dbe_extreg.push(pe->PersonID);
				dbe_extreg.push(static_cast<DBFunc>(PPDbqFuncPool::IdObjRegisterText));
			}
			else
				dbe_extreg.push(static_cast<DBFunc>(PPDbqFuncPool::IdEmpty));
			q->addField(dbe_extreg); // #8
		}
		{
			dbe_exttag.init();
			if(Filt.ExtPrmrPersonTagID) {
				dbe_exttag.push(dbconst(Filt.ExtPrmrPersonTagID));
				dbe_exttag.push(pe->PersonID);
				dbe_exttag.push(static_cast<DBFunc>(PPDbqFuncPool::IdObjTagText));
			}
			else
				dbe_exttag.push(static_cast<DBFunc>(PPDbqFuncPool::IdEmpty));
			q->addField(dbe_exttag); // #9
		}
		dbq = & daterange(pe->Dt, &Filt.Period);
		dbq = & (*dbq && timerange(pe->Tm, &Filt.TmPeriod));
		dbq = ppcheckweekday(dbq, pe->Dt, Filt.DayOfWeek);
		if(Filt.PsnOpList.IsExists())
			dbq = ppcheckfiltidlist(dbq, pe->OpID, &Filt.PsnOpList.Get());
		dbq = ppcheckfiltid(dbq, pe->PersonID, Filt.PrmrID);
		dbq = ppcheckfiltid(dbq, pe->SecondID, Filt.ScndID);
		q->where(*dbq);
		if(Filt.PrmrID)
			q->orderBy(pe->PersonID, 0);
		else if(Filt.PsnOpList.GetSingle())
			q->orderBy(pe->OpID, 0);
		else
			q->orderBy(pe->Dt, 0);
	}
	THROW(CheckQueryPtr(q));
	if(pSubTitle) {
		if(Filt.PrmrID)
			GetPersonName(Filt.PrmrID, *pSubTitle);
		if(Filt.PsnOpList.GetCount() > 0) {
			SString sub2;
			pSubTitle->CatDivIfNotEmpty('-', 1);
			GetExtObjectName(Filt.PsnOpList, PPOBJ_PERSONOPKIND, 3, sub2);
			pSubTitle->Cat(sub2);
		}
	}
	CATCH
		if(q)
			ZDELETE(q);
		else {
			delete pe;
			delete t;
		}
	ENDCATCH
	ASSIGN_PTR(pBrwId, brw_id);
	return q;
}

void * PPViewPersonEvent::GetEditExtraParam() { return reinterpret_cast<void *>(Filt.PrmrID); }

static int CellStyleFunc(const void * pData, long col, int paintAction, BrowserWindow::CellStyle * pStyle, void * extraPtr)
{
	int    ok = -1;
	PPViewBrowser * p_brw = static_cast<PPViewBrowser *>(extraPtr);
	if(p_brw) {
		PPViewPersonEvent * p_view = static_cast<PPViewPersonEvent *>(p_brw->P_View);
		ok = p_view ? p_view->CellStyleFunc_(pData, col, paintAction, pStyle, p_brw) : -1;
	}
	return ok;
}

int PPViewPersonEvent::CellStyleFunc_(const void * pData, long col, int paintAction, BrowserWindow::CellStyle * pStyle, PPViewBrowser * pBrw)
{
	int    ok = -1;
	if(pBrw && pData && pStyle && !P_TempGrpTbl) {
		const  BrowserDef * p_def = pBrw->getDef();
		if(col >= 0 && col < p_def->getCountI()) {
			const BroColumn & r_col = p_def->at(col);
			const PPViewPersonEvent::BrwHdr * p_hdr = static_cast<const PPViewPersonEvent::BrwHdr *>(pData);
			if(r_col.OrgOffs == 5) { // OprKind
				if(p_hdr->Flags & PSNEVF_FORCEPAIR) {
					pStyle->Color2 = GetColorRef(SClrViolet);
					pStyle->Flags |= BrowserWindow::CellStyle::fLeftBottomCorner;
					ok = 1;
				}
			}
		}
	}
	return ok;
}

void PPViewPersonEvent::PreprocessBrowser(PPViewBrowser * pBrw)
{
	if(pBrw) {
		if(!P_TempGrpTbl && !P_TempTbl)
			pBrw->Advise(PPAdviseBlock::evPsnEvChanged, 0, PPOBJ_PERSONEVENT, 0);
		pBrw->SetCellStyleFunc(CellStyleFunc, pBrw);
		if(!P_TempGrpTbl) {
			//BrowserDef * p_def = pBrw->getDef();
			const DBQBrowserDef * p_def = static_cast<const DBQBrowserDef *>(pBrw->getDef());
			const DBQuery * p_q = p_def ? p_def->getQuery() : 0;
			//uint pos = 0;
			//if(p_q && p_q->getFieldPosByName("PVat", &pos)) {
			//}
			SString title_buf;
			if(Filt.ExtPrmrPersonRegID) {
				GetObjectName(PPOBJ_REGISTERTYPE, Filt.ExtPrmrPersonRegID, title_buf);
				pBrw->InsColumn(-1, title_buf,  8, 0, 0, 0);
			}
			if(Filt.ExtPrmrPersonTagID) {
				GetObjectName(PPOBJ_TAG, Filt.ExtPrmrPersonTagID, title_buf);
				pBrw->InsColumn(-1, title_buf,  9, 0, 0, 0);
			}
		}
	}
}

int PPViewPersonEvent::Transmit(PPID /*id*/)
{
	int    ok = -1;
	ObjTransmitParam param;
	if(ObjTransmDialog(DLG_OBJTRANSM, &param) > 0) {
		PersonEventViewItem item;
		const PPIDArray & rary = param.DestDBDivList.Get();
		PPObjIDArray objid_ary;
		PPWaitStart();
		for(InitIteration(); NextIteration(&item) > 0; PPWaitPercent(GetCounter()))
			objid_ary.Add(PPOBJ_PERSONEVENT, item.ID);
		THROW(PPObjectTransmit::Transmit(&rary, &objid_ary, &param));
		ok = 1;
	}
	CATCHZOKPPERR
	PPWaitStop();
	return ok;
}

int PPViewPersonEvent::ChangeFlags(long action)
{
	int    ok = -1;
	if(action == 1) {
		if(Filt.Flags & PersonEventFilt::fWithoutPair) {
			THROW(PsnEvObj.CheckRights(PSNEVRT_SETFORCEPAIR));
			{
				PersonEventTbl * t = PsnEvObj.P_Tbl;
				PersonEventViewItem item;
				PPTransaction tra(1);
				THROW(tra);
				PPWaitStart();
				for(InitIteration(); NextIteration(&item) > 0; PPWaitPercent(GetCounter())) {
					if(!(item.Flags & PSNEVF_FORCEPAIR)) {
						THROW_DB(updateFor(t, 0, (t->ID == item.ID), set(t->Flags, dbconst(item.Flags | PSNEVF_FORCEPAIR))));
						ok = 1;
					}
				}
				THROW(tra.Commit());
				PPWaitStop();
			}
		}
	}
	CATCHZOKPPERR
	return ok;
}

int PPViewPersonEvent::HandleNotifyEvent(int kind, const PPNotifyEvent * pEv, PPViewBrowser * pBrw, void * extraProcPtr)
{
	int    ok = -1, update = 0;
	if(pEv) {
		if(kind == PPAdviseBlock::evPsnEvChanged) {
			if(!P_TempGrpTbl && !P_TempTbl) {
				if(pEv->IsFinish())
					update = 1;
			}
		}
		ok = 1;
	}
	if(ok > 0 && update && pBrw) {
		//pBrw->refresh();
		pBrw->Update();
	}
	return ok;
}

int PPViewPersonEvent::ProcessCommand(uint ppvCmd, const void * pHdr, PPViewBrowser * pBrw)
{
	int    ok = -1;
	if(ppvCmd == PPVCMD_EDITITEM && (Filt.Sgpe || Filt.Sgd)) {
		ok = Detail(pHdr, pBrw);
	}
	else {
		ok = PPView::ProcessCommand(ppvCmd, pHdr, pBrw);
		if(ok == -2) {
			if(ppvCmd == PPVCMD_EDITPSNOPKIND) {
				ok = -1;
				PersonEventTbl::Rec rec;
				PPID id = pHdr ? *static_cast<const  PPID *>(pHdr) : 0;
				if(id && PsnEvObj.Search(id, &rec) > 0) {
					if(PokObj.Edit(&rec.OpID, 0) == cmOK)
						ok = 1;
				}
			}
			else if(ppvCmd == PPVCMD_CHNGFLAGS) {
				ok = ChangeFlags(1);
			}
			else if(ppvCmd == PPVCMD_TRANSMIT) {
				Transmit(0);
				ok = -1;
			}
		}
	}
	return ok;
}

int PPViewPersonEvent::Detail(const void * pHdr, PPViewBrowser * pBrw)
{
	int    ok = -1;
	if(Filt.Sgpe != sgpeNone || Filt.Sgd != sgdNone) {
		PPID   id  = pHdr ? static_cast<const  PPID *>(pHdr)[0] : 0;
		PPID   id2 = pHdr ? static_cast<const  PPID *>(pHdr)[1] : 0;
		int    r = 1;
		PersonEventFilt filt = Filt;
		filt.Sgd  = sgdNone;
		filt.Sgpe = sgpeNone;
		if(Filt.Sgd != sgdNone) {
			LDATE dt = ZERODATE;
			TempPersonEventTbl::Key0 k0;
			MEMSZERO(k0);
			k0.ID  = id;
			k0.ID2 = id2;
			if(P_TempGrpTbl->search(0, &k0, spEq) > 0) {
				LTIME tm;
				dt = P_TempGrpTbl->data.Dt;
				tm.v = dt.v;
				if(Filt.Sgd == sgdWeekDay)
					filt.DayOfWeek = (int16)dt.v;
				else
					ExpandSubstDateExt(Filt.Sgd, dt, tm, &filt.Period, &filt.TmPeriod);
			}
			else
				r = 0;
		}
		if(r) {
			if(Filt.Sgpe == sgpeOp) {
				filt.PsnOpList.Z().Add(id);
			}
			else if(Filt.Sgpe == sgpePerson)
				filt.PrmrID = id;
			else if(Filt.Sgpe == sgpeCntrAg)
				filt.ScndID = id;
		}
		if(r > 0)
			ok = PPView::Execute(PPVIEW_PERSONEVENT, &filt, 0, 0);
	}
	return ok;
}

void PPViewPersonEvent::ViewTotal()
{
	TDialog * p_dlg = new TDialog(DLG_PSNEVTTOTAL);
	if(CheckDialogPtrErr(&p_dlg)) {
		PPWaitStart();
		InitIteration();
		PPWaitStop();
		p_dlg->setCtrlLong(CTL_PSNEVTTOTAL_COUNT, GetCounter().GetTotal());
		p_dlg->disableCtrl(CTL_PSNEVTTOTAL_COUNT, 1);
		ExecViewAndDestroy(p_dlg);
	}
}

int PPViewPersonEvent::Print(const void *)
{
	uint   rpt_id = 0;
	if(Filt.Sgpe != sgpeNone && Filt.Sgd != sgdNone)
		rpt_id = REPORT_PERSONEVENTSUBST2;
	else if(Filt.Sgpe != sgpeNone || Filt.Sgd != sgdNone)
		rpt_id = REPORT_PERSONEVENTSUBST;
	else
		rpt_id = REPORT_PERSONEVENT;
	return Helper_Print(rpt_id, 0);
}
//
// Implementation of PPALDD_PersonEventOp
//
PPALDD_CONSTRUCTOR(PersonEventOp)
{
	InitFixData(rscDefHdr, &H, sizeof(H));
}

PPALDD_DESTRUCTOR(PersonEventOp) { Destroy(); }

int PPALDD_PersonEventOp::InitData(PPFilt & rFilt, long rsrv)
{
	int    ok = -1;
	if(rFilt.ID == H.ID)
		ok = DlRtm::InitData(rFilt, rsrv);
	else {
		MEMSZERO(H);
		H.ID = rFilt.ID;
		PPObjPsnOpKind pok_obj;
		PPPsnOpKind pok_rec;
		if(pok_obj.Fetch(H.ID, &pok_rec) > 0) {
			#define CPY_FLD(f) H.f = pok_rec.f
			CPY_FLD(RegTypeID);
			CPY_FLD(ExValGrp);
			CPY_FLD(PairType);
			CPY_FLD(ExValSrc);
			CPY_FLD(Flags);
			CPY_FLD(LinkBillOpID);
			CPY_FLD(PairOp);
			#undef CPY_FLD
			STRNSCPY(H.Name, pok_rec.Name);
			STRNSCPY(H.Symb, pok_rec.Symb);
			ok = DlRtm::InitData(rFilt, rsrv);
		}
	}
	return ok;
}
//
// Implementation of PPALDD_PersonEventBase
//
PPALDD_CONSTRUCTOR(PersonEventBase)
{
	InitFixData(rscDefHdr, &H, sizeof(H));
	Extra[0].Ptr = new PPObjPersonEvent;
}

PPALDD_DESTRUCTOR(PersonEventBase)
{
	Destroy();
	delete static_cast<PPObjPersonEvent *>(Extra[0].Ptr);
}

int PPALDD_PersonEventBase::InitData(PPFilt & rFilt, long rsrv)
{
	int    ok = -1;
	if(rFilt.ID == H.ID)
		ok = DlRtm::InitData(rFilt, rsrv);
	else {
		MEMSZERO(H);
		H.ID = rFilt.ID;
		PPObjPersonEvent * p_obj = static_cast<PPObjPersonEvent *>(Extra[0].Ptr);
		PersonEventTbl::Rec rec;
		if(p_obj && p_obj->Search(H.ID, &rec) > 0) {
			#define CPY_FLD(f) H.f = rec.f
			CPY_FLD(Dt);
			CPY_FLD(Tm);
			CPY_FLD(OprNo);
			CPY_FLD(OpID);
			CPY_FLD(PersonID);
			CPY_FLD(SecondID);
			H.LocID = rec.LocationID;
			CPY_FLD(LinkBillID);
			CPY_FLD(Extra);
			CPY_FLD(Flags);
			CPY_FLD(EstDuration);
			CPY_FLD(PrmrSCardID);
			CPY_FLD(ScndSCardID);
			#undef CPY_FLD
			{
				// @v11.1.12 STRNSCPY(H.Memo, rec.Memo);
				// @v11.1.12 {
				SString & r_temp_buf = SLS.AcquireRvlStr();
				p_obj->P_Tbl->GetItemMemo(H.ID, r_temp_buf);
				STRNSCPY(H.Memo, r_temp_buf);
				// } @v11.1.12 
			}
			ok = DlRtm::InitData(rFilt, rsrv);
		}
	}
	return ok;
}

void PPALDD_PersonEventBase::EvaluateFunc(const DlFunc * pF, SV_Uint32 * pApl, RtmStack & rS)
{
	#define _ARG_STR(n)  (**static_cast<const SString **>(rS.GetPtr(pApl->Get(n))))
	#define _ARG_INT(n)  (*static_cast<const int *>(rS.GetPtr(pApl->Get(n))))
	#define _RET_INT     (*static_cast<int *>(rS.GetPtr(pApl->Get(0))))
	#define _RET_LONG    (*static_cast<long *>(rS.GetPtr(pApl->Get(0))))
	#define _RET_DBL     (*static_cast<double *>(rS.GetPtr(pApl->Get(0)))
	#define _RET_STR     (**static_cast<SString **>(rS.GetPtr(pApl->Get(0))))
	PPObjPersonEvent * p_pe_obj = static_cast<PPObjPersonEvent *>(Extra[0].Ptr);
	if(pF->Name == "?GetPair") {
		PPID   pair_id = 0;
		if(p_pe_obj) {
			PersonEventTbl::Rec pair_rec;
			if(p_pe_obj->SearchPairEvent(H.ID, _ARG_INT(1), 0, &pair_rec) > 0)
				pair_id = pair_rec.ID;
		}
		_RET_LONG = pair_id;
	}
	else if(pF->Name == "?GetPairDuration") {
		long   dur = 0;
		if(p_pe_obj) {
			PPID   pair_id = 0;
			PersonEventTbl::Rec rec, pair_rec;
			if(p_pe_obj->SearchPairEvent(H.ID, _ARG_INT(1), &rec, &pair_rec) > 0) {
				LDATETIME dtm1, dtm2;
				dtm1.Set(rec.Dt, rec.Tm);
				dtm2.Set(pair_rec.Dt, pair_rec.Tm);
				dur = diffdatetimesec(dtm1, dtm2);
			}
		}
		_RET_LONG = dur;
	}
}
//
// Implementation of PPALDD_PersonEvent
//
PPALDD_CONSTRUCTOR(PersonEvent)
{
	if(Valid) {
		AssignHeadData(&H, sizeof(H));
		AssignDefIterData(&I, sizeof(I));
	}
}

PPALDD_DESTRUCTOR(PersonEvent) { Destroy(); }

int PPALDD_PersonEvent::InitData(PPFilt & rFilt, long rsrv)
{
	INIT_PPVIEW_ALDD_DATA_U(PersonEvent, rsrv);
	if(p_filt) {
		H.FltBeg = p_filt->Period.low;
		H.FltEnd = p_filt->Period.upp;
		H.FltPrmrID = p_filt->PrmrID;
		H.FltScndID = p_filt->ScndID;
		H.FltPsnEvSubst = p_filt->Sgpe;
		H.FltDateSubst = p_filt->Sgd;
	}
	return DlRtm::InitData(rFilt, rsrv);
}

int PPALDD_PersonEvent::InitIteration(PPIterID iterId, int sortId, long /*rsrv*/)
{
	INIT_PPVIEW_ALDD_ITER(PersonEvent);
}

int PPALDD_PersonEvent::NextIteration(PPIterID iterId)
{
	START_PPVIEW_ALDD_ITER(PersonEvent);
	//const PersonEventFilt * p_filt = (const PersonEventFilt *)p_v->GetBaseFilt();
	PPWaitPercent(p_v->GetCounter());
	MEMSZERO(I);
	I.EvID = item.ID;
	item.GrpText1.CopyTo(I.GrpText1, sizeof(I.GrpText1));
	item.GrpText2.CopyTo(I.GrpText2, sizeof(I.GrpText2));
	memzero(I.AvgEvTime, sizeof(I.AvgEvTime));
	item.AvgEvTime.CopyTo(I.AvgEvTime, sizeof(I.AvgEvTime));
	I.GrpCount = item.GrpCount;
	FINISH_PPVIEW_ALDD_ITER();
}

void PPALDD_PersonEvent::Destroy() { DESTROY_PPVIEW_ALDD(PersonEvent); }
//
// Implementation of PPALDD_PsnOpKindView
//
struct ALD_PsnOpKindBlock {
	ALD_PsnOpKindBlock() : P_En(0)
	{
	}
	~ALD_PsnOpKindBlock()
	{
		delete P_En;
	}
	PPObjPsnOpKind Obj;
	SEnum * P_En;
};

PPALDD_CONSTRUCTOR(PsnOpKindView)
{
	if(Valid) {
		AssignHeadData(&H, sizeof(H));
		AssignDefIterData(&I, sizeof(I));
	}
}

PPALDD_DESTRUCTOR(PsnOpKindView) { Destroy(); }

int PPALDD_PsnOpKindView::InitData(PPFilt & rFilt, long rsrv)
{
	Extra[0].Ptr = new ALD_PsnOpKindBlock;
	return DlRtm::InitData(rFilt, rsrv);
}

int PPALDD_PsnOpKindView::InitIteration(PPIterID iterId, int /*sortId*/, long /*rsrv*/)
{
	IterProlog(iterId, 1);
	ALD_PsnOpKindBlock * p_blk = static_cast<ALD_PsnOpKindBlock *>(Extra[0].Ptr);
	if(p_blk) {
		ZDELETE(p_blk->P_En);
		p_blk->P_En = new SEnum(PPRef->Enum(PPOBJ_PERSONOPKIND, 0));
	}
	return BIN(p_blk && p_blk->P_En);
}

int PPALDD_PsnOpKindView::NextIteration(PPIterID iterId)
{
	int    ok = -1;
	IterProlog(iterId, 0);
	ALD_PsnOpKindBlock * p_blk = static_cast<ALD_PsnOpKindBlock *>(Extra[0].Ptr);
	PPPsnOpKind item;
	if(p_blk && p_blk->P_En && p_blk->P_En->Next(&item) > 0) {
		I.PsnOpKindID = item.ID;
		I.Level       = p_blk->Obj.GetLevel(item.ID);
		ok = DlRtm::NextIteration(iterId);
	}
	return ok;
}

void PPALDD_PsnOpKindView::Destroy()
{
	delete static_cast<ALD_PsnOpKindBlock *>(Extra[0].Ptr);
	Extra[0].Ptr = 0;
	Extra[1].Ptr = 0;
}
