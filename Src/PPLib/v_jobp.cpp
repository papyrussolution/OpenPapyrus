// V_JOBP.CPP
// Copyright (c) A.Sobolev 2005, 2007, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021, 2022, 2023
// @codepage UTF-8
// Редактирование списка задач JobServer'а
//
#include <pp.h>
#pragma hdrstop
//
//
//
class JobItemDialog : public TDialog {
	DECL_DIALOG_DATA(PPJob);
public:
	JobItemDialog(PPJobMngr * pMngr, PPJobPool * pJobPool) : TDialog(DLG_JOBITEM), P_Mngr(pMngr), P_JobPool(pJobPool)
	{
		P_Mngr->GetResourceList(0, CmdSymbList);
		if(P_JobPool) {
			PPJob job;
			for(PPID id = 0; P_JobPool->Enum(&id, &job) > 0;)
				JobList.Add(job.ID, job.Name);
		}
		SetupTimePicker(this, CTL_JOBITEM_SCHDLBEFORE, CTLTM_JOBITEM_SCHDLBEFORE);
	}
	DECL_DIALOG_SETDTS()
	{
		int    ok = 1;
		int    db_id = 0;
		PPDbEntrySet2 dbes;
		PPIniFile ini_file;
		SString db_symb, db_name;
		StrAssocArray cmd_txt_list;
		PPID   cmd_id = 0;
		RVALUEPTR(Data, pData);
		PreserveDbSymb = Data.DbSymb; // @v11.3.5
		setCtrlString(CTL_JOBITEM_NAME, Data.Name);
		setCtrlData(CTL_JOBITEM_SYMB, Data.Symb);
		setCtrlLong(CTL_JOBITEM_ID, Data.ID);
		AddClusterAssoc(CTL_JOBITEM_FLAGS, 0, PPJob::fNotifyByMail);
		AddClusterAssoc(CTL_JOBITEM_FLAGS, 1, PPJob::fDisable);
		AddClusterAssoc(CTL_JOBITEM_FLAGS, 2, PPJob::fOnStartUp);
		AddClusterAssoc(CTL_JOBITEM_FLAGS, 3, PPJob::fPermanent);
		AddClusterAssoc(CTL_JOBITEM_FLAGS, 4, PPJob::fUnSheduled);
		AddClusterAssoc(CTL_JOBITEM_FLAGS, 5, PPJob::fSkipEmptyNotification);
		SetClusterData(CTL_JOBITEM_FLAGS, Data.Flags);
		setCtrlTime(CTL_JOBITEM_SCHDLBEFORE, Data.ScheduleBeforeTime);
		dbes.ReadFromProfile(&ini_file);
		if(Data.DbSymb.NotEmpty()) {
			db_id = dbes.GetBySymb(Data.DbSymb, 0);
			if(db_id)
				dbes.SetSelection(db_id);
		}
		SetupDBEntryComboBox(this, CTLSEL_JOBITEM_DBSYMB, &dbes, 0);
		if(db_id == 0 && Data.DbSymb.NotEmpty())
			setCtrlString(CTL_JOBITEM_DBSYMB, Data.DbSymb);
		disableCtrl(CTLSEL_JOBITEM_DBSYMB, 1);
		P_Mngr->GetResourceList(1, cmd_txt_list);
		cmd_txt_list.SortByText();
		uint   pos = 0;
		if(CmdSymbList.SearchByTextNc(Data.Descr.Symb, &pos))
			cmd_id = CmdSymbList.Get(pos).Id;
		SetupStrAssocCombo(this, CTLSEL_JOBITEM_CMD, cmd_txt_list, cmd_id, 0);
		SetupWordSelector(CTLSEL_JOBITEM_CMD, 0, cmd_id, 2, WordSel_ExtraBlock::fAlwaysSearchBySubStr); // @v10.7.8
		SetupStrAssocCombo(this, CTLSEL_JOBITEM_NEXTJOB, JobList, Data.NextJobID, 0, 0, 0);
		disableCtrl(CTLSEL_JOBITEM_CMD, cmd_id);
		{
			PPJobDescr job_descr;
			if(cmd_id && P_Mngr->LoadResource(cmd_id, &job_descr) > 0)
				enableCommand(cmJobParam, !(job_descr.Flags & PPJobDescr::fNoParam));
			else
				enableCommand(cmJobParam, 0);
			enableCommand(cmSchedule, !(Data.Flags & PPJob::fUnSheduled));
			DisableClusterItem(CTL_JOBITEM_FLAGS, 5, (Data.Flags & PPJob::fUnSheduled));
			disableCtrl(CTL_JOBITEM_SCHDLBEFORE, (Data.Flags & PPJob::fUnSheduled));
		}
		return ok;
	}
	DECL_DIALOG_GETDTS()
	{
		int    ok = 1;
		PPID   job_id = 0;
		uint   sel = 0;
		getCtrlString(sel = CTL_JOBITEM_NAME, Data.Name);
		THROW_PP(Data.Name.NotEmptyS(), PPERR_NAMENEEDED);
		getCtrlData(sel = CTL_JOBITEM_SYMB, Data.Symb);
		THROW(P_JobPool->CheckUniqueJob(&Data));
		getCtrlData(sel = CTLSEL_JOBITEM_CMD, &job_id);
		THROW_PP(CmdSymbList.Search(job_id), PPERR_INVJOBCMD);
		THROW(P_Mngr->LoadResource(job_id, &Data.Descr));
		getCtrlData(sel = CTLSEL_JOBITEM_NEXTJOB, &Data.NextJobID);
		GetClusterData(CTL_JOBITEM_FLAGS, &Data.Flags);
		Data.ScheduleBeforeTime = getCtrlTime(CTL_JOBITEM_SCHDLBEFORE);
		// @v11.3.5 {
		if(oneof2(job_id, PPJOB_STYLOQSENDINDEXINGCONTENT, PPJOB_STYLOQPREPAREAHEAD)) // @v11.4.8 PPJOB_STYLOQPREPAREAHEAD
			Data.DbSymb.Z();
		else
			Data.DbSymb = PreserveDbSymb;
		// } @v11.3.5 
		THROW(CheckRecursion(&Data));
		ASSIGN_PTR(pData, Data);
		CATCHZOKPPERRBYDLG
		return ok;
	}
private:
	DECL_HANDLE_EVENT;
	void   editSchedule();
	int    CheckRecursion(const PPJob * pData);

