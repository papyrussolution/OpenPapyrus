// V_JOBP.CPP
// Copyright (c) A.Sobolev 2005, 2007, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018
// @codepage UTF-8
//
// Редактирование списка процессорных задач
//
#include <pp.h>
#pragma hdrstop
//
//
//
class JobItemDialog : public TDialog {
public:
	JobItemDialog(PPJobMngr * pMngr, PPJobPool * pJobPool) : TDialog(DLG_JOBITEM), P_Mngr(pMngr), P_JobPool(pJobPool)
	{
		P_Mngr->GetResourceList(0, &CmdSymbList);
		if(P_JobPool) {
			PPJob job;
			for(PPID id = 0; P_JobPool->Enum(&id, &job) > 0;)
				JobList.Add(job.ID, job.Name);
		}
		SetupTimePicker(this, CTL_JOBITEM_SCHDLBEFORE, CTLTM_JOBITEM_SCHDLBEFORE);
	}
	int    setDTS(const PPJob *);
	int    getDTS(PPJob *);
private:
	DECL_HANDLE_EVENT;
	void   editSchedule();
	int    CheckRecursion(const PPJob * pData);

	PPJob  Data;
	PPJobPool * P_JobPool;
	PPJobMngr * P_Mngr;
	StrAssocArray CmdSymbList;
	StrAssocArray JobList;
};

