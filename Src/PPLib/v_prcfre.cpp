// V_PRCFRE.CPP
// Copyright (c) A.Sobolev 2006, 2007, 2008, 2011, 2013, 2014, 2015, 2016, 2017
//
#include <pp.h>
#pragma hdrstop
//
// @ModuleDef(PPViewPrcBusy)
//
IMPLEMENT_PPFILT_FACTORY(PrcBusy); SLAPI PrcBusyFilt::PrcBusyFilt() : PPBaseFilt(PPFILT_PRCBUSY, 0, 0)
{
	SetFlatChunk(offsetof(PrcBusyFilt, ReserveStart),
		offsetof(PrcBusyFilt, Reserve)-offsetof(PrcBusyFilt, ReserveStart)+sizeof(Reserve));
	Init(1, 0);
}

SLAPI PPViewPrcBusy::PPViewPrcBusy() : PPView(0, &Filt, PPVIEW_PRCBUSY)
{
	ImplementFlags |= implChangeFilt;
	P_TempTbl = 0;
	Grid.P_View = this;
}

SLAPI PPViewPrcBusy::~PPViewPrcBusy()
{
	UpdateTimeBrowser(1);
	delete P_TempTbl;
}

PPBaseFilt * PPViewPrcBusy::CreateFilt(void * extraPtr) const
{
	PrcBusyFilt * p_filt = new PrcBusyFilt;
	if(p_filt) {
		p_filt->Flags |= PrcBusyFilt::fFree;
		p_filt->Period.Start.d = LConfig.OperDate;
	}
	return p_filt;
}
//
//
//
#define GRP_PRC 1

class PrcBusyFiltDialog : public TDialog {
public:
	PrcBusyFiltDialog() : TDialog(DLG_PRCBUSYFLT)
	{
		SetupCalDate(CTLCAL_PRCBUSYFLT_STDT, CTL_PRCBUSYFLT_STDT);
		SetupTimePicker(this, CTL_PRCBUSYFLT_STTM, CTLTM_PRCBUSYFLT_STTM); // @v6.8.2
		SetupCalDate(CTLCAL_PRCBUSYFLT_FNDT, CTL_PRCBUSYFLT_FNDT);
		SetupTimePicker(this, CTL_PRCBUSYFLT_FNTM, CTLTM_PRCBUSYFLT_FNTM); // @v6.8.2
		addGroup(GRP_PRC, new PrcCtrlGroup(CTLSEL_PRCBUSYFLT_PRC));
	}
	int    setDTS(const PrcBusyFilt *);
	int    getDTS(PrcBusyFilt *);
private:
	PrcBusyFilt Data;
};

int PrcBusyFiltDialog::setDTS(const PrcBusyFilt * pData)
{
	int    ok = 1;
	Data = *pData;
	PrcCtrlGroup::Rec prc_grp_rec(Data.PrcID);
	setGroupData(GRP_PRC, &prc_grp_rec);
	setCtrlData(CTL_PRCBUSYFLT_STDT, &Data.Period.Start.d);
	setCtrlData(CTL_PRCBUSYFLT_STTM, &Data.Period.Start.t);
	if(Data.Period.Finish.IsFar())
		Data.Period.Finish.SetZero();
	setCtrlData(CTL_PRCBUSYFLT_FNDT,   &Data.Period.Finish.d);
	setCtrlData(CTL_PRCBUSYFLT_FNTM,   &Data.Period.Finish.t);
	setCtrlData(CTL_PRCBUSYFLT_MINDUR, &Data.MinDuration);
	AddClusterAssoc(CTL_PRCBUSYFLT_FLAGS, 0, PrcBusyFilt::fFree);
	AddClusterAssoc(CTL_PRCBUSYFLT_FLAGS, 1, PrcBusyFilt::fShowTimeGraph); // @v6.7.11
	SetClusterData(CTL_PRCBUSYFLT_FLAGS, Data.Flags);
	return ok;
}

int PrcBusyFiltDialog::getDTS(PrcBusyFilt * pData)
{
	int    ok = 1;
	uint   sel = 0;
	PrcCtrlGroup::Rec prc_grp_rec;
	getGroupData(GRP_PRC, &prc_grp_rec);
	Data.PrcID = prc_grp_rec.PrcID;
	getCtrlData(CTL_PRCBUSYFLT_STDT, &Data.Period.Start.d);
	getCtrlData(CTL_PRCBUSYFLT_STTM, &Data.Period.Start.t);
	getCtrlData(CTL_PRCBUSYFLT_FNDT, &Data.Period.Finish.d);
	getCtrlData(CTL_PRCBUSYFLT_FNTM, &Data.Period.Finish.t);
	getCtrlData(CTL_PRCBUSYFLT_MINDUR, &Data.MinDuration);
	GetClusterData(CTL_PRCBUSYFLT_FLAGS, &Data.Flags);
	ASSIGN_PTR(pData, Data);
	return ok;
}

int SLAPI PPViewPrcBusy::EditBaseFilt(PPBaseFilt * pFilt)
{
	if(!Filt.IsA(pFilt))
		return 0;
	PrcBusyFilt * p_filt = (PrcBusyFilt *)pFilt;
	DIALOG_PROC_BODY(PrcBusyFiltDialog, p_filt);
}

