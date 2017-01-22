// PPSESS.CPP
// Copyright (c) A.Sobolev 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017
//
#include <pp.h>
#pragma hdrstop
#include <private\_ppo.h>
#include <idea.h>
#include <float.h>
#include <signal.h>
#include <ppsoapclient.h>
//
//
//
#ifdef NDEBUG
	#define USE_ADVEVQUEUE 1
	const int __UseAdvEvQueue = 1;
#else
	#define USE_ADVEVQUEUE 1
	const int __UseAdvEvQueue = 1;
#endif
//
//
//
class DbDict_DL600 : public DbDictionary {
public:
	static DbDictionary * CreateInstance(const char * pPath, long options);

	SLAPI  DbDict_DL600() : DbDictionary()
	{
	}
	virtual int SLAPI LoadTableSpec(DBTable * pTbl, const char * pTblName);
	virtual int SLAPI CreateTableSpec(DBTable * pTbl);
	virtual int SLAPI DropTableSpec(const char * pTblName, DbTableStat * pStat);
	virtual int SLAPI GetTableID(const char * pTblName, long * pID, DbTableStat * pStat);
	virtual int SLAPI GetTableInfo(long tblID, DbTableStat * pStat);
	virtual int SLAPI GetListOfTables(long options, StrAssocArray * pList);
private:
	int    SLAPI ExtractStat(DlContext * pCtx, const DlScope * pScope, DbTableStat * pStat) const;
};
//
// Descr: Процедура создания словаря DbDict_DL600. Используется для установки
//   функцией DbDictionary::SetCreateInstanceProc
//
/*static*/ DbDictionary * DbDict_DL600::CreateInstance(const char * pPath, long options)
{
	return new DbDict_DL600;
}

int SLAPI DbDict_DL600::LoadTableSpec(DBTable * pTbl, const char * pTblName)
{
	int    ok = 1;
	DlContext * p_ctx = DS.GetInterfaceContext(PPSession::ctxDatabase);
	THROW(p_ctx);
	THROW(p_ctx->LoadDbTableSpec(pTblName, pTbl, 0));
	CATCHZOK
	return ok;
}

int SLAPI DbDict_DL600::CreateTableSpec(DBTable * pTbl)
{
	int    ok = 1;
	if(pTbl->tableName[0] == 0) {
		SString tbl_name;
		ulong  t = SLS.GetTLA().Rg.Get();
		(tbl_name = "CT").Cat(t).CopyTo(pTbl->tableName, sizeof(pTbl->tableName));
	}
	DlContext * p_ctx = DS.GetInterfaceContext(PPSession::ctxDatabase);
	THROW(p_ctx);
	THROW(p_ctx->CreateNewDbTableSpec(pTbl));
	CATCHZOK
	return ok;
}

int SLAPI DbDict_DL600::DropTableSpec(const char * pTblName, DbTableStat * pStat)
{
	int    ok = 1;
	DlContext * p_ctx = DS.GetInterfaceContext(PPSession::ctxDatabase);
	THROW(p_ctx);
	THROW(p_ctx->DropDbTableSpec(pTblName));
	CATCHZOK
	return ok;
}

int SLAPI DbDict_DL600::ExtractStat(DlContext * pCtx, const DlScope * pScope, DbTableStat * pStat) const
{
	CtmExprConst c;
	pStat->Clear();
	pStat->ID = pScope->GetId();
	pStat->TblName = pScope->GetName();
	if(pScope->GetConst(DlScope::cdbtFileName, &c)) {
		char   s_buf[256];
		s_buf[0] = 0;
		pCtx->GetConstData(c, s_buf, sizeof(s_buf));
		pStat->Location = s_buf;
	}
	if(pScope->GetConst(DlScope::cdbtAccess, &c)) {
		uint32 accs = 0;
		pCtx->GetConstData(c, &accs, sizeof(accs));
		pStat->OwnerLevel = accs;
	}
	if(pScope->GetAttrib(DlScope::sfDbtVLR, 0))
		pStat->Flags |= XTF_VLR;
	if(pScope->GetAttrib(DlScope::sfDbtVAT, 0))
		pStat->Flags |= XTF_VAT;
	if(pScope->GetAttrib(DlScope::sfDbtTruncate, 0))
		pStat->Flags |= XTF_TRUNCATE;
	if(pScope->GetAttrib(DlScope::sfDbtCompress, 0))
		pStat->Flags |= XTF_COMPRESS;
	if(pScope->GetAttrib(DlScope::sfDbtBalanced, 0))
		pStat->Flags |= XTF_BALANCED;
	if(pScope->GetAttrib(DlScope::sfDbtTemporary, 0))
		pStat->Flags |= XTF_TEMP;
	if(pScope->GetAttrib(DlScope::sfDbtSystem, 0))
		pStat->Flags |= XTF_DICT;
	pStat->RetItems = (DbTableStat::iID|DbTableStat::iOwnerLevel|DbTableStat::iFlags|DbTableStat::iName|DbTableStat::iLocation);
	return 1;
}

int SLAPI DbDict_DL600::GetTableID(const char * pTblName, long * pID, DbTableStat * pStat)
{
	int    ok = 1;
	DlContext * p_ctx = DS.GetInterfaceContext(PPSession::ctxDatabase);
	THROW(p_ctx);
	const DlScope * p_scope = p_ctx->GetScopeByName_Const(DlScope::kDbTable, pTblName);
	THROW(p_scope);
	ASSIGN_PTR(pID, p_scope->GetId());
	if(pStat) {
		ExtractStat(p_ctx, p_scope, pStat);
	}
	CATCHZOK
	return ok;
}

int SLAPI DbDict_DL600::GetTableInfo(long tblID, DbTableStat * pStat)
{
	int    ok = 1;
	DlContext * p_ctx = DS.GetInterfaceContext(PPSession::ctxDatabase);
	THROW(p_ctx);
	const DlScope * p_scope = p_ctx->GetScope_Const(tblID, DlScope::kDbTable);
	THROW(p_scope);
	if(pStat) {
		ExtractStat(p_ctx, p_scope, pStat);
	}
	CATCHZOK
	return ok;
}

int SLAPI DbDict_DL600::GetListOfTables(long options, StrAssocArray * pList)
{
	int    ok = 1;
	DlContext * p_ctx = DS.GetInterfaceContext(PPSession::ctxDatabase);
	THROW(p_ctx);
	THROW(p_ctx->GetDbTableSpecList(pList));
	CATCHZOK
	return ok;
}
//
//
//
int SLAPI StatusWinChange(int onLogon /*=0*/, long timer/*=-1*/)
{
	int    ok = 1;
	DWORD  gr_gdiobj  = 0;
	DWORD  gr_userobj = 0;
	SString temp_buf;
	TProgram * p_app = APPL;
	if(p_app && DBS.GetConstTLA().P_CurDict) {
		PPThreadLocalArea & r_tla = DS.GetTLA();
		SString sbuf, db_name;
		p_app->ClearStatusBar();
		if(timer >= 0) {
			PPLoadTextWin(PPTXT_AUTOEXIT_INFO, sbuf);
			temp_buf.Printf(sbuf, timer);
			p_app->AddStatusBarItem(temp_buf, 0, GetColorRef(SClrRed), 0, GetColorRef(SClrBlack));
		}
		p_app->AddStatusBarItem(GetMainOrgName(sbuf).Transf(CTRANSF_INNER_TO_OUTER), 0, 0, cmViewStatus);
		p_app->AddStatusBarItem((sbuf = r_tla.CurDbDivName).Transf(CTRANSF_INNER_TO_OUTER), 0, 0, cmViewStatus);
		GetLocationName(LConfig.Location, sbuf);
		p_app->AddStatusBarItem(sbuf.Transf(CTRANSF_INNER_TO_OUTER), 0, 0, cmViewStatus);
		(sbuf = 0).Cat(LConfig.OperDate, MKSFMT(0, DATF_DMY | DATF_CENTURY));
		p_app->AddStatusBarItem(sbuf, 0, 0, cmViewStatus);
		if(CurDict && CurDict->GetDbName(db_name) > 0) {
			p_app->AddStatusBarItem((sbuf = "DB").CatDiv(':', 2).Cat(db_name.Transf(CTRANSF_INNER_TO_OUTER)), 0, 0, cmViewStatus);
		}
		p_app->AddStatusBarItem("www.petroglif.ru", 0, GetColorRef(SClrAliceblue), cmGotoSite, GetColorRef(SClrBlue));
		//turistti
		gr_gdiobj  = GetGuiResources(GetCurrentProcess(), GR_GDIOBJECTS);
		gr_userobj = GetGuiResources(GetCurrentProcess(), GR_USEROBJECTS);
		//end turistti
#ifndef NDEBUG
		{
			MemHeapTracer mht;
			MemHeapTracer::Stat mht_stat;
			if(mht.CalcStat(&mht_stat)) {
				(sbuf = 0).
					Cat(mht_stat.UsedBlockCount).CatDiv('-', 1).
					Cat(mht_stat.UsedSize).CatDiv('-', 1).
					Cat(mht_stat.UnusedBlockCount).CatDiv('-', 1).
					Cat(mht_stat.UnusedSize).CatDiv(';', 2).
					CatEq("GDI", (long)gr_gdiobj).CatDiv(';', 2).
					CatEq("USER", (long)gr_userobj).CatDiv(';', 2).
					Cat("FOCUS").Eq().CatHex((long)::GetFocus()).CatDiv(';', 2).
					Cat("CAPTURE").Eq().CatHex((long)::GetCapture());
				p_app->AddStatusBarItem(sbuf);
			}
			else {
				p_app->AddStatusBarItem("Heap Corrupted");
			}
		}
#endif
		if(CConfig.Flags & CCFLG_3TIER) {
			static SCycleTimer * p_timer = 0;
			ENTER_CRITICAL_SECTION
			if(!p_timer || p_timer->Check(0)) {
				PPJobSrvClient * p_cli = DS.GetClientSession(1);
				if(p_cli) {
					ZDELETE(p_timer);
					p_app->AddStatusBarItem("  ", 0, GetColorRef(SClrGreen), 0, GetColorRef(SClrWhite));
				}
				else {
					SETIFZ(p_timer, new SCycleTimer(30000));
					p_app->AddStatusBarItem("  ", 0, GetColorRef(SClrRed), 0, GetColorRef(SClrYellow));
				}
			}
			LEAVE_CRITICAL_SECTION
		}
		if(r_tla.P_Ref) {
			PPProjectConfig prj_cfg;
			if(PPObjProject::FetchConfig(&prj_cfg)) {
				const  int check_new_task = BIN((onLogon && prj_cfg.Flags & PRJCFGF_NEWTASKNOTICEONLOGIN) || (!onLogon && prj_cfg.Flags & PRJCFGF_NEWTASKNOTICE));
				const  int rmnd_incompl_task = BIN(prj_cfg.Flags & PRJCFGF_INCOMPLETETASKREMIND);
				if(check_new_task || rmnd_incompl_task) {
					PPID   employer = 0;
					GetCurUserPerson(&employer, 0);
					if(employer) {
						SETIFZ(r_tla.P_TodoObj, new PPObjPrjTask);
						PPObjPrjTask * p_todo_obj = DS.GetTLA().P_TodoObj;
						if(p_todo_obj) {
							PrjTaskTbl * t = p_todo_obj->P_Tbl;
							PrjTaskTbl::Key4 k4;
							if(check_new_task) {
								MEMSZERO(k4);
								k4.EmployerID = employer;
								if(t->search(4, &k4, spGe) && t->data.EmployerID == employer) {
									BExtQuery q(t, 4);
									DBQ * dbq = 0;
									dbq = ppcheckfiltid(dbq, t->EmployerID, employer);
									q.select(t->EmployerID, t->Flags, t->Status, 0L).where(*dbq);
									for(q.initIteration(0, &k4, spGe); q.nextIteration() > 0;) {
										const PrjTaskTbl::Rec & r_rec = t->data;
										if(!(r_rec.Flags & TODOF_OPENEDBYEMPL) && !oneof2(r_rec.Status, TODOSTTS_REJECTED, TODOSTTS_COMPLETED)) {
											PPGetWord(PPWORD_NEWTASK, 1, temp_buf);
											APPL->AddStatusBarItem(temp_buf, ICON_NEWTASK, 0, cmPrjTask_ByStatus);
											break;
										}
									}
								}
							}
							if(rmnd_incompl_task) {
								DateRange period;
								LDATE cur_dt = getcurdate_();
								period.SetDate(cur_dt);
								plusdate(&period.upp, abs(prj_cfg.RemindPrd.low), 0);
								if(prj_cfg.RemindPrd.low != prj_cfg.RemindPrd.upp)
									plusdate(&period.low, -abs(prj_cfg.RemindPrd.upp), 0);
								MEMSZERO(k4);
								k4.EmployerID = employer;
								k4.Dt = period.low;
								if(t->search(4, &k4, spGe) && t->data.EmployerID == employer && t->data.StartDt >= period.low) {
									DBQ * dbq = 0;
									dbq = ppcheckfiltid(dbq, t->EmployerID, employer);
									dbq = &(*dbq && daterange(t->StartDt, &period));
									BExtQuery q(t, 4);
									q.select(t->EmployerID, t->StartDt, t->Status, 0L).where(*dbq);
									for(q.initIteration(0, &k4, spGe); q.nextIteration() > 0;) {
										const PrjTaskTbl::Rec & r_rec = t->data;
										if(!oneof2(r_rec.Status, TODOSTTS_REJECTED, TODOSTTS_COMPLETED)) {
											PPGetWord(PPWORD_INCOMPLETETASK, 1, temp_buf);
											APPL->AddStatusBarItem(temp_buf, ICON_TASKREMINDER, 0, cmPrjTask_ByReminder);
											break;
										}
									}
								}
							}
						}
					}
				}
			}
		}
		if(DS.GetPrivateBasket()) {
			PPGetWord(PPWORD_PRIVATEBASKET, 1, temp_buf);
			p_app->AddStatusBarItem(temp_buf, ICON_BASKET_SMALL, 0, cmPrivateBasket);
		}
		{
		}
		//
		// Уведомление о наличие новых версий Papyrus
		//
		{
			static SCycleTimer * p_timer2 = 0;
			static int upd_available = 0;
			ENTER_CRITICAL_SECTION
			int timer_expired = BIN(!p_timer2 ||p_timer2->Check(0));
			if(timer_expired) {
				UserInterfaceSettings ui_cfg;
				ZDELETE(p_timer2);
				upd_available = 0;
				ui_cfg.Restore();
				if(ui_cfg.Flags & UserInterfaceSettings::fUpdateReminder) {
					if(PPUhttClient::ViewNewVerList(0) > 0)
						upd_available = 1;
				}
				SETIFZ(p_timer2, new SCycleTimer(900000));
			}
			if(upd_available) {
				PPLoadText(PPTXT_UPDATEAVAILABLE, temp_buf);
				temp_buf.Transf(CTRANSF_INNER_TO_OUTER);
				p_app->AddStatusBarItem(temp_buf, 0, GetColorRef(SClrGreenyellow), cmViewNewVersionList, GetColorRef(SClrRed));
			}
			LEAVE_CRITICAL_SECTION
		}
		p_app->UpdateStatusBar();
	}
	else
		ok = -1;
	return ok;
}

#define SIGN_PPTLA 0x7D08E311L

PPThreadLocalArea::IdleCommand::IdleCommand(long repeatEachSeconds) : SCycleTimer(repeatEachSeconds * 1000)
{
}

PPThreadLocalArea::IdleCommand::~IdleCommand()
{
}

int FASTCALL PPThreadLocalArea::IdleCommand::Run(const LDATETIME & rPrevRunTime)
{
	return -1;
}

SLAPI PPThreadLocalArea::PPThreadLocalArea() : Prf(1), UfpSess(0)
{
	memzero(this, offsetof(PPThreadLocalArea, Rights));
	Sign = SIGN_PPTLA;
	PrnDirId = labs(SLS.GetTLA().Rg.Get());
	RegisterAdviseObjects();
}

SLAPI PPThreadLocalArea::~PPThreadLocalArea()
{
	ZDELETE(P_WObj);  // @v8.3.6
	ZDELETE(P_WbObj); // @v8.3.6
	ZDELETE(P_TodoObj); // @v8.5.11
	ZDELETE(P_BObj);
	ZDELETE(P_Ref);
	ZDELETE(P_SysJ);
	ZDELETE(P_ObjSync);
	ZDELETE(P_GtaJ);
	ZDELETE(P_ErrCtx);
	ZFREE(P_PtrVect);
	PtrVectDim = 0;
	ZDELETE(P_ExpCtx);
	ZDELETE(P_IfcCtx);
	Sign = 0;
}