	PPJobPool * P_JobPool;
	PPJobMngr * P_Mngr;
	StrAssocArray CmdSymbList;
	StrAssocArray JobList;
	SString PreserveDbSymb; // @v11.3.5
};

IMPL_HANDLE_EVENT(JobItemDialog)
{
	TDialog::handleEvent(event);
	if(TVCOMMAND)
		if(event.isCbSelected(CTLSEL_JOBITEM_CMD)) {
			PPJobDescr job_descr;
			const  PPID job_id = getCtrlLong(CTLSEL_JOBITEM_CMD);
			if(job_id && P_Mngr->LoadResource(job_id, &job_descr) > 0) {
				// @v11.3.5 {
				if(oneof2(job_id, PPJOB_STYLOQSENDINDEXINGCONTENT, PPJOB_STYLOQPREPAREAHEAD)) // @v11.4.8 PPJOB_STYLOQPREPAREAHEAD
					Data.DbSymb.Z();
				else
					Data.DbSymb = PreserveDbSymb;
				// } @v11.3.5 
				enableCommand(cmJobParam, !(job_descr.Flags & PPJobDescr::fNoParam));
				SString name_buf;
				getCtrlString(CTL_JOBITEM_NAME, name_buf);
				if(!name_buf.NotEmptyS())
					setCtrlString(CTL_JOBITEM_NAME, name_buf = job_descr.Text);
			}
			else
				enableCommand(cmJobParam, 0);
		}
		else if(event.isClusterClk(CTL_JOBITEM_FLAGS)) {
			GetClusterData(CTL_JOBITEM_FLAGS, &Data.Flags);
			enableCommand(cmSchedule, !(Data.Flags & PPJob::fUnSheduled));
			DisableClusterItem(CTL_JOBITEM_FLAGS, 5, (Data.Flags & PPJob::fUnSheduled));
			disableCtrl(CTL_JOBITEM_SCHDLBEFORE, (Data.Flags & PPJob::fUnSheduled));
		}
		else if(TVCMD == cmSchedule)
			editSchedule();
		else if(TVCMD == cmJobParam) {
			// @v11.0.0 P_Mngr->EditJobParam(getCtrlLong(CTLSEL_JOBITEM_CMD), Data, &Data.Param);
			// @v11.0.0 {
			Data.Descr.CmdID = getCtrlLong(CTLSEL_JOBITEM_CMD);
			P_Mngr->EditJobParam(Data);
			// } @v11.0.0 
		}
		else if(TVCMD == cmEmailParam) {
            EmailToBlock etb;
            SString temp_buf;
            etb.MailAccID = Data.EmailAccID;
			Data.GetExtStrData(Data.extssEMailSubj, etb.Subj);
			Data.GetExtStrData(Data.extssEMailAddrList, temp_buf);
			if(temp_buf.NotEmptyS()) {
				StringSet ss(';', temp_buf);
				long   _i = 0;
				for(uint ssp = 0; ss.get(&ssp, temp_buf);) {
					if(temp_buf.NotEmptyS())
						etb.AddrList.Add(++_i, temp_buf);
				}
			}
			if(etb.Edit(0) > 0) {
				Data.EmailAccID = etb.MailAccID;
				Data.PutExtStrData(Data.extssEMailSubj, etb.Subj);
				StringSet ss(";");
				if(etb.AddrList.getCount()) {
					for(uint i = 0; i < etb.AddrList.getCount(); i++) {
						temp_buf = etb.AddrList.at_WithoutParent(i).Txt;
						if(temp_buf.NotEmptyS())
							ss.add(temp_buf);
					}
				}
				temp_buf = ss.getBuf();
				Data.PutExtStrData(Data.extssEMailAddrList, temp_buf);
			}
		}
		else
			return;
	else
		return;
	clearEvent(event);
}