int SLAPI PPViewPrcBusy::ProcessPrc(PPID prcID, BExtInsert * pBei)
{
	int    ok = 1;
	STimeChunkAssocArray * p_tch_list = 0; // @test
	STimeChunkArray worktime_list;
	SString temp_buf;
	ProcessorTbl::Rec prc_rec;
	PrcBusyArray busy_list;
	THROW_PP(P_TempTbl, PPERR_PPVIEWNOTINITED);
	THROW_DB(deleteFrom(P_TempTbl, 0, P_TempTbl->PrcID == prcID));
	THROW(TSesObj.P_Tbl->LoadBusyArray(prcID, 0, TSESK_SESSION, &Filt.Period, &busy_list));
	//
	// Получаем список периодов рабочего времени
	//
	if(TSesObj.GetPrc(prcID, &prc_rec, 1, 1) > 0 && prc_rec.LinkObjType == PPOBJ_PERSON && prc_rec.LinkObjID) {
		PPID   reg_cal_id = SlObj.PsnObj.GetConfig().RegStaffCalID;
		if(reg_cal_id) {
			DateRange period;
			period.Set(Filt.Period.Start.d, Filt.Period.Finish.d);
			LDATE curdt = getcurdate_();
			SETIFZ(period.low, encodedate(1, curdt.month(), curdt.year()));
			if(!period.upp || period.upp > encodedate(31, 12, curdt.year()+1))
				period.upp = encodedate(curdt.dayspermonth(), curdt.month(), curdt.year());
			ScObjAssoc scoa;
			ScObj.InitScObjAssoc(reg_cal_id, 0, prc_rec.LinkObjID, &scoa);
			THROW(ScObj.CalcPeriod(scoa, period, 0, 0, 0, &worktime_list));
			worktime_list.Sort();
		}
	}
	{
		p_tch_list = new STimeChunkAssocArray(prcID);
		for(uint i = 0; i < busy_list.getCount(); i++) {
			const PrcBusy * p_item = (const PrcBusy *)busy_list.at(i);
			p_tch_list->Add(p_item->TSessID, p_item->Status, (const STimeChunk *)p_item, 0);
		}
		Grid.SetRow(p_tch_list, (TSesObj.GetPrc(prcID, &prc_rec, 1, 1) > 0) ? prc_rec.Name : 0);
		if(worktime_list.getCount()) {
			STimeChunkArray holiday_list;
			worktime_list.GetFreeList(&holiday_list);
			if(holiday_list.getCount())
				Grid.SetHolidayList(prcID, &holiday_list);
		}
	}
	if(Filt.Flags & PrcBusyFilt::fFree) {
		PrcBusyArray free_list;
		busy_list.GetFreeList(&free_list);
		free_list.Limit(&Filt.Period);
		busy_list.copy(free_list);
		busy_list.Sort(0);
		if(worktime_list.getCount()) {
			STimeChunkArray temp_list;
			//
			// Получаем инверсию списка рабочих периодов человека-процессора.
			//
			temp_list = worktime_list;
			temp_list.Sort();
			free_list.freeAll();
			for(uint i = 0; i < busy_list.getCount(); i++) {
				const PrcBusy * p_item1 = (const PrcBusy *)busy_list.at(i);
				for(uint j = 0; j < temp_list.getCount(); j++) {
					STimeChunk sect;
					if(p_item1->STimeChunk::Intersect(*(const STimeChunk *)temp_list.at(j), &sect) > 0) {
						PrcBusy temp_entry = *p_item1;
						temp_entry.Start  = sect.Start;
						temp_entry.Finish = sect.Finish;
						free_list.Add(temp_entry, 0);
					}
				}
			}
			free_list.Sort();
			busy_list = free_list;
		}
	}
	for(uint i = 0; i < busy_list.getCount(); i++) {
		const PrcBusy & entry = *(PrcBusy *)busy_list.at(i);
		TempPrcBusyTbl::Rec rec;
		MEMSZERO(rec);
		rec.PrcID = prcID;
		rec.StDt = entry.Start.d;
		rec.StTm = entry.Start.t;
		if(!entry.Finish.IsFar()) {
			rec.FnDt = entry.Finish.d;
			rec.FnTm = entry.Finish.t;
			rec.Duration = entry.GetDuration();
		}
		const long min_duration = Filt.MinDuration.totalsec();
		if(!rec.Duration || rec.Duration >= min_duration) {
			if(!(Filt.Flags & PrcBusyFilt::fFree))
				rec.TSessID = entry.TSessID;
			temp_buf = 0;
			if(rec.Duration) {
				int    days = rec.Duration / (24 * 3600);
				if(days)
					temp_buf.Cat(days).CatChar('d').Space();
				if(rec.Duration % (24 * 3600)) {
					LTIME  t;
					t.settotalsec(rec.Duration % (24 * 3600));
					temp_buf.Cat(t);
				}
			}
			else
				temp_buf.CatCharN('#', 3);
			temp_buf.CopyTo(rec.TxtDuration, sizeof(rec.TxtDuration));
			entry.ToStr(temp_buf = 0).CopyTo(rec.TxtPeriod, sizeof(rec.TxtPeriod));
			if(pBei) {
				THROW_DB(pBei->insert(&rec));
			}
			else {
				P_TempTbl->copyBufFrom(&rec);
				THROW_DB(P_TempTbl->insertRec());
			}
		}
	}
	CATCHZOK
	return ok;
}

PP_CREATE_TEMP_FILE_PROC(CreateTempFile, TempPrcBusy);

