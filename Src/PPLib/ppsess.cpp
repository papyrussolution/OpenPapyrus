// PPSESS.CPP
// Copyright (c) A.Sobolev 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021
//
#include <pp.h>
#pragma hdrstop
#include <private\_ppo.h>
#include <float.h>
#include <signal.h>
#include <ppsoapclient.h>
#include <sartre.h>
//
//
//
// @v10.4.8 (replaced with _PPConst.UseAdvEvQueue) #define USE_ADVEVQUEUE 1
const PPConstParam _PPConst;
//
//
//
class DbDict_DL600 : public DbDictionary {
public:
	static DbDictionary * CreateInstance(const char * pPath, long options);

	DbDict_DL600() : DbDictionary()
	{
	}
	virtual int LoadTableSpec(DBTable * pTbl, const char * pTblName);
	virtual int CreateTableSpec(DBTable * pTbl);
	virtual int DropTableSpec(const char * pTblName, DbTableStat * pStat);
	virtual int GetTableID(const char * pTblName, long * pID, DbTableStat * pStat);
	virtual int GetTableInfo(long tblID, DbTableStat * pStat);
	virtual int GetListOfTables(long options, StrAssocArray * pList);
private:
	int    ExtractStat(const DlContext * pCtx, const DlScope * pScope, DbTableStat * pStat) const;
};
//
// Descr: Процедура создания словаря DbDict_DL600. Используется для установки
//   функцией DbDictionary::SetCreateInstanceProc
//
/*static*/ DbDictionary * DbDict_DL600::CreateInstance(const char * pPath, long options)
{
	return new DbDict_DL600;
}

int DbDict_DL600::LoadTableSpec(DBTable * pTbl, const char * pTblName)
{
	int    ok = 1;
	DlContext * p_ctx = DS.GetInterfaceContext(PPSession::ctxDatabase);
	THROW(p_ctx);
	THROW(p_ctx->LoadDbTableSpec(pTblName, pTbl, 0));
	CATCHZOK
	return ok;
}

int DbDict_DL600::CreateTableSpec(DBTable * pTbl)
{
	int    ok = 1;
	if(pTbl->GetTableName()[0] == 0) {
		SString tbl_name;
		ulong  t = SLS.GetTLA().Rg.Get();
		pTbl->SetTableName((tbl_name = "CT").Cat(t));
	}
	DlContext * p_ctx = DS.GetInterfaceContext(PPSession::ctxDatabase);
	THROW(p_ctx);
	THROW(p_ctx->CreateNewDbTableSpec(pTbl));
	CATCHZOK
	return ok;
}

int DbDict_DL600::DropTableSpec(const char * pTblName, DbTableStat * pStat)
{
	int    ok = 1;
	DlContext * p_ctx = DS.GetInterfaceContext(PPSession::ctxDatabase);
	THROW(p_ctx);
	THROW(p_ctx->DropDbTableSpec(pTblName));
	CATCHZOK
	return ok;
}

static void FASTCALL SetXtfByDlScope(const DlScope * pScope, DbTableStat * pStat, int dlscopeAttr, long flagToSet)
{
	if(pScope->GetAttrib(dlscopeAttr, 0))
		pStat->Flags |= flagToSet;
}

int DbDict_DL600::ExtractStat(const DlContext * pCtx, const DlScope * pScope, DbTableStat * pStat) const
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
	SetXtfByDlScope(pScope, pStat, DlScope::sfDbtVLR, XTF_VLR);
	SetXtfByDlScope(pScope, pStat, DlScope::sfDbtVAT, XTF_VAT);
	SetXtfByDlScope(pScope, pStat, DlScope::sfDbtTruncate, XTF_TRUNCATE);
	SetXtfByDlScope(pScope, pStat, DlScope::sfDbtCompress, XTF_COMPRESS);
	SetXtfByDlScope(pScope, pStat, DlScope::sfDbtBalanced, XTF_BALANCED);
	SetXtfByDlScope(pScope, pStat, DlScope::sfDbtTemporary, XTF_TEMP);
	SetXtfByDlScope(pScope, pStat, DlScope::sfDbtSystem, XTF_DICT);
	pStat->RetItems = (DbTableStat::iID|DbTableStat::iOwnerLevel|DbTableStat::iFlags|DbTableStat::iName|DbTableStat::iLocation);
	return 1;
}

int DbDict_DL600::GetTableID(const char * pTblName, long * pID, DbTableStat * pStat)
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

int DbDict_DL600::GetTableInfo(long tblID, DbTableStat * pStat)
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

int DbDict_DL600::GetListOfTables(long options, StrAssocArray * pList)
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
int FASTCALL StatusWinChange(int onLogon /*=0*/, long timer/*=-1*/)
{
	int    ok = 1;
	DWORD  gr_gdiobj  = 0;
	DWORD  gr_userobj = 0;
	//SString temp_buf;
	TProgram * p_app = APPL;
	if(p_app && DBS.GetConstTLA().P_CurDict) {
		PPThreadLocalArea & r_tla = DS.GetTLA();
		DbProvider * p_dict = CurDict;
		{
			SString & r_sbuf = SLS.AcquireRvlStr();
			p_app->ClearStatusBar();
			if(timer >= 0) {
				PPLoadTextWin(PPTXT_AUTOEXIT_INFO, r_sbuf);
				SString & r_temp_buf = SLS.AcquireRvlStr();
				r_temp_buf.Printf(r_sbuf, timer);
				p_app->AddStatusBarItem(r_temp_buf, 0, GetColorRef(SClrRed), 0, GetColorRef(SClrBlack));
			}
			p_app->AddStatusBarItem(GetMainOrgName(r_sbuf).Transf(CTRANSF_INNER_TO_OUTER), 0, 0, cmViewStatus);
			p_app->AddStatusBarItem((r_sbuf = r_tla.CurDbDivName).Transf(CTRANSF_INNER_TO_OUTER), 0, 0, cmViewStatus);
			GetLocationName(LConfig.Location, r_sbuf);
			p_app->AddStatusBarItem(r_sbuf.Transf(CTRANSF_INNER_TO_OUTER), 0, 0, cmViewStatus);
			r_sbuf.Z().Cat(LConfig.OperDate, MKSFMT(0, DATF_DMY | DATF_CENTURY));
			p_app->AddStatusBarItem(r_sbuf, 0, 0, cmViewStatus);
			{
				if(p_dict) {
					SString & r_temp_buf = SLS.AcquireRvlStr();
					if(p_dict->GetDbName(r_temp_buf) > 0) {
						p_app->AddStatusBarItem((r_sbuf = "DB").CatDiv(':', 2).Cat(r_temp_buf.Transf(CTRANSF_INNER_TO_OUTER)), 0, 0, cmViewStatus);
					}
				}
			}
			p_app->AddStatusBarItem("www.petroglif.ru", 0, GetColorRef(SClrAliceblue), cmGotoSite, GetColorRef(SClrBlue));
			//turistti
			gr_gdiobj  = ::GetGuiResources(GetCurrentProcess(), GR_GDIOBJECTS);
			gr_userobj = ::GetGuiResources(GetCurrentProcess(), GR_USEROBJECTS);
			//end turistti
#ifndef NDEBUG
			{
				MemHeapTracer mht;
				MemHeapTracer::Stat mht_stat;
				if(mht.CalcStat(&mht_stat)) {
					r_sbuf.Z().Cat(mht_stat.UsedBlockCount).CatDiv('-', 1).
						Cat(mht_stat.UsedSize).CatDiv('-', 1).
						Cat(mht_stat.UnusedBlockCount).CatDiv('-', 1).
						Cat(mht_stat.UnusedSize).CatDiv(';', 2).
						CatEq("GDI", gr_gdiobj).CatDiv(';', 2).
						CatEq("USER", gr_userobj).CatDiv(';', 2).
						Cat("FOCUS").Eq().CatHex((long)::GetFocus()).CatDiv(';', 2).
						Cat("CAPTURE").Eq().CatHex((long)::GetCapture());
					p_app->AddStatusBarItem(r_sbuf);
				}
				else {
					p_app->AddStatusBarItem("Heap Corrupted");
				}
			}
#endif
		}
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
					PPObjPerson::GetCurUserPerson(&employer, 0);
					if(employer) {
						SETIFZ(r_tla.P_TodoObj, new PPObjPrjTask);
						PPObjPrjTask * p_todo_obj = DS.GetTLA().P_TodoObj;
						if(p_todo_obj) {
							PrjTaskCore * t = p_todo_obj->P_Tbl;
							PrjTaskTbl::Rec todo_rec;
							if(check_new_task) {
								// @v10.0.0 {
								for(SEnum en = t->EnumByEmployer(employer, 0, 0); en.Next(&todo_rec) > 0;) {
									if(!(todo_rec.Flags & TODOF_OPENEDBYEMPL) && !oneof2(todo_rec.Status, TODOSTTS_REJECTED, TODOSTTS_COMPLETED)) {
										SString & r_temp_buf = SLS.AcquireRvlStr();
										PPLoadString("newtask", r_temp_buf);
										APPL->AddStatusBarItem(r_temp_buf.Transf(CTRANSF_INNER_TO_OUTER), ICON_NEWTASK, 0, cmPrjTask_ByStatus);
										break;
									}
								}
								// } @v10.0.0
								/* @v10.0.0
								PrjTaskTbl::Key4 k4;
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
											//PPGetWord(PPWORD_NEWTASK, 1, temp_buf);
											PPLoadString("newtask", temp_buf);
											APPL->AddStatusBarItem(temp_buf.Transf(CTRANSF_INNER_TO_OUTER), ICON_NEWTASK, 0, cmPrjTask_ByStatus);
											break;
										}
									}
								}
								*/
							}
							if(rmnd_incompl_task) {
								DateRange period;
								LDATE cur_dt = getcurdate_();
								period.SetDate(cur_dt);
								plusdate(&period.upp, abs(prj_cfg.RemindPrd.low), 0);
								if(prj_cfg.RemindPrd.low != prj_cfg.RemindPrd.upp)
									plusdate(&period.low, -abs(prj_cfg.RemindPrd.upp), 0);
								// @v10.0.0 {
								for(SEnum en = t->EnumByEmployer(employer, &period, 0); en.Next(&todo_rec) > 0;) {
									if(!oneof2(todo_rec.Status, TODOSTTS_REJECTED, TODOSTTS_COMPLETED)) {
										SString & r_temp_buf = SLS.AcquireRvlStr();
										PPLoadString("incompletetasks", r_temp_buf);
										APPL->AddStatusBarItem(r_temp_buf.Transf(CTRANSF_INNER_TO_OUTER), ICON_TASKREMINDER, 0, cmPrjTask_ByReminder);
										break;
									}
								}
								// } @v10.0.0
								/* @v10.0.0
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
											//PPGetWord(PPWORD_INCOMPLETETASK, 1, temp_buf);
											PPLoadString("incompletetasks", temp_buf);
											APPL->AddStatusBarItem(temp_buf.Transf(CTRANSF_INNER_TO_OUTER), ICON_TASKREMINDER, 0, cmPrjTask_ByReminder);
											break;
										}
									}
								}
								*/
							}
						}
					}
				}
			}
		}
		// @v10.5.8 {
		{
			TWindow * p_phn_pane = static_cast<PPApp *>(APPL)->FindPhonePaneDialog();
			if(p_phn_pane) {
				APPL->AddStatusBarItem("Phone Pane", /*PPDV_PHONE03*/ICON_PHONE, 0, /*cmPrjTask_ByReminder*/cmOpenPhonePane);
			}
		}
		// } @v10.5.8
		if(DS.GetPrivateBasket()) {
			SString & r_temp_buf = SLS.AcquireRvlStr();
			PPLoadString("privategoodsbasket", r_temp_buf);
			r_temp_buf.Transf(CTRANSF_INNER_TO_OUTER);
			p_app->AddStatusBarItem(r_temp_buf, ICON_BASKET_SMALL, 0, cmPrivateBasket);
		}
		{
			HWND   h_curr_wnd = ::GetTopWindow(APPL->GetFrameWindow());
			if(h_curr_wnd) {
				SString & r_temp_buf = SLS.AcquireRvlStr();
				TView::SGetWindowClassName(h_curr_wnd, r_temp_buf.Z());
				if(r_temp_buf == "STextBrowser") {
					STextBrowser * p_view = static_cast<STextBrowser *>(TView::GetWindowUserData(h_curr_wnd));
					if(p_view && p_view->IsConsistent()) {
						STextBrowser::StatusBlock sb;
						if(p_view->GetStatus(&sb)) {
							r_temp_buf.Z();
							sb.Cp.ToStr(SCodepageIdent::fmtXML, r_temp_buf);
							r_temp_buf.Space().Cat("Ln").CatDiv(':', 2).Cat(sb.LineNo).CatChar('/').Cat(sb.LineCount).Space().
								Cat("Col").CatDiv(':', 2).Cat(sb.ColumnNo);
							p_app->AddStatusBarItem(r_temp_buf, 0, 0, 0);
						}
					}
				}
			}
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
				SString & r_temp_buf = SLS.AcquireRvlStr();
				PPLoadText(PPTXT_UPDATEAVAILABLE, r_temp_buf);
				r_temp_buf.Transf(CTRANSF_INNER_TO_OUTER);
				p_app->AddStatusBarItem(r_temp_buf, 0, GetColorRef(SClrGreenyellow), cmViewNewVersionList, GetColorRef(SClrRed));
			}
			LEAVE_CRITICAL_SECTION
		}
		p_app->UpdateStatusBar();
	}
	else
		ok = -1;
	return ok;
}

PPRevolver_StringSetSCD::PPRevolver_StringSetSCD(uint c) : TSRevolver <PPStringSetSCD>(c) {}
StringSet & PPRevolver_StringSetSCD::Get() { return Implement_Get().Z(); }

// @v10.9.12 (replaced with _PPConst.Signature_PPThreadLocalArea) #define SIGN_PPTLA 0x7D08E311L

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

PPThreadLocalArea::PPThreadLocalArea() : Prf(1), UfpSess(0), RvlSsSCD(256)
{
	memzero(this, offsetof(PPThreadLocalArea, Rights));
	Sign = _PPConst.Signature_PPThreadLocalArea;
	PrnDirId = labs(SLS.GetTLA().Rg.Get());
	RegisterAdviseObjects();
}

PPThreadLocalArea::~PPThreadLocalArea()
{
	ZDELETE(P_WObj);
	ZDELETE(P_WbObj);
	ZDELETE(P_TodoObj);
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
	ZDELETE(P_SrDb);
	ZDELETE(P_PhnSvcEvRespr);
	ZDELETE(P_MqbEvRespr); // @v10.5.7
	ZDELETE(P_SysMntnc); // @v10.6.1
	Sign = 0;
}

int  PPThreadLocalArea::SetupEventResponder(int eventResponderId)
{
	int    ok = -1;
	switch(eventResponderId) {
		case eventresponderPhoneService: ok = SETIFZ(P_PhnSvcEvRespr, new PhoneServiceEventResponder); break;
		case eventresponderMqb: ok = SETIFZ(P_MqbEvRespr, new MqbEventResponder); break;
		case eventresponderSysMaintenance: ok = SETIFZ(P_SysMntnc, new SysMaintenanceEventResponder); break;
	}
	return ok;
}

void PPThreadLocalArea::ReleaseEventResponder(int eventResponderId)
{
	switch(eventResponderId) {
		case eventresponderPhoneService: ZDELETE(P_PhnSvcEvRespr); break;
		case eventresponderMqb: ZDELETE(P_MqbEvRespr); break;
		case eventresponderSysMaintenance: ZDELETE(P_SysMntnc); break;
	}
}