int SLAPI PPThreadLocalArea::RegisterAdviseObjects()
{
	class IdleCmdUpdateStatusWin : public IdleCommand {
	public:
	#ifndef NDEBUG
		#define UPD_STATUS_PERIOD 1
	#else
		#define UPD_STATUS_PERIOD 5
	#endif
		IdleCmdUpdateStatusWin() : IdleCommand(UPD_STATUS_PERIOD)
		{
			OnLogon = 1;
		}
		virtual int FASTCALL Run(const LDATETIME & rPrevRunTime)
		{
			return (StatusWinChange(OnLogon), OnLogon = 0, 1);
		}
	private:
		int    OnLogon;
	};
	class IdleCmdQuitSession : public IdleCommand {
	public:
		IdleCmdQuitSession() : IdleCommand(10)
		{
			Timer = -1;
		}
		virtual int FASTCALL Run(const LDATETIME & rPrevRunTime)
		{
			const  char * p_quit = "QUIT";
			long   quit_after = 5 * 60;
			SString buf, path;
			PPGetFileName(PPFILNAM_PPLOCK, buf);
			PPGetFilePath(PPPATH_BIN, buf, path);
			if(Timer == -1) {
				if(fileExists(path)) {
					long   sec = 0;
					SFile  f(path, SFile::mRead);
					if(f.IsValid()) {
						f.ReadLine(buf = 0);
						if((sec = (buf.CmpNC(p_quit) == 0) ? quit_after : buf.ToLong()) > 0) {
							Timer = sec;
							PPLogMessage(PPFILNAM_INFO_LOG, PPSTR_TEXT, PPTXT_ACTIVESESSION, LOGMSGF_TIME|LOGMSGF_USER|LOGMSGF_DBINFO);
							Restart(1000);
						}
					}
				}
			}
			else if(fileExists(path)) {
				if(Timer == 0) {
					DS.Logout();
					exit(1);
				}
				else {
					StatusWinChange(0, Timer);
					Timer--;
				}
			}
			else {
				Timer = -1;
				Restart(10000);
			}
			return 1;
		}
	private:
		long   Timer;
	};
	class IdleCmdUpdateObjList : public IdleCommand, private PPAdviseEventQueue::Client {
	public:
		IdleCmdUpdateObjList(long refreshPeriod, PPID objTypeID, PPID notifyID) : IdleCommand(refreshPeriod)
		{
			ObjTypeID = objTypeID;
			NotifyID = notifyID;
		}
		virtual int FASTCALL Run(const LDATETIME & rPrevRunTime)
		{
			int    ok = -1;
			if(NotifyID && ObjTypeID) {
				PPAdviseList adv_list;
				PPAdviseEventQueue * p_queue = DS.GetAdviseEventQueue(this);
				//
				// Установить маркер очереди необходимо даже если подписчиков на события нет.
				//
				const uint evqc = (p_queue && p_queue->Get(Marker, EvqList) > 0) ? EvqList.getCount() : 0;
				if(evqc)
					Marker = EvqList.at(evqc-1).Ident;
				if(DS.GetAdviseList(NotifyID, 0, adv_list) > 0) {
					PPThreadLocalArea & r_tla = DS.GetTLA();
					IdList.clear();
					if(!p_queue) {
						SysJournal * p_sj = r_tla.P_SysJ;
						CALLPTRMEMB(p_sj, GetObjListByEventSince((ObjTypeID == -1) ? 0 : ObjTypeID, 0, rPrevRunTime, IdList));
					}
					else if(evqc) {
						for(uint i = 0; i < evqc; i++) {
							const PPAdviseEvent & r_ev = EvqList.at(i);
							if(r_ev.Action && ((ObjTypeID == -1) || r_ev.Oid.Obj == ObjTypeID)) {
								IdList.add(r_ev.Oid.Id);
							}
						}
						IdList.sortAndUndup();
					}
					{
						const uint c = IdList.getCount();
						if(c) {
							PPNotifyEvent ev;
							PPAdviseBlock adv_blk;
							LDATETIME prev_dtm = rPrevRunTime;
							for(uint j = 0; adv_list.Enum(&j, &adv_blk);) {
								if(adv_blk.Proc) {
									for(uint i = 0; i < c; i++) {
										ev.Clear();
										ev.ObjType = ObjTypeID;
										ev.ObjID   = IdList.get(i);
										ev.ExtDtm  = prev_dtm;
										adv_blk.Proc(NotifyID, &ev, adv_blk.ProcExtPtr);
									}
									{ // finalize {
										ev.SetFinishTag();
										ev.ExtDtm = prev_dtm;
										adv_blk.Proc(NotifyID, &ev, adv_blk.ProcExtPtr);
									} // } finalize
								}
							}
							ok = 1;
						}
					}
				}
			}
			return ok;
		}
	private:
		PPID   ObjTypeID;
		PPID   NotifyID;
		PPIDArray IdList;
	};
	class IdleCmdUpdateTSessList : public IdleCommand, private PPAdviseEventQueue::Client {
	public:
		IdleCmdUpdateTSessList(long refreshPeriod, PPID notifyID) : IdleCommand(refreshPeriod)
		{
			NotifyID = notifyID;
		}
		virtual int FASTCALL Run(const LDATETIME & rPrevRunTime)
		{
			int    ok = -1;
			PPAdviseList adv_list;
			if(NotifyID) {
				PPAdviseEventQueue * p_queue = DS.GetAdviseEventQueue(this);
				//
				// Установить маркер очереди необходимо даже если подписчиков на события нет.
				//
				const uint evqc = (p_queue && p_queue->Get(Marker, EvqList) > 0) ? EvqList.getCount() : 0;
				if(evqc)
					Marker = EvqList.at(evqc-1).Ident;
				if(DS.GetAdviseList(NotifyID, 0, adv_list) > 0) {
					PPThreadLocalArea & r_tla = DS.GetTLA();
					TSArray <PPAdviseEvent> result_list;
					if(!p_queue) {
						SysJournal * p_sj = r_tla.P_SysJ;
						if(p_sj) {
							SysJournalTbl::Key0 k;
							DBQ * dbq = 0;
							k.Dt = rPrevRunTime.d;
							k.Tm = rPrevRunTime.t;
							BExtQuery q(p_sj, 0, 128);
							dbq = &(*dbq && p_sj->Dt >= rPrevRunTime.d);
							dbq = ppcheckfiltid(dbq, p_sj->ObjType, PPOBJ_TSESSION);
							q.selectAll().where(*dbq);
							for(q.initIteration(0, &k, spGe); q.nextIteration() > 0;) {
								if(cmp(rPrevRunTime, p_sj->data.Dt, p_sj->data.Tm) < 0) {
									PPAdviseEvent ev;
									ev = p_sj->data;
									result_list.insert(&ev);
								}
							}
						}
					}
					else if(evqc) {
						for(uint i = 0; i < evqc; i++) {
							const PPAdviseEvent & r_ev = EvqList.at(i);
							if(r_ev.Action && r_ev.Oid.Obj == PPOBJ_TSESSION) {
								result_list.insert(&r_ev);
							}
						}
					}
					{
						const uint rlc = result_list.getCount();
						if(rlc) {
							PPNotifyEvent nev;
							PPAdviseBlock adv_blk;
							LDATETIME prev_dtm = rPrevRunTime;
							for(uint j = 0; adv_list.Enum(&j, &adv_blk);) {
								if(adv_blk.Proc) {
									for(uint i = 0; i < rlc; i++) {
										const PPAdviseEvent & r_ev = result_list.at(i);
										nev.Clear();
										nev.Action  = r_ev.Action;
										assert(r_ev.Oid.Obj == PPOBJ_TSESSION);
										nev.ObjType = r_ev.Oid.Obj;
										nev.ObjID   = r_ev.Oid.Id;
										nev.ExtInt_ = r_ev.SjExtra;
										adv_blk.Proc(NotifyID, &nev, adv_blk.ProcExtPtr);
									}
									// finalize {
									nev.SetFinishTag();
									nev.ExtDtm = prev_dtm;
									adv_blk.Proc(NotifyID, &nev, adv_blk.ProcExtPtr);
									// } finalize
								}
							}
							ok = 1;
						}
					}
				}
			}
			return ok;
		}
	private:
		PPID   NotifyID;
	};
	class IdleCmdUpdateBizScoreOnDesktop : public IdleCommand, private PPAdviseEventQueue::Client {
	public:
		IdleCmdUpdateBizScoreOnDesktop() : IdleCommand(30)
		{
		}
		virtual int FASTCALL Run(const LDATETIME & rPrevRunTime)
		{
			int    ok = -1;
			PPAdviseList adv_list;
			PPAdviseEventQueue * p_queue = DS.GetAdviseEventQueue(this);
			//
			// Установить маркер очереди необходимо даже если подписчиков на события нет.
			//
			const uint evqc = (p_queue && p_queue->Get(Marker, EvqList) > 0) ? EvqList.getCount() : 0;
			if(evqc)
				Marker = EvqList.at(evqc-1).Ident;
			if(DS.GetAdviseList(PPAdviseBlock::evBizScoreChanged, 0, adv_list) > 0) {
				PPThreadLocalArea & r_tla = DS.GetTLA();
				int   do_notify = 0;
				if(!p_queue) {
					SysJournal * p_sj = r_tla.P_SysJ;
					if(p_sj) {
						LAssocArray id_list;
						SysJournalTbl::Rec sysj_rec;
						MEMSZERO(sysj_rec);
						LDATETIME prev_dtm = rPrevRunTime;
						if(p_sj->GetLastEvent(PPACN_BIZSCOREUPDATED, &prev_dtm, 2) > 0) {
							do_notify = 1;
						}
					}
				}
				else if(evqc) {
					const int32 _action = PPACN_BIZSCOREUPDATED;
					uint  _p = 0;
					if(EvqList.lsearch(&_action, &_p, CMPF_LONG, offsetof(PPAdviseEvent, Action))) {
						do_notify = 1;
					}
				}
				if(do_notify) {
					PPNotifyEvent ev;
					PPAdviseBlock adv_blk;
					for(uint j = 0; adv_list.Enum(&j, &adv_blk);) {
						if(adv_blk.Proc) {
							ev.Clear();
							ev.ObjType = PPOBJ_BIZSCORE;
							adv_blk.Proc(PPAdviseBlock::evBizScoreChanged, &ev, adv_blk.ProcExtPtr);
						}
					}
					ok = 1;
				}
			}
			return ok;
		}
	};
	class IdleCmdUpdateCaches : public IdleCommand, private PPAdviseEventQueue::Client {
	public:
		IdleCmdUpdateCaches() : IdleCommand(30)
		{
		}
		virtual int FASTCALL Run(const LDATETIME & rPrevRunTime)
		{
			return DS.DirtyDbCache(DBS.GetDbPathID(), this);
		}
	};
#if USE_ADVEVQUEUE==2
	class IdleCmdTestAdvEvQueue : public IdleCommand, private PPAdviseEventQueue::Client {
	public:
		IdleCmdTestAdvEvQueue() : IdleCommand(10)
		{
		}
		virtual int FASTCALL Run(const LDATETIME & rPrevRunTime)
		{
			PPAdviseEventQueue * p_queue = DS.GetAdviseEventQueue(this);
			if(p_queue) {
				if(p_queue->Get(Marker, EvqList) > 0) {
					SString msg_buf, temp_buf;
					for(uint i = 0; i < EvqList.getCount(); i++) {
						/*
							int64  Ident;
							LDATETIME Dtm;
							int32  Action;
							PPObjID Oid;
							int32  UserID;
							int32  SjExtra;
							long   Flags;
						*/
                        PPAdviseEvent & r_ev = EvqList.at(i);
                        (msg_buf = "AdviseEvent").CatDiv(':', 2).CatEq("Ident", r_ev.Ident).Space().CatEq("Dtm", r_ev.Dtm).Space().
							CatEq("Action", r_ev.Action).Space();
						r_ev.Oid.ToStr(0, temp_buf = 0);
						msg_buf.CatEq("Oid", temp_buf).Space().CatEq("UserID", r_ev.UserID).Space().
							CatEq("SjExtra", r_ev.SjExtra).Space().CatEq("Flags", r_ev.Flags);
						PPLogMessage(PPFILNAM_DEBUG_LOG, msg_buf, LOGMSGF_DBINFO|LOGMSGF_TIME|LOGMSGF_THREADINFO);
						Marker = r_ev.Ident;
					}
				}
			}
			return -1;
		}
	};
#endif // } USE_ADVEVQUEUE==2
	class IdleCmdUpdateLogsMon : public IdleCommand {
	public:
		IdleCmdUpdateLogsMon(long refreshPeriod, PPID objTypeID, PPID notifyID) : IdleCommand(refreshPeriod)
		{
			ObjTypeID = objTypeID;
			NotifyID = notifyID;
		}
		virtual int FASTCALL Run(const LDATETIME & rPrevRunTime)
		{
			int    ok = -1;
			if(NotifyID && ObjTypeID) {
				uint   i = 0, j = 0;
				PPAdviseList adv_list;
				if(DS.GetAdviseList(NotifyID, 0, adv_list) > 0) {
					PPNotifyEvent ev;
					PPAdviseBlock adv_blk;
					for(j = 0; adv_list.Enum(&j, &adv_blk);) {
						if(adv_blk.Proc) {
							ev.Clear();
							ev.ObjType = ObjTypeID;
							ev.ObjID   = -1;
							ev.ExtDtm  = rPrevRunTime;
							adv_blk.Proc(NotifyID, &ev, adv_blk.ProcExtPtr);
							// finalize {
							ev.SetFinishTag();
							ev.ExtInt_ = 0;
							ev.ExtDtm = rPrevRunTime;
							adv_blk.Proc(NotifyID, &ev, adv_blk.ProcExtPtr);
							// } finalize
						}
					}
				}
				ok = 1;
			}
			return ok;
		}
	private:
		PPID   ObjTypeID;
		PPID   NotifyID;
	};
	class IdleCmdQuartz : public IdleCommand {
	public:
		IdleCmdQuartz(PPID notifyID) : IdleCommand(1)
		{
			NotifyID = notifyID;
		}
		virtual int FASTCALL Run(const LDATETIME & rPrevRunTime)
		{
			int    ok = -1;
			PPAdviseList adv_list;
			if(DS.GetAdviseList(NotifyID, 0, adv_list) > 0) {
				PPNotifyEvent ev;
				PPAdviseBlock adv_blk;
				for(uint j = 0; adv_list.Enum(&j, &adv_blk);) {
					if(adv_blk.Proc) {
						ev.Clear();
						ev.ObjID   = -1;
						adv_blk.Proc(NotifyID, &ev, adv_blk.ProcExtPtr);
						ok = 1;
					}
				}
			}
			return ok;
		}
	private:
		PPID   NotifyID;
	};

	int    ok = 1;
	IdleCmdList.insert(new IdleCmdUpdateStatusWin);
	IdleCmdList.insert(new IdleCmdQuitSession);
	IdleCmdList.insert(new IdleCmdUpdateCaches);
	IdleCmdList.insert(new IdleCmdUpdateBizScoreOnDesktop);
	IdleCmdList.insert(new IdleCmdUpdateObjList(5, PPOBJ_PRJTASK, PPAdviseBlock::evTodoChanged));
	IdleCmdList.insert(new IdleCmdUpdateObjList(30, PPOBJ_BILL, PPAdviseBlock::evBillChanged));
	IdleCmdList.insert(new IdleCmdUpdateObjList(5, PPOBJ_PERSONEVENT, PPAdviseBlock::evPsnEvChanged)); // @v8.0.3
	IdleCmdList.insert(new IdleCmdUpdateTSessList(30, PPAdviseBlock::evTSessChanged));
	IdleCmdList.insert(new IdleCmdUpdateObjList(5,  -1, PPAdviseBlock::evSysJournalChanged));
	IdleCmdList.insert(new IdleCmdUpdateLogsMon(10, -1, PPAdviseBlock::evLogsChanged));
	IdleCmdList.insert(new IdleCmdQuartz(PPAdviseBlock::evQuartz));
#if USE_ADVEVQUEUE==2
	IdleCmdList.insert(new IdleCmdTestAdvEvQueue); // @debug
#endif
	return ok;
}

int  SLAPI PPThreadLocalArea::IsAuth() const
{
	return BIN(State & stAuth);
}

long SLAPI PPThreadLocalArea::GetId() const
{
	return Id;
}

ThreadID SLAPI PPThreadLocalArea::GetThreadID() const
{
	return TId;
}

int SLAPI PPThreadLocalArea::IsConsistent() const
{
	return BIN(Sign == SIGN_PPTLA);
}

PPView * SLAPI PPThreadLocalArea::GetPPViewPtr(int32 id) const
{
	return (id > 0 && id <= (int32)SrvViewList.getCount()) ? SrvViewList.at(id-1) : 0;
}

int32 SLAPI PPThreadLocalArea::CreatePPViewPtr(PPView * pView)
{
	for(uint i = 0; i < SrvViewList.getCount(); i++) {
		if(SrvViewList.at(i) == 0) {
			SrvViewList.atPut(i, pView);
			return (int32)(i+1);
		}
	}
	SrvViewList.insert(pView);
	return (int32)SrvViewList.getCount();
}

int SLAPI PPThreadLocalArea::ReleasePPViewPtr(int32 id)
{
	if(id > 0 && id <= (int32)SrvViewList.getCount()) {
		SrvViewList.atPut(id-1, 0);
		return 1;
	}
	else
		return 0;
}
//
//
//
static ACount TlpC(0); // @global @threadsafe

SLAPI  __PPThrLocPtr::__PPThrLocPtr()
{
	Idx = TlpC.Incr();
}

int SLAPI __PPThrLocPtr::IsOpened()
{
	return BIN(DS.GetTLA().GetPtrNonIncrement(Idx));
}

void * FASTCALL __PPThrLocPtr::Helper_Open(SClassWrapper & cw)
{
	PPThreadLocalArea & tla = DS.GetTLA();
	if(!tla.GetPtrNonIncrement(Idx))
		tla.CreatePtr(Idx, cw.Create());
	return tla.GetPtr(Idx);
}

void FASTCALL __PPThrLocPtr::Helper_Close(SClassWrapper & cw, void * ptr)
{
	if(DS.GetTLA().ReleasePtr(Idx) > 0)
		cw.Destroy(ptr);
}
//
//
//
void * FASTCALL PPThreadLocalArea::GetPtr(uint idx) const
{
	void * p = 0;
	if(idx > 0 && idx <= PtrVectDim) {
		PtrEntry * p_entry = &P_PtrVect[idx-1];
		if(p_entry->Ptr) {
			p_entry->RefCounter++;
			p = p_entry->Ptr;
		}
	}
	return p;
}

void * FASTCALL PPThreadLocalArea::GetPtrNonIncrement(uint idx) const
{
	void * p = 0;
	if(idx > 0 && idx <= PtrVectDim) {
		PtrEntry * p_entry = &P_PtrVect[idx-1];
		p = p_entry->Ptr;
	}
	return p;
}

int SLAPI PPThreadLocalArea::CreatePtr(uint idx, void * ptr)
{
	if(idx > PtrVectDim) {
		uint   new_dim = ALIGNSIZE(idx, 6);
		PtrEntry * p = (PtrEntry *)realloc(P_PtrVect, sizeof(PtrEntry) * new_dim);
		if(p) {
			memzero(p+PtrVectDim, sizeof(PtrEntry) * (new_dim - PtrVectDim));
			P_PtrVect = p;
			PtrVectDim = new_dim;
		}
		else
			return 0;
	}
	P_PtrVect[idx-1].Ptr = ptr;
	P_PtrVect[idx-1].RefCounter = 0;
	P_PtrVect[idx-1].InUse = 1;
	return 1;
}

int FASTCALL PPThreadLocalArea::ReleasePtr(uint idx)
{
	int    ok = 0;
	if(idx > 0 && idx <= PtrVectDim) {
		PtrEntry * p_entry = &P_PtrVect[idx-1];
		if(p_entry->RefCounter)
			p_entry->RefCounter--;
		if(p_entry->RefCounter == 0) {
			p_entry->Ptr = 0;
			p_entry->InUse = 0;
			ok = 1;
		}
		else
			ok = -1;
	}
	return ok;
}

void SLAPI PPThreadLocalArea::PushErrContext()
{
	ErrContext ctx;
	ctx.Err = LastErr;
	ctx.CrwErr = LastCrwErr;
	ctx.BtrErr = BtrError;
	ctx.SlErr = SLibError;
	ctx.LastErrObj = LastErrObj;
	ctx.AddedMsgString = AddedMsgString;
	ctx.DbFileName = DBTable::GetLastErrorFileName();
	delete P_ErrCtx;
	P_ErrCtx = new ErrContext;
	*P_ErrCtx = ctx;
}

void SLAPI PPThreadLocalArea::PopErrContext()
{
	if(P_ErrCtx) {
		LastErr    = P_ErrCtx->Err;
		LastCrwErr = P_ErrCtx->CrwErr;
		BtrError   = P_ErrCtx->BtrErr;
		SLibError  = P_ErrCtx->SlErr;
		LastErrObj = P_ErrCtx->LastErrObj;
		AddedMsgString = P_ErrCtx->AddedMsgString;
		DBTable::InitErrFileName(P_ErrCtx->DbFileName);
		ZDELETE(P_ErrCtx);
	}
}

int PPThreadLocalArea::SLAPI InitMainOrgData(int reset)
{
	int    ok = 1;
	if(reset) {
		State &= ~stMainOrgInit;
		ok = 2;
	}
	else if(State & stMainOrgInit)
		ok = -1;
	else if(Cc.MainOrgID) {
		PPObjStaffList stlobj;
		Cc.MainOrgDirector_ = 0;
		Cc.MainOrgAccountant_ = 0;
		PersonPostTbl::Rec post_rec;
		stlobj.GetFixedPostOnDate(Cc.MainOrgID, PPFIXSTF_DIRECTOR, ZERODATE, &post_rec);
		Cc.MainOrgDirector_ = post_rec.PersonID;
		stlobj.GetFixedPostOnDate(Cc.MainOrgID, PPFIXSTF_ACCOUNTANT, ZERODATE, &post_rec);
		Cc.MainOrgAccountant_ = post_rec.PersonID;
		if(!Cc.MainOrgDirector_ || !Cc.MainOrgAccountant_) {
			PPCommConfig temp_cfg_rec;
			GetCommConfig(&temp_cfg_rec);
			SETIFZ(Cc.MainOrgDirector_, temp_cfg_rec.MainOrgDirector_);
			SETIFZ(Cc.MainOrgAccountant_, temp_cfg_rec.MainOrgAccountant_);
		}
		ok = 1;
	}
	else
		ok = 0;
	return ok;
}

int SLAPI PPThreadLocalArea::SetIfcConfigParam(const char * pParam, const char * pValue)
{
	int    ok = 1;
	SString param_buf = pParam;
	if(param_buf.NotEmptyS()) {
		if(pValue) {
			SString val_buf;
			IfcConfig.Add(param_buf, (val_buf = pValue).Strip(), 1);
		}
		else
			IfcConfig.Remove(param_buf);
	}
	else
		ok = -1;
	return ok;
}

int SLAPI PPThreadLocalArea::GetIfcConfigParam(const char * pParam, SString & rValue) const
{
	rValue = 0;
	return IfcConfig.Search(pParam, &rValue, 0);
}

PPThreadLocalArea::PrivateCart::PrivateCart()
{
	P = 0;
}

PPThreadLocalArea::PrivateCart::~PrivateCart()
{
	ZDELETE(P);
}

int PPThreadLocalArea::PrivateCart::operator !() const
{
	return (P == 0);
}

int PPThreadLocalArea::PrivateCart::Is(PPID basketID) const
{
	return BIN(P && basketID && P->BasketID == basketID);
}

PPBasketPacket * PPThreadLocalArea::PrivateCart::Get()
{
	return P ? &P->Pack : 0;
}