void JobItemDialog::editSchedule()
{
	RepeatingDialog * dlg = new RepeatingDialog(RepeatingDialog::fEditTime);
	if(CheckDialogPtrErr(&dlg)) {
		dlg->setDTS(&Data.Dtr);
		for(int valid_data = 0; !valid_data && ExecView(dlg) == cmOK;)
			if(dlg->getDTS(&Data.Dtr))
				valid_data = 1;
	}
	delete dlg;
}

int JobItemDialog::CheckRecursion(const PPJob * pData)
{
	int    ok = 1;
	if(pData && P_JobPool) {
		const PPJob * p_job = pData;
		PPIDArray job_list;
		do {
			THROW_PP(!job_list.lsearch(p_job->ID), PPERR_JOBITEMLOOP);
			THROW_SL(job_list.add(p_job->ID));
			p_job = p_job->NextJobID ? P_JobPool->GetJobItem(p_job->NextJobID) : 0;
		} while(p_job);
	}
	CATCHZOK
	return ok;
}

int EditJobItem(PPJobMngr * pMngr, PPJobPool * pJobPool, PPJob * pData) { DIALOG_PROC_BODY_P2(JobItemDialog, pMngr, pJobPool, pData); }
//
//
//
class JobPoolDialog : public PPListDialog {
public:
	JobPoolDialog(PPJobMngr * pMngr, PPJobPool * pData) : PPListDialog(DLG_JOBPOOL, CTL_JOBPOOL_LIST), P_Mngr(pMngr), P_Data(pData), ForAllDb(false)
	{
		updateList(-1);
	}
private:
	DECL_HANDLE_EVENT
	{
		PPListDialog::handleEvent(event);
		if(event.isCmd(cmPrint))
			PPAlddPrint(REPORT_JOBPOOL, PView(P_Data), 0);
		else if(event.isClusterClk(CTL_JOBPOOL_FLAGS)) {
			ForAllDb = LOGIC(getCtrlUInt16(CTL_JOBPOOL_FLAGS) & 0x0001);
			updateList(-1);
		}
		else
			return;
		clearEvent(event);
	}
	virtual int setupList();
	virtual int addItem(long * pPos, long * pID);
	virtual int editItem(long pos, long id);
	virtual int delItem(long pos, long id);

	PPJobMngr * P_Mngr;
	PPJobPool * P_Data;
	bool   ForAllDb;    // Показывать задачи для всех баз данных
	uint8  Reserve[3];  // @alignment
};