int PPThreadLocalArea::RegisterAdviseObjects()
{
	class IdleCmdUpdateStatusWin : public IdleCommand {
	public:
	#ifndef NDEBUG
		#define UPD_STATUS_PERIOD 1
	#else
		#define UPD_STATUS_PERIOD 5
	#endif
		IdleCmdUpdateStatusWin(long repeatPeriodSec) : IdleCommand(repeatPeriodSec/*UPD_STATUS_PERIOD*/), OnLogon(1)
		{
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
		IdleCmdQuitSession(long repeatPeriodSec) : IdleCommand(repeatPeriodSec/*10*/), Timer(-1)
		{
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
						f.ReadLine(buf);
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
	class IdleCmdMqb : public IdleCommand, private PPAdviseEventQueue::Client {
	public:
		IdleCmdMqb(long refreshPeriod, PPID notifyID) : IdleCommand(refreshPeriod), NotifyID(notifyID)
		{
		}
		virtual int FASTCALL Run(const LDATETIME & rPrevRunTime)
		{
			int    ok = -1;
			PPAdviseEventQueue * p_queue = DS.GetAdviseEventQueue(this);
			const uint evqc = (p_queue && p_queue->Get(Marker, EvqList) > 0) ? EvqList.getCount() : 0;
			if(evqc) {
				Marker = EvqList.at(evqc-1).Ident; // Установить маркер очереди необходимо даже если подписчиков на события нет.
				PPAdviseList adv_list;
				if(!NotifyID || NotifyID == PPAdviseBlock::evMqbMessage)
					DS.GetAdviseList(PPAdviseBlock::evMqbMessage, 0, adv_list);
				if(adv_list.GetCount()) {
					SString temp_buf;
					PPThreadLocalArea & r_tla = DS.GetTLA();
					PPAdviseBlock adv_blk;
					PPNotifyEvent ev;
					for(uint i = 0; i < EvqList.getCount(); i++) {
						const PPAdviseEvent & r_ev = EvqList.at(i);
						if(r_ev.Action == PPEVNT_MQB_MESSAGE) {
							for(uint j = 0; adv_list.Enum(&j, &adv_blk);) {
								if(adv_blk.Proc) {
									ev.Clear();
									ev.Action = r_ev.Action;
									ev.P_MqbEnv = new PPMqbClient::Envelope;
									if(ev.P_MqbEnv && r_ev.ConvertToMqbEnvelope(EvqList, *ev.P_MqbEnv) > 0) {
										ev.ExtDtm  = rPrevRunTime;
										adv_blk.Proc(PPAdviseBlock::evMqbMessage, &ev, adv_blk.ProcExtPtr);
										adv_blk.Proc(PPAdviseBlock::evMqbMessage, &ev.Finalize(rPrevRunTime, 0), adv_blk.ProcExtPtr); // finalize
										ok = 1;
									}
								}
							}
						}
					}
				}
			}
			return ok;
		}
	private:
		PPID   NotifyID;
	};
	class IdleCmdPhoneSvc : public IdleCommand, private PPAdviseEventQueue::Client {
	public:
		IdleCmdPhoneSvc(long refreshPeriod, PPID notifyID) : IdleCommand(refreshPeriod), NotifyID(notifyID)
		{
		}
		void   FASTCALL SetupPhoneEvent(PPNotifyEvent & rN, const PPAdviseEvent & rSrc, SString & rTempBuf)
		{
			rN.Clear();
			rN.ObjType = rSrc.Oid.Obj; // @v10.0.02
			rN.ObjID = rSrc.Oid.Id; // @v10.0.02
			rN.Action = rSrc.Action;
			EvqList.GetS(rSrc.ChannelP, rTempBuf);
			rN.PutExtStrData(rN.extssChannel, rTempBuf);
			EvqList.GetS(rSrc.CallerIdP, rTempBuf);
			rN.PutExtStrData(rN.extssCallerId, rTempBuf);
			EvqList.GetS(rSrc.ConnectedLineNumP, rTempBuf);
			rN.PutExtStrData(rN.extssConnectedLineNum, rTempBuf);
			EvqList.GetS(rSrc.ContextP, rTempBuf);
			rN.PutExtStrData(rN.extssContext, rTempBuf);
			EvqList.GetS(rSrc.ExtenP, rTempBuf);
			rN.PutExtStrData(rN.extssExten, rTempBuf);
		}
		virtual int FASTCALL Run(const LDATETIME & rPrevRunTime)
		{
			int    ok = -1;
			PPAdviseEventQueue * p_queue = DS.GetAdviseEventQueue(this);
			const uint evqc = (p_queue && p_queue->Get(Marker, EvqList) > 0) ? EvqList.getCount() : 0;
			if(evqc) {
				//
				// Установить маркер очереди необходимо даже если подписчиков на события нет.
				//
				Marker = EvqList.at(evqc-1).Ident;
				PPAdviseList adv_list_ringing;
				PPAdviseList adv_list_up;
				if(!NotifyID || NotifyID == PPAdviseBlock::evPhoneRinging)
					DS.GetAdviseList(PPAdviseBlock::evPhoneRinging, 0, adv_list_ringing);
				if(!NotifyID || NotifyID == PPAdviseBlock::evPhoneUp)
					DS.GetAdviseList(PPAdviseBlock::evPhoneUp, 0, adv_list_up);
				if(adv_list_ringing.GetCount() || adv_list_up.GetCount()) {
					SString temp_buf;
					PPThreadLocalArea & r_tla = DS.GetTLA();
					PPAdviseBlock adv_blk;
					PPNotifyEvent ev;
					for(uint i = 0; i < evqc; i++) {
						const PPAdviseEvent & r_ev = EvqList.at(i);
						if(r_ev.Action == PPEVNT_PHNS_RINGING) {
							for(uint j = 0; adv_list_ringing.Enum(&j, &adv_blk);) {
								if(adv_blk.Proc) {
									SetupPhoneEvent(ev, r_ev, temp_buf);
									ev.ExtDtm  = rPrevRunTime;
									adv_blk.Proc(PPAdviseBlock::evPhoneRinging, &ev, adv_blk.ProcExtPtr);
									adv_blk.Proc(PPAdviseBlock::evPhoneRinging, &ev.Finalize(rPrevRunTime, 0), adv_blk.ProcExtPtr); // finalize
									ok = 1;
								}
							}
						}
						else if(r_ev.Action == PPEVNT_PHNC_UP) {
							for(uint j = 0; adv_list_up.Enum(&j, &adv_blk);) {
								if(adv_blk.Proc) {
									SetupPhoneEvent(ev, r_ev, temp_buf);
									ev.ExtDtm  = rPrevRunTime;
									adv_blk.Proc(PPAdviseBlock::evPhoneUp, &ev, adv_blk.ProcExtPtr);
									adv_blk.Proc(PPAdviseBlock::evPhoneUp, &ev.Finalize(rPrevRunTime, 0), adv_blk.ProcExtPtr); // finalize
									ok = 1;
								}
							}
						}
					}
				}
			}
			return ok;
		}
		PPID   NotifyID;
	};
	class IdleCmdUpdateObjList : public IdleCommand, private PPAdviseEventQueue::Client {
	public:
		IdleCmdUpdateObjList(long refreshPeriod, PPID objTypeID, PPID notifyID) : IdleCommand(refreshPeriod), ObjTypeID(objTypeID), NotifyID(notifyID)
		{
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
							if(r_ev.Action && ((ObjTypeID == -1) || r_ev.Oid.Obj == ObjTypeID))
								IdList.add(r_ev.Oid.Id);
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
									adv_blk.Proc(NotifyID, &ev.Finalize(prev_dtm, 0), adv_blk.ProcExtPtr); // finalize
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
	class IdleCmdConfigUpdated : public IdleCommand, private PPAdviseEventQueue::Client { // @v10.3.1
	public:
		IdleCmdConfigUpdated(long refreshPeriod, PPID configID, PPID notifyID) : IdleCommand(refreshPeriod), ConfigID(configID), NotifyID(notifyID)
		{
		}
		virtual int FASTCALL Run(const LDATETIME & rPrevRunTime)
		{
			int    ok = -1;
			if(NotifyID) {
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
						CALLPTRMEMB(p_sj, GetUpdatedConfigListSince((ConfigID > 0) ? ConfigID : 0, rPrevRunTime, IdList));
					}
					else if(evqc) {
						for(uint i = 0; i < evqc; i++) {
							const PPAdviseEvent & r_ev = EvqList.at(i);
							if(r_ev.Action && (!ConfigID || r_ev.Oid.Obj == ConfigID))
								IdList.add(r_ev.Oid.Obj);
						}
						IdList.sortAndUndup();
					}
					{
						const uint c = IdList.getCount();
						if(c) {
							PPNotifyEvent ev;
							PPAdviseBlock adv_blk;
							const LDATETIME prev_dtm = rPrevRunTime;
							for(uint j = 0; adv_list.Enum(&j, &adv_blk);) {
								if(adv_blk.Proc) {
									for(uint i = 0; i < c; i++) {
										ev.Clear();
										ev.ObjType = IdList.get(i);
										ev.ObjID   = 0;
										ev.ExtDtm  = prev_dtm;
										adv_blk.Proc(NotifyID, &ev, adv_blk.ProcExtPtr);
									}
									adv_blk.Proc(NotifyID, &ev.Finalize(prev_dtm, 0), adv_blk.ProcExtPtr); // finalize
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
		PPID   ConfigID;
		PPIDArray IdList;
	};
	class IdleCmdUpdateTSessList : public IdleCommand, private PPAdviseEventQueue::Client {
	public:
		IdleCmdUpdateTSessList(long refreshPeriod, PPID notifyID) : IdleCommand(refreshPeriod), NotifyID(notifyID)
		{
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
					PPAdviseEventVector result_list;
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
							if(r_ev.Action && r_ev.Oid.Obj == PPOBJ_TSESSION)
								EvqList.MoveItemTo(i, result_list);
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
										assert(r_ev.Oid.Obj == PPOBJ_TSESSION);
										nev.Clear();
										nev.Action  = r_ev.Action;
										nev.ObjType = r_ev.Oid.Obj;
										nev.ObjID   = r_ev.Oid.Id;
										nev.ExtInt_ = r_ev.SjExtra;
										adv_blk.Proc(NotifyID, &nev, adv_blk.ProcExtPtr);
									}
									adv_blk.Proc(NotifyID, &nev.Finalize(prev_dtm, 0), adv_blk.ProcExtPtr); // finalize
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
	class IdleCmdEventCreation : public IdleCommand, private PPAdviseEventQueue::Client {
		const  PPID NotifyID;
	public:
		IdleCmdEventCreation(long repeatPeriodSec) : IdleCommand(repeatPeriodSec), NotifyID(PPAdviseBlock::evEventCreated)
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
			if(DS.GetAdviseList(NotifyID, 0, adv_list) > 0) {
				PPThreadLocalArea & r_tla = DS.GetTLA();
				PPAdviseEventVector result_list;
				if(p_queue && evqc) {
					for(uint i = 0; i < evqc; i++) {
						const PPAdviseEvent & r_ev = EvqList.at(i);
						if(r_ev.Action == PPACN_EVENTDETECTION && r_ev.Oid.Obj == PPOBJ_EVENTSUBSCRIPTION)
							EvqList.MoveItemTo(i, result_list);
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
									assert(r_ev.Action == PPACN_EVENTDETECTION);
									assert(r_ev.Oid.Obj == PPOBJ_EVENTSUBSCRIPTION);
									nev.Clear();
									nev.Action  = r_ev.Action;
									nev.ObjType = r_ev.Oid.Obj;
									nev.ObjID   = r_ev.Oid.Id;
									nev.ExtInt_ = r_ev.SjExtra;
									adv_blk.Proc(NotifyID, &nev, adv_blk.ProcExtPtr);
								}
								adv_blk.Proc(NotifyID, &nev.Finalize(prev_dtm, 0), adv_blk.ProcExtPtr); // finalize
							}
						}
						ok = 1;
					}
				}
			}
			return ok;
		}
	};
	class IdleCmdUpdateBizScoreOnDesktop : public IdleCommand, private PPAdviseEventQueue::Client {
	public:
		IdleCmdUpdateBizScoreOnDesktop(long repeatPeriodSec) : IdleCommand(repeatPeriodSec/*30*/)
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
						// @v10.6.4 MEMSZERO(sysj_rec);
						LDATETIME prev_dtm = rPrevRunTime;
						if(p_sj->GetLastEvent(PPACN_BIZSCOREUPDATED, 0/*extraVal*/, &prev_dtm, 2) > 0)
							do_notify = 1;
					}
				}
				else if(evqc) {
					const int32 _action = PPACN_BIZSCOREUPDATED;
					uint  _p = 0;
					if(EvqList.lsearch(&_action, &_p, CMPF_LONG, offsetof(PPAdviseEvent, Action)))
						do_notify = 1;
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
		IdleCmdUpdateCaches(long repeatPeriodSec) : IdleCommand(repeatPeriodSec/*30*/)
		{
		}
		virtual int FASTCALL Run(const LDATETIME & rPrevRunTime)
		{
			return DS.DirtyDbCache(DBS.GetDbPathID(), this);
		}
	};
	class IdleCmdUpdateLogsMon : public IdleCommand {
	public:
		IdleCmdUpdateLogsMon(long refreshPeriod, PPID objTypeID, PPID notifyID) : IdleCommand(refreshPeriod), ObjTypeID(objTypeID), NotifyID(notifyID)
		{
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
							adv_blk.Proc(NotifyID, &ev.Finalize(rPrevRunTime, 0), adv_blk.ProcExtPtr); // finalize
						}
					}
				}
				ok = 1;
			}
			return ok;
		}
	private:
		PPID   ObjTypeID;
		const  PPID NotifyID;
	};
	class IdleCmdQuartz : public IdleCommand {
	public:
		IdleCmdQuartz(PPID notifyID) : IdleCommand(1), NotifyID(notifyID)
		{
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
	IdleCmdList.insert(new IdleCmdUpdateStatusWin(UPD_STATUS_PERIOD));
	IdleCmdList.insert(new IdleCmdQuitSession(10));
	IdleCmdList.insert(new IdleCmdUpdateCaches(10)); // @v10.4.4 30-->10
	IdleCmdList.insert(new IdleCmdUpdateBizScoreOnDesktop(30));
	IdleCmdList.insert(new IdleCmdUpdateObjList(25, PPOBJ_PRJTASK, PPAdviseBlock::evTodoChanged)); // @v10.4.4 5-->25
	IdleCmdList.insert(new IdleCmdUpdateObjList(21, PPOBJ_BILL, PPAdviseBlock::evBillChanged)); // @v10.4.4 30-->21
	IdleCmdList.insert(new IdleCmdUpdateObjList(23, PPOBJ_PERSONEVENT, PPAdviseBlock::evPsnEvChanged)); // @v10.4.4 5-->23
	IdleCmdList.insert(new IdleCmdUpdateTSessList(30, PPAdviseBlock::evTSessChanged));
	IdleCmdList.insert(new IdleCmdUpdateObjList(27,  -1, PPAdviseBlock::evSysJournalChanged)); // @v10.4.4 5-->27
	IdleCmdList.insert(new IdleCmdUpdateLogsMon(10, -1, PPAdviseBlock::evLogsChanged)); // @note Это, похоже, просто Quartz
	IdleCmdList.insert(new IdleCmdQuartz(PPAdviseBlock::evQuartz));
	IdleCmdList.insert(new IdleCmdPhoneSvc(2, 0));
	IdleCmdList.insert(new IdleCmdConfigUpdated(60, 0, PPAdviseBlock::evConfigChanged)); // @v10.3.1
	{
#ifdef NDEBUG
		const long mqb_refresh_period = 73;
#else
		const long mqb_refresh_period = 17;
#endif
		IdleCmdList.insert(new IdleCmdMqb(mqb_refresh_period, PPAdviseBlock::evMqbMessage)); // @v10.5.7
	}
	IdleCmdList.insert(new IdleCmdEventCreation(83)); // @v10.9.0
// @v10.4.8 #if USE_ADVEVQUEUE==2
	if(_PPConst.UseAdvEvQueue == 2) { // @v10.4.8
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
							PPAdviseEvent & r_ev = EvqList.at(i);
							(msg_buf = "AdviseEvent").CatDiv(':', 2).CatEq("Ident", r_ev.Ident).Space().CatEq("Dtm", r_ev.Dtm).Space().
								CatEq("Action", r_ev.Action).Space();
							r_ev.Oid.ToStr(temp_buf);
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
		IdleCmdList.insert(new IdleCmdTestAdvEvQueue); // @debug
	}
// @v10.4.8 #endif
	return ok;
}

int    PPThreadLocalArea::IsAuth() const { return (State & stAuth) ? 1 : PPSetError(PPERR_SESSNAUTH); }
int    PPThreadLocalArea::IsConsistent() const { return BIN(Sign == _PPConst.Signature_PPThreadLocalArea); }
PPView * PPThreadLocalArea::GetPPViewPtr(int32 id) const { return (id > 0 && id <= SrvViewList.getCountI()) ? SrvViewList.at(id-1) : 0; }

int32 PPThreadLocalArea::CreatePPViewPtr(PPView * pView)
{
	for(uint i = 0; i < SrvViewList.getCount(); i++) {
		if(SrvViewList.at(i) == 0) {
			SrvViewList.atPut(i, pView);
			return static_cast<int32>(i+1);
		}
	}
	SrvViewList.insert(pView);
	return SrvViewList.getCountI();
}

int PPThreadLocalArea::ReleasePPViewPtr(int32 id)
{
	if(id > 0 && id <= SrvViewList.getCountI()) {
		SrvViewList.atPut(id-1, 0);
		return 1;
	}
	else
		return 0;
}
//
//
//
static ACount TlpC(ACount::ctrDontInitialize); // @global @threadsafe

__PPThrLocPtr::__PPThrLocPtr() : Idx(TlpC.Incr())
{
}

int __PPThrLocPtr::IsOpened()
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

int PPThreadLocalArea::CreatePtr(uint idx, void * ptr)
{
	if(idx > PtrVectDim) {
		uint   new_dim = ALIGNSIZE(idx, 6);
		PtrEntry * p = static_cast<PtrEntry *>(SAlloc::R(P_PtrVect, sizeof(PtrEntry) * new_dim));
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

void PPThreadLocalArea::PushErrContext()
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

void PPThreadLocalArea::PopErrContext()
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

int PPThreadLocalArea::InitMainOrgData(int reset)
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

int PPThreadLocalArea::SetIfcConfigParam(const char * pParam, const char * pValue)
{
	int    ok = 1;
	SString param_buf(pParam);
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

SrDatabase * PPThreadLocalArea::GetSrDatabase()
{
	SrDatabase * p_db = 0;
	if(P_SrDb)
		p_db = P_SrDb;
    else if(Cc.Flags2 & CCFLG2_USESARTREDB) {
		SString db_path;
		PPGetPath(PPPATH_SARTREDB, db_path);
		if(IsDirectory(db_path.RmvLastSlash())) {
            p_db = new SrDatabase();
            if(p_db) {
				if(p_db->Open(db_path, SrDatabase::oReadOnly))
					P_SrDb = p_db;
				else
					ZDELETE(p_db);
            }
		}
    }
    return p_db;
}

int PPThreadLocalArea::GetIfcConfigParam(const char * pParam, SString & rValue) const
{
	rValue.Z();
	return IfcConfig.Search(pParam, &rValue, 0);
}

PPThreadLocalArea::PrivateCart::PrivateCart() : P(0)
{
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
/*static*/const char * PPSession::P_JobLogin = "$SYSSERVICE$"; // @global
/*static*/const char * PPSession::P_EmptyBaseCreationLogin = "$EMPTYBASECREATION$"; // @global

PPSession::ThreadCollection::ThreadCollection() : TSCollection <PPThread> ()
{
	setFlag(aryEachItem, 0);
}

int FASTCALL PPSession::ThreadCollection::Add(const PPThread * pThread)
{
	int    ok = 1;
	if(pThread) {
		SRWLOCKER(RwL, SReadWriteLocker::Write);
		const uint   c = getCount();
		for(uint i = 0; ok > 0 && i < c; i++)
			if(at(i) == pThread)
				ok = -1;
		if(ok > 0)
			ok = insert(pThread) ? 1 : PPSetErrorSLib();
	}
	return ok;
}

int FASTCALL PPSession::ThreadCollection::Remove(ThreadID id)
{
	int    ok = -1;
	{
		SRWLOCKER(RwL, SReadWriteLocker::Write);
		const uint   c = getCount();
		for(uint i = 0; ok < 0 && i < c; i++) {
			PPThread * p_thread = at(i);
			if(p_thread && p_thread->GetThreadID() == id)
				ok = atFree(i) ? 1 : PPSetErrorSLib();
		}
	}
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
	{
		SRWLOCKER(RwL, SReadWriteLocker::Read);
		c = getCount();
	}
	return c;
}

int FASTCALL PPSession::ThreadCollection::GetInfoList(int type, TSCollection <PPThread::Info> & rList)
{
	int    ok = 1;
	{
		SRWLOCKER(RwL, SReadWriteLocker::Read);
		const  uint c = getCount();
		for(uint i = 0; i < c; i++) {
			const PPThread * p = at(i);
			if(p && p->IsConsistent()) {
				if(!type || p->GetKind() == type) {
					PPThread::Info * p_info = rList.CreateNewItem();
					if(p_info)
						p->GetInfo(*p_info);
				}
			}
		}
	}
	return ok;
}

void FASTCALL PPSession::ThreadCollection::LocStkToStr(SString & rBuf)
{
	rBuf.Z();
	{
		SRWLOCKER(RwL, SReadWriteLocker::Read);
		const  uint c = getCount();
		for(uint i = 0; i < c; i++) {
			const PPThread * p = at(i);
			if(p && p->IsConsistent()) {
				rBuf.CatEq("PPThread", p->GetThreadID()).CatDiv(':', 2).CR();
				p->LockStackToStr(rBuf);
			}
		}
	}
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
	{
		SRWLOCKER(RwL, SReadWriteLocker::Read);
		const uint c = getCount();
		for(uint i = 0; i < c; i++) {
			PPThread * p_thread = at(i);
			if(p_thread && p_thread->IsConsistent() && p_thread->GetThreadID() == tId) {
				p_ret = p_thread;
				break;
			}
		}
	}
	return p_ret;
}

PPThread * FASTCALL PPSession::ThreadCollection::SearchBySessId(int32 sessId)
{
	PPThread * p_ret = 0;
	{
		SRWLOCKER(RwL, SReadWriteLocker::Read);
		const uint c = getCount();
		for(uint i = 0; i < c; i++) {
			PPThread * p_thread = at(i);
			if(p_thread && p_thread->IsConsistent() && p_thread->GetUniqueSessID() == sessId) {
				p_ret = p_thread;
				break;
			}
		}
	}
	return p_ret;
}

PPThread * FASTCALL PPSession::ThreadCollection::SearchIdle(int kind)
{
	PPThread * p_ret = 0;
	{
		SRWLOCKER(RwL, SReadWriteLocker::Read);
		const uint c = getCount();
		for(uint i = 0; i < c; i++) {
			PPThread * p_thread = at(i);
			if(p_thread && p_thread->IsConsistent() && p_thread->GetKind() == kind && p_thread->IsIdle()) {
				p_ret = p_thread;
				break;
			}
		}
	}
	return p_ret;
}

uint FASTCALL PPSession::ThreadCollection::GetCount(int kind)
{
	uint   result = 0;
	{
		SRWLOCKER(RwL, SReadWriteLocker::Read);
		const uint c = getCount();
		for(uint i = 0; i < c; i++) {
			PPThread * p_thread = at(i);
			if(p_thread && p_thread->IsConsistent() && p_thread->GetKind() == kind) {
				result++;
			}
		}
	}
	return result;
}
//
//
//
PPSession::RegSessData::RegSessData()
{
	THISZERO();
}

PPSession::LoggerIntermediateBlock::LoggerIntermediateBlock(const PPSession & rS) : CfgMaxFileSize(rS.GetMaxLogFileSize())
{
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

PPSession::PPSession() : Id(1), ExtFlags_(0), P_ObjIdentBlk(0), P_LogQueue(0), P_DbCtx(0), P_AlbatrosCfg(0), P_SrStxSet(0),
	MaxLogFileSize(32768), State(0), TlsIdx(::TlsAlloc()), P_ExtCfgDb(0)
{
	InitThread(0);
}

PPSession::~PPSession()
{
	ReleaseThread();
	TlsFree(TlsIdx);
	delete P_ObjIdentBlk;
	delete P_DbCtx;
	delete P_AlbatrosCfg;
	delete P_SrStxSet; // @v9.8.10
	delete P_ExtCfgDb; // @v10.6.7
	// Don't destroy P_LogQueue (на объект может ссылаться поток PPLogMsgSession потому удалять его нельзя)
}

int PPSession::EnsureExtCfgDb()
{
	int    ok = -1;
	SString dbpath_buf;
	PPGetPath(PPPATH_WORKSPACE, dbpath_buf);
	dbpath_buf.SetLastSlash().Cat("bdbconfig");
	if(!IsDirectory(dbpath_buf)) {
		createDir(dbpath_buf);
		//
		SString lock_path;
		(lock_path = dbpath_buf).SetLastSlash().Cat("ppcfgdbcrlock");
		const int  is_locked = BIN(fileExists(lock_path) && SFile::IsOpenedForWriting(lock_path));
		if(!is_locked) {
			SFile f_lck(lock_path, SFile::mWrite);
			{
				PPConfigDatabase * p_db = new PPConfigDatabase(dbpath_buf);
				ZDELETE(p_db);
			}
			f_lck.Close();
			SFile::Remove(lock_path);
		}
	}
	return ok;
}
//
// Attention: вызов PPSession::InitExtCfgDb() должен быть защищен блокировкой ExtCfgDbLock
//
int PPSession::InitExtCfgDb()
{
	int    ok = -1;	
	if(!P_ExtCfgDb) {
		static int string_history_disabled = 0; // @global (полагаемся на то, что вызов этой функции защищен блокировкой)
		int cc_shu = string_history_disabled ? 0 : CConfig.StringHistoryUsage;
		if(cc_shu < 0) {
			UserInterfaceSettings ui_cfg;
			if(ui_cfg.Restore() > 0)
				cc_shu = (ui_cfg.Flags & UserInterfaceSettings::fStringHistoryDisabled) ? 0 : 1;
			else
				cc_shu = 1;
		}
		if(!cc_shu) {
			string_history_disabled = 1;
			ok = 0;
		}
		else if(cc_shu > 0) {
			SString temp_buf;
			PPGetPath(PPPATH_WORKSPACE, temp_buf);
			temp_buf.SetLastSlash().Cat("bdbconfig");
			if(!IsDirectory(temp_buf))
				createDir(temp_buf);
			if(IsDirectory(temp_buf))
				P_ExtCfgDb = new PPConfigDatabase(temp_buf);
			if(P_ExtCfgDb)
				ok = 1;
			else 
				ok = 0;
		}
		else
			ok = 0;
	}
	return ok;
}

int PPSession::GetStringHistory(const char * pKey, const char * pSubUtf8, long flags, StringSet & rList)
{
	int    ok = 0;
	ExtCfgDbLock.Lock();
	if(InitExtCfgDb()) {
		ok = P_ExtCfgDb->GetStringHistory(pKey, pSubUtf8, flags, rList);
	}
	ExtCfgDbLock.Unlock();
	return ok;
}

int PPSession::GetStringHistoryRecent(const char * pKey, uint maxItems, StringSet & rList)
{
	int    ok = 0;
	ExtCfgDbLock.Lock();
	if(InitExtCfgDb()) {
		ok = P_ExtCfgDb->GetRecentStringHistory(pKey, maxItems, rList);
	}
	ExtCfgDbLock.Unlock();
	return ok;
}

int PPSession::AddStringHistory(const char * pKey, const char * pTextUtf8)
{
	int    ok = 0;
	ExtCfgDbLock.Lock();
	if(InitExtCfgDb()) {
		ok = P_ExtCfgDb->AddStringHistory(pKey, pTextUtf8);
	}
	ExtCfgDbLock.Unlock();
	return ok;
}

int PPSession::SaveStringHistory()
{
	int    ok = 0;
	ExtCfgDbLock.Lock();
	if(P_ExtCfgDb) {
		ok = P_ExtCfgDb->SaveStringHistory(0, 1/*use_ta*/);
	}
	ExtCfgDbLock.Unlock();
	return ok;
}

/*uint64 PPSession::GetProfileTime()
{
	return GetTLA().Prf.GetAbsTimeMicroseconds();
}*/

void PPSession::GProfileStart(const char * pFileName, long lineNo, const char * pAddedInfo)
{
	GPrf.Start(pFileName, lineNo, pAddedInfo);
}

void PPSession::GProfileFinish(const char * pFileName, long lineNo)
{
	GPrf.Finish(pFileName, lineNo);
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
		assert(pos == sstrlen(pVarSymb));
		if(si != varId || pos != sstrlen(pVarSymb))
			ok = 0;
    }
    {
		size_t pos = 0;
		(temp_buf = pVarSymb).ToLower().Cat("0xyz");
		long si = rSt.Translate(temp_buf, &pos, 0);
		assert(si == varId);
		assert(pos == sstrlen(pVarSymb));
		if(si != varId || pos != sstrlen(pVarSymb))
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
		_TestSymbVar(st, PPSYM_DUEDATE,        "DUEDATE"); // @v10.4.8
		_TestSymbVar(st, PPSYM_FGDUEDATE,      "FGDUEDATE"); // @v10.4.8
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
int TestPPObjBillParseText(); // @prototype

int HarfBuzzTestAlgs(); // ###
int HarfBuzzTestArray(); // ###
int HarfBuzzTestBitmap(); // ###
int HarfBuzzTestIter(); // ###
int HarfBuzzTestNumber(); // ###
int HarfBuzzTestUnicodeRanges(); // ###
int HarfBuzzTestMeta(); // ###
int HarfBuzzTestCommon(const char * pFileName); // ###
int HarfBuzzTestBufferSerialize(const char * pFileName); // ###
int HarfBuzzTestGPosSizeParams(const char * pFileName); // ###
int HarfBuzzTestOtGlyphName(const char * pFileName); // ###
int HarfBuzzTestOtMeta(const char * pFileName); // ###

static void InitTest()
{
#ifndef NDEBUG // {
	{
		//TestSStringPerf();
		//TestPPObjBillParseText();
	}
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
		STATIC_ASSERT(sizeof(SColor) == sizeof(RGBQUAD));
		assert(memcmp(&c, &q, sizeof(q)) == 0);
		q = (RGBQUAD)c;
		assert(memcmp(&c, &q, sizeof(q)) == 0);
	}
	STATIC_ASSERT(sizeof(KeyDownCommand) == 4);
	STATIC_ASSERT(sizeof(TView) % 4 == 0);
	STATIC_ASSERT(sizeof(TWindow) % 4 == 0);
	STATIC_ASSERT(sizeof(TDialog) % 4 == 0);
	STATIC_ASSERT(sizeof(DBFH) == 32);
	STATIC_ASSERT(sizeof(DBFF) == 32);
	STATIC_ASSERT(DBRPL_ERROR == 0);
	STATIC_ASSERT(sizeof(DBRowId) == 32);
	//
	// Так как множество классов наследуются от DBTable важно, чтобы
	// размер DBTable был кратен 32 (для выравнивания по кэш-линии).
	//
	STATIC_ASSERT(sizeof(DBTable) % 32 == 0);
	//
	// Записи системного журнала и резервной
	// таблицы системного журнала должны быть эквивалентны.
	//
	STATIC_ASSERT(sizeof(SysJournalTbl::Rec) == sizeof(SjRsrvTbl::Rec));
	//
	// Размер внутренней структуры электронного адреса должен быть равен 16 байтам и
	// поле Addr таблицы EAddrTbl должен быть бинарно эквивалентен PPEAddr.
	//
	STATIC_ASSERT(sizeof(PPEAddr) == 16);
	STATIC_ASSERT(sizeof(PPEAddr) == sizeof(static_cast<const EAddrTbl::Rec *>(0)->Addr));
	STATIC_ASSERT(sizeof(PPDynanicObjItem) == sizeof(Reference2Tbl::Rec));
	STATIC_ASSERT(sizeof(PPStaffEntry) == sizeof(Reference2Tbl::Rec));
	STATIC_ASSERT(sizeof(PPAccount) == sizeof(Reference2Tbl::Rec));
	{
        PPAccount::_A_ a1;
        PPAccount::_A_ a2;
        a1.Ac = 20;
        a1.Sb = 5;
        a2.Ac = 20;
        a2.Sb = 0;
        assert(*reinterpret_cast<const long *>(&a1) > *reinterpret_cast<const long *>(&a2));
	}
	STATIC_ASSERT(sizeof(PPBankAccount) == sizeof(RegisterTbl::Rec));
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
	STATIC_ASSERT(sizeof(PPTimeSeries) == sizeof(Reference2Tbl::Rec)); // @v10.7.5
	STATIC_ASSERT(sizeof(PPTssModel) == sizeof(Reference2Tbl::Rec)); // @v10.7.5
	STATIC_ASSERT(sizeof(PPBarcodePrinter_)-sizeof(SString) == sizeof(Reference_Tbl::Rec));
	STATIC_ASSERT(sizeof(PPBarcodePrinter2)-sizeof(SString) == sizeof(Reference2Tbl::Rec));
	STATIC_ASSERT(sizeof(PPInternetAccount_)-sizeof(SString) == sizeof(Reference_Tbl::Rec));
	STATIC_ASSERT(sizeof(PPInternetAccount2)-sizeof(SString) == sizeof(Reference2Tbl::Rec));
	STATIC_ASSERT(sizeof(PPAlbatrosCfgHdr) == offsetof(PropertyTbl::Rec, VT));
	STATIC_ASSERT(sizeof(PersonCore::RelationRecord) == sizeof(ObjAssocTbl::Rec));
	// @v10.9.2 (PPFreight is expanded) assert(sizeof(PPFreight) == offsetof(PropertyTbl::Rec, VT));
	STATIC_ASSERT(sizeof(PPRFIDDevice) == sizeof(Reference2Tbl::Rec));
	STATIC_ASSERT(sizeof(PPSmsAccount) == sizeof(Reference2Tbl::Rec));
	STATIC_ASSERT(sizeof(PPUhttStore) == sizeof(Reference2Tbl::Rec));
	STATIC_ASSERT(sizeof(PPGeoTrackingMode) == 8);
	STATIC_ASSERT(sizeof(PPCycleFilt) == 4);
	STATIC_ASSERT(sizeof(PPBill::Agreement) == offsetof(PropertyTbl::Rec, VT)); // @v10.1.12
	//
	// Гарантируем, что функции семейства PPSetError всегда возвращают 0
	// БОльшая часть кода закладывается на этот факт.
	//
	assert(PPSetErrorNoMem() == 0);
	assert(PPSetErrorSLib() == 0);
	assert(PPSetErrorDB() == 0);
	assert(PPSetError(0) == 0);
	assert(PPSetError(0, "") == 0);
	assert(PPSetError(0, 0L) == 0);
	//
	assert(_TestSymbols());
	{
		//
		// Убедимся, что функции TView::GetId() и TView::TestId() адекватно
		// работают с нулевым указателем this.
		//
		TView * p_zero_view = 0;
		assert(p_zero_view->GetId() == 0);
		assert(p_zero_view->TestId(1) == 0);
	}
	{
		//
		// Возможность системы получить стоп-событие, созданное в SlSession, критична!
		//
		SString evnam;
		Evnt test_stop_event(SLS.GetStopEventName(evnam), Evnt::modeOpen);
		assert(test_stop_event.IsValid());
	}
	// @v11.0.4 {
#if _MSC_VER >= 1900
	{
		assert(HarfBuzzTestAlgs());
		assert(HarfBuzzTestArray());
		assert(HarfBuzzTestBitmap());
		assert(HarfBuzzTestIter());
		assert(HarfBuzzTestNumber());
		assert(HarfBuzzTestUnicodeRanges());
		assert(HarfBuzzTestMeta());
	}
#endif
	// } @v11.0.4 
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

int PPCallHelp(void * hWnd, uint cmd, uint ctx); // @prototype(pptvutil.cpp)
int ExecDateCalendar(void * hParentWnd, LDATE * pDate); // @prototype(calendar.cpp)
static int PPLoadStringFunc(const char * pSignature, SString & rBuf) { return PPLoadString(pSignature, rBuf); }
static int PPExpandStringFunc(SString & rBuf, int ctransf) { return PPExpandString(rBuf, ctransf); }

static int PPQueryPathFunc(const char * pSignature, SString & rBuf)
{
    rBuf.Z();
	static const SIntToSymbTabEntry path_symb_list[] = {
		{ PPPATH_BIN, "bin" }, { PPPATH_LOCAL, "local" }, { PPPATH_TEMP, "temp" },         { PPPATH_IN, "in" },
		{ PPPATH_OUT, "out" }, { PPPATH_LOG, "log" },     { PPPATH_TESTROOT, "testroot" }, { PPPATH_WORKSPACE, "workspace" },
	};
	int    path_id = SIntToSymbTab_GetId(path_symb_list, SIZEOFARRAY(path_symb_list), pSignature);
	return path_id ? PPGetPath(path_id, rBuf) : 0;
}

static int PPGetGlobalSecureConfig(SGlobalSecureConfig * pCfg)
{
	if(pCfg) {
		PPIniFile ini_file;
		SString temp_buf;
		ini_file.Get(PPINISECT_GLOBALSECURE, PPINIPARAM_CAPATH, temp_buf.Z());
		pCfg->CaPath = temp_buf.Strip();
		ini_file.Get(PPINISECT_GLOBALSECURE, PPINIPARAM_CAFILE, temp_buf.Z());
		pCfg->CaFile = temp_buf.Strip();
	}
	return 1;
}

static int PPGetDefaultEncrKey(SString & rBuf)
{
    PPVersionInfo vi;
    return vi.GetDefaultEncrKey(rBuf);
}

/*static*/int FASTCALL PPSession::GetStartUpOption(int o, SString & rArgBuf)
{
	static const char * p_cmdl_symbols = "?,CASH,EXP,IMP,IN,OUT,BATCH,SYNCPUT,SYNCGET,DB,BACKUP,BILLCASH,PRC,"
		"TSESS,GOODSINFO,VERHIST,RECOVERTRANSFER,CONVERTRBCBNK,NOLOGIN,PPOS,EXPORTDIALOGS,SELFBUILD,SARTRTEST,"
		"AUTOTRANSLATE,CONVERTCIPHER,PPINISUBST,UHTTGOODSTOGITHUBEXPORT,UILANG";
	int    ok = 0;
	//ENTER_CRITICAL_SECTION
	/*if(!p_cmdl_symbols && o >= 0) {
		if(!p_cmdl_symbols) {
			p_cmdl_symbols = new SString;
			PPLoadString(PPSTR_SYMB, PPSSYM_CMDLINEOP, *p_cmdl_symbols);
		}
	}*/
	rArgBuf.Z();
	SString sym;
	if(p_cmdl_symbols) {
		StringSet ss(',', p_cmdl_symbols);
		for(int i = 1; !ok && i < _argc; i++) {
			const char * arg = _argv[i];
			if(arg[0] == '/' || arg[0] == '-') {
				int   k = 0;
				for(uint pos = 0; !ok && k <= o && ss.get(&pos, sym); k++) {
					if(k == o) {
						arg++;
						size_t len = sym.Len();
						if(strnicmp(arg, sym, len) == 0) {
							if(arg[len] == ':')
							   	len++;
							(rArgBuf = arg+len).Strip();
							ok = 1;
						}
					}
				}
			}
		}
	}
	/*if(o < 0) {
		ZDELETE(p_cmdl_symbols);
	}*/
	//LEAVE_CRITICAL_SECTION
	return ok;
}

int PPSession::Init(long flags, HINSTANCE hInst)
{
	int    ok = 1;
	SString temp_buf;
	signal(SIGFPE, reinterpret_cast<void (*)(int)>(FpeCatcher));
	SLS.Init(0, hInst);
	{
		PPVersionInfo vi = GetVersionInfo();
		//vi.GetProductName(temp_buf);
		vi.GetTextAttrib(vi.taiProductName, temp_buf);
		SLS.SetAppName(temp_buf);
		SetExtFlag(ECF_OPENSOURCE, vi.GetFlags() & PapyrusPrivateBlock::fOpenSource);
	}
	SLS.InitWSA();
	{
		typedef VOID (WINAPI * DISABLEPROCESSWINDOWSGHOSTING)(VOID);
		SDynLibrary lib_user32("USER32.DLL");
		if(lib_user32.IsValid()) {
			DISABLEPROCESSWINDOWSGHOSTING proc_DisableProcessWindowsGhosting = reinterpret_cast<DISABLEPROCESSWINDOWSGHOSTING>(lib_user32.GetProcAddr("DisableProcessWindowsGhosting"));
			if(proc_DisableProcessWindowsGhosting)
				proc_DisableProcessWindowsGhosting();
		}
	}
	{
		ENTER_CRITICAL_SECTION
		getExecPath(BinPath.Z()).SetLastSlash();
		LEAVE_CRITICAL_SECTION
	}
	RegisterSTAcct();
	PPDbqFuncPool::Register();
	{
		PPIniFile ini_file(0, 0, 0, 1); // @v10.3.11 useIniBuf=1
		if(GetStartUpOption(cmdlUiLang, temp_buf)) { // @v10.4.4
			const int slang = RecognizeLinguaSymb(temp_buf, 0);
			if(slang > 0)
                SLS.SetUiLanguageId(slang, 0);
		}
		else if(ini_file.GetParam("config", "uilanguage", temp_buf) > 0 && temp_buf.NotEmptyS()) {
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
            epb.F_QueryPath = PPQueryPathFunc; // @v9.8.7
            SLS.SetExtraProcBlock(&epb);
			//SLS.SetLoadStringFunc(PPLoadStringFunc);
			//SLS.SetExpandStringFunc(PPExpandStringFunc); // @v9.0.11
			//SLS.SetCallHelpFunc(PPCallHelp);
			//SLS.SetGlobalSecureConfigFunc(PPGetGlobalSecureConfig);
		}
		{
			//
			// Флаг устанавливается по умолчанию. Параметром DETECTDBTEXISTBYOPEN его можно отменить
			//
			DbSession::Config dbcfg = DBS.GetConfig();
			int    iv = 0;
			dbcfg.Flags |= DbSession::fDetectExistByOpen;
			if(ini_file.GetInt(PPINISECT_CONFIG, PPINIPARAM_DETECTDBTEXISTBYOPEN, &iv) > 0) {
				if(iv == 0)
					dbcfg.Flags &= ~DbSession::fDetectExistByOpen;
				else if(iv == 100)
					SetExtFlag(ECF_DETECTCRDBTEXISTBYOPEN, 1);
			}
			if(ini_file.Get(PPINISECT_CONFIG, PPINIPARAM_BTRNWLOCK, temp_buf) > 0) {
				if(temp_buf.IsEqiAscii("disable")) {
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
			DBS.SetConfig(&dbcfg);
			{
				int    max_log_file_size = 0;
				if(ini_file.GetInt(PPINISECT_CONFIG, PPINIPARAM_MAXLOGFILESIZE, &max_log_file_size) > 0) {
					if(checkirange(max_log_file_size, 1, SKILOBYTE(1024))) {
						MaxLogFileSize = max_log_file_size;
					}
				}
			}
			if(flags & PPSession::fInitPaths) {
				MemLeakTracer mlt;
				SString path, root_path;
				PPGetPath(PPPATH_SYSROOT, root_path);
				{
					path = (ini_file.Get(PPINISECT_PATH, PPINIPARAM_TEMP, temp_buf.Z()) > 0) ? temp_buf.cptr() : 0;
					Helper_SetPath(PPPATH_TEMP, path);
				}
				{
					path = (ini_file.Get(PPINISECT_PATH, PPINIPARAM_LOG, temp_buf) > 0) ? temp_buf.cptr() : 0;
					if(!path.NotEmptyS()) {
						PPIniFile::GetParamSymb(PPINIPARAM_LOG, temp_buf.Z());
						(path = root_path).SetLastSlash().Cat(temp_buf);
					}
					if(!IsDirectory(path) && !createDir(path))
						path = root_path.RmvLastSlash();
					if(Helper_SetPath(PPPATH_LOG, path))
						SLS.SetLogPath(path);
				}
				{
					path = (ini_file.Get(PPINISECT_PATH, PPINIPARAM_PACK, temp_buf.Z()) > 0) ? temp_buf.cptr() : 0;
					if(!path.NotEmptyS())
						(path = root_path).SetLastSlash().Cat("PACK");
					Helper_SetPath(PPPATH_PACK, path);
				}
				{
					path = (ini_file.Get(PPINISECT_PATH, PPINIPARAM_SPII, temp_buf.Z()) > 0) ? temp_buf.cptr() : 0;
					Helper_SetPath(PPPATH_SPII, path);
				}
				{
					path = (ini_file.Get(PPINISECT_PATH, PPINIPARAM_SARTREDB, temp_buf.Z()) > 0) ? temp_buf.cptr() : 0;
					Helper_SetPath(PPPATH_SARTREDB, path);
				}
				{
					path = (ini_file.Get(PPINISECT_PATH, PPINIPARAM_WORKSPACE, temp_buf.Z()) > 0) ? temp_buf.cptr() : 0;
					if(!path.NotEmptyS())
						(path = root_path).SetLastSlash().Cat("WORKSPACE");
					Helper_SetPath(PPPATH_WORKSPACE, path);
				}
				{
					path = (ini_file.Get(PPINISECT_SYSTEM, PPINIPARAM_REPORTDATAPATH, temp_buf.Z()) > 0) ? temp_buf.cptr() : 0;
					if(!path.NotEmptyS()) {
						path = (ini_file.Get(PPINISECT_CONFIG, PPINIPARAM_REPORTDATAPATH, temp_buf.Z()) > 0) ? temp_buf.cptr() : 0;
						if(!path.NotEmptyS())
							PPGetPath(PPPATH_TEMP, path);
					}
					Helper_SetPath(PPPATH_REPORTDATA, path);
				}
				LoadDriveMapping(&ini_file);
			}
		}
	}
	if(!(flags & fDenyLogQueue)) { // Для DLL-режима не используем поток журналов (какие-то траблы с потоками - надо разбираться)
		P_LogQueue = new PPLogMsgQueue;
		if(P_LogQueue) {
			PPLogMsgSession * p_sess = new PPLogMsgSession(P_LogQueue);
			p_sess->Start(0);
		}
	}
	SetExtFlag(ECF_DBDICTDL600, 1);
	if(CheckExtFlag(ECF_DBDICTDL600))
		DbDictionary::SetCreateInstanceProc(DbDict_DL600::CreateInstance);
	// @v11.1.2 {
	{
		StringSet host_list;
		host_list.add("uhtt.ru");
		CheckRemoteHosts(host_list); 
	}
	// } @v11.1.2
	InitTest();
	CATCHZOK
	return ok;
}

void FASTCALL PPSession::MoveCommonPathOnInitThread(long pathID)
{
	SString temp_buf;
	if(CommonPaths.GetPath(pathID, 0, temp_buf) > 0) {
		SetPath(pathID, temp_buf, 0, 1);
		if(pathID == PPPATH_LOG) // @v10.7.9 @fix PPPATH_TEMP-->PPPATH_LOG
			SLS.SetLogPath(temp_buf);
	}
}

int PPSession::InitThread(const PPThread * pThread)
{
	PPThreadLocalArea * p_tla = new PPThreadLocalArea;
	ENTER_CRITICAL_SECTION
	TlsSetValue(TlsIdx, p_tla);
	static const long common_path_id_list[] = { PPPATH_BIN, PPPATH_LOG, PPPATH_TEMP, PPPATH_SPII, PPPATH_SARTREDB, PPPATH_REPORTDATA, PPPATH_WORKSPACE };
	for(uint i = 0; i < SIZEOFARRAY(common_path_id_list); i++)
		MoveCommonPathOnInitThread(common_path_id_list[i]);
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

void PPSession::ReleaseThread()
{
	PPThreadLocalArea * p_tla = static_cast<PPThreadLocalArea *>(TlsGetValue(TlsIdx));
	if(p_tla) {
		ThreadList.Remove(p_tla->GetThreadID());
		delete p_tla;
		TlsSetValue(TlsIdx, 0);
	}
	else {
		assert(0);
	}
}
//
// См. примечание к определению функций PP.H
//
#define MAX_GETTLA_TRY 5

PPThreadLocalArea & PPSession::GetTLA() { return *static_cast<PPThreadLocalArea *>(SGetTls(TlsIdx)); }
const PPThreadLocalArea & PPSession::GetConstTLA() const { return *static_cast<PPThreadLocalArea *>(SGetTls(TlsIdx)); }
int PPSession::GetThreadInfoList(int type, TSCollection <PPThread::Info> & rList) { return ThreadList.GetInfoList(type, rList); }
int PPSession::GetThreadInfo(ThreadID tId, PPThread::Info & rInfo) { return ThreadList.GetInfo(tId, rInfo); }
int FASTCALL PPSession::PushLogMsgToQueue(const PPLogMsgItem & rItem) { return P_LogQueue ? P_LogQueue->Push(rItem) : -1; }

int PPSession::IsThreadInteractive() const
{
	if(/*CS_SERVER*/CheckExtFlag(ECF_SYSSERVICE))
		return 0;
	else {
		const PPThreadLocalArea & r_tla = GetConstTLA();
		if(r_tla.IsConsistent())
			return (r_tla.State & PPThreadLocalArea::stNonInteractive) ? 0 : 1;
		else
			return 0;
	}
}

int PPSession::SetThreadNotification(int type, const void * pData)
{
	int    ok = -1;
	if(type == stntMessage)
		ok = ThreadList.SetMessage(GetConstTLA().GetThreadID(), 1, static_cast<const char *>(pData));
	else if(type == stntText)
		ok = ThreadList.SetMessage(GetConstTLA().GetThreadID(), 0, static_cast<const char *>(pData));
	return ok;
}

void PPSession::LogLocStk()
{
	SString out_buf;
	ThreadList.LocStkToStr(out_buf);
	PPLogMessage(PPFILNAM_DEBUG_LOG, out_buf, LOGMSGF_TIME);
}

int PPSession::LockingDllServer(int cmd)
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
//
//
//
int PPSession::Helper_SetPath(int pathId, SString & rPath)
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
int PPSession::LogAction(PPID action, PPID obj, PPID id, long extData, int use_ta)
{
	int    ok = -1;
	if(action) {
		SysJournal * p_sj = GetTLA().P_SysJ;
		ok = p_sj ? p_sj->LogEvent(action, obj, id, extData, use_ta) : -1;
	}
	return ok;
}

GtaJournalCore * PPSession::GetGtaJ()
{
	PPThreadLocalArea & r_tla = GetTLA();
	SETIFZ(r_tla.P_GtaJ, new GtaJournalCore);
	return r_tla.P_GtaJ;
}
//
//
//
int PPSession::MakeMachineID(MACAddr * pMachineID)
{
	int    ok = -1;
	MACAddr addr;
	if(GetFirstMACAddr(&addr)) {
		ok = 2;
	}
	else {
		char buf[32];
		// @v11.1.1 IdeaRandMem(buf, sizeof(buf));
		SObfuscateBuffer(buf, sizeof(buf)); // @v11.1.1 
		memcpy(addr.Addr, buf+3, sizeof(addr.Addr));
		ok = 1;
	}
	ASSIGN_PTR(pMachineID, addr);
	return ok;
}

int PPSession::GetMachineID(MACAddr * pMachineID, int forceUpdate)
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
			if(*reinterpret_cast<const long *>(buf) == signature) {
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
				const TCHAR * p_ucfn = SUcSwitch(fname);
				DWORD fattr = GetFileAttributes(p_ucfn);
				if(fattr != 0xffffffff)
					::SetFileAttributes(p_ucfn, fattr | FILE_ATTRIBUTE_HIDDEN);
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

int LogTerminalSessInfo(ulong processID, ulong termSessID, const char * pAddMsgString)
{
	/* @v7.9.9 Пользы не получили, а журнал забивается //
	SString msg_buf, buf;
	PPLoadText(PPTXT_TERMINALSESSINFO, buf);
	msg_buf.Printf(buf.cptr(), pAddMsgString, (long)processID, (long)termSessID);
	PPLogMessage(PPFILNAM_INFO_LOG, msg_buf, LOGMSGF_TIME|LOGMSGF_USER|LOGMSGF_DBINFO);
	*/
	return 1;
}

int PPSession::CheckLicense(MACAddr * pMachineID, int * pIsDemo)
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
			if(r_item.ObjID != 1) // Серверные сессии не учитываем при подсчете занятых лицензий
				ma_list.addUnique(ma);
			if(ma.Cmp(machine_id) == 0 && (cur_term_sess_id == 0 || cur_term_sess_id == term_sess_id))
				this_machine_logged = 1;
		}
		ok = (this_machine_logged || max_user_count > ma_list.getCountI()) ? 1 : -1;
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
	const _E * p_e1 = static_cast<const _E *>(i1);
	const _E * p_e2 = static_cast<const _E *>(i2);
	if((r = p_e1->MchnID.Cmp(p_e2->MchnID)) > 0)
		return 1;
	else if(r < 0)
		return -1;
	else
		return cmp_long(p_e1->TerminalSessID, p_e2->TerminalSessID);
}

int PPSession::GetUsedLicCount(int32 * pUsedLicCount)
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

static int _dbOpenException(const char * pFileName, int btrErr)
{
	BtrError = NZOR(btrErr, BE_FILNOPEN);
	SString temp_buf(pFileName);
	PPError(PPERR_DBENGINE, temp_buf.Transf(CTRANSF_OUTER_TO_INNER));
	CALLPTRMEMB(APPL, CloseAllBrowsers());
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
int PPSession::OpenDictionary2(DbLoginBlock * pBlk, long flags)
{
	int    ok = 1;
	int    r;
	SString data_path, temp_path, temp_buf;
	PPVersionInfo ver_inf(0);
	const SVerT this_ver   = ver_inf.GetVersion();
	const SVerT this_db_min_ver = ver_inf.GetVersion(1);
	pBlk->GetAttr(DbLoginBlock::attrDbPath, data_path);
	PPVerHistory verh;
	PPVerHistory::Info vh_info;
	pBlk->GetAttr(DbLoginBlock::attrServerType, temp_buf);
	const SqlServerType server_type = GetSqlServerTypeBySymb(temp_buf);
	//
	// Проверяем доступность каталога базы данных
	//
	THROW_PP_S(server_type == sqlstMySQL || ::access(data_path, 0) == 0, PPERR_DBDIRNFOUND, data_path); // @v10.9.3 server_type == sqlstMySQL
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
		if(server_type == sqlstORA) {
			// @todo SOraDbProvider должен инициализировать DbPathID
			THROW_MEM(p_db = new SOraDbProvider(data_path));
		}
		else if(server_type == sqlstMySQL) {
#if (_MSC_VER >= 1900)
			THROW_MEM(p_db = new SMySqlDbProvider());
#endif
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

int PPSession::SetupConfigByOps()
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
		if(oneof3(op_rec.OpTypeID, PPOPT_DRAFTRECEIPT, PPOPT_DRAFTEXPEND, PPOPT_DRAFTQUOTREQ)) // @v10.5.7 PPOPT_DRAFTQUOTREQ
			cc.Flags |= CCFLG_USEDRAFTBILL;
	}
	if(missingnoupdrestopflag)
		PPMessage(mfInfo, PPINF_MISSINGNOUPDRESTOPFLAG);
	return ok;
}

int PPSession::FetchConfig(PPID obj, PPID objID, PPConfig * pCfg)
{
	int    ok = 1, r;
	PPConfig tmp, global;
	Reference * p_ref = GetTLA().P_Ref;
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
	tmp.UserID = (obj == PPOBJ_USR) ? objID : 0;
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

int PPSession::FetchAlbatrosConfig(PPAlbatrossConfig * pCfg)
{
	PPObjGlobalUserAcc gua_obj; // @v10.6.3
	return gua_obj.FetchAlbatossConfig(pCfg); // @v10.6.3
	/* @v10.6.3
	int    ok = 1;
	ENTER_CRITICAL_SECTION
	if(pCfg) {
		if(!P_AlbatrosCfg) {
			P_AlbatrosCfg = new PPAlbatrossConfig;
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
	*/
}

int PPSession::CheckSystemAccount(DbLoginBlock * pDlb, PPSecur * pSecur)
{
	int    ok = -1;
	TCHAR  domain_user[64];
	DWORD  duser_len = sizeof(domain_user);
	memzero(domain_user, sizeof(domain_user));
	THROW(OpenDictionary2(pDlb, odfDontInitSync)); // @v9.4.9 odfDontInitSync
	if(::GetUserName(domain_user, &duser_len)) { // @unicodeproblem
		PPID   user_id = 0;
		Reference ref;
		SString user_name_buf(SUcSwitch(domain_user));
		if(ref.SearchName(PPOBJ_USR, &user_id, user_name_buf) > 0) {
			char   pw[32];
			SString domain;
			memzero(pw, sizeof(pw));
			PPIniFile ini_file;
			ini_file.Get(PPINISECT_SYSTEM, PPINIPARAM_DOMAINNAME, domain);
			const PPSecur & r_secur = *(PPSecur*)&ref.data;
			Reference::GetPassword(&r_secur, pw, sizeof(pw));
			if(SCheckSystemCredentials(domain, user_name_buf, pw)) {
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

int PPSession::SetExtFlagByIniIntParam(PPIniFile & rIniFile, uint sect, uint param, long extFlags, int reqValue)
{
	int    ok = 0;
	int    iv = 0;
	if(rIniFile.GetInt(sect, param, &iv) > 0 && (iv == reqValue || (reqValue == 999 && iv != 0))) {
		SetExtFlag(extFlags, 1);
		ok = 1;
	}
	return ok;
}

class PPDbDispatchSession : public PPThread {
public:
	PPDbDispatchSession(long dbPathID, const char * pDbSymb) : PPThread(PPThread::kDbDispatcher, pDbSymb, 0),
		DbPathID(dbPathID), DbSymb(pDbSymb)
	{
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
		THROW(DS.Login(DbSymb, PPSession::P_JobLogin, secret, PPSession::loginfSkipLicChecking));
		memzero(secret, sizeof(secret));
		{
			PPLoadText(PPTXT_LOG_DISPTHRCROK, msg_buf);
			msg_buf.Space().CatQStr(DbSymb);
			PPLogMessage(PPFILNAM_INFO_LOG, msg_buf, LOGMSGF_TIME);
		}
		for(int stop = 0; !stop;) {
			timer.Set(getcurdatetime_().addsec(5), 0);
			uint   h_count = 0;
			HANDLE h_list[32];
			h_list[h_count++] = timer;
			h_list[h_count++] = stop_event;
			uint   r = WaitForMultipleObjects(h_count, h_list, 0, INFINITE);
			if(r == WAIT_OBJECT_0 + 0) { // timer
				DS.DirtyDbCache(DbPathID, 0);
				// @v10.2.4 {
				{
					PPAdviseList adv_list;
					if(DS.GetAdviseList(PPAdviseBlock::evQuartz, 0, adv_list) > 0) {
						PPNotifyEvent ev;
						PPAdviseBlock adv_blk;
						for(uint j = 0; adv_list.Enum(&j, &adv_blk);) {
							if(adv_blk.Proc) {
								ev.Clear();
								ev.ObjID   = -1;
								adv_blk.Proc(PPAdviseBlock::evQuartz, &ev, adv_blk.ProcExtPtr);
							}
						}
					}
				}
				// } @v10.2.4
			}
			else if(r == WAIT_OBJECT_0 + 2) { // stop event
				stop = 1; // quit loop
			}
			else if(r == WAIT_FAILED) {
				// error
			}
		}
		CATCH
			PPLoadText(PPTXT_LOG_DISPTHRCRERR, msg_buf);
			PPGetLastErrorMessage(1, temp_buf);
			msg_buf.Space().CatQStr(DbSymb).CatDiv(':', 2).Cat(temp_buf);
			PPLogMessage(PPFILNAM_ERR_LOG, msg_buf, LOGMSGF_TIME);
		ENDCATCH
		DS.Logout();
		memzero(secret, sizeof(secret));
	}
	long   DbPathID;
	SString DbSymb;
};

class PPAdviseEventCollectorSjSession : public PPThread {
public:
	PPAdviseEventCollectorSjSession(const DbLoginBlock & rLB, const PPPhoneServicePacket * pPhnSvcPack,
		PPMqbClient::InitParam * pMqbParam, long cycleMs) :
		PPThread(PPThread::kEventCollector, 0, 0), /*CycleMs((cycleMs > 0) ? cycleMs : 29989),*/ /*CyclePhnSvcMs(1500),*/ LB(rLB), P_Sj(0), State(0)
	{
		RVALUEPTR(StartUp_PhnSvcPack, pPhnSvcPack);
		RVALUEPTR(StartUp_MqbParam, pMqbParam); // @v10.5.7
	}
private:
	virtual void Shutdown()
	{
		//
		// То же, что и PPThread::Shutdown() но без DS.Logout()
		//
		DS.ReleaseThread();
		DBS.ReleaseThread();
		SlThread::Shutdown();
	}
	AsteriskAmiClient * CreatePhnSvcClient(AsteriskAmiClient * pOldCli)
	{
		ZDELETE(pOldCli);
		AsteriskAmiClient * p_phnsvc_cli = 0;
		if(StartUp_PhnSvcPack.Rec.ID) {
			SString temp_buf;
			SString addr_buf, user_buf, secret_buf;
			StartUp_PhnSvcPack.GetExField(PHNSVCEXSTR_ADDR, addr_buf);
			StartUp_PhnSvcPack.GetExField(PHNSVCEXSTR_PORT, temp_buf);
			int    port = temp_buf.ToLong();
			StartUp_PhnSvcPack.GetExField(PHNSVCEXSTR_USER, user_buf);
			StartUp_PhnSvcPack.GetPassword(secret_buf);
			p_phnsvc_cli = new AsteriskAmiClient(/*AsteriskAmiClient::fDoLog*/0);
			if(p_phnsvc_cli && p_phnsvc_cli->Connect(addr_buf, port)) {
				if(p_phnsvc_cli->Login(user_buf, secret_buf)) {
					PhnSvcLocalUpChannelSymb = StartUp_PhnSvcPack.LocalChannelSymb;
					PhnSvcLocalScanChannelSymb = StartUp_PhnSvcPack.ScanChannelSymb;
				}
				else {
					PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR|LOGMSGF_TIME|LOGMSGF_COMP|LOGMSGF_USER);
					ZDELETE(p_phnsvc_cli);
				}
			}
			else {
				PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR|LOGMSGF_TIME|LOGMSGF_COMP|LOGMSGF_USER);
				ZDELETE(p_phnsvc_cli);
			}
			secret_buf.Obfuscate();
		}
		return p_phnsvc_cli;
	}
	virtual void Run()
	{
		const int   do_debug_log = 0; // @debug
		const long  pollperiod_phnsvc = 1000;
		const long  pollperiod_sj = 3000;
		const long  pollperiod_mqc = 500;
		EvPollTiming pt_sj(pollperiod_sj, false);
		EvPollTiming pt_phnsvc(pollperiod_phnsvc, false);
		EvPollTiming pt_mqc(pollperiod_mqc, false);
		EvPollTiming pt_purge(3600000, true); // этот тайминг не надо исполнять при запуске. Потому registerImmediate = 1
		const int  use_sj_scan_alg2 = 0;
		SString msg_buf, temp_buf;
		DBRowId last_sj_rowid; // @v10.4.4
		PPAdviseEventVector temp_list;
		PhnSvcChannelStatusPool chnl_status_list;
		PhnSvcChannelStatus chnl_status;
		PPMqbClient::Envelope mqb_envelop;
		Evnt   stop_event(SLS.GetStopEventName(temp_buf), Evnt::modeOpen);
		BExtQuery * p_q = 0;
		//PPMqbClient * p_mqb_cli = CreateMqbClient(); // @v10.5.7
		PPMqbClient * p_mqb_cli = PPMqbClient::CreateInstance(StartUp_MqbParam); // @v11.0.9
		AsteriskAmiClient * p_phnsvc_cli = CreatePhnSvcClient(0);
		LDATETIME sj_since;
		const long __cycle_hs = (p_mqb_cli ? 37 : (p_phnsvc_cli ? 83 : 293)); // Период таймера в сотых долях секунды (37)
		// @v10.6.0 {
		int    queue_stat_flags_inited = 0;
		SETFLAG(State, stPhnSvc, p_phnsvc_cli);
		SETFLAG(State, stMqb, p_mqb_cli);
		// } @v10.6.0
		THROW(DS.OpenDictionary2(&LB, PPSession::odfDontInitSync));
		THROW_MEM(P_Sj = new SysJournal);
		if(use_sj_scan_alg2) {
			SysJournalTbl::Key0 sjk0;
			sjk0.Dt = MAXDATE;
			sjk0.Tm = MAXTIME;
			if(P_Sj->search(0, &sjk0, spLast))
				P_Sj->getPosition(&last_sj_rowid);
		}
		sj_since = getcurdatetime_();
		for(int stop = 0; !stop;) {
			uint   h_count = 0;
			HANDLE h_list[32];
			h_list[h_count++] = stop_event;
			//
			STimer __timer;  // Таймер для отмера времени до следующего опроса источников событий
			__timer.Set(getcurdatetime_().addhs(__cycle_hs), 0);
			h_list[h_count++] = __timer;
			uint   r = ::WaitForMultipleObjects(h_count, h_list, 0, /*CycleMs*//*INFINITE*/60000);
			switch(r) {
				case (WAIT_OBJECT_0 + 0): // stop event
					if(do_debug_log) {
						PPLogMessage(PPFILNAM_DEBUG_AEQ_LOG, "StopEvent", LOGMSGF_DBINFO|LOGMSGF_TIME|LOGMSGF_USER);
					}
					stop = 1; // quit loop
					break;
				case WAIT_TIMEOUT:
					// Если по каким-то причинам сработал таймаут, то перезаряжаем цикл по-новой
					// Предполагается, что это событие крайне маловероятно!
					if(do_debug_log) {
						PPLogMessage(PPFILNAM_DEBUG_AEQ_LOG, "TimeOut", LOGMSGF_DBINFO|LOGMSGF_TIME|LOGMSGF_USER);
					}
					break;
				case (WAIT_OBJECT_0 + 1): // __timer event
					{
						temp_list.Clear();
						PPAdviseEventQueue * p_queue = 0;
						PPAdviseEventQueue::Stat addendum_queue_stat;
						if(pt_purge.IsTime()) {
							if(SETIFZ(p_queue, DS.GetAdviseEventQueue(0))) {
								p_queue->Purge();
								pt_purge.Register();
							}
						}
						if(pt_sj.IsTime()) {
							if(SETIFZ(p_queue, DS.GetAdviseEventQueue(0))) {
								// @v10.6.0 {
								// Мы вынуждены устанавливать флаги статистики очереди в рабочем цикле из-за
								// того, что в момент старта потока очередь может еще и не существовать.
								if(!queue_stat_flags_inited) {
									if(State & (stMqb|stPhnSvc)) {
										PPAdviseEventQueue * p_queue = DS.GetAdviseEventQueue(0);
										if(p_queue) {
											uint qsf = 0;
											if(State & stMqb)
												qsf |= PPAdviseEventQueue::Stat::stMqbCliInit;
											if(State & stPhnSvc)
												qsf |= PPAdviseEventQueue::Stat::stPhnSvcInit;
											p_queue->SetStatFlag(qsf);
										}
									}
									queue_stat_flags_inited = 1;
								}
								// } @v10.6.0
								LDATETIME last_ev_dtm = ZERODATETIME;
								SysJournalTbl::Key0 k0, k0_;
								if(use_sj_scan_alg2) {
									if(!last_sj_rowid) {
										k0.Dt = MAXDATE;
										k0.Tm = MAXTIME;
										if(P_Sj->search(0, &k0, spLast)) {
											PPAdviseEvent ev;
											ev = P_Sj->data;
											temp_list.insert(&ev);
											last_ev_dtm.Set(P_Sj->data.Dt, P_Sj->data.Tm);
											P_Sj->getPosition(&last_sj_rowid);
										}
									}
									else {
										while(P_Sj->search(0, &k0, spNext)) {
											PPAdviseEvent ev;
											ev = P_Sj->data;
											temp_list.insert(&ev);
											last_ev_dtm.Set(P_Sj->data.Dt, P_Sj->data.Tm);
											P_Sj->getPosition(&last_sj_rowid);
										}
									}
								}
								else {
									k0.Dt = sj_since.d;
									k0.Tm = sj_since.t;
									k0_ = k0;
									if(p_q)
										p_q->resetEof();
									else if(P_Sj->search(&k0_, spGt)) {
										p_q = new BExtQuery(P_Sj, 0);
										p_q->selectAll().where(P_Sj->Dt >= sj_since.d);
										p_q->initIteration(0, &k0, spGt);
									}
									if(p_q) {
										while(p_q->nextIteration() > 0) {
											SysJournalTbl::Rec rec;
											P_Sj->copyBufTo(&rec);
											if(cmp(sj_since, rec.Dt, rec.Tm) < 0) {
												PPAdviseEvent ev;
												ev = rec;
												temp_list.insert(&ev);
												last_ev_dtm.Set(rec.Dt, rec.Tm);
												addendum_queue_stat.SjMsgCount++;
											}
										}
									}
									if(!BTROKORNFOUND) {
										PPSetErrorDB();
										PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR|LOGMSGF_DBINFO|LOGMSGF_TIME|LOGMSGF_USER);
										PPLogMessage(PPFILNAM_INFO_LOG, PPSTR_TEXT, PPTXT_ASYNCEVQUEUESJFAULT, LOGMSGF_DBINFO|LOGMSGF_TIME|LOGMSGF_USER);
										BExtQuery::ZDelete(&p_q);
									}
								}
								if(!!last_ev_dtm) {
									sj_since = last_ev_dtm;
									{
										const LDATETIME local_cdtm = getcurdatetime_();
										// Если время последнего события превышает текущее время, то придется
										// считать, что Since равно текущему времени.
										// Такая ситуация возможна при сбое часов одного из компьютеров, генерирующего
										// события в системном журнале.
										if(cmp(sj_since, local_cdtm) > 0)
											sj_since = local_cdtm;
									}
								}
								addendum_queue_stat.SjPrcCount = 1;
								pt_sj.Register();
							}
						}
						if(State & stPhnSvc && pt_phnsvc.IsTime()) { // @v10.6.0 p_phnsvc_cli-->(State & stPhnSvc)
							if(SETIFZ(p_queue, DS.GetAdviseEventQueue(0))) {
								// @v10.6.0 Немного меняем схему: ранее, если !p_phnsvc_cli то мы больше не обращались к телефонному сервису.
								// Однако могло случиться что очередная попытка получения статуса и не удачного восстановления соединения
								// полность обрывало возможность получения сообщений в дальнейшем.
								// Теперь смотрим на то был ли клиент инициализирован изначально (State & stPhnSvc). Если да, то
								// будем каждый раз пытаться восстановить работу клиента.
								SETIFZ(p_phnsvc_cli, CreatePhnSvcClient(p_phnsvc_cli)); // @v10.6.0
								int gcs_ret = p_phnsvc_cli ? p_phnsvc_cli->GetChannelStatus(0, chnl_status_list) : 0;
								if(!gcs_ret) {
									PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR|LOGMSGF_TIME|LOGMSGF_COMP);
									p_phnsvc_cli = CreatePhnSvcClient(p_phnsvc_cli);
									if(p_phnsvc_cli)
										gcs_ret = p_phnsvc_cli->GetChannelStatus(0, chnl_status_list);
								}
								if(gcs_ret && chnl_status_list.GetCount()) {
									for(uint si = 0; si < chnl_status_list.GetCount(); si++) {
										chnl_status_list.Get(si, chnl_status);
										int32   local_action = 0;
										if(chnl_status.State == PhnSvcChannelStatus::stUp) {
											if(PPObjPhoneService::IsPhnChannelAcceptable(PhnSvcLocalUpChannelSymb, chnl_status.Channel) > 0)
												local_action = PPEVNT_PHNC_UP;
										}
										else if(chnl_status.State == PhnSvcChannelStatus::stRinging) {
											if(PhnSvcLocalScanChannelSymb.IsEmpty() ||
												PPObjPhoneService::IsPhnChannelAcceptable(PhnSvcLocalScanChannelSymb, chnl_status.Channel) > 0)
												local_action = PPEVNT_PHNS_RINGING;
										}
										if(local_action) {
											PPAdviseEvent ev;
											ev.SetupAndAppendToVector(chnl_status, local_action, StartUp_PhnSvcPack.Rec.ID, temp_list);
											addendum_queue_stat.PhnSvcMsgCount++;
										}
									}
								}
								addendum_queue_stat.PhnSvcPrcCount = 1;
								pt_phnsvc.Register();
							}
						}
						if(p_mqb_cli && pt_mqc.IsTime()) {
							if(SETIFZ(p_queue, DS.GetAdviseEventQueue(0))) {
								while(p_mqb_cli->ConsumeMessage(mqb_envelop, 0) > 0) {
									PPAdviseEvent ev;
									ev.SetupAndAppendToVector(mqb_envelop, temp_list);
									p_mqb_cli->Ack(mqb_envelop.DeliveryTag, 0);
									addendum_queue_stat.MqbMsgCount++;
								}
								addendum_queue_stat.MqbPrcCount = 1;
								pt_mqc.Register();
							}
						}
						if(p_queue) {
							p_queue->Push(temp_list);
							p_queue->AddStatCounters(addendum_queue_stat); // @v10.6.0
						}
						if(do_debug_log) {
							(temp_buf = "TimerSjEvent").Space().CatEq("use_sj_scan_alg2", static_cast<long>(use_sj_scan_alg2));
							if(!p_queue)
								temp_buf.Space().Cat("Queue is zero");
							else
								temp_buf.Space().CatEq("QueueAddendum", temp_list.getCount());
							PPLogMessage(PPFILNAM_DEBUG_AEQ_LOG, temp_buf, LOGMSGF_DBINFO|LOGMSGF_TIME|LOGMSGF_USER);
						}
					}
					break;
				case WAIT_FAILED:
					if(do_debug_log) {
						PPLogMessage(PPFILNAM_DEBUG_AEQ_LOG, "QueueFailed", LOGMSGF_DBINFO|LOGMSGF_TIME|LOGMSGF_USER);
					}
					// error
					break;
			}
		}
		CATCH
			{
			}
		ENDCATCH
		delete p_phnsvc_cli;
		delete p_mqb_cli;
		delete p_q;
		ZDELETE(P_Sj);
		DBS.CloseDictionary();
	}
	DbLoginBlock LB;
	SysJournal * P_Sj;
	SString PhnSvcLocalUpChannelSymb;   // Символ канала (каналов), по которым должны регистрироваться события подъема трубки
	SString PhnSvcLocalScanChannelSymb; // Символ канала (каналов), события по которым должны регистрироваться
	PPPhoneServicePacket StartUp_PhnSvcPack;
	PhnSvcChannelStatusPool PhnSvcStP;
	PPMqbClient::InitParam StartUp_MqbParam; // @v10.5.7
	enum {
		stPhnSvc = 0x0001, // Устанавливается если при старте потока был инициирован клиент телефонного сервиса
		stMqb    = 0x0002  // Устанавливается если при старте потока был инициирован клиент брокера сообщений
	};
	uint   State;
};

int PPSession::Helper_Process_HostAvailability_Query(const char * pHost, int query) // @v11.1.2
{
	int    result = -1;
	if(!isempty(pHost)) {
		ENTER_CRITICAL_SECTION
		if(query < 0) {
			if(AvailableHostList.searchNcAscii(pHost, 0)) {
				result = 1;
			}
			else if(UnavailableHostList.searchNcAscii(pHost, 0)) {
				result = 0;
			}
		}
		else if(query > 0) {
			if(!AvailableHostList.searchNcAscii(pHost, 0)) {
				AvailableHostList.add(pHost);
				result = 1;
			}
		}
		else {
			if(!UnavailableHostList.searchNcAscii(pHost, 0)) {
				UnavailableHostList.add(pHost);
				result = 1;
			}
		}
		LEAVE_CRITICAL_SECTION
	}
	return result;
}

void PPSession::CheckRemoteHosts(const StringSet & rHostList) // @v11.1.2
{
	class InnerThread : public PPThread {
	public:
		InnerThread(const StringSet & rHostList) : PPThread(kUnknown, 0, 0), HostList(rHostList)
		{
		}
		virtual void Run()
		{
			SString temp_buf;
			for(uint ssp = 0; HostList.get(&ssp, temp_buf);) {
				temp_buf.Strip();
				hostent * p_host = gethostbyname(temp_buf);
				if(!p_host) {
					DS.Helper_Process_HostAvailability_Query(temp_buf, 0);
				}
				else {
					DS.Helper_Process_HostAvailability_Query(temp_buf, 1);
				}
			}
		}
		StringSet HostList;
	};
	InnerThread * p_thread = new InnerThread(rHostList);
	p_thread->Start(0);
}

int PPSession::GetHostAvailability(const char * pHost)
{
	return Helper_Process_HostAvailability_Query(pHost, -1);
}

int PPSession::Login(const char * pDbSymb, const char * pUserName, const char * pPassword, long flags)
{
	enum {
        logmOrdinary          = 0,
        logmSystem            = 1, // Под именем SYSTEM
        logmService           = 2, // Под именем PPSession::P_JobLogin
        logmEmptyBaseCreation = 3  // Под именем PPSession::P_EmptyBaseCreationLogin
	};
	int    ok = 1, r;
	int    debug_r = 0;
	uint   db_state = 0; // @v9.9.0 Флаги состояния базы данных
	SString dict_path;
	SString data_path;
	SString db_symb;
	SString msg_buf;
	SString temp_buf;
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
		r_tla.State &= ~PPThreadLocalArea::stAuth;
		THROW(OpenDictionary2(&blk, 0));
		debug_r = 4;
		r_tla.Prf.InitUserProfile(user_name); // Инициализация профайлера с параметрами БД сразу после соединения с сервером БД.
		r_tla.UfpSess.Begin(PPUPRF_SESSION);  // Профилирование всей сессии работы в БД (Login..Logout)
		PPUserFuncProfiler ufp(PPUPRF_LOGIN); // Профилирование собственно процесса авторизации в базе данных
		const long db_path_id = DBS.GetDbPathID();
		DbProvider * p_dict = CurDict;
		assert(p_dict);
		p_dict->GetDatabaseState(&db_state);
		{
			//
			// Имя SYSTEM является встроенным аналогом имени MASTER и отличается //
			// от него только идентификатором меню (MENU_SYSTEM)
			//
			if(sstreqi_ascii(user_name, "SYSTEM")) {
				STRNSCPY(user_name, "MASTER");
				logmode = logmSystem;
			}
			else if(sstreqi_ascii(user_name, PPSession::P_JobLogin))
				logmode = logmService;
			else if(sstreqi_ascii(user_name, PPSession::P_EmptyBaseCreationLogin))
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
					PPVersionInfo ver_inf(0);
					const SVerT this_ver = ver_inf.GetVersion();
					const SVerT this_db_min_ver = ver_inf.GetVersion(1);
					// @v10.6.2 {
					class DbrSignalFile {
					public:
						DbrSignalFile(const char * pPath, const SVerT & rVer)
						{
							SString name;
							int   mj, mn, rv;
							rVer.Get(&mj, &mn, &rv);
							name.Z().Cat("dbr").CatChar('-').CatLongZ(mj, 2).CatLongZ(mn, 2).CatLongZ(rv, 2).Dot().Cat("signal");
							(FileName = pPath).SetLastSlash().Cat(name);
							(Direc = pPath).SetLastSlash().Cat("signal");
							(FileName2 = Direc).SetLastSlash().Cat(name);
						}
						int    IsExists()
						{
							return (::fileExists(FileName2) || ::fileExists(FileName));
						}
						void   Create()
						{
							::createEmptyFile((IsDirectory(Direc) || createDir(Direc)) ? FileName2 : FileName);
						}
					private:
						SString Direc;
						SString FileName;
						SString FileName2;
					};
					// } @v10.6.2
					/* @v10.6.2
					SString dbr_signal_file_name;
					{
						int   mj, mn, rv;
						this_ver.Get(&mj, &mn, &rv);
                        (dbr_signal_file_name = data_path).SetLastSlash().Cat("dbr").
							CatChar('-').CatLongZ(mj, 2).CatLongZ(mn, 2).CatLongZ(rv, 2).Dot().Cat("signal");
					}
					if(!::fileExists(dbr_signal_file_name)) { */
					DbrSignalFile dbr_signal(data_path, this_ver); // @v10.6.2
					if(!dbr_signal.IsExists()) { // @v10.6.2
						THROW_PP(!GetSync().IsDBLocked(), PPERR_SYNCDBLOCKED);
						debug_r = 8;
						PPWaitStart();

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
						THROW(ConvertSCardSeries9809()); // @v9.8.9
						THROW(Convert9811()); // @v9.8.11
						// @v10.2.9 THROW(Convert10012()); // @v10.0.12
						THROW(Convert10209()); // @v10.2.9
						THROW(Convert10507()); // @v10.5.7
						THROW(Convert10702()); // @v10.7.2
						THROW(Convert10903()); // @v10.9.3 конвертация ссылок на рабочие столы и меню в группах и пользователях
						THROW(Convert10905()); // @v10.9.5
						THROW(Convert11004()); // @v11.0.4 Конвертация TSessLine (добавлены поля LotDimX, LotDimY, LotDimZ)
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
                        // @v10.6.2 ::createEmptyFile(dbr_signal_file_name);
						dbr_signal.Create(); // @v10.6.2
						PPWaitStop();
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
				THROW_PP(r == 0, PPERR_INVUSERORPASSW);
			}
			THROW(r = p_ref->SearchName(PPOBJ_USR, &r_lc.UserID, user_name));
			if(r < 0) {
				id = 0;
				THROW(r = p_ref->EnumItems(PPOBJ_USR, &id));
				THROW_PP(r < 0, PPERR_INVUSERORPASSW);
				empty_secur_base = 1;
				r_lc.UserID = 0;
				PTR32(pw)[0] = 0;
			}
			else {
				usr_rec = *reinterpret_cast<const PPSecur *>(&p_ref->data);
				THROW(Reference::VerifySecur(&usr_rec, 0));
				Reference::GetPassword(&usr_rec, pw, sizeof(pw));
			}
			THROW(FetchConfig(PPOBJ_USR, r_lc.UserID, &r_lc));
			SLS.SetUiFlag(sluifUseLargeDialogs, 0);
			if(!oneof2(logmode, logmService, logmEmptyBaseCreation)) {
				int    pw_is_wrong = 1;
				// @v10.1.10 {
				if(usr_rec.ID != PPUSR_MASTER && checkdate(usr_rec.ExpiryDate)) {
					THROW_PP_S(getcurdate_() <= usr_rec.ExpiryDate, PPERR_USRACCEXPIRED, usr_rec.Name);
				}
				// } @v10.1.10
				if(pw[0] && (r_lc.Flags & CFGFLG_SEC_CASESENSPASSW) ? strcmp(pw, pPassword) : stricmp866(pw, pPassword)) {
					if(logmode == logmSystem) {
						// для совместимости со старыми версиями (раньше использовался другой механизм шифрования)
						decrypt(memcpy(pw, usr_rec.Password, sizeof(pw)), sizeof(pw));
						if(stricmp866(pw, pPassword) == 0)
							pw_is_wrong = 0;
					}
				}
				else
					pw_is_wrong = 0;
				memzero(pw, sizeof(pw));
				THROW_PP(pw_is_wrong == 0, PPERR_INVUSERORPASSW);
				if(!CheckExtFlag(ECF_SYSSERVICE)) {
					//
					// Уникальность входа не должна проверяться для сеансов JobServer'а
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
					{
						PPObjSecur sec_obj(PPOBJ_USR, 0);
						sec_obj.GetPrivateDesktop(r_lc.UserID, r_lc.DesktopUuid_);
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
				SETFLAG(r_cc.Flags2, CCFLG2_USESDONPURCHOP, suppl_agt.Flags & AGTF_USESDONPURCHOP);
			}
			if(!empty_secur_base)
				THROW(r_tla.Paths.Get(PPOBJ_USR, r_lc.UserID));
			SetPath(PPPATH_DAT, data_path, 0, 1);
			SetPath(PPPATH_SYS, dict_path, 0, 1);
			r_tla.UserName = user_name;
			if(logmode == logmSystem)
				r_lc.Menu = MENU_SYSTEM;
			else if(logmode == logmService)
				r_lc.Menu = -1;
			if(r_lc.UserID && r_lc.UserID != PPUSR_MASTER) {
				PPAccessRestriction accsr;
				THROW(r_tla.Rights.Get(PPOBJ_USR, r_lc.UserID, 0/*ignoreCheckSum*/));
				r_tla.Rights.GetAccessRestriction(accsr);
				r_tla.Rights.ExtentOpRights();
				getcurdatetime(&cdt, &ctm);
				if(usr_rec.PwUpdate && accsr.PwPeriod && diffdate(&cdt, &usr_rec.PwUpdate, 0) > accsr.PwPeriod)
					r_lc.State |= CFGST_PWEXPIRED;
				r_lc.AccessLevel = accsr.AccessLevel;
			}
			else
				r_lc.AccessLevel   = 0;
			if(!(flags & loginfSkipLicChecking)) { // @v10.8.11
				THROW_PP(CheckLicense(&machine_id, &is_demo) > 0, PPERR_MAX_SESSION_DEST);
			}
			SETFLAG(r_lc.State, CFGST_DEMOMODE, is_demo);
			if(!CMng.HasDbEntry(db_path_id)) {
				const ulong cur_process_id = GetCurrentProcessId();
				const char * p_func_name = "Login";
				if(!::ProcessIdToSessionId(cur_process_id, &term_sess_id)) {
					PPGetMessage(mfError, PPERR_SLIB, 0, 0, msg_buf.Z());
					msg_buf.Space().CatEq("Function", p_func_name);
					PPLogMessage(PPFILNAM_ERR_LOG, msg_buf, LOGMSGF_TIME|LOGMSGF_USER|LOGMSGF_DBINFO);
				}
				LogTerminalSessInfo(cur_process_id, term_sess_id, p_func_name);
			}
			GetSync().LoginUser(r_lc.UserID, user_name, &r_lc.SessionID, &machine_id, term_sess_id);
			r_lc.State |= CFGST_INITIATE;
			if(empty_secur_base)
				r_lc.State |= CFGST_EMPTYBASE;
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
			if(r_lc.MainOrg)
				SetMainOrgID(r_lc.MainOrg, 1);
			else if(r_cc.MainOrgID)
				SetMainOrgID(r_cc.MainOrgID, 1);
			if(r_lc.Location)
				SetLocation(r_lc.Location, 1 /*notInteractive*/);
			if(r_lc.DBDiv) {
				r_tla.CurDbDivName.Id = r_lc.DBDiv;
				GetObjectName(PPOBJ_DBDIV, r_lc.DBDiv, r_tla.CurDbDivName);
			}
			if(!(p_dict->GetCapability() & DbProvider::cSQL)) { // @debug
				if(PPCheckDatabaseChain() == 0) {
					SDelay(10000);
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
						// @v10.1.4 SetExtFlag(~(ECF_SYSSERVICE|ECF_DBDICTDL600|ECF_DETECTCRDBTEXISTBYOPEN|ECF_OPENSOURCE), 0); // @v9.4.9 ECF_OPENSOURCE
						// @v10.1.4 {
						//SetExtFlag(ECF_SYSSERVICE, 0);
						//SetExtFlag(ECF_DBDICTDL600, 0);
						//SetExtFlag(ECF_DETECTCRDBTEXISTBYOPEN, 0);
						//SetExtFlag(ECF_OPENSOURCE, 0);
						//SetExtFlag(ECF_FULLGOODSCACHE, 0);
						//SetExtFlag(ECF_PREPROCBRWONCHGFILT, 0);
						//SetExtFlag(ECF_TRACESYNCLOT, 0);
						//SetExtFlag(ECF_DISABLEASYNCADVQUEUE, 0);
						//SetExtFlag(ECF_DLLMODULE, 0);
						SetExtFlag(ECF_AVERAGE, 0);
						SetExtFlag(ECF_INITONLOGIN, 0);
						SetExtFlag(ECF_CHKPAN_USEGDSLOCASSOC, 0);
						SetExtFlag(ECF_DEBUGDIRTYMTX, 0);
						SetExtFlag(ECF_USECDB, 0);
						SetExtFlag(ECF_RCPTDLVRLOCASWAREHOUSE, 0);
						SetExtFlag(ECF_USESJLOGINEVENT, 0);
						SetExtFlag(ECF_CODEPREFIXEDLIST, 0);
						SetExtFlag(ECF_USEGEOTRACKING, 0);
						// } @v10.1.4
						SetExtFlagByIniIntParam(ini_file, PPINISECT_CONFIG, PPINIPARAM_GRPACK,                  ECF_GOODSRESTPACK,          1);
						SetExtFlagByIniIntParam(ini_file, PPINISECT_CONFIG, PPINIPARAM_TIDPACK,                 ECF_TRFRITEMPACK,           1);
						SetExtFlagByIniIntParam(ini_file, PPINISECT_CONFIG, PPINIPARAM_GBFSDEBT,                ECF_GOODSBILLFILTSHOWDEBT,  1);
						SetExtFlagByIniIntParam(ini_file, PPINISECT_CONFIG, PPINIPARAM_ECOGOODSSEL,             ECF_ECOGOODSSEL,            1);
						SetExtFlagByIniIntParam(ini_file, PPINISECT_CONFIG, PPINIPARAM_433_OLDGENBARCODEMETHOD, ECF_433OLDGENBARCODEMETHOD, 1);
						SetExtFlagByIniIntParam(ini_file, PPINISECT_CONFIG, PPINIPARAM_AVERAGE,                 ECF_AVERAGE,                1);
						{
							SetExtFlag(ECF_PREPROCBRWONCHGFILT, 1);
							if(ini_file.GetInt(PPINISECT_SYSTEM, PPINIPARAM_PREPROCESSBRWFROMCHNGFLT, &(iv = 0)) > 0 && iv == 0)
								SetExtFlag(ECF_PREPROCBRWONCHGFILT, 0);
						}
						SetExtFlagByIniIntParam(ini_file, PPINISECT_CONFIG, PPINIPARAM_SHTRIH_USEGOODSLOCASSOC, ECF_CHKPAN_USEGDSLOCASSOC,  999);
						SetExtFlagByIniIntParam(ini_file, PPINISECT_CONFIG, PPINIPARAM_DEBUG_MTX_DIRTY,         ECF_DEBUGDIRTYMTX,          999);
						SetExtFlagByIniIntParam(ini_file, PPINISECT_CONFIG, PPINIPARAM_USE_CDB,                 ECF_USECDB,                 999);
						SetExtFlagByIniIntParam(ini_file, PPINISECT_CONFIG, PPINIPARAM_RCPTDLVRLOCASWAREHOUSE,  ECF_RCPTDLVRLOCASWAREHOUSE, 999);
						SetExtFlagByIniIntParam(ini_file, PPINISECT_CONFIG, PPINIPARAM_USESJLOGINEVENT,         ECF_USESJLOGINEVENT,        999);
						if(!SetExtFlagByIniIntParam(ini_file, PPINISECT_CONFIG, PPINIPARAM_CODEPREFIXEDLIST, ECF_CODEPREFIXEDLIST, 999))
							SetExtFlag(ECF_CODEPREFIXEDLIST, 0);
						if(ini_file.GetInt(PPINISECT_CONFIG, PPINIPARAM_DISABLEASYNCADVQUEUE, &(iv = 0)) > 0 && iv != 0)
							SetExtFlag(ECF_DISABLEASYNCADVQUEUE, 1);
						else
							SetExtFlag(ECF_DISABLEASYNCADVQUEUE, 0);
						{
							SetExtFlag(ECF_TRACESYNCLOT, 0);
							if(ini_file.GetParam("config", "tracesynclot", temp_buf.Z()) > 0) {
								long tsl = temp_buf.ToLong();
								if(tsl > 0)
									SetExtFlag(ECF_TRACESYNCLOT, 1);
							}
						}
						if(ini_file.GetInt(PPINISECT_CONFIG, PPINIPARAM_USEGEOTRACKING, &(iv = 0)) > 0 && iv != 0)
							SetExtFlag(ECF_USEGEOTRACKING, 1);
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
					if(checkdate(dt))
						r_cc._390_DisCalcMethodLockDate = dt;
				}
				if(ini_file.Get(PPINISECT_SYSTEM, PPINIPARAM_3918_TDISCALCMETHODLOCKDATE, sv) > 0) {
					dt = strtodate_(sv, DATF_DMY);
					if(checkdate(dt))
						r_cc._3918_TDisCalcMethodLockDate = dt;
				}
				r_cc._405_TDisCalcMethodLockDate = ZERODATE;
				if(ini_file.Get(PPINISECT_SYSTEM, PPINIPARAM_405_TDISCALCMETHODLOCKDATE, sv) > 0) {
					dt = strtodate_(sv, DATF_DMY);
					if(checkdate(dt))
						r_cc._405_TDisCalcMethodLockDate = dt;
				}
				r_cc._418_TDisCalcMethodLockDate = ZERODATE;
				if(ini_file.Get(PPINISECT_SYSTEM, PPINIPARAM_418_TDISCALCMETHODLOCKDATE, sv) > 0) {
					dt = strtodate_(sv, DATF_DMY);
					if(checkdate(dt))
						r_cc._418_TDisCalcMethodLockDate = dt;
				}
				{
					r_tla.SCardPatterns.Id = 1;
					r_tla.SCardPatterns.CopyFrom(0);
					if(ini_file.Get(PPINISECT_CONFIG, PPINIPARAM_SCARD_PATTERNS, sv) && sv.NotEmptyS())
						r_tla.SCardPatterns.CopyFrom(sv);
				}
				{
					r_tla.DL600XMLEntityParam.Z();
					if(ini_file.Get(PPINISECT_CONFIG, PPINIPARAM_DL600XMLENTITY, sv) && sv.NotEmptyS())
						r_tla.DL600XMLEntityParam = sv;
				}
				{
					r_tla.DL600XmlCp = cpANSI; // Правильно было бы UTF-8, но для обратной совместимости придется по умолчанию использовать ANSI
					if(ini_file.Get(PPINISECT_CONFIG, PPINIPARAM_DL600XMLCP, sv) && sv.NotEmptyS())
						r_tla.DL600XmlCp.FromStr(sv);
				}
				if(ini_file.GetInt(PPINISECT_CONFIG, PPINIPARAM_ADJCPANCCLINETRANS, &(iv = 0)) > 0 && iv != 0)
					r_cc.Flags2 |= CCFLG2_ADJCPANCCLINETRANS;
				else
					r_cc.Flags2 &= ~CCFLG2_ADJCPANCCLINETRANS; // @paranoic
				if(ini_file.GetInt(PPINISECT_CONFIG, PPINIPARAM_DONTUSE3TIERGMTX, &(iv = 0)) > 0 && iv != 0)
					r_cc.Flags2 |= CCFLG2_DONTUSE3TIERGMTX;
				else
					r_cc.Flags2 &= ~CCFLG2_DONTUSE3TIERGMTX; // @paranoic
				// @v10.1.9 {
				{
					r_cc.Flags2 &= ~CCFLG2_USEVETIS;
					PPAlbatrossConfig acfg;
					if(DS.FetchAlbatrosConfig(&acfg) > 0) {
						acfg.GetExtStrData(ALBATROSEXSTR_VETISUSER, temp_buf.Z());
						if(temp_buf.NotEmpty()) {
							acfg.GetExtStrData(ALBATROSEXSTR_VETISAPIKEY, temp_buf.Z());
							if(temp_buf.NotEmpty())
								r_cc.Flags2 |= CCFLG2_USEVETIS;
						}
					}
				}
				// } @v10.1.9
				// @v10.5.9 {
				if(ini_file.GetInt(PPINISECT_CONFIG, PPINIPARAM_DEVELOPMENT, &(iv = 0)) > 0 && iv == 1)
					r_cc.Flags2 |= CCFLG2_DEVELOPMENT;
				else
					r_cc.Flags2 &= ~CCFLG2_DEVELOPMENT;
				// } @v10.5.9
				// @v10.7.3 {
				if(ini_file.GetInt(PPINISECT_CONFIG, PPINIPARAM_VERIFYARTOLOCMETHODS, &(iv = 0)) > 0 && iv == 1)
					r_cc.Flags2 |= CCFLG2_VERIFYARTOLOCMETHS;
				else
					r_cc.Flags2 &= ~CCFLG2_VERIFYARTOLOCMETHS;
				// } @v10.7.3
				r_cc._InvcMergeTaxCalcAlg2Since = ZERODATE;
				if(ini_file.Get(PPINISECT_CONFIG, PPINIPARAM_INVCMERGETAXCALCALG2SINCE, sv) > 0) {
					dt = strtodate_(sv, DATF_DMY);
					if(checkdate(dt))
						r_cc._InvcMergeTaxCalcAlg2Since = dt;
				}
				// @v10.7.9 {
				r_cc.StringHistoryUsage = 0; // @v10.8.7 -1-->0
				if(ini_file.GetInt(PPINISECT_CONFIG, PPINIPARAM_STRINGHISTORYUSAGE, &(iv = 0)) > 0) {
					if(iv > 0)
						r_cc.StringHistoryUsage = 1;
					else if(iv < 0)
						r_cc.StringHistoryUsage = -1;
					else
						r_cc.StringHistoryUsage = 0;
				}
				// } @v10.7.9
				// @v10.9.12 {
				{
					//#define CCFLG2_HIDEINVENTORYSTOCK  0x00010000L // @v10.9.12 Флаг, предписывающий скрывать значения учетных остатков
						// инициируются по параметру в pp.ini [config] PPINIPARAM_INVENTORYSTOCKVIEWRESTRICTION
					if(ini_file.Get(PPINISECT_CONFIG, PPINIPARAM_INVENTORYSTOCKVIEWRESTRICTION, sv) > 0) {
						// При дублировании символов приоритет у токена, который находится дальше по списку
						// Например: 
						//   "user1,user2,!user1" - user1 имеет право видеть остатки, ибо второй раз указан с отрицанием
						//   "!user2,user1,user2" - user2 не имеет права видеть остатки, ибо второй раз указан без отрицания
						// Квантор $ALL$ запрещает доступ к просмотру всех пользователей кроме master и тех, что указаны после с отрицанием
						StringSet ss;
						bool _for_all = false;
						bool _this_user_found = false;
						bool _this_user_found_with_neg = false;
						//DS.GetConstTLA().Cu
						sv.Tokenize(";,", ss);
						for(uint ssp = 0; ss.get(&ssp, temp_buf);) {
							if(temp_buf.NotEmptyS()) {
								if(temp_buf.IsEqiAscii("$ALL$")) {
									_for_all = true;
								}
								else if(temp_buf.IsEqNC(usr_rec.Symb)) {
									_this_user_found = true;
									_this_user_found_with_neg = false;
								}
								else if(temp_buf.C(0) == '!' && temp_buf.ShiftLeft().IsEqNC(usr_rec.Symb)) {
									_this_user_found = false;
									_this_user_found_with_neg = true;
								}
							}
						}
						if(_for_all) {
							if(!(r_lc.State & CFGST_MASTER) && !_this_user_found_with_neg)
								r_cc.Flags2 |= CCFLG2_HIDEINVENTORYSTOCK;
						}
						else if(_this_user_found)
							r_cc.Flags2 |= CCFLG2_HIDEINVENTORYSTOCK;
					}
				}
				// } @v10.9.12
				r_tla.Bac.Load();
			}
			if(CheckExtFlag(ECF_USESJLOGINEVENT))
				LogAction(PPACN_LOGIN, 0, 0, r_lc.SessionID, 1);
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
				int    is_new_cdb_entry = BIN(CMng.CreateDbEntry(db_path_id) > 0);
				if(CheckExtFlag(ECF_SYSSERVICE)) {
					if(is_new_cdb_entry) {
						PPDbDispatchSession * p_sess = new PPDbDispatchSession(db_path_id, db_symb);
						p_sess->Start(0);
					}
				}
				else {
					if(_PPConst.UseAdvEvQueue) {
						int    cycle_ms = 0;
						SString mqb_domain; // Имя домена для идентификации при обмене через брокера сообщений
						const PPPhoneServicePacket * p_phnsvc_pack = 0;
						PPPhoneServicePacket ps_pack; 
						PPMqbClient::InitParam mqb_init_param; // @v10.5.7
						PPMqbClient::InitParam * p_mqb_init_param = 0; // @v10.5.7
						{
							PPEquipConfig eq_cfg;
							ReadEquipConfig(&eq_cfg);
							if(eq_cfg.PhnSvcID) {
								PPObjPhoneService ps_obj(0);
								if(ps_obj.GetPacket(eq_cfg.PhnSvcID, &ps_pack) > 0)
									r_tla.DefPhnSvcID = eq_cfg.PhnSvcID;
							}
						}
						ini_file.GetInt(PPINISECT_CONFIG, PPINIPARAM_ADVISEEVENTCOLLECTORPERIOD, &cycle_ms);
						if(cycle_ms <= 0 || cycle_ms > 600000)
							cycle_ms = 5113;
						if(r_tla.DefPhnSvcID) { // Пакет ps_pack инициализирован выше (r_tla.DefPhnSvcID != 0 - однозначно свидетельствует об этом)
							UserInterfaceSettings ui_cfg;
							if(ui_cfg.Restore() > 0 && ui_cfg.Flags & ui_cfg.fPollVoipService)
								p_phnsvc_pack = &ps_pack;
						}
						// @v10.5.7 {
						if(PPMqbClient::SetupInitParam(mqb_init_param, 0, &mqb_domain)) {
							int   use_mqb_for_dbx = 0;
							PPMqbClient::RoutingParamEntry rpe;
							if(r_lc.DBDiv && r_lc.UserID) {
								if(r_lc.UserID == PPUSR_MASTER)
									use_mqb_for_dbx = 1;
								else {
									PPAccessRestriction accsr;
									THROW(r_tla.Rights.Get(PPOBJ_USR, r_lc.UserID, 0/*ignoreCheckSum*/));
									r_tla.Rights.GetAccessRestriction(accsr);
									if(accsr.CFlags & accsr.cfAllowDbxReceive)
										use_mqb_for_dbx = 1;
								}
							}
							if(use_mqb_for_dbx && rpe.SetupReserved(PPMqbClient::rtrsrvPapyrusDbx, mqb_domain, 0, r_lc.DBDiv, 0)) {
								PPMqbClient::RoutingParamEntry * p_new_entry = mqb_init_param.ConsumeParamList.CreateNewItem();
								ASSIGN_PTR(p_new_entry, rpe);
								p_mqb_init_param = &mqb_init_param;
							}
							if(rpe.SetupReserved(PPMqbClient::rtrsrvRpcListener, mqb_domain, 0, 0, 0)) {
								PPMqbClient::RoutingParamEntry * p_new_entry = mqb_init_param.ConsumeParamList.CreateNewItem();
								ASSIGN_PTR(p_new_entry, rpe);
								p_mqb_init_param = &mqb_init_param;
							}
						}
						// } @v10.5.7
						PPAdviseEventCollectorSjSession * p_evc = new PPAdviseEventCollectorSjSession(blk, p_phnsvc_pack, p_mqb_init_param, cycle_ms);
						p_evc->Start(0);
						r_tla.P_AeqThrd = p_evc;
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
			r_tla.SetupEventResponder(r_tla.eventresponderSysMaintenance); // @v10.6.1
			r_tla.SetupEventResponder(r_tla.eventresponderPhoneService); // @v9.8.12
			r_tla.SetupEventResponder(r_tla.eventresponderMqb); // @v10.5.7
			if(CConfig.Flags & CCFLG_DEBUG) { // @v10.4.0 (ранее информация о системном аккаунте выводилась всегда)
				TCHAR  domain_user[128];
				DWORD  duser_len = SIZEOFARRAY(domain_user);
				memzero(domain_user, sizeof(domain_user));
				if(!::GetUserName(domain_user, &duser_len)) // @unicodeproblem
					STRNSCPY(domain_user, _T("!undefined"));
				PPLoadText(PPTXT_LOGININFO, temp_buf.Z());
				msg_buf.Printf(temp_buf, domain_user);
				PPLogMessage(PPFILNAM_INFO_LOG, msg_buf, LOGMSGF_TIME|LOGMSGF_DBINFO|LOGMSGF_USER|LOGMSGF_COMP);
				// @v10.4.0 {
				{
					// Информация о текущих путях
					r_tla.Paths.DumpToStr(temp_buf);
					PPLogMessage(PPFILNAM_INFO_LOG, temp_buf, LOGMSGF_DBINFO|LOGMSGF_USER|LOGMSGF_TIME);
				}
				// } @v10.4.0
			}
			r_tla.State |= PPThreadLocalArea::stAuth;
			ufp.Commit();
			// @v9.9.0 {
#if !defined(_PPDLL) && !defined(_PPSERVER)
			if(oneof2(logmode, logmOrdinary, logmSystem) && db_state & DbProvider::dbstContinuous) {
				PPLoadText(PPTXT_DBINCONTINUOUSMODE, msg_buf);
				PPTooltipMessage(msg_buf, 0, 0, 10000, GetColorRef(SClrOrangered),
					SMessageWindow::fTopmost|SMessageWindow::fSizeByText|SMessageWindow::fPreserveFocus|SMessageWindow::fTextAlignLeft);
			}
#endif
			// } @v9.9.0
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

int PPSession::Register()
{
	int    ok = 1;
	RegSessData data;
	// @v10.7.9 @ctr MEMSZERO(data);
	data.Uuid = SLS.GetSessUuid();
	data.InitTime = getcurdatetime_();
	data.Ver = DS.GetVersion();
	SString uuid_buf;
	data.Uuid.ToStr(S_GUID::fmtIDL, uuid_buf);
	WinRegKey reg_key(HKEY_CURRENT_USER, PPRegKeys::Sessions, 0);
	return reg_key.PutBinary(uuid_buf, &data, sizeof(data)-offsetof(RegSessData, ReserveStart));
}

int PPSession::GetRegisteredSess(const S_GUID & rUuid, PPSession::RegSessData * pData)
{
	int    ok = -1;
	RegSessData data;
	// @v10.7.9 @ctr MEMSZERO(data);
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

int PPSession::Unregister()
{
	WinRegKey reg_key;
	SString uuid_buf;
	SLS.GetSessUuid().ToStr(S_GUID::fmtIDL, uuid_buf);
	return reg_key.DeleteValue(HKEY_CURRENT_USER, PPRegKeys::Sessions, uuid_buf);
}

const SrSyntaxRuleSet * PPSession::GetSrSyntaxRuleSet()
{
	ENTER_CRITICAL_SECTION
	if(!P_SrStxSet && !(State & stSrStxInvalid)) {
		SString src_file_name;
		PPGetFilePath(PPPATH_DD, "syntax.sr", src_file_name);
		if(fileExists(src_file_name)) {
			SFile f_in(src_file_name, SFile::mRead);
			if(f_in.IsValid()) {
				SString line_buf;
				SString src_buf;
				while(f_in.ReadLine(line_buf)) {
					src_buf.Cat(line_buf);
				}
				P_SrStxSet = new SrSyntaxRuleSet;
				if(P_SrStxSet) {
					int r = P_SrStxSet->Parse(src_buf);
					if(r) {
						SrDatabase * p_srdb = GetTLA().GetSrDatabase();
						if(!p_srdb || !P_SrStxSet->ResolveSyntaxRules(*p_srdb)) {
							ZDELETE(P_SrStxSet);
							State |= stSrStxInvalid;
						}
					}
					else {
						SString temp_buf;
						PPGetLastErrorMessage(1, temp_buf);
						line_buf.Z().Cat(src_file_name).CatParStr(P_SrStxSet->LineNo).CatDiv(':', 2).Cat(temp_buf);
						PPLogMessage(PPFILNAM_ERR_LOG, line_buf, LOGMSGF_USER|LOGMSGF_TIME);
						ZDELETE(P_SrStxSet);
						State |= stSrStxInvalid;
					}
				}
			}
		}
		else
			State |= stSrStxInvalid;
	}
	LEAVE_CRITICAL_SECTION
	return P_SrStxSet;
}

int    PPSession::SetDbCacheDeferredState(long dbPathID, int set) { return CMng.SetDeferredState(dbPathID, set); }
int    FASTCALL PPSession::IsDbCacheDeferredState(long dbPathID) { return CMng.IsDeferredState(dbPathID); }

int PPSession::DirtyDbCache(long dbPathID, PPAdviseEventQueue::Client * pCli)
{
	int    ok = 1;
	if(dbPathID && DBS.IsConsistent()) {
		//
		// Следующие три статических объекта защищены общей критической секцией
		//
		static PPIDArray * p_ev_list = 0;
		static PPIDArray * p_comm_dirty_cache_ev_list = 0;
		static PPIDArray * p_addendum_ev_list = 0;
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
						PPACN_QUOTUPD2,
						PPACN_UPDBILLWLABEL,
						PPACN_BILLWROFF,
						PPACN_BILLWROFFUNDO,
						PPACN_BILLSTATUSUPD, // @v10.4.4
						PPACN_UPDBILLFREIGHT, // @v10.4.5
						0L);
					p_comm_dirty_cache_ev_list->sort();
					p_addendum_ev_list = new PPIDArray;
					p_addendum_ev_list->addzlist(PPACN_OBJTAGUPD, PPACN_OBJTAGRMV, PPACN_OBJTAGADD,
						PPACN_CONFIGUPDATED, PPACN_TSSTRATEGYUPD, 0L); // @v10.3.2 PPACN_CONFIGUPDATED // @v10.3.11 PPACN_TSSTRATEGYUPD
					p_addendum_ev_list->sort();
					p_ev_list = new PPIDArray;
					p_ev_list->addUnique(p_comm_dirty_cache_ev_list);
					p_ev_list->addUnique(p_addendum_ev_list);
					p_ev_list->sort();
				}
				assert(p_ev_list && p_addendum_ev_list && p_comm_dirty_cache_ev_list);
			}
			const  uint64 tm_start = SLS.GetProfileTime();
			const  LDATETIME cur = getcurdatetime_();
			const  LDATETIME last_cache_update = CMng.GetLastUpdate(dbPathID);
			uint   dirty_call_count = 0;
			PPAdviseList adv_list;
			struct SjEntry { // @flat
				PPID   Action;  // @v10.3.2 int16-->PPID
				PPID   ObjType; // @v10.3.2 int16-->PPID
				PPID   ObjID;
				long   Extra;
			};
			SVector list(sizeof(SjEntry), O_ARRAY);
			PPAdviseEventVector evq_list;
			PPAdviseEventQueue * p_queue = (pCli && !CheckExtFlag(ECF_DISABLEASYNCADVQUEUE)) ? CMng.GetAdviseEventQueue(dbPathID) : 0;
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
								entry.Action  = p_sj->data.Action;
								entry.ObjType = p_sj->data.ObjType;
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
					entry.Action  = r_ev.Action;
					entry.ObjType = r_ev.Oid.Obj;
					entry.ObjID   = r_ev.Oid.Id;
					entry.Extra   = r_ev.SjExtra;
					list.insert(&entry);
				}
			}
			const uint ev_count = list.getCount();
			if(ev_count) {
				SjEntry * p_item;
				uint   i = 0;
				while(list.enumItems(&i, reinterpret_cast<void **>(&p_item))) {
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
							for(i = 0; list.enumItems(&i, reinterpret_cast<void **>(&p_item));) {
								if((!adv_blk.Action || p_item->Action == adv_blk.Action) && (!adv_blk.ObjType || adv_blk.ObjType == p_item->ObjType)) {
									ev.Clear();
									ev.Action  = p_item->Action;
									ev.ObjType = p_item->ObjType;
									ev.ObjID   = p_item->ObjID;
									ev.ExtInt_ = p_item->Extra;
									adv_blk.Proc(PPAdviseBlock::evDirtyCacheBySysJ, &ev, adv_blk.ProcExtPtr);
								}
							}
							adv_blk.Proc(PPAdviseBlock::evDirtyCacheBySysJ, &ev.Finalize(ZERODATETIME, 0), adv_blk.ProcExtPtr); // finalize
						}
					}
				}
			}
			uint64 tm_finish = SLS.GetProfileTime();
			{
				SString msg_buf;
				(msg_buf = "Cache was updated").CatDiv(':', 2).CatEq("events", ev_count).Space().
					CatEq("calls", dirty_call_count).Space().CatEq("time", (int64)(tm_finish-tm_start));
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
int CreateBackupCopy(const char *, int);

void PPThreadLocalArea::OnLogout()
{
	State &= ~stAuth;
	SrvViewList.freeAll();
	ZDELETE(P_WObj);
	ZDELETE(P_WbObj);
	ZDELETE(P_TodoObj);
	ZDELETE(P_Ref);
	ZDELETE(P_BObj);
	ZDELETE(P_GtaJ);
	if(P_SysJ) {
		if(DS.CheckExtFlag(ECF_USESJLOGINEVENT))
			P_SysJ->LogEvent(PPACN_LOGOUT, 0, 0, 0, 1);
		ZDELETE(P_SysJ);
	}
	ZDELETE(P_ObjSync);
	if(Lc.SessionID)
		Sync.LogoutUser(Lc.SessionID);
	if(SrvSess.GetState() & PPJobSrvClient::stConnected) {
		SrvSess.Logout();
		SrvSess.Disconnect();
	}
}

int PPSession::Logout()
{
	PPThreadLocalArea & r_tla = GetTLA();
	if(r_tla.State & PPThreadLocalArea::stAuth) {
		const SString active_user = r_tla.UserName;
		SString temp_buf;
		r_tla.ReleaseEventResponder(r_tla.eventresponderSysMaintenance); // @v10.6.1
		r_tla.ReleaseEventResponder(r_tla.eventresponderPhoneService);
		r_tla.ReleaseEventResponder(r_tla.eventresponderMqb); // @v10.5.7
		SetPrivateBasket(0, 1);
		//
		// Удаляем временный каталог для отчетных данных
		//
		GetPath(PPPATH_TEMP, temp_buf);
		temp_buf.SetLastSlash().CatLongZ(r_tla.PrnDirId, 8);
		PPRemoveFilesByExt(temp_buf, "*", 0, 0);
		::RemoveDirectory(SUcSwitch(temp_buf));
		//
		if(CCfg().Flags & CCFLG_DEBUG)
			CMng.LogCacheStat();
		r_tla.OnLogout();
		r_tla.Prf.FlashUserProfileAccumEntries();
		r_tla.UfpSess.Commit();
		DBS.CloseDictionary();
		Btrieve::Reset(0);
		if(!CheckExtFlag(ECF_SYSSERVICE) && active_user.NotEmpty())
			CreateBackupCopy(active_user, 0);
		GetSync().Release(); // @todo ReleaseSync()
		GPrf.Output(0, 0);
		// @v10.8.0 {
		{
			if(SLS.GetAllocStat().Output(temp_buf) > 0) {
				SString path;
				PPGetFilePath(PPPATH_LOG, "allocstat.log", path);
				PPLogMessage(path, temp_buf, LOGMSGF_TIME|LOGMSGF_COMP);
			}
		}
		// } @v10.8.0 
		// @v8.6.7 {
		// @todo Аккуратно остановить поток PPAdviseEventCollectorSjSession
		// } @v8.6.7
		// @v10.0.06 {
		if(r_tla.P_AeqThrd) {
			r_tla.P_AeqThrd->Stop(30);
			r_tla.P_AeqThrd = 0;
		}
		// } @v10.0.06
	}
	return 1;
}

SVerT PPSession::GetVersion() const
{
	PPVersionInfo _ver = GetVersionInfo();
	return _ver.GetVersion(0);
}

int PPSession::SetLocation(PPID locID, int notInteractive /*= 0*/)
{
	int    ok = 1;
	LocationTbl::Rec rec;
	PPObjLocation loc_obj;
	if(loc_obj.Fetch(locID, &rec) > 0) {
		GetTLA().Lc.Location = locID;
		SETFLAG(GetTLA().Lc.State, CFGST_WAREHOUSE, rec.Type == LOCTYP_WAREHOUSE);
	}
	else {
		GetTLA().Lc.Location = 0;
		ok = 0;
	}
	if(!notInteractive)
		StatusWinChange(); // @UI
	return ok;
}

void PPSession::SetOperDate(LDATE date)
{
	int    d;
	GetTLA().Lc.OperDate = date;
	decodedate(&d, &DefaultMonth, &DefaultYear, &date);
}

int PPSession::SetDemoMode(int s)
{
	const int c = BIN(GetTLA().Lc.State & CFGST_DEMOMODE);
	SETFLAG(GetTLA().Lc.State, CFGST_DEMOMODE, s);
	return c;
}

long PPSession::SetLCfgFlags(long f)
{
	const long c = GetTLA().Lc.Flags;
	GetTLA().Lc.Flags = f;
	return c;
}

short PPSession::SetRealizeOrder(short s)
{
	const short c = GetTLA().Lc.RealizeOrder;
	GetTLA().Lc.RealizeOrder = s;
	return c;
}

int PPSession::SetMainOrgID(PPID id, int enforce)
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
			r_tla.MainOrgCountryCode = 0;
			if(psn_obj.Search(id, &psn_rec) > 0) {
				r_tla.MainOrgName.Id = id;
				r_tla.MainOrgName.CopyFrom(psn_rec.Name);
				// @v9.7.8 {
				{
					LocationTbl::Rec loc_rec;
					int    is_loc_identified = 0;
					if(psn_rec.RLoc && psn_obj.LocObj.Fetch(psn_rec.RLoc, &loc_rec) > 0)
						is_loc_identified = 1;
					else if(psn_rec.MainLoc && psn_obj.LocObj.Fetch(psn_rec.MainLoc, &loc_rec) > 0)
						is_loc_identified = 1;
					if(is_loc_identified) {
						PPID   country_id = 0;
						PPCountryBlock cb;
						if(psn_obj.LocObj.GetCountry(&loc_rec, &country_id, &cb) > 0 && cb.Code.NotEmptyS())
							r_tla.MainOrgCountryCode = cb.Code;
					}
				}
				// } @v9.7.8
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

int PPSession::RestCheckingStatus(int s)
{
	long * p_flags = &GetTLA().Lc.Flags;
	int    c = BIN((*p_flags) & CFGFLG_CHECKTURNREST);
	if(s > 0)
		(*p_flags) |= CFGFLG_CHECKTURNREST;
	else if(s == 0)
		(*p_flags) &= ~CFGFLG_CHECKTURNREST;
	return c;
}

int FASTCALL PPSession::CheckExtFlag(long f) const
{
	int    result = 0;
	ExtFlagsLck.Lock();
	result = BIN(ExtFlags_ & f);
	ExtFlagsLck.Unlock();
	return result;
}

long PPSession::SetExtFlag(long f, int set)
{
	long   prev = 0;
	ExtFlagsLck.Lock();
	prev = ExtFlags_;
	if(set > 0)
		ExtFlags_ |= f;
	else if(set == 0)
		ExtFlags_ &= ~f;
	ExtFlagsLck.Unlock();
	return prev;
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
	SString entry, drv;
	for(uint i = 0; get(&i, entry) > 0;) {
		if(entry.Divide('=', drv, rMapping) > 0 && toupper(drv[0]) == toupper(drive)) {
			rMapping.Strip();
			return 1;
		}
	}
	rMapping.Z();
	return 0;
}

int PPDriveMapping::ConvertPathToUnc(SString & rPath) const
{
	if(rPath.NotEmptyS() && isalpha(rPath[0]) && rPath[1] == ':') {
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

int PPSession::GetLocalPath(SString & rBuf)
{
	SString comp_name;
	if(!SGetComputerName(comp_name))
		comp_name = "COMMON";
	rBuf.Z();
	GetPath(PPPATH_BIN, rBuf);
	rBuf.SetLastSlash().Cat("LOCAL").SetLastSlash().Cat(comp_name);
	return 1;
}

int PPSession::GetPath(PPID pathID, SString & rBuf)
{
	int    ok = 1;
	switch(pathID) {
		case PPPATH_BIN:
			rBuf = BinPath;
			break;
		case PPPATH_TESTROOT:
			{
				//
				// Путь нужен для определения местонахождения всякого тестового смака.
				// Он находится в SRC/PPTEST
				// Так как текущий файл находится в одном из подкаталогов SRC (скорее всего в PPLIB)
				// то нам нужен один уровень вверх и сразу в PPTEST (..\pptest).
				//
				SString temp_buf;
				GetPath(PPPATH_BIN, temp_buf); // @recursion
                SPathStruc ps(temp_buf);
                ps.Dir.SetLastSlash().Cat("..\\..\\src\\pptest");
				ps.Merge(SPathStruc::fDrv|SPathStruc::fDir, rBuf);
			}
			break;
		case PPPATH_SYSROOT:
			{
				SString & r_temp_buf = SLS.AcquireRvlStr(); // @v9.9.10
				GetPath(PPPATH_BIN, r_temp_buf); // @recursion
				rBuf = r_temp_buf.RmvLastSlash();
				int    last = rBuf.Last();
				while(last && last != '\\' && last != '/')
					last = rBuf.TrimRight().Last();
				if(!last)
					rBuf = r_temp_buf;
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
		rBuf.Z();
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

PPRFile::PPRFile() : Sign(PPRFILE_SIGN), ID(0), PathID(0), SrcPathID(0), Flags(0)
{
}

PPRFile::~PPRFile()
{
	Sign = 0;
}

int PPRFile::IsConsistent() const
{
	return BIN(Sign == PPRFILE_SIGN);
}

PPRFile & PPRFile::Z()
{
	Sign = PPRFILE_SIGN;
	ID = 0;
	PathID = 0;
	SrcPathID = 0;
	Flags = 0;
	Name.Z();
	Descr.Z();
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
			rInfo.PathID    = static_cast<long>(p_rez->getUINT());
			rInfo.SrcPathID = static_cast<long>(p_rez->getUINT());
			rInfo.Flags     = static_cast<long>(p_rez->getUINT());
			p_rez->getString(rInfo.Descr, 2);   // description (OEM encoding)
			ok = 1;
		}
	}
	else
		ok = 0;
	return ok;
}

long PPSession::GetMaxLogFileSize() const { return MaxLogFileSize; }
int  PPSession::GetRFileInfo(PPID fileId, PPRFile & rInfo) { return Helper_GetRFileInfo(fileId, rInfo); }

SEnumImp * PPSession::EnumRFileInfo()
{
	class PPRFileEnum : public SEnumImp {
	public:
		PPRFileEnum() : P_Rez(P_SlRez), DwPos(0)
		{
		}
		virtual int Next(void * pRec)
		{
			int    ok = 0;
			PPRFile * p_info = static_cast<PPRFile *>(pRec);
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

PPAdviseEventQueue * PPSession::GetAdviseEventQueue(PPAdviseEventQueue::Client * pCli)
{
	PPAdviseEventQueue * p_queue = 0;
	if(!CheckExtFlag(ECF_DISABLEASYNCADVQUEUE) && DBS.IsConsistent()) {
		const long db_path_id = DBS.GetDbPathID();
		p_queue = CMng.GetAdviseEventQueue(db_path_id);
        if(pCli && p_queue)
			pCli->Register(db_path_id, p_queue);
	}
	return p_queue;
}

void PPSaveErrContext() { DS.GetTLA().PushErrContext(); }
void PPRestoreErrContext() { DS.GetTLA().PopErrContext(); }

DlContext * PPSession::Helper_GetInterfaceContext(DlContext ** ppCtx, uint fileId, int crit)
{
	int    is_allocated = 0;
	SCriticalSection::Data * p_csd = 0;
	if(*ppCtx == 0) {
		if(crit) {
			p_csd = new SCriticalSection::Data;
			CALLPTRMEMB(p_csd, Enter());
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
	CALLPTRMEMB(p_csd, Leave());
	delete p_csd;
	return *ppCtx;
}

DlContext * PPSession::GetInterfaceContext(int ctxType)
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

PPNotifyEvent::PPNotifyEvent() : PPExtStrContainer(), Action(0), ObjType(0), ObjID(0), ExtInt_(0), ExtDtm(ZERODATETIME), P_MqbEnv(0)
{
}

PPNotifyEvent::~PPNotifyEvent()
{
	delete P_MqbEnv;
}

void PPNotifyEvent::Clear()
{
	Action = 0;
	ObjType = 0;
	ObjID = 0;
	ExtInt_ = 0;
	ExtDtm.Z();
	ExtString.Z();
	ZDELETE(P_MqbEnv);
}

PPNotifyEvent & PPNotifyEvent::Finalize(const LDATETIME & rExtDtm, long extInt)
{
	Action = -1;
	ExtDtm = rExtDtm;
	ExtInt_ = extInt;
	return *this;
}

int PPNotifyEvent::IsFinish() const
{
	return (Action == -1);
}

PPAdviseBlock::PPAdviseBlock()
{
	THISZERO();
}

PPAdviseList::PPAdviseList() : SArray(sizeof(PPAdviseBlock), O_ARRAY), LastCookie(0)
{
}

uint PPAdviseList::GetCount()
{
	uint   c = 0;
	{
		SRWLOCKER(RwL, SReadWriteLocker::Read);
		c = getCount();
	}
	return c;
}

int FASTCALL PPAdviseList::Enum(uint * pI, PPAdviseBlock * pBlk) const
{
	int    ok = 0;
	uint   i = DEREFPTRORZ(pI);
	if(i < getCount()) {
		ASSIGN_PTR(pBlk, *static_cast<const PPAdviseBlock *>(at(i)));
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
	{
		SRWLOCKER(RwL, SReadWriteLocker::Read);
		const uint c = getCount();
		if(c) { // @speedcritical
			for(uint i = 0; i < c; i++) {
				const PPAdviseBlock * p_blk = static_cast<const PPAdviseBlock *>(at(i));
				if(p_blk->Kind == kind && (!objType || p_blk->ObjType == objType) &&
					(!p_blk->TId || p_blk->TId == tId) && (!p_blk->DbPathID || p_blk->DbPathID == dbPathID)) {
					rList.insert(p_blk);
					ok = 1;
				}
			}
		}
	}
	return ok;
}

int PPAdviseList::Advise(long * pCookie, const PPAdviseBlock * pBlk)
{
	int    ok = -1;
	{
		SRWLOCKER(RwL, SReadWriteLocker::Write);
		if(pBlk) {
			const long cookie = ++LastCookie;
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
	}
	return ok;
}

int PPSession::GetAdviseList(int kind, PPID objType, PPAdviseList & rList)
	{ return AdvList.CreateList(kind, GetConstTLA().GetThreadID(), DBS.GetDbPathID(), objType, rList); }
StringSet & PPSession::AcquireRvlSsSCD()
	{ return GetTLA().RvlSsSCD.Get(); }

void PPSession::ProcessIdle()
{
	ENTER_CRITICAL_SECTION // @v10.9.0
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
	LEAVE_CRITICAL_SECTION // @v10.9.0
}

PPSession::ObjIdentBlock::ObjIdentBlock() /*: SymbList(256, 1)*/ : P_ShT(0)
{
	PPIDArray obj_type_list;
	PPGetObjTypeList(&obj_type_list, gotlfExcludeDyn);
	SString name_buf;
	for(uint i = 0; i < obj_type_list.getCount(); i++) {
		const PPID obj_type = obj_type_list.get(i);
		if(PPLoadString(PPSTR_OBJNAMES, (uint)obj_type, name_buf))
			TitleList.AddFast(obj_type, name_buf);
	}
	{
		PPLoadText(PPTXT_CFGNAMES, name_buf);
		StringSet ss(';', name_buf);
		for(uint i = 0, j = 1; ss.get(&i, name_buf) > 0; j++)
			TitleList.AddFast(PPOBJ_FIRST_CFG_OBJ + j, name_buf);
	}
	TitleList.SortByID();
	P_ShT = PPGetStringHash(PPSTR_HASHTOKEN);
}

int PPSession::GetObjectTypeSymb(PPID objType, SString & rBuf)
{
	rBuf.Z();
	int    ok = 0;
	if(!P_ObjIdentBlk)
		DO_CRITICAL(SETIFZ(P_ObjIdentBlk, new ObjIdentBlock));
	if(P_ObjIdentBlk && P_ObjIdentBlk->P_ShT) {
		uint   val = 0;
		const long ext = HiWord(objType);
		switch(LoWord(objType)) {
			case PPOBJ_UNIT:          val = PPHS_UNIT;      break;
			case PPOBJ_QUOTKIND:      val = PPHS_QUOTKIND; break;
			case PPOBJ_LOCATION:      val = PPHS_LOCATION; break;
			case PPOBJ_GOODS:         val = PPHS_GOODS;    break;
			case PPOBJ_GOODSGROUP:    val = PPHS_GOODSGROUP; break;
			case PPOBJ_BRAND:         val = PPHS_BRAND; break;
			case PPOBJ_GOODSTYPE:     val = PPHS_GOODSTYPE; break;
			case PPOBJ_GOODSCLASS:    val = PPHS_GOODSCLASS; break;
			case PPOBJ_GOODSARCODE:   val = PPHS_GOODSARCODE; break;
			case PPOBJ_PERSON:        val = PPHS_PERSON; break;
			case PPOBJ_PERSONKIND:      val = PPHS_PERSONKIND; break;
			case PPOBJ_PRSNSTATUS:    val = PPHS_PERSONSTATUS; break;
			case PPOBJ_PRSNCATEGORY:  val = PPHS_PERSONCATEGORY; break;
			case PPOBJ_GLOBALUSERACC: val = PPHS_GLOBALUSER; break;
			case PPOBJ_DL600DATA:     val = PPHS_DL600; break;
			case PPOBJ_WORLD:
				switch(ext) {
					case WORLDOBJ_CITY:    val = PPHS_CITY; break;
					case WORLDOBJ_COUNTRY: val = PPHS_COUNTRY; break;
					case 0:                val = PPHS_WORLD; break;
				}
				break;
			case PPOBJ_QUOT2:        val = PPHS_QUOT; break;
			case PPOBJ_CURRENCY:     val = PPHS_CURRENCY; break;
			case PPOBJ_CURRATETYPE:  val = PPHS_CURRATETYPE; break;
			case PPOBJ_SPECSERIES:   val = PPHS_SPECSERIES; break;
			case PPOBJ_SCARD:        val = PPHS_SCARD; break;
			case PPOBJ_SCARDSERIES:  val = PPHS_SCARDSERIES; break;
			case PPOBJ_CASHNODE:     val = PPHS_POSNODE; break;
			case PPOBJ_CURRATEIDENT: val = PPHS_CURRATEIDENT; break;
			case PPOBJ_UHTTSCARDOP:  val = PPHS_UHTTSCARDOP; break;
			case PPOBJ_LOT:       val = PPHS_LOT; break;
			case PPOBJ_BILL:      val = PPHS_BILL; break;
			case PPOBJ_UHTTSTORE: val = PPHS_UHTTSTORE; break;
			case PPOBJ_OPRKIND:   val = PPHS_OPRKIND; break;
			case PPOBJ_WORKBOOK:  val = PPHS_WORKBOOK; break;
			case PPOBJ_CCHECK:    val = PPHS_CCHECK; break;
			case PPOBJ_PROCESSOR: val = PPHS_PROCESSOR; break;
			case PPOBJ_TSESSION:  val = PPHS_TSESSION; break;
			case PPOBJ_STYLOPALM: val = PPHS_STYLOPALM; break;
			case PPOBJ_GEOTRACKING: val = PPHS_GEOTRACKING; break; // @v10.1.5
		}
		if(val)
			ok = P_ObjIdentBlk->P_ShT->GetByAssoc(val, rBuf);
	}
	return ok;
}

PPID PPSession::GetObjectTypeBySymb(const char * pSymb, long * pExtraParam)
{
	PPID   obj_type = 0;
	long   ext = 0;
	if(!P_ObjIdentBlk)
		DO_CRITICAL(SETIFZ(P_ObjIdentBlk, new ObjIdentBlock));
	if(P_ObjIdentBlk && P_ObjIdentBlk->P_ShT) {
		SString symb(pSymb);
		uint   val = 0;
		uint   hs_id = 0;
		if(P_ObjIdentBlk->P_ShT->Search(symb.ToLower(), &hs_id, 0)) {
			switch(hs_id) {
				case PPHS_UNIT:           val = PPOBJ_UNIT; break;
				case PPHS_QUOTKIND:       val = PPOBJ_QUOTKIND; break;
				case PPHS_LOCATION:       val = PPOBJ_LOCATION; break;
				case PPHS_GOODS:          val = PPOBJ_GOODS; break;
				case PPHS_GOODSGROUP:     val = PPOBJ_GOODSGROUP; break;
				case PPHS_BRAND:          val = PPOBJ_BRAND; break;
				case PPHS_GOODSTYPE:      val = PPOBJ_GOODSTYPE; break;
				case PPHS_GOODSCLASS:     val = PPOBJ_GOODSCLASS; break;
				case PPHS_GOODSARCODE:    val = PPOBJ_GOODSARCODE; break;
				case PPHS_PERSON:         val = PPOBJ_PERSON; break;
				case PPHS_PERSONKIND:     val = PPOBJ_PERSONKIND; break;
				case PPHS_PERSONSTATUS:   val = PPOBJ_PRSNSTATUS; break;
				case PPHS_PERSONCATEGORY: val = PPOBJ_PRSNCATEGORY; break;
				case PPHS_GLOBALUSER:     val = PPOBJ_GLOBALUSERACC; break;
				case PPHS_DL600:          val = PPOBJ_DL600DATA; break;
				case PPHS_WORLD:          val = PPOBJ_WORLD; break;
				case PPHS_CITY:           val = PPOBJ_WORLD | (WORLDOBJ_CITY << 16); break;
				case PPHS_COUNTRY:        val = PPOBJ_WORLD | (WORLDOBJ_COUNTRY << 16); break;
				case PPHS_QUOT:           val = PPOBJ_QUOT2; break;
				case PPHS_CURRENCY:       val = PPOBJ_CURRENCY; break;
				case PPHS_CURRATETYPE:    val = PPOBJ_CURRATETYPE; break;
				case PPHS_SPECSERIES:     val = PPOBJ_SPECSERIES; break;
				case PPHS_SCARD:          val = PPOBJ_SCARD; break;
				case PPHS_SCARDSERIES:    val = PPOBJ_SCARDSERIES; break;
				case PPHS_POSNODE:        val = PPOBJ_CASHNODE; break;
				case PPHS_CURRATEIDENT:   val = PPOBJ_CURRATEIDENT; break;
				case PPHS_UHTTSCARDOP:    val = PPOBJ_UHTTSCARDOP; break;
				case PPHS_LOT:            val = PPOBJ_LOT; break;
				case PPHS_BILL:           val = PPOBJ_BILL; break;
				case PPHS_UHTTSTORE:      val = PPOBJ_UHTTSTORE; break;
				case PPHS_OPRKIND:        val = PPOBJ_OPRKIND; break;
				case PPHS_WORKBOOK:       val = PPOBJ_WORKBOOK; break;
				case PPHS_CCHECK:         val = PPOBJ_CCHECK; break;
				case PPHS_PROCESSOR:      val = PPOBJ_PROCESSOR; break;
				case PPHS_TSESSION:       val = PPOBJ_TSESSION; break;
				case PPHS_STYLOPALM:      val = PPOBJ_STYLOPALM; break;
				case PPHS_STYLODEVICE:    val = PPOBJ_STYLOPALM; break;
				case PPHS_GEOTRACKING:    val = PPOBJ_GEOTRACKING; break; // @v10.1.5
				case PPHS_TAG:            val = PPOBJ_TAG; break; // @v10.9.4   
				default: PPSetError(PPERR_OBJTYPEBYSYMBNFOUND, pSymb); break;
			}
			obj_type = LoWord(val);
			ext = HiWord(val);
		}
		else {
			PPSetError(PPERR_OBJTYPEBYSYMBNFOUND, pSymb);
		}
	}
	ASSIGN_PTR(pExtraParam, ext);
	return obj_type;
}

int PPSession::GetObjectTitle(PPID objType, SString & rBuf)
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
				rBuf.Z().CatEq("DYN OBJ", objType);
			ok = 1;
		}
		else
			rBuf.Z().Cat(objType);
	}
	else if(P_ObjIdentBlk) {
		if(objType >= PPOBJ_FIRST_CFG_OBJ && objType < PPOBJ_LAST_CFG_OBJ) {
			if(P_ObjIdentBlk->TitleList.GetText(objType, rBuf) > 0)
				ok = 2;
			else
				rBuf.Z().Cat(objType);
		}
		else if(P_ObjIdentBlk->TitleList.GetText(objType, rBuf) > 0)
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
					P_ObjIdentBlk->TitleList.AddFast(objType, name_buf); // @v9.9.3 Add-->AddFast
					P_ObjIdentBlk->TitleList.SortByID(); // @v9.9.3
					rBuf = name_buf;
					found = 1;
				}
				LEAVE_CRITICAL_SECTION
			}
			if(!found)
				rBuf.Z().Cat(objType);
		}
	}
	return ok;
}

PPJobSrvClient * PPSession::GetClientSession(int dontReconnect)
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
					if(PPRef->GetItem(PPOBJ_USR, LConfig.UserID, &usr_rec) > 0) {
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

PPSync & PPSession::GetSync() { return GetTLA().Sync; }
void   PPSession::SetCurCashNodeID(PPID cashNodeID) { GetTLA().Lc.Cash = cashNodeID; }
void   PPSession::SetDefBillCashID(PPID billCashID) { GetTLA().Lc.DefBillCashID = billCashID; }
void   PPSession::SetMenu(short menuId) { GetTLA().Lc.Menu = menuId; }
void   PPSession::SetStateFlag(long f, int set) { SETFLAG(GetTLA().Lc.State, f, set); }
int    PPSession::CheckStateFlag(long f) const { return BIN(GetConstTLA().Lc.State & f); }
int    PPSession::SetPath(PPID pathID, const char * pBuf, short flags, int replace) { return GetTLA().Paths.SetPath(pathID, pBuf, flags, replace); }
int    PPSession::LoadDriveMapping(PPIniFile * pIniFile) { return DrvMap.Load(pIniFile); }
int    PPSession::GetDriveMapping(int drive, SString & rMapping) const { return DrvMap.Get(drive, rMapping); }
int    PPSession::ConvertPathToUnc(SString & rPath) const { return DrvMap.ConvertPathToUnc(rPath); }
int    PPSession::SetDbLocalObjCache(ObjCache * pCache) { return CMng.AddCache(DBS.GetDbPathID(), pCache); }
int    PPSession::Advise(long * pCookie, const PPAdviseBlock * pBlk) { return AdvList.Advise(pCookie, pBlk); }
int    PPSession::Unadvise(long cookie) { return AdvList.Advise(&cookie, 0); }
void   PPSession::SetTempLogFileName(const char * pFileName) { GetTLA().TempLogFile = pFileName; }
int    PPSession::SetPrivateBasket(PPBasketPacket * pPack, int use_ta) { return GetTLA().Cart.Set(pPack, use_ta); }
PPBasketPacket * PPSession::GetPrivateBasket() { return GetTLA().Cart.Get(); }
int    PPSession::StopThread(ThreadID tId) { return ThreadList.StopThread(tId); }

int PPSession::IsThreadStopped()
{
	int    ok = 0;
	const PPThread * p_thread = ThreadList.SearchById(GetConstTLA().GetThreadID());
	ok = BIN(p_thread && p_thread->IsStopping());
	return ok;
}

void PPAdviseEvent::Clear()
{
	THISZERO();
}

PPAdviseEvent & FASTCALL PPAdviseEvent::operator = (const SysJournalTbl::Rec & rSjRec)
{
	Clear();
	Dtm.Set(rSjRec.Dt, rSjRec.Tm);
	Action = rSjRec.Action;
	Oid.Set(rSjRec.ObjType, rSjRec.ObjID);
	UserID = rSjRec.UserID;
	SjExtra = rSjRec.Extra;
	return *this;
}

int PPAdviseEvent::SetupAndAppendToVector(const PhnSvcChannelStatus & rS, int32 action, PPID phnSvcID, PPAdviseEventVector & rAev)
{
	int    ok = 1;
	if(action) {
		// @v10.2.3 {
		/*
		SString outer_caller_id = chnl_status.CallerId;
		if(chnl_status.BridgeId.NotEmpty()) {
			PhnSvcChannelStatusPool bp;
			PhnSvcChannelStatus local_status;
			chnl_status_list.GetListWithSameBridge(chnl_status.BridgeId, -1, bp);
			for(uint i = 0; i < bp.GetCount(); i++) {
				if(bp.Get(i, local_status) && !local_status.Channel.IsEqiAscii(chnl_status.Channel)) {
					outer_caller_id = local_status.CallerId;
					break;
				}
			}
		}
		*/
		// } @v10.2.3
		Action = action;
		Oid.Set(PPOBJ_PHONESERVICE, phnSvcID); // @v10.0.02
		Dtm = getcurdatetime_();
		Priority = rS.Priority;
		Duration = rS.Seconds;
		rAev.AddS(rS.Channel, &ChannelP);
		rAev.AddS(rS.CallerId, &CallerIdP);
		rAev.AddS(rS.ConnectedLineNum, &ConnectedLineNumP);
		rAev.AddS(rS.Context, &ContextP); // @v9.9.12
		rAev.AddS(rS.Exten, &ExtenP); // @v9.9.12
		rAev.AddS(rS.BridgeId, &BridgeP); // @v10.0.02
		//rAev.AddS(outer_caller_id, &ev.OuterCallerIdP); // @v10.2.3
		rAev.insert(this);
	}
	return ok;
}

int PPAdviseEvent::ConvertToMqbEnvelope(const PPAdviseEventVector & rAev, PPMqbClient::Envelope & rE) const
{
	int    ok = 1;
	rE.Z();
	if(Action == PPEVNT_MQB_MESSAGE) {
		rE.Msg.Props.TimeStamp = MqbTimeStamp;
		rE.Msg.Props.Priority = Priority;
		rE.ChannelN = MqbChannelN;
		rE.DeliveryTag = MqbDeliveryTag;
		rE.Flags = MqbEnvFlags;
		rE.Msg.Props.Flags = MqbMsgFlags;
		rE.Msg.Props.ContentType = MqbContentType;
		rE.Msg.Props.Encoding = MqbEncoding;
		rE.Msg.Props.DeliveryMode = MqbDeliveryMode;

		rAev.GetS(MqbConsumerTagP, rE.ConsumerTag);
		rAev.GetS(MqbExchangeP, rE.Exchange);
		rAev.GetS(MqbRoutingKeyP, rE.RoutingKey);
		rAev.GetS(MqbCorrelationIdP, rE.Msg.Props.CorrelationId);
		rAev.GetS(MqbReplyToP, rE.Msg.Props.ReplyTo);
		rAev.GetS(MqbExpirationP, rE.Msg.Props.Expiration);
		rAev.GetS(MqbMessageIdP, rE.Msg.Props.MessageId);
		rAev.GetS(MqbTypeP, rE.Msg.Props.Type);
		rAev.GetS(MqbUserIdP, rE.Msg.Props.UserId);
		rAev.GetS(MqbAppIdP, rE.Msg.Props.AppId);
		rAev.GetS(MqbClusterIdP, rE.Msg.Props.ClusterId);
		if(Flags & fMqbExtraIdxIsValid) {
			const PPAdviseEventVector::MqbExtra * p_mqb_extra = rAev.GetMqbExtra(MqbExtraIdx);
			if(p_mqb_extra) {
				SString key_buf;
				SString val_buf;
				for(uint propidx = 0; propidx < p_mqb_extra->PropsPosList.getCount(); propidx++) {
					const LAssoc & r_kv_pos = p_mqb_extra->PropsPosList.at(propidx);
					rAev.GetS(r_kv_pos.Key, key_buf);
					rAev.GetS(r_kv_pos.Val, val_buf);
					if(key_buf.NotEmpty()) {
						rE.Msg.Props.Headers.Add(key_buf, val_buf, 0);
					}
				}
				rE.Msg.Body = p_mqb_extra->Body;
			}
		}
	}
	else
		ok = 0;
	return ok;
}

int PPAdviseEvent::SetupAndAppendToVector(const PPMqbClient::Envelope & rS, PPAdviseEventVector & rAev)
{
	int    ok = 1;
	Action = PPEVNT_MQB_MESSAGE;
	Dtm = getcurdatetime_();
	MqbTimeStamp = rS.Msg.Props.TimeStamp;
	Priority = rS.Msg.Props.Priority;
	MqbChannelN = rS.ChannelN;
	MqbDeliveryTag = rS.DeliveryTag;
	MqbEnvFlags = rS.Flags;
	MqbMsgFlags = rS.Msg.Props.Flags;
	MqbContentType = rS.Msg.Props.ContentType;
	MqbEncoding = rS.Msg.Props.Encoding;
	MqbDeliveryMode = rS.Msg.Props.DeliveryMode;

	rAev.AddS(rS.ConsumerTag, &MqbConsumerTagP);
	rAev.AddS(rS.Exchange, &MqbExchangeP);
	rAev.AddS(rS.RoutingKey, &MqbRoutingKeyP);
	rAev.AddS(rS.Msg.Props.CorrelationId, &MqbCorrelationIdP);
	rAev.AddS(rS.Msg.Props.ReplyTo, &MqbReplyToP);
	rAev.AddS(rS.Msg.Props.Expiration, &MqbExpirationP);
	rAev.AddS(rS.Msg.Props.MessageId, &MqbMessageIdP);
	rAev.AddS(rS.Msg.Props.Type, &MqbTypeP);
	rAev.AddS(rS.Msg.Props.UserId, &MqbUserIdP);
	rAev.AddS(rS.Msg.Props.AppId, &MqbAppIdP);
	rAev.AddS(rS.Msg.Props.ClusterId, &MqbClusterIdP);
	{
		MqbExtraIdx = 0;
		Flags &= ~fMqbExtraIdxIsValid;
		if(rS.Msg.Props.Headers.getCount() || rS.Msg.Body.GetAvailableSize()) {
			PPAdviseEventVector::MqbExtra * p_mqb_extra = rAev.CreateNewMqbExtra(&MqbExtraIdx);
			if(p_mqb_extra) {
				for(uint propidx = 0; propidx < rS.Msg.Props.Headers.getCount(); propidx++) {
					StrStrAssocArray::Item prop_item = rS.Msg.Props.Headers.at(propidx);
					uint   kp = 0;
					uint   vp = 0;
					rAev.AddS(prop_item.Key, &kp);
					rAev.AddS(prop_item.Val, &vp);
					p_mqb_extra->PropsPosList.Add(static_cast<long>(kp), static_cast<long>(vp));
				}
				p_mqb_extra->Body = rS.Msg.Body;
				Flags |= fMqbExtraIdxIsValid;
			}
		}
	}
	rAev.insert(this);
	return ok;
}

PPAdviseEventVector::PPAdviseEventVector()
{
}

void PPAdviseEventVector::Clear()
{
	SVector::clear();
	SStrGroup::ClearS();
	MqbExtraList.freeAll();
}

PPAdviseEventVector::MqbExtra * PPAdviseEventVector::CreateNewMqbExtra(uint * pPos)
{
	return MqbExtraList.CreateNewItem(pPos);
}

const PPAdviseEventVector::MqbExtra * FASTCALL PPAdviseEventVector::GetMqbExtra(uint pos) const
{
	return (pos < MqbExtraList.getCount()) ? MqbExtraList.at(pos) : 0;
}

uint FASTCALL PPAdviseEventVector::MoveItemTo(uint pos, PPAdviseEventVector & rDest) const
{
	uint   result = 0;
	if(pos < getCount()) {
		PPAdviseEvent item = at(pos);
		SString temp_buf;
#define MOVETEXT(fldp) GetS(item.fldp, temp_buf); rDest.AddS(temp_buf, &item.fldp)
		if(item.ChannelP || item.CallerIdP || item.ConnectedLineNumP || item.ContextP || item.ExtenP || item.BridgeP) {
			MOVETEXT(ChannelP);
			MOVETEXT(CallerIdP);
			MOVETEXT(ConnectedLineNumP);
			MOVETEXT(ContextP);
			MOVETEXT(ExtenP);
			MOVETEXT(BridgeP);
		}
		if(item.MqbConsumerTagP || item.MqbExchangeP || item.MqbRoutingKeyP || item.MqbCorrelationIdP || item.MqbReplyToP ||
			item.MqbExpirationP || item.MqbMessageIdP || item.MqbTypeP || item.MqbUserIdP || item.MqbAppIdP || item.MqbClusterIdP)
		{
			MOVETEXT(MqbConsumerTagP);   // @v10.5.7
			MOVETEXT(MqbExchangeP);      // @v10.5.7
			MOVETEXT(MqbRoutingKeyP);    // @v10.5.7
			MOVETEXT(MqbCorrelationIdP); // @v10.5.7
			MOVETEXT(MqbReplyToP);       // @v10.5.7
			MOVETEXT(MqbExpirationP);    // @v10.5.7
			MOVETEXT(MqbMessageIdP);     // @v10.5.7
			MOVETEXT(MqbTypeP);          // @v10.5.7
			MOVETEXT(MqbUserIdP);        // @v10.5.7
			MOVETEXT(MqbAppIdP);         // @v10.5.7
			MOVETEXT(MqbClusterIdP);     // @v10.5.7
		}
#undef MOVETEXT
		if(item.MqbExtraIdx < MqbExtraList.getCount() && item.Flags & item.fMqbExtraIdxIsValid) {
			item.Flags &= ~item.fMqbExtraIdxIsValid;
			const MqbExtra * p_extra = MqbExtraList.at(item.MqbExtraIdx);
			if(p_extra) {
				SString key_buf;
				MqbExtra * p_dest_extra = rDest.MqbExtraList.CreateNewItem(&item.MqbExtraIdx);
				for(uint i = 0; i < p_extra->PropsPosList.getCount(); i++) {
					const LAssoc & r_ai = p_extra->PropsPosList.at(i);
					uint   new_key_pos = 0;
					uint   new_val_pos = 0;
					GetS(r_ai.Key, key_buf);
					rDest.AddS(key_buf, &new_key_pos);
					GetS(r_ai.Val, temp_buf);
					rDest.AddS(temp_buf, &new_val_pos);
					p_dest_extra->PropsPosList.Add(static_cast<long>(new_key_pos), static_cast<long>(new_val_pos));
				}
				p_dest_extra->Body = p_extra->Body;
				item.Flags |= item.fMqbExtraIdxIsValid;
			}
		}
		rDest.insert(&item);
		result = rDest.getCount();
	}
	return result;
}

int PPAdviseEventVector::Pack()
{
	int    ok = -1;
	if(Pool.getDataLen()) {
		void * p_pack_handle = Pack_Start();
		THROW_SL(p_pack_handle);
		const uint c = getCount();
		for(uint i = 0; i < c; i++) {
			PPAdviseEvent & r_item = at(i);
			THROW_SL(Pack_Replace(p_pack_handle, r_item.ChannelP));
			THROW_SL(Pack_Replace(p_pack_handle, r_item.CallerIdP));
			THROW_SL(Pack_Replace(p_pack_handle, r_item.ConnectedLineNumP));
			THROW_SL(Pack_Replace(p_pack_handle, r_item.ContextP));
			THROW_SL(Pack_Replace(p_pack_handle, r_item.ExtenP));
			THROW_SL(Pack_Replace(p_pack_handle, r_item.BridgeP)); // @v10.0.02

			THROW_SL(Pack_Replace(p_pack_handle, r_item.MqbConsumerTagP));   // @v10.5.7
			THROW_SL(Pack_Replace(p_pack_handle, r_item.MqbExchangeP));      // @v10.5.7
			THROW_SL(Pack_Replace(p_pack_handle, r_item.MqbRoutingKeyP));    // @v10.5.7
			THROW_SL(Pack_Replace(p_pack_handle, r_item.MqbCorrelationIdP)); // @v10.5.7
			THROW_SL(Pack_Replace(p_pack_handle, r_item.MqbReplyToP));       // @v10.5.7
			THROW_SL(Pack_Replace(p_pack_handle, r_item.MqbExpirationP));    // @v10.5.7
			THROW_SL(Pack_Replace(p_pack_handle, r_item.MqbMessageIdP));     // @v10.5.7
			THROW_SL(Pack_Replace(p_pack_handle, r_item.MqbTypeP));          // @v10.5.7
			THROW_SL(Pack_Replace(p_pack_handle, r_item.MqbUserIdP));        // @v10.5.7
			THROW_SL(Pack_Replace(p_pack_handle, r_item.MqbAppIdP));         // @v10.5.7
			THROW_SL(Pack_Replace(p_pack_handle, r_item.MqbClusterIdP));     // @v10.5.7
			if(r_item.Flags & r_item.fMqbExtraIdxIsValid && r_item.MqbExtraIdx < MqbExtraList.getCount()) {
				MqbExtra * p_extra = MqbExtraList.at(r_item.MqbExtraIdx);
				if(p_extra) {
					for(uint propidx = 0; propidx < p_extra->PropsPosList.getCount(); propidx++) {
						LAssoc & r_ai = p_extra->PropsPosList.at(propidx);
						uint   key_pos = static_cast<uint>(r_ai.Key);
						uint   val_pos = static_cast<uint>(r_ai.Val);
						THROW_SL(Pack_Replace(p_pack_handle, key_pos));
						THROW_SL(Pack_Replace(p_pack_handle, val_pos));
						r_ai.Key = static_cast<long>(key_pos);
						r_ai.Val = static_cast<long>(val_pos);
					}
				}
			}
		}
		Pack_Finish(p_pack_handle);
		uint extraidx = MqbExtraList.getCount();
		if(extraidx) do {
			extraidx--;
			int extraidx_used = 0;
			for(uint i = 0; !extraidx_used && i < c; i++) {
				const PPAdviseEvent & r_item = at(i);
				if(r_item.Flags & r_item.fMqbExtraIdxIsValid && r_item.MqbExtraIdx == extraidx)
					extraidx_used = 1;
			}
			if(!extraidx_used) {
				for(uint j = 0; j < c; j++) {
					PPAdviseEvent & r_item = at(j);
					if(r_item.Flags & r_item.fMqbExtraIdxIsValid) {
						assert(r_item.MqbExtraIdx != extraidx);
						if(r_item.MqbExtraIdx > extraidx)
							r_item.MqbExtraIdx--;
					}
				}
				MqbExtraList.atFree(extraidx);
			}
		} while(extraidx);
		ok = 1;
	}
	CATCHZOK
	return ok;
}

#define ADVEVQCLISIGN 0x12ABCDEF

PPAdviseEventQueue::Client::Client() : Sign(ADVEVQCLISIGN), Marker(0)
{
}

PPAdviseEventQueue::Client::~Client()
{
	Sign = 0;
}

int PPAdviseEventQueue::Client::IsConsistent() const { return BIN(Sign == ADVEVQCLISIGN); }
int64 PPAdviseEventQueue::Client::GetMarker() const { return Marker; }

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

PPAdviseEventQueue::Stat::Stat() : LivingTime(0), StartClock(0), Push_Count(0), Get_Count(0), GetDecline_Count(0), MaxLength(0),
	State(0), SjPrcCount(0), SjMsgCount(0), PhnSvcPrcCount(0), PhnSvcMsgCount(0), MqbPrcCount(0), MqbMsgCount(0)
{
}

PPAdviseEventQueue::PPAdviseEventQueue() :
	/*TSVector <PPAdviseEvent> ()*/PPAdviseEventVector(), CliList(/*DEFCOLLECTDELTA,*/(aryDataOwner|aryPtrContainer)), LastIdent(0)
{
	S.StartClock = clock();
}

void FASTCALL PPAdviseEventQueue::SetStatFlag(uint f)
{
	if(f & (Stat::stMqbCliInit|Stat::stPhnSvcInit)) {
		SLck.Lock();
		if(f & Stat::stMqbCliInit)
			S.State |= Stat::stMqbCliInit;
		if(f & Stat::stPhnSvcInit)
			S.State |= Stat::stPhnSvcInit;
		SLck.Unlock();
	}
}

void FASTCALL PPAdviseEventQueue::AddStatCounters(const Stat & rAddendum)
{
    SLck.Lock();
	S.SjPrcCount += rAddendum.SjPrcCount;
	S.SjMsgCount += rAddendum.SjMsgCount;
	S.PhnSvcPrcCount += rAddendum.PhnSvcPrcCount;
	S.PhnSvcMsgCount += rAddendum.PhnSvcMsgCount;
	S.MqbPrcCount += rAddendum.MqbPrcCount;
	S.MqbMsgCount += rAddendum.MqbMsgCount;
    SLck.Unlock();
}

int PPAdviseEventQueue::GetStat(PPAdviseEventQueue::Stat & rStat)
{
	int    ok = 1;
    SLck.Lock();
    S.LivingTime = static_cast<int64>(clock()) - S.StartClock;
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
			const Client * p_cli = static_cast<const Client *>(CliList.at(i));
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

int FASTCALL PPAdviseEventQueue::Push(const PPAdviseEventVector & rList)
{
	int    ok = 1;
	uint   ql = 0;
	if(rList.getCount()) {
		SRWLOCKER(Lck, SReadWriteLocker::Write);
		for(uint i = 0; i < rList.getCount(); i++) {
			uint rp = rList.MoveItemTo(i, *this);
			if(rp)
				at(rp-1).Ident = ++LastIdent;
		}
		ql = getCount();
	}
	{
		SLck.Lock();
		S.LivingTime = static_cast<int64>(clock()) - S.StartClock;
		S.Push_Count++;
		SETMAX(S.MaxLength, ql);
		SLck.Unlock();
	}
	return ok;
}

uint PPAdviseEventQueue::GetCount()
{
	uint   c = 0;
	{
		SRWLOCKERTIMEOUT(Lck, SReadWriteLocker::Read, 0);
		if(!!_rwl)
			c = getCount();
	}
	return c;
}

int PPAdviseEventQueue::Get(int64 lowIdent, PPAdviseEventVector & rList)
{
	int    ok = -1;
	int    declined = 0;
	//
	// Попытка получить доступ к очереди на чтение без ожидания.
	// Принцип работы очереди в том, чтобы клиентский поток не ждал пока
	// серверный поток получит очередную порцию данных от источника.
	//
	{
		SRWLOCKERTIMEOUT(Lck, SReadWriteLocker::Read, 0);
		if(!_rwl) {
			declined = 1;
		}
		else {
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
					MoveItemTo(_pos, rList);
				}
			}
			ok = 1;
		}
	}
	{
		SLck.Lock();
		S.Get_Count++;
		if(declined)
			S.GetDecline_Count++;
		SLck.Unlock();
	}
	return ok;
}

int PPAdviseEventQueue::Purge()
{
	int    ok = -1;
	{
		SRWLOCKER(Lck, SReadWriteLocker::Write);
		const  uint _c = getCount();
		if(_c > 1024) {
			int64  marker = 0x0fffffffffffffff;
			{
				ClLck.Lock();
				for(uint i = 0; i < CliList.getCount(); i++) {
					const Client * p_cli = static_cast<const Client *>(CliList.at(i));
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
			PPAdviseEventVector::Pack();
		}
	}
	return ok;
}
//
//
//
SysMaintenanceEventResponder::SysMaintenanceEventResponder() : Signature(_PPConst.Signature_SysMaintenanceEventResponder), AdvCookie(0)
{
	{
		PPAdviseBlock adv_blk;
		adv_blk.Kind = PPAdviseBlock::evQuartz;
		adv_blk.ProcExtPtr = this;
		adv_blk.Proc = SysMaintenanceEventResponder::AdviseCallback;
		DS.Advise(&AdvCookie, &adv_blk);
	}
}

SysMaintenanceEventResponder::~SysMaintenanceEventResponder()
{
}

int SysMaintenanceEventResponder::IsConsistent() const
	{ return (Signature == _PPConst.Signature_SysMaintenanceEventResponder); }

/*static*/int SysMaintenanceEventResponder::AdviseCallback(int kind, const PPNotifyEvent * pEv, void * procExtPtr)
{
	int    ok = -1;
	if(kind == PPAdviseBlock::evQuartz) {
		const double prob_common_mqs_config = 0.000005; // @v10.7.6 0.00001-->0.000005
		// @v10.8.12 {
#ifdef NDEBUG
		const double prob_event_detection   = 0.000020; 
#else
		const double prob_event_detection   = 0.020000;
#endif
		// @v10.8.12 {
		if(SLS.GetTLA().Rg.GetProbabilityEvent(prob_event_detection)) {
			PROFILE_START
			PPObjEventSubscription es_obj(0);
			if(!es_obj.Run())
				PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR|LOGMSGF_DBINFO|LOGMSGF_TIME|LOGMSGF_USER);
			PROFILE_END 
		}
		// } @v10.8.12 
		if(SLS.GetTLA().Rg.GetProbabilityEvent(prob_common_mqs_config)) {
			SString logmsg_buf;
			LDATETIME last_ev_dtm = ZERODATETIME;
			SysJournal * p_sj = DS.GetTLA().P_SysJ;
			if(p_sj) {
				LDATETIME since = ZERODATETIME;
				SysJournalTbl::Rec sj_rec;
				if(p_sj->GetLastEvent(PPACN_SYSMAINTENANCE, SYSMAINTENANCEFUNC_AUTOMQSCONFIG, &since, 3, &sj_rec) > 0) {
					last_ev_dtm.Set(sj_rec.Dt, sj_rec.Tm);
				}
			}
			if(!last_ev_dtm) {
				SString temp_buf;
				SString org_val_buf;
				PPUhttClient uhtt_cli;
				PPAlbatrossConfig temp_alb_cfg;
				int result_uhtt_get = uhtt_cli.GetCommonMqsConfig(temp_alb_cfg);
				int result_cfgmngr_get = 0;
				int result_cfgmngr_put = 0;
				if(result_uhtt_get > 0) {
					PPAlbatrossConfig current_alb_cfg;
					result_cfgmngr_get = PPAlbatrosCfgMngr::Get(&current_alb_cfg);
					if(result_cfgmngr_get != 0) {
						int    do_update = 0;
						const  int8 txt_fld_list[] = { ALBATROSEXSTR_MQC_HOST, ALBATROSEXSTR_MQC_VIRTHOST, ALBATROSEXSTR_MQC_USER };
						for(uint fli = 0; fli < SIZEOFARRAY(txt_fld_list); fli++) {
							const int txt_fld_id = txt_fld_list[fli];
							temp_alb_cfg.GetExtStrData(txt_fld_id, temp_buf);
							current_alb_cfg.GetExtStrData(txt_fld_id, org_val_buf);
							if(temp_buf != org_val_buf) {
								current_alb_cfg.PutExtStrData(txt_fld_id, temp_buf);
								do_update = 1;
							}
						}
						temp_alb_cfg.GetPassword(ALBATROSEXSTR_MQC_SECRET, temp_buf);
						current_alb_cfg.GetPassword(ALBATROSEXSTR_MQC_SECRET, org_val_buf);
						if(temp_buf != org_val_buf) {
							current_alb_cfg.SetPassword(ALBATROSEXSTR_MQC_SECRET, temp_buf);
							do_update = 1;
						}
						if(do_update)
							result_cfgmngr_put = PPAlbatrosCfgMngr::Put(&current_alb_cfg, 1);
						else
							result_cfgmngr_put = -1;
						DS.LogAction(PPACN_SYSMAINTENANCE, PPCFGOBJ_ALBATROS, 0, SYSMAINTENANCEFUNC_AUTOMQSCONFIG, 1);
					}
				}
				PPLoadText(PPTXT_LOG_SYSMNTNC_MQSCONFIG, logmsg_buf);
				temp_buf.Z().CatEq("probability", prob_common_mqs_config, MKSFMTD(0, 8, 0)).Space().CatEq("uhtt_get", static_cast<long>(result_uhtt_get)).Space().
					CatEq("cfgmgr_get", static_cast<long>(result_cfgmngr_get)).Space().CatEq("cfgmgr_put", static_cast<long>(result_cfgmngr_put));
				logmsg_buf.CatDiv(':', 2).Cat(temp_buf);
				PPLogMessage(PPFILNAM_INFO_LOG, logmsg_buf, LOGMSGF_TIME|LOGMSGF_USER|LOGMSGF_COMP|LOGMSGF_DBINFO);
			}
			else {
				PPLoadText(PPTXT_LOG_SYSMNTNC_MQSCONFIG_SKIPTE, logmsg_buf);
				logmsg_buf.CatDiv(':', 2).Cat(last_ev_dtm, DATF_ISO8601|DATF_CENTURY, 0);
				PPLogMessage(PPFILNAM_INFO_LOG, logmsg_buf, LOGMSGF_TIME|LOGMSGF_USER|LOGMSGF_COMP|LOGMSGF_DBINFO);
			}
		}
	}
	return ok;
}

/*static*/int SysMaintenanceEventResponder::ResponseByAdviseCallback(const SString & rResponseMsg, const PPMqbClient::Envelope * pEnv, SysMaintenanceEventResponder * pSelf, SString &rDomainBuf)
{
	return -1;
}
//
//
//
#if SLTEST_RUNNING // {

SLTEST_R(ObjTypeSymb)
{
	struct Test_ObjSymbEntry {
		const  char * P_Symb;
		long   Id;
		long   HsId;
	};
	static Test_ObjSymbEntry __Test_ObjSymbList[] = {
		{ "UNIT",           PPOBJ_UNIT,          PPHS_UNIT },
		{ "QUOTKIND",       PPOBJ_QUOTKIND,      PPHS_QUOTKIND },
		{ "LOCATION",       PPOBJ_LOCATION,      PPHS_LOCATION },
		{ "GOODS",          PPOBJ_GOODS,         PPHS_GOODS },
		{ "GOODSGROUP",     PPOBJ_GOODSGROUP,    PPHS_GOODSGROUP },
		{ "BRAND",          PPOBJ_BRAND,         PPHS_BRAND },
		{ "GOODSTYPE",      PPOBJ_GOODSTYPE,     PPHS_GOODSTYPE },
		{ "GOODSCLASS",     PPOBJ_GOODSCLASS,    PPHS_GOODSCLASS },
		{ "GOODSARCODE",    PPOBJ_GOODSARCODE,   PPHS_GOODSARCODE },
		{ "PERSON",         PPOBJ_PERSON,        PPHS_PERSON },
		{ "PERSONKIND",     PPOBJ_PERSONKIND,      PPHS_PERSONKIND },
		{ "PERSONSTATUS",   PPOBJ_PRSNSTATUS,    PPHS_PERSONSTATUS },
		{ "PERSONCATEGORY", PPOBJ_PRSNCATEGORY,  PPHS_PERSONCATEGORY },
		{ "GLOBALUSER",     PPOBJ_GLOBALUSERACC, PPHS_GLOBALUSER },
		{ "DL600",          PPOBJ_DL600DATA,     PPHS_DL600 },
		{ "WORLD",          PPOBJ_WORLD,         PPHS_WORLD },
		{ "CITY",           PPOBJ_WORLD | (WORLDOBJ_CITY << 16),    PPHS_CITY },
		{ "COUNTRY",        PPOBJ_WORLD | (WORLDOBJ_COUNTRY << 16), PPHS_COUNTRY },
		{ "QUOT",           PPOBJ_QUOT2,         PPHS_QUOT },
		{ "CURRENCY",       PPOBJ_CURRENCY,      PPHS_CURRENCY },
		{ "CURRATETYPE",    PPOBJ_CURRATETYPE,   PPHS_CURRATETYPE },
		{ "SPECSERIES",     PPOBJ_SPECSERIES,    PPHS_SPECSERIES },
		{ "SCARD",          PPOBJ_SCARD,         PPHS_SCARD },
		{ "SCARDSERIES",    PPOBJ_SCARDSERIES,   PPHS_SCARDSERIES },
		{ "POSNODE",        PPOBJ_CASHNODE,      PPHS_POSNODE },
		{ "CURRATEIDENT",   PPOBJ_CURRATEIDENT,  PPHS_CURRATEIDENT },
		{ "UHTTSCARDOP",    PPOBJ_UHTTSCARDOP,   PPHS_UHTTSCARDOP },
		{ "LOT",            PPOBJ_LOT,           PPHS_LOT },
		{ "BILL",           PPOBJ_BILL,          PPHS_BILL },
		{ "UHTTSTORE",      PPOBJ_UHTTSTORE,     PPHS_UHTTSTORE },
		{ "OPRKIND",        PPOBJ_OPRKIND,       PPHS_OPRKIND },
		{ "WORKBOOK",       PPOBJ_WORKBOOK,      PPHS_WORKBOOK },
		{ "CCHECK",         PPOBJ_CCHECK,        PPHS_CCHECK },
		{ "PROCESSOR",      PPOBJ_PROCESSOR,     PPHS_PROCESSOR },
		{ "TSESSION",       PPOBJ_TSESSION,      PPHS_TSESSION },
		{ "STYLOPALM",      PPOBJ_STYLOPALM,     PPHS_STYLOPALM },
		{ "STYLODEVICE",    PPOBJ_STYLOPALM,     PPHS_STYLODEVICE }
	};

	int    ok = 1;
	SString temp_buf;
	SString symb;
	long   ext_param = 0;
	PPID   obj_type = 0;
	for(uint i = 0; i < SIZEOFARRAY(__Test_ObjSymbList); i++) {
		Test_ObjSymbEntry & r_entry = __Test_ObjSymbList[i];
		ext_param = 0;
		obj_type = 0;
		{
			temp_buf = r_entry.P_Symb;

			obj_type = DS.GetObjectTypeBySymb(temp_buf, &ext_param);
			SLTEST_CHECK_EQ(r_entry.Id, MakeLong(obj_type, ext_param));
			if(r_entry.HsId != PPHS_STYLODEVICE) { // Дублированный (запасной) символ
				SLTEST_CHECK_LT(0L, DS.GetObjectTypeSymb(r_entry.Id, symb));
				SLTEST_CHECK_NZ(sstreqi_ascii(symb, temp_buf));
			}
		}
		{
			(temp_buf = r_entry.P_Symb).ToLower();

			obj_type = DS.GetObjectTypeBySymb(temp_buf, &ext_param);
			SLTEST_CHECK_EQ(r_entry.Id, MakeLong(obj_type, ext_param));
			if(r_entry.HsId != PPHS_STYLODEVICE) { // Дублированный (запасной) символ
				SLTEST_CHECK_LT(0L, DS.GetObjectTypeSymb(r_entry.Id, symb));
				SLTEST_CHECK_NZ(sstreqi_ascii(symb, temp_buf));
			}
		}
	}
	{
		ext_param = 0;
		obj_type = 0;
		{
			temp_buf = "CANTRY";

			obj_type = DS.GetObjectTypeBySymb(temp_buf, &ext_param);
			SLTEST_CHECK_EQ(0L, MakeLong(obj_type, ext_param));

			symb = "abracadabra";
			SLTEST_CHECK_EQ(0L, DS.GetObjectTypeSymb(31139, symb));
			SLTEST_CHECK_NZ(symb.IsEmpty());
		}
		{
			temp_buf = "id";

			obj_type = DS.GetObjectTypeBySymb(temp_buf, &ext_param);
			SLTEST_CHECK_EQ(0L, MakeLong(obj_type, ext_param));

			symb = "abracadabra";
			SLTEST_CHECK_EQ(0L, DS.GetObjectTypeSymb(31139, symb));
			SLTEST_CHECK_NZ(symb.IsEmpty());
		}
	}
	return CurrentStatus;
}

#endif // } SLTEST_RUNNING
//
//
//
#pragma warning(disable:4073)
#pragma init_seg(user)
PPSession  DS; // @global