IMPL_HANDLE_EVENT(JobItemDialog)
{
	TDialog::handleEvent(event);
	if(TVCOMMAND)
		if(event.isCbSelected(CTLSEL_JOBITEM_CMD)) {
			PPJobDescr job_descr;
			PPID   job_id = getCtrlLong(CTLSEL_JOBITEM_CMD);
			if(job_id && P_Mngr->LoadResource(job_id, &job_descr) > 0) {
				enableCommand(cmJobParam, !(job_descr.Flags & PPJobDescr::fNoParam));
				SString name_buf;
				getCtrlString(CTL_JOBITEM_NAME, name_buf);
				if(!name_buf.NotEmptyS())
					setCtrlString(CTL_JOBITEM_NAME, name_buf = job_descr.Text);
			}
			else
				enableCommand(cmJobParam, 0);
		}
		// @v8.2.5 {
		else if(event.isClusterClk(CTL_JOBITEM_FLAGS)) {
			GetClusterData(CTL_JOBITEM_FLAGS, &Data.Flags);
			enableCommand(cmSchedule, !(Data.Flags & PPJob::fUnSheduled));
			DisableClusterItem(CTL_JOBITEM_FLAGS, 5, (Data.Flags & PPJob::fUnSheduled)); // @v9.2.11
			disableCtrl(CTL_JOBITEM_SCHDLBEFORE, (Data.Flags & PPJob::fUnSheduled)); // @v9.2.11
		}
		// } @v8.2.5
		else if(TVCMD == cmSchedule)
			editSchedule();
		else if(TVCMD == cmJobParam)
			P_Mngr->EditJobParam(getCtrlLong(CTLSEL_JOBITEM_CMD), &Data.Param);
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

int JobItemDialog::setDTS(const PPJob * pData)
{
	int    ok = 1;
	int    db_id = 0;
	PPDbEntrySet2 dbes;
	PPIniFile ini_file;
	SString db_symb, db_name;
	StrAssocArray cmd_txt_list;
	PPID   cmd_id = 0;
	Data = *pData;
	setCtrlString(CTL_JOBITEM_NAME, Data.Name);
	setCtrlData(CTL_JOBITEM_SYMB, Data.Symb);
	setCtrlLong(CTL_JOBITEM_ID, Data.ID);
	AddClusterAssoc(CTL_JOBITEM_FLAGS, 0, PPJob::fNotifyByMail);
	AddClusterAssoc(CTL_JOBITEM_FLAGS, 1, PPJob::fDisable);
	AddClusterAssoc(CTL_JOBITEM_FLAGS, 2, PPJob::fOnStartUp);
	AddClusterAssoc(CTL_JOBITEM_FLAGS, 3, PPJob::fPermanent);
	AddClusterAssoc(CTL_JOBITEM_FLAGS, 4, PPJob::fUnSheduled); // @v8.2.5
	AddClusterAssoc(CTL_JOBITEM_FLAGS, 5, PPJob::fSkipEmptyNotification); // @v8.2.11
	SetClusterData(CTL_JOBITEM_FLAGS, Data.Flags);
	setCtrlTime(CTL_JOBITEM_SCHDLBEFORE, Data.ScheduleBeforeTime); // @v9.2.11
	dbes.ReadFromProfile(&ini_file);
	if(Data.DbSymb.NotEmpty()) {
		db_id = dbes.GetBySymb(Data.DbSymb, 0);
		if(db_id)
			dbes.SetSelection(db_id);
	}
	SetupDBEntryComboBox(this, CTLSEL_JOBITEM_DBSYMB, &dbes);
	if(db_id == 0 && Data.DbSymb.NotEmpty())
		setCtrlString(CTL_JOBITEM_DBSYMB, Data.DbSymb);
	disableCtrl(CTLSEL_JOBITEM_DBSYMB, 1);
	P_Mngr->GetResourceList(1, &cmd_txt_list);
	cmd_txt_list.SortByText();
	uint   pos = 0;
	if(CmdSymbList.SearchByText(Data.Descr.Symb, 1, &pos))
		cmd_id = CmdSymbList.Get(pos).Id;
	SetupStrAssocCombo(this, CTLSEL_JOBITEM_CMD, &cmd_txt_list, cmd_id, 0);
	SetupStrAssocCombo(this, CTLSEL_JOBITEM_NEXTJOB, &JobList, Data.NextJobID, 0, 0, 0);
	disableCtrl(CTLSEL_JOBITEM_CMD, cmd_id);
	{
		PPJobDescr job_descr;
		if(cmd_id && P_Mngr->LoadResource(cmd_id, &job_descr) > 0)
			enableCommand(cmJobParam, !(job_descr.Flags & PPJobDescr::fNoParam));
		else
			enableCommand(cmJobParam, 0);
		enableCommand(cmSchedule, !(Data.Flags & PPJob::fUnSheduled)); // @v8.2.5
		DisableClusterItem(CTL_JOBITEM_FLAGS, 5, (Data.Flags & PPJob::fUnSheduled)); // @v9.2.11
		disableCtrl(CTL_JOBITEM_SCHDLBEFORE, (Data.Flags & PPJob::fUnSheduled)); // @v9.2.11
	}
	return ok;
}

int JobItemDialog::getDTS(PPJob * pData)
{
	int    ok = 1;
	PPID   cmd_id = 0;
	uint   sel = 0;
	getCtrlString(sel = CTL_JOBITEM_NAME, Data.Name);
	THROW_PP(Data.Name.NotEmptyS(), PPERR_NAMENEEDED);
	getCtrlData(sel = CTL_JOBITEM_SYMB, Data.Symb);
	THROW(P_JobPool->CheckUniqueJob(&Data));
	getCtrlData(sel = CTLSEL_JOBITEM_CMD, &cmd_id);
	THROW_PP(CmdSymbList.Search(cmd_id), PPERR_INVJOBCMD);
	THROW(P_Mngr->LoadResource(cmd_id, &Data.Descr));
	getCtrlData(sel = CTLSEL_JOBITEM_NEXTJOB, &Data.NextJobID);
	GetClusterData(CTL_JOBITEM_FLAGS, &Data.Flags);
	Data.ScheduleBeforeTime = getCtrlTime(CTL_JOBITEM_SCHDLBEFORE); // @v9.2.11
	THROW(CheckRecursion(&Data));
	ASSIGN_PTR(pData, Data);
	CATCH
		ok = PPErrorByDialog(this, sel);
	ENDCATCH
	return ok;
}

int JobItemDialog::CheckRecursion(const PPJob * pData)
{
	int    ok = 1;
	if(pData && P_JobPool) {
		const PPJob * p_job = pData;
		PPIDArray job_list;
		do {
			THROW_PP(job_list.lsearch(p_job->ID) <= 0, PPERR_JOBITEMLOOP);
			THROW_SL(job_list.add(p_job->ID));
			p_job = p_job->NextJobID ? P_JobPool->GetJob(p_job->NextJobID) : 0;
		} while(p_job);
	}
	CATCHZOK
	return ok;
}

int SLAPI EditJobItem(PPJobMngr * pMngr, PPJobPool * pJobPool, PPJob * pData) { DIALOG_PROC_BODY_P2(JobItemDialog, pMngr, pJobPool, pData); }
//
//
//
class JobPoolDialog : public PPListDialog {
public:
	JobPoolDialog(PPJobMngr * pMngr, PPJobPool * pData) : PPListDialog(DLG_JOBPOOL, CTL_JOBPOOL_LIST), P_Mngr(pMngr), P_Data(pData), ForAllDb(0)
	{
		updateList(-1);
	}
private:
	DECL_HANDLE_EVENT
	{
		PPListDialog::handleEvent(event);
		if(event.isCmd(cmPrint)) {
			PView pv(P_Data);
			PPAlddPrint(REPORT_JOBPOOL, &pv, 0);
		}
		else if(event.isClusterClk(CTL_JOBPOOL_FLAGS)) {
			ForAllDb = BIN(getCtrlUInt16(CTL_JOBPOOL_FLAGS) & 0x0001);
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
	int    ForAllDb;    // Показывать задачи для всех баз данных
};

int JobPoolDialog::setupList()
{
	int    ok = 1;
	SString save_db_symb;
	PPJob  job;
	SString buf;
	StringSet ss(SLBColumnDelim);
	for(PPID id = 0; ok && P_Data->Enum(&id, &job, ForAllDb);) {
		ss.clear();
		(ss += job.Name) += job.Descr.Symb;
		ss += job.Dtr.Format(0, buf);
		if(job.NextJobID) {
			const PPJob * p_job = P_Data->GetJob(job.NextJobID, ForAllDb);
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
		THROW(P_Data->PutJob(&id, &job));
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
		const PPJob * p_job = P_Data->GetJob(id, ForAllDb);
		if(p_job) {
			PPJob job = *p_job;
			if(EditJobItem(P_Mngr, P_Data, &job) > 0)
				ok = P_Data->PutJob(&id, &job);
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
	int    ok = CheckCfgRights(PPCFGOBJ_JOBPOOL, PPR_DEL, 0) > 0 ? P_Data->PutJob(&id, 0) : 0;
	if(!ok)
		PPError();
	return ok;
}

int SLAPI ViewJobPool()
{
	MemLeakTracer mlt;
	int    ok = -1;
	PPJobMngr mngr;
	PPJobPool pool(&mngr, 0, 0);
	SString db_symb;
	JobPoolDialog * dlg = 0;
	THROW(CheckCfgRights(PPCFGOBJ_JOBPOOL, PPR_READ, 0));
	THROW_PP(CurDict->GetDbSymb(db_symb) > 0, PPERR_DBSYMBUNDEF);
	THROW(mngr.LoadPool(db_symb, &pool, 0));
	THROW(CheckDialogPtrErr(&(dlg = new JobPoolDialog(&mngr, &pool))));
	while(ExecView(dlg) == cmOK) {
		THROW(CheckCfgRights(PPCFGOBJ_JOBPOOL, PPR_MOD, 0));
		if(mngr.SavePool(&pool)) {
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
	int ok = -1;
	IterProlog(iterId, 0);
	long n = (uint)I.nn;
	PPJob item;
	PPJobPool * p_jobpool = (PPJobPool*)Extra[1].Ptr;
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
IMPLEMENT_PPFILT_FACTORY(Job); SLAPI JobFilt::JobFilt() : PPBaseFilt(PPFILT_JOB, 0, 1)
{
	SetFlatChunk(offsetof(JobFilt, ReserveStart),
		offsetof(JobFilt, ReserveEnd)-offsetof(JobFilt, ReserveStart)+sizeof(ReserveEnd));
	Init(1, 0);
}

SLAPI PPViewJob::PPViewJob() : PPView(0, &Filt, PPVIEW_JOB), IsChanged(0), P_Pool(0)
{
	ImplementFlags |= (implBrowseArray|implDontEditNullFilter);
	LoadPool();
}

SLAPI PPViewJob::~PPViewJob()
{
	if(IsChanged == 1 && CONFIRM(PPCFM_DATACHANGED))
		SavePool();
	ZDELETE(P_Pool);
}

int SLAPI PPViewJob::LoadPool()
{
	int    ok = 1;
	SString db_symb;
	SavePool();
	ZDELETE(P_Pool);
	THROW_MEM(P_Pool = new PPJobPool(&Mngr, 0, 0));
	THROW_PP(CurDict->GetDbSymb(db_symb) > 0, PPERR_DBSYMBUNDEF);
	THROW(Mngr.LoadPool(db_symb, P_Pool, 0));
	Mngr.GetResourceList(0, &CmdSymbList);
	CATCH
		ZDELETE(P_Pool);
		ok = PPErrorZ();
	ENDCATCH
	return ok;
}

int SLAPI PPViewJob::SavePool()
{
	int    ok = -1;
	if(P_Pool) {
		if(Mngr.SavePool(P_Pool)) {
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
public:
	JobFiltDialog(PPJobMngr * pMngr, PPJobPool * pJobPool) : TDialog(DLG_JOBFILT), P_Mngr(pMngr), P_Pool(pJobPool)
	{
		P_Mngr->GetResourceList(0, &CmdSymbList);
	}
	int setDTS(const JobFilt * pData)
	{
		if(!RVALUEPTR(Data, pData))
			Data.Init(1, 0);
		{
			StrAssocArray cmd_txt_list;
			P_Mngr->GetResourceList(1, &cmd_txt_list);
			cmd_txt_list.SortByText();
			SetupStrAssocCombo(this, CTLSEL_JOBFILT_CMD, &cmd_txt_list, Data.CmdId, 0);
		}
		AddClusterAssoc(CTL_JOBFILT_FLAGS, 0, JobFilt::fForAllDb);
		SetClusterData(CTL_JOBFILT_FLAGS, Data.Flags);
		return 1;
	}
	int getDTS(JobFilt * pData)
	{
		int ok = 1;
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
	JobFilt Data;
	StrAssocArray CmdSymbList;
	PPJobMngr * P_Mngr;
	PPJobPool * P_Pool;
};

int SLAPI PPViewJob::EditBaseFilt(PPBaseFilt * pFilt)
{
	DIALOG_PROC_BODY_P2ERR(JobFiltDialog, &Mngr, P_Pool, (JobFilt*)pFilt);
}

int SLAPI PPViewJob::Init_(const PPBaseFilt * pFilt)
{
	int    ok = 1;
	THROW(Helper_InitBaseFilt(pFilt));
	THROW(MakeList());
	CATCHZOK
	return ok;
}

int SLAPI PPViewJob::InitIteration()
{
	Counter.Init(List.getCount());
	return 1;
}

int FASTCALL PPViewJob::NextIteration(JobViewItem * pItem)
{
	int    ok = -1;
	if(pItem) {
		while(Counter < Counter.GetTotal() && ok < 0) {
			if(pItem) {
				ASSIGN_PTR(pItem, List.at(Counter));
				ok = 1;
			}
			else
				ok = 1;
			Counter.Increment();
		}
	}
	return ok;
}

int SLAPI PPViewJob::AddItem(PPID * pID)
{
	int    ok = -1;
	if(P_Pool) {
		PPJob  job;
		THROW(CheckCfgRights(PPCFGOBJ_JOBPOOL, PPR_INS, 0));
		job.DbSymb = P_Pool->GetDbSymb();
		if(EditJobItem(&Mngr, P_Pool, &job) > 0) {
			PPID   id = 0;
			THROW(P_Pool->PutJob(&id, &job));
			ASSIGN_PTR(pID, id);
			IsChanged = 1;
			ok = 1;
		}
	}
	CATCHZOKPPERR
	return ok;
}

int SLAPI PPViewJob::EditItem(PPID id)
{
	int    ok = -1;
	if(P_Pool && CheckCfgRights(PPCFGOBJ_JOBPOOL, PPR_MOD, 0)) {
		const PPJob * p_job = P_Pool->GetJob(id, (Filt.Flags & JobFilt::fForAllDb));
		if(p_job) {
			PPJob job = *p_job;
			if(EditJobItem(&Mngr, P_Pool, &job) > 0)
				ok = P_Pool->PutJob(&id, &job);
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

int SLAPI PPViewJob::DeleteItem(PPID id)
{
	int    ok = (P_Pool) ? (CheckCfgRights(PPCFGOBJ_JOBPOOL, PPR_DEL, 0) > 0 ? P_Pool->PutJob(&id, 0) : 0) : -1;
	if(ok > 0)
		IsChanged = 1;
	else if(!ok)
		PPError();
	return ok;
}

// virtual
SArray  * SLAPI PPViewJob::CreateBrowserArray(uint * pBrwId, SString * pSubTitle)
{
	SArray * p_array = new SArray(List);
	uint   brw_id = BROWSER_JOB;
	ASSIGN_PTR(pBrwId, brw_id);
	return p_array;
}

int SLAPI PPViewJob::_GetDataForBrowser(SBrowserDataProcBlock * pBlk)
{
	int    ok = 0;
	if(pBlk->P_SrcData && pBlk->P_DestData) {
		ok = 1;
		JobViewItem * p_item = (JobViewItem*)pBlk->P_SrcData;
		int    r = 0;
		switch(pBlk->ColumnN) {
			case 0: // @id
				pBlk->Set(p_item->ID);
				break;
			case 1: // @name
				pBlk->Set(p_item->Name);
				break;
			case 2: // @symb
				pBlk->Set(p_item->Symb);
				break;
			case 3: // @datetime repeating
				pBlk->Set(p_item->Dtr);
				break;
			case 4: // @next job
				pBlk->Set(p_item->NextJob);
				break;
			case 5: // @database symbol
				pBlk->Set(p_item->DbSymb);
				break;
		}
	}
	return ok;
}

// static
int PPViewJob::GetDataForBrowser(SBrowserDataProcBlock * pBlk)
{
	PPViewJob * p_v = (PPViewJob *)pBlk->ExtraPtr;
	return p_v ? p_v->_GetDataForBrowser(pBlk) : 0;
}

void SLAPI PPViewJob::PreprocessBrowser(PPViewBrowser * pBrw)
{
	CALLPTRMEMB(pBrw, SetDefUserProc(PPViewJob::GetDataForBrowser, this));
}

// virtual
int SLAPI PPViewJob::ProcessCommand(uint ppvCmd, const void * pHdr, PPViewBrowser * pBrw)
{
	int    ok = (ppvCmd == PPVCMD_PRINT) ? -2 : PPView::ProcessCommand(ppvCmd, pHdr, pBrw);
	if(ok == -2) {
		AryBrowserDef * p_def = pBrw ? (AryBrowserDef *)pBrw->getDef() : 0;
		const  long cur_pos = p_def ? p_def->_curItem() : 0;
		long   update_pos = cur_pos;
		PPID   update_id = 0;
		PPID   job_id = pHdr ? *(long *)pHdr : 0;
		switch(ppvCmd) {
			case PPVCMD_ADDITEM:
				ok = AddItem(&update_id);
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

// virtual
int SLAPI PPViewJob::Print(const void *)
{
	int    ok = -1;
	if(P_Pool) {
		PView pv(P_Pool);
		ok = PPAlddPrint(REPORT_JOBPOOL, &pv, 0);
	}
	return ok;
}

int SLAPI PPViewJob::CheckForFilt(PPJob & rJob)
{
	int    r = 1;
	if(Filt.CmdId) {
		uint pos = 0;
		PPID cmd_id = 0;
		if(CmdSymbList.SearchByText(rJob.Descr.Symb, 1, &pos))
			cmd_id = CmdSymbList.Get(pos).Id;
		r = BIN(Filt.CmdId == cmd_id);
	}
	return r;
}

int SLAPI PPViewJob::MakeList()
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
					const PPJob * p_job = P_Pool->GetJob(job.NextJobID, Filt.Flags & JobFilt::fForAllDb);
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