int JobPoolDialog::setupList()
{
	int    ok = 1;
	SString save_db_symb;
	PPJob  job;
	SString buf;
	StringSet ss(SLBColumnDelim);
	for(PPID id = 0; ok && P_Data->Enum(&id, &job, ForAllDb);) {
		ss.Z();
		(ss += job.Name) += job.Descr.Symb;
		ss += job.Dtr.Format(0, buf);
		if(job.NextJobID) {
			const PPJob * p_job = P_Data->GetJobItem(job.NextJobID, ForAllDb);
			if(p_job)
				buf.CopyFrom(p_job->Name);
		}
		else
			buf.Z();
		ss += buf;
		if(!addStringToList(id, ss.getBuf()))
			ok = 0;
	}
	return ok;
}

int JobPoolDialog::addItem(long * pPos, long * pID)
{
	int    ok = -1;
	PPJob  job;
	THROW(CheckCfgRights(PPCFGOBJ_JOBPOOL, PPR_INS, 0));
	job.DbSymb = P_Data->GetDbSymb();
	if(EditJobItem(P_Mngr, P_Data, &job) > 0) {
		PPID   id = 0;
		THROW(P_Data->PutJobItem(&id, &job));
		ASSIGN_PTR(pID, id);
		ok = 1;
	}
	CATCHZOKPPERR
	return ok;
}

int JobPoolDialog::editItem(long pos, long id)
{
	int    ok = -1;
	if(CheckCfgRights(PPCFGOBJ_JOBPOOL, PPR_MOD, 0)) {
		const PPJob * p_job = P_Data->GetJobItem(id, ForAllDb);
		if(p_job) {
			PPJob job = *p_job;
			if(EditJobItem(P_Mngr, P_Data, &job) > 0)
				ok = P_Data->PutJobItem(&id, &job);
			if(!ok)
				PPError();
		}
	}
	else
		ok = PPErrorZ();
	return ok;
}

int JobPoolDialog::delItem(long pos, long id)
{
	int    ok = CheckCfgRights(PPCFGOBJ_JOBPOOL, PPR_DEL, 0) > 0 ? P_Data->PutJobItem(&id, 0) : 0;
	if(!ok)
		PPError();
	return ok;
}

int ViewJobPool()
{
	MemLeakTracer mlt;
	int    ok = -1;
	PPJobMngr mngr;
	PPJobPool pool(&mngr, 0, 0);
	SString db_symb;
	JobPoolDialog * dlg = 0;
	THROW(CheckCfgRights(PPCFGOBJ_JOBPOOL, PPR_READ, 0));
	THROW_PP(CurDict->GetDbSymb(db_symb), PPERR_DBSYMBUNDEF);
	THROW(mngr.LoadPool2(db_symb, &pool, false)); //@erik v10.7.4 LoadPool-->LoadPool2
	THROW(CheckDialogPtrErr(&(dlg = new JobPoolDialog(&mngr, &pool))));
	while(ExecView(dlg) == cmOK) {
		THROW(CheckCfgRights(PPCFGOBJ_JOBPOOL, PPR_MOD, 0));
		//if(mngr.SavePool(&pool)) { //@erik v10.7.4
		if(mngr.SavePool2(&pool)) { //@erik v10.7.4
			DS.LogAction(PPACN_CONFIGUPDATED, PPCFGOBJ_JOBPOOL, 0, 0, 1);
			ok = 1;
			break;
		}
		else
			PPError();
	}
	CATCHZOKPPERR
	delete dlg;
	return ok;
}
//
// Implementation of PPALDD_JobPool
//
PPALDD_CONSTRUCTOR(JobPool)
{
	InitFixData(rscDefHdr, &H, sizeof(H));
	InitFixData(rscDefIter, &I, sizeof(I));
}

PPALDD_DESTRUCTOR(JobPool)
{
	Destroy();
}

int PPALDD_JobPool::InitData(PPFilt & rFilt, long rsrv)
{
	Extra[1].Ptr = rFilt.Ptr;
	return DlRtm::InitData(rFilt, rsrv);
}

int PPALDD_JobPool::InitIteration(long iterId, int sortId, long rsrv)
{
	IterProlog(iterId, 1);
	if(sortId >= 0)
		SortIdx = sortId;
	I.nn = 0;
	return 1;
}

