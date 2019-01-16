// V_LOGSMON.CPP
// A. Kurilov 2008, 2009, 2015, 2016, 2018
//
#include <pp.h>
#pragma hdrstop
//
//virtual
int LogsMonitorFilt::Describe(long flags, SString & rBuff) const
{
	int    ok = 1;
	rBuff.Z();
	for(uint i=0; i<Selected.getCount(); i++) {
		if(i>2) {
			rBuff.CatCharN('.', 3);
			break;
		}
		rBuff.Cat(Selected.at(i).FileName).Comma();
	}
	return ok;
}
//
class SelectLogsDialog : public Lst2LstAryDialog {
public:
	SelectLogsDialog(ListToListUIData *, SArray *pLList, SArray *pRList);
	virtual SArray * getSelected();
};
//
class LogsMonitorFiltDialog : public PPListDialog {
public:
	explicit LogsMonitorFiltDialog(uint resID);
	int	setDTS(const LogsMonitorFilt *);
	int	getDTS(LogsMonitorFilt *);
private:
	DECL_HANDLE_EVENT;
	virtual int setupList();
	//virtual int	actionList();
	LogsMonitorFilt Filt;
	LogsArray SelectedLogs;
};
//
void LoadAllLogs(LogsArray *pLogsBuff)
{
	SString buff;
	PPLoadText(PPTXT_LOGSNAMES, buff);
	StringSet ss(';', buff);
	if(pLogsBuff) {
		pLogsBuff->clear();
		for(uint i = 0; ss.get(&i, buff);) {
			uint j = 0;
			LogFileEntry e;
			MEMSZERO(e);
			StringSet ss1(',', buff);
			ss1.get(&j, buff.Z());
			e.ID = buff.ToLong();
			ss1.get(&j, buff.Z());
			buff.CopyTo(e.LogName, sizeof(e.LogName));
			ss1.get(&j, buff.Z());
			buff.CopyTo(e.FileName, sizeof(e.FileName));
			pLogsBuff->insert(&e);
		}
	}
}
//
// @implement LogsMonitorFilt[Dialog] {
//
IMPLEMENT_PPFILT_FACTORY(LogsMonitor); LogsMonitorFilt::LogsMonitorFilt() : PPBaseFilt(PPFILT_LOGSMONITOR, 0, 0)
{
	SetFlatChunk(offsetof(LogsMonitorFilt, ReserveStart), offsetof(LogsMonitorFilt, Reserve) - offsetof(LogsMonitorFilt, ReserveStart));
	SetBranchSArray(offsetof(LogsMonitorFilt, Selected));
	Init(1, 0);
}
//
LogsMonitorFiltDialog::LogsMonitorFiltDialog(uint resID) : PPListDialog(DLG_LOGSMON, CTL_LOGSMON_LIST)
{
	// SetupCalPeriod(CTLCAL_LOGSMONFILT_PERIOD, CTL_LOGSMONFILT_PERIOD);
}
//
IMPL_HANDLE_EVENT(LogsMonitorFiltDialog)
{
	PPListDialog::handleEvent(event);
	if(event.isCmd(cmEditLogsList)) {
		LogsArray l_logs, r_logs;
		LoadAllLogs(&l_logs);
		ListToListAryData lists_data(0, &l_logs, &r_logs);
		SelectLogsDialog *p_dlg = 0;
		if(CheckDialogPtrErr(&(p_dlg = new SelectLogsDialog(&lists_data, &l_logs, &r_logs)))) {
			if(ExecView(p_dlg) == cmOK) {
				SArray *p_selected = p_dlg->getSelected();
				SelectedLogs.clear();
				if(p_selected) {
					for(uint i=0; i<p_selected->getCount(); i++)
						SelectedLogs.insert(p_selected->at(i));
					updateList(-1);
				}
			}
			delete p_dlg;
		}
		clearEvent(event);
	}
}
//
int LogsMonitorFiltDialog::setupList()
{
	int    ok = 1;
	SString path;
	PPGetPath(PPPATH_LOG, path);
	path.SetLastSlash();
	for(uint i=0; ok && i<SelectedLogs.getCount(); i++) {
		const LogFileEntry *p_entry = &SelectedLogs.at(i);
		StringSet ss(SLBColumnDelim);
		ss.add(p_entry->FileName);
		ss.add(p_entry->LogName);
		if(!addStringToList(SelectedLogs.at(i).ID, ss.getBuf()))
			ok = 0;
	}
	return ok;
}
//
#if 0
int LogsMonitorFiltDialog::actionList()
{
	ListToListData data(PPOBJ_ACTION, -1, &Filt.ActionIDList);
	data.TitleStrID = 0;
	if(ListToListDialog(&data) > 0) {
		if(Filt.ActionIDList.isList()) {
			SetComboBoxListText(this, CTLSEL_LOGSMON_ACTION);
			disableCtrl(CTLSEL_LOGSMON_ACTION, 1);
		}
		else {
			setCtrlLong(CTLSEL_LOGSMON_ACTION, Filt.ActionIDList.getSingle());
			disableCtrl(CTLSEL_LOGSMON_ACTION, 0);
		}
		return 1;
	}
	else
		return -1;
}
#endif // 0
//
int LogsMonitorFiltDialog::setDTS(const LogsMonitorFilt *pFilt)
{
	Filt = *pFilt;
	return 1;
}
//
int LogsMonitorFiltDialog::getDTS(LogsMonitorFilt *pFilt)
{
	Filt.Selected = SelectedLogs;
	*pFilt = Filt;
	return 1;
}
//
// } @implement LogsMonitorFilt[Dialog]
//
// @implement SelectLogsDialog {
//
SelectLogsDialog::SelectLogsDialog(ListToListUIData *pData, SArray *pLList, SArray *pRList) : Lst2LstAryDialog(DLG_SELECT_LOGS, pData, pLList, pRList)
{
}
//
SArray * SelectLogsDialog::getSelected()
{
	return GetRight();
}
//
// } @implement SelectLogsDialog
//
// @implement PPViewLogsMonitor {
//
int SLAPI PPViewLogsMonitor::EditBaseFilt(PPBaseFilt *pBaseFilt)
{
	if(!Filt.IsA(pBaseFilt))
		return 0;
	LogsMonitorFilt *p_filt = (LogsMonitorFilt *)pBaseFilt;
	DIALOG_PROC_BODY_P1(LogsMonitorFiltDialog, DLG_LOGSMON, p_filt);
}
//
PP_CREATE_TEMP_FILE_PROC(CreateTempFile, TempLogFileMon);
//
SLAPI PPViewLogsMonitor::PPViewLogsMonitor() : PPView(0, &Filt, PPVIEW_LOGSMONITOR), P_TmpTbl(0), FirstTime(1)
{
	// смещения строк для всех журналов установить равным 0
	LogsArray all_logs;
	LoadAllLogs(&all_logs);
	for(uint i=0; i<all_logs.getCount(); i++)
		LogsOffsets.AddUnique(all_logs.at(i).ID, 0, 0);
}
//
SLAPI PPViewLogsMonitor::~PPViewLogsMonitor()
{
	delete P_TmpTbl;
}
//
int SLAPI PPViewLogsMonitor::InitIteration()
{
	int    ok = 1;
	DBQ  * dbq = 0;
	TempLogFileMonTbl::Key0 k, ks;
	MEMSZERO(k);
	BExtQuery::ZDelete(&P_IterQuery);
	if(P_TmpTbl) {
		P_IterQuery = new BExtQuery(P_TmpTbl, 0);
		P_IterQuery->selectAll();
		ks = k;
		Counter.Init(P_IterQuery->countIterations(0, &ks, spGe));
		P_IterQuery->initIteration(0, &k, spGe);
	}
	else
		ok = 0;
	return ok;
}
//
int FASTCALL PPViewLogsMonitor::NextIteration(LogsMonitorViewItem * pItem)
{
	int    ret = 1;
	while(pItem && P_IterQuery && P_IterQuery->nextIteration() > 0) {
		P_TmpTbl->copyBufTo(pItem);
		Counter.Increment();
	}
	return ret;
}
//
int SLAPI PPViewLogsMonitor::Init_(const PPBaseFilt *pFilt)
{
	P_TmpTbl = CreateTempFile();
	if(Helper_InitBaseFilt(pFilt))
		return this->UpdateTempTable(&Filt.Selected);
	else
		return -1;
}
//
DBQuery * SLAPI PPViewLogsMonitor::CreateBrowserQuery(uint *pBrwId, SString *)
{
	DBQuery	 *q = 0;
	DBE		 dbe_logfname;
	TempLogFileMonTbl *p_tbl = new TempLogFileMonTbl(P_TmpTbl->GetName());
	PPDbqFuncPool::InitObjNameFunc(dbe_logfname, PPDbqFuncPool::IdLogFileName, p_tbl->LogFileId);
	if(p_tbl) {
		q = & select (
			p_tbl->ID__,    // #0
			dbe_logfname,	// #1
			p_tbl->LineNo,	// #2
			p_tbl->Dt,		// #3
			p_tbl->Tm,		// #4
			p_tbl->UserName,// #5
			p_tbl->DbSymb,	// #6
			p_tbl->Text,	// #7
			0).from(p_tbl, 0).orderBy(p_tbl->Dt, p_tbl->Tm, 0);
		ASSIGN_PTR(pBrwId, BROWSER_LOGSMON);
	}
	return q;
}
//
int SLAPI PPViewLogsMonitor::ProcessCommand(uint ppvCmd, const void *pHdr, PPViewBrowser *pBrw)
{
	int ok = PPView::ProcessCommand(ppvCmd, pHdr, pBrw);
	if(ok == -2) {
		BrwHdr hdr;
		if(pHdr)
			hdr = *(PPViewLogsMonitor::BrwHdr *)pHdr;
		else
			MEMSZERO(hdr);
	}
	return ok;
}
//
#if 0
int SLAPI PPViewLogsMonitor::Print(const void *)
{
	return Helper_Print(REPORT_LOGSMON, 0);
}
#endif // 0
//
void SLAPI PPViewLogsMonitor::PreprocessBrowser(PPViewBrowser *pBrw)
{
	CALLPTRMEMB(pBrw, Advise(PPAdviseBlock::evLogsChanged, 0, -1, 0));
}
//
int SLAPI PPViewLogsMonitor::HandleNotifyEvent(int kind, const PPNotifyEvent *pEv, PPViewBrowser *pBrw, void * extraProcPtr)
{
	int    ok = 1, update = 0;
	if(FirstTime) {
		pBrw->bottom();
		FirstTime = 0;
	}
	if(pEv && kind==PPAdviseBlock::evLogsChanged) {
		LogsArray expired_logs;
		SString log_fname;
		SString logs_fpath;	PPGetPath(PPPATH_LOG, logs_fpath);
		SFileUtil::Stat log_file_stat;
		// из тех журналов, что отмечены в фильтре получить список измененных в ExpiredLogs
		for(uint i = 0; i < Filt.Selected.getCount(); i++) {
			(log_fname = logs_fpath).Cat(Filt.Selected.at(i).FileName);
			SFileUtil::GetStat(log_fname, &log_file_stat);
			if(cmp(log_file_stat.ModTime, this->LastUpdated)>0) {
				expired_logs.insert(&Filt.Selected.at(i));
				update = 1;
			}
		}
		if(update && pBrw) {
			if(this->UpdateTempTable(&expired_logs)) {
				pBrw->Update();
				pBrw->bottom();
			}
			else
				ok = 0;
		}
	}
	return ok;
}
//
int PPViewLogsMonitor::UpdateTempTable(LogsArray *pExpiredLogs)
{
	int    ok = 1;
	if(pExpiredLogs && P_TmpTbl) {
		//
		uint i, pos;
		long j, offset;
		SFile log_file;
		FILE *p_log_file;
		SString log_fname;
		SString logs_fpath;	PPGetPath(PPPATH_LOG, logs_fpath);
		SString log_line;
		SString tail;
		SString date;
		SString time;
		SString t;
		TempLogFileMonTbl::Rec log_record;
		//
		for(i = 0; i < pExpiredLogs->getCount(); i++) {
			(log_fname = logs_fpath).Cat(pExpiredLogs->at(i).FileName);
			if(log_file.Open(log_fname, SFile::mRead) && LogsOffsets.Search(Filt.Selected.at(i).ID, &offset, &pos)) {
				// "быстро" переместиться в нужную строку
				const size_t block_size = 0x100; // изначально читать достаточно большими блоками
				p_log_file = log_file.operator FILE *();
				char c, cbuff[0x100];
				for(j = 0; j < offset; ) { // j: номер строки в файле
					if(size_t(offset - j) < block_size) { // если "конец близок" то читать по одному байту
						if(fread(&c, 1, 1, p_log_file)) {
							if(c=='\n')
								j ++;
						}
					}
					else if(fread(cbuff, block_size, 1, p_log_file)==1) {
						for(pos = 0; pos < block_size; pos ++) {
							if(cbuff[pos]=='\n')
								j ++;
						}
					}
					else
						j = offset; // чтобы не повисло в случае если весь файл является одной строкой
				}
				// читаем начиная с нужной строки
				for(/* j=offset */; log_file.ReadLine(log_line); j++) {
					MEMSZERO(log_record);
					log_record.LogFileId = Filt.Selected.at(i).ID;
					log_record.LineNo = j;
					// #0 обязательно дата до 1-го пробела
					if(log_line.Divide(' ', date, tail) > 0 && date.Len()>4) {
						strtodate(date, 0, &log_record.Dt);
						// #1 обязательно время до 1-й табуляции
						if(tail.Divide('\t', time, log_line) > 0 && time.Len()>4) {
							strtotime(time, 0, &log_record.Tm);
							// #2 необязательно имя пользователя до 2-й табуляции
							if(log_line.Divide('\t', t, tail) > 0)
								t.ToOem().CopyTo(log_record.UserName, sizeof(log_record.UserName));
							else
								tail = log_line;
							// #3 необязательно имя базы до 3-й табуляции
							if(tail.Divide('\t', t, log_line) > 0)
								t.ToOem().CopyTo(log_record.DbSymb, sizeof(log_record.DbSymb));
							else
								log_line = tail;
							// #4 обязательно сообщение до конца
							if(log_line.NotEmpty()) {
								log_line.ToOem().CopyTo(log_record.Text, sizeof(log_record.Text));
								THROW_DB(P_TmpTbl->insertRecBuf(&log_record));
							}
						}
					}
				}
				LogsOffsets.Update(log_record.LogFileId, j);
				log_file.Close();
			}
		}
		getcurdatetime(&this->LastUpdated);
	}
	CATCHZOK
	return ok;
}
//
// } @implement PPViewLogsMonitor
//