int SLAPI PPViewPrcBusy::Update(const PPIDArray & rPrcList)
{
	int    ok = 1;
	if(P_TempTbl && rPrcList.getCount()) {
		PPObjProcessor prc_obj;
		ProcessorTbl::Rec prc_rec;
		BExtInsert bei(P_TempTbl);
		PPTransaction tra(ppDbDependTransaction, 1);
		THROW(tra);
		for(uint i = 0; i < rPrcList.getCount(); i++) {
			const PPID prc_id = rPrcList.at(i);
			if(prc_obj.Fetch(prc_id, &prc_rec) > 0 && prc_rec.Kind == PPPRCK_PROCESSOR && !(prc_rec.Flags & PRCF_PASSIVE)) {
				PPWaitMsg(prc_rec.Name); // @v7.0.6
				THROW(ProcessPrc(prc_id, &bei));
			}
		}
		THROW_DB(bei.flash());
		THROW(tra.Commit());
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

int SLAPI PPViewPrcBusy::Init_(const PPBaseFilt * pFilt)
{
	int    ok = 1;
	Grid.freeAll();
	PPObjProcessor prc_obj;
	ProcessorTbl::Rec prc_rec;
	THROW(Helper_InitBaseFilt(pFilt));
	Filt.Period.Start.d = Filt.Period.Start.d.getactual(ZERODATE);   // @v6.9.4
	Filt.Period.Finish.d = Filt.Period.Finish.d.getactual(ZERODATE); // @v6.9.4
	if(!Filt.Period.Finish.d) {
		LDATE cur = getcurdate_();
		Filt.Period.Finish.d = plusdate(MAX(cur, Filt.Period.Start.d), 30);
		//Filt.Period.Finish.SetFar();
	}
	ZDELETE(P_TempTbl);
	THROW(P_TempTbl = CreateTempFile());
	PrcList.freeAll();
	if(prc_obj.Fetch(Filt.PrcID, &prc_rec) > 0) {
		if(prc_rec.Kind == PPPRCK_GROUP) {
			THROW(prc_obj.GetChildIDList(Filt.PrcID, 1, &PrcList));
		}
		else
			PrcList.add(Filt.PrcID);
	}
	else
		THROW(prc_obj.GetChildIDList(Filt.PrcID, 1, &PrcList));
	THROW(Update(PrcList));
	CATCHZOK
	return ok;
}

int SLAPI PPViewPrcBusy::InitIteration()
{
	int    ok = 1;
	char   k[MAXKEYLEN];
	ZDELETE(P_IterQuery);
	if(P_TempTbl) {
		Counter.Init(P_TempTbl);
		P_IterQuery = new BExtQuery(P_TempTbl, 1);
		memzero(k, sizeof(k));
		P_IterQuery->selectAll();
		P_IterQuery->initIteration(0, k, spFirst);
	}
	else
		ok = PPSetError(PPERR_PPVIEWNOTINITED);
	return ok;
}

void SLAPI PPViewPrcBusy::RecToViewItem(const TempPrcBusyTbl::Rec * pRec, PrcBusyViewItem * pItem) const
{
	PrcBusyViewItem item;
	MEMSZERO(item);
	if(pRec) {
		item.PrcID = pRec->PrcID;
		item.Period.Start.Set(pRec->StDt, pRec->StTm);
		item.Period.Finish.Set(pRec->FnDt, pRec->FnTm);
		item.TSessID  = pRec->TSessID;
		item.Duration = pRec->Duration;
		STRNSCPY(item.TxtPeriod, pRec->TxtPeriod);
		STRNSCPY(item.TxtDuration, pRec->TxtDuration);
	}
	ASSIGN_PTR(pItem, item);
}

int FASTCALL PPViewPrcBusy::NextIteration(PrcBusyViewItem * pItem)
{
	if(P_IterQuery && P_IterQuery->nextIteration() > 0) {
		RecToViewItem(&P_TempTbl->data, pItem);
		return 1;
	}
	return -1;
}

int SLAPI PPViewPrcBusy::AddSession(PrcBusyViewItem * pItem)
{
	int    ok = -1;
	PPID   id = 0;
	PPID   prc_id = 0;
	if(pItem && (Filt.Flags & PrcBusyFilt::fFree) && pItem->PrcID)
		prc_id = pItem->PrcID;
	TSessionPacket pack;
	THROW(TSesObj.InitPacket(&pack, TSESK_SESSION, prc_id, 0, 0));
	if(pItem && (Filt.Flags & PrcBusyFilt::fFree) && pItem->PrcID) {
		pack.Rec.StDt  = pItem->Period.Start.d;
		pack.Rec.StTm  = pItem->Period.Start.t;
		pack.Rec.FinDt = pItem->Period.Finish.d;
		pack.Rec.FinTm = pItem->Period.Finish.t;
	}
	while(ok < 0 && TSesObj.EditDialog(&pack) > 0) {
		id = NZOR(pack.Rec.ID, id); // Во время редактирования нулевой идентификатор мог стать действительным
		if(TSesObj.PutPacket(&id, &pack, 1)) {
			ProcessPrc(pack.Rec.PrcID, 0);
			ok = 1;
		}
		else
			PPError();
	}
	CATCHZOKPPERR
	return ok;
}

int SLAPI PPViewPrcBusy::ViewTSessLines(PPID sessID)
{
	int    ok = -1;
	if(sessID) {
		TSessLineFilt filt(sessID);
		ViewTSessLine(&filt);
		ok = 1;
	}
	return ok;
}

int SLAPI PPViewPrcBusy::Print(const void * pHdr)
{
	PPID   __id = pHdr ? *(PPID *)pHdr : 0;
	PrcBusyViewItem item;
	if(GetItem(__id, &item) > 0 && item.TSessID) {
		PPFilt pf;
		pf.Ptr = 0;
		pf.ID  = item.TSessID;
		PPAlddPrint(REPORT_TSESSION, &pf);
	}
	return -1;
}

//virtual
int SLAPI PPViewPrcBusy::PreprocessBrowser(PPViewBrowser * pBrw)
{
	CALLPTRMEMB(pBrw, Advise(PPAdviseBlock::evTSessChanged, 0, PPOBJ_TSESSION, 0));
	return 1;
}

int SLAPI PPViewPrcBusy::HandleNotifyEvent(int kind, const PPNotifyEvent * pEv, PPViewBrowser * pBrw, void * extraProcPtr)
{
	int    ok = -1;
	if(pEv && P_TempTbl) {
		if(kind == PPAdviseBlock::evTSessChanged) {
			if(pEv->IsFinish()) {
				if(UpdatePrcList.getCount()) {
					ok = Update(UpdatePrcList);
					if(ok > 0) {
						pBrw->refresh(); // ??? pBrw->Update()
						UpdateTimeBrowser(0);
					}
					UpdatePrcList.clear();
				}
			}
			else if(pEv->ObjType == PPOBJ_TSESSION) {
				PPIDArray prc_list;
				TSessionTbl::Rec tses_rec;
				if(pEv->Action == PPACN_OBJRMV) {
					UpdatePrcList.addUnique(&PrcList);
				}
				else if(TSesObj.Search(pEv->ObjID, &tses_rec) > 0) {
					if(PrcList.lsearch(tses_rec.PrcID))
						UpdatePrcList.addUnique(tses_rec.PrcID);
					if(pEv->Action == PPACN_UPDTSESSPRC) {
						const PPID prev_prc_id = pEv->ExtInt_;
						if(prev_prc_id && PrcList.lsearch(prev_prc_id))
							UpdatePrcList.addUnique(prev_prc_id);
					}
				}
			}
		}
	}
	return ok;
}

DBQuery * SLAPI PPViewPrcBusy::CreateBrowserQuery(uint * pBrwId, SString * pSubTitle)
{
	static DbqStringSubst status_subst(5); // @global @threadsafe

	uint   brw_id = (Filt.Flags & PrcBusyFilt::fFree) ? BROWSER_PRCFREE : BROWSER_PRCBUSY;
	DBQ  * dbq = 0;
	DBE  * dbe_status = 0;
	DBE    dbe_prc, dbe_ar;
	TempPrcBusyTbl * p_tt = 0;
	TSessionTbl * p_st = 0;
	DBQuery * p_q = 0;
	if(P_TempTbl) {
		THROW(CheckTblPtr(p_tt = new TempPrcBusyTbl(P_TempTbl->fileName)));
		THROW(CheckTblPtr(p_st = new TSessionTbl));
		PPDbqFuncPool::InitObjNameFunc(dbe_prc, PPDbqFuncPool::IdObjNamePrc, p_tt->PrcID);
		PPDbqFuncPool::InitObjNameFunc(dbe_ar,  PPDbqFuncPool::IdObjNameAr,  p_st->ArID);
		dbe_status = & enumtoa(p_st->Status, 5, status_subst.Get(PPTXT_TSESS_STATUS));
		p_q = & select(
			p_tt->ID__,          // #0
			dbe_prc,             // #1
			p_tt->TxtPeriod,     // #2
			p_tt->TxtDuration,   // #3
			p_st->Num,           // #4
			p_st->Amount,        // #5
			*dbe_status,         // #6
			p_st->Incomplete,    // #7
			dbe_ar,              // #8
			0L).from(p_tt, p_st, 0L).where(p_st->ID += p_tt->TSessID);
		p_q->orderBy(/*p_tt->PrcID*/p_tt->StDt, p_tt->StTm, 0L);
		delete dbe_status;
	}
	THROW(CheckQueryPtr(p_q));
	if(pSubTitle) {
		*pSubTitle = 0;
		if(Filt.PrcID) {
			ProcessorTbl::Rec prc_rec;
			if(TSesObj.GetPrc(Filt.PrcID, &prc_rec, 0, 1) > 0)
				pSubTitle->Cat(prc_rec.Name);
		}
	}
	CATCH
		if(p_q)
			ZDELETE(p_q);
		else {
			delete p_tt;
			delete p_st;
		}
	ENDCATCH
	ASSIGN_PTR(pBrwId, brw_id);
	return p_q;
}

int SLAPI PPViewPrcBusy::OnExecBrowser(PPViewBrowser * pBrw)
{
	pBrw->SetupToolbarCombo(PPOBJ_PROCESSOR, Filt.PrcID, OLW_CANSELUPLEVEL, 0);
	if(Filt.Flags & PrcBusyFilt::fShowTimeGraph) {
		TimeChunkBrowser();
	}
	return -1;
}

int SLAPI PPViewPrcBusy::GetItem(PPID id, PrcBusyViewItem * pItem)
{
	int    ok = 0;
	if(P_TempTbl) {
		TempPrcBusyTbl::Rec rec;
		if(SearchByID(P_TempTbl, 0, id, &rec) > 0) {
			RecToViewItem(&rec, pItem);
			ok = 1;
		}
	}
	return ok;
}

int SLAPI PPViewPrcBusy::Detail(const void * pHdr, PPViewBrowser * pBrw)
{
	int    ok = -1;
	PPID   id = pHdr ? *(PPID *)pHdr : 0;
	PrcBusyViewItem item;
	if(GetItem(id, &item) > 0 && item.TSessID) {
		if(TSesObj.Edit(&item.TSessID, 0) == cmOK) {
			ProcessPrc(item.PrcID, 0);
			ok = 1;
		}
	}
	return ok;
}

int SLAPI PPViewPrcBusy::EditTimeGridItem(PPID * pID, PPID rowID, const LDATETIME & rDtm)
{
	int    ok = -1;
	if(*pID) {
		//
		// Редактировать элемент
		//
		TSessionTbl::Rec ses_rec;
		if(TSesObj.Search(*pID, &ses_rec) > 0) {
			PPID old_prc_id = ses_rec.PrcID;
			if(TSesObj.Edit(pID, 0) == cmOK) {
				PPID new_prc_id = 0;
				if(TSesObj.Search(*pID, &ses_rec) > 0)
					new_prc_id = ses_rec.PrcID;
				THROW(ProcessPrc(old_prc_id, 0));
				if(new_prc_id && new_prc_id != old_prc_id)
					THROW(ProcessPrc(new_prc_id, 0));
				ok = 1;
			}
		}
	}
	else if(rDtm.d) {
		//
		// Создать элемент
		//
		int    r = 0;
		TSessionPacket pack;
		PPID   prc_id = NZOR(rowID, PrcList.getSingle());
		THROW(TSesObj.InitPacket(&pack, TSESK_SESSION, prc_id, 0, 0));
		pack.Rec.StDt = rDtm.d;
		pack.Rec.StTm = rDtm.t;
		while(ok < 0 && (r = TSesObj.EditDialog(&pack)) > 0) {
			*pID = NZOR(pack.Rec.ID, *pID); // Во время редактирования нулевой идентификатор мог стать действительным
			if(r == 1) {
				if(TSesObj.PutPacket(pID, &pack, 1)) {
					THROW(ProcessPrc(pack.Rec.PrcID, 0));
					ok = 1;
				}
				else
					PPError();
			}
			else
				ok = 1;
		}
	}
	else {
		//
		// Редактировать процессор
		//
		if(rowID) {
			PPObjProcessor prc_obj;
			if(prc_obj.Edit(&rowID, 0) == cmOK) {
				THROW(ProcessPrc(rowID, 0));
				ok = 1;
			}
		}
	}
	CATCHZOKPPERR
	return ok;
}

int SLAPI PPViewPrcBusy::UpdateTimeGridItem(PPID sessID, PPID prcID, const STimeChunk & rNewChunk)
{
	int    ok = -1;
	PPID   prev_prc_id = 0;
	TSessionTbl::Rec ses_rec;
	THROW(TSesObj.Search(sessID, &ses_rec) > 0);
	prev_prc_id = ses_rec.PrcID;
	ses_rec.StDt  = rNewChunk.Start.d;
	ses_rec.StTm  = rNewChunk.Start.t;
	ses_rec.FinDt = rNewChunk.Finish.d;
	ses_rec.FinTm = rNewChunk.Finish.t;
	if(oneof3(ses_rec.Status, TSESST_PLANNED, TSESST_PENDING, TSESST_INPROCESS))
		ses_rec.PlannedTiming = rNewChunk.GetDuration();
	THROW(TSesObj.CheckNewPrc(&ses_rec, prcID) > 0);
	ses_rec.PrcID = prcID;
	THROW(TSesObj.CheckSessionTime(ses_rec));
	THROW(TSesObj.CheckSuperSessLink(&ses_rec, ses_rec.ParentID, 0));
	THROW(TSesObj.PutRec(&sessID, &ses_rec, 1));
	THROW(ProcessPrc(ses_rec.PrcID, 0));
	if(prev_prc_id && prev_prc_id != ses_rec.PrcID)
		THROW(ProcessPrc(prev_prc_id, 0));
	ok = 1;
	CATCHZOKPPERR
	return ok;
}

int SLAPI PPViewPrcBusy::ProcessCommand(uint ppvCmd, const void * pHdr, PPViewBrowser * pBrw)
{
	int    ok = PPView::ProcessCommand(ppvCmd, pHdr, pBrw);
	if(ok == -2) {
		PPID   __id = pHdr ? *(PPID *)pHdr : 0;
		PrcBusyViewItem item;
		switch(ppvCmd) {
			case PPVCMD_ADDITEM:
				ok = (GetItem(__id, &item) > 0) ? AddSession(&item) : AddSession(0);
				if(ok > 0)
					UpdateTimeBrowser(0);
				break;
			case PPVCMD_DELETEITEM:
				ok = -1;
				if(GetItem(__id, &item) > 0 && item.TSessID) {
					ok = TSesObj.RemoveObjV(item.TSessID, 0, PPObject::rmv_default, 0) ? 1 : 0;
					if(ok > 0) {
						ProcessPrc(item.PrcID, 0);
						UpdateTimeBrowser(0);
					}
				}
				break;
			case PPVCMD_EDITPRC:
				ok = -1;
				if(GetItem(__id, &item) > 0 && item.PrcID) {
					PPObjProcessor prc_obj;
					if(prc_obj.Edit(&item.PrcID, 0) == cmOK) {
						ProcessPrc(item.PrcID, 0);
						UpdateTimeBrowser(0);
						ok = 1;
					}
				}
				break;
			case PPVCMD_VIEWTSESS:
				ok = -1;
				if(GetItem(__id, &item) > 0 && item.PrcID) {
					TSessionFilt filt;
					filt.PrcID = item.PrcID;
					::ViewTSession(&filt);
				}
				break;
			case PPVCMD_VIEWTSESSLINES:
				ok = -1;
				if(GetItem(__id, &item) > 0 && item.TSessID)
					ok = ViewTSessLines(item.TSessID);
				break;
			case PPVCMD_WROFFSESS:
				ok = -1;
				if(GetItem(__id, &item) > 0 && item.TSessID) {
					PUGL   pugl;
					PPIDArray sess_list;
					sess_list.add(item.TSessID);
					int    r = TSesObj.WriteOff(&sess_list, &pugl, 1);
					if(r > 0)
						ok = 1;
					else if(r == -2) {
						pugl.ClearActions();
						pugl.OPcug = PCUG_CANCEL;
						ProcessUnsuffisientList(DLG_MSGNCMPL4, &pugl);
					}
					else if(r == 0)
						ok = PPErrorZ();
				}
				break;
			case PPVCMD_WROFFBILLS:
				ok = -1;
				if(GetItem(__id, &item) > 0 && item.TSessID)
					ViewBillsByPool(PPASS_TSESSBILLPOOL, item.TSessID);
				break;
			case PPVCMD_DFCTBILLS:
				ok = -1;
				if(GetItem(__id, &item) > 0 && item.TSessID)
					ViewBillsByPool(PPASS_TSDBILLPOOL, item.TSessID);
				break;
			case PPVCMD_CHECKPAN:
				ok = -1;
				if(GetItem(__id, &item) > 0 && item.TSessID)
					TSesObj.CallCheckPaneBySess(item.TSessID);
				break;
			case PPVCMD_TB_CBX_SELECTED:
				{
					ok = -1;
					PPID   prc_id = 0;
					if(pBrw && pBrw->GetToolbarComboData(&prc_id) && Filt.PrcID != prc_id) {
						Filt.PrcID = prc_id;
						ok = ChangeFilt(1, pBrw);
						if(ok > 0) {
							UpdateTimeBrowser(0);
						}
					}
				}
				break;
			case PPVCMD_REFRESH:
				ok = -1;
				if(Update(PrcList) > 0) {
					ok = 1;
					//pBrw->refresh(); // ??? pBrw->Update()
				}
				break;
			case PPVCMD_CHANGEFILT:
				{
					STimeChunkBrowser * p_brw = PPFindLastTimeChunkBrowser();
					ok = ChangeFilt(0, pBrw);
					if(p_brw)
						if(ok == 2) {
							PPCloseBrowser(p_brw);
							TimeChunkBrowser();
							::SetFocus(pBrw->H());
						}
						else if(ok > 0)
							p_brw->UpdateData();
				}
				break;
			case PPVCMD_TOGGLE:
				{
					INVERSEFLAG(Filt.Flags, PrcBusyFilt::fFree);
					STimeChunkBrowser * p_brw = PPFindLastTimeChunkBrowser();
					ok = ChangeFilt(1, pBrw);
					if(p_brw)
						if(ok == 2) {
							PPCloseBrowser(p_brw);
							TimeChunkBrowser();
							::SetFocus(pBrw->H());
						}
						else if(ok > 0)
							p_brw->UpdateData();
				}
				break;
			case PPVCMD_TIMEGRAPH:
				ok = -1;
				TimeChunkBrowser();
				break;
		}
	}
	return ok;
}
//
// Implementation of PPALDD_PrcBusyView
//
PPALDD_CONSTRUCTOR(PrcBusyView)
{
	if(Valid) {
		AssignHeadData(&H, sizeof(H));
		AssignIterData(1, &I, sizeof(I));
	}
}

PPALDD_DESTRUCTOR(PrcBusyView)
{
	Destroy();
}

int PPALDD_PrcBusyView::InitData(PPFilt & rFilt, long rsrv)
{
	INIT_PPVIEW_ALDD_DATA_U(PrcBusy, rsrv);
	H.FltPrcID = p_filt->PrcID;
	H.FltStDt  = p_filt->Period.Start.d;
	H.FltStTm  = p_filt->Period.Start.t;
	H.FltFnDt  = p_filt->Period.Finish.d;
	H.FltFnTm  = p_filt->Period.Finish.t;
	H.FltMinDuration = p_filt->MinDuration;
	H.FltFlags = p_filt->Flags;
	H.fFree    = BIN(p_filt->Flags & PrcBusyFilt::fFree);
	return DlRtm::InitData(rFilt, rsrv);
}

int PPALDD_PrcBusyView::InitIteration(PPIterID iterId, int sortId, long rsrv)
{
	INIT_PPVIEW_ALDD_ITER(PrcBusy);
}

int PPALDD_PrcBusyView::NextIteration(PPIterID iterId)
{
	START_PPVIEW_ALDD_ITER(PrcBusy);
	I.PrcID = item.PrcID;
	I.StDt  = item.Period.Start.d;
	I.StTm  = item.Period.Start.t;
	I.FnDt  = item.Period.Finish.d;
	I.FnTm  = item.Period.Finish.t;
	I.Duration = item.Duration;
	FINISH_PPVIEW_ALDD_ITER();
}

void PPALDD_PrcBusyView::Destroy()
{
	DESTROY_PPVIEW_ALDD(PrcBusy);
}
//
// STimeChunkBrowser {
//
SLAPI PPViewPrcBusy::PrcBusyTimeChunkGrid::PrcBusyTimeChunkGrid() : STimeChunkGrid()
{
}

SLAPI PPViewPrcBusy::PrcBusyTimeChunkGrid::~PrcBusyTimeChunkGrid()
{
}

int PPViewPrcBusy::PrcBusyTimeChunkGrid::GetText(int item, long id, SString & rBuf)
{
	int    ok = -1;
	SString temp_buf;
	TSessionTbl::Rec ses_rec;
	PPCheckInPersonMngr cip_mgr;
	rBuf = 0;
	if(item == iTitle) {
		rBuf = "TEST";
		ok = 1;
	}
	else if(item == iRow) {
		if(id < 0)
			ok = 1;
		else
			ok = STimeChunkGrid::GetText(item, id, rBuf);
	}
	else if(item == iChunk) {
		if(P_View->TSesObj.Search(id, &ses_rec) > 0) {
			TechTbl::Rec tec_rec;
			// @v8.1.8 Отображение статьи теперь имеет приоритет перед товаром
			if(ses_rec.ArID)
				GetArticleName(ses_rec.ArID, temp_buf);
			else if(ses_rec.TechID && P_View->TSesObj.GetTech(ses_rec.TechID, &tec_rec, 1) > 0 && tec_rec.GoodsID)
				GetGoodsName(tec_rec.GoodsID, temp_buf);
			else if(ses_rec.Ar2ID)
				GetArticleName(ses_rec.Ar2ID, temp_buf);
			rBuf.Cat(temp_buf);
			{
				ProcessorTbl::Rec prc_rec;
				MEMSZERO(prc_rec);
				if(P_View->TSesObj.GetPrc(ses_rec.PrcID, &prc_rec, 1, 1) > 0) {
					if(prc_rec.Flags & PRCF_ALLOWCIP) {
						PPCheckInPersonArray cip_list;
						cip_mgr.GetList(PPCheckInPersonItem::kTSession, id, cip_list);
						uint   reg_count = 0, ci_count = 0, canceled_count = 0;
						cip_list.Count(&reg_count, &ci_count, &canceled_count);
						if(reg_count) {
							PPLoadString("registered", temp_buf);
							rBuf.CatDivIfNotEmpty('\n', 0).Cat(temp_buf).CatDiv(':', 2);
							(temp_buf = 0).Cat((long)reg_count);
							if(ci_count || canceled_count)
								temp_buf.CatChar('/').Cat((long)ci_count);
								if(canceled_count)
									temp_buf.CatChar('/').Cat((long)canceled_count);
							rBuf.Cat(temp_buf);
						}
					}
				}
			}
			if(ses_rec.Memo[0] != 0)
				rBuf.CatDivIfNotEmpty('\n', 0).Cat(ses_rec.Memo);
			ok = 1;
		}
	}
	else if(item == iChunkBallon) {
		if(P_View->TSesObj.Search(id, &ses_rec) > 0) {
			// процессор, основной товар, время начала и окончания, контрагент, примечание
			TechTbl::Rec tec_rec;
			ProcessorTbl::Rec prc_rec;
			MEMSZERO(prc_rec);
			if(P_View->TSesObj.GetPrc(ses_rec.PrcID, &prc_rec, 1, 1) > 0) {
				rBuf.CatDivIfNotEmpty('\n', 0).Cat(prc_rec.Name);
			}
			if(ses_rec.TechID && P_View->TSesObj.GetTech(ses_rec.TechID, &tec_rec, 1) > 0 && tec_rec.GoodsID) {
				rBuf.CatDivIfNotEmpty('\n', 0).Cat(GetGoodsName(tec_rec.GoodsID, temp_buf));
			}
			STimeChunkAssoc tca;
			if(GetChunk(id, 0, &tca) > 0) {
				tca.Chunk.ToStr(temp_buf = 0, STimeChunk::fmtOmitSec);
				rBuf.CatDivIfNotEmpty('\n', 0).Cat(temp_buf);
			}
			{
				PPObjArticle ar_obj;
				ArticleTbl::Rec ar_rec;
				ar_rec.ID = 0;
				if(ses_rec.ArID) {
					if(ar_obj.Fetch(ses_rec.ArID, &ar_rec) > 0) {
						temp_buf = ar_rec.Name;
					}
				}
				else if(ses_rec.Ar2ID) {
					if(ar_obj.Fetch(ses_rec.Ar2ID, &ar_rec) > 0) {
						temp_buf = ar_rec.Name;
					}
				}
				else
					temp_buf = 0;
				if(temp_buf.NotEmptyS()) {
					rBuf.CatDivIfNotEmpty('\n', 0).Cat(temp_buf);

					const PPID psn_id = ObjectToPerson(ar_rec.ID, 0);
					if(psn_id) {
						PPELinkArray elnk_list;
						PersonCore::GetELinks(psn_id, &elnk_list);
						if(elnk_list.GetSinglePhone(temp_buf = 0, 0) > 0)
							rBuf.Space().Cat(temp_buf);
					}
				}
			}
			{
				if(prc_rec.Flags & PRCF_ALLOWCIP) {
					PPCheckInPersonArray cip_list;
					cip_mgr.GetList(PPCheckInPersonItem::kTSession, id, cip_list);
					uint   reg_count = 0, ci_count = 0, canceled_count = 0;
					cip_list.Count(&reg_count, &ci_count, &canceled_count);
					if(reg_count) {
						PPLoadString("registered", temp_buf);
						rBuf.CatDivIfNotEmpty('\n', 0).Cat(temp_buf).CatDiv(':', 2);
						(temp_buf = 0).Cat((long)reg_count);
						if(ci_count || canceled_count)
							temp_buf.CatChar('/').Cat((long)ci_count);
							if(canceled_count)
								temp_buf.CatChar('/').Cat((long)canceled_count);
						rBuf.Cat(temp_buf);
					}
				}
			}
			if(ses_rec.Memo[0] != 0)
				rBuf.CatDivIfNotEmpty('\n', 0).Cat(ses_rec.Memo);
			if(rBuf.NotEmptyS())
				ok = 1;
		}
	}
	return ok;
}

int PPViewPrcBusy::PrcBusyTimeChunkGrid::GetColor(long id, STimeChunkGrid::Color * pClr)
{
	int    ok = 1;
	TSessionTbl::Rec ses_rec;
	const PPTSessConfig & r_cfg = P_View->TSesObj.GetConfig();
	if(P_View->TSesObj.Search(id, &ses_rec) > 0) {
		pClr->Status = ses_rec.Status;
		switch(ses_rec.Status) {
			case TSESST_PLANNED:   pClr->C = r_cfg.ColorPlannedStatus;    break;
			case TSESST_PENDING:   pClr->C = r_cfg.ColorPendingStatus;    break;
			case TSESST_INPROCESS: pClr->C = r_cfg.ColorInProgressStatus; break;
			case TSESST_CLOSED:    pClr->C = r_cfg.ColorClosedStatus;     break;
			case TSESST_CANCELED:  pClr->C = r_cfg.ColorCanceledStatus;   break;
			default:
				pClr->Status = -1;
				pClr->C = RGB(0x00, 0x89, 0xC0);
				ok = 1;
		}
		if(pClr->Status >= 0 && pClr->C != 0)
			pClr->C--;
	}
	else
		ok = -1;
	SETIFZ(pClr->C, RGB(0x00, 0x89, 0xC0));
	return ok;
}

int PPViewPrcBusy::PrcBusyTimeChunkGrid::Edit(int item, long rowID, const LDATETIME & rTm, long * pID)
{
	int    ok = -1;
	if(item == iChunk) {
		ok = P_View->EditTimeGridItem(pID, rowID, rTm);
	}
	else if(item == iRow) {
		LDATETIME dtm_zero;
		dtm_zero.SetZero();
		PPID   id_zero = 0;
		ok = P_View->EditTimeGridItem(&id_zero, rowID, dtm_zero);
	}
	return ok;
}

int PPViewPrcBusy::PrcBusyTimeChunkGrid::MoveChunk(int mode, long id, long rowId, const STimeChunk & rNewChunk)
{
	int    ok = -1;
	if(mode == mmCanResizeLeft || mode == mmCanResizeRight)
		ok = 1;
	else if(mode == mmCanMove)
		ok = 1;
	else if(mode == mmCommit) {
		ok = P_View->UpdateTimeGridItem(id, rowId, rNewChunk);
	}
	return ok;
}

int SLAPI PPViewPrcBusy::UpdateTimeBrowser(int destroy)
{
	SString title_buf, temp_buf;
	ProcessorTbl::Rec prc_rec;
	title_buf = GetDescr();
	if(Filt.PrcID && TSesObj.GetPrc(Filt.PrcID, &prc_rec, 0, 1) > 0) {
		// title_buf.Space().CatParStr((temp_buf = prc_rec.Name).Transf(CTRANSF_INNER_TO_OUTER));
		title_buf = prc_rec.Name;
	}
	else
		title_buf.Transf(CTRANSF_OUTER_TO_INNER);
	return PPView::UpdateTimeBrowser(&Grid, title_buf, destroy);
}

int SLAPI PPViewPrcBusy::TimeChunkBrowser()
{
	UpdateTimeBrowser(1);
	STimeChunkBrowser * p_brw = new STimeChunkBrowser;
	STimeChunkBrowser::Param p, saved_params;
	InitSTimeChunkBrowserParam("PPViewPrcBusy", &p);
	saved_params.RegSaveParam = p.RegSaveParam;
	{
		long   tcbquant = TSesObj.GetConfig().TimeChunkBrowserQuant;
		for(uint i = 0; i < PrcList.getCount(); i++) {
			const PPID prc_id = PrcList.get(i);
			ProcessorTbl::Rec prc_rec;
			if(TSesObj.GetPrc(prc_id, &prc_rec, 1, 1) > 0) {
				if(prc_rec.TcbQuant > 0) {
					//
					// В записи процессора квант хранится в 5-секундных единицах
					//
					if(tcbquant <= 0)
						tcbquant = (prc_rec.TcbQuant * 5);
					else
						SETMIN(tcbquant, (prc_rec.TcbQuant * 5));
				}
			}
		}
		if(tcbquant <= 0)
			tcbquant = (15 * 60);
		p.Quant = tcbquant;
	}
	SETFLAG(p.Flags, STimeChunkBrowser::Param::fSnapToQuant,
		TSesObj.GetConfig().Flags & PPTSessConfig::fSnapInTimeChunkBrowser);
	p.PixQuant = 20;
	p.PixRow = 20;
	p.PixRowMargin = 5;
	p.HdrLevelHeight = 20;
	p.DefBounds.Set(Filt.Period.Start.d, plusdate(Filt.Period.Finish.d, 30));
	if(p_brw->RestoreParameters(saved_params) > 0) {
		p.PixQuant = saved_params.PixQuant;
		p.PixRow   = saved_params.PixRow;
		p.PixRowMargin = saved_params.PixRowMargin;
		p.TextZonePart = saved_params.TextZonePart;
	}
	p_brw->SetBmpId(STimeChunkBrowser::bmpModeGantt, BM_TIMEGRAPH_GANTT);
	p_brw->SetBmpId(STimeChunkBrowser::bmpModeHourDay, BM_TIMEGRAPH_HOURDAY);
	p_brw->SetBmpId(STimeChunkBrowser::bmpBack, BM_BACK);
	if(PrcList.getSingle())
		p.ViewType = STimeChunkBrowser::Param::vHourDay;
	p_brw->SetParam(&p);
	{
		PPID   reg_cal_id = SlObj.PsnObj.GetConfig().RegStaffCalID;
		if(reg_cal_id) {
			STimeChunkArray work_list, collapse_list;
			DateRange period;
			period.Set(Filt.Period.Start.d, Filt.Period.Finish.d);
			LDATE curdt = getcurdate_();
			SETIFZ(period.low, encodedate(1, curdt.month(), curdt.year()));
			if(!period.upp || period.upp > encodedate(31, 12, curdt.year()+1))
				period.upp = encodedate(curdt.dayspermonth(), curdt.month(), curdt.year());
			ScObjAssoc scoa;
			ScObj.InitScObjAssoc(reg_cal_id, 0, 0L, &scoa);
			if(ScObj.CalcPeriod(scoa, period, 0, 0, 0, &work_list)) {
				work_list.Sort();
				if(work_list.getCount()) {
					work_list.GetFreeList(&collapse_list);
					collapse_list.Sort();
					if(collapse_list.getCount())
						Grid.SetCollapseList(&collapse_list);
				}
			}
		}
	}
	p_brw->SetData(&Grid, 0);
	p_brw->SetResID(((PPApp *)APPL)->LastCmd);
	{
		SString title_buf, temp_buf;
		title_buf = GetDescr();
		if(Filt.PrcID) {
			ProcessorTbl::Rec prc_rec;
			if(TSesObj.GetPrc(Filt.PrcID, &prc_rec, 0, 1) > 0) {
				// title_buf.Space().CatParStr((temp_buf = prc_rec.Name).Transf(CTRANSF_INNER_TO_OUTER));
				(title_buf = prc_rec.Name).Transf(CTRANSF_INNER_TO_OUTER);
			}
		}
		p_brw->setTitle(title_buf.Transf(CTRANSF_OUTER_TO_INNER));
	}
	InsertView(p_brw);
	return 1;
}
//
// }
//