int PPALDD_JobPool::NextIteration(long iterId)
{
	int    ok = -1;
	IterProlog(iterId, 0);
	long n = static_cast<uint>(I.nn);
	PPJob item;
	PPJobPool * p_jobpool = static_cast<PPJobPool *>(Extra[1].Ptr);
	if(p_jobpool->Enum(&n, &item) > 0) {
		SString buf;
		I.ID = item.ID;
		item.Name.CopyTo(I.Name, sizeof(I.Name));
		item.Descr.Symb.CopyTo(I.Symb, sizeof(I.Symb));
		item.Dtr.Format(0, buf);
		buf.CopyTo(I.DateTimeRepeating, sizeof(I.DateTimeRepeating));
		I.nn = n;
		ok = DlRtm::NextIteration(iterId);
	}
	return ok;
}

void PPALDD_JobPool::Destroy()
{
	Extra[0].Ptr = 0;
	Extra[1].Ptr = 0;
}
//
// PPViewJob
//
IMPLEMENT_PPFILT_FACTORY(Job); JobFilt::JobFilt() : PPBaseFilt(PPFILT_JOB, 0, 1)
{
	SetFlatChunk(offsetof(JobFilt, ReserveStart),
		offsetof(JobFilt, ReserveEnd)-offsetof(JobFilt, ReserveStart)+sizeof(ReserveEnd));
	Init(1, 0);
}

PPViewJob::PPViewJob() : PPView(0, &Filt, PPVIEW_JOB, (implBrowseArray|implDontEditNullFilter), 0), IsChanged(0), P_Pool(0)
{
	LoadPool();
}

PPViewJob::~PPViewJob()
{
	if(IsChanged == 1 && CONFIRM(PPCFM_DATACHANGED))
		SavePool();
	ZDELETE(P_Pool);
}

int PPViewJob::LoadPool()
{
	int    ok = 1;
	SString db_symb;
	SavePool();
	ZDELETE(P_Pool);
	THROW_MEM(P_Pool = new PPJobPool(&Mngr, 0, 0));
	THROW_PP(CurDict->GetDbSymb(db_symb), PPERR_DBSYMBUNDEF);
	THROW(Mngr.LoadPool2(db_symb, P_Pool, false)); // @erik v10.7.4 LoadPool-->LoadPool2
	Mngr.GetResourceList(0, CmdSymbList);
	CATCH
		ZDELETE(P_Pool);
		ok = PPErrorZ();
	ENDCATCH
	return ok;
}

int PPViewJob::SavePool()
{
	int    ok = -1;
	if(P_Pool) {
		//if(Mngr.SavePool(P_Pool)) { //@erik v10.7.4
		if(Mngr.SavePool2(P_Pool)) { //@erik v10.7.4
			IsChanged = 0;
			DS.LogAction(PPACN_CONFIGUPDATED, PPCFGOBJ_JOBPOOL, 0, 0, 1);
			ok = 1;
		}
		else
			ok = (PPError(), 0);
	}
	return ok;
}

class JobFiltDialog : public TDialog {
	DECL_DIALOG_DATA(JobFilt);
public:
	JobFiltDialog(PPJobMngr & rMngr, PPJobPool * pJobPool) : TDialog(DLG_JOBFILT), R_Mngr(rMngr), P_Pool(pJobPool)
	{
		R_Mngr.GetResourceList(0, CmdSymbList);
	}
	DECL_DIALOG_SETDTS()
	{
		if(!RVALUEPTR(Data, pData))
			Data.Init(1, 0);
		{
			StrAssocArray cmd_txt_list;
			R_Mngr.GetResourceList(1, cmd_txt_list);
			cmd_txt_list.SortByText();
			SetupStrAssocCombo(this, CTLSEL_JOBFILT_CMD, cmd_txt_list, Data.CmdId, 0);
		}
		AddClusterAssoc(CTL_JOBFILT_FLAGS, 0, JobFilt::fForAllDb);
		SetClusterData(CTL_JOBFILT_FLAGS, Data.Flags);
		return 1;
	}
	DECL_DIALOG_GETDTS()
	{
		int    ok = 1;
		uint sel = 0;
		getCtrlData(sel = CTLSEL_JOBFILT_CMD, &Data.CmdId);
		if(Data.CmdId)
			THROW_PP(CmdSymbList.Search(Data.CmdId), PPERR_INVJOBCMD);
		GetClusterData(CTL_JOBFILT_FLAGS, &Data.Flags);
		ASSIGN_PTR(pData, Data);
		ok = 1;
		CATCH
			ok = (selectCtrl(sel), 0);
		ENDCATCH
		return ok;
	}
private:
	StrAssocArray CmdSymbList;
	PPJobMngr & R_Mngr;
	PPJobPool * P_Pool;
};