int PPThreadLocalArea::PrivateCart::Set(const PPBasketPacket * pPack, int use_ta)
{
	int    ok = 1;
	if(pPack && P && pPack->Head.ID == P->Pack.Head.ID) {
		P->Pack = *pPack;
	}
	else {
		PPObjGoodsBasket gb_obj;
		PPTransaction tra(use_ta);
		THROW(tra);
		if(P) {
			//
			// Так как функция PPObjGoodsBasket::PutPacket проверяет приватную коризну,
			// подменяем указатель P так, чтобы PPObjGoodsBasket::PutPacket не узнала,
			// что приватная корзина существует (иначе попадем в рекурсию).
			//
			PPBasketCombine * _p = P;
			P = 0;
			_p->Pack.Head.Flags &= ~GBASKF_PRIVATE;
			THROW(gb_obj.PutPacket(&_p->BasketID, &_p->Pack, 0));
			_p->Lck.Unlock();
			ZDELETE(_p);
		}
		if(pPack) {
			THROW_MEM(P = new PPBasketCombine);
			P->Pack = *pPack;
			PPID   id = P->Pack.Head.ID;
			if(id == 0) {
				P->Pack.Head.Flags &= ~GBASKF_PRIVATE;
				THROW(gb_obj.PutPacket(&id, &P->Pack, 0));
			}
			P->Pack.Head.Flags |= GBASKF_PRIVATE;
			P->BasketID = id;
			THROW(P->Lck.Lock(id));
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}
//
//
//
//static
const char * PPSession::P_JobLogin = "$SYSSERVICE$"; // @global
const char * PPSession::P_EmptyBaseCreationLogin = "$EMPTYBASECREATION$"; // @global

PPSession::ThreadCollection::ThreadCollection() : TSCollection <PPThread> ()
{
	setFlag(aryEachItem, 0);
}

int FASTCALL PPSession::ThreadCollection::Add(const PPThread * pThread)
{
	int    ok = 1;
	if(pThread) {
		RwL.WriteLock();
		const uint   c = getCount();
		for(uint i = 0; ok > 0 && i < c; i++)
			if(at(i) == pThread)
				ok = -1;
		if(ok > 0)
			ok = insert(pThread) ? 1 : PPSetErrorSLib();
		RwL.Unlock();
	}
	return ok;
}

int FASTCALL PPSession::ThreadCollection::Remove(ThreadID id)
{
	int    ok = -1;
	RwL.WriteLock();
	const uint   c = getCount();
	for(uint i = 0; ok < 0 && i < c; i++) {
		PPThread * p_thread = at(i);
		if(p_thread && p_thread->GetThreadID() == id)
			ok = atFree(i) ? 1 : PPSetErrorSLib();
	}
	RwL.Unlock();
	return ok;
}

int FASTCALL PPSession::ThreadCollection::SetMessage(ThreadID id, int kind, const char * pMsg)
{
	int    ok = -1;
	if(oneof2(kind, 0, 1)) {
		PPThread * p_thread = SearchById(id);
		if(p_thread) {
			if(kind == 0)
				p_thread->SetText(pMsg);
			else if(kind == 1)
				p_thread->SetMessage(pMsg);
			ok = 1;
		}
	}
	return ok;
}

uint PPSession::ThreadCollection::GetCount()
{
	uint   c = 0;
	RwL.ReadLock();
	c = getCount();
	RwL.Unlock();
	return c;
}

int FASTCALL PPSession::ThreadCollection::GetInfoList(TSCollection <PPThread::Info> & rList)
{
	int    ok = 1;
	RwL.ReadLock();
	const  uint c = getCount();
	for(uint i = 0; i < c; i++) {
		PPThread * p = at(i);
		if(p && p->IsConsistent()) {
			PPThread::Info * p_info = new PPThread::Info;
			if(p_info) {
				p->GetInfo(*p_info);
				rList.insert(p_info);
			}
		}
	}
	RwL.Unlock();
	return ok;
}

int FASTCALL PPSession::ThreadCollection::GetInfo(ThreadID tId, PPThread::Info & rInfo)
{
	PPThread * p_target = SearchById(tId);
	if(p_target) {
		p_target->GetInfo(rInfo);
		return 1;
	}
	else
		return 0;
}

int FASTCALL PPSession::ThreadCollection::StopThread(ThreadID tId)
{
	int    ok = 1;
	PPThread * p_target = 0;
	THROW_PP(tId != DS.GetConstTLA().GetThreadID(), PPERR_THREADCANTBESTOPPED);
	THROW(p_target = SearchById(tId));
	THROW_PP(oneof2(p_target->GetKind(), PPThread::kJob, PPThread::kNetSession), PPERR_THREADCANTBESTOPPED);
	p_target->SetStopState();
	CATCHZOK
	return ok;
}

PPThread * FASTCALL PPSession::ThreadCollection::SearchById(ThreadID tId)
{
	PPThread * p_ret = 0;
	RwL.ReadLock();
	const uint c = getCount();
	for(uint i = 0; i < c; i++) {
		PPThread * p_thread = at(i);
		if(p_thread && p_thread->IsConsistent() && p_thread->GetThreadID() == tId) {
			p_ret = p_thread;
			break;
		}
	}
	RwL.Unlock();
	return p_ret;
}

PPThread * FASTCALL PPSession::ThreadCollection::SearchBySessId(int32 sessId)
{
	PPThread * p_ret = 0;
	RwL.ReadLock();
	const uint c = getCount();
	for(uint i = 0; i < c; i++) {
		PPThread * p_thread = at(i);
		if(p_thread && p_thread->IsConsistent() && p_thread->GetUniqueSessID() == sessId) {
			p_ret = p_thread;
			break;
		}
	}
	RwL.Unlock();
	return p_ret;
}
//
//
//
SLAPI PPSession::RegSessData::RegSessData()
{
	THISZERO();
}
//
//
//
int PPSession::SetThreadSock(int32 uniqueSessID, TcpSocket & rSock, PPJobSrvReply * pReply)
{
	int    ok = -1;
	if(uniqueSessID == DS.GetConstTLA().GetId())
		ok = PPSetError(PPERR_THREADCANTSETSOCK);
	else {
		PPThread * p_target = ThreadList.SearchBySessId(uniqueSessID);
		if(p_target) {
			if(p_target->GetKind() != PPThread::kNetSession)
				ok = PPSetError(PPERR_THREADCANTSETSOCK);
			else
				ok = p_target->SubstituteSock(rSock, pReply);
		}
		else
			ok = 0;
	}
	return ok;
}

SLAPI PPSession::PPSession()
{
	Id = 1;
	ExtFlags_ = 0;
	P_ObjIdentBlk = 0;
	P_LogQueue = 0; // @v8.9.12
	TlsIdx = -1L;
	P_DbCtx = 0;
	P_AlbatrosCfg = 0;
	MaxLogFileSize = 32768;
#ifdef _MT
	TlsIdx = TlsAlloc();
	InitThread(0);
#endif
}

SLAPI PPSession::~PPSession()
{
#ifdef _MT
	ReleaseThread();
	TlsFree(TlsIdx);
#endif
	delete P_ObjIdentBlk;
	delete P_DbCtx;
	delete P_AlbatrosCfg;
	// Don't destroy P_LogQueue (на объект может ссылаться поток PPLogMsgSession потому удалять его нельзя)
}

uint64 SLAPI PPSession::GetProfileTime()
{
	return GetTLA().Prf.GetAbsTimeMicroseconds();
}

int SLAPI PPSession::GProfileStart(const char * pFileName, long lineNo, const char * pAddedInfo)
{
	return GPrf.Start(pFileName, lineNo, pAddedInfo);
}

int SLAPI PPSession::GProfileFinish(const char * pFileName, long lineNo)
{
	return GPrf.Finish(pFileName, lineNo);
}

#ifndef NDEBUG // {

static int _TestSymbVar(PPSymbTranslator & rSt, int varId, const char * pVarSymb)
{
	int    ok = 1;
    SString temp_buf;
    {
		size_t pos = 0;
		(temp_buf = pVarSymb).Cat("*abc");
		long si = rSt.Translate(temp_buf, &pos, 0);
		assert(si == varId);
		assert(pos == strlen(pVarSymb));
		if(si != varId || pos != strlen(pVarSymb))
			ok = 0;
    }
    {
		size_t pos = 0;
		(temp_buf = pVarSymb).ToLower().Cat("0xyz");
		long si = rSt.Translate(temp_buf, &pos, 0);
		assert(si == varId);
		assert(pos == strlen(pVarSymb));
		if(si != varId || pos != strlen(pVarSymb))
			ok = 0;
    }
    return ok;
}

static int _TestSymbols()
{
	int   ok = 1;
	{
		PPSymbTranslator st(PPSSYM_SYMB);
		_TestSymbVar(st, PPSYM_LINK,           "LINK");
		_TestSymbVar(st, PPSYM_BILLNO,         "BILLNO");
		_TestSymbVar(st, PPSYM_DATE,           "DATE");
		_TestSymbVar(st, PPSYM_PAYDATE,        "PAYDATE");

		_TestSymbVar(st, PPSYM_AMOUNT,         "AMOUNT");
		_TestSymbVar(st, PPSYM_AMOUNT,         "AMT");

		_TestSymbVar(st, PPSYM_LOCCODE,        "LOCCODE");

		_TestSymbVar(st, PPSYM_LOCATION,       "LOCATION");
		_TestSymbVar(st, PPSYM_LOCATION,       "LOC");

		_TestSymbVar(st, PPSYM_BILLOBJ2,       "BILLOBJ2");
		_TestSymbVar(st, PPSYM_BILLOBJ2,       "OBJ2");
		_TestSymbVar(st, PPSYM_BILLOBJ2,       "EXTOBJ");

		_TestSymbVar(st, PPSYM_OBJECT,         "OBJECT");
		_TestSymbVar(st, PPSYM_OBJECT,         "OBJ");

		_TestSymbVar(st, PPSYM_PAYER,          "PAYER");
		_TestSymbVar(st, PPSYM_AGENT,          "AGENT");
		_TestSymbVar(st, PPSYM_REGNAM,         "REGNAME");
		_TestSymbVar(st, PPSYM_REGSN,          "REGSN");
		_TestSymbVar(st, PPSYM_REGNO,          "REGNO");
		_TestSymbVar(st, PPSYM_REGORG,         "REGORG");
		_TestSymbVar(st, PPSYM_TRADELIC,       "TRADELIC");
		_TestSymbVar(st, PPSYM_BILLMEMO,       "BILLMEMO");
		_TestSymbVar(st, PPSYM_GC_NAME,        "GCNAME");
		_TestSymbVar(st, PPSYM_GC_KIND,        "GCKIND");
		_TestSymbVar(st, PPSYM_GC_GRADE,       "GCGRADE");
		_TestSymbVar(st, PPSYM_GC_ADDPROP,     "GCADDPROP");
		_TestSymbVar(st, PPSYM_GC_DIMX,        "GCDIMX");
		_TestSymbVar(st, PPSYM_GC_DIMY,        "GCDIMY");
		_TestSymbVar(st, PPSYM_GC_DIMZ,        "GCDIMZ");
		_TestSymbVar(st, PPSYM_ADVLNACC,       "ALACC");
		_TestSymbVar(st, PPSYM_ADVLNAR,        "ALAR");
		_TestSymbVar(st, PPSYM_ADVLNAMT,       "ALAMT");
		_TestSymbVar(st, PPSYM_RECKON,         "RCKN");
		_TestSymbVar(st, PPSYM_CLIENTADDR,     "CLIENTADDR");
		_TestSymbVar(st, PPSYM_GRNAME,         "GRNAME");
		_TestSymbVar(st, PPSYM_PHPERU,         "PHPU");
		_TestSymbVar(st, PPSYM_BRAND,          "BRAND");
		_TestSymbVar(st, PPSYM_CLIENT,         "CLIENT");
		_TestSymbVar(st, PPSYM_PARENT,         "PARENT");
		_TestSymbVar(st, PPSYM_GC_ADD2PROP,    "GCADD2PROP");
		_TestSymbVar(st, PPSYM_GC_DIMW,        "GCDIMW");
		_TestSymbVar(st, PPSYM_TSESS,          "TSESS");
		_TestSymbVar(st, PPSYM_PRC,            "PRC");
		_TestSymbVar(st, PPSYM_TECH,           "TECH");
		_TestSymbVar(st, PPSYM_MEMO,           "MEMO");
		_TestSymbVar(st, PPSYM_EXPIRY,         "EXPIRY");
		_TestSymbVar(st, PPSYM_AGTCODE,        "AGTCODE");
		_TestSymbVar(st, PPSYM_AGTDATE,        "AGTDATE");
		_TestSymbVar(st, PPSYM_AGTEXPIRY,      "AGTEXPIRY");
		_TestSymbVar(st, PPSYM_MODEL,          "MODEL");
		_TestSymbVar(st, PPSYM_CODE,           "CODE");
		_TestSymbVar(st, PPSYM_SUBCODE,        "SUBCODE");
		_TestSymbVar(st, PPSYM_OWNER,          "OWNER");
		_TestSymbVar(st, PPSYM_CAPTAIN,        "CAPTAIN");
		_TestSymbVar(st, PPSYM_INVOICEDATE,    "INVOICEDATE");
		_TestSymbVar(st, PPSYM_INVOICENO,      "INVOICENO");
		_TestSymbVar(st, PPSYM_DLVRLOCCODE,    "DLVRLOCCODE");
		_TestSymbVar(st, PPSYM_FGDATE,         "FGDATE");
		_TestSymbVar(st, PPSYM_INN,            "INN");
		_TestSymbVar(st, PPSYM_KPP,            "KPP");
	}
	/*
	{
		2  "0DBT,0DB,0D,0ДБТ,0ДБ,0Д,1CRD,1CR,1KRD,1KR,1C,1K,1КРД,1КР,1К"
		PPSymbTranslator st(PPSSYM_ACCSIDE);
	}
	*/
	return ok;
}

#endif // } _DEBUG

int TestSStringPerf(); // @prototype
int SLAPI TestPPObjBillParseText(); // @prototype

void InitTest()
{
#ifndef NDEBUG // {
	{
		//TestSStringPerf();
		//TestPPObjBillParseText();
	}
	{
		//
		// Эта проверка нужна мне для успокоения.
		// Ибо меня преследует фобия, что такое равенство не выполняется.
		//
		char   temp_buf[32];
		assert((void *)temp_buf == (void *)&temp_buf);
	}
	{
		//
		// Тестирование макроса SETIFZ
		//
		int    a = 1;
		SETIFZ(a, 2);
		assert(a == 1);
		a = 0;
		SETIFZ(a, 2);
		assert(a == 2);
		{
			void * ptr = 0;
			if(SETIFZ(ptr, malloc(128))) {
				assert(ptr != 0);
			}
			else {
				assert(ptr == 0);
			}
			ZFREE(ptr);
			//
			const char * p_abc = "abc";
			ptr = (void *)p_abc;
			if(SETIFZ(ptr, malloc(128))) {
				assert(ptr == p_abc);
			}
			else {
				assert(ptr == 0);
			}
			ptr = 0;
			p_abc = 0;
			if(SETIFZ(ptr, (void *)p_abc)) {
				assert(0);
			}
			else {
				assert(ptr == p_abc);
			}
		}

	}
	{
		//
		// Удостоверяемся в том, что SIZEOFARRAY работает правильно (тоже фобия)
		//
		struct TestStruc {
			const char * P_S;
			int16  I16;
		};
		TestStruc test_array[] = {
			{ "Abc", 1 },
			{ "Ab2", 2 },
			{ "Ab3", 3 },
			{ "Ab4", 4 },
			{ "Ab5", 5 }
		};
		assert(SIZEOFARRAY(test_array) == 5);
	}
	assert(sizeof(char) == 1);
	assert(sizeof(int) == 4);
	assert(sizeof(unsigned int) == 4);

	assert(sizeof(int8) == 1);
	assert(sizeof(uint8) == 1);
	assert(sizeof(int16) == 2);
	assert(sizeof(uint16) == 2);
	assert(sizeof(int32) == 4);
	assert(sizeof(uint32) == 4);
	assert(sizeof(int64) == 8);
	assert(sizeof(uint64) == 8);
	assert(sizeof(S_GUID) == 16);
	assert(sizeof(IntRange) == 8);
	assert(sizeof(RealRange) == 16);
	assert(sizeof(SBaseBuffer) == 8);
	assert(sizeof(DateRepeating) == 8);
	assert(sizeof(DateTimeRepeating) == 12);
	assert(sizeof(KeyDownCommand) == 4); // @v8.1.6

	assert(sizeof(DBFH) == 32);
	assert(sizeof(DBFF) == 32);
	{
		//
		// Проверка совместимости типа SColor с WinGDI-типом RGBQUAD.
		//
		SColor c(1, 2, 3, 0);
		RGBQUAD q;
		q.rgbReserved = 0;
		q.rgbRed = 1;
		q.rgbGreen = 2;
		q.rgbBlue = 3;
		q.rgbReserved = 0;
		assert(sizeof(SColor) == sizeof(RGBQUAD));
		assert(memcmp(&c, &q, sizeof(q)) == 0);
		q = (RGBQUAD)c;
		assert(memcmp(&c, &q, sizeof(q)) == 0);
	}
	{
		//
		// Убедимся, что функции TView::GetId() и TView::TestId() адекватно
		// работают с нулевым указателем this.
		//
		TView * p_zero_view = 0;
		assert(p_zero_view->GetId() == 0);
		assert(p_zero_view->TestId(1) == 0);
	}
	assert(sizeof(STypEx) == 16);
	assert(DBRPL_ERROR == 0); // @v8.8.2
	assert(sizeof(CommPortParams) == 6);
	assert(sizeof(DBRowId) == 32);
	//
	// Так как множество классов наследуются от DBTable важно, чтобы
	// размер DBTable был кратен 32 (для выравнивания по кэш-линии).
	//
	assert(sizeof(DBTable) % 32 == 0);
	assert(sizeof(TView) % 4 == 0);
	assert(sizeof(TWindow) % 4 == 0);
	assert(sizeof(TDialog) % 4 == 0);
	//
	// Записи системного журнала и резервной
	// таблицы системного журнала должны быть эквивалентны.
	//
	assert(sizeof(SysJournalTbl::Rec) == sizeof(SjRsrvTbl::Rec));
	//
	// Размер внутренней структуры электронного адреса должен быть равен 16 байтам и
	// поле Addr таблицы EAddrTbl должен быть бинарно эквивалентен PPEAddr.
	//
	assert(sizeof(PPEAddr) == 16);
	assert(sizeof(PPEAddr) == sizeof(((EAddrTbl::Rec *)0)->Addr));
	//
	assert(sizeof(PPDynanicObjItem) == sizeof(Reference2Tbl::Rec)); // @v8.2.3
	assert(sizeof(PPStaffEntry) == sizeof(Reference2Tbl::Rec)); // @v9.0.3
	assert(sizeof(PPAccount) == sizeof(Reference2Tbl::Rec)); // @v9.0.3
	{
        PPAccount::_A_ a1;
        PPAccount::_A_ a2;
        a1.Ac = 20;
        a1.Sb = 5;
        a2.Ac = 20;
        a2.Sb = 0;
        assert(*(long *)&a1 > *(long *)&a2);
	}
	assert(sizeof(PPBankAccount) == sizeof(RegisterTbl::Rec)); // @v9.0.4
	REF_TEST_RECSIZE(PPObjectTag);
	REF_TEST_RECSIZE(PPSecur);
	REF_TEST_RECSIZE(PPSecur);
	REF_TEST_RECSIZE(PPSecur);
	REF_TEST_RECSIZE(PPBarcodeStruc);
	REF_TEST_RECSIZE(PPUnit);
	REF_TEST_RECSIZE(PPNamedObjAssoc);
	REF_TEST_RECSIZE(PPPersonKind);
	REF_TEST_RECSIZE(PPPersonStatus);
	REF_TEST_RECSIZE(PPELinkKind);
	REF_TEST_RECSIZE(PPCurrency);
	REF_TEST_RECSIZE(PPCurRateType);
	REF_TEST_RECSIZE(PPAmountType);
	REF_TEST_RECSIZE(PPOprType);
	REF_TEST_RECSIZE(PPOpCounter);
	REF_TEST_RECSIZE(PPGdsCls);
	REF_TEST_RECSIZE(PPAssetWrOffGrp);
	REF_TEST_RECSIZE(PPOprKind);
	REF_TEST_RECSIZE(PPBillStatus);
	REF_TEST_RECSIZE(PPAccSheet);
	REF_TEST_RECSIZE(PPCashNode);
	REF_TEST_RECSIZE(PPLocPrinter);
	REF_TEST_RECSIZE(PPStyloPalm);
	REF_TEST_RECSIZE(PPTouchScreen);
	REF_TEST_RECSIZE(PPDBDiv);
	REF_TEST_RECSIZE(PPGoodsType);
	REF_TEST_RECSIZE(PPGoodsStrucHeader);
	REF_TEST_RECSIZE(PPGoodsTax);
	REF_TEST_RECSIZE(PPRegisterType);
	REF_TEST_RECSIZE(PPQuotKind);
	REF_TEST_RECSIZE(PPPsnOpKind);
	REF_TEST_RECSIZE(PPWorldObjStatus);
	REF_TEST_RECSIZE(PPPersonRelType);
	REF_TEST_RECSIZE(PPSalCharge);
	REF_TEST_RECSIZE(PPDateTimeRep);
	REF_TEST_RECSIZE(PPDutySched);
	REF_TEST_RECSIZE(PPStaffCal);
	REF_TEST_RECSIZE(PPScale);
	REF_TEST_RECSIZE(PPBhtTerminal);

	REF_TEST_RECSIZE(PPSCardSeries);
	REF_TEST_RECSIZE(PPDraftWrOff);
	REF_TEST_RECSIZE(PPAdvBillKind);
	REF_TEST_RECSIZE(PPGoodsBasket);
	REF_TEST_RECSIZE(PPDraftCreateRule);
	REF_TEST_RECSIZE(PPGoodsInfo);

	assert(sizeof(PPBarcodePrinter_)-sizeof(SString) == sizeof(Reference_Tbl::Rec));
	assert(sizeof(PPBarcodePrinter2)-sizeof(SString) == sizeof(Reference2Tbl::Rec));
	assert(sizeof(PPInternetAccount_)-sizeof(SString) == sizeof(Reference_Tbl::Rec));
	assert(sizeof(PPInternetAccount2)-sizeof(SString) == sizeof(Reference2Tbl::Rec));
	assert(sizeof(PPAlbatrosCfgHdr) == offsetof(PropertyTbl::Rec, VT)); // @v7.2.7

	assert(sizeof(PersonCore::RelationRecord) == sizeof(ObjAssocTbl::Rec));
	assert(sizeof(PPFreight) == offsetof(PropertyTbl::Rec, VT));
	assert(sizeof(PPRFIDDevice) == sizeof(Reference2Tbl::Rec));

	assert(sizeof(PPSmsAccount) == sizeof(Reference2Tbl::Rec));
	assert(sizeof(PPUhttStore) == sizeof(Reference2Tbl::Rec)); // @v7.6.1

	assert(sizeof(PPGeoTrackingMode) == 8); // @v8.6.8
	// @v9.0.11 {
	//
	// Гарантируем, что функции семейства PPSetError всегда возвращают 0
	// БОльщая часть кода закладывается на этот факт.
	//
	assert(PPSetErrorNoMem() == 0);
	assert(PPSetErrorSLib() == 0);
	assert(PPSetErrorDB() == 0);
	assert(PPSetError(0) == 0);
	assert(PPSetError(0, "") == 0);
	assert(PPSetError(0, 0L) == 0);
	// } @v9.0.11
	assert(_TestSymbols());
	{
		//
		// @v9.3.4 Возможность системы получить стоп-событие, созданное в SlSession, критична!
		//
		SString evnam;
		Evnt test_stop_event(SLS.GetStopEventName(evnam), Evnt::modeOpen);
		assert(test_stop_event.IsValid());
	}
#endif // } _DEBUG
}

static void FpeCatcher(int sig, int fpe)
{
	int    err;
	if(sig == SIGFPE) {
		switch(fpe) {
			case FPE_EXPLICITGEN : err = PPERR_FPE_EXPLICITGEN; break;
			case FPE_INEXACT     : err = PPERR_FPE_INEXACT;     break;
			case FPE_INVALID     : err = PPERR_FPE_INVALID;     break;
			case FPE_OVERFLOW    : err = PPERR_FPE_OVERFLOW;    break;
			case FPE_UNDERFLOW   : err = PPERR_FPE_UNDERFLOW;   break;
			case FPE_ZERODIVIDE  : err = PPERR_FPE_ZERODIVIDE;  break;
			default: return;
		}
		PPError(err, 0);
#ifndef _PPSERVER
		APPL->CloseAllBrowsers();
		DS.Logout();
		exit(err);
#endif
	}
}

int PPCallHelp(uint32 wnd, uint cmd, uint ctx); // @prototype(pptvutil.cpp)
int ExecDateCalendar(/*HWND*/uint32 hParent, LDATE * pDate); // @prototype(calendar.cpp)

static int PPLoadStringFunc(const char * pSignature, SString & rBuf)
{
	return PPLoadString(pSignature, rBuf);
}

static int PPExpandStringFunc(SString & rBuf, int ctransf)
{
	return PPExpandString(rBuf, ctransf);
}

static int PPGetGlobalSecureConfig(SGlobalSecureConfig * pCfg)
{
	if(pCfg) {
		PPIniFile ini_file;
		SString temp_buf;
		ini_file.Get(PPINISECT_GLOBALSECURE, PPINIPARAM_CAPATH, temp_buf = 0);
		pCfg->CaPath = temp_buf.Strip();
		ini_file.Get(PPINISECT_GLOBALSECURE, PPINIPARAM_CAFILE, temp_buf = 0);
		pCfg->CaFile = temp_buf.Strip();
	}
	return 1;
}

static int PPGetDefaultEncrKey(SString & rBuf)
{
    PPVersionInfo vi;
    return vi.GetDefaultEncrKey(rBuf);
}

int SLAPI PPSession::Init(long flags, HINSTANCE hInst)
{
	int    ok = 1;
	SString temp_buf;
	signal(SIGFPE,  (void (*)(int))FpeCatcher);
	SLS.Init(0, hInst);
	{
		PPVersionInfo vi = GetVersionInfo();
		vi.GetProductName(temp_buf);
		SLS.SetAppName(temp_buf);
		SetExtFlag(ECF_OPENSOURCE, vi.GetFlags() & PapyrusPrivateBlock::fOpenSource); // @v9.4.9
	}
	SLS.InitWSA();
	{
		typedef VOID (WINAPI * DISABLEPROCESSWINDOWSGHOSTING)(VOID);
		SDynLibrary lib_user32("USER32.DLL");
		if(lib_user32.IsValid()) {
			DISABLEPROCESSWINDOWSGHOSTING proc_DisableProcessWindowsGhosting =
				(DISABLEPROCESSWINDOWSGHOSTING)lib_user32.GetProcAddr("DisableProcessWindowsGhosting");
			if(proc_DisableProcessWindowsGhosting) {
				proc_DisableProcessWindowsGhosting();
			}
		}
	}
	{
		ENTER_CRITICAL_SECTION
		getExecPath(BinPath = 0).SetLastSlash();
		LEAVE_CRITICAL_SECTION
	}
	RegisterSTAcct();
	PPDbqFuncPool::Register();
	{
		PPIniFile ini_file;
		if(ini_file.GetParam("config", "uilanguage", temp_buf) > 0 && temp_buf.NotEmptyS()) {
			const int slang = RecognizeLinguaSymb(temp_buf, 0);
			if(slang > 0)
                SLS.SetUiLanguageId(slang, 0);
		}
		THROW(PPInitStrings());
		{
			SlExtraProcBlock epb;
			SLS.GetExtraProcBlock(&epb);
            epb.F_LoadString = PPLoadStringFunc;
            epb.F_ExpandString = PPExpandStringFunc;
            epb.F_GetGlobalSecureConfig = PPGetGlobalSecureConfig;
            epb.F_CallHelp = PPCallHelp;
            epb.F_CallCalc = PPCalculator;
            epb.F_CallCalendar = ExecDateCalendar;
            epb.F_GetDefaultEncrKey = PPGetDefaultEncrKey; // @v9.4.6
            SLS.SetExtraProcBlock(&epb);
			//SLS.SetLoadStringFunc(PPLoadStringFunc);
			//SLS.SetExpandStringFunc(PPExpandStringFunc); // @v9.0.11
			//SLS.SetCallHelpFunc(PPCallHelp);
			//SLS.SetGlobalSecureConfigFunc(PPGetGlobalSecureConfig); // @v7.6.7
		}
		{
			//
			// @v8.0.2 Теперь флаг устанавливается по умолчанию. Параметром DETECTDBTEXISTBYOPEN его можно отменить
			//
			DbSession::Config dbcfg;
			DBS.GetConfig(dbcfg);
			int    iv = 0;
			dbcfg.Flags |= DbSession::fDetectExistByOpen;
			if(ini_file.GetInt(PPINISECT_CONFIG, PPINIPARAM_DETECTDBTEXISTBYOPEN, &iv) > 0) {
				if(iv == 0)
					dbcfg.Flags &= ~DbSession::fDetectExistByOpen;
				else if(iv == 100)
					SetExtFlag(ECF_DETECTCRDBTEXISTBYOPEN, 1);
			}
			// @v8.0.2 {
			if(ini_file.Get(PPINISECT_CONFIG, PPINIPARAM_BTRNWLOCK, temp_buf) > 0) {
				if(temp_buf.CmpNC("disable") == 0) { // @v8.6.3
					dbcfg.NWaitLockTries = BTR_RECLOCKDISABLE;
					dbcfg.NWaitLockTryTimeout = 0;
				}
				else {
					SString m, t;
					if(temp_buf.Divide(',', m, t) > 0) {
						if(m.ToLong() >= 0)
							dbcfg.NWaitLockTries = m.ToLong();
						if(t.ToLong() > 0)
							dbcfg.NWaitLockTryTimeout = t.ToLong();
					}
					else if(temp_buf.ToLong() >= 0)
						dbcfg.NWaitLockTries = temp_buf.ToLong();
				}
			}
			// } @v8.0.2
			DBS.SetConfig(&dbcfg);
			{
				// @v8.0.6 {
				int    max_log_file_size = 0;
				if(ini_file.GetInt(PPINISECT_CONFIG, PPINIPARAM_MAXLOGFILESIZE, &max_log_file_size) > 0) {
					if(max_log_file_size > 0 && max_log_file_size <= (1024*1024)) {
						MaxLogFileSize = max_log_file_size; // @v8.0.6
					}
				}
				// } @v8.0.6
			}
			if(flags & PPSession::fInitPaths) {
				MemLeakTracer mlt;
				SString path, root_path;
				PPGetPath(PPPATH_SYSROOT, root_path);
				{
					path = (ini_file.Get(PPINISECT_PATH, PPINIPARAM_TEMP, temp_buf = 0) > 0) ? temp_buf : (const char *)0;
					Helper_SetPath(PPPATH_TEMP, path);
				}
				{
					path = (ini_file.Get(PPINISECT_PATH, PPINIPARAM_LOG, temp_buf) > 0) ? temp_buf : (const char *)0;
					if(!path.NotEmptyS()) {
						PPIniFile::GetParamSymb(PPINIPARAM_LOG, temp_buf = 0);
						(path = root_path).SetLastSlash().Cat(temp_buf);
					}
					if(!isDir(path) && !createDir(path))
						path = root_path.RmvLastSlash();
					if(Helper_SetPath(PPPATH_LOG, path))
						SLS.SetLogPath(path);
				}
				{
					path = (ini_file.Get(PPINISECT_PATH, PPINIPARAM_PACK, temp_buf = 0) > 0) ? temp_buf : (const char *)0;
					if(!path.NotEmptyS())
						(path = root_path).SetLastSlash().Cat("PACK");
					Helper_SetPath(PPPATH_PACK, path);
				}
				{
					path = (ini_file.Get(PPINISECT_PATH, PPINIPARAM_SPII, temp_buf = 0) > 0) ? temp_buf : (const char *)0;
					Helper_SetPath(PPPATH_SPII, path);
				}
				LoadDriveMapping(&ini_file);
			}
		}
	}
	// @v8.9.12 {
	if(!(flags & fDenyLogQueue)) { // @v9.2.6 Для DLL-режима не используем поток журналов (какие-то траблы с потоками - надо разбираться)
		P_LogQueue = new PPLogMsgQueue;
		if(P_LogQueue) {
			PPLogMsgSession * p_sess = new PPLogMsgSession(P_LogQueue);
			p_sess->Start(0);
		}
	}
	// } @v8.9.12
	SetExtFlag(ECF_DBDICTDL600, 1);
	if(CheckExtFlag(ECF_DBDICTDL600))
		DbDictionary::SetCreateInstanceProc(DbDict_DL600::CreateInstance);
	InitTest();
	CATCHZOK
	return ok;
}

int SLAPI PPSession::InitThread(const PPThread * pThread)
{
	PPThreadLocalArea * p_tla = new PPThreadLocalArea;
	ENTER_CRITICAL_SECTION
	TlsSetValue(TlsIdx, p_tla);
	SString path;
	if(CommonPaths.GetPath(PPPATH_BIN, 0, path) > 0) {
		SetPath(PPPATH_BIN, path, 0, 1);
	}
	if(CommonPaths.GetPath(PPPATH_LOG, 0, path) > 0) {
		SetPath(PPPATH_LOG, path, 0, 1);
		SLS.SetLogPath(path);
	}
	if(CommonPaths.GetPath(PPPATH_TEMP, 0, path) > 0)
		SetPath(PPPATH_TEMP, path, 0, 1);
	// @v8.5.10 {
	if(CommonPaths.GetPath(PPPATH_SPII, 0, path) > 0)
		SetPath(PPPATH_SPII, path, 0, 1);
	// } @v8.5.10
	LEAVE_CRITICAL_SECTION
	if(pThread) {
		p_tla->TId = pThread->GetThreadID();
		ThreadList.Add(pThread);
	}
	else
		p_tla->TId = 0;
	p_tla->Id = LastThread.Incr();
	GetTLA().Lc.OperDate = getcurdate_();
	p_tla->Prf.InitUserProfile(0);
	return 1;
}

int SLAPI PPSession::ReleaseThread()
{
	PPThreadLocalArea * p_tla = (PPThreadLocalArea *)TlsGetValue(TlsIdx);
	if(p_tla) {
		ThreadList.Remove(p_tla->GetThreadID());
		delete p_tla;
		TlsSetValue(TlsIdx, 0);
	}
	else {
		assert(0);
	}
	return 1;
}
//
// См. примечание к определению функций PP.H
//
#define MAX_GETTLA_TRY 5

PPThreadLocalArea & SLAPI PPSession::GetTLA()
{
	return *(PPThreadLocalArea *)SGetTls(TlsIdx);
}

const PPThreadLocalArea & SLAPI PPSession::GetConstTLA() const
{
	return *(PPThreadLocalArea *)SGetTls(TlsIdx);
}

int PPSession::SetThreadNotification(int type, const void * pData)
{
	int    ok = -1;
	if(type == stntMessage)
		ok = ThreadList.SetMessage(GetConstTLA().GetThreadID(), 1, (const char *)pData);
	else if(type == stntText)
		ok = ThreadList.SetMessage(GetConstTLA().GetThreadID(), 0, (const char *)pData);
	return ok;
}

int PPSession::GetThreadInfoList(TSCollection <PPThread::Info> & rList)
{
	return ThreadList.GetInfoList(rList);
}

int PPSession::GetThreadInfo(ThreadID tId, PPThread::Info & rInfo)
{
	return ThreadList.GetInfo(tId, rInfo);
}

int SLAPI PPSession::LockingDllServer(int cmd)
{
	int    ok = 1;
	if(cmd == ldsLock)
		DllRef.Incr();
	else if(cmd == ldsUnlock)
		DllRef.Decr();
	else if(cmd == ldsCanUnload)
		ok = DllRef ? 0 : 1;
	else {
		GetTLA().LastErr = PPERR_INVPARAM;
		ok = 0;
	}
	return ok;
}

int FASTCALL PPSession::PushLogMsgToQueue(const PPLogMsgItem & rItem)
{
	return P_LogQueue ? P_LogQueue->Push(rItem) : -1;
}
//
//
//
int SLAPI PPSession::Helper_SetPath(int pathId, SString & rPath)
{
	int    ok = 0;
	if(rPath.NotEmptyS()) {
		ENTER_CRITICAL_SECTION
		CommonPaths.SetPath(pathId, rPath, 0, 1);
		LEAVE_CRITICAL_SECTION
		SetPath(pathId, rPath, 0, 1);
		ok = 1;
	}
	return ok;
}
//
//
//
int SLAPI PPSession::LogAction(PPID action, PPID obj, PPID id, long extData, int use_ta)
{
	int    ok = -1;
	if(action) {
		SysJournal * p_sj = GetTLA().P_SysJ;
		ok = p_sj ? p_sj->LogEvent(action, obj, id, extData, use_ta) : -1;
	}
	return ok;
}

GtaJournalCore * SLAPI PPSession::GetGtaJ()
{
	PPThreadLocalArea & r_tla = GetTLA();
	if(!r_tla.P_GtaJ)
		r_tla.P_GtaJ = new GtaJournalCore;
	return r_tla.P_GtaJ;
}
//
//
//
int SLAPI PPSession::MakeMachineID(MACAddr * pMachineID)
{
	int    ok = -1;
	MACAddr addr;
	if(GetFirstMACAddr(&addr)) {
		ok = 2;
	}
	else {
		char buf[32];
		IdeaRandMem(buf, sizeof(buf));
		memcpy(addr.Addr, buf+3, sizeof(addr.Addr));
		ok = 1;
	}
	ASSIGN_PTR(pMachineID, addr);
	return ok;
}

int SLAPI PPSession::GetMachineID(MACAddr * pMachineID, int forceUpdate)
{
	int    ok = -1;
	FILE * f = 0;
	MACAddr machine_id;
	if(GetFirstMACAddr(&machine_id)) {
		ok = 1;
	}
	else {
		const  long   signature = 0x494D5050L; // "PPMI"
		char   fname[MAXPATH], buf[32];
		STRNSCPY(fname, "c:\\ppmchnid");
		if(fileExists(fname)) {
			PPSetAddedMsgString(fname);
			THROW_PP(f = fopen(fname, "r"), PPERR_CANTOPENFILE);
			fread(buf, sizeof(signature), 1, f);
			if(*(long*)buf == signature) {
				LTIME t = getcurtime_();
				if(!forceUpdate || (t % 17) != 1) {
					fread(&machine_id, sizeof(machine_id), 1, f);
					ok = 1;
				}
			}
		}
		if(ok < 0) {
			THROW_PP_S(f = fopen(fname, "w"), PPERR_CANTOPENFILE, fname);
			MakeMachineID(&machine_id);
			memcpy(buf, &signature, sizeof(signature));
			fwrite(buf, sizeof(signature), 1, f);
			fwrite(&machine_id, sizeof(machine_id), 1, f);
			SFile::ZClose(&f);
			{
				DWORD fattr = GetFileAttributes(fname);
				if(fattr != 0xffffffff)
					::SetFileAttributes(fname, fattr | FILE_ATTRIBUTE_HIDDEN); // @unicodeproblem
			}
		}
	}
	CATCH
		ok = 0;
		MakeMachineID(&machine_id);
	ENDCATCH
	SFile::ZClose(&f);
	ASSIGN_PTR(pMachineID, machine_id);
	return ok;
}

int SLAPI LogTerminalSessInfo(ulong processID, ulong termSessID, const char * pAddMsgString)
{
	/* @v7.9.9 Пользы не получили, а журнал забивается //
	SString msg_buf, buf;
	PPLoadString(PPSTR_TEXT, PPTXT_TERMINALSESSINFO, buf);
	msg_buf.Printf((const char*)buf, pAddMsgString, (long)processID, (long)termSessID);
	PPLogMessage(PPFILNAM_INFO_LOG, msg_buf, LOGMSGF_TIME|LOGMSGF_USER|LOGMSGF_DBINFO);
	*/
	return 1;
}

int SLAPI PPSession::CheckLicense(MACAddr * pMachineID, int * pIsDemo)
{
	int    ok = -1;
	ulong  cur_term_sess_id = 0;
	MACAddrArray ma_list;
	MACAddr machine_id;
	PPLicData lic;
	int32  max_user_count = CheckExtFlag(ECF_OPENSOURCE) ? 1000 : ((PPGetLicData(&lic) > 0) ? lic.LicCount : 0);
	if(!max_user_count) {
		ASSIGN_PTR(pIsDemo, 1);
		max_user_count = 1;
	}
	else
		ASSIGN_PTR(pIsDemo, 0);
	GetMachineID(&machine_id, 0);
	{
		const ulong cur_process_id = GetCurrentProcessId();
		const char * p_func_name = "CheckLicense";
		if(!ProcessIdToSessionId(cur_process_id, &cur_term_sess_id)) {
			SString msg_buf;
			PPGetMessage(mfError, PPERR_SLIB, 0, 0, msg_buf);
			msg_buf.Space().CatEq("Function", p_func_name);
			PPLogMessage(PPFILNAM_ERR_LOG, msg_buf, LOGMSGF_TIME|LOGMSGF_USER|LOGMSGF_DBINFO);
		}
		LogTerminalSessInfo(cur_process_id, cur_term_sess_id, p_func_name);
	}
	{
		int    this_machine_logged = 0;
		PPSyncArray sync_array;
		GetSync().GetItemsList(PPSYNC_DBLOCK, &sync_array);
		for(uint i = 0; i < sync_array.getCount(); i++) {
			const PPSyncItem & r_item = sync_array.at(i);
			const ulong  term_sess_id = r_item.TerminalSessID;
			const MACAddr ma = r_item.MchnID;
			if(r_item.ObjID != 1) // @v7.1.0 Серверные сессии не учитываем при подсчете занятых лицензий
				ma_list.addUnique(ma);
			if(ma.Cmp(machine_id) == 0 && (cur_term_sess_id == 0 || cur_term_sess_id == term_sess_id))
				this_machine_logged = 1;
		}
		ok = (this_machine_logged || max_user_count > (int32)ma_list.getCount()) ? 1 : -1;
		if(!this_machine_logged)
			GetMachineID(&machine_id, 1);
	}
	ASSIGN_PTR(pMachineID, machine_id);
	return ok;
}

struct _E {
	ulong   TerminalSessID;
	MACAddr MchnID;
};

IMPL_CMPFUNC(_E, i1, i2)
{
	int    r = 0;
	_E   * p_e1 = (_E*)i1;
	_E   * p_e2 = (_E*)i2;
	if((r = p_e1->MchnID.Cmp(p_e2->MchnID)) > 0)
		return 1;
	else if(r < 0)
		return -1;
	else
		return cmp_long(p_e1->TerminalSessID, p_e2->TerminalSessID);
}

int SLAPI PPSession::GetUsedLicCount(int32 * pUsedLicCount)
{
	int    ok  = 1;
	int32  used_lic_count = 0;
	SArray machine_list(sizeof(_E));
	PPSyncArray sync_array;
	GetSync().GetItemsList(PPSYNC_DBLOCK, &sync_array);
	for(uint i = 0; i < sync_array.getCount(); i++) {
		_E _e;
		_e.TerminalSessID = sync_array.at(i).TerminalSessID;
		_e.MchnID         = sync_array.at(i).MchnID;
		if(!machine_list.lsearch(&_e, 0, PTR_CMPFUNC(_E)))
			machine_list.insert(&_e);
	}
	used_lic_count = machine_list.getCount();
	ASSIGN_PTR(pUsedLicCount, used_lic_count);
	return ok;
}

// Prototype
int SLAPI __BTest();

static int _dbOpenException(const char * pFileName, int btrErr)
{
	BtrError = NZOR(btrErr, BE_FILNOPEN);
	SString temp_buf = pFileName;
	PPError(PPERR_DBENGINE, temp_buf.ToOem());
	if(APPL)
		APPL->CloseAllBrowsers();
	DS.Logout();
	exit(-1);
	return 0;
}

static int _dbLoadStructure(const char * pTblName, DBTable * pTbl, long options)
{
	int    ok = 1;
	DlContext * p_ctx = DS.GetInterfaceContext(PPSession::ctxDatabase);
	THROW(p_ctx);
	THROW(p_ctx->LoadDbTableSpec(pTblName, pTbl, 0));
	CATCHZOK
	return ok;
}
//
//
//
int SLAPI PPSession::OpenDictionary2(DbLoginBlock * pBlk, long flags)
{
	int    ok = 1, r;
	SString data_path, temp_path, temp_buf;

	PPVersionInfo ver_inf(0);
	const SVerT this_ver   = ver_inf.GetVersion();
	const SVerT this_db_min_ver = ver_inf.GetVersion(1);

	pBlk->GetAttr(DbLoginBlock::attrDbPath, data_path);
	PPVerHistory verh;
	PPVerHistory::Info vh_info;
	//
	// Проверяем доступность каталога базы данных
	//
	THROW_PP_S(::access(data_path, 0) == 0, PPERR_DBDIRNFOUND, data_path);
	if(!(flags & PPSession::odfDontInitSync)) {
		//
		// Инициализируем таблицу блокировок и проверяем не заблокирована ли база данных
		//
		THROW(GetSync().Init(data_path)); // @todo InitSync(data_path)
		THROW_PP(!GetSync().IsDBLocked(), PPERR_SYNCDBLOCKED);
	}
	GetPath(PPPATH_TEMP, temp_path);
	pBlk->SetAttr(DbLoginBlock::attrTempPath, temp_path);
	//
	// Считываем информацию о версии базы данных и проверяем не является ли
	// текущая версия системы меньше допустимой для этой базы данных.
	//
	THROW(r = verh.Read(data_path, &vh_info));
	if(r > 0 && this_ver.Cmp(&vh_info.MinVer) < 0) {
		int    mj, mn, r;
		vh_info.MinVer.Get(&mj, &mn, &r);
		CALLEXCEPT_PP_S(PPERR_MINVER, temp_buf.CatDotTriplet(mj, mn, r));
	}
	if(this_ver.Cmp(&vh_info.CurVer) > 0 || vh_info.DbUUID.IsZero()) {
		vh_info.MinVer = this_db_min_ver;
		vh_info.CurVer = this_ver;
		if(vh_info.DbUUID.IsZero())
			vh_info.DbUUID.Generate();
		THROW(verh.Write(data_path, &vh_info));
	}
	pBlk->SetAttr(DbLoginBlock::attrDbUuid, vh_info.DbUUID.ToStr(S_GUID::fmtIDL, temp_buf));
	DBTable::OpenExceptionProc = _dbOpenException;
	//
	// Теперь, когда проверки и инициализация, относящаяся к собственно Papyrus'у завершены
	// можно создавать экземпляр провайдера базы данных и инициализировать его.
	//
	{
		DbProvider * p_db = 0;
		pBlk->GetAttr(DbLoginBlock::attrServerType, temp_buf);
		if(temp_buf.CmpNC("ORACLE") == 0) {
			// @todo SOraDbProvider должен инициализировать DbPathID
			THROW_MEM(p_db = new SOraDbProvider(data_path));
		}
		else {
			pBlk->GetAttr(DbLoginBlock::attrDictPath, temp_buf);
			THROW_MEM(p_db = new BDictionary(temp_buf, data_path, temp_path));
		}
		THROW_DB(p_db->Login(pBlk, 0));
		THROW_DB(DBS.OpenDictionary2(p_db));
	}
	CATCHZOK
	return ok;
}

int SLAPI PPSession::SetupConfigByOps()
{
	int    ok = 1;
	int    missingnoupdrestopflag = 0;
	PPObjOprKind op_obj;
	PPOprKind op_rec;
	PPCommConfig & cc = GetTLA().Cc;
	for(PPID op_id = 0; EnumOperations(0, &op_id, &op_rec) > 0;) {
		if(!(cc.Flags & CCFLG_USENOUPDRESTOPFLAG) && op_rec.Flags & OPKF_NOUPDLOTREST)
			missingnoupdrestopflag = 1;
		if(op_rec.OpTypeID == PPOPT_ACCTURN && op_rec.Flags & OPKF_ADVACC)
			cc.Flags |= CCFLG_USEADVBILLITEMS;
		if(oneof2(op_rec.OpTypeID, PPOPT_DRAFTRECEIPT, PPOPT_DRAFTEXPEND))
			cc.Flags |= CCFLG_USEDRAFTBILL;
	}
	if(missingnoupdrestopflag)
		PPMessage(mfInfo, PPINF_MISSINGNOUPDRESTOPFLAG, 0);
	return ok;
}

int SLAPI PPSession::FetchConfig(PPID obj, PPID objID, PPConfig * pCfg)
{
	int    ok = 1, r;
	PPConfig tmp, global;
	Reference * p_ref = GetTLA().P_Ref;
	// @v9.4.8 (constructor) MEMSZERO(tmp);
	// @v9.4.8 (constructor) MEMSZERO(global);
	if(objID == 0) {
		objID = DEFCFG_USERID;
		r = -1;
	}
	else {
		THROW(r = p_ref->GetConfig(obj, objID, PPPRP_CFG, &tmp, sizeof(tmp)));
	}
	if(r < 0) {
		MEMSZERO(tmp);
		tmp.Tag           = 0;
		tmp.ObjID         = objID;
		tmp.PropID        = PPPRP_CFG;
		tmp.AccessLevel   = DEFCFG_ACCESS;
		tmp.BaseCurID     = DEFCFG_CURRENCY;
		tmp.RealizeOrder  = DEFCFG_RLZORD;
		tmp.Menu          = DEFCFG_MENU;
		tmp.LocAccSheetID = DEFCFG_LOCSHEET;
		tmp.Location      = DEFCFG_LOCATION;
		tmp.Flags         = DEFCFG_FLAGS;
	}
	if(r <= 0 || tmp.Tag == PPOBJ_CONFIG || p_ref->GetConfig(PPOBJ_CONFIG, PPCFG_MAIN, PPPRP_CFG, &global, sizeof(global)) <= 0)
		global = tmp;
	tmp.OperDate = GetTLA().Lc.OperDate;
	tmp.User  = (obj == PPOBJ_USR) ? objID : 0;
	tmp.State = 0;
	if(obj == PPOBJ_USR && objID == PPUSR_MASTER)
		tmp.State |= CFGST_MASTER;
	if(tmp.Tag < obj)
		tmp.State |= CFGST_INHERITED;
	*pCfg = tmp;
	pCfg->Flags |= (CFGFLG_UNITEINTRTRFR | CFGFLG_FORCEMANUF);
	//
	// Установка глобальных флагов и параметров
	//
	SETFLAG(pCfg->Flags, CFGFLG_FREEPRICE, global.Flags & CFGFLG_FREEPRICE);
	SETFLAG(pCfg->Flags, CFGFLG_MULTICURACCT, global.Flags & CFGFLG_MULTICURACCT);
	pCfg->DBDiv     = global.DBDiv;
	pCfg->BaseCurID = global.BaseCurID;
	pCfg->BaseRateTypeID = global.BaseRateTypeID;
	CATCHZOK
	return ok;
}

int SLAPI PPSession::FetchAlbatrosConfig(PPAlbatrosConfig * pCfg)
{
	int    ok = 1;
	ENTER_CRITICAL_SECTION
	if(pCfg) {
		if(!P_AlbatrosCfg) {
			P_AlbatrosCfg = new PPAlbatrosConfig;
			if(!P_AlbatrosCfg) {
				ok = PPSetErrorNoMem();
			}
			else {
				ok = PPAlbatrosCfgMngr::Get(P_AlbatrosCfg);
				if(ok < 0) {
					PPSetError(PPERR_UNDEFALBATROSCONFIG);
				}
			}
		}
		if(ok) {
			*pCfg = *P_AlbatrosCfg;
		}
	}
	else {
		ZDELETE(P_AlbatrosCfg);
	}
	LEAVE_CRITICAL_SECTION
	return ok;
}

int SLAPI PPSession::CheckSystemAccount(DbLoginBlock * pDlb, PPSecur * pSecur)
{
	int    ok = -1;
	char   domain_user[64];
	DWORD  duser_len = sizeof(domain_user);
	memzero(domain_user, sizeof(domain_user));
	THROW(OpenDictionary2(pDlb, odfDontInitSync)); // @v9.4.9 odfDontInitSync
	if(::GetUserName(domain_user, &duser_len)) { // @unicodeproblem
		PPID   user_id = 0;
		Reference ref;
		if(ref.SearchName(PPOBJ_USR, &user_id, domain_user) > 0) {
			char   pw[32];
			SString domain;
			memzero(pw, sizeof(pw));
			PPIniFile ini_file;
			ini_file.Get(PPINISECT_SYSTEM, PPINIPARAM_DOMAINNAME, domain);
			const PPSecur & r_secur = *(PPSecur*)&ref.data;
			Reference::GetPassword(&r_secur, pw, sizeof(pw));
			if(SCheckSystemCredentials(domain, domain_user, pw)) {
				ASSIGN_PTR(pSecur, r_secur);
				ok = 1;
			}
			memzero(pw, sizeof(pw));
		}
	}
	CATCHZOK
	DBS.CloseDictionary();
	return ok;
}

int SLAPI PPSession::Login(const char * pDbSymb, const char * pUserName, const char * pPassword)
{
	enum {
        logmOrdinary          = 0,
        logmSystem            = 1, // Под именем SYSTEM
        logmService           = 2, // Под именем PPSession::P_JobLogin
        logmEmptyBaseCreation = 3  // Под именем PPSession::P_EmptyBaseCreationLogin
	};
	int    ok = 1, r;
	int    debug_r = 0;
	SString dict_path, data_path, db_symb, msg_buf, temp_buf;
	PPIniFile ini_file(0, 0, 0, 1);
	PPDbEntrySet2 dbes;
	DbLoginBlock blk;
	char    pw[128];

	THROW(ini_file.IsValid());
	debug_r = 1;
	THROW(dbes.ReadFromProfile(&ini_file, 0));
	debug_r = 2;
	db_symb = pDbSymb;
	THROW_SL(dbes.GetBySymb(db_symb, &blk));
	debug_r = 3;
	blk.GetAttr(DbLoginBlock::attrDbPath, data_path);
	blk.GetAttr(DbLoginBlock::attrDictPath, dict_path);
	{
		PPThreadLocalArea & r_tla = GetTLA();
		MACAddr machine_id;
		//int    is_system_login = 0;
		//int    is_service_login = 0;
		int    logmode = logmOrdinary;
		int    empty_secur_base = 0, is_demo = 0;
		char   user_name[64];
		ulong  term_sess_id = 0;
		PPID   id;
		LDATE  cdt;
		LTIME  ctm;
		PPSecur usr_rec;
		PPConfig & r_lc = r_tla.Lc;
		PPCommConfig & r_cc = r_tla.Cc;
		STRNSCPY(user_name, pUserName);
		r_tla.State &= ~PPThreadLocalArea::stAuth; // @v8.6.11
		THROW(OpenDictionary2(&blk, 0));
		debug_r = 4;
		r_tla.Prf.InitUserProfile(user_name); // @v8.0.6 Инициализация профайлера с параметрами БД сразу после соединения с сервером БД.
		r_tla.UfpSess.Begin(PPUPRF_SESSION); // @v8.0.6 Профилирование всей сессии работы в БД (Login..Logout)
		PPUserFuncProfiler ufp(PPUPRF_LOGIN); // @v8.0.6 Профилирование собственно процесса авторизации в базе данных
		const long db_path_id = DBS.GetDbPathID();
		DbProvider * p_dict = CurDict;
		{
			//
			// Имя SYSTEM является встроенным аналогом имени MASTER и отличается //
			// от него только идентификатором меню (MENU_SYSTEM)
			//
			if(stricmp(user_name, "SYSTEM") == 0) {
				STRNSCPY(user_name, "MASTER");
				// @v9.4.8 is_system_login = 1;
				logmode = logmSystem; // @v9.4.8
			}
			else if(stricmp(user_name, PPSession::P_JobLogin) == 0)
				logmode = logmService;
			else if(stricmp(user_name, PPSession::P_EmptyBaseCreationLogin) == 0)
				logmode = logmEmptyBaseCreation;
		}
		{
			//
			// Следующий блок запускается только если в эту БД данным процессом не был осуществлен хотя бы один вход
			//
			ENTER_CRITICAL_SECTION
			if(!CMng.HasDbEntry(db_path_id)) {
				//
				// Тестирование доступности каталогов (только для сервера)
				//
				if(CheckExtFlag(ECF_SYSSERVICE)) {
					DBTable::OpenExceptionProc = 0;
					const  char * p_test_tbl_name = "TempAssoc";
					SString file_name;
					DBTable * p_test_tbl = new DBTable;
					THROW_MEM(p_test_tbl);
					debug_r = 5;
					THROW_DB(p_dict->CreateTempFile(p_test_tbl_name, file_name, 1));
					debug_r = 6;
					THROW_DB(p_test_tbl->open(p_test_tbl_name, file_name));
					debug_r = 7;
					ZDELETE(p_test_tbl);
					p_dict->DropFile(file_name);
					DBTable::OpenExceptionProc = _dbOpenException;
				}
				//
				// Процедура проверки необходимости конвертации и собственно конвертации
				// не запускается если провайдер базы данных НЕ Btrieve (SQL-сервера в разработке)
				//
				if(!(p_dict->GetCapability() & DbProvider::cSQL)) {
					//
					// Блок конвертации данных.
					//
					SString dbr_signal_file_name;
					PPVersionInfo ver_inf(0);
					const SVerT this_ver = ver_inf.GetVersion();
					const SVerT this_db_min_ver = ver_inf.GetVersion(1);
					{
						int   mj, mn, rv;
						this_ver.Get(&mj, &mn, &rv);
                        (dbr_signal_file_name = data_path).SetLastSlash().Cat("dbr").
							CatChar('-').CatLongZ(mj, 2).CatLongZ(mn, 2).CatLongZ(rv, 2).Dot().Cat("signal");
					}
					if(!::fileExists(dbr_signal_file_name)) {
						THROW_PP(!GetSync().IsDBLocked(), PPERR_SYNCDBLOCKED);
						debug_r = 8;
						PPWait(1);

						// @v4.7.7 Convert400();
						// @v4.7.7 Convert31102();
						// @v4.7.7 Convert31110();

						// ------

						// @v4.7.7 Convert4108();
						// [Перенесено в Convert6202()] // @v5.5.1 THROW(Convert4208());
						// @v5.5.1 THROW(Convert4402());
						// @v5.5.1 THROW(Convert4405());
						// @v5.6.8 THROW(Convert4515());
						// @v6.3.3 THROW(Convert4707());
						// @v6.3.3 THROW(Convert4802()); // AHTOXA
						// @v6.3.3 THROW(Convert4805());
						// @v6.3.3 THROW(Convert4911());
						/* bagirov
						THROW(Convert4515());
						THROW(Convert4707());
						THROW(Convert4802()); // AHTOXA
						THROW(Convert4805());
						THROW(Convert4911());
						*/
						// ------

						// [Перенесено в Convert6202()] THROW(Convert5006()); // VADIM
						// [Перенесено в Convert6407()] THROW(Convert5009()); // @v5.0.9 AHTOXA
						// [Перенесено в Convert5200()] THROW(Convert5109()); // @v5.1.9
						// @v7.8.10 THROW(Convert5200()); // @v5.2.0
						// @v7.8.10 THROW(Convert5207());
						// @v7.8.10 THROW(Convert5501()); // @v5.5.1
						// [Перенесено в Convert6202()] THROW(Convert5512()); // @v5.5.12
						// @v7.8.10 THROW(Convert5608()); // @v5.6.8
						// [Перенесено в Convert6202()] THROW(Convert5506()); // @v5.5.6 VADIM
						// @v7.8.10 THROW(Convert5810()); // @v5.8.10
						// @v8.6.1 THROW(Convert6202()); // @v6.1.9 + @v6.2.2
						// @v8.6.1 THROW(Convert6303()); // @v6.3.3
						// @v8.6.1 THROW(Convert6407()); // @v6.4.7
						// @v8.6.1 THROW(Convert6611()); // @v6.6.11
						// @v7.6.01 THROW(Convert6708()); // @v6.7.8
						// @v7.3.11 Конвертация перенесена в Convert7311() THROW(Convert7208()); // @v7.2.8
						THROW(Convert7305()); // @v7.3.5
						THROW(Convert7311()); // @v7.3.11
						THROW(Convert7506()); // @v7.5.6
						THROW(Convert7601()); // @v7.6.1
						// @v9.4.0 (Перенесено в Convert9400) THROW(Convert7702()); // @v7.7.2
						THROW(Convert7708()); // @v7.7.8
						THROW(Convert7712()); // @v7.7.12
						THROW(Convert7907());
						// @v8.3.6 THROW(Convert8203());
						THROW(Convert8306());
						THROW(Convert8800());
						THROW(Convert8910()); // @v8.9.10
						THROW(Convert9004()); // @v9.0.3 // @v9.0.4 Convert9003-->Convert9004
						THROW(Convert9108()); // @v9.1.8 GoodsDebt
						THROW(Convert9214()); // @v9.2.14 EgaisProduct
						THROW(Convert9400()); // @v9.4.0
						{
							PPVerHistory verh;
							PPVerHistory::Info vh_info;
							THROW(r = verh.Read(data_path, &vh_info));
							if(this_ver.Cmp(&vh_info.CurVer) > 0 || vh_info.DbUUID.IsZero()) {
								vh_info.MinVer = this_db_min_ver;
								vh_info.CurVer = this_ver;
								if(vh_info.DbUUID.IsZero())
									vh_info.DbUUID.Generate();
								THROW(verh.Write(data_path, &vh_info));
							}
						}
                        ::createEmptyFile(dbr_signal_file_name);
						PPWait(0);
					}
				}
			}
			LEAVE_CRITICAL_SECTION
		}
		THROW_MEM(PPRef = new Reference);
		{
			Reference * p_ref = PPRef;
			debug_r = 9;
			if(oneof2(logmode, logmService, logmEmptyBaseCreation)) {
				char   secret[64];
				PPVersionInfo vi = Ver;
				THROW(vi.GetSecret(secret, sizeof(secret)));
				r = stricmp(pPassword, secret);
				memzero(secret, sizeof(secret));
				THROW_PP(r == 0, PPERR_INVPASSWORD);
				// @v9.4.8 is_service_login = 1;
			}
			THROW(r = p_ref->SearchName(PPOBJ_USR, &r_lc.User, user_name));
			if(r < 0) {
				id = 0;
				THROW(r = p_ref->EnumItems(PPOBJ_USR, &id));
				THROW_PP(r < 0, PPERR_INVUSERNAME);
				empty_secur_base = 1;
				r_lc.User = 0;
				pw[0] = 0;
			}
			else {
				usr_rec = *(PPSecur*)&p_ref->data;
				THROW(Reference::VerifySecur(&usr_rec, 0));
				Reference::GetPassword(&usr_rec, pw, sizeof(pw));
			}
			THROW(FetchConfig(PPOBJ_USR, r_lc.User, &r_lc));
			// @v9.1.1 r_lc.Flags &= ~CCFLG_USELARGEDIALOG; // @v7.6.2 Введено после непонятного инцидента с крупными списками выбора
			SLS.SetUiFlag(sluifUseLargeDialogs, 0); // @v9.1.1
			if(!oneof2(logmode, logmService, logmEmptyBaseCreation)) {
				int    pw_is_wrong = 1;
				if(pw[0] && (r_lc.Flags & CFGFLG_SEC_CASESENSPASSW) ? strcmp(pw, pPassword) : stricmp866(pw, pPassword)) {
					if(logmode == logmSystem) {
						// для совместимости со старыми версиями (раньше использовался другой механизм шифрования)
						decrypt((char*)memcpy(pw, usr_rec.Password, sizeof(pw)), sizeof(pw));
						if(stricmp866(pw, pPassword) == 0)
							pw_is_wrong = 0;
					}
				}
				else
					pw_is_wrong = 0;
				memzero(pw, sizeof(pw));
				THROW_PP(pw_is_wrong == 0, PPERR_INVPASSWORD);
				if(!CheckExtFlag(ECF_SYSSERVICE)) {
					//
					// @v7.0.12 Блок перенесен в это место по причине того, что уникальность входа не должна
					// проверяться для сеансов JobServer'а {
					//
					if(r_lc.Flags & CFGFLG_SEC_DSBLMULTLOGIN) {
						PPSyncArray sync_array;
						GetMachineID(&machine_id, 0);
						GetSync().GetItemsList(PPSYNC_DBLOCK, &sync_array);
						for(uint i = 0; ok && i < sync_array.getCount(); i++) {
							PPSyncItem & r_item = sync_array.at(i);
							THROW_PP(r_item.ObjID == 1 || stricmp866(r_item.Name, user_name) != 0 || r_item.MchnID.Cmp(machine_id) == 0, PPERR_DUPLOGINGDISABLED);
						}
					}
					// }
					{
						PPObjSecur sec_obj(PPOBJ_USR, 0);
						sec_obj.GetPrivateDesktop(r_lc.User, &r_lc.DesktopID);
					}
				}
			}
			THROW(GetCommConfig(&r_cc));
			{
				PPSupplAgreement suppl_agt;
				PPObjArticle::GetSupplAgreement(0, &suppl_agt, 0);
				r_tla.SupplDealQuotKindID        = suppl_agt.CostQuotKindID;
				r_tla.SupplDevUpQuotKindID       = suppl_agt.DevUpQuotKindID;
				r_tla.SupplDevDnQuotKindID       = suppl_agt.DevDnQuotKindID;
				r_tla.InvalidSupplDealQuotAction = suppl_agt.InvPriceAction;
				SETFLAG(r_cc.Flags2, CCFLG2_USESDONPURCHOP, suppl_agt.Flags & AGTF_USESDONPURCHOP); // @v7.2.2
			}
			if(!empty_secur_base)
				THROW(r_tla.Paths.Get(PPOBJ_USR, r_lc.User));
			SetPath(PPPATH_DAT, data_path, 0, 1);
			SetPath(PPPATH_SYS, dict_path, 0, 1);
			r_tla.UserName = user_name;
			if(/*is_system_login*/logmode == logmSystem)
				r_lc.Menu = MENU_SYSTEM;
			else if(/*is_service_login*/logmode == logmService)
				r_lc.Menu = -1;
			if(r_lc.User && r_lc.User != PPUSR_MASTER) {
				PPAccessRestriction accsr;
				THROW(r_tla.Rights.Get(PPOBJ_USR, r_lc.User));
				r_tla.Rights.GetAccessRestriction(accsr);
				r_tla.Rights.ExtentOpRights();
				getcurdatetime(&cdt, &ctm);
				if(usr_rec.PwUpdate && accsr.PwPeriod && diffdate(&cdt, &usr_rec.PwUpdate, 0) > accsr.PwPeriod)
					r_lc.State |= CFGST_PWEXPIRED;
				r_lc.AccessLevel = accsr.AccessLevel;
			}
			else
				r_lc.AccessLevel   = 0;
			THROW_PP(CheckLicense(&machine_id, &is_demo) > 0, PPERR_MAX_SESSION_DEST);
			SETFLAG(r_lc.State, CFGST_DEMOMODE, is_demo);
			if(!CMng.HasDbEntry(db_path_id)) {
				const ulong cur_process_id = GetCurrentProcessId();
				const char * p_func_name = "Login";
				if(!ProcessIdToSessionId(cur_process_id, &term_sess_id)) {
					PPGetMessage(mfError, PPERR_SLIB, 0, 0, msg_buf = 0);
					msg_buf.Space().CatEq("Function", p_func_name);
					PPLogMessage(PPFILNAM_ERR_LOG, msg_buf, LOGMSGF_TIME|LOGMSGF_USER|LOGMSGF_DBINFO);
				}
				LogTerminalSessInfo(cur_process_id, term_sess_id, p_func_name);
			}
			GetSync().LoginUser(r_lc.User, user_name, &r_lc.SessionID, &machine_id, term_sess_id);
			r_lc.State |= CFGST_INITIATE;
			if(empty_secur_base)
				r_lc.State |= CFGST_EMPTYBASE;
			/* @v8.6.1 @1 Блок перенесен ниже с целью оптимизации производительности (порядок создания объектов данных)
			if(r_lc.MainOrg)
				SetMainOrgID(r_lc.MainOrg, 1);
			else if(r_cc.MainOrgID)
				SetMainOrgID(r_cc.MainOrgID, 1);
			if(r_lc.Location)
				SetLocation(r_lc.Location);
			if(r_lc.DBDiv) {
				r_tla.CurDbDivName.Id = r_lc.DBDiv;
				GetObjectName(PPOBJ_DBDIV, r_lc.DBDiv, r_tla.CurDbDivName);
			}
			*/
			//
			// Флаг ECF_FULLGOODSCACHE должен быть определен до создания экземпляра
			// PPObjGoods (который создается внутри конструктора PPObjBill)
			//
#ifndef NDEBUG
			//ExtFlags |= ECF_FULLGOODSCACHE;
#endif
			//
			SetupConfigByOps(); // Must be called before 'new PPObjBill'
			THROW_MEM(BillObj = new PPObjBill(0));
			// @v8.6.1 Блок перенесен из позиции @1 {
			if(r_lc.MainOrg)
				SetMainOrgID(r_lc.MainOrg, 1);
			else if(r_cc.MainOrgID)
				SetMainOrgID(r_cc.MainOrgID, 1);
			if(r_lc.Location)
				SetLocation(r_lc.Location);
			if(r_lc.DBDiv) {
				r_tla.CurDbDivName.Id = r_lc.DBDiv;
				GetObjectName(PPOBJ_DBDIV, r_lc.DBDiv, r_tla.CurDbDivName);
			}
			// } @v8.6.1
			if(!(p_dict->GetCapability() & DbProvider::cSQL)) { // @debug
				if(PPCheckDatabaseChain() == 0) {
					delay(10000);
					CALLEXCEPT();
				}
			}
			THROW_MEM(r_tla.P_SysJ = new SysJournal);
			THROW_MEM(r_tla.P_ObjSync = new ObjSyncCore);
			// @v8.2.5 (moved down) LogAction(PPACN_LOGIN, 0, 0, r_lc.SessionID, 1);
			{
				int    iv;
				SString sv;
				LDATE  dt;
				if(!CheckExtFlag(ECF_INITONLOGIN)) {
					if(!CheckExtFlag(ECF_INITONLOGIN)) {
						// @v8.0.3 ExtFlags = (ExtFlags & (ECF_SYSSERVICE | ECF_DBDICTDL600));
						SetExtFlag(~(ECF_SYSSERVICE|ECF_DBDICTDL600|ECF_DETECTCRDBTEXISTBYOPEN|ECF_OPENSOURCE), 0); // @v8.0.3 // @v8.2.4 ECF_DETECTCRDBTEXISTBYOPEN // @v9.4.9 ECF_OPENSOURCE
						if(ini_file.GetInt(PPINISECT_CONFIG, PPINIPARAM_GRPACK, &(iv = 0)) > 0 && iv == 1)
							SetExtFlag(ECF_GOODSRESTPACK, 1);
						if(ini_file.GetInt(PPINISECT_CONFIG, PPINIPARAM_TIDPACK, &(iv = 0)) > 0 && iv == 1)
							SetExtFlag(ECF_TRFRITEMPACK, 1);
						if(ini_file.GetInt(PPINISECT_CONFIG, PPINIPARAM_GBFSDEBT, &(iv = 0)) > 0 && iv == 1)
							SetExtFlag(ECF_GOODSBILLFILTSHOWDEBT, 1);
						if(ini_file.GetInt(PPINISECT_CONFIG, PPINIPARAM_ECOGOODSSEL, &(iv = 0)) > 0 && iv == 1)
							SetExtFlag(ECF_ECOGOODSSEL, 1);
						if(ini_file.GetInt(PPINISECT_CONFIG, PPINIPARAM_433_OLDGENBARCODEMETHOD, &(iv = 0)) > 0 && iv == 1)
							SetExtFlag(ECF_433OLDGENBARCODEMETHOD, 1);
						if(ini_file.GetInt(PPINISECT_CONFIG, PPINIPARAM_AVERAGE, &(iv = 0)) > 0 && iv == 1)
							SetExtFlag(ECF_AVERAGE, 1);
						{
							SetExtFlag(ECF_PREPROCBRWONCHGFILT, 1);
							if(ini_file.GetInt(PPINISECT_SYSTEM, PPINIPARAM_PREPROCESSBRWFROMCHNGFLT, &(iv = 0)) > 0 && iv == 0)
								SetExtFlag(ECF_PREPROCBRWONCHGFILT, 0);
						}
						if(ini_file.GetInt(PPINISECT_CONFIG, PPINIPARAM_SHTRIH_USEGOODSLOCASSOC, &(iv = 0)) > 0 && iv != 0)
							SetExtFlag(ECF_CHKPAN_USEGDSLOCASSOC, 1);
						// @v7.3.11 {
						if(ini_file.GetInt(PPINISECT_CONFIG, PPINIPARAM_DEBUG_MTX_DIRTY, &(iv = 0)) > 0 && iv != 0)
							SetExtFlag(ECF_DEBUGDIRTYMTX, 1);
						// } @v7.3.11
						// @v7.4.8 {
						if(ini_file.GetInt(PPINISECT_CONFIG, PPINIPARAM_USE_CDB, &(iv = 0)) > 0 && iv != 0)
							SetExtFlag(ECF_USECDB, 1);
						// } @v7.4.8
						// @v7.5.9 {
						if(ini_file.GetInt(PPINISECT_CONFIG, PPINIPARAM_RCPTDLVRLOCASWAREHOUSE, &(iv = 0)) > 0 && iv != 0)
							SetExtFlag(ECF_RCPTDLVRLOCASWAREHOUSE, 1);
						// } @v7.5.9
						// @v8.2.5 {
						if(ini_file.GetInt(PPINISECT_CONFIG, PPINIPARAM_USESJLOGINEVENT, &(iv = 0)) > 0 && iv != 0)
							SetExtFlag(ECF_USESJLOGINEVENT, 1);
						// } @v8.2.5
						// @v8.4.11 {
						if(ini_file.GetInt(PPINISECT_CONFIG, PPINIPARAM_CODEPREFIXEDLIST, &(iv = 0)) > 0 && iv != 0)
							SetExtFlag(ECF_CODEPREFIXEDLIST, 1);
						else
							SetExtFlag(ECF_CODEPREFIXEDLIST, 0);
						// } @v8.4.11
						// @v8.5.12 {
						if(ini_file.GetInt(PPINISECT_CONFIG, PPINIPARAM_DISABLEASYNCADVQUEUE, &(iv = 0)) > 0 && iv != 0)
							SetExtFlag(ECF_DISABLEASYNCADVQUEUE, 1);
						else
							SetExtFlag(ECF_DISABLEASYNCADVQUEUE, 0);
						// } @v8.5.12
						// @v8.5.7 {
						{
							SetExtFlag(ECF_TRACESYNCLOT, 0);
							if(ini_file.GetParam("config", "tracesynclot", temp_buf = 0) > 0) {
								long tsl = temp_buf.ToLong();
								if(tsl > 0)
									SetExtFlag(ECF_TRACESYNCLOT, 1);
							}
						}
						// } @v8.5.7
						// @v8.6.11 {
						if(ini_file.GetInt(PPINISECT_CONFIG, PPINIPARAM_USEGEOTRACKING, &(iv = 0)) > 0 && iv != 0)
							SetExtFlag(ECF_USEGEOTRACKING, 1);
						// } @v8.6.11
						SetExtFlag(ECF_INITONLOGIN, 1);
					}
				}
				//
				// По историческим причинам параметр debug может быть установлен как в зоне [system]
				// так и в зоне [config].
				//
				if(ini_file.GetInt(PPINISECT_SYSTEM, PPINIPARAM_DEBUG, &(iv = 0)) > 0 && iv == 1)
					r_cc.Flags |= CCFLG_DEBUG;
				else if(ini_file.GetInt(PPINISECT_CONFIG, PPINIPARAM_DEBUG, &(iv = 0)) > 0 && iv == 1)
					r_cc.Flags |= CCFLG_DEBUG;
				//
				if(ini_file.GetInt(PPINISECT_CONFIG, PPINIPARAM_DEBUGTRFRERROR, &(iv = 0)) > 0 && iv != 0)
					r_cc.Flags |= CCFLG_DEBUGTRFRERROR;
				if(ini_file.GetInt(PPINISECT_CONFIG, PPINIPARAM_LOGCCHECK, &(iv = 0)) > 0 && iv != 0)
					r_cc.Flags |= CCFLG_LOGCCHECK;
				r_cc._390_DisCalcMethodLockDate = ZERODATE;
				if(ini_file.Get(PPINISECT_SYSTEM, PPINIPARAM_390_DISCALCMETHODLOCKDATE, sv) > 0) {
					dt = strtodate_(sv, DATF_DMY);
					if(checkdate(dt, 0))
						r_cc._390_DisCalcMethodLockDate = dt;
				}
				if(ini_file.Get(PPINISECT_SYSTEM, PPINIPARAM_3918_TDISCALCMETHODLOCKDATE, sv) > 0) {
					dt = strtodate_(sv, DATF_DMY);
					if(checkdate(dt, 0))
						r_cc._3918_TDisCalcMethodLockDate = dt;
				}
				r_cc._405_TDisCalcMethodLockDate = ZERODATE;
				if(ini_file.Get(PPINISECT_SYSTEM, PPINIPARAM_405_TDISCALCMETHODLOCKDATE, sv) > 0) {
					dt = strtodate_(sv, DATF_DMY);
					if(checkdate(dt, 0))
						r_cc._405_TDisCalcMethodLockDate = dt;
				}
				r_cc._418_TDisCalcMethodLockDate = ZERODATE;
				if(ini_file.Get(PPINISECT_SYSTEM, PPINIPARAM_418_TDISCALCMETHODLOCKDATE, sv) > 0) {
					dt = strtodate_(sv, DATF_DMY);
					if(checkdate(dt, 0))
						r_cc._418_TDisCalcMethodLockDate = dt;
				}
				{
					r_tla.SCardPatterns.Id = 1;
					r_tla.SCardPatterns.CopyFrom(0);
					if(ini_file.Get(PPINISECT_CONFIG, PPINIPARAM_SCARD_PATTERNS, sv) && sv.NotEmptyS())
						r_tla.SCardPatterns.CopyFrom(sv);
				}
				// @v7.8.0 {
				{
					r_tla.DL600XMLEntityParam = 0;
					if(ini_file.Get(PPINISECT_CONFIG, PPINIPARAM_DL600XMLENTITY, sv) && sv.NotEmptyS())
						r_tla.DL600XMLEntityParam = sv;
				}
				// } @v7.8.0
				// @v9.4.6 {
				{
					r_tla.DL600XmlCp = cpANSI; // Правильно было бы UTF-8, но для обратной совместимости придется по умолчанию использовать ANSI
					if(ini_file.Get(PPINISECT_CONFIG, PPINIPARAM_DL600XMLCP, sv) && sv.NotEmptyS())
						r_tla.DL600XmlCp.FromStr(sv);
				}
				// } @v9.4.6
				// @v7.9.6 {
				if(ini_file.GetInt(PPINISECT_CONFIG, PPINIPARAM_ADJCPANCCLINETRANS, &(iv = 0)) > 0 && iv != 0)
					r_cc.Flags2 |= CCFLG2_ADJCPANCCLINETRANS;
				else
					r_cc.Flags2 &= ~CCFLG2_ADJCPANCCLINETRANS; // @paranoic
				// } @v7.9.6
				// @v8.1.9 {
				if(ini_file.GetInt(PPINISECT_CONFIG, PPINIPARAM_DONTUSE3TIERGMTX, &(iv = 0)) > 0 && iv != 0)
					r_cc.Flags2 |= CCFLG2_DONTUSE3TIERGMTX;
				else
					r_cc.Flags2 &= ~CCFLG2_DONTUSE3TIERGMTX; // @paranoic
				// } @v8.1.9
				// @v8.6.0 {
				r_cc._InvcMergeTaxCalcAlg2Since = ZERODATE;
				if(ini_file.Get(PPINISECT_CONFIG, PPINIPARAM_INVCMERGETAXCALCALG2SINCE, sv) > 0) {
					dt = strtodate_(sv, DATF_DMY);
					if(checkdate(dt, 0))
						r_cc._InvcMergeTaxCalcAlg2Since = dt;
				}
				// } @v8.6.0
				r_tla.Bac.Load();
			}
			// @v8.2.5 {
			if(CheckExtFlag(ECF_USESJLOGINEVENT))
				LogAction(PPACN_LOGIN, 0, 0, r_lc.SessionID, 1);
			// } @v8.2.5
			if(p_dict->GetCapability() & DbProvider::cSQL) {
				//
				// Для Oracle необходимо, чтобы все регулярные таблицы были созданы ради того,
				// чтобы не возникла ситуация, когда отсутствующая таблица создается внутри транзакции.
				//
				SendObjMessage(DBMSG_DUMMY, 0, 0, 0);
			}
			if(logmode == logmEmptyBaseCreation) {
				PPObject::CreateReservedObjects(PPObject::mrfInitializeDb);
				if(PPCheckDatabaseChain() < 0) {
					PPChainDatabase(0);
				}
			}
			else {
				class PPDbDispatchSession : public PPThread {
				public:
					SLAPI PPDbDispatchSession(long dbPathID, const char * pDbSymb) : PPThread(PPThread::kDbDispatcher, pDbSymb, 0)
					{
						DbPathID = dbPathID;
						DbSymb = pDbSymb;
					}
				private:
					virtual void Run()
					{
						SString msg_buf, temp_buf;
						STimer timer;
						Evnt   stop_event(SLS.GetStopEventName(temp_buf), Evnt::modeOpen);
						char   secret[64];

						PPVersionInfo vi = DS.GetVersionInfo();
						THROW(vi.GetSecret(secret, sizeof(secret)));
						THROW(DS.Login(DbSymb, PPSession::P_JobLogin, secret));
						memzero(secret, sizeof(secret));
						{
							PPLoadText(PPTXT_LOG_DISPTHRCROK, msg_buf);
							msg_buf.Space().CatQStr(DbSymb);
							PPLogMessage(PPFILNAM_INFO_LOG, msg_buf, LOGMSGF_TIME);
						}
						for(int stop = 0; !stop;) {
							LDATETIME dtm = getcurdatetime_();
							dtm.addsec(5);
							timer.Set(dtm, 0);
							uint   h_count = 0;
							HANDLE h_list[32];
							h_list[h_count++] = timer;
							h_list[h_count++] = stop_event;
							uint   r = WaitForMultipleObjects(h_count, h_list, 0, INFINITE);
							if(r == WAIT_OBJECT_0 + 0) { // timer
								DS.DirtyDbCache(DbPathID, 0);
							}
							else if(r == WAIT_OBJECT_0 + 2) { // stop event
								stop = 1; // quit loop
							}
							else if(r == WAIT_FAILED) {
								// error
							}
						}
						CATCH
							{
								//PPTXT_LOG_DISPTHRCROK        "Диспетчерский поток сервера успешно создан"
								//PPTXT_LOG_DISPTHRCRERR       "Ошибка создания диспетчерского потока сервера"
								PPLoadText(PPTXT_LOG_DISPTHRCRERR, msg_buf);
								PPGetMessage(mfError, PPErrCode, 0, 1, temp_buf);
								msg_buf.Space().CatQStr(DbSymb).CatDiv(':', 2).Cat(temp_buf);
								PPLogMessage(PPFILNAM_ERR_LOG, msg_buf, LOGMSGF_TIME);
							}
						ENDCATCH
						DS.Logout();
						memzero(secret, sizeof(secret));
					}
					long   DbPathID;
					SString DbSymb;
				};
				int    is_new_cdb_entry = BIN(CMng.CreateDbEntry(db_path_id) > 0);
				if(CheckExtFlag(ECF_SYSSERVICE)) {
					if(is_new_cdb_entry) {
						PPDbDispatchSession * p_sess = new PPDbDispatchSession(db_path_id, db_symb);
						p_sess->Start(0 /* @v8.5.10 1-->0*/);
					}
				}
				else {
					{
						class PPAdviseEventCollectorSjSession : public PPThread {
						public:
							SLAPI PPAdviseEventCollectorSjSession(const DbLoginBlock & rLB, long cycleMs) :
								PPThread(PPThread::kEventCollector, 0, 0)
							{
								CycleMs = (cycleMs > 0) ? cycleMs : 29989;
								LB = rLB;
								P_Sj = 0;
							}
						private:
							virtual void SLAPI Shutdown()
							{
								//
								// То же, что и PPThread::Shutdown() но без DS.Logout()
								//
								DS.ReleaseThread();
								DBS.ReleaseThread();
								SlThread::Shutdown();
							}
							virtual void Run()
							{
								const long purge_cycle = 3600;

								SString msg_buf, temp_buf;
								//STimer timer;
								LDATETIME dtm;
								LDATETIME last_purge_time = getcurdatetime_();
								TSArray <PPAdviseEvent> temp_list;
								Evnt   stop_event(SLS.GetStopEventName(temp_buf), Evnt::modeOpen);
								BExtQuery * p_q = 0;

								THROW(DS.OpenDictionary2(&LB, PPSession::odfDontInitSync)); // @v9.4.9 PPSession::odfDontInitSync
								THROW_MEM(P_Sj = new SysJournal);

								Since = getcurdatetime_();
								for(int stop = 0; !stop;) {
									//dtm = getcurdatetime_();
									//dtm.addhs(CycleMs / 10);
									//timer.Set(dtm, 0);

									uint   h_count = 0;
									HANDLE h_list[32];
									//h_list[h_count++] = timer;
									h_list[h_count++] = stop_event;
									uint   r = WaitForMultipleObjects(h_count, h_list, 0, CycleMs/*INFINITE*/);
									//if(r == WAIT_OBJECT_0 + 0) { // timer
									if(r == WAIT_TIMEOUT) {
										LDATETIME last_ev_dtm = ZERODATETIME;
										temp_list.clear();
										PPAdviseEventQueue * p_queue = DS.GetAdviseEventQueue(0);
										if(p_queue) {
											{
												dtm = getcurdatetime_();
												if(diffdatetime(dtm, last_purge_time, 3, 0) >= purge_cycle) {
													p_queue->Purge();
													last_purge_time = dtm;
												}
											}
											SysJournalTbl::Key0 k0, k0_;
											k0.Dt = Since.d;
											k0.Tm = Since.t;
											k0_ = k0;
											if(!p_q) {
												if(P_Sj->search(&k0_, spGt)) {
													p_q = new BExtQuery(P_Sj, 0);
													p_q->selectAll().where(P_Sj->Dt >= Since.d);
													p_q->initIteration(0, &k0, spGt);
												}
											}
											else {
												p_q->resetEof();
											}
											if(p_q) {
												int ir;
												while((ir = p_q->nextIteration()) > 0) {
													SysJournalTbl::Rec rec;
													P_Sj->copyBufTo(&rec);
													if(cmp(Since, rec.Dt, rec.Tm) < 0) {
														PPAdviseEvent ev;
														ev = rec;
														temp_list.insert(&ev);
														last_ev_dtm.Set(rec.Dt, rec.Tm);
													}
												}
												p_queue->Push(temp_list);
												if(!!last_ev_dtm)
													Since = last_ev_dtm;
												if(!BTROKORNFOUND) {
													PPSetErrorDB();
													PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR|LOGMSGF_DBINFO|LOGMSGF_TIME|LOGMSGF_USER);
													PPLogMessage(PPFILNAM_INFO_LOG, PPSTR_TEXT, PPTXT_ASYNCEVQUEUESJFAULT, LOGMSGF_DBINFO|LOGMSGF_TIME|LOGMSGF_USER);
													ZDELETE(p_q);
												}
											}
										}
									}
									else if(r == WAIT_OBJECT_0 + 0) { // stop event
										stop = 1; // quit loop
									}
									else if(r == WAIT_FAILED) {
										// error
									}
								}
								CATCH
									{
									}
								ENDCATCH
								delete p_q;
								ZDELETE(P_Sj);
								DBS.CloseDictionary();
							}
							long   CycleMs;
							LDATETIME Since;
							DbLoginBlock LB;
							SysJournal * P_Sj;
						};
#if USE_ADVEVQUEUE
						int    cycle_ms = 0;
						ini_file.GetInt(PPINISECT_CONFIG, PPINIPARAM_ADVISEEVENTCOLLECTORPERIOD, &cycle_ms);
						if(cycle_ms <= 0 || cycle_ms > 600000)
							cycle_ms = 5113;
						PPAdviseEventCollectorSjSession * p_evc = new PPAdviseEventCollectorSjSession(blk, cycle_ms);
						p_evc->Start(0);
						r_tla.P_AeqThrd = p_evc; // @v8.6.7
#endif // USE_ADVEVQUEUE
					}
					int    r = 0;
					if(ini_file.GetInt(PPINISECT_CONFIG, PPINIPARAM_3TIER, &r) > 0 && r) {
						if(r_tla.SrvSess.Connect(0, -1)) {
							r_cc.Flags |= CCFLG_3TIER;
							struct InfProcWrapper {
								static int Proc(const char * pMsg, void * pParam)
								{
									PPWaitMsg(pMsg);
									return 1;
								}
							};
							r_tla.SrvSess.Login(db_symb, pUserName, pPassword);
							r_tla.SrvSess.SetInformerProc(InfProcWrapper::Proc, 0);
						}
					}
				}
			}
			// @v8.1.12 {
			{
				char   domain_user[128];
				DWORD  duser_len = sizeof(domain_user);
				memzero(domain_user, sizeof(domain_user));
				if(!::GetUserName(domain_user, &duser_len)) // @unicodeproblem
					STRNSCPY(domain_user, "!undefined");
				PPLoadText(PPTXT_LOGININFO, temp_buf = 0);
				msg_buf.Printf(temp_buf, domain_user);
				PPLogMessage(PPFILNAM_INFO_LOG, msg_buf, LOGMSGF_TIME|LOGMSGF_DBINFO|LOGMSGF_USER|LOGMSGF_COMP);
			}
			r_tla.State |= PPThreadLocalArea::stAuth; // @v8.6.11
			ufp.Commit();
			// } @v8.1.12
		}
	}
	CATCH
		ok = 0;
		ZDELETE(BillObj);
		ZDELETE(PPRef);
		DBS.CloseDictionary();
	ENDCATCH
	memzero(pw, sizeof(pw));
	return ok;
}

int SLAPI PPSession::Register()
{
	int    ok = 1;
	RegSessData data;
	MEMSZERO(data);
	data.Uuid = SLS.GetSessUuid();
	data.InitTime = getcurdatetime_();
	data.Ver = DS.GetVersion();
	SString uuid_buf;
	data.Uuid.ToStr(S_GUID::fmtIDL, uuid_buf);
	WinRegKey reg_key(HKEY_CURRENT_USER, PPRegKeys::Sessions, 0);
	return reg_key.PutBinary(uuid_buf, &data, sizeof(data)-offsetof(RegSessData, ReserveStart));
}

int SLAPI PPSession::GetRegisteredSess(const S_GUID & rUuid, PPSession::RegSessData * pData)
{
	int    ok = -1;
	RegSessData data;
	MEMSZERO(data);
	SString uuid_buf;
	rUuid.ToStr(S_GUID::fmtIDL, uuid_buf);
	WinRegKey reg_key(HKEY_CURRENT_USER, PPRegKeys::Sessions, 1);
	if(reg_key.GetBinary(uuid_buf, &data, sizeof(data)-offsetof(RegSessData, ReserveStart)) > 0) {
		data.Uuid = rUuid;
		ASSIGN_PTR(pData, data);
		ok = 1;
	}
	return ok;
}

int SLAPI PPSession::Unregister()
{
	int    ok = 1;
	WinRegKey reg_key;
	SString uuid_buf;
	SLS.GetSessUuid().ToStr(S_GUID::fmtIDL, uuid_buf);
	return reg_key.DeleteValue(HKEY_CURRENT_USER, PPRegKeys::Sessions, uuid_buf);
}

int SLAPI PPSession::SetDbCacheDeferredState(long dbPathID, int set)
{
	return CMng.SetDeferredState(dbPathID, set);
}

int FASTCALL PPSession::IsDbCacheDeferredState(long dbPathID)
{
	return CMng.IsDeferredState(dbPathID);
}

int SLAPI PPSession::DirtyDbCache(long dbPathID, /*int64 * pAdvQueueMarker*/PPAdviseEventQueue::Client * pCli)
{
	int    ok = 1;
	if(dbPathID && DBS.IsConsistent()) {
		//
		// Следующие три статических объекта защищены общей критической секцией
		//
		static PPIDArray * p_ev_list = 0;
		static PPIDArray * p_comm_dirty_cache_ev_list = 0;
		static PPIDArray * p_addendum_ev_list = 0;
		//
		if(!CMng.IsDeferredState(dbPathID)) {
			ENTER_CRITICAL_SECTION
			{
				//
				// Единоразовая инициализация глобальных списков
				//
				if(!p_ev_list) {
					p_comm_dirty_cache_ev_list = new PPIDArray;
					p_comm_dirty_cache_ev_list->addzlist(
						PPACN_OBJADD,
						PPACN_OBJUPD,
						PPACN_OBJRMV,
						PPACN_OBJUNIFY,
						PPACN_GOODSQUOTUPD,
						PPACN_SCARDDISUPD,
						PPACN_GOODSNODISRMVD,
						PPACN_GOODSVATGRPCHD,
						PPACN_GOODSMTXUPD,
						PPACN_UPDBILL,
						PPACN_RMVBILL,
						PPACN_OBJEXTMEMOUPD,
						PPACN_UPDBILLEXT,
						PPACN_MTXGOODSADD,
						PPACN_QUOTUPD2,      // @v7.3.3
						PPACN_UPDBILLWLABEL, // @v8.1.8
						PPACN_BILLWROFF,     // @v8.8.3
						PPACN_BILLWROFFUNDO, // @v8.8.3
						0L);
					p_comm_dirty_cache_ev_list->sort();
					p_addendum_ev_list = new PPIDArray;
					p_addendum_ev_list->addzlist(
						PPACN_OBJTAGUPD,
						PPACN_OBJTAGRMV,
						PPACN_OBJTAGADD, // @v8.0.5
						0L);
					p_addendum_ev_list->sort();
					p_ev_list = new PPIDArray;
					p_ev_list->addUnique(p_comm_dirty_cache_ev_list);
					p_ev_list->addUnique(p_addendum_ev_list);
					p_ev_list->sort();
				}
				assert(p_ev_list && p_addendum_ev_list && p_comm_dirty_cache_ev_list);
			}
			const uint64 tm_start = GetProfileTime();
			const LDATETIME cur = getcurdatetime_();
			LDATETIME last_cache_update = CMng.GetLastUpdate(dbPathID);

			uint   dirty_call_count = 0;
			PPAdviseList adv_list;
			struct SjEntry {
				int16  Action;
				int16  ObjType;
				PPID   ObjID;
				long   Extra;
			};
			SArray list(sizeof(SjEntry), 64, O_ARRAY);

			TSArray <PPAdviseEvent> evq_list;
			PPAdviseEventQueue * p_queue = (pCli && __UseAdvEvQueue && !CheckExtFlag(ECF_DISABLEASYNCADVQUEUE)) ? CMng.GetAdviseEventQueue(dbPathID) : 0;
			if(pCli && p_queue)
				pCli->Register(dbPathID, p_queue);
			const uint evqc = (p_queue && p_queue->Get(pCli->GetMarker(), evq_list) > 0) ? evq_list.getCount() : 0;
			if(evqc) {
				assert(pCli);
				pCli->Marker = evq_list.at(evqc-1).Ident;
			}
			if(!p_queue) {
				if(last_cache_update.d) {
					SysJournal * p_sj = GetTLA().P_SysJ;
					if(p_sj) {
						SysJournalTbl::Key0 k;
						k.Dt = last_cache_update.d;
						k.Tm = last_cache_update.t;
						BExtQuery q(p_sj, 0);
						q.selectAll().where(p_sj->Dt >= last_cache_update.d);
						for(q.initIteration(0, &k, spGe); q.nextIteration() > 0;) {
							if(cmp(last_cache_update, p_sj->data.Dt, p_sj->data.Tm) < 0 && p_ev_list->bsearch(p_sj->data.Action)) {
								SjEntry entry;
								entry.Action  = (int16)p_sj->data.Action;
								entry.ObjType = (int16)p_sj->data.ObjType;
								entry.ObjID   = p_sj->data.ObjID;
								entry.Extra   = p_sj->data.Extra;
								list.insert(&entry);
							}
						}
					}
				}
			}
			else if(evqc) {
				for(uint i = 0; i < evqc; i++) {
					const PPAdviseEvent & r_ev = evq_list.at(i);
					SjEntry entry;
					entry.Action  = (int16)r_ev.Action;
					entry.ObjType = (int16)r_ev.Oid.Obj;
					entry.ObjID   = r_ev.Oid.Id;
					entry.Extra   = r_ev.SjExtra;
					list.insert(&entry);
				}
			}
			const uint ev_count = list.getCount();
			if(ev_count) {
				SjEntry * p_item;
				uint   i = 0;
				for(i = 0; list.enumItems(&i, (void **)&p_item);) {
					if(p_item->ObjType && p_item->ObjID && p_comm_dirty_cache_ev_list->bsearch(p_item->Action)) {
						ObjCache * p_cache = GetDbLocalObjCache(p_item->ObjType);
						if(p_cache) {
							p_cache->Dirty(p_item->ObjID);
							dirty_call_count++;
						}
					}
				}
				if(GetAdviseList(PPAdviseBlock::evDirtyCacheBySysJ, 0, adv_list) > 0) {
					PPNotifyEvent ev;
					PPAdviseBlock adv_blk;
					for(uint j = 0; adv_list.Enum(&j, &adv_blk);) {
						if(adv_blk.Proc) {
							for(i = 0; list.enumItems(&i, (void **)&p_item);) {
								if((!adv_blk.Action || p_item->Action == adv_blk.Action) && (!adv_blk.ObjType || adv_blk.ObjType == p_item->ObjType)) {
									ev.Action  = p_item->Action;
									ev.ObjType = p_item->ObjType;
									ev.ObjID   = p_item->ObjID;
									ev.ExtInt_ = p_item->Extra;
									ev.ExtDtm.SetZero();
									ev.ExtStr  = 0;
									adv_blk.Proc(PPAdviseBlock::evDirtyCacheBySysJ, &ev, adv_blk.ProcExtPtr);
								}
							}
							adv_blk.Proc(PPAdviseBlock::evDirtyCacheBySysJ, &ev.SetFinishTag(), adv_blk.ProcExtPtr); // finalize
						}
					}
				}
			}
			uint64 tm_finish = GetProfileTime();
			{
				SString msg_buf;
				(msg_buf = "Cache was updated").CatDiv(':', 2).CatEq("events", (long)ev_count).Space().
					CatEq("calls", (long)dirty_call_count).Space().CatEq("time", (int64)(tm_finish-tm_start));
				SetThreadNotification(PPSession::stntMessage, msg_buf);
			}
			CMng.SetLastUpdate(dbPathID, cur);
			LEAVE_CRITICAL_SECTION
		}
		else {
			SetThreadNotification(PPSession::stntMessage, "Cache is in deferred state");
		}
	}
	return ok;
}

// Prototype
int SLAPI CreateBackupCopy(const char *, int);

int SLAPI PPThreadLocalArea::OnLogout()
{
	State &= ~stAuth; // @v8.6.11
	SrvViewList.freeAll();
	ZDELETE(P_WObj);  // @v8.3.6
	ZDELETE(P_WbObj); // @v8.3.6
	ZDELETE(P_TodoObj); // @v8.5.11
	ZDELETE(P_Ref);
	ZDELETE(P_BObj);
	ZDELETE(P_GtaJ); // @v7.4.10
	if(P_SysJ) {
		if(DS.CheckExtFlag(ECF_USESJLOGINEVENT)) // @v8.2.5
			P_SysJ->LogEvent(PPACN_LOGOUT, 0, 0, 0, 1);
		ZDELETE(P_SysJ);
	}
	ZDELETE(P_ObjSync);
	if(Lc.SessionID) // @v9.1.12 @fix
		Sync.LogoutUser(Lc.SessionID);
	if(SrvSess.GetState() & PPJobSrvClient::stConnected) {
		SrvSess.Logout();
		SrvSess.Disconnect();
	}
	return 1;
}

int SLAPI PPSession::Logout()
{
	PPThreadLocalArea & r_tla = GetTLA();
	if(r_tla.State & PPThreadLocalArea::stAuth) { // @v9.2.1
		const SString active_user = r_tla.UserName;
		SString pn;
		SetPrivateBasket(0, 1);
		//
		// Удаляем временный каталог для отчетных данных
		//
		GetPath(PPPATH_TEMP, pn);
		pn.SetLastSlash().CatLongZ(r_tla.PrnDirId, 8);
		PPRemoveFilesByExt(pn, "*");
		::RemoveDirectory(pn); // @unicodeproblem
		//
		if(CCfg().Flags & CCFLG_DEBUG)
			CMng.LogCacheStat();
		r_tla.OnLogout();
		r_tla.Prf.FlashUserProfileAccumEntries(); // @v8.1.3
		r_tla.UfpSess.Commit(); // @v8.0.6
		DBS.CloseDictionary();
		Btrieve::Reset(0);
		if(!CheckExtFlag(ECF_SYSSERVICE) && active_user.NotEmpty())
			CreateBackupCopy(active_user, 0);
		GetSync().Release(); // @todo ReleaseSync()
		GPrf.Output(0, 0); // @v8.0.3 PROFILE_REPORT(0)-->GPrf.Output(0, 0)
		// @v8.6.7 {
		// @todo Аккуратно остановить поток PPAdviseEventCollectorSjSession
		// } @v8.6.7
	}
	return 1;
}

const PPVersionInfo & SLAPI PPSession::GetVersionInfo() const
{
	return Ver;
}

SVerT SLAPI PPSession::GetVersion() const
{
	PPVersionInfo _ver = GetVersionInfo();
	return _ver.GetVersion(0);
}

PPSync & SLAPI PPSession::GetSync()
{
	return GetTLA().Sync;
}

int SLAPI PPSession::SetLocation(PPID locID)
{
	LocationTbl::Rec rec;
	if(SearchObject(PPOBJ_LOCATION, locID, &rec) > 0) {
		GetTLA().Lc.Location = locID;
		SETFLAG(GetTLA().Lc.State, CFGST_WAREHOUSE, rec.Type == LOCTYP_WAREHOUSE);
		return 1;
	}
	else
		GetTLA().Lc.Location = 0;
	StatusWinChange(); // @UI
	return 0;
}

int SLAPI PPSession::SetOperDate(LDATE date)
{
	int    d;
	GetTLA().Lc.OperDate = date;
	decodedate(&d, &DefaultMonth, &DefaultYear, &date);
	return 1;
}

int SLAPI PPSession::SetCurCashNodeID(PPID cashNodeID)
{
	GetTLA().Lc.Cash = cashNodeID;
	return 1;
}

int SLAPI PPSession::SetDefBillCashID(PPID billCashID)
{
	GetTLA().Lc.DefBillCashID = billCashID;
	return 1;
}

int SLAPI PPSession::SetMenu(short menuId)
{
	GetTLA().Lc.Menu = menuId;
	return 1;
}

int SLAPI PPSession::SetDemoMode(int s)
{
	int    c = BIN(GetTLA().Lc.State & CFGST_DEMOMODE);
	SETFLAG(GetTLA().Lc.State, CFGST_DEMOMODE, s);
	return c;
}

long SLAPI PPSession::SetLCfgFlags(long f)
{
	long   c = GetTLA().Lc.Flags;
	GetTLA().Lc.Flags = f;
	return c;
}

short SLAPI PPSession::SetRealizeOrder(short s)
{
	short  c = GetTLA().Lc.RealizeOrder;
	GetTLA().Lc.RealizeOrder = s;
	return c;
}

#if 0 // Вариант функции до v8.6.1 {

int SLAPI PPSession::SetMainOrgID(PPID id, int enforce)
{
	PPThreadLocalArea & tla = GetTLA();
	PPCommConfig & cc = tla.Cc;
	if(!enforce && (id == cc.MainOrgID))
		return 1;
	else {
		PPObjStaffList stlobj;
		if(stlobj.PsnObj.P_Tbl->IsBelongToKind(id, PPPRK_MAIN) > 0) {
			PersonTbl::Rec psn_rec;
			cc.MainOrgID = id;
			cc.MainOrgDirector = 0;
			cc.MainOrgAccountant = 0;
			if(stlobj.PsnObj.Search(id, &psn_rec) > 0) {
				PersonPostTbl::Rec post_rec;
				stlobj.GetFixedPostOnDate(id, PPFIXSTF_DIRECTOR, ZERODATE, &post_rec);
				cc.MainOrgDirector = post_rec.PersonID;
				stlobj.GetFixedPostOnDate(id, PPFIXSTF_ACCOUNTANT, ZERODATE, &post_rec);
				cc.MainOrgAccountant = post_rec.PersonID;
				tla.MainOrgName.Id = id;
				tla.MainOrgName.CopyFrom(psn_rec.Name);
			}
			else {
				tla.MainOrgName.Id = 0;
				tla.MainOrgName.CopyFrom(0);
			}
			if(!cc.MainOrgDirector || !cc.MainOrgAccountant) {
				PPCommConfig temp_cfg_rec;
				GetCommConfig(&temp_cfg_rec);
				SETIFZ(cc.MainOrgDirector, temp_cfg_rec.MainOrgDirector);
				SETIFZ(cc.MainOrgAccountant, temp_cfg_rec.MainOrgAccountant);
			}
			return 1;
		}
		else
			return PPSetError(PPERR_NOMAINORGID);
	}
}

#endif // } 0 Вариант функции до v8.6.1

int SLAPI PPSession::SetMainOrgID(PPID id, int enforce)
{
	int    ok = 1;
	PPThreadLocalArea & r_tla = GetTLA();
	PPCommConfig & cc = r_tla.Cc;
	if(enforce || (id != cc.MainOrgID)) {
		r_tla.InitMainOrgData(1 /* reset global main org attributes */);
		PPObjPerson psn_obj;
		if(psn_obj.P_Tbl->IsBelongToKind(id, PPPRK_MAIN) > 0) {
			PersonTbl::Rec psn_rec;
			cc.MainOrgID = id;
			if(psn_obj.Search(id, &psn_rec) > 0) {
				r_tla.MainOrgName.Id = id;
				r_tla.MainOrgName.CopyFrom(psn_rec.Name);
			}
			else {
				r_tla.MainOrgName.Id = 0;
				r_tla.MainOrgName.CopyFrom(0);
			}
		}
		else
			ok = PPSetError(PPERR_NOMAINORGID);
	}
	return ok;
}

int SLAPI PPSession::RestCheckingStatus(int s)
{
	long * p_flags = &GetTLA().Lc.Flags;
	int    c = BIN((*p_flags) & CFGFLG_CHECKTURNREST);
	if(s > 0)
		(*p_flags) |= CFGFLG_CHECKTURNREST;
	else if(s == 0)
		(*p_flags) &= ~CFGFLG_CHECKTURNREST;
	return c;
}

int FASTCALL PPSession::CheckExtFlag(long f)
{
	int    result = 0;
	ExtFlagsLck.Lock();
	result = BIN(ExtFlags_ & f);
	ExtFlagsLck.Unlock();
	return result;
}

long SLAPI PPSession::SetExtFlag(long f, int set)
{
	long   prev = 0;
	ExtFlagsLck.Lock();
	prev = ExtFlags_;
	if(set > 0) {
		ExtFlags_ |= f;
	}
	else if(set == 0) {
		ExtFlags_ &= ~f;
	}
	ExtFlagsLck.Unlock();
	return prev;
}

int SLAPI PPSession::SetStateFlag(long f, int set)
{
	SETFLAG(GetTLA().Lc.State, f, set);
	return 1;
}

int SLAPI PPSession::CheckStateFlag(long f) const
{
	return BIN(GetConstTLA().Lc.State & f);
}

int SLAPI PPSession::SetPath(PPID pathID, const char * pBuf, short flags, int replace)
{
	return GetTLA().Paths.SetPath(pathID, pBuf, flags, replace);
}

PPDriveMapping::PPDriveMapping() : StringSet(";")
{
}

int PPDriveMapping::Load(PPIniFile * pIniFile)
{
	if(pIniFile) {
		clear();
		setDelim(";");
		pIniFile->GetEntryList(PPINISECT_DRIVEMAPPING, this, 1);
		return 1;
	}
	else
		return 0;
}

int PPDriveMapping::Get(int drive, SString & rMapping) const
{
	SString entry, drv, map;
	for(uint i = 0; get(&i, entry) > 0;)
		if(entry.Divide('=', drv, map) > 0 && toupper(drv[0]) == toupper(drive)) {
			rMapping = map;
			return 1;
		}
	return 0;
}

int PPDriveMapping::ConvertPathToUnc(SString & rPath) const
{
	if(isalpha(rPath[0]) && rPath[1] == ':') {
		SString unc;
		if(Get(rPath[0], unc) > 0) {
			if(unc.Last() != '\\' && unc.Last() != '/')
				unc.CatChar('\\');
			rPath.ShiftLeft(2);
			rPath.ShiftLeftChr('\\');
			rPath.ShiftLeftChr('/');
			unc.Cat(rPath);
			rPath = unc;
			return 1;
		}
	}
	return -1;
}

int SLAPI PPSession::GetLocalPath(SString & rBuf)
{
	SString comp_name;
	if(!SGetComputerName(comp_name))
		comp_name = "COMMON";
	rBuf = 0;
	GetPath(PPPATH_BIN, rBuf);
	rBuf.SetLastSlash().Cat("LOCAL").SetLastSlash().Cat(comp_name);
	return 1;
}

int SLAPI PPSession::LoadDriveMapping(PPIniFile * pIniFile)
	{ return DrvMap.Load(pIniFile); }
int SLAPI PPSession::GetDriveMapping(int drive, SString & rMapping) const
	{ return DrvMap.Get(drive, rMapping); }
int SLAPI PPSession::ConvertPathToUnc(SString & rPath) const
	{ return DrvMap.ConvertPathToUnc(rPath); }

int SLAPI PPSession::GetPath(PPID pathID, SString & rBuf)
{
	int    ok = 1;
	switch(pathID) {
		case PPPATH_BIN:
			PROFILE_START
			rBuf = BinPath; // @v8.1.6
			// @v8.1.6 getExecPath(rBuf).SetLastSlash();
			PROFILE_END
			break;
		case PPPATH_SYSROOT:
			{
				SString temp_str;
				GetPath(PPPATH_BIN, temp_str); // @recursion
				rBuf = temp_str.RmvLastSlash();
				int    last = rBuf.Last();
				while(last && last != '\\' && last != '/')
					last = rBuf.TrimRight().Last();
				if(!last)
					rBuf = temp_str;
			}
			break;
		case PPPATH_DOC:
			GetPath(PPPATH_SYSROOT, rBuf); // @recursion
			rBuf.SetLastSlash().Cat("DOC");
			break;
		case PPPATH_DD:
			GetPath(PPPATH_SYSROOT, rBuf); // @recursion
			rBuf.SetLastSlash().CatCharN('D', 2);
			break;
		case PPPATH_WTM:
			GetPath(PPPATH_DD, rBuf); // @recursion
			rBuf.SetLastSlash().Cat("WTM");
			break;
		case PPPATH_LOCAL:
			GetLocalPath(rBuf);
			break;
		default:
			ok = GetTLA().Paths.Get(0, 0, pathID, rBuf);
			break;
	}
	if(ok > 0)
		ConvertPathToUnc(rBuf);
	else
		rBuf = 0;
	return ok;
}

/*struct PPRFile {
	PPID   ID;
	PPID   PathID;
	PPID   SrcPathID;
	long   Flags;
	SString Name;
	SString Descr;
};

// PPRFILE_XXX PP_RCDECLRFILE { "Symb\0", "file_name", PPPATH_XXX(1), PPPATH_XXX(2), flags, "Descript\0" }
*/


#define PPRFILE_SIGN 0xfeedbac5U

SLAPI PPRFile::PPRFile()
{
	Clear();
}

SLAPI PPRFile::~PPRFile()
{
	Sign = 0;
}

int SLAPI PPRFile::IsConsistent() const
{
	return BIN(Sign == PPRFILE_SIGN);
}

PPRFile & SLAPI PPRFile::Clear()
{
	Sign = PPRFILE_SIGN;
	ID = 0;
	PathID = 0;
	SrcPathID = 0;
	Flags = 0;
	Name = 0;
	Descr = 0;
	return *this;
}

static int FASTCALL Helper_GetRFileInfo(PPID fileId, PPRFile & rInfo)
{
	int    ok = -1;
	TVRez * p_rez = P_SlRez;
	if(p_rez) {
		long   offs = 0, sz = 0;
		SString temp_buf;
		if(p_rez->findResource(fileId, PP_RCDECLRFILE, &offs, &sz) > 0) {
			rInfo.ID = fileId;
			p_rez->getString(temp_buf, 2); // symbol
			p_rez->getString(rInfo.Name, 2);    // name (Char encoding)
			rInfo.PathID    = (long)p_rez->getUINT();
			rInfo.SrcPathID = (long)p_rez->getUINT();
			rInfo.Flags     = (long)p_rez->getUINT();
			p_rez->getString(rInfo.Descr, 2);   // description (OEM encoding)
			ok = 1;
		}
	}
	else
		ok = 0;
	return ok;
}

long SLAPI PPSession::GetMaxLogFileSize() const
{
	return MaxLogFileSize;
}

int SLAPI PPSession::GetRFileInfo(PPID fileId, PPRFile & rInfo)
{
	return Helper_GetRFileInfo(fileId, rInfo);
}

SEnumImp * PPSession::EnumRFileInfo()
{
	class PPRFileEnum : public SEnumImp {
	public:
		PPRFileEnum()
		{
			P_Rez = P_SlRez;
			DwPos = 0;
		}
		virtual int Next(void * pRec)
		{
			int    ok = 0;
			PPRFile * p_info = (PPRFile *)pRec;
			if(P_Rez && p_info && p_info->IsConsistent()) {
				uint   file_id = 0;
				if(P_Rez->enumResources(PP_RCDECLRFILE, &file_id, &DwPos) > 0)
					ok = Helper_GetRFileInfo((long)file_id, *p_info);
			}
			return ok;
		}
	protected:
		TVRez * P_Rez;
		ulong  DwPos;
	};
	return new PPRFileEnum;
}

int SLAPI PPSession::SetDbLocalObjCache(ObjCache * pCache)
	{ return CMng.AddCache(DBS.GetDbPathID(), pCache); }

ObjCache * FASTCALL PPSession::GetDbLocalObjCache(PPID objType)
{
	const long db_path_id = DBS.GetDbPathID();
	ObjCache * p_cache = CMng.GetCache(db_path_id, objType);
	if(!p_cache) {
		if(oneof2(objType, PPOBJ_GOODSGROUP, PPOBJ_BRAND))
			p_cache = CMng.GetCache(db_path_id, PPOBJ_GOODS);
	}
	return p_cache;
}

PPAdviseEventQueue * SLAPI PPSession::GetAdviseEventQueue(PPAdviseEventQueue::Client * pCli)
{
	PPAdviseEventQueue * p_queue = 0;
	if(__UseAdvEvQueue && !CheckExtFlag(ECF_DISABLEASYNCADVQUEUE) && DBS.IsConsistent()) {
		const long db_path_id = DBS.GetDbPathID();
		p_queue = CMng.GetAdviseEventQueue(db_path_id);
        if(pCli && p_queue)
			pCli->Register(db_path_id, p_queue);
	}
	return p_queue;
}

void SLAPI PPSaveErrContext()
	{ DS.GetTLA().PushErrContext(); }
void SLAPI PPRestoreErrContext()
	{ DS.GetTLA().PopErrContext(); }

DlContext * SLAPI PPSession::Helper_GetInterfaceContext(DlContext ** ppCtx, uint fileId, int crit)
{
	int    is_allocated = 0;
	SCriticalSection::Data * p_csd = 0;
	if(*ppCtx == 0) {
		if(crit) {
			p_csd = new SCriticalSection::Data;
			p_csd->Enter();
		}
		if(*ppCtx == 0) {
			SString file_name;
			PPGetFilePath(PPPATH_BIN, fileId, file_name);
			if(file_name.NotEmptyS()) {
				THROW_SL(fileExists(file_name));
				THROW_MEM(*ppCtx = new DlContext);
				is_allocated = 1;
				THROW((*ppCtx)->Init(file_name));
			}
		}
	}
	CATCH
		if(is_allocated)
			ZDELETE(*ppCtx);
	ENDCATCH
	if(p_csd)
		p_csd->Leave();
	return *ppCtx;
}

DlContext * SLAPI PPSession::GetInterfaceContext(int ctxType)
{
	DlContext * p_ctx = 0;
	if(oneof2(ctxType, ctxtExportData, ctxtInterface)) {
		PPThreadLocalArea & r_tla = DS.GetTLA();
		if(ctxType == ctxtExportData)
			p_ctx = Helper_GetInterfaceContext(&r_tla.P_ExpCtx, PPFILNAM_DL600EXP, 0);
		else if(ctxType == ctxtInterface)
			p_ctx = Helper_GetInterfaceContext(&r_tla.P_IfcCtx, PPFILNAM_DL600IFC, 0);
	}
	else if(ctxType == ctxDatabase)
		p_ctx = Helper_GetInterfaceContext(&P_DbCtx, PPFILNAM_DL600DBS, 1);
	return p_ctx;
}

SLAPI PPAdviseList::PPAdviseList() : SArray(sizeof(PPAdviseBlock), 16, O_ARRAY)
{
	LastCookie = 0;
}

uint PPAdviseList::GetCount()
{
	uint   c = 0;
	RwL.ReadLock();
	c = getCount();
	RwL.Unlock();
	return c;
}

int FASTCALL PPAdviseList::Enum(uint * pI, PPAdviseBlock * pBlk) const
{
	int    ok = 0;
	uint   i = pI ? *pI : 0;
	if(i < getCount()) {
		ASSIGN_PTR(pBlk, *(PPAdviseBlock *)at(i));
		++i;
		ok = 1;
	}
	ASSIGN_PTR(pI, i);
	return ok;
}

int PPAdviseList::CreateList(int kind, ThreadID tId, long dbPathID, PPID objType, PPAdviseList & rList)
{
	int    ok = -1;
	rList.clear();
	RwL.ReadLock();
	const uint c = getCount();
	for(uint i = 0; i < c; i++) {
		const PPAdviseBlock * p_blk = (PPAdviseBlock *)at(i);
		if(p_blk->Kind == kind && (!objType || p_blk->ObjType == objType) &&
			(!p_blk->TId || p_blk->TId == tId) && (!p_blk->DbPathID || p_blk->DbPathID == dbPathID)) {
			rList.insert(p_blk);
			ok = 1;
		}
	}
	RwL.Unlock();
	return ok;
}

int SLAPI PPAdviseList::Advise(long * pCookie, const PPAdviseBlock * pBlk)
{
	int    ok = -1;
	RwL.WriteLock();
	if(pBlk) {
		long   cookie = ++LastCookie;
		PPAdviseBlock blk = *pBlk;
		blk.Cookie = cookie;
		ok = insert(&blk) ? 1 : PPSetErrorSLib();
		if(ok)
			ASSIGN_PTR(pCookie, cookie);
	}
	else if(pCookie) {
		uint pos = 0;
		if(lsearch(pCookie, &pos, CMPF_LONG)) {
			atFree(pos);
			ok = 1;
		}
	}
	RwL.Unlock();
	return ok;
}

int SLAPI PPSession::Advise(long * pCookie, const PPAdviseBlock * pBlk)
	{ return AdvList.Advise(pCookie, pBlk); }
int SLAPI PPSession::Unadvise(long cookie)
	{ return AdvList.Advise(&cookie, 0); }
int SLAPI PPSession::GetAdviseList(int kind, PPID objType, PPAdviseList & rList)
	{ return AdvList.CreateList(kind, GetConstTLA().GetThreadID(), DBS.GetDbPathID(), objType, rList); }

void SLAPI PPSession::ProcessIdle()
{
	PPThreadLocalArea & r_tla = GetTLA();
	const uint c = r_tla.IdleCmdList.getCount();
	for(uint i = 0; i < c; i++) {
		PPThreadLocalArea::IdleCommand * p_cmd = r_tla.IdleCmdList.at(i);
		if(p_cmd) {
			LDATETIME prev;
			if(p_cmd->Check(&prev))
				p_cmd->Run(prev);
		}
	}
}

PPSession::ObjIdentBlock::ObjIdentBlock() : SymbList(256, 1)
{
	PPIDArray obj_type_list;
	PPGetObjTypeList(&obj_type_list, gotlfExcludeDyn);
	SString name_buf;
	for(uint i = 0; i < obj_type_list.getCount(); i++) {
		const PPID obj_type = obj_type_list.get(i);
		if(PPLoadString(PPSTR_OBJNAMES, (uint)obj_type, name_buf))
			TitleList.Add(obj_type, name_buf);
	}
	{
		SString name_list;
		PPLoadText(PPTXT_CFGNAMES, name_list);
		StringSet ss(';', name_list);
		for(uint i = 0, j = 1; ss.get(&i, name_buf) > 0; j++)
			TitleList.Add(PPOBJ_FIRST_CFG_OBJ + j, name_buf);
	}
	{
		SymbList.Add("UNIT",           PPOBJ_UNIT);
		SymbList.Add("QUOTKIND",       PPOBJ_QUOTKIND); // @v7.2.2
		SymbList.Add("LOCATION",       PPOBJ_LOCATION); // @v7.2.2
		SymbList.Add("GOODS",          PPOBJ_GOODS);
		SymbList.Add("GOODSGROUP",     PPOBJ_GOODSGROUP);
		SymbList.Add("BRAND",          PPOBJ_BRAND);
		SymbList.Add("GOODSTYPE",      PPOBJ_GOODSTYPE);
		SymbList.Add("GOODSCLASS",     PPOBJ_GOODSCLASS);
		SymbList.Add("GOODSARCODE",    PPOBJ_GOODSARCODE);
		SymbList.Add("PERSON",         PPOBJ_PERSON);
		SymbList.Add("PERSONKIND",     PPOBJ_PRSNKIND);
		SymbList.Add("PERSONSTATUS",   PPOBJ_PRSNSTATUS);
		SymbList.Add("PERSONCATEGORY", PPOBJ_PRSNCATEGORY);
		SymbList.Add("GLOBALUSER",     PPOBJ_GLOBALUSERACC);
		SymbList.Add("DL600",          PPOBJ_DL600DATA);
		SymbList.Add("WORLD",          PPOBJ_WORLD);
		SymbList.Add("CITY",           PPOBJ_WORLD | (WORLDOBJ_CITY << 16));
		SymbList.Add("COUNTRY",        PPOBJ_WORLD | (WORLDOBJ_COUNTRY << 16));
		SymbList.Add("QUOT",           PPOBJ_QUOT2);
		SymbList.Add("CURRENCY",       PPOBJ_CURRENCY);
		SymbList.Add("CURRATETYPE",    PPOBJ_CURRATETYPE);
		SymbList.Add("SPECSERIES",     PPOBJ_SPECSERIES);   // @v7.3.2
		SymbList.Add("SCARD",          PPOBJ_SCARD);
		SymbList.Add("POSNODE",        PPOBJ_CASHNODE);     // @v7.3.7
		SymbList.Add("CURRATEIDENT",   PPOBJ_CURRATEIDENT); // @v7.4.6
		SymbList.Add("UHTTSCARDOP",    PPOBJ_UHTTSCARDOP);
		SymbList.Add("LOT",            PPOBJ_LOT);          // @v7.4.11
		SymbList.Add("BILL",           PPOBJ_BILL);         // @v7.5.5
		SymbList.Add("UHTTSTORE",      PPOBJ_UHTTSTORE);    // @v7.6.1 @Muxa
		SymbList.Add("OPRKIND",        PPOBJ_OPRKIND);      // @v7.6.6 @Muxa
		SymbList.Add("WORKBOOK",       PPOBJ_WORKBOOK);
		SymbList.Add("CCHECK",         PPOBJ_CCHECK);       // @v8.2.11
		SymbList.Add("PROCESSOR",      PPOBJ_PROCESSOR);    // @v8.7.0
		SymbList.Add("TSESSION",       PPOBJ_TSESSION);     // @v8.7.0
		SymbList.Add("STYLOPALM",      PPOBJ_STYLOPALM);    // @v8.7.0
		SymbList.Add("STYLODEVICE",    PPOBJ_STYLOPALM);    // @v8.7.2
	}
}

int SLAPI PPSession::GetObjectTypeSymb(PPID objType, SString & rBuf)
{
	rBuf = 0;
	long   ext = 0;
	if(!P_ObjIdentBlk)
		DO_CRITICAL(SETIFZ(P_ObjIdentBlk, new ObjIdentBlock));
	return P_ObjIdentBlk ? P_ObjIdentBlk->SymbList.GetByAssoc(objType, rBuf) : 0;
}

PPID SLAPI PPSession::GetObjectTypeBySymb(const char * pSymb, long * pExtraParam)
{
	PPID   obj_type = 0;
	long   ext = 0;
	if(!P_ObjIdentBlk)
		DO_CRITICAL(SETIFZ(P_ObjIdentBlk, new ObjIdentBlock));
	if(P_ObjIdentBlk) {
		SString symb = pSymb;
		uint   val = 0;
		if(P_ObjIdentBlk->SymbList.Search(symb.ToUpper(), &val, 0)) {
			obj_type = LoWord(val);
			ext = HiWord(val);
		}
		else {
			PPSetError(PPERR_OBJTYPEBYSYMBNFOUND);
		}
	}
	ASSIGN_PTR(pExtraParam, ext);
	return obj_type;
}

int SLAPI PPSession::GetObjectTitle(PPID objType, SString & rBuf)
{
	int    ok = -1;
	if(!P_ObjIdentBlk)
		DO_CRITICAL(SETIFZ(P_ObjIdentBlk, new ObjIdentBlock));
	if(IS_DYN_OBJTYPE(objType)) {
		ReferenceTbl::Rec ref_rec;
		Reference * p_ref = GetTLA().P_Ref;
		if(p_ref && p_ref->GetItem(PPOBJ_DYNAMICOBJS, objType, &ref_rec) > 0) {
			rBuf = ref_rec.ObjName;
			if(!rBuf.NotEmptyS())
				(rBuf = 0).CatEq("DYN OBJ", objType);
			ok = 1;
		}
		else
			(rBuf = 0).Cat(objType);
	}
	else if(P_ObjIdentBlk) {
		if(objType >= PPOBJ_FIRST_CFG_OBJ && objType < PPOBJ_LAST_CFG_OBJ) {
			if(P_ObjIdentBlk->TitleList.Get(objType, rBuf) > 0)
				ok = 2;
			else
				(rBuf = 0).Cat(objType);
		}
		else if(P_ObjIdentBlk->TitleList.Get(objType, rBuf) > 0)
			ok = 1;
		else {
			int    found = 0;
			if(objType > 0) {
				//
				// Могут существовать объекты, которые не входят в перечисление, возвращаемое
				// PPGetObjTypeList().
				// В этом случае применяем индивидуальный подход...
				//
				ENTER_CRITICAL_SECTION
				SString name_buf;
				if(PPLoadString(PPSTR_OBJNAMES, (uint)objType, name_buf)) {
					P_ObjIdentBlk->TitleList.Add(objType, name_buf);
					rBuf = name_buf;
					found = 1;
				}
				LEAVE_CRITICAL_SECTION
			}
			if(!found)
				(rBuf = 0).Cat(objType);
		}
	}
	return ok;
}

int SLAPI PPSession::SetTempLogFileName(const char * pFileName)
{
	GetTLA().TempLogFile = pFileName;
	return 1;
}

int SLAPI PPSession::SetPrivateBasket(PPBasketPacket * pPack, int use_ta)
{
	return GetTLA().Cart.Set(pPack, use_ta);
}

PPBasketPacket * SLAPI PPSession::GetPrivateBasket()
{
	return GetTLA().Cart.Get();
}

PPJobSrvClient * SLAPI PPSession::GetClientSession(int dontReconnect)
{
	PPJobSrvClient * p_cli = &GetTLA().SrvSess;
	if(p_cli && CConfig.Flags & CCFLG_3TIER) {
		if(!p_cli->Sync(1)) {
			if(dontReconnect)
				p_cli = 0;
			else {
				int r = p_cli->Reconnect(0, -1);
				if(r > 0 && r != 2) {
					PPSecur usr_rec;
					char   pw[128];
					SString user_name, db_symb;
					CurDict->GetDbSymb(db_symb);
					if(PPRef->GetItem(PPOBJ_USR, LConfig.User, &usr_rec) > 0) {
						Reference::GetPassword(&usr_rec, pw, sizeof(pw));
						if(p_cli->Login(db_symb, usr_rec.Name, pw))
							r = 1;
					}
				}
				if(r <= 0)
					p_cli = 0;
			}
		}
	}
	else
		p_cli = 0;
	return p_cli;
}

int SLAPI PPSession::StopThread(ThreadID tId)
{
	int    ok = -1;
#ifdef _MT // {
	ok = ThreadList.StopThread(tId);
#endif // } _MT
	return ok;
}

int SLAPI PPSession::IsThreadStopped()
{
	int    ok = 0;
#ifdef _MT // {
	const PPThread * p_thread = ThreadList.SearchById(GetConstTLA().GetThreadID());
	ok = BIN(p_thread && p_thread->IsStopping());
#endif // } _MT
	return ok;
}

PPAdviseEvent & FASTCALL PPAdviseEvent::operator = (const SysJournalTbl::Rec & rSjRec)
{
	Ident = 0;
	Dtm.Set(rSjRec.Dt, rSjRec.Tm);
	Action = rSjRec.Action;
	Oid.Set(rSjRec.ObjType, rSjRec.ObjID);
	UserID = rSjRec.UserID;
	SjExtra = rSjRec.Extra;
	Flags = 0;
	ExtraObj = 0;
	return *this;
}

#define ADVEVQCLISIGN 0x12ABCDEF

PPAdviseEventQueue::Client::Client()
{
	Sign = ADVEVQCLISIGN;
	Marker = 0;
}

PPAdviseEventQueue::Client::~Client()
{
	Sign = 0;
}

int PPAdviseEventQueue::Client::IsConsistent() const
{
	return BIN(Sign == ADVEVQCLISIGN);
}

int64 PPAdviseEventQueue::Client::GetMarker() const
{
	return Marker;
}

int PPAdviseEventQueue::Client::Register(long dbPathID, PPAdviseEventQueue * pQueue)
{
	int    ok = -1;
	if(dbPathID && pQueue && !RegDbList.lsearch(dbPathID)) {
		ok = pQueue->RegisterClient(this);
		RegDbList.add(dbPathID);
		assert(ok > 0);
	}
	return ok;
}

PPAdviseEventQueue::Stat::Stat()
{
	LivingTime = 0;
	StartClock = 0;
	Push_Count = 0;
	Get_Count = 0;
	GetDecline_Count = 0;
	MaxLength = 0;
}

SLAPI PPAdviseEventQueue::PPAdviseEventQueue() : TSArray <PPAdviseEvent> (), CliList(DEFCOLLECTDELTA, (aryDataOwner|aryPtrContainer))
{
	LastIdent = 0;
	//
	S.StartClock = clock();
}

int SLAPI PPAdviseEventQueue::GetStat(PPAdviseEventQueue::Stat & rStat)
{
	int    ok = 1;
    SLck.Lock();
    S.LivingTime = (int64)clock() - S.StartClock;
    rStat = S;
    SLck.Unlock();
    return ok;
}

int FASTCALL PPAdviseEventQueue::RegisterClient(const Client * pCli)
{
	int    ok = -1;
	ClLck.Lock();
	if(pCli && pCli->IsConsistent()) {
		uint _pos = 0;
		for(uint i = 0; !_pos && i < CliList.getCount(); i++) {
			const Client * p_cli = (const Client *)CliList.at(i);
			if(p_cli == pCli) {
				_pos = i+1;
			}
		}
		if(!_pos) {
            CliList.insert(pCli);
			ok = 1;
		}
	}
	ClLck.Unlock();
	return ok;
}

int FASTCALL PPAdviseEventQueue::Push(const TSArray <PPAdviseEvent> & rList)
{
	int    ok = 1;
	uint   ql = 0;
	if(rList.getCount()) {
		Lck.WriteLock();
		for(uint i = 0; i < rList.getCount(); i++) {
			PPAdviseEvent ev = rList.at(i);
			ev.Ident = ++LastIdent;
			insert(&ev);
		}
		ql = getCount();
		Lck.Unlock();
	}
	SLck.Lock();
	S.LivingTime = (int64)clock() - S.StartClock;
	S.Push_Count++;
	if(ql > S.MaxLength)
		S.MaxLength = ql;
	SLck.Unlock();
	return ok;
}

uint PPAdviseEventQueue::GetCount()
{
	uint   c = 0;
	Lck.ReadLockT(0);
	c = getCount();
	Lck.Unlock();
	return c;
}

int PPAdviseEventQueue::Get(int64 lowIdent, TSArray <PPAdviseEvent> & rList)
{
	int    ok = -1;
	int    declined = 0;
	//
	// Попытка получить доступ к очереди на чтение без ожидания.
	// Принцип работы очереди в том, чтобы клиентский поток не ждал пока
	// серверный поток получит очередную порцию данных от источника.
	//
	if(Lck.ReadLockT(0) > 0) {
		rList.clear();
		const  uint _c = getCount();
		if(_c && lowIdent <= at(_c-1).Ident) {
			uint _pos = 0;
			if(lowIdent >= at(0).Ident) {
				if(bsearch(&lowIdent, &_pos, PTR_CMPFUNC(int64)))
					_pos++;
				else {
					_pos = _c;
					if(_pos) do {
						const int64 _zi = at(--_pos).Ident;
						assert(_zi > at(_pos).Ident); // Тест на сортированный порядок элементов
						if(lowIdent >= _zi) {
							_pos++;
							break;
						}
					} while(_pos);
				}
			}
			for(; _pos < _c; _pos++) {
				rList.insert(&at(_pos));
			}
		}
		Lck.Unlock();
		ok = 1;
	}
	else
		declined = 1;
	SLck.Lock();
	S.Get_Count++;
	if(declined)
		S.GetDecline_Count++;
	SLck.Unlock();
	return ok;
}

int PPAdviseEventQueue::Purge()
{
	int    ok = -1;
	Lck.WriteLock();
	const  uint _c = getCount();
	if(_c > 1024) {
		int64  marker = 0x0fffffffffffffff;
		{
			ClLck.Lock();
			for(uint i = 0; i < CliList.getCount(); i++) {
				const Client * p_cli = (const Client *)CliList.at(i);
				if(p_cli && p_cli->IsConsistent()) {
					const int64 cm = p_cli->GetMarker();
					if(cm > 0 && cm < marker)
						marker = cm;
				}
			}
			ClLck.Unlock();
		}
		{
			const int64 last_ident = at(_c-1).Ident;
			if(marker >= last_ident) {
				freeAll();
				ok = 1;
			}
			else if(marker >= at(0).Ident) {
				uint   _low = 0;
				uint   _upp = 0;
				int    skip = 0;
				uint   _pos = 0;
				if(bsearch(&marker, &_pos, PTR_CMPFUNC(int64))) {
					_upp = _pos;
				}
				else {
					for(_pos = 0; _pos < _c; _pos++) {
						const int64 _zi = at(_pos).Ident;
						assert(!_pos || _zi > at(_pos-1).Ident); // Тест на сортированный порядок элементов
						if(_zi > marker) {
							assert(_pos > 0); // Из-за предшествующих условий _pos не может быть первым (0)
							if(_pos > 0) // @paranoic
								_upp = _pos-1;
							else
								skip = 1;
							break;
						}
					}
				}
				if(!skip && _upp >= _low) {
					freeChunk(_low, _upp);
					ok = 1;
				}
			}
		}
	}
	Lck.Unlock();
	return ok;
}
//
//
//
#pragma warning(disable:4073)
#pragma init_seg(user)
PPSession  DS; // @global