int PPViewJob::EditBaseFilt(PPBaseFilt * pFilt)
{
	DIALOG_PROC_BODY_P2ERR(JobFiltDialog, Mngr, P_Pool, static_cast<JobFilt *>(pFilt));
}

int PPViewJob::Init_(const PPBaseFilt * pFilt)
{
	int    ok = 1;
	THROW(Helper_InitBaseFilt(pFilt));
	THROW(MakeList());
	CATCHZOK
	return ok;
}

int PPViewJob::InitIteration()
{
	Counter.Init(List.getCount());
	return 1;
}

int FASTCALL PPViewJob::NextIteration(JobViewItem * pItem)
{
	int    ok = -1;
	while(Counter < Counter.GetTotal() && ok < 0) {
		ASSIGN_PTR(pItem, List.at(Counter));
		ok = 1;
		Counter.Increment();
	}
	return ok;
}

int PPViewJob::AddItem(PPID * pID)
{
	int    ok = -1;
	if(P_Pool) {
		PPJob  job;
		THROW(CheckCfgRights(PPCFGOBJ_JOBPOOL, PPR_INS, 0));
		job.DbSymb = P_Pool->GetDbSymb();
		if(EditJobItem(&Mngr, P_Pool, &job) > 0) {
			PPID   id = 0;
			THROW(P_Pool->PutJobItem(&id, &job));
			ASSIGN_PTR(pID, id);
			IsChanged = 1;
			ok = 1;
		}
	}
	CATCHZOKPPERR
	return ok;
}

int PPViewJob::EditItem(PPID id)
{
	int    ok = -1;
	if(P_Pool && CheckCfgRights(PPCFGOBJ_JOBPOOL, PPR_MOD, 0)) {
		const PPJob * p_job = P_Pool->GetJobItem(id, LOGIC(Filt.Flags & JobFilt::fForAllDb));
		if(p_job) {
			PPJob job = *p_job;
			if(EditJobItem(&Mngr, P_Pool, &job) > 0)
				ok = P_Pool->PutJobItem(&id, &job);
			if(ok > 0)
				IsChanged = 1;
			else if(!ok)
				PPError();
		}
	}
	else
		ok = PPErrorZ();
	return ok;
}

int PPViewJob::DeleteItem(PPID id)
{
	int    ok = (P_Pool) ? (CheckCfgRights(PPCFGOBJ_JOBPOOL, PPR_DEL, 0) > 0 ? P_Pool->PutJobItem(&id, 0) : 0) : -1;
	if(ok > 0)
		IsChanged = 1;
	else if(!ok)
		PPError();
	return ok;
}

/*virtual*/SArray  * PPViewJob::CreateBrowserArray(uint * pBrwId, SString * pSubTitle)
{
	ASSIGN_PTR(pBrwId, BROWSER_JOB);
	return new SArray(List);
}

int PPViewJob::_GetDataForBrowser(SBrowserDataProcBlock * pBlk)
{
	int    ok = 0;
	if(pBlk->P_SrcData && pBlk->P_DestData) {
		ok = 1;
		const  JobViewItem * p_item = static_cast<const JobViewItem *>(pBlk->P_SrcData);
		int    r = 0;
		switch(pBlk->ColumnN) {
			case 0: pBlk->Set(p_item->ID); break; // @id
			case 1: pBlk->Set(p_item->Name); break; // @name
			case 2: pBlk->Set(p_item->Symb); break; // @symb
			case 3: pBlk->Set(p_item->Dtr); break; // @datetime repeating
			case 4: pBlk->Set(p_item->NextJob); break; // @next job
			case 5: pBlk->Set(p_item->DbSymb); break; // @database symbol
		}
	}
	return ok;
}

/*static*/int FASTCALL PPViewJob::GetDataForBrowser(SBrowserDataProcBlock * pBlk)
{
	PPViewJob * p_v = static_cast<PPViewJob *>(pBlk->ExtraPtr);
	return p_v ? p_v->_GetDataForBrowser(pBlk) : 0;
}

/*static*/int PPViewJob::CellStyleFunc(const void * pData, long col, int paintAction, BrowserWindow::CellStyle * pStyle, void * extraPtr)
{
	int    ok = -1;
	PPViewBrowser * p_brw = static_cast<PPViewBrowser *>(extraPtr);
	if(p_brw) {
		PPViewJob * p_view = static_cast<PPViewJob *>(p_brw->P_View);
		ok = p_view ? p_view->CellStyleFunc_(pData, col, paintAction, pStyle, p_brw) : -1;
	}
	return ok;
}

int PPViewJob::CellStyleFunc_(const void * pData, long col, int paintAction, BrowserWindow::CellStyle * pStyle, PPViewBrowser * pBrw)
{
	int    ok = -1;
	if(pBrw && pData && pStyle) {
		const  BrowserDef * p_def = pBrw->getDef();
		if(col >= 0 && col < p_def->getCountI()) {
			const BroColumn & r_col = p_def->at(col);
			const JobViewItem * p_item = static_cast<const JobViewItem *>(pData);
			if(r_col.OrgOffs == 1) { // name
				if(p_item->Flags & PPJob::fDisable) {
					ok = pStyle->SetFullCellColor(GetColorRef(SClrLightgrey));
				}
			}
		}
	}
	return ok;
}

void PPViewJob::PreprocessBrowser(PPViewBrowser * pBrw)
{
	if(pBrw) {
		pBrw->SetDefUserProc(PPViewJob::GetDataForBrowser, this);
		pBrw->SetCellStyleFunc(PPViewJob::CellStyleFunc, pBrw);
	}
}

/*virtual*/int PPViewJob::ProcessCommand(uint ppvCmd, const void * pHdr, PPViewBrowser * pBrw)
{
	int    ok = (ppvCmd == PPVCMD_PRINT) ? -2 : PPView::ProcessCommand(ppvCmd, pHdr, pBrw);
	if(ok == -2) {
		AryBrowserDef * p_def = pBrw ? static_cast<AryBrowserDef *>(pBrw->getDef()) : 0;
		const  long cur_pos = p_def ? p_def->_curItem() : 0;
		long   update_pos = cur_pos;
		PPID   update_id = 0;
		PPID   job_id = pHdr ? *static_cast<const long *>(pHdr) : 0;
		switch(ppvCmd) {
			case PPVCMD_ADDITEM:
				ok = AddItem(&update_id);
				break;
			case PPVCMD_ADDBYSAMPLE: // @v10.4.3
				ok = -1;
				if(job_id && P_Pool && CheckCfgRights(PPCFGOBJ_JOBPOOL, PPR_MOD, 0)) {
					const PPJob * p_job = P_Pool->GetJobItem(job_id, LOGIC(Filt.Flags & JobFilt::fForAllDb));
					if(p_job) {
						PPJob  new_job = *p_job;
						PPID   new_id = 0;
						new_job.ID = 0;
						SString name_pattern = p_job->Name;
						long   name_uniq_counter = 1;
						int    is_there_dup_name = 0;
						do {
							(new_job.Name = name_pattern).Space().CatChar('#').Cat(++name_uniq_counter);
							PPJob en_job;
							for(PPID en_id = 0; !is_there_dup_name && P_Pool->Enum(&en_id, &en_job, 0);) {
								if(en_job.Name.IsEqNC(new_job.Name))
									is_there_dup_name = 1;
							}
						} while(is_there_dup_name);
						int    local_ok = 1;
						if(EditJobItem(&Mngr, P_Pool, &new_job) > 0) {
							local_ok = P_Pool->PutJobItem(&new_id, &new_job);
						}
						if(local_ok > 0) {
							update_id = job_id;
							ok = 1;
						}
						else if(!local_ok)
							PPError();
					}
				}
				break;
			case PPVCMD_EDITITEM:
				ok = -1;
				if(job_id && EditItem(job_id) > 0) {
					update_id = job_id;
					ok = 1;
				}
				break;
			case PPVCMD_DELETEITEM:
				ok = -1;
				if(job_id && DeleteItem(job_id)) {
					update_pos = (cur_pos > 0) ? (cur_pos-1) : 0;
					ok = 1;
				}
				break;
			case PPVCMD_SAVE:
				ok = -1;
				LoadPool(); // внутри вызывается функция SavePool
				break;
			case PPVCMD_EXECJOBIMM:
				{
					if(PPMaster) {
						const SymbHashTable * p_ht = PPGetStringHash(PPSTR_HASHTOKEN);
						const PPJob * p_job = P_Pool->GetJobItem(job_id, LOGIC(Filt.Flags & JobFilt::fForAllDb));
						if(p_ht && p_job) {
							PPJobSrvClient cli;
							PPJobSrvReply reply;
							if(cli.Connect(0, 0)) {
								SString cmd_buf;
								SString fmt_buf;
								SString msg_buf;
								p_ht->GetByAssoc(PPHS_EXECJOBIMM, cmd_buf);
								if(cmd_buf.NotEmpty()) {
									cmd_buf.Space().Cat(p_job->ID);
									PPJobSrvReply reply;
									cli.ExecSrvCmd(cmd_buf, reply);
									uint  msg_options = 0;
									if(reply.CheckRepError()) {
										PPLoadText(PPTXT_EXECJOBIMM_SUCCESS, fmt_buf);
										msg_buf.Printf(fmt_buf, p_job->Name.cptr());
										msg_options = mfInfo|mfOK;
									}
									else {
										SString temp_buf;
										PPGetLastErrorMessage(1, temp_buf);
										PPLoadText(PPTXT_EXECJOBIMM_ERROR, fmt_buf);
										(msg_buf = fmt_buf).CatDiv(':', 2).CatParStr(p_job->Name).Space().Cat(temp_buf);
										msg_options = mfError|mfOK;
									}
									PPOutputMessage(msg_buf, msg_options);
								}
							}
							else
								PPError();
						}
					}
				}
				break;
			case PPVCMD_PRINT:
				ok = -1;
				Print(pHdr);
				break;
		}
		if(ok > 0) {
			MakeList();
			if(pBrw) {
				if(p_def) {
					SArray * p_array = new SArray(List);
					p_def->setArray(p_array, 0, 1);
					pBrw->setRange(p_array->getCount());
					uint   temp_pos = 0;
					if(update_id > 0 && List.lsearch(&update_id, &temp_pos, CMPF_LONG))
						update_pos = temp_pos;
					if(update_pos >= 0)
						pBrw->go(update_pos);
					else if(update_pos == MAXLONG)
						pBrw->go(p_array->getCount() - 1);
				}
				pBrw->Update();
			}
		}
	}
	return ok;
}

/*virtual*/int PPViewJob::Print(const void *)
{
	return P_Pool ? PPAlddPrint(REPORT_JOBPOOL, PView(P_Pool), 0) : -1;
}

int PPViewJob::CheckForFilt(PPJob & rJob)
{
	int    r = 1;
	if(Filt.CmdId) {
		uint pos = 0;
		PPID cmd_id = 0;
		if(CmdSymbList.SearchByTextNc(rJob.Descr.Symb, &pos))
			cmd_id = CmdSymbList.Get(pos).Id;
		r = BIN(Filt.CmdId == cmd_id);
	}
	return r;
}

int PPViewJob::MakeList()
{
	int    ok = -1;
	List.freeAll();
	if(P_Pool) {
		PPJob  job;
		SString temp_buf;
		for(PPID id = 0; ok && P_Pool->Enum(&id, &job, Filt.Flags & JobFilt::fForAllDb);) {
			if(CheckForFilt(job) > 0) {
				JobViewItem item;
 				MEMSZERO(item);
				item.ID = job.ID;
				job.Name.CopyTo(item.Name, sizeof(item.Name));
	      		job.DbSymb.CopyTo(item.DbSymb, sizeof(item.DbSymb));
				if(job.Flags & PPJob::fUnSheduled)
					(temp_buf = "none").CopyTo(item.Dtr, sizeof(item.Dtr));
				else
					job.Dtr.Format(0, temp_buf).CopyTo(item.Dtr, sizeof(item.Dtr));
				item.Flags = job.Flags;
				item.EstimatedTime = job.EstimatedTime;
				item.LastRunningTime = job.LastRunningTime;
				item.Ver = job.Ver;
				if(job.NextJobID) {
					const PPJob * p_job = P_Pool->GetJobItem(job.NextJobID, LOGIC(Filt.Flags & JobFilt::fForAllDb));
					if(p_job)
						p_job->Name.CopyTo(item.NextJob, sizeof(item.NextJob));
				}
				STRNSCPY(item.Symb, job.Symb);
				List.insert(&item);
			}
		}
		List.sort(CMPF_LONG);
	}
	return ok;
}
