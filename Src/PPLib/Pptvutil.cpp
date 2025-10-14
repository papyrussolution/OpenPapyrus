// PPTVUTIL.CPP
// Copyright (c) A.Sobolev 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021, 2022, 2023, 2024, 2025
// @codepage UTF-8
//
#include <pp.h>
#pragma hdrstop
#define HMONITOR_DECLARED
#include <shlobj.h>
#include <htmlhelp.h>
#pragma comment(lib, "htmlhelp.lib")

TWindow * PPApp::FindPhonePaneDialog()
{
	const long res_id = DLG_PHNCPANE;
	for(TView * p = P_DeskTop->GetFirstView(); p != 0; p = p->nextView()) {
		if(p->IsConsistent() && p->GetSubSign() == TV_SUBSIGN_DIALOG && static_cast<const TDialog *>(p)->resourceID == res_id)
			return static_cast<TWindow *>(p);
	}
	return 0;
}

bool FASTCALL GetModelessStatus(bool outerModeless) { return outerModeless; }
TView * ValidView(TView * pView) { return APPL->validView(pView); }
ushort FASTCALL ExecView(TWindow * pView) { return pView ? APPL->P_DeskTop->execView(pView) : cmError; }

ushort FASTCALL ExecViewAndDestroy(TWindow * pView)
{
	ushort r = pView ? APPL->P_DeskTop->execView(pView) : cmError;
	delete pView;
	return r;
}

ushort STDCALL CheckExecAndDestroyDialog(TDialog * pDlg, int genErrMsg, int toCascade)
{
	ushort ret = 0;
	if(genErrMsg ? CheckDialogPtrErr(&pDlg) : CheckDialogPtr(&pDlg)) {
		if(toCascade)
			pDlg->ToCascade();
		ret = ExecViewAndDestroy(pDlg);
	}
	return ret;
}

ushort FASTCALL ExecView(TBaseBrowserWindow * v)
{
	return v ? APPL->P_DeskTop->execView(v) : cmError;
}

int FASTCALL InsertView(TBaseBrowserWindow * v)
{
	if(v) {
		APPL->P_DeskTop->Insert_(v);
		return v->Insert();
	}
	else
		return 0;
}

int InitSTimeChunkBrowserParam(const char * pSymbol, STimeChunkBrowser::Param * pParam)
{
	int    ok = 1;
	if(pParam) {
		pParam->Z();
		pParam->RegSaveParam = pSymbol;
		{
			if(APPL->GetUiSettings().Flags & UserInterfaceSettings::fTcbInterlaced)
				pParam->Flags |= pParam->fInterlaced;
		}
		pParam->Flags |= pParam->fUseToolTip;
	}
	else
		ok = 0;
	return ok;
}
//
// В следующих трех функциях проверка не ненулевой APPL сделана из-за того, что
// функции эти могут вызываться в контексте JobServer'а
//
BrowserWindow * PPFindLastBrowser() { return APPL ? static_cast<BrowserWindow *>(APPL->FindBrowser(static_cast<PPApp *>(APPL)->LastCmd, 0)) : 0; }
STimeChunkBrowser * PPFindLastTimeChunkBrowser() { return APPL ? static_cast<STimeChunkBrowser *>(APPL->FindBrowser(static_cast<PPApp *>(APPL)->LastCmd, 1)) : 0; }
PPPaintCloth * PPFindLastPaintCloth() { return APPL ? (PPPaintCloth*)APPL->FindBrowser(static_cast<PPApp *>(APPL)->LastCmd, 2) : 0; }
static STextBrowser * PPFindLastTextBrowser(const char * pFileName) { return APPL ? static_cast<STextBrowser *>(APPL->FindBrowser(static_cast<PPApp *>(APPL)->LastCmd, 3, pFileName)) : 0; }

void PPViewTextBrowser(const char * pFileName, const char * pTitle, const char * pLexerSymb, int toolbarId)
{
	STextBrowser * p_brw = PPFindLastTextBrowser(pFileName);
	if(p_brw) {
		PPCloseBrowser(p_brw);
		p_brw = 0;
	}
	p_brw = new STextBrowser(pFileName, pLexerSymb, toolbarId);
	{
		SString title_buf;
		if(!isempty(pTitle))
			title_buf = pTitle;
		else {
			SFsPath ps(pFileName);
			ps.Merge(SFsPath::fNam|SFsPath::fExt, title_buf);
		}
		p_brw->setTitle(title_buf);
	}
#ifndef NDEBUG
	p_brw->SetSpecialMode(STextBrowser::spcmSartrTest);
	{
		KeyDownCommand k;
		k.SetTvKeyCode(kbCtrlB);
		p_brw->SetKeybAccelerator(k, PPVCMD_SETUPSARTREINDICATORS);
	}
#endif
	InsertView(p_brw);
}

int FASTCALL PPOpenBrowser(BrowserWindow * pW, int modeless)
{
	int    ok = cmCancel;
	if(modeless) {
		pW->SetResID(static_cast<PPApp *>(APPL)->LastCmd);
		ok = InsertView(pW);
	}
	else
		ok = ExecView(pW);
	return ok;
}

void FASTCALL PPCloseBrowser(TBaseBrowserWindow * pW) { CALLPTRMEMB(pW, endModal(cmCancel)); }
uint STDCALL GetComboBoxLinkID(TDialog * dlg, uint comboBoxCtlID) { return dlg->getCtrlView(comboBoxCtlID)->GetId(); }

int STDCALL SetComboBoxLinkText(TDialog * dlg, uint comboBoxCtlID, const char * pText)
{
	ComboBox * p_combo = static_cast<ComboBox *>(dlg->getCtrlView(comboBoxCtlID));
	if(p_combo) {
		p_combo->setInputLineText(pText);
		return 1;
	}
	else
		return 0;
}

int STDCALL SetComboBoxListText(TDialog * dlg, uint comboBoxCtlID)
{
	SString temp_buf;
	PPLoadString("list", temp_buf);
	return SetComboBoxLinkText(dlg, comboBoxCtlID, temp_buf);
}

SString & FASTCALL PPFormatPeriod(const DateRange * pPeriod, SString & rBuf)
{
	// @v12.2.4 usage PPLoadStringS("daterange_from", SLS.AcquireRvlStr()) and PPLoadStringS("daterange_to", SLS.AcquireRvlStr()) insted russian text
	rBuf.Z();
	if(pPeriod) {
		LDATE  beg = pPeriod->low;
		LDATE  end = pPeriod->upp;
		if(beg) {
			if(beg != end) {
				rBuf.Cat(PPLoadStringS("daterange_from", SLS.AcquireRvlStr())).Space();
			}
			rBuf.Cat(beg, DATF_DMY);
		}
		if(end && beg != end) {
			if(beg)
				rBuf.Space();
			rBuf.Cat(PPLoadStringS("daterange_to", SLS.AcquireRvlStr())).Space().Cat(end, DATF_DMY);
		}
	}
	// @v12.2.4 return rBuf.Transf(CTRANSF_OUTER_TO_INNER);
	return rBuf; // @v12.2.4
}

SString & FASTCALL PPFormatPeriod(const LDATETIME & rBeg, const LDATETIME & rEnd, SString & rBuf)
{
	// @v12.2.4 usage PPLoadStringS("daterange_from", SLS.AcquireRvlStr()) and PPLoadStringS("daterange_to", SLS.AcquireRvlStr()) insted russian text
	rBuf.Z();
	if(rBeg.d) {
		if(rBeg.d != rEnd.d) {
			rBuf.Cat(PPLoadStringS("daterange_from", SLS.AcquireRvlStr())).Space();
		}
		rBuf.Cat(rBeg.d, DATF_DMY);
		if(rBeg.t)
			rBuf.Space().Cat(rBeg.t, TIMF_HMS);
	}
	if(rEnd.d && rBeg.d != rEnd.d) {
		if(rBeg.d)
			rBuf.Space();
		rBuf.Cat(PPLoadStringS("daterange_to", SLS.AcquireRvlStr())).Space().Cat(rEnd.d, DATF_DMY);
		if(rEnd.t)
			rBuf.Space().Cat(rEnd.t, TIMF_HMS);
	}
	// @v12.2.4 return rBuf.Transf(CTRANSF_OUTER_TO_INNER);
	return rBuf; // @v12.2.4
}

void STDCALL SetPeriodInput(TDialog * dlg, uint fldID, const DateRange & rPeriod)
{
	if(dlg) {
		// @v12.3.7 char   b[64];
		// @v12.3.7 b[0] = 0;
		// @v12.3.7 periodfmt(rPeriod, b);
		SString temp_buf;
		rPeriod.ToStr(0, temp_buf);
		dlg->setCtrlString(fldID, temp_buf);
	}
}

static int Helper_GetPeriodInput(TDialog * dlg, uint fldID, DateRange * pPeriod, long strtoperiodFlags)
{
	int    ok = -1;
	char   b[64];
	b[0] = 0;
	if(dlg && dlg->getCtrlData(fldID, b)) {
		if(pPeriod->FromStr(b, strtoperiodFlags)) {
			if(checkdate(pPeriod->low, 1) && checkdate(pPeriod->upp, 1)) {
				const LDATE a_low = pPeriod->low.getactual(ZERODATE);
				const LDATE a_upp = pPeriod->upp.getactual(ZERODATE);
				ok = BIN(!a_upp || !a_low || diffdate(a_upp, a_low) >= 0);
			}
			else
				ok = 0;
		}
		else
			ok = 0;
		if(!ok) {
			dlg->selectCtrl(fldID);
			ok = PPSetError(PPERR_INVPERIODINPUT, b);
		}
	}
	return ok;
}

int    STDCALL GetPeriodInput(TDialog * dlg, uint fldID, DateRange * pPeriod) { return Helper_GetPeriodInput(dlg, fldID, pPeriod, 0); }
int    STDCALL GetPeriodInput(TDialog * dlg, uint fldID, DateRange * pPeriod, long strtoperiodFlags) { return Helper_GetPeriodInput(dlg, fldID, pPeriod, strtoperiodFlags); }

void STDCALL SetTimeRangeInput(TDialog * pDlg, uint ctl, long fmt, const TimeRange * pTimePeriod)
{
	SString buf;
	if(pTimePeriod && !pTimePeriod->IsZero())
		pTimePeriod->ToStr(fmt, buf);
	CALLPTRMEMB(pDlg, setCtrlString(ctl, buf));
}

void STDCALL SetTimeRangeInput(TDialog * pDlg, uint ctl, long fmt, const LTIME * pLow, const LTIME * pUpp)
{
	TimeRange tr;
	tr.Set(DEREFPTROR(pLow, ZEROTIME), DEREFPTROR(pUpp, ZEROTIME));
	SetTimeRangeInput(pDlg, ctl, fmt, &tr);
}

int STDCALL GetTimeRangeInput(TDialog * pDlg, uint ctl, long fmt, TimeRange * pTimePeriod)
{
	int    ok = -1;
	TimeRange prd;
	if(GetTimeRangeInput(pDlg, ctl, fmt, &prd.low, &prd.upp) > 0) {
		ok = 1;
		ASSIGN_PTR(pTimePeriod, prd);
	}
	return ok;
}

int STDCALL GetTimeRangeInput(TDialog * pDlg, uint ctl, long fmt, LTIME * pLow, LTIME * pUpp)
{
	int    ok = -1;
	if(pDlg) {
		size_t pos = 0;
		long   format = NZOR(fmt, TIMF_HMS);
		LTIME  low = ZEROTIME, upp = ZEROTIME;
		SString buf;
		pDlg->getCtrlString(ctl, buf);
		if(buf.Search("..", 0, 1, &pos) > 0 || buf.Search(",,", 0, 1, &pos) > 0) {
			const size_t delim_len = 2;
			SString tm_buf;
			buf.Sub(0, pos, tm_buf);
			strtotime(tm_buf, format, &low);
			buf.Sub(pos + delim_len, buf.Len() - pos - delim_len, tm_buf.Z());
			strtotime(tm_buf, format, &upp);
		}
		else {
			strtotime(buf, format, &low);
			upp = low;
		}
		THROW_SL(checktime(low) > 0 && checktime(upp) > 0);
		ASSIGN_PTR(pLow, low);
		ASSIGN_PTR(pUpp, upp);
		ok = 1;
	}
	CATCHZOK
	return ok;
}

int FASTCALL SetRealRangeInput(TDialog * dlg, uint ctl, double lo, double up, int prc)
{
	char   buf[256];
	char * b = buf;
	if(lo == up && lo == 0)
		b[0] = 0;
	else {
		long   flags = NMBF_NOZERO|NMBF_TRICOMMA;
		SETSFMTPRC(flags, prc);
		b += sstrlen(realfmt(lo, flags, b));
		if(up != lo) {
			*b++ = '.';
			*b++ = '.';
			realfmt(up, flags, b);
		}
	}
	dlg->setCtrlData(ctl, buf);
	return 1;
}

int FASTCALL GetRealRangeInput(TDialog * dlg, uint ctl, double * pLow, double * pUpp)
{
	char   buf[256];
	double low = 0.0;
	double upp = 0.0;
	if(dlg->getCtrlData(ctl, memzero(buf, sizeof(buf)))) {
		strtorrng(buf, &low, &upp);
		ASSIGN_PTR(pLow, low);
		ASSIGN_PTR(pUpp, upp);
		return 1;
	}
	else
		return 0;
}

int FASTCALL SetRealRangeInput(TDialog * dlg, uint ctl, const RealRange * pRng, int prc) { return SetRealRangeInput(dlg, ctl, pRng->low, pRng->upp, prc); }
int FASTCALL GetRealRangeInput(TDialog * dlg, uint ctl, RealRange * pRng) { return GetRealRangeInput(dlg, ctl, pRng ? &pRng->low : 0, pRng ? &pRng->upp : 0); }

int FASTCALL SetIntRangeInput(TDialog * dlg, uint ctl, const IntRange * pR)
{
	SString temp_buf;
	return dlg->setCtrlString(ctl, pR->Format(0, temp_buf));
}

int FASTCALL GetIntRangeInput(TDialog * dlg, uint ctl, IntRange * pR)
{
	SString temp_buf;
	if(dlg->getCtrlString(ctl, temp_buf)) {
		// @v12.2.1 temp_buf.ToIntRange(*pR, SString::torfAny);
		// @v12.2.1 {
		if(temp_buf.NotEmptyS())
			temp_buf.ToIntRange(*pR, SString::torfAny);
		else {
			pR->Z();
		}
		// } @v12.2.1 
		return 1;
	}
	else
		return 0;
}

int PPExecuteContextMenu(TView * pView, uint menuID)
{
	int    ok = -1;
	if(pView && pView->P_Owner) {
		TVRez * p_rez = P_SlRez;
		if(p_rez) {
			uint   cmd = 0;
			uint   key = 0;
			{
				uint   cnt = 0;
				SString descr;
				TMenuPopup menu;
				THROW_PP(p_rez->findResource(menuID, PP_RCDECLCTRLMENU), PPERR_RESFAULT);
				cnt = p_rez->getUINT();
				for(uint i = 0; i < cnt; i++) {
					p_rez->getString(descr, 2);
					SLS.ExpandString(descr, CTRANSF_UTF8_TO_INNER);
					const uint key_code = p_rez->getLONG();
					const uint cmd_code = p_rez->getLONG();
					menu.Add(descr.Transf(CTRANSF_INNER_TO_OUTER), cmd_code, key_code);
				}
				menu.Execute(pView->getHandle(), TMenuPopup::efRet, &cmd, &key);
			}
			if(cmd) {
				TView::messageCommand(pView->P_Owner, cmd);
				ok = 1;
			}
			else if(key) {
				TView::messageKeyDown(pView->P_Owner, cmd);
				ok = 1;
			}
		}
	}
	CATCHZOK
	return ok;
}

int FASTCALL PPSetupCtrlMenu(TDialog * pDlg, uint ctl, uint ctlButton, uint ctrlMenuID)
{
	int    ok = 1;
	if(pDlg) {
		TView * p_ctrl = pDlg->getCtrlView(ctl);
		TView * p_button = pDlg->getCtrlView(ctlButton);
		if(p_ctrl && p_button) {
			TVRez * p_rez = P_SlRez;
			if(p_rez) {
				uint   cnt = 0;
				long   key_code = 0;
				long   cmd_code = 0;
				SString descr;
				THROW_PP(p_rez->findResource(ctrlMenuID, PP_RCDECLCTRLMENU), PPERR_RESFAULT);
				cnt = p_rez->getUINT();
				for(uint i = 0; i < cnt; i++) {
					p_rez->getString(descr, 2);
					SLS.ExpandString(descr, CTRANSF_UTF8_TO_INNER);
					key_code = p_rez->getLONG();
					cmd_code = p_rez->getLONG();
					pDlg->AddLocalMenuItem(ctl, ctlButton, key_code, descr.Transf(CTRANSF_INNER_TO_OUTER));
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

void FASTCALL DisableOKButton(TDialog * dlg)
{
	if(dlg) {
		dlg->enableCommand(cmOK, 0);
		dlg->SetDefaultButton(STDCTL_CANCELBUTTON, true);
	}
}

int STDCALL SetupGeoLocButton(TDialog * pDlg, uint inputCtlId, uint btnCmd) // @v11.6.2 @construction
{
	int    ok = -1;
	if(pDlg && btnCmd) {
		if(inputCtlId) {
			/*
			const  PPID def_phn_svc_id = DS.GetConstTLA().DefPhnSvcID;
			if(def_phn_svc_id) {
				SString temp_buf;
				pDlg->getCtrlString(inputCtlId, temp_buf);
				if(temp_buf.NotEmptyS()) {
					SString phone_buf;
					temp_buf.Transf(CTRANSF_INNER_TO_UTF8).Utf8ToLower();
					PPEAddr::Phone::NormalizeStr(temp_buf, 0, phone_buf);
					if(phone_buf.Len() >= 5)
						ok = 1;
				}
			}
			*/
			ok = 1;
		}
		if(ok > 0) {
			pDlg->showButton(btnCmd, 1);
			pDlg->setButtonBitmap(btnCmd, PPDV_LOGOGOOGLEMAPS01);
		}
		else
			pDlg->showButton(btnCmd, 0);
	}
	return ok;
}

int STDCALL SetupPhoneButton(TDialog * pDlg, uint inputCtlId, uint btnCmd)
{
	int    ok = -1;
	if(pDlg && btnCmd) {
		if(inputCtlId) {
			const  PPID def_phn_svc_id = DS.GetConstTLA().DefPhnSvcID;
			if(def_phn_svc_id) {
				SString temp_buf;
				pDlg->getCtrlString(inputCtlId, temp_buf);
				if(temp_buf.NotEmptyS()) {
					SString phone_buf;
					temp_buf.Transf(CTRANSF_INNER_TO_UTF8).Utf8ToLower();
					PPEAddr::Phone::NormalizeStr(temp_buf, 0, phone_buf);
					if(phone_buf.Len() >= 5)
						ok = 1;
				}
			}
		}
		if(ok > 0) {
			pDlg->showButton(btnCmd, 1);
			pDlg->setButtonBitmap(btnCmd, IDB_PHONEFORWARDED);
		}
		else
			pDlg->showButton(btnCmd, 0);
	}
	return ok;
}

void ViewAsyncEventQueueStat()
{
	class AsyncEventQueueStatDialog : public TDialog {
	public:
		AsyncEventQueueStatDialog() : TDialog(DLG_AEQSTAT), T(5000)
		{
			Setup();
		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(TVCMD == cmIdle) {
				if(T.Check(0)) {
					Setup();
					T.Restart(5000);
				}
			}
		}
		void   Setup()
		{
			PPAdviseEventQueue::Stat stat;
			PPAdviseEventQueue * p_queue = DS.GetAdviseEventQueue(0);
			CALLPTRMEMB(p_queue, GetStat(stat));
			setCtrlLong(CTL_AEQSTAT_LIVING, (long)stat.LivingTime);
			setCtrlLong(CTL_AEQSTAT_GETCOUNT, stat.Get_Count);
			setCtrlLong(CTL_AEQSTAT_GETDECLCOUNT, stat.GetDecline_Count);
			setCtrlLong(CTL_AEQSTAT_PUTCOUNT, stat.Push_Count);
			setCtrlLong(CTL_AEQSTAT_MAXQUEUE, stat.MaxLength);
			{
				SString temp_buf;
				SString info_buf;
				if(stat.State & stat.stPhnSvcInit) {
					PPLoadString("aeqstat_stphnsvcinit", temp_buf);
					info_buf.CatDivIfNotEmpty(' ', 0).Cat(temp_buf);
				}
				if(stat.State & stat.stMqbCliInit) {
					PPLoadString("aeqstat_stmqbcliinit", temp_buf);
					info_buf.CatDivIfNotEmpty(' ', 0).Cat(temp_buf);
				}
				setCtrlString(CTL_AEQSTAT_STATEINFO, info_buf);
			}
			setCtrlLong(CTL_AEQSTAT_SJPRCCOUNT, stat.SjPrcCount);
			setCtrlLong(CTL_AEQSTAT_SJMSGCOUNT, stat.SjMsgCount);
			setCtrlLong(CTL_AEQSTAT_PHNPRCCOUNT, stat.PhnSvcPrcCount);
			setCtrlLong(CTL_AEQSTAT_PHNMSGCOUNT, stat.PhnSvcMsgCount);
			setCtrlLong(CTL_AEQSTAT_MQBPRCCOUNT, stat.MqbPrcCount);
			setCtrlLong(CTL_AEQSTAT_MQBMSGCOUNT, stat.MqbMsgCount);
		}
		SCycleTimer T;
	};
	AsyncEventQueueStatDialog * dlg = new AsyncEventQueueStatDialog;
	if(CheckDialogPtrErr(&dlg)) {
		ExecViewAndDestroy(dlg);
	}
}

int ViewStatus()
{
	struct CtrlToPathMapEntry {
		uint16  CtlId;
		uint16  PathID;
	};
	static const CtrlToPathMapEntry ctrl_to_path_map[] = {
		{ CTL_STATUS_BINPATH,  PPPATH_BIN },
		{ CTL_STATUS_INPATH,   PPPATH_IN },
		{ CTL_STATUS_OUTPATH,  PPPATH_OUT },
		{ CTL_STATUS_TEMPPATH, PPPATH_TEMP },
		{ CTL_STATUS_LOGPATH,  PPPATH_LOG } // @v11.0.0
	};
	class StatusDialog : public TDialog {
		enum {
			dummyFirst = 1,
			brushValidPath,
			brushInvalidPath
		};
	public:
		StatusDialog() : TDialog(DLG_STATUS)
		{
			Ptb.SetBrush(brushValidPath,   SPaintObj::bsSolid, GetColorRef(SClrAqua),  0);
			Ptb.SetBrush(brushInvalidPath, SPaintObj::bsSolid, GetColorRef(SClrCoral), 0);
			SetCtrlBitmap(CTL_STATUS_IMG, BM_PICT_STATUS);
			PPAdviseEventQueue * p_queue = DS.GetAdviseEventQueue(0);
			enableCommand(cmAeqStat, BIN(p_queue));
		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(event.isCmd(cmAeqStat)) {
				ViewAsyncEventQueueStat();
				clearEvent(event);
			}
			else if(event.isCmd(cmCtlColor)) {
				TDrawCtrlData * p_dc = static_cast<TDrawCtrlData *>(TVINFOPTR);
				if(p_dc) {
					//static const uint16 ctl_list[] = {CTL_STATUS_BINPATH, CTL_STATUS_INPATH, CTL_STATUS_OUTPATH, CTL_STATUS_TEMPPATH};
					SString path;
					for(uint i = 0; i < SIZEOFARRAY(/*ctl_list*/ctrl_to_path_map); i++) {
						const uint16 ctl_id = /*ctl_list*/ctrl_to_path_map[i].CtlId;
						if(p_dc->H_Ctl == getCtrlHandle(ctl_id)) {
							getCtrlString(ctl_id, path);
							int tool_id = SFile::IsDir(path.RmvLastSlash()) ? brushValidPath : brushInvalidPath;
							if(tool_id) {
								::SetBkMode(p_dc->H_DC, TRANSPARENT);
								p_dc->H_Br = static_cast<HBRUSH>(Ptb.Get(tool_id));
							}
							clearEvent(event);
							break;
						}
					}
				}
			}
		}
		SPaintToolBox Ptb;
	};
	int    ok = 1;
	DbProvider * p_dict = CurDict;
	PPID   main_org_id = 0;
	SString sbuf;
	SString datapath;
	LDATE  oper_dt = LConfig.OperDate;
	StatusDialog * dlg = new StatusDialog();
	THROW(CheckDialogPtr(&dlg));
	dlg->setCtrlString(CTL_STATUS_USR, GetCurUserName(sbuf));
	GetLocationName(LConfig.Location, sbuf);
	dlg->setCtrlString(CTL_STATUS_LOC, sbuf);
	GetMainOrgID(&main_org_id);
	SetupPPObjCombo(dlg, CTLSEL_STATUS_MAINORG, PPOBJ_PERSON, main_org_id, 0, reinterpret_cast<void *>(PPPRK_MAIN));
	dlg->setCtrlData(CTL_STATUS_DATE, &oper_dt);
	GetObjectName(PPOBJ_DBDIV, LConfig.DBDiv, sbuf);
	dlg->setCtrlString(CTL_STATUS_DBDIV, sbuf);
	if(p_dict) {
		p_dict->GetDbSymb(sbuf);
		p_dict->GetDataPath(datapath);
	}
	else {
		sbuf.Z();
		datapath.Z();
	}
	dlg->setCtrlString(CTL_STATUS_DBSYMBOL, sbuf);
	dlg->setCtrlString(CTL_STATUS_DATAPATH, datapath.Transf(CTRANSF_OUTER_TO_INNER));
	for(uint i = 0; i < SIZEOFARRAY(ctrl_to_path_map); i++) {
		const CtrlToPathMapEntry & r_map_entry = ctrl_to_path_map[i];
		PPGetPath(r_map_entry.PathID, datapath);
		dlg->setCtrlString(r_map_entry.CtlId, datapath.Transf(CTRANSF_OUTER_TO_INNER));
	}
	if(LConfig.Flags & CFGFLG_USEGOODSMATRIX) {
		PPLoadText(PPTXT_GOODSMATRIX_IS_USED, sbuf);
		dlg->setStaticText(CTL_STATUS_USEGDSMATRIX, sbuf);
	}
	if(ExecView(dlg) == cmOK) {
		dlg->getCtrlData(CTLSEL_STATUS_MAINORG, &main_org_id);
		THROW(DS.SetMainOrgID(main_org_id, 1));
		StatusWinChange();
	}
	CATCHZOKPPERR
	delete dlg;
	return ok;
}

int STDCALL SetupDBEntryComboBox(TDialog * dlg, uint ctl, const PPDbEntrySet2 * pDbes, const LongArray * pDbesIdxList)
{
	int    ok = 1;
	ComboBox * cb = static_cast<ComboBox *>(dlg->getCtrlView(ctl));
	if(cb && pDbes && pDbes->GetCount()) {
		StrAssocArray * p_list = new StrAssocArray;
		pDbes->MakeList(p_list, DbLoginBlockArray::loUseFriendlyName, pDbesIdxList);
		ListWindow * p_lw = CreateListWindow(p_list, lbtDblClkNotify | lbtFocNotify | lbtDisposeData);
		cb->setListWindow(p_lw, pDbes->GetSelection());
	}
	return ok;
}

int STDCALL SetupDBTableComboBox(TDialog * dlg, uint ctl, PPDbEntrySet2 * pDbes, long dbID, BTBLID tblID)
{
	int    ok = 1;
	ComboBox * cb = static_cast<ComboBox *>(dlg->getCtrlView(ctl));
	if(cb) {
		DbLoginBlock blk;
		if(dbID && pDbes->GetByID(dbID, &blk)) {
			SString dict_path, data_path;
			blk.GetAttr(DbLoginBlock::attrDbPath, data_path);
			blk.GetAttr(DbLoginBlock::attrDictPath, dict_path);

			BDictionary dict(dict_path, data_path);
			StrAssocArray * p_tbl_list = new StrAssocArray;
			dict.GetListOfTables(0, p_tbl_list);
			p_tbl_list->SortByText();
			cb->setListWindow(CreateListWindow(p_tbl_list, lbtDisposeData), tblID);
		}
	}
	return ok;
}

int InputDateDialog(const char * pTitle, const char * pInputTitle, LDATE * pDate)
{
	int    ok = -1;
	TDialog * dlg = new TDialog(DLG_DATE);
	if(CheckDialogPtrErr(&dlg)) {
		dlg->setTitle(pTitle);
		if(pInputTitle)
			dlg->setLabelText(CTL_DATE_DATE, pInputTitle);
		LDATE  temp_dt = *pDate;
		dlg->SetupCalDate(CTLCAL_DATE_DATE, CTL_DATE_DATE);
		dlg->setCtrlData(CTL_DATE_DATE, &temp_dt);
		while(ok < 0 && ExecView(dlg) == cmOK) {
			dlg->getCtrlData(CTL_DATE_DATE, &temp_dt);
			if(checkdate(temp_dt, 1)) {
				ASSIGN_PTR(pDate, temp_dt);
				ok = 1;
			}
			else
				PPError(PPERR_SLIB);
		}
	}
	else
		ok = 0;
	delete dlg;
	return ok;
}
//
//
//
DateAddDialogParam::DateAddDialogParam() : BaseDate(ZERODATE), Period(PRD_MONTH), PeriodCount(1), ResultDate(ZERODATE)
{
}

int DateAddDialogParam::Recalc()
{
	if(!checkdate(BaseDate))
		BaseDate = getcurdate_();
	LDATE   td = BaseDate;
	plusperiod(&td, Period, PeriodCount, 0);
	ResultDate = td;
	return (ResultDate != BaseDate) ? 1 : -1;
}

int DateAddDialog(DateAddDialogParam * pData)
{
	class __DateAddDialog : public TDialog {
		DECL_DIALOG_DATA(DateAddDialogParam);
	public:
		__DateAddDialog() : TDialog(DLG_DATEADD)
		{
		}
		DECL_DIALOG_SETDTS()
		{
			int    ok = 1;
			RVALUEPTR(Data, pData);
			Data.Recalc();
			SetupStringCombo(this, CTLSEL_DATEADD_PRD, PPTXT_CYCLELIST, Data.Period);
			setCtrlLong(CTL_DATEADD_PRDCOUNT, Data.PeriodCount);
			Update();
			return ok;
		}
		DECL_DIALOG_GETDTS()
		{
			int    ok = 1;
			Data.Period = getCtrlLong(CTLSEL_DATEADD_PRD);
			Data.PeriodCount = getCtrlLong(CTL_DATEADD_PRDCOUNT);
			Data.Recalc();
			ASSIGN_PTR(pData, Data);
			return ok;
		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(event.isCbSelected(CTLSEL_DATEADD_PRD)) {
				Data.Period = getCtrlLong(CTLSEL_DATEADD_PRD);
				Update();
				clearEvent(event);
			}
			else if(event.isCmd(cmInputUpdated)) {
				if(event.isCtlEvent(CTL_DATEADD_PRDCOUNT)) {
					Data.PeriodCount = getCtrlLong(CTL_DATEADD_PRDCOUNT);
					Update();
					clearEvent(event);
				}
			}
		}
		void   Update()
		{
			Data.Recalc();
			setCtrlDate(CTL_DATEADD_SRC, Data.BaseDate);
			setCtrlDate(CTL_DATEADD_RESULT, Data.ResultDate);
		}
	};
	DIALOG_PROC_BODY(__DateAddDialog, pData);
}

int DateRangeDialog(const char * pTitle, const char * pInputTitle, DateRange & rPeriod)
{
	int    ok = -1;
	TDialog * dlg = new TDialog(DLG_DATERNG);
	if(CheckDialogPtrErr(&dlg)) {
		dlg->setTitle(pTitle);
		if(pInputTitle)
			dlg->setLabelText(CTL_DATERNG_PERIOD, pInputTitle);
		dlg->SetupCalPeriod(CTLCAL_DATERNG_PERIOD, CTL_DATERNG_PERIOD);
		SetPeriodInput(dlg, CTL_DATERNG_PERIOD, rPeriod);
		while(ok < 0 && ExecView(dlg) == cmOK)
			if(GetPeriodInput(dlg, CTL_DATERNG_PERIOD, &rPeriod))
				ok = 1;
			else
				PPError();
	}
	else
		ok = 0;
	delete dlg;
	return ok;
}

int InputQttyDialog(const char * pTitle, const char * pInputTitle, double * pQtty)
{
	int    ok = -1;
	TDialog * dlg = new TDialog(DLG_QTTY);
	if(CheckDialogPtrErr(&dlg)) {
		dlg->setTitle(pTitle);
		if(pInputTitle)
			dlg->setLabelText(CTL_QTTY_QTTY, pInputTitle);
		dlg->setCtrlReal(CTL_QTTY_QTTY, DEREFPTRORZ(pQtty));
		if(ExecView(dlg) == cmOK) {
			ASSIGN_PTR(pQtty, dlg->getCtrlReal(CTL_QTTY_QTTY));
			ok = 1;
		}
	}
	else
		ok = 0;
	delete dlg;
	return ok;
}

int FASTCALL CheckDialogPtr(void * ppDlg/*, int genErrMsg*/)
{
	int    ok = 1;
	TDialog ** dlg = static_cast<TDialog **>(ppDlg);
	if(ValidView(*dlg) == 0) {
		*static_cast<void **>(ppDlg) = 0;
		ok = PPSetError(PPERR_DLGLOADFAULT);
		/*
		if(genErrMsg)
			PPError();
		*/
	}
	return ok;
}

int FASTCALL CheckDialogPtrErr(void * ppDlg)
{
	int    ok = 1;
	TDialog ** dlg = static_cast<TDialog **>(ppDlg);
	if(ValidView(*dlg) == 0) {
		*static_cast<void **>(ppDlg) = 0;
		ok = PPSetError(PPERR_DLGLOADFAULT);
		PPError();
	}
	return ok;
}

int STDCALL PPErrorByDialog(TDialog * dlg, uint ctlID, int err)
{
	PPError(err, 0);
	CALLPTRMEMB(dlg, selectCtrl(ctlID));
	return 0;
}

int STDCALL PPErrorByDialog(TDialog * dlg, uint ctlID)
{
	PPError(-1, 0);
	CALLPTRMEMB(dlg, selectCtrl(ctlID));
	return 0;
}

int PasswordDialog(uint dlgID, char * pBuf, size_t pwSize, size_t minLen, int withoutEncrypt)
{
	int    ok = -1;
	int    valid_data = 0;
	char   b1[32];
	char   b2[32];
	TDialog * dlg = new TDialog(NZOR(dlgID, DLG_PASSWORD));
	if(CheckDialogPtrErr(&dlg)) {
		b1[0] = 0;
		dlg->setCtrlData(CTL_PASSWORD_FIRST,  b1);
		dlg->setCtrlData(CTL_PASSWORD_SECOND, b1);
		while(!valid_data && ExecView(dlg) == cmOK) {
			dlg->getCtrlData(CTL_PASSWORD_FIRST,  b1);
			dlg->getCtrlData(CTL_PASSWORD_SECOND, b2);
			if(stricmp866(b1, b2) != 0)
				PPError(PPERR_PASSNOTIDENT, 0);
			else if(minLen && sstrlen(b1) < minLen)
				PPError(PPERR_PASSMINLEN, itoa(minLen, b2, 10));
			else {
				valid_data = 1;
				if(withoutEncrypt)
					strnzcpy(pBuf, b1, pwSize);
				else {
					Reference::Encrypt(Reference::crymRef2, b1, pBuf, pwSize);
				}
				ok = 1;
			}
		}
		delete dlg;
		memzero(b1, sizeof(b1));
		memzero(b2, sizeof(b2));
	}
	else
		ok = 0;
	return ok;
}

int FASTCALL SetupStrListBox(TView * pList)
{
	int    ok = -1;
	if(TView::IsSubSign(pList, TV_SUBSIGN_LISTBOX)) {
		SmartListBox * p_lb = static_cast<SmartListBox *>(pList);
		if(!p_lb->HasState(SmartListBox::stTreeList)) {
			StrAssocArray * p_data = new StrAssocArray;
			StrAssocListBoxDef * p_def = new StrAssocListBoxDef(p_data, lbtDisposeData|lbtDblClkNotify|lbtFocNotify);
			if(p_def == 0)
				ok = PPSetErrorNoMem();
			else {
				p_lb->setDef(p_def);
				ok = 1;
			}
		}
	}
	return ok;
}

int FASTCALL SetupStrListBox(TDialog * dlg, uint ctl)
{
	return SetupStrListBox(dlg->getCtrlView(ctl));
}

int SetupTreeListBox(TDialog * dlg, uint ctl, StrAssocArray * pData, uint fl, uint lbfl)
{
	int    ok = -1;
	SmartListBox * p_box = static_cast<SmartListBox *>(dlg->getCtrlView(ctl));
	if(p_box) {
		p_box->ViewOptions |= lbfl;
		StdTreeListBoxDef * p_def = new StdTreeListBoxDef(pData, NZOR(fl, (lbtDisposeData|lbtDblClkNotify)), 0);
		if(p_def == 0)
			ok = PPSetErrorNoMem();
		else {
			p_box->setDef(p_def);
			ok = 1;
		}
	}
	return ok;
}
//
// Lst2LstDialogUI
//
ListToListUIData::ListToListUIData()
{
	THISZERO();
}

Lst2LstDialogUI::Lst2LstDialogUI(uint rezID, ListToListUIData * pData) : TDialog(rezID), Data(*pData)
{
	setup();
}

int    Lst2LstDialogUI::addItem() { return -1; }
int    Lst2LstDialogUI::addNewItem() { return -1; }
int    Lst2LstDialogUI::removeItem() { return -1; }
int    Lst2LstDialogUI::addAll() { return -1; }
int    Lst2LstDialogUI::removeAll() { return -1; }
SmartListBox * Lst2LstDialogUI::GetLeftList() { return static_cast<SmartListBox *>(getCtrlView(Data.LeftCtlId)); }
SmartListBox * Lst2LstDialogUI::GetRightList() { return static_cast<SmartListBox *>(getCtrlView(Data.RightCtlId)); }

uint Lst2LstDialogUI::GetLeftSelectionList(LongArray * pList)
{
	SmartListBox * p_lv = GetLeftList();
	return p_lv ? p_lv->GetSelectionList(pList) : 0;
}

uint Lst2LstDialogUI::GetRightSelectionList(LongArray * pList)
{
	SmartListBox * p_lv = GetRightList();
	return p_lv ? p_lv->GetSelectionList(pList) : 0;
}

int Lst2LstDialogUI::setup()
{
	if(Data.TitleStrID)
		setTitle(PPLoadTextS(Data.TitleStrID, SLS.AcquireRvlStr()));
	else
		setTitle(Data.P_Title);
	SETIFZ(Data.LeftCtlId, CTL_LST2LST_LST1);
	SETIFZ(Data.RightCtlId, CTL_LST2LST_LST2);
	enableCommand(cmForward, true);
	enableCommand(cmAllForward, true);
	enableCommand(cmaInsert, LOGIC(Data.Flags & ListToListUIData::fCanInsertNewItem));
	SetDefaultButton(CTL_LST2LST_FW, true);
	SetDefaultButton(CTL_LST2LST_BW, false);
	return 1;
}

IMPL_HANDLE_EVENT(Lst2LstDialogUI)
{
	TDialog::handleEvent(event);
	if(TVCOMMAND)
		switch(TVCMD) {
			case cmaInsert:     addNewItem(); break;
			case cmForward:     
				addItem();    
				break;
			case cmAllForward:  addAll();     break;
			case cmBackward:    removeItem(); break;
			case cmAllBackward: removeAll();  break;
			case cmLBDblClk:
				if(P_Current) {
  					int    action = 1;
					SmartListBox * list = static_cast<SmartListBox *>(getCtrlView(GetCurrId()));
					if(list && list->IsTreeList()) {
						PPID cur_id = 0;
						list->P_Def->getCurID(&cur_id);
						if(static_cast<const StdTreeListBoxDef *>(list->P_Def)->HasChildren(cur_id))
							action = 0;
					}
					if(action && isCurrCtlID(Data.LeftCtlId))
						addItem();
					else if(action && isCurrCtlID(Data.RightCtlId))
						removeItem();
					else
						return;
				}
				else
					return;
				break;
			default:
				return;
		}
	else if(TVBROADCAST)
		if(TVCMD == cmReceivedFocus && TVINFOVIEW) {
			defButton d = b_ok;
			if(event.isCtlEvent(Data.LeftCtlId))
				d = b_fw;
			else if(event.isCtlEvent(Data.RightCtlId))
				d = b_bw;
			SetDefaultButton(CTL_LST2LST_FW, d == b_fw);
			SetDefaultButton(CTL_LST2LST_BW, d == b_bw);
			return;
		}
		else
			return;
	else if(TVKEYDOWN) {
		uint   i = GetCurrId();
		if(i) {
			switch(TVKEY) {
				case kbEnter:
			   		if(i == Data.LeftCtlId)
						addItem();
					else if(i == Data.RightCtlId)
						removeItem();
					else
						return;
					break;
				case kbIns:
		   			if(oneof2(i, Data.LeftCtlId, Data.RightCtlId))
						addItem();
 					else
						return;
					break;
				case kbDel:
		   			if(oneof2(i, Data.LeftCtlId, Data.RightCtlId))
						removeItem();
 					else
						return;
					break;
				default:
					return;
			}
		}
		else
			return;
	}
	else
		return;
	clearEvent(event);
}
//
// Lst2LstAryDialog
//
ListToListAryData::ListToListAryData(uint rezID, SArray * pLList, SArray * pRList) : ListToListUIData(), RezID(rezID), P_LList(pLList), P_RList(pRList)
{
}

Lst2LstAryDialog::Lst2LstAryDialog(uint rezID, ListToListUIData * pData, SArray * pLeft, SArray * pRight) : Lst2LstDialogUI(rezID, pData)
{
	P_Right = new SArray(pRight->getItemSize()/*sizeof(TaggedString)*/);
	P_Right->copy(*pRight);
	P_Left = new SArray(pLeft->getItemSize()/*sizeof(TaggedString)*/);
	P_Left->copy(*pLeft);
	setupLeftList();
	setupRightList();
}

SArray * Lst2LstAryDialog::GetRight() const { return P_Right; }
SArray * Lst2LstAryDialog::GetLeft() const { return P_Left; }
int    Lst2LstAryDialog::getDTS(SArray * pList) { return pList->copy(*P_Right); }
int    Lst2LstAryDialog::setupRightList() { return SetupList(P_Right, GetRightList()); }
int    Lst2LstAryDialog::setupLeftList() { return SetupList(P_Left, GetLeftList()); }

int Lst2LstAryDialog::SetupList(SArray * pA, SmartListBox * pL)
{
	if(SmartListBox::IsValidS(pL)) {
		const long pos = pL->P_Def->_curItem();
		StdListBoxDef * def = new StdListBoxDef(pA, lbtFocNotify|lbtDblClkNotify, MKSTYPE(S_ZSTRING, 64));
		pL->setDef(def);
		pL->P_Def->go(pos);
		pL->Draw_();
	}
	return 1;
}

int Lst2LstAryDialog::addItem()
{
	int    ok = 1;
	uint   idx = 0;
	PPID   tmp;
	SmartListBox * p_view = GetLeftList();
	if(p_view && p_view->getCurID(&tmp) && tmp && !P_Right->lsearch(&tmp, &idx, CMPF_LONG, 0)) {
		P_Left->lsearch(&tmp, &idx, CMPF_LONG, 0);
		const void * p_current = P_Left->at(idx);
		THROW(P_Right->insert(p_current));
		THROW(setupRightList());
	}
	CATCHZOKPPERR
	return ok;
}

int Lst2LstAryDialog::removeItem()
{
	int    ok = 1;
	uint   idx = 0;
	PPID   tmp;
	SmartListBox * p_lbx = GetRightList();
	if(p_lbx && p_lbx->getCurID(&tmp)) {
		P_Right->lsearch(&tmp, &idx, CMPF_LONG, 0);
		if(P_Right->atFree(idx) > 0)
			THROW(setupRightList());
	}
	CATCHZOKPPERR
	return ok;
}

int Lst2LstAryDialog::addAll()
{
	if(P_Right && P_Left)
		P_Right->copy(*P_Left);
	return setupRightList() ? 1 : PPErrorZ();
}

int Lst2LstAryDialog::removeAll()
{
	P_Right->freeAll();
	return setupRightList() ? 1 : PPErrorZ();
}
//
// Lst2LstObjDialog
//
ListToListData::ListToListData(PPID objType, void * extraPtr, PPIDArray * pList) : ListToListUIData(), ObjType(objType), ExtraPtr(extraPtr), P_SrcList(0), P_List(pList)
{
}

ListToListData::ListToListData(const StrAssocArray * pSrcList, PPID objType, PPIDArray * pList) :
	ObjType(objType), ExtraPtr(0), P_SrcList(pSrcList), P_List(pList)
{
}

Lst2LstObjDialog::Lst2LstObjDialog(uint rezID, ListToListData * aData) : Lst2LstDialogUI(rezID, aData), Data(*aData), P_Object(0)
{
	Data.P_List = new PPIDArray(*aData->P_List);
	setup();
}

Lst2LstObjDialog::~Lst2LstObjDialog()
{
	delete P_Object;
	delete Data.P_List;
}


IMPL_HANDLE_EVENT(Lst2LstObjDialog)
{
	Lst2LstDialogUI::handleEvent(event);
	if(event.isCmd(cmSelectByTag)) {
		SelectByTag();
		clearEvent(event);
	}
}

int Lst2LstObjDialog::getDTS(PPIDArray * P_List)
{
	return P_List->copy(*Data.P_List);
}

void Lst2LstObjDialog::GetItemText(long id, SString & rBuf)
{
	if(Data.P_SrcList) {
		Data.P_SrcList->GetText(id, rBuf);
	}
	else if(P_Object) {
		rBuf.Z();
		P_Object->GetName(id, &rBuf);
		if(rBuf.IsEmpty())
			ideqvalstr(id, rBuf);
	}
	else
		ideqvalstr(id, rBuf.Z());
}

int Lst2LstObjDialog::setupRightTList()
{
	int    ok = 1;
	PPID * p_id = 0;
	uint   i;
	SString name_buf;
	StdTreeListBoxDef * p_def = 0;
	StdTreeListBoxDef * p_l_def = static_cast<StdTreeListBoxDef *>(GetLeftList()->P_Def);
	SmartListBox * p_r_lbx = GetRightList();
	StrAssocArray * p_list = new StrAssocArray;
	THROW_MEM(p_list);
	for(i = 0; Data.P_List->enumItems(&i, (void **)&p_id);) {
		const  long id = *p_id;
		long   parent_id = 0;
		p_l_def->GetParent(id, &parent_id);
		parent_id = (parent_id && Data.P_List->lsearch(parent_id)) ? parent_id : 0;
		GetItemText(id, name_buf);
		THROW_SL(p_list->Add(id, parent_id, name_buf));
	}
	p_list->SortByText();
	p_def = new StdTreeListBoxDef(p_list, lbtDisposeData|lbtDblClkNotify, 0);
	THROW_MEM(p_def);
	p_r_lbx->setDef(p_def);
	p_r_lbx->P_Def->go(0);
	p_r_lbx->Draw_();
	CATCH
		if(p_def)
			delete p_def;
		else
			delete p_list;
		ok = 0;
	ENDCATCH
	return ok;
}

int Lst2LstObjDialog::setupRightList()
{
	int    ok = 1;
	StrAssocListBoxDef * p_def = 0;
	StrAssocArray * p_ary = 0;
	setCtrlLong(CTL_LST2LST_CT2, Data.P_List ? Data.P_List->getCountI() : 0);
	if(Data.Flags & ListToListData::fIsTreeList) {
		return setupRightTList();
	}
	else {
		SmartListBox * p_lb = GetRightList();
		if(p_lb) {
			SString name_buf;
			const long pos = p_lb->P_Def ? p_lb->P_Def->_curItem() : 0L;
			THROW_MEM(p_ary = new StrAssocArray);
			for(uint i = 0; i < Data.P_List->getCount(); i++) {
				const  PPID id = Data.P_List->get(i);
				GetItemText(id, name_buf);
				THROW_SL(p_ary->Add(id, name_buf));
			}
			p_ary->SortByText();
			THROW_MEM(p_def = new StrAssocListBoxDef(p_ary, lbtDisposeData|lbtDblClkNotify));
			p_lb->setDef(p_def);
			p_lb->P_Def->go(pos);
			p_lb->Draw_();
		}
	}
	CATCH
		if(p_def)
			delete p_def;
		else
			delete p_ary;
		ok = 0;
	ENDCATCH
	return ok;
}

int Lst2LstObjDialog::setupLeftList()
{
	int    ok = 1;
	ListBoxDef * p_def = 0;
	if(Data.P_SrcList) {
		StrAssocArray * p_data = new StrAssocArray(*Data.P_SrcList);
		if(Data.Flags & ListToListData::fIsTreeList)
			p_def = new StdTreeListBoxDef(p_data, lbtDblClkNotify|lbtFocNotify|lbtDisposeData, 0);
		else
			p_def = new StrAssocListBoxDef(p_data, lbtDblClkNotify|lbtFocNotify|lbtDisposeData);
	}
	else if(P_Object) {
		p_def = P_Object->Selector(0, 0, Data.ExtraPtr);
	}
	if(p_def) {
		SmartListBox * p_list = GetLeftList();
		if(p_list) {
			p_list->setDef(p_def);
			p_list->Draw_();
		}
	}
	else
		ok = 0;
	return ok;
}

bool Lst2LstObjDialog::IsSelectionByTagEnabled(PPID * pRealObjType)
{
	bool   yes = false;
	PPID   real_obj_type = 0;
	if(oneof4(Data.ObjType, PPOBJ_PERSON, PPOBJ_GOODS, PPOBJ_GLOBALUSERACC, PPOBJ_WORKBOOK)) {
		real_obj_type = Data.ObjType;
		yes = true;
	}
	else if(Data.ObjType == PPOBJ_ARTICLE) {
		if(Data.ExtraPtr) {
			const ArticleFilt * p_ar_filt = static_cast<const ArticleFilt *>(Data.ExtraPtr);
			if(p_ar_filt->AccSheetID) {
				PPObjAccSheet acs_obj;
				PPAccSheet acs_rec;
				if(acs_obj.Fetch(p_ar_filt->AccSheetID, &acs_rec) > 0 && acs_rec.Assoc == PPOBJ_PERSON) {
					real_obj_type = PPOBJ_PERSON;
					yes = true;	
				}
			}
		}
	}
	ASSIGN_PTR(pRealObjType, real_obj_type);
	return yes;
}

int Lst2LstObjDialog::setup()
{
	int    ok = 1;
	if(Data.ObjType) {
		THROW(P_Object = GetPPObject(Data.ObjType, Data.ExtraPtr));
	}
	else {
		ZDELETE(P_Object);
	}
	showButton(cmSelectByTag, IsSelectionByTagEnabled(0));  // @v11.0.3
	THROW(setupLeftList());
	THROW(setupRightList());
	CATCHZOKPPERR
	return ok;
}

int Lst2LstObjDialog::SelectByTag()
{
/*
	if(pDef) {
		if(Data.P_List->addUnique(id) > 0) {
*/
	int    ok = -1;
	PPObjArticle * p_ar_obj = 0;
	if(Data.P_List) {
		PPID   real_obj_type = 0;
		if(IsSelectionByTagEnabled(&real_obj_type)) {
			assert(real_obj_type);
			TagFilt tag_filt;
			if(EditTagFilt(real_obj_type, &tag_filt) > 0) {
				PPID acs_id = 0;
				PPAccSheet acs_rec;
				if(Data.ObjType == PPOBJ_ARTICLE) {
					if(Data.ExtraPtr) {
						const ArticleFilt * p_ar_filt = static_cast<const ArticleFilt *>(Data.ExtraPtr);
						if(p_ar_filt->AccSheetID) {
							PPObjAccSheet acs_obj;
							if(acs_obj.Fetch(p_ar_filt->AccSheetID, &acs_rec) > 0 && acs_rec.Assoc == PPOBJ_PERSON) {
								acs_id = acs_rec.ID;							
								p_ar_obj = new PPObjArticle;
							}
						}
					}
				}
				PPObjTag tag_obj;
				UintHashTable selection_list;
				UintHashTable exclude_list;
				tag_obj.GetObjListByFilt(real_obj_type, &tag_filt, selection_list, exclude_list);
				for(ulong _iter_id = 0; selection_list.Enum(&_iter_id);) {
					PPID   obj_id_to_add = 0;
					if(Data.ObjType == PPOBJ_ARTICLE && real_obj_type == PPOBJ_PERSON) {
						assert(p_ar_obj);
						PPID   ar_id = 0;
						if(p_ar_obj && p_ar_obj->P_Tbl->PersonToArticle(_iter_id, acs_id, &ar_id)) {
							obj_id_to_add = ar_id;
						}
					}
					else {
						obj_id_to_add = static_cast<PPID>(_iter_id);
					}
					if(obj_id_to_add) {
						Data.P_List->addUnique(obj_id_to_add);
						ok = 1;
					}
				}
				if(ok > 0) {
					setupRightList();
				}
			}
		}
	}
	delete p_ar_obj;
	return ok;
}

int Lst2LstObjDialog::addNewItem()
{
	int    ok = -1;
	if(Data.Flags & ListToListData::fCanInsertNewItem && P_Object ) {
		SmartListBox * p_view = GetLeftList();
		PPID   obj_id = 0;
		if(p_view && P_Object->Edit(&obj_id, Data.ExtraPtr) > 0) {
			// @v11.1.10 P_Object->UpdateSelector(p_view->def, 0, Data.ExtraPtr);
			P_Object->Selector(p_view->P_Def, 0, Data.ExtraPtr); // @v11.1.10
			p_view->Search_(&obj_id, 0, srchFirst|lbSrchByID);
			p_view->Draw_();
			ok = 1;
		}
	}
	return ok;
}

int FASTCALL Lst2LstObjDialog::Helper_AddItemRecursive(PPID id, StdTreeListBoxDef * pDef)
{
	int    ok = 1;
	if(pDef) {
		if(Data.P_List->addUnique(id) > 0) {
			LongArray child_list;
			THROW_SL(pDef->GetListByParent(id, child_list));
			for(uint i = 0; i < child_list.getCount(); i++) {
				THROW(Helper_AddItemRecursive(child_list.get(i), pDef)); // @recursion
			}
		}
	}
	CATCHZOK
	return ok;
}

int Lst2LstObjDialog::addItem()
{
	int    ok = 1;
	SmartListBox * p_view = GetLeftList();
	LongArray sel_list;
	uint sc = GetLeftSelectionList(&sel_list);
	//
	//if(p_view && p_view->getCurID(&id) && id && !Data.P_List->lsearch(id)) {
	if(p_view && sel_list.getCount()) {
		for(uint i = 0; i < sel_list.getCount(); i++) {
			PPID id = sel_list.get(i);
			if(id > 0) {
				if(Data.Flags & ListToListData::fIsTreeList) {
					THROW(Helper_AddItemRecursive(id, (StdTreeListBoxDef *)p_view->P_Def));
				}
				else {
					THROW(Data.P_List->add(id));
				}
			}
		}
		THROW(setupRightList());
	}
	else
		ok = -1;
	CATCHZOKPPERR
	return ok;
}

int FASTCALL Lst2LstObjDialog::Helper_RemoveItemRecursive(PPID id, StdTreeListBoxDef * pDef)
{
	int    ok = -1;
	if(pDef) {
		LongArray child_list;
		THROW_SL(pDef->GetListByParent(id, child_list));
		for(uint i = 0; i < child_list.getCount(); i++) {
			int r;
			THROW(r = Helper_RemoveItemRecursive(child_list.get(i), pDef)); // @recursion
			if(r > 0)
				ok = 1;
		}
		if(Data.P_List->freeByKey(id, 0) > 0)
			ok = 1;
	}
	CATCHZOK
	return ok;
}

int Lst2LstObjDialog::removeItem()
{
	int    ok = 1;
	SmartListBox * p_view = GetRightList();
	LongArray sel_list;
	uint sc = GetRightSelectionList(&sel_list);
	if(p_view && sel_list.getCount()) {
		for(uint i = 0; i < sel_list.getCount(); i++) {
			const PPID id = sel_list.get(i);
			if(id > 0)
				Data.P_List->freeByKey(id, 0);
		}
		THROW(setupRightList());
	}
	CATCHZOKPPERR
	return ok;
}

int Lst2LstObjDialog::addAll()
{
	int    ok = -1;
	SmartListBox * p_l = GetLeftList();
	if(SmartListBox::IsValidS(p_l)) {
		LongArray id_list;
		if(p_l->P_Def->getIdList(id_list) > 0) {
			Data.P_List->freeAll();
			Data.P_List->addUnique(&id_list);
			THROW(setupRightList());
			ok = 1;
		}
	}
	CATCHZOKPPERR
	return ok;
}

int Lst2LstObjDialog::removeAll()
{
	Data.P_List->freeAll();
	return setupRightList() ? 1 : PPErrorZ();
}

int FASTCALL ListToListDialog(ListToListData * pData)
{
	int    r;
	Lst2LstObjDialog * dlg = new Lst2LstObjDialog((pData && (pData->Flags & ListToListData::fIsTreeList)) ? DLG_TLST2TLST : DLG_LST2LST, pData);
	if(CheckDialogPtr(&dlg)) {
		if(pData && pData->ObjType) {
			SString obj_type_symb;
			DS.GetObjectTypeSymb(pData->ObjType, obj_type_symb);
			dlg->SetStorableUserParamsSymbSuffix(obj_type_symb);
		}
		if((r = ExecView(dlg)) == cmOK)
			if(!dlg->getDTS(pData->P_List))
				r = 0;
		delete dlg;
		if(r)
			return (r == cmOK) ? 1 : -1;
	}
	return 0;
}
//
// WLDialog
//
WLDialog::WLDialog(uint rezID, uint _wlCtlID) : TDialog(rezID), wlCtlID(_wlCtlID), wl(0)
{
}

void   WLDialog::setWL(int s) { toggleWL(s); }
int    WLDialog::getWL() const { return BIN(wl); }

void WLDialog::toggleWL(int s)
{
	if(BillObj->CheckRights(BILLRT_USEWLABEL)) {
		wl = (s < 0) ? (wl ? 0 : 1) : (s ? 1 : 0);
		uint   c = wl ? 0xFBU : 0x20U;
		setStaticText(wlCtlID, (char *)&c);
	}
	else
		wl = 0;
}

IMPL_HANDLE_EVENT(WLDialog)
{
	TDialog::handleEvent(event);
	if(event.isKeyDown(kbF8)) {
		toggleWL(-1);
		clearEvent(event);
	}
}

int GetDeviceTypeName(uint dvcClass, PPID deviceTypeID, SString & rBuf)
{
	rBuf.Z();
	int    ok = -1;
	int    str_id = 0;
	int    ini_sect_id = PPAbstractDevice::GetDrvIniSectByDvcClass(dvcClass, &str_id, 0);
	if(ini_sect_id) {
		int    idx = 0;
		//uint   old_dev_count = 0;
		SString line_buf, item_buf, id_buf, txt_buf;
		SString path;
		PPGetFilePath(PPPATH_BIN, "ppdrv.ini", path);
		PPIniFile ini_file(path);
		if(str_id && PPLoadText(str_id, line_buf)) {
			for(idx = 0; PPGetSubStr(line_buf, idx, item_buf) > 0; idx++) {
				long   id = 0;
				if(item_buf.Divide(',', id_buf, txt_buf) > 0)
					id = id_buf.ToLong();
				else {
					id = (idx+1);
					txt_buf = item_buf;
				}
				if(id == deviceTypeID) {
					rBuf = txt_buf;
					ok = 1;
				}
				//old_dev_count++;
			}
		}
		if(ok < 0 && GetStrFromDrvIni(ini_file, ini_sect_id, deviceTypeID, /*old_dev_count*/PPCMT_FIRST_DYN_DVC, line_buf)) {
			SString symbol, drv_name;
			int    drv_impl = 0;
			if(PPAbstractDevice::ParseRegEntry(line_buf, symbol, drv_name, path, &drv_impl)) {
				rBuf = drv_name;
				ok = 1;
			}
		}
		if(ok <= 0)
			rBuf.Z().CatEq("Unkn device type", deviceTypeID);
	}
	return ok;
}

//
//
// @vmiller {
int SetupStringComboDevice(TDialog * dlg, uint ctlID, uint dvcClass, long initID, uint /*flags*/)
{
	int    ok = 1;
	int    str_id = 0;
	StrAssocArray * p_list = 0;
	int    ini_sect_id = PPAbstractDevice::GetDrvIniSectByDvcClass(dvcClass, &str_id, 0);
	if(ini_sect_id) {
		ComboBox * p_cb = static_cast<ComboBox *>(dlg->getCtrlView(ctlID));
		if(p_cb) {
			int    idx = 0;
			SString line_buf, item_buf, id_buf, txt_buf;
			SString symbol, drv_name;
			SString path;
			PPGetFilePath(PPPATH_BIN, "ppdrv.ini", path);
			PPIniFile ini_file(path);
			THROW_MEM(p_list = new StrAssocArray());
			if(str_id && PPLoadText(str_id, line_buf)) {
				for(idx = 0; PPGetSubStr(line_buf, idx, item_buf) > 0; idx++) {
					long   id = 0;
					if(item_buf.Divide(',', id_buf, txt_buf) > 0)
						id = id_buf.ToLong();
					else {
						id = (idx+1);
						txt_buf = item_buf;
					}
					if(!txt_buf.IsEqiAscii("Unused")) {
						THROW_SL(p_list->Add(id, txt_buf));
					}
				}
				//list_count = p_list->getCount();
			}
			for(int i = /*(idx + 1)*/PPCMT_FIRST_DYN_DVC; GetStrFromDrvIni(ini_file, ini_sect_id, i, /*list_count*/PPCMT_FIRST_DYN_DVC, line_buf) > 0; i++) {
				int    drv_impl = 0;
				if(PPAbstractDevice::ParseRegEntry(line_buf, symbol, drv_name, path, &drv_impl)) {
					THROW_SL(p_list->Add((int)i, drv_name));
				}
			}
			p_cb->setListWindow(CreateListWindow(p_list, lbtDisposeData | lbtDblClkNotify), initID);
		}
	}
	else
		ok = -1;
	CATCH
		ZDELETE(p_list);
		ok = 0;
	ENDCATCH
	return ok;
}
// } @vmiller

static int Helper_SetupStringCombo(TDialog * dlg, uint ctlID, const SString & rLineBuf, const StrAssocArray * pAddendumList, long initID)
{
	int    ok = 1;
	StrAssocArray * p_list = 0;
	if(rLineBuf.NotEmpty()) {
		ComboBox * p_cb = 0;
		if((p_cb = static_cast<ComboBox *>(dlg->getCtrlView(ctlID))) != 0) {
			int    idx = 0;
			SString item_buf, id_buf, txt_buf;
			THROW_MEM(p_list = new StrAssocArray());
			for(idx = 0; PPGetSubStr(rLineBuf, idx, item_buf) > 0; idx++) {
				long   id = 0;
				if(item_buf.Divide(',', id_buf, txt_buf) > 0)
					id = id_buf.ToLong();
				else {
					id = (idx+1);
					txt_buf = item_buf;
				}
				THROW_SL(p_list->Add(id, txt_buf));
			}
			if(pAddendumList && pAddendumList->getCount()) {
				for(uint i = 0; i < pAddendumList->getCount(); i++) {
					StrAssocArray::Item ai = pAddendumList->Get(i);
					THROW_SL(p_list->Add(ai.Id, ai.Txt, 0));
				}
			}
			p_cb->setListWindow(CreateListWindow(p_list, lbtDisposeData|lbtDblClkNotify), initID);
		}
	}
	CATCH
		ZDELETE(p_list);
		ok = 0;
	ENDCATCH
	return ok;
}

int STDCALL SetupStringCombo(TDialog * dlg, uint ctlID, int strID, long initID)
{
	SString line_buf;
	return PPLoadText(strID, line_buf) ? Helper_SetupStringCombo(dlg, ctlID, line_buf, 0, initID) : 0;
}

int STDCALL SetupStringCombo(TDialog * dlg, uint ctlID, const char * pStrSignature, long initID)
{
	SString line_buf;
	return PPLoadString(pStrSignature, line_buf) ? Helper_SetupStringCombo(dlg, ctlID, line_buf, 0, initID) : 0;
}

int STDCALL SetupStringComboWithAddendum(TDialog * dlg, uint ctlID, const char * pStrSignature, const StrAssocArray * pAddendumList, long initID)
{
	SString line_buf;
	return PPLoadString(pStrSignature, line_buf) ? Helper_SetupStringCombo(dlg, ctlID, line_buf, pAddendumList, initID) : 0;
}

int STDCALL SetupStrAssocTreeCombo(TWindow * dlg, uint ctlID, const StrAssocArray & rList, long initID, uint flags, int ownerDrawListBox/*= 0*/)
{
	int    ok = 1;
	ListWindow * p_lw = 0;
	ComboBox   * p_cb = static_cast<ComboBox *>(dlg->getCtrlView(ctlID));
	if(p_cb) {
		const uint options = ownerDrawListBox ? (lbtOwnerDraw|lbtDisposeData|lbtDblClkNotify) : (lbtDisposeData|lbtDblClkNotify);
		StrAssocArray * p_list = new StrAssocArray(rList);
		THROW_MEM(p_list);
		THROW_MEM(p_lw = new ListWindow(new StdTreeListBoxDef(p_list, options, MKSTYPE(S_ZSTRING, 128))));
		p_cb->setListWindow(p_lw, initID);
	}
	CATCHZOK
	return ok;
}

int STDCALL SetupStrAssocCombo(TWindow * dlg, uint ctlID, const StrAssocArray & rList, long initID, uint flags, size_t offs, int ownerDrawListBox)
{
	int    ok = 1;
	ListWindow * p_lw = 0;
	ComboBox   * p_cb = static_cast<ComboBox *>(dlg->getCtrlView(ctlID));
	if(p_cb) {
		const uint options = ownerDrawListBox ? (lbtOwnerDraw|lbtDisposeData|lbtDblClkNotify) : (lbtDisposeData|lbtDblClkNotify);
		StrAssocArray * p_list = new StrAssocArray;
		THROW_MEM(p_list);
		if(offs) {
			for(uint i = 0; i < rList.getCount(); i++) {
				StrAssocArray::Item item = rList.at_WithoutParent(i);
				const size_t len = sstrlen(item.Txt);
				if(offs < len)
					p_list->Add(item.Id, item.Txt+offs);
				else
					p_list->Add(item.Id, 0);
			}
		}
		else
			*p_list = rList;
		THROW_MEM(p_lw = new ListWindow(new StrAssocListBoxDef(p_list, options)));
		p_cb->setListWindow(p_lw, initID);
	}
	CATCHZOK
	return ok;
}

int STDCALL SetupSCollectionComboBox(TDialog * dlg, uint ctl, SCollection * pSC, long initID)
{
	int    ok = 1;
	ComboBox * cb = static_cast<ComboBox *>(dlg->getCtrlView(ctl));
	if(cb && pSC && pSC->getCount()) {
		ListWindow * lw = CreateListWindow_Simple(0);
		for(uint i = 0; i < pSC->getCount(); i++) {
			char * p_buf = (char *)pSC->at(i);
			if(p_buf && p_buf[0] != '\0') {
				char   temp_buf[256]; // @v11.2.5 [128]-->[256]
				long   id = 0;
				uint   pos = 0;
				StringSet ss(',', p_buf);
				if(ss.IsCountGreaterThan(1)) {
					ss.get(&pos, temp_buf, sizeof(temp_buf));
					id = atol(temp_buf);
				}
				else
					id = i + 1;
				ss.get(&pos, temp_buf, sizeof(temp_buf));
				lw->listBox()->addItem(id, temp_buf);
			}
		}
		cb->setListWindow(lw, initID);
	}
	return ok;
}

int SetupSubstDateCombo(TDialog * dlg, uint ctlID, long initID)
{
	return SetupStringCombo(dlg, ctlID, PPTXT_SUBSTDATELIST, initID);
}

int STDCALL SetupSubstPersonCombo(TDialog * pDlg, uint ctlID, SubstGrpPerson sgp)
{
	PPID   id = 0;
	PPID   init_id = 0;
	SString buf;
	SString word_rel;
	SString id_buf;
	SString txt_buf;
	PPPersonRelType item;
	StrAssocArray ary;
	PPObjPersonRelType relt_obj;
	PPLoadText(PPTXT_SUBSTPERSONLIST, buf);
	StringSet ss(';', buf);
	for(uint i = 0; ss.get(&i, buf);)
		if(buf.Divide(',', id_buf, txt_buf) > 0)
			ary.Add(id_buf.ToLong(), txt_buf);
	PPLoadString("relation", word_rel);
	for(id = 0; relt_obj.EnumItems(&id, &item) > 0;)
		if(item.Cardinality & (PPPersonRelType::cOneToOne | PPPersonRelType::cManyToOne)) {
			(buf = word_rel).Colon().Cat(item.Name);
			ary.Add(id + (long)sgpFirstRelation, buf);
		}
	init_id = (long)sgp;
	return SetupStrAssocCombo(pDlg, ctlID, ary, init_id, 0);
}

int SetupSubstGoodsCombo(TDialog * dlg, uint ctlID, long initID)
{
	int    ok = 1;
	ComboBox   * p_cb = 0;
	ListWindow * p_lw = 0;
	if((p_cb = static_cast<ComboBox *>(dlg->getCtrlView(ctlID))) != 0) {
		int    idx = 0;
		PrcssrAlcReport::Config alr_cfg;
		SString item_buf, id_buf, txt_buf;
		THROW(p_lw = CreateListWindow_Simple(lbtDblClkNotify));
		for(idx = 0; PPGetSubStr(PPTXT_SUBSTGOODSLIST, idx, item_buf) > 0; idx++) {
			long   id = 0;
			if(item_buf.Divide(',', id_buf, txt_buf) > 0)
				id = id_buf.ToLong();
			else {
				id = (idx+1);
				txt_buf = item_buf;
			}
			if(id != sggAlcoCategory || (PrcssrAlcReport::ReadConfig(&alr_cfg) > 0 && (alr_cfg.CategoryTagID || alr_cfg.CategoryClsDim))) {
				p_lw->listBox()->addItem(id, txt_buf);
			}
		}
		{
			long   count = 0;
			GoodsGroupTotal ggrp_total;
			PPObjGoodsGroup ggobj;
			ggobj.CalcTotal(&ggrp_total);
			count = ggrp_total.MaxLevel + sggGroupSecondLvl - 1;
			PPLoadText(PPTXT_GROUPLEVELX, item_buf);
			for(long j = 2, id = sggGroupSecondLvl; id < count; id++, j++) {
				txt_buf.Printf(item_buf.cptr(), j);
				p_lw->listBox()->addItem(id, txt_buf);
			}
		}
		{
			PPObjectTag tag_rec;
			for(SEnum en = PPRef->Enum(PPOBJ_TAG, 0); en.Next(&tag_rec) > 0;) {
				if(tag_rec.ObjTypeID == PPOBJ_GOODS && tag_rec.TagDataType != OTTYP_GROUP) {
					txt_buf.Z().Cat("Tag").CatDiv(':', 2).Cat(tag_rec.Name);
					p_lw->listBox()->addItem(tag_rec.ID + sggTagBias, txt_buf);
				}
			}
		}
		p_cb->setListWindow(p_lw, initID);
	}
	CATCHZOK
	return ok;
}

int SetupSubstBillCombo(TDialog * pDlg, uint ctlID, SubstGrpBill sgb)
{
	SString buf, id_buf, txt_buf;
	StrAssocArray ary;
	PPLoadText(PPTXT_SUBSTBILLLIST, buf);
	StringSet ss(';', buf);
	for(uint i = 0; ss.get(&i, buf);)
		if(buf.Divide(',', id_buf, txt_buf) > 0)
			ary.Add(id_buf.ToLong(), txt_buf);
	return SetupStrAssocCombo(pDlg, ctlID, ary, (long)sgb.S, 0);
}

int SetupSubstSCardCombo(TDialog * pDlg, uint ctlID, SubstGrpSCard sgc)
{
	SString buf, id_buf, txt_buf;
	StrAssocArray ary;
	PPLoadText(PPTXT_SUBSTSCARDLIST, buf);
	StringSet ss(';', buf);
	for(uint i = 0; ss.get(&i, buf);)
		if(buf.Divide(',', id_buf, txt_buf) > 0)
			ary.Add(id_buf.ToLong(), txt_buf);
	return SetupStrAssocCombo(pDlg, ctlID, ary, static_cast<long>(sgc), 0);
}
//
//
//
CycleCtrlGroup::CycleCtrlGroup(uint ctlSelCycle, uint ctlNumCycles, uint ctlPeriod) : CtrlGroup(),
	InpUpdLock(0), P_PrdDialog(0), CtlSelCycle(ctlSelCycle), CtlPdc(0), CtlNumCycles(ctlNumCycles), CtlPeriod(ctlPeriod)
{
}

CycleCtrlGroup::CycleCtrlGroup(uint ctlSelCycle, uint ctlPdc, uint ctlNumCycles, uint ctlPeriod) : CtrlGroup(),
	InpUpdLock(0), P_PrdDialog(0), CtlSelCycle(ctlSelCycle), CtlPdc(ctlPdc), CtlNumCycles(ctlNumCycles), CtlPeriod(ctlPeriod)
{
}

CycleCtrlGroup::CycleCtrlGroup(uint ctlSelCycle, uint ctlNumCycles, TDialog * pPeriodDlg, uint ctlPeriod) : CtrlGroup(),
	InpUpdLock(0), P_PrdDialog(pPeriodDlg), CtlSelCycle(ctlSelCycle), CtlPdc(0), CtlNumCycles(ctlNumCycles), CtlPeriod(ctlPeriod)
{
}

int CycleCtrlGroup::setupCycleCombo(TDialog * pDlg, int cycleID)
{
	int    ok = 1;
	if(pDlg && pDlg->getCtrlView(CtlSelCycle)) {
		const long cycle_id = ((cycleID & PRD_PRECDAYSMASK) == PRD_PRECDAYSMASK) ? PRD_PRECDAYSMASK : cycleID;
		ok = SetupStringCombo(pDlg, CtlSelCycle, PPTXT_CYCLELIST, 0);
		if(CtlPdc && pDlg && pDlg->getCtrlView(CtlPdc)) {
			ComboBox * p_cb = static_cast<ComboBox *>(pDlg->getCtrlView(CtlSelCycle));
			ListWindow * p_lw = p_cb->getListWindow();
			if(p_lw) {
				ListBoxDef * p_def = p_lw->getDef();
				if(p_def) {
					SString temp_buf;
					PPLoadString("preccyclelen", temp_buf);
					p_def->addItem(PRD_PRECDAYSMASK, temp_buf);
				}
			}
			pDlg->disableCtrl(CtlPdc, cycle_id != PRD_PRECDAYSMASK);
			if(cycle_id == PRD_PRECDAYSMASK) {
				pDlg->setCtrlUInt16(CtlPdc, (cycleID & ~PRD_PRECDAYSMASK));
			}
		}
		pDlg->setCtrlLong(CtlSelCycle, cycle_id);
	}
	return ok;
}

int CycleCtrlGroup::Recalc(TDialog * pDlg, uint leaderCtl)
{
	InpUpdLock++;
	int    ok = 1;
	int    enable_pdc = 0;
	PPCycleFilt cf;
	DateRange prd, prev_prd;
	PPCycleArray ca;
	long   c = pDlg->getCtrlLong(CtlSelCycle);
	cf.Cycle = (int16)c;
	if(cf.Cycle) {
		int   num_cycles = pDlg->getCtrlLong(CtlNumCycles);
		if(num_cycles <= 0 && (leaderCtl && leaderCtl != CtlNumCycles))
			num_cycles = 1;
		if((cf.Cycle & PRD_PRECDAYSMASK) == PRD_PRECDAYSMASK) {
			cf.Cycle = (int16)PRD_PRECDAYSMASK;
			uint16 pdc = pDlg->getCtrlUInt16(CtlPdc);
			if((pdc == 0 || pdc > (0xffffffff & ~PRD_PRECDAYSMASK)) && (leaderCtl && leaderCtl != CtlPdc))
				pdc = 1;
			cf.Cycle |= pdc;
			enable_pdc = 1;
		}
		GetPeriodInput(NZOR(P_PrdDialog, pDlg), CtlPeriod, &prd);
		prev_prd = prd;
		if(leaderCtl) {
			if(leaderCtl == CtlPeriod)
				cf.NumCycles = 0;
			else if(leaderCtl == CtlNumCycles) {
				cf.NumCycles = num_cycles;
				prd.upp = ZERODATE;
			}
			else if(leaderCtl == CtlSelCycle)
				cf.NumCycles = 0;
			else
				cf.NumCycles = num_cycles;
		}
		else {
			cf.NumCycles = num_cycles;
		}
		ca.init(&prd, cf);
		ca.getCycleParams(&prd, &cf);
		if(!prd.IsEq(prev_prd) && (leaderCtl && leaderCtl != CtlPeriod))
			SetPeriodInput(NZOR(P_PrdDialog, pDlg), CtlPeriod, prd);
		if(leaderCtl && leaderCtl != CtlNumCycles)
			pDlg->setCtrlData(CtlNumCycles, &cf.NumCycles);
	}
	if(CtlPdc)
		pDlg->disableCtrl(CtlPdc, !enable_pdc);
	InpUpdLock--;
	return ok;
}

void CycleCtrlGroup::handleEvent(TDialog * pDlg, TEvent & event)
{
	if(pDlg) {
		if(CtlPeriod) {
			if(event.isCbSelected(CtlSelCycle)) {
				Recalc(pDlg, CtlSelCycle);
				pDlg->clearEvent(event);
			}
			else if(CtlNumCycles && event.isCmd(cmInputUpdatedByBtn) && event.getCtlID() == CtlPeriod) {
				DateRange prd;
				if(GetPeriodInput(NZOR(P_PrdDialog, pDlg), CtlPeriod, &prd) > 0) {
					if(!InpUpdLock) {
						Recalc(pDlg, CtlPeriod);
					}
				}
				pDlg->clearEvent(event);
			}
			else if(event.isCmd(cmInputUpdated)) {
				if(!InpUpdLock) {
					if(CtlNumCycles && event.isCtlEvent(CtlNumCycles)) {
						Recalc(pDlg, CtlNumCycles);
					}
					else if(CtlPdc && event.isCtlEvent(CtlPdc)) {
						Recalc(pDlg, CtlPdc);
					}
				}
			}
		}
		else if(CtlPdc) {
			if(event.isCbSelected(CtlSelCycle)) {
				int    enable_pdc = 0;
				PPCycleFilt cf;
				const long c = pDlg->getCtrlLong(CtlSelCycle);
				cf.Cycle = (int16)c;
				if((cf.Cycle & PRD_PRECDAYSMASK) == PRD_PRECDAYSMASK) {
					cf.Cycle = (int16)PRD_PRECDAYSMASK;
					enable_pdc = 1;
				}
				pDlg->disableCtrl(CtlPdc, !enable_pdc);
				pDlg->clearEvent(event);
			}
		}
	}
}

int CycleCtrlGroup::setData(TDialog * pDlg, void * pData)
{
	Rec    rec;
	if(pData)
		rec = *static_cast<Rec *>(pData);
	if(setupCycleCombo(pDlg, rec.C.Cycle)) {
		pDlg->setCtrlData(CtlNumCycles, &rec.C.NumCycles);
		return 1;
	}
	else
		return 0;
}

int CycleCtrlGroup::getData(TDialog * pDlg, void * pData)
{
	int    ok = 1;
	long   c = 0;
	Rec    rec;
	pDlg->getCtrlData(CtlSelCycle, &c);
	rec.C.Cycle = (int16)c;
    if(c == PRD_PRECDAYSMASK) {
        uint16 pdc = pDlg->getCtrlUInt16(CtlPdc);
        if(pdc == 0 || (pdc > (0xffffffff & ~PRD_PRECDAYSMASK))) {
            pDlg->selectCtrl(CtlPdc);
            PPSetError(PPERR_INVCYCLEPDC, (long)pdc);
            ok = 0;
        }
        else {
        	rec.C.Cycle |= pdc;
        }
    }
    if(ok) {
		pDlg->getCtrlData(CtlNumCycles, &rec.C.NumCycles);
		if(pData)
			*static_cast<Rec *>(pData) = rec;
    }
	return ok;
}
//
//
//
static int SplitPath(const char * pDirNFile, SString & rDir, SString & rFile)
{
	int    ok = -1;
	rDir.Z();
	rFile.Z();
	if(!isempty(pDirNFile)) {
		SFsPath  ps(pDirNFile);
		ps.Merge(0, SFsPath::fNam|SFsPath::fExt, rDir);
		ps.Split(pDirNFile);
		ps.Merge(0, SFsPath::fDrv|SFsPath::fDir, rFile);
		ok = 1;
	}
	return ok;
}
//
//
//
/*static*/int FileBrowseCtrlGroup::Setup(TDialog * dlg, uint btnCtlID, uint inputCtlID, uint grpID, int titleTextId, int patternId, long flags)
{
	int    ok = 1;
	if(dlg) {
		SString title;
		if(titleTextId)
			PPLoadTextWin(titleTextId, title);
		FileBrowseCtrlGroup * p_fbb = new FileBrowseCtrlGroup(btnCtlID, inputCtlID, title, flags);
		if(p_fbb) {
			p_fbb->addPattern(patternId);
			dlg->addGroup(grpID, p_fbb);
			if(btnCtlID) {
				TButton * p_btn = static_cast<TButton *>(dlg->getCtrlView(btnCtlID));
				CALLPTRMEMB(p_btn, SetBitmap(IDB_FILEBROWSE));
			}
		}
		else
			ok = PPSetErrorNoMem();
	}
	else
		ok = -1;
	return ok;
}

FileBrowseCtrlGroup::Rec::Rec()
{
	FilePath[0] = 0;
}

FileBrowseCtrlGroup::FileBrowseCtrlGroup(uint buttonId, uint inputId, const char * pTitle, long flags) :
	ButtonCtlId(buttonId), InputCtlId(inputId), Flags(flags), Title(pTitle)
{
	if(Title.Strip().IsEmpty()) {
		PPGetSubStr(PPTXT_FILE_OR_PATH_SELECTION, (Flags & fbcgfPath) ? PPFOPS_PATH : PPFOPS_FILE, Title);
		Title.Transf(CTRANSF_INNER_TO_OUTER);
	}
}

int FileBrowseCtrlGroup::addPattern(uint strID)
{
	int    ok = 1;
	SString temp_buf, name, pattern;
	SETIFZ(strID, PPTXT_FILPAT_ALL);
	if(PPLoadTextWin(strID, temp_buf) && temp_buf.Divide(':', name, pattern) > 0) {
		Patterns.add(name);
		Patterns.add(pattern);
	}
	else
		ok = 0;
	return ok;
}

void FileBrowseCtrlGroup::setInitPath(const char * pInitPath)
{
	SString  dir;
	SString  fname;
	const  bool is_wild = IsWild(pInitPath);
	bool   is_exists = false;
	if(is_wild) {
		SplitPath(pInitPath, dir, fname);
		InitDir = dir;
		InitFile = fname;
	}
	else {
		if(!isempty(pInitPath)) {
			// @v12.2.4 not_exist = ::access(pInitPath, 0);
			is_exists = fileExists(pInitPath); // @v12.2.4
			if(!is_exists) {
				SplitPath(pInitPath, dir, fname);
				// @v12.2.4 not_exist = ::access(dir, 0);
				is_exists = fileExists(dir); // @v12.2.4
				fname.Z();
			}
			else if(Flags & fbcgfPath) {
				dir = pInitPath;
				fname.Z();
			}
			else
				SplitPath(pInitPath, dir, fname);
		}
		if(!is_exists && Data.FilePath[0]) {
			// @v12.2.4 not_exist = ::access(Data.FilePath, 0);
			is_exists = fileExists(Data.FilePath); // @v12.2.4
			if(!is_exists) {
				SplitPath(Data.FilePath, dir, fname);
				// @v12.2.4 not_exist = ::access(dir, 0);
				is_exists = fileExists(dir); // @v12.2.4
				fname.Z();
			}
			else if(Flags & fbcgfPath) {
				dir = Data.FilePath;
				fname.Z();
			}
			else
				SplitPath(Data.FilePath, dir, fname);
		}
		if(!is_exists) {
			if(Flags)
				PPGetPath((Flags & fbcgfLogFile) ? PPPATH_LOG : PPPATH_SYSROOT, dir);
			else
				dir.Z();
		}
		InitDir  = dir;
		fname.ShiftLeftChr('\\').ShiftLeftChr('\\');
		InitFile = (Flags & fbcgfPath) ? "*.*" : fname;
	}
}

static const TCHAR * MakeOpenFileInitPattern(const StringSet & rPattern, STempBuffer & rResult)
{
	const size_t src_pattern_len = rPattern.getDataLen();
	if(src_pattern_len == 0)
		return 0;
	else if(sizeof(TCHAR) == sizeof(wchar_t)) {
		SString & r_temp_buf = SLS.AcquireRvlStr();
		rResult.Alloc((src_pattern_len * sizeof(TCHAR)) + 64); // @safe(64)
		size_t result_pos = 0;
		for(uint ssp = 0; rPattern.get(&ssp, r_temp_buf);) {
			memcpy(static_cast<wchar_t *>(rResult.vptr()) + result_pos, SUcSwitchW(r_temp_buf), r_temp_buf.Len() * sizeof(wchar_t));
			result_pos += r_temp_buf.Len();
			static_cast<wchar_t *>(rResult.vptr())[result_pos++] = 0;
		}
		static_cast<wchar_t *>(rResult.vptr())[result_pos++] = 0;
		return static_cast<TCHAR *>(rResult.vptr());
	}
	else
		return SUcSwitch(rPattern.getBuf());
}

int FileBrowseCtrlGroup::showFileBrowse(TDialog * pDlg)
{
	int    ok = -1;
	OPENFILENAME sofn;
	TCHAR  file_name[1024];
	SString temp_buf;
	SString result_file_name;
	SString reg_key_buf;
	STempBuffer filter_buf(16);
	reg_key_buf.Cat("FileBrowseLastPath").CatChar('(').Cat(pDlg->GetId()).CatDiv(',', 2).Cat(InputCtlId).CatChar(')');
	RecentItemsStorage ris(reg_key_buf, 20, PTR_CMPFUNC(FilePathUtf8));
	StringSet ss_ris;
	file_name[0] = 0;
	pDlg->getCtrlString(InputCtlId, temp_buf);
	temp_buf.Transf(CTRANSF_INNER_TO_OUTER);
	if(temp_buf.IsEmpty() || !fileExists(temp_buf))
		temp_buf = InitFile;
	STRNSCPY(file_name, SUcSwitch(temp_buf));
	if(!InitDir.NotEmptyS()) {
		if(Flags & fbcgfSaveLastPath) {
			if(ris.GetList(ss_ris) > 0) {
				ss_ris.reverse();
				for(uint ssp = 0; ss_ris.get(&ssp, temp_buf);) {
					temp_buf.Transf(CTRANSF_UTF8_TO_OUTER);
					if(SFile::IsDir(temp_buf)) {
						InitDir = temp_buf;
						break;
					}
				}
			}
		}
		if(!InitDir.NotEmptyS())
			setInitPath(SUcSwitch(file_name));
	}
	if(Flags & fbcgfPath) {
		SVerT dllver = SDynLibrary::GetVersion("shell32.dll");
		if(dllver.IsGe(4, 0, 0)) {
			struct FolderBrowserHandler {
				struct Param {
					SStringU InitPath;
				};
				static int CALLBACK Proc(HWND hwnd, UINT uMsg, LPARAM lp, LPARAM pData)
				{
					switch(uMsg) {
						case BFFM_INITIALIZED:
							{
								Param * p_param = reinterpret_cast<Param *>(pData);
								if(p_param && p_param->InitPath.Len()) {
									//if(GetCurrentDirectory(sizeof(szDir) / sizeof(TCHAR), szDir)) {
									// WParam is TRUE since you are passing a path. It would be FALSE if you were passing a pidl. 
									::SendMessage(hwnd, BFFM_SETSELECTION, TRUE, (LPARAM)static_cast<const wchar_t *>(p_param->InitPath));
								}
							}
							break;
						case BFFM_SELCHANGED:
							// Set the status window to the currently selected path. 
							{
								TCHAR szDir[MAX_PATH];
								if(SHGetPathFromIDList((LPITEMIDLIST)lp, szDir)) {
									::SendMessage(hwnd, BFFM_SETSTATUSTEXT, 0, (LPARAM)szDir);
								}
							}
							break;
					}
					return 0;
				}
			};
			FolderBrowserHandler::Param hp;
			hp.InitPath.CopyFromMb_OUTER(InitDir, InitDir.Len());
			BROWSEINFOW bi;
			MEMSZERO(bi);
			bi.hwndOwner = pDlg->H();
			bi.pidlRoot = NULL;
			bi.pszDisplayName = NULL;
			bi.lpszTitle = SUcSwitch(Title);
			// BIF_NEWDIALOGSTYLE is supported by Win 2000 or later (Version 5.0) 
			bi.ulFlags = BIF_NEWDIALOGSTYLE|BIF_EDITBOX|BIF_STATUSTEXT|BIF_RETURNONLYFSDIRS|BIF_RETURNFSANCESTORS;
			bi.lpfn = FolderBrowserHandler::Proc;
			bi.lParam = reinterpret_cast<LPARAM>(&hp);
			bi.iImage = 0;
			LPITEMIDLIST pidl = SHBrowseForFolderW(&bi);
			if(pidl) {
				WCHAR result_path[MAX_PATH];
				// Convert the item ID list's binary representation into a file system path 
				SHGetPathFromIDListW(pidl, result_path);
				result_file_name.CopyUtf8FromUnicode(result_path, sstrlen(result_path), 1);
				{
					// Allocate a pointer to an IMalloc interface. Get the address of our task allocator's IMalloc interface. 
					LPMALLOC p_malloc;
					SHGetMalloc(&p_malloc);
					p_malloc->Free(pidl); // Free the item ID list allocated by SHGetSpecialFolderLocation 
					p_malloc->Release(); // Free our task allocator 
				}
				ok = 1;
			}
		}
	}
	else {
		memzero(&sofn, sizeof(sofn));
		sofn.lStructSize = sizeof(sofn);
		sofn.hwndOwner   = pDlg->H();
		sofn.lpstrFilter = IsWild(InitFile) ? SUcSwitch(InitFile) : MakeOpenFileInitPattern(Patterns, filter_buf);
		sofn.lpstrFile   = file_name;
		sofn.nMaxFile    = SIZEOFARRAY(file_name);
		sofn.lpstrTitle  = SUcSwitch(Title);
		sofn.Flags       = (OFN_EXPLORER|OFN_HIDEREADONLY|OFN_LONGNAMES|OFN_NOCHANGEDIR);
		if(!(Flags & fbcgfAllowNExists))
			sofn.Flags |= OFN_FILEMUSTEXIST;
		if(Flags & fbcgfPath)
			sofn.Flags  |= (OFN_NOVALIDATE|OFN_PATHMUSTEXIST);
		sofn.lpstrInitialDir = SUcSwitch(InitDir);
		ok = ::GetOpenFileName(/*(LPOPENFILENAME)*/&sofn);
		if(ok) {
			result_file_name = SUcSwitch(file_name);
		}
	}
	if(ok > 0) {
		SFsPath ps(result_file_name);
		ps.Merge(SFsPath::fDrv|SFsPath::fDir, InitDir);
		if(Flags & fbcgfSaveLastPath) {
			(temp_buf = InitDir).Transf(CTRANSF_OUTER_TO_UTF8);
			ris.CheckIn(temp_buf);
		}
		/*if(Flags & fbcgfPath) {
			if(sofn.nFileExtension != 0)
				file_name[sofn.nFileOffset] = 0;
			result_file_name.RmvLastSlash();
		}*/
		if(InputCtlId) {
			result_file_name.Transf(CTRANSF_OUTER_TO_INNER);
			pDlg->setCtrlString(InputCtlId, result_file_name);
			TView::messageBroadcast(pDlg, cmCommitInput, pDlg->getCtrlView(InputCtlId));
			result_file_name.Transf(CTRANSF_INNER_TO_OUTER);
		}
	}
	else
		result_file_name.Z();
	STRNSCPY(Data.FilePath, result_file_name);
	return ok;
}

void FileBrowseCtrlGroup::handleEvent(TDialog * pDlg, TEvent &event)
{
	if(TVCOMMAND)
		if(TVCMD == cmGroupInserted) {
			pDlg->clearEvent(event);
		}
		else if(TVCMD == 0 && event.isCtlEvent(ButtonCtlId) || TVCMD == ButtonCtlId) {
			showFileBrowse(pDlg);
			pDlg->clearEvent(event);
		}
}

int FileBrowseCtrlGroup::getData(TDialog *, void * pData)
{
	return pData ? (*static_cast<Rec *>(pData) = Data, 1) : 0;
}

int PPOpenFile(uint strID, SString & rPath, long flags, HWND owner)
{
	int    ok = -1;
	SString temp_buf, name, pattern;
	if(PPLoadTextWin(strID, temp_buf)) {
		StringSet ss_pat;
		StringSet ss(',', temp_buf);
		for(uint i = 0; ss.get(&i, temp_buf);) {
			if(temp_buf.Divide(':', name, pattern) > 0) {
				ss_pat.add(name);
				ss_pat.add(pattern);
			}
		}
		ok = PPOpenFile(rPath, ss_pat, flags, owner);
	}
	return ok;
}

int PPOpenFile(SString & rPath, const StringSet & rPatterns, long flags, HWND owner)
{
	int    ok = -1;
	OPENFILENAME sofn;
	TCHAR  file_name[MAX_PATH];
	SString title_buf;
	SString dir;
	SString fname;
	STempBuffer filter_buf(64);
	SplitPath(rPath, dir, fname);
	STRNSCPY(file_name, SUcSwitch(fname));
	memzero(&sofn, sizeof(sofn));
	sofn.lStructSize = sizeof(sofn);
	sofn.hwndOwner   = NZOR(owner, GetForegroundWindow());
	sofn.lpstrFilter = MakeOpenFileInitPattern(rPatterns, filter_buf);
	sofn.lpstrFile   = file_name;
	sofn.nMaxFile    = SIZEOFARRAY(file_name);
	PPLoadString("fileopen", title_buf);
	title_buf.Transf(CTRANSF_INNER_TO_OUTER);
	sofn.lpstrTitle  = SUcSwitch(title_buf);
	sofn.Flags = (OFN_EXPLORER|OFN_HIDEREADONLY|OFN_LONGNAMES|OFN_NOCHANGEDIR);
	if(!(flags & ofilfNExist))
		sofn.Flags |= OFN_FILEMUSTEXIST;
	sofn.lpstrInitialDir = SUcSwitch(dir);
	ok = GetOpenFileName(&sofn);
	if(!ok)
		file_name[0] = 0;
	rPath = SUcSwitch(file_name);
	return ok;
}

int PPOpenDir(SString & rPath, const char * pTitle, HWND owner /*=0*/)
{
	int    ok = -1;
	OPENFILENAME sofn;
	TCHAR  file_name[1024];
	StringSet patterns;
	SString   title;
	SString result_file_name;
	STempBuffer filter_buf(64);
	(title = pTitle).Transf(CTRANSF_INNER_TO_OUTER);
	{
		SString temp_buf, name, patt;
		if(!PPLoadTextWin(PPTXT_FILPAT_ALL, temp_buf) || temp_buf.Divide(':', name, patt) <= 0) {
			name = " ";
			patt = "*.*";
		}
		patterns.add(name);
		patterns.add(patt);
		file_name[0] = 0;
		//patt.CopyTo(file_name, SIZEOFARRAY(file_name));
		STRNSCPY(file_name, SUcSwitch(patt));
	}
	MEMSZERO(sofn);
	sofn.lStructSize = sizeof(sofn);
	sofn.hwndOwner   = GetForegroundWindow();
	sofn.lpstrFilter = MakeOpenFileInitPattern(patterns, filter_buf);
	sofn.lpstrFile   = file_name;
	sofn.nMaxFile    = SIZEOFARRAY(file_name);
	sofn.lpstrTitle  = SUcSwitch(title);
	sofn.Flags       = (OFN_EXPLORER|OFN_HIDEREADONLY|OFN_LONGNAMES|OFN_NOCHANGEDIR|OFN_NOVALIDATE|OFN_FILEMUSTEXIST);
	sofn.lpstrInitialDir = SUcSwitch(rPath);
	ok = GetOpenFileName(/*(LPOPENFILENAME)*/&sofn);
	if(ok != 0) {
		if(sofn.nFileExtension != 0)
			file_name[sofn.nFileOffset] = 0;
		result_file_name = SUcSwitch(file_name);
		result_file_name.RmvLastSlash();
	}
	rPath = result_file_name;
	return ok;
}

class ExtOpenFileDlg : public TDialog {
public:
	ExtOpenFileDlg(StringSet * pPatterns, SString * pDefWaitFolder) : TDialog(DLG_OPENFILE)
	{
		RVALUEPTR(Patterns, pPatterns);
		RVALUEPTR(WaitFolder, pDefWaitFolder);
		SetupCtrls();
		PPLoadText(PPTXT_BROWSEDIR, OpenDirTitle);
	}
	int    setDTS(const char * pPath);
	int    getDTS(SString & rPath, SString * pDefWaitFolder);
private:
	DECL_HANDLE_EVENT;
	void   SetupCtrls();

	StringSet Patterns;
	SString   OpenDirTitle;
	SString   DefWaitFolder;
	SString   WaitFolder;
	SString   Data;
};

IMPL_HANDLE_EVENT(ExtOpenFileDlg)
{
	TDialog::handleEvent(event);
	if(TVCOMMAND) {
		if(TVCMD == cmWaitFile) {
			SString dir, file_name;
			dir = WaitFolder;
			if(dir.NotEmptyS() && ::access(dir, 0) == 0) {
				PPWaitStart();
				if(dir.Len() > 0 && WaitNewFile(dir, file_name) > 0) {
					setCtrlString(CTL_OPENFILE_PATH, file_name);
					SDelay(1000);
					PPWaitStop();
				}
			}
		}
		else if(TVCMD == cmBrowseDir) {
			SString dir = WaitFolder;
			if(PPOpenDir(dir, OpenDirTitle, 0) > 0)
				WaitFolder = dir;
			SetupCtrls();
		}
		else if(TVCMD == cmBrowseFile) {
			SString path;
			getCtrlString(CTL_OPENFILE_PATH, path);
			path.SetIfEmpty(WaitFolder);
			if(PPOpenFile(path, Patterns, 0, 0) > 0)
				setCtrlString(CTL_OPENFILE_PATH, Data = path);

		}
		else
			return;
	}
	else
		return;
	clearEvent(event);
}

void ExtOpenFileDlg::SetupCtrls()
{
	enableCommand(cmWaitFile, BIN(::access(WaitFolder, 0) == 0));
}

int ExtOpenFileDlg::setDTS(const char * pPath)
{
	Data = pPath;
	setCtrlString(CTL_OPENFILE_PATH, Data);
	if(WaitFolder.Len() == 0) {
		SFsPath sp(Data);
		sp.Merge(0, SFsPath::fNam|SFsPath::fExt, WaitFolder);
		SetupCtrls();
	}
	return 1;
}

int ExtOpenFileDlg::getDTS(SString & rPath, SString * pDefWaitFolder)
{
	int    ok = 1;
	getCtrlString(CTL_OPENFILE_PATH, Data);
	THROW_SL(fileExists(Data));
	rPath = Data;
	ASSIGN_PTR(pDefWaitFolder, WaitFolder);
	CATCHZOK
	return ok;
}

int ExtOpenFileDialog(SString & rPath, StringSet * pPatterns, SString * pDefWaitFolder)
{
	int    ok = 0;
	int    valid_data = 0;
	ExtOpenFileDlg * p_dlg = new ExtOpenFileDlg(pPatterns, pDefWaitFolder);
	if(CheckDialogPtrErr(&p_dlg) > 0) {
		p_dlg->setDTS(rPath);
		while(!valid_data && ExecView(p_dlg) == cmOK) {
			if(p_dlg->getDTS(rPath, pDefWaitFolder) <= 0)
				PPError();
			else
				ok = valid_data = 1;
		}
	}
	delete p_dlg;
	return ok;
}
//
//
//
ImageBrowseCtrlGroup::Rec::Rec(const SString * pPath) : Flags(0), Path(pPath ? pPath->cptr() : 0)
{
}

ImageBrowseCtrlGroup::Rec::Rec(SImageBuffer * pImgBuf) : Flags(0)
{
	if(pImgBuf) {
		ImgBuf = *pImgBuf;
		Flags |= fImageBuffer;
	}
}

ImageBrowseCtrlGroup::ImageBrowseCtrlGroup(uint ctlImage, uint cmChgImage, uint cmDeleteImage, int allowChangeImage /*=1*/, long flags /*=0*/) :
	CtlImage(ctlImage), CmChgImage(cmChgImage), CmDelImage(cmDeleteImage), AllowChangeImage(allowChangeImage), Flags(flags)
{
	SString buf, name, ext;
	uint patterns_id = PPTXT_PICFILESEXTS; // can be PPTXT_FILPAT_PICT
	PPLoadTextWin(/*patternsID*/patterns_id, buf);
	StringSet ss(',', buf);
	for(uint i = 0; ss.get(&i, buf);) {
		if(buf.Divide(':', name, ext) > 0) {
			Patterns.add(name);
			Patterns.add(ext);
		}
	}
	{
		PPPersonConfig psn_cfg;
		if(PPObjPerson::ReadConfig(&psn_cfg) > 0)
			DefWaitFolder = psn_cfg.AddImageFolder;
	}
}

int ImageBrowseCtrlGroup::setData(TDialog * pDlg, void * pData)
{
	if(!RVALUEPTR(Data, static_cast<Rec *>(pData))) {
		Data.Path.Z();
		Data.ImgBuf.Destroy();
	}
	{
		TView * p_v = pDlg->getCtrlView(CtlImage);
		if(p_v && p_v->GetSubSign() == TV_SUBSIGN_IMAGEVIEW) {
			if(Data.Flags & ImageBrowseCtrlGroup::Rec::fImageBuffer) {
				TImageView * p_iv = static_cast<TImageView *>(p_v);
				SDrawImage * p_fig = new SDrawImage(Data.ImgBuf);
				if(p_fig)
					p_iv->SetOuterFigure(p_fig); // p_fig переходит в собственность p_iv
			}
			else {
				pDlg->setCtrlString(CtlImage, Data.Path);
			}
		}
	}
	if(CmChgImage)
		pDlg->enableCommand(CmChgImage, AllowChangeImage);
	if(CmDelImage)
		pDlg->enableCommand(CmDelImage, AllowChangeImage);
	pDlg->enableCommand(cmPasteImage, AllowChangeImage);
	return 1;
}

int ImageBrowseCtrlGroup::getData(TDialog * pDlg, void * pData)
{
	if(pData)
		*static_cast<Rec *>(pData) = Data;
	return 1;
}

void ImageBrowseCtrlGroup::handleEvent(TDialog * pDlg, TEvent & event)
{
	if(event.isCmd(cmImageDblClk)) {
		if(!(Flags & fDisableDetail)) {
			if(Data.Path.NotEmpty() && fileExists(Data.Path)) {
				// @debug {
				/*{
					SImageBuffer imgbuf;
					if(imgbuf.Load(Data.Path)) {
						SClipboard::Copy_Image(imgbuf);
					}
				}*/
				// } @debug
				ViewImageInfo(Data.Path, 0, 0);
			}
		}
	}
	else if(event.isCmd(CmChgImage)) {
		int    r = 0;
		SString path;
		if(Flags & fUseExtOpenDlg) {
			r = ExtOpenFileDialog(path = Data.Path, &Patterns, &DefWaitFolder);
			if(r > 0) {
				PPPersonConfig psn_cfg;
				if(PPObjPerson::ReadConfig(&psn_cfg) > 0) {
					psn_cfg.AddImageFolder = DefWaitFolder;
					PPObjPerson::WriteConfig(&psn_cfg, 1);
				}
			}
		}
		else
			r = PPOpenFile(path, Patterns, 0, 0);
		if(r > 0) {
			TView * p_v = pDlg->getCtrlView(CtlImage);
			if(p_v && p_v->GetSubSign() == TV_SUBSIGN_IMAGEVIEW) {
				if(Data.Flags & ImageBrowseCtrlGroup::Rec::fImageBuffer) {
					SImageBuffer imgbuf;
					if(imgbuf.Load(path)) {
						TImageView * p_iv = static_cast<TImageView *>(p_v);
						Data.ImgBuf = imgbuf;
						imgbuf.Destroy();
						SDrawImage * p_fig = new SDrawImage(Data.ImgBuf);
						if(p_fig) {
							p_iv->SetOuterFigure(p_fig); // p_fig переходит в собственность p_iv
							Data.Flags |= Rec::fUpdated;
							p_v->MessageCommandToOwner(cmImageChanged); // @v12.0.4
						}
					}
				}
				else {
					pDlg->setCtrlString(CtlImage, Data.Path = path);
					Data.Flags |= Rec::fUpdated;
					p_v->MessageCommandToOwner(cmImageChanged); // @v12.0.4
				}
			}
		}
		pDlg->clearEvent(event);
	}
	else if(event.isCmd(CmDelImage)) {
		TView * p_v = pDlg->getCtrlView(CtlImage);
		if(p_v && p_v->GetSubSign() == TV_SUBSIGN_IMAGEVIEW) {
			if(Data.Flags & ImageBrowseCtrlGroup::Rec::fImageBuffer) {
				TImageView * p_iv = static_cast<TImageView *>(p_v);
				p_iv->SetOuterFigure(0);
				Data.ImgBuf.Destroy();
			}
			else {
				pDlg->setCtrlString(CtlImage, Data.Path.Z());
			}
			Data.Flags |= Rec::fUpdated;
			p_v->MessageCommandToOwner(cmImageChanged); // @v12.0.4
		}
		pDlg->clearEvent(event);
	}
	else if(event.isCmd(cmPasteImage)) {
		TView * p_v = pDlg->getCtrlView(CtlImage);
		if(p_v && p_v->GetSubSign() == TV_SUBSIGN_IMAGEVIEW) {
			if(Data.Flags & ImageBrowseCtrlGroup::Rec::fImageBuffer) {
				if(SClipboard::Paste_Image(Data.ImgBuf)) {
					TImageView * p_iv = static_cast<TImageView *>(p_v);
					SDrawImage * p_fig = new SDrawImage(Data.ImgBuf);
					if(p_fig)
						p_iv->SetOuterFigure(p_fig); // p_fig переходит в собственность p_iv
				}
			}
			else {
				long   start = 0;
				SString temp_dir, temp_path;
				PPGetPath(PPPATH_TEMP, temp_dir);
				temp_dir.SetLastSlash().Cat("IMG");
				if(!SFile::IsDir(temp_dir))
					SFile::CreateDir(temp_dir);
				MakeTempFileName(temp_dir, "pst", "jpg", &start, temp_path);
				if(SClipboard::CopyPaste(GetDlgItem(pDlg->H(), CtlImage), 0, temp_path) > 0) {
					Data.Flags |= Rec::fUpdated;
					pDlg->setCtrlString(CtlImage, Data.Path = temp_path);
					pDlg->clearEvent(event);
					p_v->MessageCommandToOwner(cmImageChanged); // @v12.0.4
				}
			}
		}
	}
	else if(event.isKeyDown(kbF7)) {
		if(Data.Path.NotEmpty() && fileExists(Data.Path) && CONFIRM(PPCFM_PRINTIMAGE)) {
			SPrinting prn(APPL->H_MainWnd);
			int    prn_ok = 1;
			if(!prn.Init(0))
				prn_ok = PPSetErrorSLib();
			else if(!prn.PrintImage(Data.Path))
				prn_ok = 0;
			if(!prn_ok)
				PPError();
		}
	}
}
//
//
//
int STDCALL BarcodeInputDialog(int initChar, SString & rBuf)
{
	class BarcodeSrchDialog : public TDialog {
	public:
		BarcodeSrchDialog() : TDialog(DLG_SRCHBCODE)
		{
			disableCtrl(CTL_SRCHBCODE_CHKDIG, true);
			SetCtrlBitmap(CTL_SRCHBCODE_IMG, BM_BARCODE);
		}
	private:
		DECL_HANDLE_EVENT
		{
			if(event.isCmd(cmOK)) {
				SString code;
				BarcodeTbl::Rec  bc_rec;
				getCtrlString(CTL_SRCHBCODE_CODE, code);
				if(GObj.SearchBy2dBarcode(code, &bc_rec, 0) > 0)
					setCtrlData(CTL_SRCHBCODE_CODE, bc_rec.Code);
			}
			TDialog::handleEvent(event);
			if(event.isKeyDown(kbF2))
				setChkDig();
			else if(TVBROADCAST && TVCMD == cmChangedFocus && event.isCtlEvent(CTL_SRCHBCODE_CODE))
				setChkDig();
			else
				return;
			clearEvent(event);
		}
		void   setChkDig()
		{
			SString buf;
			getCtrlString(CTL_SRCHBCODE_CODE, buf);
			const int cd = CalcBarcodeCheckDigit(buf);
			setCtrlString(CTL_SRCHBCODE_CHKDIG, buf.Z().Cat(cd));
		}
		PPObjGoods GObj;
	};
	int    ok = -1;
	SString code;
	BarcodeSrchDialog * dlg = new BarcodeSrchDialog;
	if(CheckDialogPtrErr(&dlg)) {
		if(isalnum(initChar))
			code.CatChar(initChar);
		else
			code = rBuf;
		dlg->setCtrlString(CTL_SRCHBCODE_CODE, code);
		if(code.NotEmpty() && isalnum(initChar)) {
			TInputLine * il = static_cast<TInputLine *>(dlg->getCtrlView(CTL_SRCHBCODE_CODE));
			CALLPTRMEMB(il, disableDeleteSelection(1));
		}
		if(ExecView(dlg) == cmOK) {
			dlg->getCtrlString(CTL_SRCHBCODE_CODE, code);
			if(code.NotEmptyS())
				ok = 1;
		}
	}
	else
		ok = 0;
	delete dlg;
	rBuf = code;
	return ok;
}

int STDCALL SelectorDialog(uint dlgID, uint ctlID, uint * pVal /* IN,OUT */, const char * pTitle /*=0*/)
{
	int    ok = -1;
	TDialog * dlg = new TDialog(dlgID);
	if(CheckDialogPtrErr(&dlg)) {
		dlg->setTitle(pTitle);
		dlg->setCtrlUInt16(ctlID, *pVal);
		if(ExecView(dlg) == cmOK) {
			*pVal = dlg->getCtrlUInt16(ctlID);
			ok = 1;
		}
	}
	else
		ok = 0;
	delete dlg;
	return ok;
}
//
//
//
ListBoxSelDialog::ListBoxSelDialog(ListBoxDef * pDef, uint dlgID, uint flags/*PPListDialog::fXXX*/) :
	PPListDialog((dlgID) ? dlgID : ((pDef && pDef->_isTreeList()) ? DLG_LBXSELT : DLG_LBXSEL), CTL_LBXSEL_LIST, flags), P_Def(pDef)
{
	showCtrl(STDCTL_INSBUTTON,  false);
	showCtrl(STDCTL_EDITBUTTON, false);
	showCtrl(STDCTL_DELBUTTON,  false);
	showCtrl(CTL_LBXSEL_UPBTN,   false);
	showCtrl(CTL_LBXSEL_DOWNBTN, false);
	updateList(-1);
}

// AHTOXA {
int ListBoxSelDialog::setupList()
{
	CALLPTRMEMB(P_Box, setDef(P_Def));
	return 1;
}

int ListBoxSelDialog::editItem(long pos, long id)
{
	if(pos >= 0 && IsInState(sfModal))
		endModal(cmOK);
	return -1;
}

int ListBoxSelDialog::getDTS(PPID * pID)
{
	long   pos = 0, id = 0;
	getCurItem(&pos, &id);
	ASSIGN_PTR(pID, id);
	return 1;
}

int ListBoxSelDialog::setDTS(PPID id)
{
	CALLPTRMEMB(P_Box, TransmitData(+1, &id));
	return 1;
}

/*static*/int STDCALL ListBoxSelDialog::Run(PPID objID, PPID * pID, void * extraPtr)
{
	int    ok = -1;
	PPObject * ppobj = GetPPObject(objID, extraPtr);
	ListBoxDef * p_def = ppobj ? ppobj->Selector(0, 0, extraPtr) : 0;
	ListBoxSelDialog * p_dlg = 0;
	if(p_def) {
		p_dlg = new ListBoxSelDialog(p_def, 0, 0);
		if(CheckDialogPtrErr(&p_dlg)) {
			PPID id = DEREFPTRORZ(pID);
			SString obj_title;
			p_dlg->setSubTitle(GetObjectTitle(objID, obj_title));
			p_dlg->setDTS(id);
			if(ExecView(p_dlg) == cmOK) {
				p_dlg->getDTS(&id);
				ASSIGN_PTR(pID, id);
				ok = 1;
			}
		}
		else
			ok = 0;
	}
	else
		ok = 0;
	delete p_dlg;
	delete ppobj;
	return ok;
}

/*static*/int STDCALL ListBoxSelDialog::Run(uint dlgID, StrAssocArray * pAry, PPID * pID/*, uint flags*/)
{
	int    ok = -1;
	ListBoxSelDialog * p_dlg = 0;
	ListBoxDef * p_def = pAry ? new StrAssocListBoxDef(pAry, lbtDblClkNotify|lbtFocNotify) : 0;
	if(p_def && CheckDialogPtrErr(&(p_dlg = new ListBoxSelDialog(p_def, dlgID, 0)))) {
		PPID id = DEREFPTRORZ(pID);
		p_dlg->setDTS(id);
		if(ExecView(p_dlg) == cmOK) {
			p_dlg->getDTS(&id);
			ASSIGN_PTR(pID, id);
			ok = 1;
		}
		else
			ok = 0;
	}
	else
		ok = 0;
	delete p_dlg;
	return ok;
}

int ComboBoxSelDialog2(const StrAssocArray & rAry, uint subTitleStrId, uint labelStrId, long * pSelectedId, uint flags)
{
	int    ok = -1;
	long   sel_id = DEREFPTRORZ(pSelectedId);
	TDialog * p_dlg = new TDialog(DLG_CBXSEL);
	if(CheckDialogPtrErr(&p_dlg)) {
		SString temp_buf;
		if(subTitleStrId)
			p_dlg->setSubTitle(PPLoadTextS(subTitleStrId, temp_buf));
		if(labelStrId)
			p_dlg->setLabelText(CTL_CBXSEL_COMBO, PPLoadTextS(labelStrId, temp_buf));
		SetupStrAssocCombo(p_dlg, CTLSEL_CBXSEL_COMBO, rAry, sel_id, flags);
		if(ExecView(p_dlg) == cmOK) {
			p_dlg->getCtrlData(CTLSEL_CBXSEL_COMBO, &sel_id);
			ok = 1;
		}
	}
	else
		ok = 0;
	delete p_dlg;
	ASSIGN_PTR(pSelectedId, sel_id);
	return ok;
}

int  AdvComboBoxSelDialog(const StrAssocArray & rAry, SString & rTitle, SString & rLabel, PPID * pID, SString * pName, uint flags)
{
	int    ok = -1;
	TDialog * p_dlg = new TDialog(DLG_ADVCBXSEL);
	if(CheckDialogPtrErr(&p_dlg)) {
		PPID   id = DEREFPTRORZ(pID);
		SString subtitle, label;
		if(rTitle.Len())
			p_dlg->setSubTitle(rTitle);
		if(rLabel.Len())
			p_dlg->setLabelText(CTL_CBXSEL_COMBO, rLabel);
		SetupStrAssocCombo(p_dlg, CTLSEL_CBXSEL_COMBO, rAry, id, flags);
		if(pName)
			p_dlg->setCtrlString(CTL_CBXSEL_NAME, *pName);
		p_dlg->disableCtrl(CTL_CBXSEL_NAME, !pName);
		while(ok < 0 && ExecView(p_dlg) == cmOK) {
			p_dlg->getCtrlData(CTLSEL_CBXSEL_COMBO, &id);
			if(pName)
				p_dlg->getCtrlString(CTL_CBXSEL_NAME, *pName);
			if(!pName || pName->Len()) {
				ASSIGN_PTR(pID, id);
				ok = 1;
			}
			else
				PPErrorByDialog(p_dlg, CTL_CBXSEL_NAME, PPERR_NAMENEEDED);
		}
	}
	else
		ok = 0;
	delete p_dlg;
	return ok;
}

/*static*/int STDCALL ListBoxSelDialog::Run(StrAssocArray * pAry, uint titleStrId, PPID * pID/*, uint flags*/)
{
	SString title;
	if(titleStrId)
		PPLoadText(titleStrId, title);
	return ListBoxSelDialog::Run(pAry, title, pID/*, flags*/);
}

/*static*/int STDCALL ListBoxSelDialog::Run(StrAssocArray * pAry, const char * pTitle, PPIDArray * pIdList) // @construction
{
	int    ok = -1;
	ListBoxSelDialog * p_dlg = 0;
	ListBoxDef * p_def = pAry ? new StrAssocListBoxDef(pAry, lbtDblClkNotify|lbtFocNotify) : 0;
	if(p_def && CheckDialogPtrErr(&(p_dlg = new ListBoxSelDialog(p_def, 0, PPListDialog::fMultiselect)))) {
		{
			SString title_buf;
			if(pTitle && pTitle[0] == '@') {
				if(PPLoadString(pTitle+1, title_buf) <= 0)
					title_buf = pTitle;
			}
			else
				title_buf = pTitle;
			p_dlg->setTitle(title_buf);
		}
		PPID   id = 0;//DEREFPTRORZ(pID);
		// @stub {
		if(pIdList)
			id = pIdList->getSingle();
		// } @stub 
		p_dlg->setDTS(id);
		if(ExecView(p_dlg) == cmOK) {
			p_dlg->getDTS(&id);
			if(pIdList)
				pIdList->add(id);
			ok = 1;
		}
		else
			ok = 0;
	}
	else
		ok = 0;
	delete p_dlg;
	return ok;
}

#if 0 // @v12.3.6 (Этот блок отработал свою функцию - переходим на регулярное описание диалогов в DL00) {

// @construction {

static const float FixedCtrlHeight = 21.0f;
// {75; 21}

/*static*/const char * LayoutedListDialog_Base::GetWindowTitle(const char * pOuterTitle)
{
	return isempty(pOuterTitle) ? "" : pOuterTitle;
}

LayoutedListDialog_Base::LayoutedListDialog_Base(const Param & rParam, ListBoxDef * pDef, TWindow * pTW) : P_TW(pTW), P(rParam), P_Def__(pDef), LldState(0)
{
}
	
LayoutedListDialog_Base::~LayoutedListDialog_Base()
{
	if(!(LldState & lldsDefBailed)) {
		delete P_Def__;
	}
}

void LayoutedListDialog_Base::OnInit2(TWindowBase::CreateBlock * pBlk)
{
	{
		const TRect _def_rect(0, 0, 40, 40);
		SString font_face;
		//PPGetSubStr(PPTXT_FONTFACE, /*PPFONT_MSSANSSERIF*/PPFONT_ARIAL, font_face);
		//TView::setFont(HW, font_face, 12);
		if(P.Flags & fHeaderStaticText) {
			TStaticText * p_st = new TStaticText(_def_rect, 0/*spcFlags*/, 0);
			P_TW->InsertCtlWithCorrespondingNativeItem(p_st, STDCTL_HEADERSTATICTEXT, 0, /*extraPtr*/0);
		}
		{
			TGroupBox * p_gb = new TGroupBox(_def_rect);
			P_TW->InsertCtlWithCorrespondingNativeItem(p_gb, CtlListGroupBox, 0, /*extraPtr*/0);
		}
		{
			SmartListBox * p_lb = P.ColumnDescription.NotEmpty() ? new SmartListBox(_def_rect, P_Def__, P.ColumnDescription) : new SmartListBox(_def_rect, P_Def__, P_Def__->_isTreeList());
			if(p_lb) {
				LldState |= lldsDefBailed;
				P_TW->InsertCtlWithCorrespondingNativeItem(p_lb, STDCTL_SINGLELISTBOX, 0, /*extraPtr*/0);
				if(font_face.NotEmpty()) {
					P_TW->SetCtrlFont(STDCTL_SINGLELISTBOX, font_face, /*16*//*22*/12);
				}
			}
		}
		if(P.Flags & (fcedCreate|fcedEdit|fcedDelete)) {
			if(P.Flags & fcedCreate) {
				P_TW->InsertCtlWithCorrespondingNativeItem(new TButton(_def_rect, "@but_add", cmaInsert, 0, 0), STDCTL_INSBUTTON, 0, /*extraPtr*/0);
			}
			if(P.Flags & fcedEdit) {
				P_TW->InsertCtlWithCorrespondingNativeItem(new TButton(_def_rect, "@but_edit", cmaEdit, 0, 0), STDCTL_EDITBUTTON, 0, /*extraPtr*/0);
			}
			if(P.Flags & fcedDelete) {
				P_TW->InsertCtlWithCorrespondingNativeItem(new TButton(_def_rect, "@but_delete", cmaDelete, 0, 0), STDCTL_DELBUTTON, 0, /*extraPtr*/0);
			}
		}
		P_TW->InsertCtlWithCorrespondingNativeItem(new TButton(_def_rect, "@but_ok", cmOK, 0, 0), STDCTL_OKBUTTON, 0, /*extraPtr*/0);
		P_TW->InsertCtlWithCorrespondingNativeItem(new TButton(_def_rect, "@but_cancel", cmCancel, 0, 0), STDCTL_CANCELBUTTON, 0, /*extraPtr*/0);
	}
	{
		class InnerBlock {
		public:
			static void __stdcall SetupLayoutItemFrameProc(SUiLayout * pItem, const SUiLayout::Result & rR)
			{
				if(pItem) {
					TView * p = static_cast<TView *>(SUiLayout::GetManagedPtr(pItem));
					if(p)
						p->changeBounds(TRect(pItem->GetFrameAdjustedToParent()));
				}
			}
			static void InsertButtonLayout(TWindow * pMaster, SUiLayout * pLoParent, ushort ctlId, SUiLayoutParam & rP, float growFactor)
			{
				TView * p = pMaster ? pMaster->getCtrlView(ctlId) : 0;
				if(p) {
					rP.GrowFactor = growFactor;
					SUiLayout * p_lo_item = pLoParent->InsertItem(p, &rP);
					p_lo_item->SetCallbacks(0, InnerBlock::SetupLayoutItemFrameProc, p);
				}
			}		
			static SUiLayout * InsertCtrlLayout(TWindow * pMaster, SUiLayout * pLoParent, ushort ctlId, SUiLayoutParam & rP, float growFactor)
			{
				SUiLayout * p_lo_item = 0;
				TView * p = pMaster ? pMaster->getCtrlView(ctlId) : 0;
				if(p) {
					rP.GrowFactor = growFactor;
					p_lo_item = pLoParent->InsertItem(p, &rP);
					p_lo_item->SetCallbacks(0, InnerBlock::SetupLayoutItemFrameProc, p);
				}
				return p_lo_item;
			}		
		};
		SUiLayout * p_lo_result = 0;
		{
			const UiDescription * p_uid = SLS.GetUiDescription();
			if(p_uid) {
				const SUiLayout * p_lo = p_uid->GetLayoutBySymbC("listdialog");
				if(p_lo)
					p_lo_result = new SUiLayout(*p_lo);
			}
		}
		if(p_lo_result) {
			SUiLayout * p_lo_header = p_lo_result->FindBySymb("listdialog_header");
			SUiLayout * p_lo_footer = p_lo_result->FindBySymb("listdialog_footer");
			SUiLayout * p_lo_body = p_lo_result->FindBySymb("listdialog_body");
			SUiLayout * p_lo_groupbox = 0;
			if(P.Flags & fHeaderStaticText) {
				if(p_lo_header) {
					TView * p = P_TW->getCtrlView(STDCTL_HEADERSTATICTEXT);
					if(TView::IsSubSign(p, TV_SUBSIGN_STATIC)) {
						
					}
					SUiLayoutParam alb;
					alb.GrowFactor = 1.0f;
					//alb.SetFixedSizeY(FixedCtrlHeight);
					alb.SetVariableSizeX(SUiLayoutParam::szByContainer, 1.0f);
					alb.SetVariableSizeY(SUiLayoutParam::szByContainer, 1.0f);
					alb.Margin.Set(DefMargin);
					InnerBlock::InsertCtrlLayout(P_TW, p_lo_header, STDCTL_HEADERSTATICTEXT, alb, 1.0f);
				}
			}
			{
				SUiLayoutParam alb(DIREC_VERT);
				alb.GrowFactor = 1.0f;
				//alb.SetFixedSizeY(FixedCtrlHeight);
				alb.SetVariableSizeX(SUiLayoutParam::szByContainer, 1.0f);
				alb.SetVariableSizeY(SUiLayoutParam::szByContainer, 1.0f);
				alb.Margin.Set(DefMargin);
				alb.Padding.a.y = 8.0f;
				//alb.Padding.b.x += 20.0f;
				p_lo_groupbox = InnerBlock::InsertCtrlLayout(P_TW, p_lo_body, CtlListGroupBox, alb, 1.0f);
			}
			{
				SUiLayoutParam alb;
				alb.GrowFactor = 1.0f;
				//alb.SetFixedSizeY(FixedCtrlHeight);
				alb.SetVariableSizeX(SUiLayoutParam::szByContainer, 1.0f);
				alb.SetVariableSizeY(SUiLayoutParam::szByContainer, 1.0f);
				alb.Margin.Set(8.0f);
				{					
					TView * p = P_TW->getCtrlView(STDCTL_SINGLELISTBOX);
					if(TView::IsSubSign(p, TV_SUBSIGN_LISTBOX)) {
						const SmartListBox * p_lb = static_cast<SmartListBox *>(p);
						if(!p_lb->IsMultiColumn() && !p_lb->IsTreeList()) // Припуск для скролл-бара не нужен для мультиколоночного списка и для treeview
							alb.Margin.b.x += 20.0f; 
					}
				}
				//alb.Padding.Set(def_margin);
				//alb.Padding.b.x += 20.0f;
				InnerBlock::InsertCtrlLayout(P_TW, NZOR(p_lo_groupbox, p_lo_body), STDCTL_SINGLELISTBOX, alb, 1.0f);
			}
			if(p_lo_groupbox) {
				if(P.Flags & (fcedCreate|fcedEdit|fcedDelete)) {
					SUiLayout * p_lo_ced_button_group = 0;
					{
						SUiLayoutParam alb(DIREC_HORZ);
						alb.GrowFactor = 1.0f;
						alb.SetVariableSizeX(SUiLayoutParam::szByContainer, 1.0f);
						alb.SetFixedSizeY(68.0f);
						alb.Margin.Set(1.0f);
						//alb.Margin.b.x += 20.0f;
						//alb.Padding.Set(def_margin);
						//alb.Padding.b.x += 20.0f;
						p_lo_ced_button_group = p_lo_groupbox->InsertItem(0, &alb);
					}
					if(p_lo_ced_button_group) {
						SUiLayoutParam alb;
						alb.GrowFactor = 0.0f;
						alb.SetFixedSizeX(80.0f);
						alb.SetFixedSizeY(FixedCtrlHeight);
						alb.Margin.Set(DefMargin);
						alb.Padding.Set(DefMargin);
						if(P.Flags & fcedCreate) {
							InnerBlock::InsertButtonLayout(P_TW, p_lo_ced_button_group, STDCTL_INSBUTTON, alb, 0.0f);
						}
						if(P.Flags & fcedEdit) {
							InnerBlock::InsertButtonLayout(P_TW, p_lo_ced_button_group, STDCTL_EDITBUTTON, alb, 0.0f);
						}
						if(P.Flags & fcedDelete) {
							InnerBlock::InsertButtonLayout(P_TW, p_lo_ced_button_group, STDCTL_DELBUTTON, alb, 0.0f);
						}							
					}
				}
			}
			{
				SUiLayoutParam alb;
				alb.GrowFactor = 0.0f;
				alb.SetFixedSizeX(80.0f);
				alb.SetFixedSizeY(FixedCtrlHeight);
				alb.Margin.Set(DefMargin);
				alb.Padding.Set(DefMargin);
				InnerBlock::InsertButtonLayout(P_TW, p_lo_footer, STDCTL_OKBUTTON, alb, 0.0f);
				InnerBlock::InsertButtonLayout(P_TW, p_lo_footer, STDCTL_CANCELBUTTON, alb, 0.0f);
			}
		}
		P_TW->SetLayout(p_lo_result);
	}
	P_TW->EvaluateLayout(pBlk->Coord);
}

void LayoutedListDialog_Base::DrawLayout(TCanvas2 & rCanv, const SUiLayout * pLo)
{
	if(pLo) {
		{
			FRect lo_rect = pLo->GetFrameAdjustedToParent();
			int   pen_ident = 0;
			int   brush_ident = 0;
			TWhatmanToolArray::Item tool_item;
			const SDrawFigure * p_fig = 0;
			SPaintToolBox * p_tb = APPL->GetUiToolBox();
			if(p_tb) {
				SString symb;
				// Прежде всего закрасим фон
				rCanv.Rect(lo_rect, 0, TProgram::tbiListBkgBrush);
				if(pen_ident) {
					/*if(pLo == P_LoFocused) {
						pen_ident = TProgram::tbiIconAccentColor;
					}*/
					rCanv.Rect(lo_rect);
					rCanv.Stroke(pen_ident, 0);
				}
				if(brush_ident) {
					rCanv.Rect(lo_rect);
					rCanv.Fill(brush_ident, 0);
				}
				if(p_fig) {
					const uint _w = 16;
					const uint _h = 16;
					SImageBuffer ib(_w, _h);
					{
						if(!tool_item.ReplacedColor.IsEmpty()) {
							SColor replacement_color = p_tb->GetColor(TProgram::tbiIconRegColor);
							rCanv.SetColorReplacement(tool_item.ReplacedColor, replacement_color);
						}
						LMatrix2D mtx;
						SViewPort vp;
						rCanv.Fill(SColor(192, 192, 192, 255), 0); // Прозрачный фон
						rCanv.PushTransform();
						p_fig->GetViewPort(&vp);
						rCanv.PushTransform();
						rCanv.AddTransform(vp.GetMatrix(lo_rect, mtx));
						rCanv.Draw(p_fig);
						rCanv.PopTransform();
					}
				}
			}
		}
		for(uint ci = 0; ci < pLo->GetChildrenCount(); ci++) {
			DrawLayout(rCanv, pLo->GetChildC(ci)); // @recursion
		}
	}
}

/*static*/int LayoutedListDialog_Base::Exec(TWindow * pView)
{
	int    ok = -1;
	if(pView) {
		const UiDescription * p_uid = SLS.GetUiDescription();
		const SUiLayout * p_lo = 0;
		THROW(p_uid);
		p_lo = p_uid->GetLayoutBySymbC("listdialog");
		THROW(p_lo);
		ok = APPL->P_DeskTop->execView(pView);
		if(ok > 0) {
			//p_win->getDTS(&rData);
		}
	}
	CATCHZOK
	delete pView;
	return ok;
}

LayoutedListWindow::LayoutedListWindow(const Param & rParam, ListBoxDef * pDef) : 
	TWindowBase(SUcSwitch(LayoutedListDialog_Base::GetWindowTitle(rParam.Title)), wbcDrawBuffer),
	LayoutedListDialog_Base(rParam, pDef, static_cast<TWindow *>(this))
{
}
	
LayoutedListWindow::~LayoutedListWindow()
{
}

IMPL_HANDLE_EVENT(LayoutedListWindow)
{
	TWindowBase::handleEvent(event);
	if(event.isKeyDown(kbEsc)) {
		if(IsInState(sfModal)) {
			EndModalCmd = cmCancel;
			clearEvent(event);
		}
	}
	else if(TVINFOPTR) {
		if(event.isCmd(cmInit)) {
			OnInit2(static_cast<TWindowBase::CreateBlock *>(TVINFOPTR));
			// don't clearEvent
		}
		else if(event.isCmd(cmClose)) {
			if(IsInState(sfModal)) {
				EndModalCmd = cmCancel;
				clearEvent(event);
			}
		}
		else if(event.isCmd(cmOK) || event.isCmd(cmCancel)) {
			if(IsInState(sfModal)) {
				EndModalCmd = event.message.command;
				clearEvent(event);
			}
			else if(event.message.command == cmCancel) {
				close();
				return; // Окно разрушено - делать в этой процедуре больше нечего!
			}
		}
		else if(event.isCmd(cmPaint)) {
			PaintEvent * p_blk = static_cast<PaintEvent *>(TVINFOPTR);
			//CreateFont_();
			if(oneof2(p_blk->PaintType, PaintEvent::tPaint, PaintEvent::tEraseBackground)) {
				SPaintToolBox * p_tb = APPL->GetUiToolBox();
				if(p_tb) {
					if(GetWbCapability() & wbcDrawBuffer) {
						// Если используется буферизованная отрисовка, то фон нужно перерисовать в любом случае а на событие PaintEvent::tEraseBackground
						// не реагировать
						if(p_blk->PaintType == PaintEvent::tPaint) {
							TCanvas2 canv(*p_tb, static_cast<HDC>(p_blk->H_DeviceContext));
							canv.Rect(p_blk->Rect, 0, TProgram::tbiListBkgBrush);
							DrawLayout(canv, P_Lfc);
						}
					}
					else {
						TCanvas2 canv(*p_tb, static_cast<HDC>(p_blk->H_DeviceContext));
						if(p_blk->PaintType == PaintEvent::tEraseBackground)
							canv.Rect(p_blk->Rect, 0, TProgram::tbiListBkgBrush);
						if(p_blk->PaintType == PaintEvent::tPaint)
							DrawLayout(canv, P_Lfc);
					}
				}
				clearEvent(event);
			}
		}
	}
}

/*virtual*/SmartListBox * LayoutedListDialog_Base::GetListBoxCtl() const
{
	TView * p_box_view = P_TW ? P_TW->getCtrlView(STDCTL_SINGLELISTBOX) : 0;
	return TView::IsSubSign(p_box_view, TV_SUBSIGN_LISTBOX) ? static_cast<SmartListBox *>(p_box_view) : 0;
}

LayoutedListDialog::LayoutedListDialog(const Param & rParam, ListBoxDef * pDef) : 
	TDialog(LayoutedListDialog_Base::GetWindowTitle(rParam.Title), wbcDrawBuffer|wbcStorableUserParams, TDialog::coEmpty),
	LayoutedListDialog_Base(rParam, pDef, static_cast<TWindow *>(this))
{
	if(rParam.Symb.NotEmpty())
		SetStorableUserParamsSymb(rParam.Symb);
	if(!rParam.Bounds.IsEmpty()) {
		TDialog::setBounds(rParam.Bounds);
	}
	TDialog::BuildEmptyWindow(0);
}
	
LayoutedListDialog::~LayoutedListDialog()
{
}

IMPL_HANDLE_EVENT(LayoutedListDialog)
{
	if(event.isCmd(cmInit)) {
		if(TVINFOPTR)
			OnInit2(static_cast<TWindowBase::CreateBlock *>(TVINFOPTR));
		// don't clearEvent
	}
	TDialog::handleEvent(event);
	if(TVCOMMAND) {
		switch(TVCMD) {
			case cmClose:
				if(IsInState(sfModal)) {
					EndModalCmd = cmCancel;
					clearEvent(event);
				}
				else {
					close();
					return; // Окно разрушено - делать в этой процедуре больше нечего!
				}
				break;
			case cmOK:
				if(IsInState(sfModal)) {
					EndModalCmd = event.message.command;
					clearEvent(event);
				}
				break;
			case cmCancel:
				if(IsInState(sfModal)) {
					EndModalCmd = event.message.command;
					clearEvent(event);
				}
				else {
					close();
					return; // Окно разрушено - делать в этой процедуре больше нечего!
				}
				break;
			case cmPaint:
				{
					PaintEvent * p_blk = static_cast<PaintEvent *>(TVINFOPTR);
					//CreateFont_();
					if(p_blk && oneof2(p_blk->PaintType, PaintEvent::tPaint, PaintEvent::tEraseBackground)) {
						SPaintToolBox * p_tb = APPL->GetUiToolBox();
						if(p_tb) {
							if(GetWbCapability() & wbcDrawBuffer) {
								// Если используется буферизованная отрисовка, то фон нужно перерисовать в любом случае а на событие PaintEvent::tEraseBackground
								// не реагировать
								if(p_blk->PaintType == PaintEvent::tPaint) {
									TCanvas2 canv(*p_tb, static_cast<HDC>(p_blk->H_DeviceContext));
									canv.Rect(p_blk->Rect, 0, TProgram::tbiListBkgBrush);
									DrawLayout(canv, P_Lfc);
								}
							}
							else {
								TCanvas2 canv(*p_tb, static_cast<HDC>(p_blk->H_DeviceContext));
								if(p_blk->PaintType == PaintEvent::tEraseBackground)
									canv.Rect(p_blk->Rect, 0, TProgram::tbiListBkgBrush);
								if(p_blk->PaintType == PaintEvent::tPaint)
									DrawLayout(canv, P_Lfc);
							}
						}
						clearEvent(event);
					}
				}
				break;
			case cmDown:
			case cmUp:
				{
					long   p = 0;
					long   i = 0;
					if(getCurItem(&p, &i)) {
						const int up = BIN(TVCMD == cmUp);
						if(moveItem(p, i, up) > 0)
							updateList(up ? p-1 : p+1);
					}
				}
				break;
			case cmLBDblClk:
				{
					SmartListBox * p_box = GetListBoxCtl();
					if(SmartListBox::IsValidS(p_box)) {
						int    edit = 1;
						bool   is_tree_list = false;
						PPID   cur_id = 0;
						long   p = 0;
						long   i = 0;
						p_box->P_Def->getCurID(&cur_id);
						if(p_box->IsTreeList()) {
							is_tree_list = true;
							if(static_cast<const StdTreeListBoxDef *>(p_box->P_Def)->HasChildren(cur_id))
								edit = 0;
						}
						if(event.isCtlEvent(p_box->GetId())) {
							if(cur_id && false/*Options & oOnDblClkOk*/) {
								TView::messageCommand(this, cmOK);
							}
							else if(edit && getCurItem(&p, &i) && editItem(p, i) > 0) {
								const long id = is_tree_list ? i : p;
								if(is_tree_list)
									updateListById(id);
								else
									updateList(id);
							}
							else
								return;
						}
						else
							return;
					}
					else
						return;
				}
				break;
			case cmaInsert:
				{
					SmartListBox * p_box = GetListBoxCtl();
					if(p_box) {
						long   p = 0;
						long   i = 0;
						int    r = addItem(&p, &i);
						if(r == 2)
							updateListById(i);
						else if(r > 0)
							updateList(p);
					}
				}
				break;
			case cmaDelete:
				{
					long   p = 0;
					long   i = 0;
					if(getCurItem(&p, &i) && delItem(p, i) > 0) {
						updateList(-1);
					}
				}
				break;
			case cmaEdit:
				{
					long   p = 0;
					long   i = 0;
					if(getCurItem(&p, &i) && editItem(p, i) > 0) {
						SmartListBox * p_box = GetListBoxCtl();
						assert(p_box); // Если getCurItem() вернул !0 то p_box != 0
						const bool is_tree_list = p_box->IsTreeList();
						const long id = is_tree_list ? i : p;
						if(is_tree_list)
							updateListById(id);
						else
							updateList(id);
					}
				}
				break;
		}
	}
	else if(event.isKeyDown(kbEsc)) {
		if(IsInState(sfModal)) {
			EndModalCmd = cmCancel;
			clearEvent(event);
		}
	}
}

#if 0 // @v12.3.6 {
static ListBoxDef * Test_LayoutedListDialog_MakeTestData(bool multiColumn, bool treeView, SString & rListColumnsDefinition)
{
	rListColumnsDefinition.Z();
	ListBoxDef * p_result = 0;
	if(treeView) {
		PPObjGoodsGroup gg_obj;
		StrAssocArray * p_list = gg_obj.MakeStrAssocList(0);
		if(p_list) {
			p_result = new StdTreeListBoxDef(p_list, lbtDblClkNotify|lbtFocNotify|lbtSelNotify|lbtDisposeData, MKSTYPE(S_ZSTRING, 128));
		}
	}
	else {
		//static const char * P_TestDataFile = "D:/Papyrus/Src/PPTEST/DATA/person.txt";
		SString file_name;
		PPGetPath(PPPATH_TESTROOT, file_name);
		file_name.SetLastSlash().Cat("data").SetLastSlash().Cat("person.txt");
		SFile f_in(file_name, SFile::mRead);
		if(f_in.IsValid()) {
			StrAssocArray * p_data = new StrAssocArray;
			p_result = new StrAssocListBoxDef(p_data, lbtDisposeData|lbtDblClkNotify|lbtFocNotify);
			//
			uint   line_no = 0;
			SString temp_buf;
			SString line_buf;
			StringSet ss(SLBColumnDelim);
			while(f_in.ReadLine(line_buf, SFile::rlfChomp|SFile::rlfStrip)) {
				line_no++;
				if(line_no >= 8) { // В начале файла там какой-то мусор, который я поосторожничал убирать
					ss.Z();
					line_buf.Transf(CTRANSF_OUTER_TO_INNER);
					line_buf.Tokenize("|", ss);
					if(multiColumn) {
						p_result->addItem(line_no, ss.getBuf());
					}
					else {
						if(ss.get(0U, temp_buf)) {
							p_result->addItem(line_no, temp_buf);
						}
					}
				}
			}
			if(multiColumn)
				rListColumnsDefinition = "20,L,name;20,L,addr;10,L,zip";
		}
	}
	return p_result;
}

int Test_LayoutedListDialog()
{
	#define LIST_SELECTION_DIALOG_BASE_CLASS LayoutedListDialog

	class __TestLayoutedListDialog : public LIST_SELECTION_DIALOG_BASE_CLASS {
	public:
		__TestLayoutedListDialog(const LayoutedListDialog_Base::Param & rParam, ListBoxDef * pDef) : LIST_SELECTION_DIALOG_BASE_CLASS(rParam, pDef)
		{
		}
		~__TestLayoutedListDialog()
		{
		}
	private:
		DECL_HANDLE_EVENT
		{
			LIST_SELECTION_DIALOG_BASE_CLASS::handleEvent(event);
		}
	};
	
	int    ok = 0;
	// @debug {
	{
		static bool single = false;
		if(!single) {
			SLS.SaturateRvlStrPool(512);
			SLS.SaturateRvlStrUPool(512);
			single = true;
		}
	}
	// } @debug 
	{
		LayoutedListDialog_Base::Param param;
		param.Symb = "Test_LayoutedListDialog";
		param.Bounds.Set(0, 0, 295, 300); 
		ListBoxDef * p_lb_def = Test_LayoutedListDialog_MakeTestData(false/*multiColumn*/, true/*treeView*/, param.ColumnDescription);
		if(p_lb_def) {
			param.Flags |= (LayoutedListDialog_Base::fcedCreate|LayoutedListDialog_Base::fcedEdit|LayoutedListDialog_Base::fcedDelete|LayoutedListDialog_Base::fOkCancel);
			param.Flags |= LayoutedListDialog_Base::fHeaderStaticText;
			param.Title = "Test list selection dialog";
			__TestLayoutedListDialog * p_view = new __TestLayoutedListDialog(param, p_lb_def);
			if(p_view) {
				ok = LayoutedListDialog_Base::Exec(p_view);
			}
		}
	}
	return ok;
}
// } @construction
#endif // } @v12.3.6
#endif // } 0 @v12.3.6 (Этот блок отработал свою функцию - переходим на регулярное описание диалогов в DL00) {

/*static*/int STDCALL ListBoxSelDialog::Run(StrAssocArray * pAry, const char * pTitle, PPID * pID/*, uint flags*/)
{
	int    ok = -1;
	ListBoxSelDialog * p_dlg = 0;
	ListBoxDef * p_def = pAry ? new StrAssocListBoxDef(pAry, lbtDblClkNotify|lbtFocNotify) : 0;
	if(p_def && CheckDialogPtrErr(&(p_dlg = new ListBoxSelDialog(p_def, 0, 0)))) {
		{
			SString title_buf;
			if(pTitle && pTitle[0] == '@') {
				if(PPLoadString(pTitle+1, title_buf) <= 0)
					title_buf = pTitle;
			}
			else
				title_buf = pTitle;
			p_dlg->setTitle(title_buf);
		}
		PPID   id = DEREFPTRORZ(pID);
		p_dlg->setDTS(id);
		if(ExecView(p_dlg) == cmOK) {
			p_dlg->getDTS(&id);
			ASSIGN_PTR(pID, id);
			ok = 1;
		}
		else
			ok = 0;
	}
	else
		ok = 0;
	delete p_dlg;
	return ok;
}
//
// PPTXT_BRWCOLORSSCHEMASNAMES - schemas ids and names
// BrwColorsSchemas defined in app.h

class UICfgDialog : public TDialog {
	DECL_DIALOG_DATA(UserInterfaceSettings); // UICfg;
public:
	UICfgDialog() : TDialog(DLG_UICFG)
	{
		SetupStringCombo(this, CTLSEL_UICFG_SELBCLRSHM, PPTXT_BRWCOLORSSCHEMASNAMES, 1);
		SetupStringCombo(this, CTLSEL_UICFG_WNDVIEWKIND, PPTXT_WINDOWSVIEWKINDS, 1);
	}
	DECL_DIALOG_SETDTS()
	{
		long   cmbb_pos = 1;
		SString temp_buf;
		RVALUEPTR(Data, pData);
		cmbb_pos = (Data.TableViewStyle >= 0 && Data.TableViewStyle < NUMBRWCOLORSCHEMA) ? (Data.TableViewStyle + 1) : 1;
		static_cast<ComboBox *>(getCtrlView(CTLSEL_UICFG_SELBCLRSHM))->TransmitData(+1, &cmbb_pos);
		cmbb_pos = (Data.WindowViewStyle >= 0) ? (Data.WindowViewStyle + 1) : 1;
		static_cast<ComboBox *>(getCtrlView(CTLSEL_UICFG_WNDVIEWKIND))->TransmitData(+1, &cmbb_pos);
		setCtrlData(CTL_UICFG_LISTELEMCOUNT, &Data.ListElemCount);
		AddClusterAssoc(CTL_UICFG_FLAGS,  0, UserInterfaceSettings::fDontExitBrowserByEsc);
		AddClusterAssoc(CTL_UICFG_FLAGS,  1, UserInterfaceSettings::fShowShortcuts);
		AddClusterAssoc(CTL_UICFG_FLAGS,  2, UserInterfaceSettings::fAddToBasketItemCurBrwItemAsQtty);
		AddClusterAssoc(CTL_UICFG_FLAGS,  3, UserInterfaceSettings::fShowBizScoreOnDesktop);
		AddClusterAssoc(CTL_UICFG_FLAGS,  4, UserInterfaceSettings::fDisableNotFoundWindow);
		AddClusterAssoc(CTL_UICFG_FLAGS,  5, UserInterfaceSettings::fUpdateReminder);
		AddClusterAssoc(CTL_UICFG_FLAGS,  6, UserInterfaceSettings::fTcbInterlaced);
		AddClusterAssoc(CTL_UICFG_FLAGS,  7, UserInterfaceSettings::fDisableBeep);
		AddClusterAssoc(CTL_UICFG_FLAGS,  8, UserInterfaceSettings::fBasketItemFocusPckg);
		AddClusterAssoc(CTL_UICFG_FLAGS,  9, UserInterfaceSettings::fOldModifSignSelection);
		AddClusterAssoc(CTL_UICFG_FLAGS, 10, UserInterfaceSettings::fExtGoodsSelMainName);
		AddClusterAssoc(CTL_UICFG_FLAGS, 11, UserInterfaceSettings::fExtGoodsSelHideGenerics);
		AddClusterAssoc(CTL_UICFG_FLAGS, 12, UserInterfaceSettings::fPollVoipService);
		AddClusterAssoc(CTL_UICFG_FLAGS, 13, UserInterfaceSettings::fStringHistoryDisabled);
		AddClusterAssoc(CTL_UICFG_FLAGS, 14, UserInterfaceSettings::fDateTimePickerBefore1124); // @v11.2.6
		INVERSEFLAG(Data.Flags, UserInterfaceSettings::fDontExitBrowserByEsc);
		SetClusterData(CTL_UICFG_FLAGS, Data.Flags);
		{
			const  long t = CHKXORFLAGS(Data.Flags, UserInterfaceSettings::fEnalbeBillMultiPrint, UserInterfaceSettings::fDisableBillMultiPrint);
			AddClusterAssocDef(CTL_UICFG_MULTBILLPRINT, 0, 0);
			AddClusterAssoc(CTL_UICFG_MULTBILLPRINT, 1, UserInterfaceSettings::fEnalbeBillMultiPrint);
			AddClusterAssoc(CTL_UICFG_MULTBILLPRINT, 2, UserInterfaceSettings::fDisableBillMultiPrint);
			SetClusterData(CTL_UICFG_MULTBILLPRINT, t);
		}
		{
			Data.TableFont.ToStr(temp_buf.Z(), 0);
			setStaticText(CTL_UICFG_ST_TABLEFONT, temp_buf);
		}
		{
			Data.ListFont.ToStr(temp_buf.Z(), 0);
			setStaticText(CTL_UICFG_ST_LISTFONT, temp_buf);
		}
		setCtrlString(CTL_UICFG_SPCINPDRV, Data.SpecialInputDeviceSymb);
		return 1;
	}
	DECL_DIALOG_GETDTS()
	{
		long   id = 0;
		getCtrlData(CTLSEL_UICFG_SELBCLRSHM, &id);
		id--;
		Data.TableViewStyle = (id >= 0 && id < NUMBRWCOLORSCHEMA) ? id : 0;
		getCtrlData(CTLSEL_UICFG_WNDVIEWKIND, &(id = 0));
		Data.WindowViewStyle = (id > 0) ? (id - 1) : 0;
		getCtrlData(CTL_UICFG_LISTELEMCOUNT, &Data.ListElemCount);
		GetClusterData(CTL_UICFG_FLAGS, &Data.Flags);
		INVERSEFLAG(Data.Flags, UserInterfaceSettings::fDontExitBrowserByEsc);
		{
			const long t = GetClusterData(CTL_UICFG_MULTBILLPRINT);
			Data.Flags &= ~(UserInterfaceSettings::fEnalbeBillMultiPrint | UserInterfaceSettings::fDisableBillMultiPrint);
			if(t & UserInterfaceSettings::fEnalbeBillMultiPrint)
				Data.Flags |= UserInterfaceSettings::fEnalbeBillMultiPrint;
			else if(t & UserInterfaceSettings::fDisableBillMultiPrint)
				Data.Flags |= UserInterfaceSettings::fDisableBillMultiPrint;
		}
		getCtrlString(CTL_UICFG_SPCINPDRV, Data.SpecialInputDeviceSymb);
		ASSIGN_PTR(pData, Data);
		return 1;
	}
private:
	DECL_HANDLE_EVENT
	{
		TDialog::handleEvent(event);
		if(event.isCmd(cmSelBrowserFont))
			SelectFont(Data.TableFont, CTL_UICFG_ST_TABLEFONT);
		else if(event.isCmd(cmSelListFont))
			SelectFont(Data.ListFont, CTL_UICFG_ST_LISTFONT);
		else if(event.isCmd(cmUiCfgBITF)) 
			EditBillItemBrowserOptions();
		else
			return;
		clearEvent(event);
	}
	void   EditBillItemBrowserOptions()
	{
		class BillItemBrowserOptions : public TDialog {
		public:
			BillItemBrowserOptions() : TDialog(DLG_UICFG_BITF)
			{
			}
		private:
			DECL_HANDLE_EVENT
			{
				TDialog::handleEvent(event);
				if(event.isClusterClk(CTL_UICFG_BITF_FLAGS)) {
					
				}
			}
		};
		BillItemBrowserOptions * dlg = new BillItemBrowserOptions();
		if(CheckDialogPtr(&dlg)) {
			dlg->AddClusterAssoc(CTL_UICFG_BITF_FLAGS, 0, UserInterfaceSettings::bitfUseCommCfgForBarcodeSerialOptions);
			dlg->AddClusterAssoc(CTL_UICFG_BITF_FLAGS, 1, UserInterfaceSettings::bitfShowBarcode);
			dlg->AddClusterAssoc(CTL_UICFG_BITF_FLAGS, 2, UserInterfaceSettings::bitfShowSerial);
			dlg->AddClusterAssoc(CTL_UICFG_BITF_FLAGS, 3, UserInterfaceSettings::bitfShowMargin);
			dlg->SetClusterData(CTL_UICFG_BITF_FLAGS, Data.BillItemTableFlags);
			if(ExecView(dlg) == cmOK) {
				long   temp_value = 0;
				dlg->GetClusterData(CTL_UICFG_BITF_FLAGS, &temp_value);
				SETFLAGBYSAMPLE(Data.BillItemTableFlags, UserInterfaceSettings::bitfUseCommCfgForBarcodeSerialOptions, temp_value);
				if(!(Data.BillItemTableFlags & UserInterfaceSettings::bitfUseCommCfgForBarcodeSerialOptions)) {
					SETFLAGBYSAMPLE(Data.BillItemTableFlags, UserInterfaceSettings::bitfShowBarcode, temp_value);
					SETFLAGBYSAMPLE(Data.BillItemTableFlags, UserInterfaceSettings::bitfShowSerial, temp_value);
				}
				else {
					Data.BillItemTableFlags &= ~(UserInterfaceSettings::bitfShowBarcode|UserInterfaceSettings::bitfShowSerial);
				}
				SETFLAGBYSAMPLE(Data.BillItemTableFlags, UserInterfaceSettings::bitfShowMargin, temp_value);
			}
		}
		delete dlg;
	}
	void   SelectFont(SFontDescr & rFd, uint indCtlId);
};

void UICfgDialog::SelectFont(SFontDescr & rFd, uint indCtlId)
{
	CHOOSEFONT font;
	LOGFONT log_font;
	MEMSZERO(font);
	MEMSZERO(log_font);
	font.hwndOwner   = H();
	font.Flags       = CF_FORCEFONTEXIST|CF_SCREENFONTS|CF_INITTOLOGFONTSTRUCT|CF_NOVERTFONTS|CF_NOSCRIPTSEL;
	rFd.MakeLogFont(&log_font);
	font.lpLogFont   = &log_font;
	font.lStructSize = sizeof(font);
	if(ChooseFont(&font)) {
		rFd.SetLogFont(font.lpLogFont);
		if(indCtlId) {
			SString temp_buf;
			rFd.ToStr(temp_buf, 0);
			setStaticText(indCtlId, temp_buf);
		}
	}
	else if(CommDlgExtendedError() != 0)
		PPError(PPERR_DLGLOADFAULT);
}

int UISettingsDialog()
{
	int    r = 0;
	UICfgDialog	* p_dlg = new UICfgDialog();
	if(CheckDialogPtrErr(&p_dlg)) {
		uint   v = 0;
		UserInterfaceSettings uiset = APPL->GetUiSettings(); // @v11.2.6 
		// @v11.2.6 uiset.Restore();
		p_dlg->setDTS(&uiset);
		if(ExecView(p_dlg) == cmOK) {
			p_dlg->getDTS(&uiset);
			// @v11.2.6 uiset.Save();
			APPL->UpdateUiSettings(uiset); // @v11.2.6 
			r = 1;
		}
		else
			r = -1;
	}
	delete p_dlg;
	return r;
}

EmbedDialog::EmbedDialog(uint resID) : TDialog(resID), P_ChildDlg(0)
{
}

EmbedDialog::~EmbedDialog()
{
	delete P_ChildDlg;
}

int EmbedDialog::Embed(TDialog * pDlg)
{
	int    ok = -1;
	ZDELETE(P_ChildDlg);
	if(pDlg) {
		THROW(CheckDialogPtr(&pDlg));
		P_ChildDlg = pDlg;
		HWND  hwnd_child = P_ChildDlg->H();
		P_ChildDlg->destroyCtrl(STDCTL_OKBUTTON);
		P_ChildDlg->destroyCtrl(STDCTL_CANCELBUTTON);
		TView::SetWindowProp(hwnd_child, GWL_STYLE, WS_CHILD);
		TView::SetWindowProp(hwnd_child, GWL_EXSTYLE, 0L);
		::SetParent(hwnd_child, H());
		ok = 1;
	}
	CATCHZOK
	::SetFocus(H());
	return ok;
}

/*virtual*/void EmbedDialog::setChildPos(uint neighbourCtl)
{
	if(P_ChildDlg && P_ChildDlg->H()) {
		RECT   child_rect;
		RECT   ctl_rect;
		RECT   dlg_rect;
		const  long   parent_style = TView::SGetWindowStyle(H());
		GetWindowRect(GetDlgItem(H(), neighbourCtl), &ctl_rect);
		GetWindowRect(H(), &dlg_rect);
		child_rect.left   = ctl_rect.right - dlg_rect.left - GetSystemMetrics(SM_CXEDGE);
		child_rect.top    = ctl_rect.top - dlg_rect.top - GetSystemMetrics(SM_CYBORDER) -
			GetSystemMetrics(SM_CYEDGE) - (((parent_style & WS_CAPTION) == WS_CAPTION) ? GetSystemMetrics(SM_CYCAPTION) : 0);
		child_rect.bottom = ctl_rect.bottom - ctl_rect.top;
		child_rect.right  = dlg_rect.right - dlg_rect.left - GetSystemMetrics(SM_CXBORDER) * 2 - GetSystemMetrics(SM_CXEDGE) * 2 - child_rect.left;
		MoveWindow(P_ChildDlg->H(), child_rect.left, child_rect.top, child_rect.right, child_rect.bottom, 1);
		APPL->SetWindowViewByKind(P_ChildDlg->H(), TProgram::wndtypChildDialog);
		ShowWindow(P_ChildDlg->H(), SW_SHOWNORMAL);
	}
	::SetFocus(H());
}
//
//
//
SpecialInputCtrlGroup::SpecialInputCtrlGroup(uint ctlId, uint rdDelay) : CtrlGroup(), RdTimer(rdDelay), CtlId(ctlId), RdDelay(rdDelay), P_Ad(0)
{
	// @v11.2.6 UserInterfaceSettings uicfg;
	SString spc_input_device_symb = APPL->GetUiSettings().SpecialInputDeviceSymb;
	// @v11.2.6 if(uicfg.Restore() > 0 && uicfg.SpecialInputDeviceSymb.NotEmptyS()) {
	if(spc_input_device_symb.NotEmptyS()) {
		PPObjGenericDevice gd_obj;
		PPID   dvc_id = 0;
		if(gd_obj.SearchBySymb(spc_input_device_symb, &dvc_id) > 0) {
			SString temp_buf, init_buf;
			PPGenericDevicePacket gd_pack;
			THROW(gd_obj.GetPacket(dvc_id, &gd_pack) > 0);
			THROW_PP(gd_pack.Rec.DeviceClass == DVCCLS_READER, PPERR_ADDPEVDVCNOTREADER);
			gd_pack.GetExtStrData(GENDVCEXSTR_ENTRY, temp_buf);
			THROW_PP(temp_buf.NotEmptyS(), PPERR_UNDEFADVCDESCR);
			{
				THROW_MEM(P_Ad = new PPAbstractDevice(0));
				P_Ad->PCpb.Cls = gd_pack.Rec.DeviceClass;
				THROW(P_Ad->IdentifyDevice(gd_pack.Rec.DeviceClass, temp_buf));
				THROW(P_Ad->RunCmd("INIT", Out.Z()));
				gd_pack.GetExtStrData(GENDVCEXSTR_INITSTR, temp_buf.Z());
				if(temp_buf.NotEmptyS()) {
					THROW(P_Ad->RunCmd(temp_buf, Out.Z()));
				}
			}
		}
	}
	CATCH
		ZDELETE(P_Ad);
	ENDCATCH
	if(!P_Ad)
		RdTimer.Restart(0);
}

SpecialInputCtrlGroup::~SpecialInputCtrlGroup()
{
	if(P_Ad) {
		P_Ad->RunCmd("RELEASE", Out.Z());
		ZDELETE(P_Ad);
	}
}

void SpecialInputCtrlGroup::handleEvent(TDialog * pDlg, TEvent & event)
{
	if(P_Ad) {
		if(TVCMD == cmIdle) {
			TView * p_il = pDlg->getCtrlView(CtlId);
			if(TView::IsSubSign(p_il, TV_SUBSIGN_INPUTLINE)) {
				if(RdTimer.Check(0) && !p_il->IsInState(sfDisabled|sfReadOnly)) {
					P_Ad->RunCmd("LISTEN", Out.Z());
					SString temp_buf;
					if(Out.GetText(0, temp_buf) > 0 && temp_buf.NotEmptyS()) {
						temp_buf.Chomp();
						pDlg->setCtrlString(CtlId, temp_buf);
					}
				}
			}
			else {
				P_Ad->RunCmd("RELEASE", Out.Z());
				ZDELETE(P_Ad);
				RdTimer.Restart(0);
			}
		}
	}
}
//
//
//
LocationCtrlGroup::Rec::Rec(const ObjIdListFilt * pLocList, PPID parentID, PPID ownerID) : ParentID(parentID), OwnerID(ownerID)
{
	RVALUEPTR(LocList, pLocList);
}

void LocationCtrlGroup::Helper_Construct()
{
	// P_Eac = 0;
	CtlselLoc = 0;
	CtlCode = 0;
	CtlPhone = 0;
	CtlInfo = 0;
	CmEditLocList = 0;
	CmEditLoc = 0;
	Flags = 0;
	GoodsID = 0;
}

LocationCtrlGroup::LocationCtrlGroup(uint ctlselLoc, uint ctlCode, uint ctlPhone, uint cmEditLocList, uint cmEditLoc, long flags, const PPIDArray * pExtLocList)
{
	Helper_Construct();
	CtlselLoc = ctlselLoc;
	CtlCode = ctlCode;
	CtlPhone = ctlPhone;
	CmEditLocList = cmEditLocList;
	CmEditLoc = cmEditLoc;
	Flags = flags;
	RVALUEPTR(ExtLocList, pExtLocList);
}

void LocationCtrlGroup::SetInfoCtl(uint ctlId)
{
	CtlInfo = ctlId;
}

void LocationCtrlGroup::SetExtLocList(const PPIDArray * pExtLocList)
{
    if(!RVALUEPTR(ExtLocList, pExtLocList))
		ExtLocList.clear();
}

int LocationCtrlGroup::SetWarehouseCellMode(TDialog * pDlg, int enable)
{
	int    ok = 1;
	int    prev = BIN(Flags & fWarehouseCell);
	if(BIN(enable) != prev) {
		PPObjLocation loc_obj;
		SETFLAG(Flags, fWarehouseCell, enable);
		if(enable) {
			if(!(Flags & fEnableSelUpLevel)) {
				Data.LocList.Set(0);
			}
		}
		else {
			PPIDArray par_list;
			for(uint i = 0; i < Data.LocList.GetCount(); i++) {
				PPID par_id = 0;
				if(loc_obj.GetParentWarehouse(Data.LocList.Get().get(i), &par_id) > 0)
					par_list.addUnique(par_id);
			}
			Data.LocList.Set(par_list.getCount() ? &par_list : 0);
		}
		PPID   single_id = Data.LocList.GetSingle();
		LocationFilt filt(((Flags & fWarehouseCell) ? LOCTYP_WHZONE : LOCTYP_WAREHOUSE), 0, Data.ParentID);
		SetupPPObjCombo(pDlg, CtlselLoc, PPOBJ_LOCATION, single_id, ((Flags & fEnableSelUpLevel) ? OLW_CANSELUPLEVEL : 0), &filt);
	}
	else
		ok = -1;
	return ok;
}

int LocationCtrlGroup::SetCellSelectionByGoods(TDialog * pDlg, PPID goodsID)
{
	int    ok = -1;
	return ok;
}

int LocationCtrlGroup::EditLocList(TDialog * pDlg, uint ctlID, ObjIdListFilt * pLocList)
{
	int    ok = -1;
	PPIDArray ary;
	StrAssocArray * p_src_list = 0;
	THROW(pLocList && pDlg);
	if(pLocList->IsExists())
		ary = pLocList->Get();
	if(!ary.getCount())
		ary.setSingleNZ(pDlg->getCtrlLong(ctlID));
	{
		LocationFilt loc_filt((Flags & fDivision) ? LOCTYP_DIVISION : LOCTYP_WAREHOUSE);
		if(ExtLocList.getCount())
			loc_filt.ExtLocList.Set(&ExtLocList);
		THROW(p_src_list = LocObj.MakeList_(&loc_filt, 0));
		{
			ListToListData ltld(p_src_list, PPOBJ_LOCATION, &ary);
			ltld.Flags |= ListToListData::fIsTreeList;
			ltld.TitleStrID = 0; // PPTXT_XXX;
			if(ListToListDialog(&ltld) > 0) {
				pLocList->Set(ary.getCount() ? &ary : 0);
				if(pLocList->GetCount() > 1) {
					SetComboBoxListText(pDlg, ctlID);
					pDlg->disableCtrl(ctlID, true);
				}
				else {
					pDlg->setCtrlLong(ctlID, pLocList->GetSingle());
					pDlg->disableCtrl(ctlID, false);
				}
				ok = 1;
			}
		}
	}
	CATCHZOK
	delete p_src_list;
	return ok;
}

void LocationCtrlGroup::handleEvent(TDialog * pDlg, TEvent & event)
{
	if(CmEditLocList && event.isCmd(CmEditLocList)) {
		EditLocList(pDlg, CtlselLoc, &Data.LocList);
		pDlg->clearEvent(event);
	}
	else if(CtlPhone && event.isCmd(cmWSSelected) && event.isCtlEvent(CtlPhone)) {
		PPID   ea_id = 0;
		EAddrTbl::Rec ea_rec;
		pDlg->getCtrlData(CtlPhone, &ea_id);
		if(ea_id && LocObj.P_Tbl->GetEAddr(ea_id, &ea_rec) > 0) {
			if(ea_rec.LinkObjType == PPOBJ_LOCATION && ea_rec.LinkObjID)
				SetupInfo(pDlg, ea_rec.LinkObjID);
		}
	}
	else if(event.isCmd(cmInputUpdated)) {
		if(CtlCode && event.isCtlEvent(CtlCode)) {
			PPObjLocation loc_obj;
			SString loc_name;
			pDlg->getCtrlString(CtlCode, loc_name);
			PPID   id = 0;
			if(loc_obj.SearchName((Flags & fWarehouseCell) ? LOCTYP_WHCELL : LOCTYP_WAREHOUSE, Data.ParentID, loc_name, &id) > 0)
				pDlg->setCtrlLong(CtlselLoc, id);
			pDlg->clearEvent(event);
		}
	}
	else if(CmEditLoc && event.isCmd(CmEditLoc)) {
		getData(pDlg, 0);
		PPID single_id = Data.LocList.GetSingle();
		if(single_id)
			LocObj.Edit(&single_id, 0);
	}
}

void LocationCtrlGroup::SetupInfo(TDialog * pDlg, PPID locID)
{
	if(pDlg && CtlInfo) {
        SString info_buf;
		if(locID) {
			SString temp_buf;
			LocationTbl::Rec loc_rec;
			if(LocObj.Fetch(locID, &loc_rec) > 0) {
				if(loc_rec.Name[0])
					info_buf.Cat(loc_rec.Name);
				if(loc_rec.Code[0])
					info_buf.CatDivIfNotEmpty('-', 1).Cat(loc_rec.Code);
				LocationCore::GetExField(&loc_rec, LOCEXSTR_CONTACT, temp_buf);
				if(temp_buf.NotEmptyS())
					info_buf.CatDivIfNotEmpty('-', 1).Cat(temp_buf);
				LocationCore::GetExField(&loc_rec, LOCEXSTR_FULLADDR, temp_buf);
				if(temp_buf.NotEmptyS())
					info_buf.CatDivIfNotEmpty('-', 1).Cat(temp_buf);
				else {
					LocationCore::GetExField(&loc_rec, LOCEXSTR_SHORTADDR, temp_buf);
					if(temp_buf.NotEmptyS())
						info_buf.CatDivIfNotEmpty('-', 1).Cat(temp_buf);
				}
			}
		}
		pDlg->setCtrlString(CtlInfo, info_buf);
	}
}

int LocationCtrlGroup::setData(TDialog * pDlg, void * pData)
{
	Data = *static_cast<Rec *>(pData);

	SString temp_buf;
	PPID   single_id = Data.LocList.GetSingle();
	PPID   loc_type = (Flags & fWarehouseCell) ? LOCTYP_WHZONE : ((Flags & fDivision) ? LOCTYP_DIVISION : LOCTYP_WAREHOUSE);
	LocationFilt filt(loc_type, Data.OwnerID, Data.ParentID);
	if(ExtLocList.getCount())
		filt.ExtLocList.Set(&ExtLocList);
	SetupPPObjCombo(pDlg, CtlselLoc, PPOBJ_LOCATION, single_id, ((Flags & fEnableSelUpLevel) ? OLW_CANSELUPLEVEL : 0), &filt);
	if(CtlCode) {
		LocationTbl::Rec loc_rec;
		if(single_id && LocObj.Fetch(single_id, &loc_rec) > 0)
			temp_buf = loc_rec.Code;
		else
			temp_buf.Z();
		pDlg->setCtrlString(CtlCode, temp_buf);
	}
	if(Data.LocList.GetCount() > 1) {
		SetComboBoxListText(pDlg, CtlselLoc);
		pDlg->disableCtrl(CtlselLoc, true);
	}
	else {
		pDlg->setCtrlLong(CtlselLoc, single_id);
		pDlg->disableCtrl(CtlselLoc, false);
	}
	if(CtlPhone) {
		long   pse_flags = (Flags & fStandaloneByPhone) ? PhoneSelExtra::lfStandaloneLocOnly : PhoneSelExtra::lfLocation;
		PPID   ea_id = 0;
		if(single_id) {
			PPIDArray ea_list;
			if(LocObj.P_Tbl->SearchEAddrByLink(PPOBJ_LOCATION, single_id, ea_list) > 0 && ea_list.getCount())
				ea_id = ea_list.get(0);
		}
		pDlg->SetupWordSelector(CtlPhone, new PhoneSelExtra(pse_flags), ea_id, 5, 0); // Выбор адреса без комбобокса
	}
	SetupInfo(pDlg, single_id);
	return 1;
}

int LocationCtrlGroup::getData(TDialog * pDlg, void * pData)
{
	Rec * p_rec = static_cast<Rec *>(pData);
	if(Data.LocList.GetCount() <= 1) {
		PPID   temp_id = pDlg->getCtrlLong(CtlselLoc);
		PPObjLocation loc_obj;
		//Data.LocList.FreeAll();
		Data.LocList.Set(0);
        if(temp_id)
			Data.LocList.Add(temp_id);
		else if(CtlCode) {
			SString code_buf;
			pDlg->getCtrlString(CtlCode, code_buf);
			if(code_buf.NotEmptyS()) {
				PPID   loc_type = (Flags & fWarehouseCell) ? LOCTYP_WHCELL : LOCTYP_WAREHOUSE;
				if(loc_obj.P_Tbl->SearchCode(loc_type, code_buf, &temp_id, 0) > 0)
					Data.LocList.Add(temp_id);
			}
		}
		if(!Data.LocList.GetCount() && CtlPhone) {
			PPID   ea_id = 0;
			EAddrTbl::Rec ea_rec;
			pDlg->getCtrlData(CtlPhone, &ea_id);
			if(ea_id && loc_obj.P_Tbl->GetEAddr(ea_id, &ea_rec) > 0) {
                if(ea_rec.LinkObjType == PPOBJ_LOCATION && ea_rec.LinkObjID) {
					Data.LocList.Add(ea_rec.LinkObjID);
                }
			}
		}
	}
	ASSIGN_PTR(p_rec, Data);
	return 1;
}
//
//
//
/*
class TextHistorySelExtra : public WordSel_ExtraBlock {
public:
	explicit TextHistorySelExtra(const char * pKey);
	virtual StrAssocArray * GetList(const char * pText);
	virtual int Search(long id, SString & rBuf);
	virtual int SearchText(const char * pText, long * pID, SString & rBuf);
private:
	const SString Key;
};
*/
TextHistorySelExtra::TextHistorySelExtra(const char * pKey) : WordSel_ExtraBlock(0, 0, 0, 0, 2, WordSel_ExtraBlock::fFreeText), Key(pKey)
{
	SetTextMode(true);
}

/*virtual*/StrAssocArray * TextHistorySelExtra::GetRecentList()
{
	StrAssocArray * p_result = 0;
	if(Key.NotEmpty()) {
		SString temp_buf;
		StringSet ss;
		if(DS.GetStringHistoryRecent(Key, 10, ss)) {
			p_result = new StrAssocArray;
			if(p_result) {
				long  surrogate_id = 0;
				for(uint ssp = 0; ss.get(&ssp, temp_buf);) {
					temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
					p_result->Add(++surrogate_id, temp_buf, 0);
				}
			}
		}
	}
	return p_result;
}

/*virtual*/StrAssocArray * TextHistorySelExtra::GetList(const char * pText)
{
	StrAssocArray * p_result = 0;
	if(Key.NotEmpty()) {
		SString temp_buf;
		(temp_buf = pText).Transf(CTRANSF_INNER_TO_UTF8);
		StringSet ss;
		if(DS.GetStringHistory(Key, temp_buf, PPConfigDatabase::StringHistoryPool::sefSubString, ss)) {
			p_result = new StrAssocArray;
			if(p_result) {
				long  surrogate_id = 0;
				for(uint ssp = 0; ss.get(&ssp, temp_buf);) {
					temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
					p_result->Add(++surrogate_id, temp_buf, 0);
				}
			}
		}
	}
	return p_result;
}

/*virtual*/int TextHistorySelExtra::Search(long id, SString & rBuf)
{
	return 0;
}

/*virtual*/int TextHistorySelExtra::SearchText(const char * pText, long * pID, SString & rBuf)
{
	return 0;
}

/*virtual*/void TextHistorySelExtra::OnAcceptInput(const char * pText, long id)
{
	if(Key.NotEmpty()) {
		SString temp_buf;
		temp_buf = pText;
		if(temp_buf.NotEmptyS()) {
			temp_buf.Transf(CTRANSF_INNER_TO_UTF8);
			DS.AddStringHistory(Key, temp_buf);
			DS.SaveStringHistory();
		}
	}
}
//
//
//
SCardSelExtra::SCardSelExtra(PPID serId) : WordSel_ExtraBlock(0, 0, 0, 0, 2), SerID(serId)
{
	SetTextMode(false);
}

StrAssocArray * SCardSelExtra::GetList(const char * pText)
{
	SString pattern;
	StrAssocArray * p_list = 0;
	pattern = pText;
	if(pattern.Len()) {
		int    srch_substr = BIN(pattern.C(0) == '*');
		size_t len = pattern.ShiftLeftChr('*').Len();
		if(len >= MinSymbCount) {
			p_list = new StrAssocArray;
			ScObj.GetListBySubstring(pattern, SerID, p_list, srch_substr ? 0 : 1);
		}
	}
	if(p_list && p_list->getCount() == 0)
		ZDELETE(p_list);
	return p_list;
}

int SCardSelExtra::SearchText(const char * pText, long * pID, SString & rBuf)
{
	int    ok = 0;
	SCardTbl::Rec sc_rec;
	if(ScObj.SearchCode(SerID, pText, &sc_rec) > 0) {
		ASSIGN_PTR(pID, sc_rec.ID);
		rBuf = sc_rec.Code;
		ok = 1;
	}
	return ok;
}

int SCardSelExtra::Search(long id, SString & rBuf)
{
	int    ok = 1;
	SCardTbl::Rec rec;
	if(ScObj.Fetch(id, &rec) > 0)
		rBuf = rec.Code;
	else
		ok = 0;
	return ok;
}
//
//
//
ObjTagSelExtra::ObjTagSelExtra(PPID objType, PPID tagID) : WordSel_ExtraBlock(0, 0, 0, 0, 3), ObjType(objType), TagID(tagID), LocalFlags(0), LocID(0)
{
	CtrlTextMode = true;
	if(objType == PPOBJ_LOT && tagID == PPTAG_LOT_SN) {
	}
	else {
		PPRef->Ot.GetObjTextList(objType, tagID, TextBlock);
	}
}

void ObjTagSelExtra::SetupLotSerialParam(PPID locID, long flags)
{
	LocID = locID;
	Flags = flags;
}

StrAssocArray * ObjTagSelExtra::GetList(const char * pText)
{
	const bool by_lot_serial = (ObjType == PPOBJ_LOT && TagID == PPTAG_LOT_SN);
	PPObjBill * p_bobj = BillObj;
	SString temp_buf, obj_name_buf;
	StrAssocArray * p_list = 0;
	SString pattern(pText);
	if(pattern.Len()) {
		const bool srch_substr = (pattern.C(0) == '*');
		size_t len = pattern.ShiftLeftChr('*').Len();
		if(len >= MinSymbCount) {
			const StrAssocArray * p_full_list = by_lot_serial ? p_bobj->GetFullSerialList() : &TextBlock;
			if(p_full_list) {
				p_list = new StrAssocArray;
				if(p_list) {
					const uint c = p_full_list->getCount();
					for(uint i = 0; i < c; i++) {
						StrAssocArray::Item item = p_full_list->at_WithoutParent(i);
						int    r = ExtStrSrch(item.Txt, pattern, 0);
						if(r > 0) {
							int    do_add = 0;
							if(by_lot_serial) {
								ReceiptTbl::Rec lot_rec;
								if(p_bobj->trfr->Rcpt.Search(item.Id, &lot_rec) > 0) {
									if((!LocID || lot_rec.LocID == LocID) && (!(LocalFlags & lfOpenedSerialsOnly) || lot_rec.Closed == 0)) {
										ReceiptCore::MakeCodeString(&lot_rec, 0, obj_name_buf);
										do_add = 1;
									}
								}
							}
							else {
								GetObjectName(ObjType, item.Id, obj_name_buf);
								do_add = 1;
							}
							if(do_add) {
								temp_buf.Z().Cat(item.Txt);
								if(obj_name_buf.NotEmptyS())
									temp_buf.CatDiv('-', 1).Cat(obj_name_buf);
								p_list->Add(item.Id, temp_buf);
							}
						}
					}
				}
				if(by_lot_serial)
					p_bobj->ReleaseFullSerialList(p_full_list);
			}
		}
	}
	if(p_list && !p_list->getCount())
		ZDELETE(p_list);
	return p_list;
}

int ObjTagSelExtra::SearchText(const char * pText, long * pID, SString & rBuf)
{
	int    ok = 0;
	PPIDArray obj_id_list;
	PPRef->Ot.SearchObjectsByStrExactly(ObjType, TagID, pText, &obj_id_list);
	if(obj_id_list.getCount()) {
		for(uint i = 0; !ok && i < obj_id_list.getCount(); i++) {
			const  PPID obj_id = obj_id_list.get(i);
			if(GetObjectName(ObjType, obj_id, rBuf) > 0) {
				ASSIGN_PTR(pID, obj_id);
				ok = 1;
			}
		}
	}
	return ok;
}

int ObjTagSelExtra::Search(long id, SString & rBuf)
{
	rBuf.Z();
	int    ok = 0;
	ObjTagItem tag_item;
	if(TagObj.FetchTag(id, TagID, &tag_item) > 0) {
        tag_item.GetStr(rBuf);
        if(rBuf.NotEmpty())
			ok = 1;
	}
	return ok;
}
//
//
//
#define MIN_PHONE_LEN 5

PersonSelExtra::PersonSelExtra(PPID accSheetID, PPID personKindID) : WordSel_ExtraBlock(0, 0, 0, 0, 2),
	AccSheetID(accSheetID), PersonKindID(personKindID), SrchRegTypeID(0)
{
	{
		PPPersonRelTypePacket pack;
		PPObjPersonRelType obj_relt;
		if(obj_relt.GetPacket(PPPSNRELTYP_AFFIL, &pack) > 0)
			InhRegTypeList = pack.InhRegTypeList;
		InhRegTypeList.sort();
	}
	if(AccSheetID > 0 && PersonKindID == 0) {
		PPAccSheet acct_rec;
		PPObjAccSheet acct_obj;
		if(acct_obj.Fetch(AccSheetID, &acct_rec) > 0)
			SrchRegTypeID = acct_rec.CodeRegTypeID;
	}
	if(PersonKindID > 0) {
		PPPersonKind psnk_rec;
		PPObjPersonKind psnk_obj;
		if(psnk_obj.Fetch(PersonKindID, &psnk_rec) > 0)
			SrchRegTypeID = psnk_rec.CodeRegTypeID;
	}
}

StrAssocArray * PersonSelExtra::GetList(const char * pText)
{
	StrAssocArray * p_list = 0;
	SString pattern(pText);
	if(pattern.Len()) {
		//
		// сначала обычный поиск по имени (поиск неточный)
		//
		int   search_yourself = 1;
		if(P_OutDlg) {
			TView * p_view = P_OutDlg->getCtrlView(OutCtlId);
			if(p_view && p_view->GetSubSign() == TV_SUBSIGN_LISTBOX) {
				p_list = WordSel_ExtraBlock::GetList(pText);
				search_yourself = 0;
			}
		}
		if(search_yourself && pattern.ShiftLeftChr('*').Len() >= MinSymbCount) {
			assert(p_list == 0);
			p_list = new StrAssocArray;
			PsnObj.GetListBySubstring(pText, PersonKindID, p_list, 0/*fromBegStr*/);
		}
		//
		// Если не найдено ни одной персоналии/статьи, тогда ищем по номеру регистра (поиск точный)
		//
		if((!p_list || !p_list->getCount()) && pattern.ShiftLeftChr('*').Len() >= MinSymbCount) {
			SETIFZ(p_list, new StrAssocArray());
			PPIDArray psn_list;
			StrAssocArray phone_list;
			PPIDArray reg_list;
			// сначала поиск по поисковому регистру
			RegisterFilt reg_flt;
			reg_flt.Oid.Obj = PPOBJ_PERSON;
			reg_flt.RegTypeID  = SrchRegTypeID;
			reg_flt.NmbPattern = pattern;
			PsnObj.RegObj.SearchByFilt(&reg_flt, &reg_list, &psn_list);
			//
			// Если по поисковому регистру не нашли, то ищем по всем регистрам
			//
			if(SrchRegTypeID > 0 && psn_list.getCount() == 0) {
				reg_flt.RegTypeID  = 0;
				PsnObj.RegObj.SearchByFilt(&reg_flt, &reg_list, &psn_list);
			}
			if(!psn_list.getCount()) {
				//
				// Если не найдено по номеру регистра, тогда ищем по номеру телефона (поиск неточный)
				//
				SString phone_buf;
				LongArray temp_phone_list;
				pattern.Transf(CTRANSF_INNER_TO_UTF8).Utf8ToLower();
				PPEAddr::Phone::NormalizeStr(pattern, 0, phone_buf);
				LocationCore * p_locc = PsnObj.LocObj.P_Tbl;
				if(phone_buf.Len() >= MIN_PHONE_LEN && p_locc->SearchEAddrMaxLikePhone(phone_buf, 0, temp_phone_list) > 0) {
					for(uint i = 0; i < temp_phone_list.getCount(); i++) {
						EAddrTbl::Rec ea_rec;
						if(p_locc->GetEAddr(temp_phone_list.get(i), &ea_rec) > 0) {
							if(ea_rec.LinkObjType == PPOBJ_PERSON && ea_rec.LinkObjID > 0) {
								psn_list.add(ea_rec.LinkObjID);
								reinterpret_cast<const PPEAddr *>(ea_rec.Addr)->GetPhone(phone_buf.Z());
								phone_list.Add(ea_rec.LinkObjID, phone_buf.cptr());
							}
						}
					}
				}
			}
			else {
				//
				// Персоналии по номеру регистру найдены. Теперь добавим персоналии, которые являются филиалами по отношению к найденым и наследуют регистры
				//
				PPIDArray temp_list = psn_list;
				PPIDArray psn_list2;
				{
					reg_list.sortAndUndup();
					RegisterTbl::Rec reg_rec;
					for(uint i = 0; i < reg_list.getCount(); i++) {
						const  PPID reg_id = reg_list.get(i);
						if(PsnObj.RegObj.Fetch(reg_id, &reg_rec) > 0 && reg_rec.ObjType == PPOBJ_PERSON && InhRegTypeList.bsearch(reg_rec.RegTypeID, 0)) {
							psn_list2.clear();
							if(PsnObj.GetRelPersonList(psn_list.at(i), PPPSNRELTYP_AFFIL, 1, &psn_list2) > 0)
								temp_list.add(&psn_list2);
						}
					}
					psn_list = temp_list;
					psn_list.sortAndUndup();
				}
			}
			{
				const  int  use_phone_list = BIN(phone_list.getCount());
				const  uint _c = psn_list.getCount();
				SString name, temp_name;
				for(uint i = 0; i < _c; i++) {
					PPID   id = 0;
					const  PPID psn_id = psn_list.at(i);
					if(AccSheetID)
						ArObj.GetByPerson(AccSheetID, psn_id, &id);
					else
						id = psn_id;
					if(id > 0) {
						GetObjectName((AccSheetID) ? PPOBJ_ARTICLE : PPOBJ_PERSON, id, name.Z());
						if(use_phone_list) {
							(temp_name = phone_list.Get(i).Txt).Space().Cat(name);
							name = temp_name;
						}
						p_list->Add(id, 0, name);
					}
				}
			}
		}
		if(p_list && p_list->getCount() == 0)
			ZDELETE(p_list);
	}
	return p_list;
}

int PersonSelExtra::SearchText(const char * pText, long * pID, SString & rBuf)
{
	return WordSel_ExtraBlock::SearchText(pText, pID, rBuf);
}

int PersonSelExtra::Search(long id, SString & rBuf)
{
	int    ok = 1;
	if(!AccSheetID) {
		PersonTbl::Rec  psn_rec;
		if(PsnObj.Search(id, &psn_rec) > 0)
			rBuf = psn_rec.Name;
		else
			ok = 0;
	}
	else {
		ArticleTbl::Rec ar_rec;
		if(ArObj.Search(id, &ar_rec) > 0)
			rBuf = ar_rec.Name;
		else
			ok = 0;
	}
	return ok;
}
//
//
//
PhoneSelExtra::PhoneSelExtra(long localFlags) : WordSel_ExtraBlock(0, 0, 0, 0, 4), LocalFlags(localFlags)
{
	SetTextMode(false);
}

StrAssocArray * PhoneSelExtra::GetList(const char * pText)
{
	StrAssocArray * p_list = 0;
	SString pattern(pText);
	if(pattern.NotEmptyS()) {
		SString phone_buf;
		const int srch_substr = BIN(pattern.C(0) == '*');
		pattern.ShiftLeftChr('*');
		pattern.Transf(CTRANSF_INNER_TO_UTF8).Utf8ToLower();
		PPEAddr::Phone::NormalizeStr(pattern, 0, phone_buf);
		pattern = phone_buf;
		size_t len = pattern.Len();
		EAddrTbl::Rec ea_rec;
		PPIDArray eac_list;
		StrAssocArray phone_list;
		if(len >= MinSymbCount) {
			StrAssocArray src_ea_list;
			LocationCore * p_locc = PsnObj.LocObj.P_Tbl;
			SString name, temp_name;
			LocationTbl::Rec loc_rec;
			PersonTbl::Rec psn_rec;
			PsnObj.LocObj.GetEaListBySubstring(pattern, &src_ea_list, srch_substr ? 0 : 1);
			if(src_ea_list.getCount()) {
				SETIFZ(p_list, new StrAssocArray());
				for(uint i = 0; i < src_ea_list.getCount(); i++) {
					const  PPID ea_id = src_ea_list.Get(i).Id;
					if(p_locc->GetEAddr(ea_id, &ea_rec) > 0) {
						if(ea_rec.LinkObjType == PPOBJ_PERSON && LocalFlags & lfPerson && PsnObj.Fetch(ea_rec.LinkObjID, &psn_rec) > 0) {
							eac_list.add(ea_rec.ID);
							reinterpret_cast<const PPEAddr *>(ea_rec.Addr)->GetPhone(phone_buf.Z());
							(temp_name = phone_buf).Space().Cat(psn_rec.Name);
							p_list->Add(ea_id, 0, temp_name);
						}
						else if(ea_rec.LinkObjType == PPOBJ_LOCATION && (LocalFlags & (lfLocation|lfStandaloneLocOnly)) &&
							PsnObj.LocObj.Fetch(ea_rec.LinkObjID, &loc_rec) > 0) {
							if(!(LocalFlags & lfStandaloneLocOnly) || (loc_rec.Type == LOCTYP_ADDRESS && loc_rec.Flags & LOCF_STANDALONE)) {
								eac_list.add(ea_rec.ID);
								reinterpret_cast<const PPEAddr *>(ea_rec.Addr)->GetPhone(phone_buf.Z());
								temp_name = phone_buf;
								name.Z();
								if(name.IsEmpty() && loc_rec.Name[0])
									name = loc_rec.Name;
								if(name.IsEmpty())
									LocationCore::GetExField(&loc_rec, LOCEXSTR_CONTACT, name);
								name.SetIfEmpty(loc_rec.Code);
								if(name.IsEmpty())
									LocationCore::GetExField(&loc_rec, LOCEXSTR_FULLADDR, name);
								if(name.IsEmpty())
									LocationCore::GetExField(&loc_rec, LOCEXSTR_SHORTADDR, name);
								if(name.NotEmptyS())
									temp_name.Space().Cat(name);
								p_list->Add(ea_id, 0, temp_name);
							}
						}
					}
				}
			}
		}
	}
	if(p_list && p_list->getCount() == 0)
		ZDELETE(p_list);
	return p_list;
}

int PhoneSelExtra::Search(long id, SString & rBuf)
{
	int    ok = -1;
	rBuf.Z();
	EAddrTbl::Rec ea_rec;
	if(PsnObj.LocObj.P_Tbl->GetEAddr(id, &ea_rec) > 0) {
		reinterpret_cast<const PPEAddr *>(ea_rec.Addr)->GetPhone(rBuf);
		ok = 1;
	}
	return ok;
}

int PhoneSelExtra::SearchText(const char * pText, long * pID, SString & rBuf)
{
	rBuf.Z();
	int    ok = -1;
	PPID   result_id = 0;
	if(!isempty(pText)) {
		LongArray ea_list;
		if(PsnObj.LocObj.P_Tbl->SearchPhoneIndex(pText, 0, ea_list) > 0 && ea_list.getCount()) {
			result_id = ea_list.get(0);
			EAddrTbl::Rec ea_rec;
			if(PsnObj.LocObj.P_Tbl->GetEAddr(result_id, &ea_rec) > 0) {
				reinterpret_cast<const PPEAddr *>(ea_rec.Addr)->GetPhone(rBuf);
				ok = 1;
			}
		}
	}
	ASSIGN_PTR(pID, result_id);
	return ok;
}
//
//
//
PersonCtrlGroup::Rec::Rec() : PsnKindID(0), PersonID(0), SCardID(0), Flags(0)
{
}

PersonCtrlGroup::PersonCtrlGroup(uint ctlsel, uint ctlSCardCode, PPID psnKindID, long flags) :
	CtrlGroup(), Flags(flags), Ctlsel(ctlsel), CtlSCardCode(ctlSCardCode), CtlAnonym(0)
{
	Data.PsnKindID = psnKindID;
}

void   PersonCtrlGroup::SetAnonymCtrlId(uint ctl) { CtlAnonym = ctl; }
void   PersonCtrlGroup::SetPersonKind(long psnKindID) { Data.PsnKindID = psnKindID; }

int PersonCtrlGroup::setData(TDialog * pDlg, void * pData)
{
	int    ok = -1;
	if(pData) {
		long   psn_combo_flags = 0;
		if(Flags & fCanInsert)
			psn_combo_flags |= OLW_CANINSERT;
		if(Flags & fLoadDefOnOpen)
			psn_combo_flags |= OLW_LOADDEFONOPEN;
		// @v11.1.10 {
		if(Flags & fUseByContextValue)
			psn_combo_flags |= OLW_INSCONTEXTEDITEMS;
		// } @v11.1.10 
		Data = *static_cast<Rec *>(pData);
		PPID   person_id = (Data.Flags & Data.fAnonym) ? 0 :  Data.PersonID;
		SetupPPObjCombo(pDlg, Ctlsel, PPOBJ_PERSON, person_id, psn_combo_flags, reinterpret_cast<void *>(Data.PsnKindID));
		SetupAnonym(pDlg, Data.Flags & Data.fAnonym);
		if(CtlSCardCode) {
			pDlg->SetupWordSelector(CtlSCardCode, new SCardSelExtra(0), Data.SCardID, 4, 0); // Выбор карты без комбобокса
		}
	}
	return ok;
}

int PersonCtrlGroup::getData(TDialog * pDlg, void * pData)
{
	int    ok = 1;
	int    anonym = 0;
	pDlg->getCtrlData(Ctlsel, &Data.PersonID);
	if(CtlAnonym) {
		anonym = pDlg->getCtrlUInt16(CtlAnonym);
		SETFLAG(Data.Flags, Data.fAnonym, anonym);
	}
	if(anonym) {
		Data.PersonID = 0;
	}
	if(CtlSCardCode)
		pDlg->getCtrlData(CtlSCardCode, &Data.SCardID);
	if(pData) {
		*static_cast<Rec *>(pData) = Data;
	}
	return ok;
}

void PersonCtrlGroup::SetupAnonym(TDialog * pDlg, int a)
{
	if(CtlAnonym) {
		if(a < 0)
			a = BIN(pDlg->getCtrlUInt16(CtlAnonym));
		else
			pDlg->setCtrlUInt16(CtlAnonym, BIN(a > 0));
		pDlg->disableCtrl(Ctlsel, (a > 0));
	}
}

int PersonCtrlGroup::SelectByCode(TDialog * pDlg)
{
	int    ok = -1;
	ComboBox * p_combo = static_cast<ComboBox *>(pDlg->getCtrlView(Ctlsel));
	if(p_combo && p_combo->GetLink()) {
		PPPersonKind psn_kind_rec;
		PPID   reg_type_id = 0;
		if(SearchObject(PPOBJ_PERSONKIND, Data.PsnKindID, &psn_kind_rec) > 0)
			reg_type_id = psn_kind_rec.CodeRegTypeID;
		if(reg_type_id > 0) {
			SString code;
			SString title;
			PPRegisterType reg_type_rec;
			SearchObject(PPOBJ_REGISTERTYPE, reg_type_id, &reg_type_rec);
			PPLoadText(PPTXT_SEARCHPERSON, title);
			PPInputStringDialogParam isd_param(title, reg_type_rec.Name);
			if(InputStringDialog(isd_param, code) > 0) {
				PPIDArray psn_list;
				PPObjPerson psn_obj;
				if(psn_obj.GetListByRegNumber(reg_type_id, 0, code, psn_list) > 0)
					if(psn_list.getCount()) {
						pDlg->setCtrlData(Ctlsel, &psn_list.at(0));
						TView::messageCommand(pDlg, cmCBSelected, p_combo);
						ok = 1;
					}
			}
		}
	}
	return ok;
}

void PersonCtrlGroup::handleEvent(TDialog * pDlg, TEvent & event)
{
	if(event.isKeyDown(kbF2)) {
		ComboBox * p_combo = static_cast<ComboBox *>(pDlg->getCtrlView(Ctlsel));
		if(p_combo && pDlg->IsCurrentView(p_combo->GetLink())) {
			SelectByCode(pDlg);
			pDlg->clearEvent(event);
		}
	}
	else if(CtlAnonym && event.isClusterClk(CtlAnonym)) {
		SetupAnonym(pDlg, -1);
	}
	else if(Ctlsel && event.isCbSelected(Ctlsel)) {
		MessagePersonBirthDay(pDlg, pDlg->getCtrlLong(Ctlsel));
	}
	else if(event.isCmd(cmWSSelected)) {
		if(CtlSCardCode && event.isCtlEvent(CtlSCardCode)) {
			pDlg->getCtrlData(CtlSCardCode, &Data.SCardID);
			SCardTbl::Rec sc_rec;
			if(ScObj.Fetch(Data.SCardID, &sc_rec) > 0) {
				if(sc_rec.PersonID && Data.PsnKindID) {
					PPIDArray kind_list;
					PsnObj.P_Tbl->GetKindList(sc_rec.PersonID, &kind_list);
					if(!kind_list.lsearch(Data.PsnKindID)) {
						Data.SCardID = 0;
						pDlg->setCtrlData(CtlSCardCode, &Data.SCardID);
					}
				}
				if(Data.SCardID && sc_rec.PersonID != Data.PersonID) {
					Data.PersonID = sc_rec.PersonID;
					pDlg->setCtrlLong(Ctlsel, Data.PersonID);
					SetupAnonym(pDlg, BIN(!Data.PersonID));
					MessagePersonBirthDay(pDlg, Data.PersonID);
				}
			}
			pDlg->clearEvent(event);
		}
	}
}
//
//
//
PersonListCtrlGroup::Rec::Rec(PPID psnKindID, const PPIDArray * pPersonList)
{
	Init(psnKindID, pPersonList);
}

void PersonListCtrlGroup::Rec::Init(PPID psnKindID, const PPIDArray * pPersonList)
{
	PsnKindID = psnKindID;
	if(!RVALUEPTR(List, pPersonList))
		List.freeAll();
}

PersonListCtrlGroup::PersonListCtrlGroup(uint ctlsel, uint ctlSelPsnKind, uint cmPsnList, long flags) :
	CtrlGroup(), Ctlsel(ctlsel), CtlselPsnKind(ctlSelPsnKind), CmPsnList(cmPsnList), Flags(flags)
{
}

PersonListCtrlGroup::~PersonListCtrlGroup()
{
}

int PersonListCtrlGroup::Setup(TDialog * pDlg, PPID psnKindID, int force /*=0*/)
{
	int    ok = -1;
	if(pDlg) {
		PPID   new_psn_kind_id = pDlg->getCtrlLong(CtlselPsnKind);
		SETIFZ(new_psn_kind_id, Data.PsnKindID);
		if(new_psn_kind_id != Data.PsnKindID || force) {
			SetupPPObjCombo(pDlg, Ctlsel, PPOBJ_PERSON, Data.List.getSingle(), 0, reinterpret_cast<void *>(Data.PsnKindID = new_psn_kind_id));
		}
		pDlg->enableCommand(CmPsnList, LOGIC(Data.PsnKindID));

		PPID   id = Data.List.getSingle();
		PPID   prev_id = pDlg->getCtrlLong(Ctlsel);
		if(id != prev_id)
			pDlg->setCtrlData(Ctlsel, &id);
		if(Data.List.getCount() > 1)
			SetComboBoxListText(pDlg, Ctlsel);
		pDlg->disableCtrl(Ctlsel, (Data.List.getCount() > 1));
		ok = 1;
	}
	return 1;
}

int PersonListCtrlGroup::setData(TDialog * pDlg, void * pRec)
{
	if(pRec)
		Data = *static_cast<const Rec *>(pRec);
	else
		Data.Init();
	SetupPPObjCombo(pDlg, CtlselPsnKind, PPOBJ_PERSONKIND, Data.PsnKindID, 0, 0);
	Setup(pDlg, Data.PsnKindID, 1);
	return 1;
}

int PersonListCtrlGroup::getData(TDialog * pDlg, void * pData)
{
	int    ok = -1;
	if(pDlg && pData) {
		pDlg->getCtrlData(CtlselPsnKind, &Data.PsnKindID);
		PPID   id = pDlg->getCtrlLong(Ctlsel);
		if(id)
			Data.List.addUnique(id);
		Data.List.sort();
		*static_cast<Rec *>(pData) = Data;
		ok = 1;
	}
	return ok;
}

int PersonListCtrlGroup::SelectByCode(TDialog * pDlg)
{
	int    ok = -1;
	ComboBox * p_combo = static_cast<ComboBox *>(pDlg->getCtrlView(Ctlsel));
	if(p_combo && p_combo->GetLink()) {
		PPPersonKind psn_kind_rec;
		PPID   reg_type_id = 0;
		if(SearchObject(PPOBJ_PERSONKIND, Data.PsnKindID, &psn_kind_rec) > 0)
			reg_type_id = psn_kind_rec.CodeRegTypeID;
		if(reg_type_id > 0) {
			SString code;
			SString title;
			PPRegisterType reg_type_rec;
			SearchObject(PPOBJ_REGISTERTYPE, reg_type_id, &reg_type_rec);
			PPLoadText(PPTXT_SEARCHPERSON, title);
			PPInputStringDialogParam isd_param(title, reg_type_rec.Name);
			if(InputStringDialog(isd_param, code) > 0) {
				PPIDArray psn_list;
				PPObjPerson psn_obj;
				if(psn_obj.GetListByRegNumber(reg_type_id, 0, code, psn_list) > 0)
					if(psn_list.getCount()) {
						pDlg->setCtrlData(Ctlsel, &psn_list.at(0));
						TView::messageCommand(pDlg, cmCBSelected, p_combo);
						ok = 1;
					}
			}
		}
	}
	return ok;
}

void PersonListCtrlGroup::handleEvent(TDialog * pDlg, TEvent & event)
{
	if(TVCMD == CmPsnList) {
		if(Data.PsnKindID) {
			PPIDArray result_list = Data.List;
			PPID   id = pDlg->getCtrlLong(Ctlsel);
			if(id)
				result_list.addUnique(id);
			ListToListData data(PPOBJ_PERSON, reinterpret_cast<void *>(Data.PsnKindID), &result_list);
			data.TitleStrID = PPTXT_SELPERSONLIST;
			if(ListToListDialog(&data) > 0) {
				Data.List = result_list;
				Setup(pDlg, Data.PsnKindID);
			}
		}
	}
	else if(event.isCbSelected(Ctlsel)) {
		if(Data.List.getSingle()) {
			const  PPID id = pDlg->getCtrlLong(Ctlsel);
			Data.List.freeAll();
			if(id)
				Data.List.add(id);
		}
	}
	else if(CtlselPsnKind && event.isCbSelected(CtlselPsnKind)) {
		Setup(pDlg, pDlg->getCtrlLong(CtlselPsnKind));
	}
	else if(event.isKeyDown(kbF2)) {
		ComboBox * p_combo = static_cast<ComboBox *>(pDlg->getCtrlView(Ctlsel));
		if(p_combo && pDlg->IsCurrentView(p_combo->GetLink())) {
			SelectByCode(pDlg);
			pDlg->clearEvent(event);
		}
	}
	else
		return;
	pDlg->clearEvent(event);
}
//
//
//
#define SCARD_MIN_SYMBS 4

SCardCtrlGroup::Rec::Rec() : SCardID(0)
{
}

SCardCtrlGroup::SCardCtrlGroup(uint ctlselSCardSer, uint ctlSCard, uint cmScsList) : CtlselSCardSer(ctlselSCardSer), CtlSCard(ctlSCard), CmSCSerList(cmScsList)
{
}

int SCardCtrlGroup::Setup(TDialog * pDlg, int event)
{
	int    ok = -1;
	if(pDlg) {
		PPID   ser_id = 0;
		PPID   card_id = 0;
		if(event == 1) {
			if(Data.SCardSerList.getCount() > 1)
				SetComboBoxListText(pDlg, CtlselSCardSer);
			else
				pDlg->setCtrlLong(CtlselSCardSer, ser_id = Data.SCardSerList.getSingle());
			card_id = Data.SCardID;
		}
		else {
			if(event == 2) {
				ser_id = pDlg->getCtrlLong(CtlselSCardSer);
				Data.SCardSerList.clear();
				Data.SCardSerList.addnz(ser_id);
			}
			else if(event == 3) {
				if(Data.SCardSerList.getCount() > 1)
					SetComboBoxListText(pDlg, CtlselSCardSer);
				else
					pDlg->setCtrlLong(CtlselSCardSer, ser_id = Data.SCardSerList.getSingle());
			}
			card_id = pDlg->getCtrlLong(CtlSCard);
		}
		pDlg->disableCtrl(CtlselSCardSer, (Data.SCardSerList.getCount() > 1));
		pDlg->SetupWordSelector(CtlSCard, new SCardSelExtra(ser_id), card_id, SCARD_MIN_SYMBS, 0); // Выбор карты без комбобокса
		ok = 1;
	}
	return ok;
}

int SCardCtrlGroup::setData(TDialog * pDlg, void * pData)
{
	int    ok = 1;
	Rec  * p_data = static_cast<Rec *>(pData);
	RVALUEPTR(Data, p_data);
	if(CtlselSCardSer)
		SetupPPObjCombo(pDlg, CtlselSCardSer, PPOBJ_SCARDSERIES, Data.SCardSerList.getSingle(), OLW_LOADDEFONOPEN, 0);
	Setup(pDlg, 1);
	return ok;
}

int SCardCtrlGroup::getData(TDialog * pDlg, void * pData)
{
	if(CtlSCard)
		Data.SCardID = pDlg->getCtrlLong(CtlSCard);
	ASSIGN_PTR(static_cast<Rec *>(pData), Data);
	return 1;
}

void SCardCtrlGroup::handleEvent(TDialog * pDlg, TEvent & event)
{
	if(CtlselSCardSer && event.isCbSelected(CtlselSCardSer)) {
		Setup(pDlg, 2);
		pDlg->clearEvent(event);
	}
	else if(CmSCSerList && event.isCmd(CmSCSerList)) {
		PPID   ser_id = 0;
		PPIDArray list(Data.SCardSerList);
		pDlg->getCtrlData(CtlselSCardSer, &ser_id);
		if(ser_id)
			list.addUnique(ser_id);
		ListToListData data(PPOBJ_SCARDSERIES, 0, &list);
		data.Flags |= ListToListData::fIsTreeList;
		data.TitleStrID = PPTXT_SELSCARDSERLIST;
		if(ListToListDialog(&data) > 0) {
			Data.SCardSerList = list;
			Setup(pDlg, 3);
		}
		pDlg->clearEvent(event);
	}
}
//
//
//
FiasSelExtra::FiasSelExtra(PPFiasReference * pOuterFiasRef) : WordSel_ExtraBlock(), State(0)
{
	if(pOuterFiasRef) {
		P_Fr = pOuterFiasRef;
		State |= stOuterFiasRef;
	}
	else
		P_Fr = new PPFiasReference;
}

FiasSelExtra::~FiasSelExtra()
{
	if(!(State & stOuterFiasRef))
		ZDELETE(P_Fr);
}

StrAssocArray * FiasSelExtra::GetList(const char * pText)
{
	StrAssocArray * p_list = 0;
	if(P_Fr) {
		SString pattern(pText);
		if(pattern.Len()) {
			size_t len = pattern.Len();
			PPIDArray id_list;
			SString temp_buf;
			if(len == 6 && pattern.IsDec()) {
				P_Fr->FT.GetHouseListByZIP(pattern, id_list);
				if(id_list.getCount()) {
					p_list = new StrAssocArray;
					for(uint i = 0; i < id_list.getCount(); i++) {
						const  PPID _id = id_list.get(i);
						if(P_Fr->MakeAddressText(_id, PPFiasReference::matfTryHouse|PPFiasReference::matfZipPrefix, temp_buf) > 0)
							p_list->Add(_id, temp_buf, 0);
					}
				}
			}
			else if(len >= MinSymbCount) {
                P_Fr->SearchObjByText(pattern, PPFiasReference::stfPrefix, 0, id_list);
                if(id_list.getCount()) {
					p_list = new StrAssocArray;
					for(uint i = 0; i < id_list.getCount(); i++) {
						const  PPID _id = id_list.get(i);
						if(P_Fr->MakeAddressText(_id, PPFiasReference::matfZipPrefix, temp_buf) > 0)
							p_list->Add(_id, temp_buf, 0);
					}
                }
			}
		}
		if(p_list) {
			if(p_list->getCount() == 0) {
				ZDELETE(p_list);
			}
			else
				p_list->SortByText();
		}
	}
	return p_list;
}

int FiasSelExtra::Search(long id, SString & rBuf)
{
	int    ok = 0;
	if(id && P_Fr) {
		if(P_Fr->MakeAddressText(id, PPFiasReference::matfZipPrefix, rBuf) > 0)
			ok = 1;
	}
	return ok;
}

int FiasSelExtra::SearchText(const char * pText, long * pID, SString & rBuf)
{
	return -1;
}
//
//
//
FiasAddressCtrlGroup::Rec::Rec() : TerminalFiasID(0)
{
}

FiasAddressCtrlGroup::FiasAddressCtrlGroup(uint ctlEdit, uint ctlInfo) : CtlEdit(ctlEdit), CtlInfo(ctlInfo)
{
}

int FiasAddressCtrlGroup::Setup(TDialog * pDlg, int event)
{
	int    ok = -1;
	if(pDlg) {
		PPID   fias_id = 0;
		if(event == 1) {
			fias_id = Data.TerminalFiasID;
		}
		else {
			fias_id = pDlg->getCtrlLong(CtlEdit);
		}
		pDlg->SetupWordSelector(CtlEdit, new FiasSelExtra(&Fr), fias_id, 4, 0);
		ok = 1;
	}
	return ok;
}

int FiasAddressCtrlGroup::setData(TDialog * pDlg, void * pData)
{
	int    ok = 1;
	Rec  * p_data = static_cast<Rec *>(pData);
	RVALUEPTR(Data, p_data);
	Setup(pDlg, 1);
	return ok;
}

int FiasAddressCtrlGroup::getData(TDialog * pDlg, void * pData)
{
	if(CtlEdit) {
		Data.TerminalFiasID = pDlg->getCtrlLong(CtlEdit);
	}
	ASSIGN_PTR(static_cast<Rec *>(pData), Data);
	return 1;
}

void FiasAddressCtrlGroup::handleEvent(TDialog * pDlg, TEvent & event)
{
}
//
//
//
PosNodeCtrlGroup::Rec::Rec(const ObjIdListFilt * pList)
{
	RVALUEPTR(List, pList);
}

PosNodeCtrlGroup::PosNodeCtrlGroup(uint ctlselLoc, uint cmEditList) : Ctlsel(ctlselLoc), CmEditList(cmEditList)
{
}

void PosNodeCtrlGroup::handleEvent(TDialog * pDlg, TEvent & event)
{
	if(event.isCmd(CmEditList)) {
		PPIDArray node_list;
		if(Data.List.IsExists())
			node_list = Data.List.Get();
		if(!node_list.getCount())
			node_list.setSingleNZ(pDlg->getCtrlLong(Ctlsel));
		ListToListData ll_data(PPOBJ_CASHNODE, 0, &node_list);
		ll_data.TitleStrID = PPTXT_SELCASHNODES;
		ll_data.Flags |= ListToListData::fIsTreeList;
		int    r = ListToListDialog(&ll_data);
		if(r > 0) {
			Data.List.Set(&node_list);
			SetupComboByBuddyList(pDlg, Ctlsel, Data.List);
		}
		else if(!r)
			PPError();
		pDlg->clearEvent(event);
	}
}

int PosNodeCtrlGroup::setData(TDialog * pDlg, void * pData)
{
	Data = *static_cast<Rec *>(pData);
	PPObjCashNode::SelFilt sf;
	sf.Flags |= sf.fSkipPassive;
	SetupPPObjCombo(pDlg, Ctlsel, PPOBJ_CASHNODE, 0, OLW_CANSELUPLEVEL, &sf);
	SetupComboByBuddyList(pDlg, Ctlsel, Data.List);
	return 1;
}

int PosNodeCtrlGroup::getData(TDialog * pDlg, void * pData)
{
	Rec * p_rec = static_cast<Rec *>(pData);
	if(Data.List.GetCount() <= 1) {
		const  PPID temp_id = pDlg->getCtrlLong(Ctlsel);
		Data.List.Z().Add(temp_id);
	}
	*p_rec = Data;
	return 1;
}
//
//
//
QuotKindCtrlGroup::Rec::Rec(const ObjIdListFilt * pList)
{
	RVALUEPTR(List, pList);
}

QuotKindCtrlGroup::QuotKindCtrlGroup(uint ctlsel, uint cmEditList) : Ctlsel(ctlsel), CmEditList(cmEditList)
{
}

void QuotKindCtrlGroup::handleEvent(TDialog * pDlg, TEvent & event)
{
	if(event.isCmd(CmEditList)) {
		PPIDArray ary;
		if(Data.List.IsExists())
			ary = Data.List.Get();
		if(!ary.getCount())
			ary.setSingleNZ(pDlg->getCtrlLong(Ctlsel));
		ListToListData lst(PPOBJ_QUOTKIND, 0, &ary);
		lst.TitleStrID = 0; // PPTXT_XXX;
		if(ListToListDialog(&lst) > 0) {
			Data.List.Set(&ary);
			if(Data.List.GetCount() > 1) {
				SetComboBoxListText(pDlg, Ctlsel);
				pDlg->disableCtrl(Ctlsel, true);
			}
			else {
				pDlg->setCtrlLong(Ctlsel, Data.List.GetSingle());
				pDlg->disableCtrl(Ctlsel, false);
			}
		}
		pDlg->clearEvent(event);
	}
}

int QuotKindCtrlGroup::setData(TDialog * pDlg, void * pData)
{
	Data = *static_cast<const Rec *>(pData);
	SetupPPObjCombo(pDlg, Ctlsel, PPOBJ_QUOTKIND, Data.List.GetSingle(), 0, 0);
	if(Data.List.GetCount() > 1) {
		SetComboBoxListText(pDlg, Ctlsel);
		pDlg->disableCtrl(Ctlsel, true);
	}
	else {
		pDlg->setCtrlLong(Ctlsel, Data.List.GetSingle());
		pDlg->disableCtrl(Ctlsel, false);
	}
	return 1;
}

int QuotKindCtrlGroup::getData(TDialog * pDlg, void * pData)
{
	Rec * p_rec = static_cast<Rec *>(pData);
	if(Data.List.GetCount() <= 1) {
		const  PPID temp_id = pDlg->getCtrlLong(Ctlsel);
		Data.List.Z().Add(temp_id);
	}
	*p_rec = Data;
	return 1;
}
//
//
//
StaffCalCtrlGroup::Rec::Rec(const ObjIdListFilt * pList)
{
	RVALUEPTR(List, pList);
}

StaffCalCtrlGroup::StaffCalCtrlGroup(uint ctlsel, uint cmEditList) : Ctlsel(ctlsel), CmEditList(cmEditList)
{
}

void StaffCalCtrlGroup::handleEvent(TDialog * pDlg, TEvent & event)
{
	if(event.isCmd(CmEditList)) {
		PPIDArray ary;
		if(Data.List.IsExists())
			ary = Data.List.Get();
		if(!ary.getCount())
			ary.setSingleNZ(pDlg->getCtrlLong(Ctlsel));
		ListToListData lst(PPOBJ_STAFFCAL, 0, &ary);
		lst.TitleStrID = 0; // PPTXT_XXX;
		if(ListToListDialog(&lst) > 0) {
			Data.List.Set(&ary);
			if(Data.List.GetCount() > 1) {
				SetComboBoxListText(pDlg, Ctlsel);
				pDlg->disableCtrl(Ctlsel, true);
			}
			else {
				pDlg->setCtrlLong(Ctlsel, Data.List.GetSingle());
				pDlg->disableCtrl(Ctlsel, false);
			}
		}
		pDlg->clearEvent(event);
	}
}

int StaffCalCtrlGroup::setData(TDialog * pDlg, void * pData)
{
	Data = *static_cast<Rec *>(pData);
	SetupPPObjCombo(pDlg, Ctlsel, PPOBJ_STAFFCAL, Data.List.GetSingle(), 0, 0);
	if(Data.List.GetCount() > 1) {
		SetComboBoxListText(pDlg, Ctlsel);
		pDlg->disableCtrl(Ctlsel, true);
	}
	else {
		pDlg->setCtrlLong(Ctlsel, Data.List.GetSingle());
		pDlg->disableCtrl(Ctlsel, false);
	}
	return 1;
}

int StaffCalCtrlGroup::getData(TDialog * pDlg, void * pData)
{
	Rec * p_rec = static_cast<Rec *>(pData);
	if(Data.List.GetCount() <= 1) {
		const  PPID temp_id = pDlg->getCtrlLong(Ctlsel);
		Data.List.Z().Add(temp_id);
	}
	*p_rec = Data;
	return 1;
}
//
//
//
PersonOpCtrlGroup::Rec::Rec(const ObjIdListFilt * pPsnOpList, PPID prmrID, PPID scndID) : PrmrID(prmrID), ScndID(scndID)
{
	RVALUEPTR(PsnOpList, pPsnOpList);
}

PersonOpCtrlGroup::PersonOpCtrlGroup(uint ctlselPsnOp, uint ctlselPsn1, uint ctlselPsn2, uint cmEditPsnOpList) :
	CtlselPsnOp(ctlselPsnOp), CtlselPsn1(ctlselPsn1), CtlselPsn2(ctlselPsn2), CmEditPsnOpList(cmEditPsnOpList)
{
}

int PersonOpCtrlGroup::EditList(TDialog * pDlg)
{
	int    ok = -1;
	if(pDlg) {
		PPIDArray ary;
		if(Data.PsnOpList.IsExists())
			ary = Data.PsnOpList.Get();
		if(!ary.getCount()) {
			const  PPID temp_id = pDlg->getCtrlLong(CtlselPsnOp);
			if(temp_id)
				ary.insert(&temp_id);
		}
		ListToListData lst(PPOBJ_PERSONOPKIND, 0, &ary);
		lst.Flags |= ListToListData::fIsTreeList;
		lst.TitleStrID = 0; // PPTXT_XXX;
		if(ListToListDialog(&lst) > 0) {
			Data.PsnOpList.Set(&ary);
			if(Data.PsnOpList.GetCount() > 1) {
				SetComboBoxListText(pDlg, CtlselPsnOp);
				pDlg->disableCtrl(CtlselPsnOp, true);
			}
			else {
				pDlg->setCtrlLong(CtlselPsnOp, Data.PsnOpList.GetSingle());
				pDlg->disableCtrl(CtlselPsnOp, false);
			}
			ok = 1;
			ReplySelection(pDlg);
		}
	}
	else
		ok = 0;
	return ok;
}

int PersonOpCtrlGroup::ReplySelection(TDialog * pDlg)
{
	int    ok = 0;
	if(pDlg) {
		bool   disable_psn1 = false;
		bool   disable_psn2 = false;
		PPID   prev_psn1k = 0;
		PPID   prev_psn2k = 0;
		PersonOpCtrlGroup::Rec op_rec;
		PPIDArray ary;
		getData(pDlg, &op_rec);
		if(op_rec.PsnOpList.IsExists())
			ary = op_rec.PsnOpList.Get();
		for(uint i = 0; i < ary.getCount(); i++) {
			PPPsnOpKindPacket pok_pack;
			if(PokObj.GetPacket(ary.at(i), &pok_pack) > 0) {
				PPID psn1k = pok_pack.PCPrmr.PersonKindID;
				PPID psn2k = pok_pack.PCScnd.PersonKindID;
				SETIFZ(prev_psn1k, psn1k);
				if(psn1k && prev_psn1k != psn1k)
					disable_psn1 = true;
				SETIFZ(prev_psn2k, psn2k);
				if(psn2k && prev_psn2k != psn2k)
					disable_psn2 = true;
			}
		}
		if(CtlselPsn1) {
			SetupPPObjCombo(pDlg, CtlselPsn1, PPOBJ_PERSON, Data.PrmrID, OLW_LOADDEFONOPEN, reinterpret_cast<void *>(disable_psn1 ? 0 : prev_psn1k));
			pDlg->disableCtrl(CtlselPsn1, disable_psn1);
		}
		if(CtlselPsn2) {
			SetupPPObjCombo(pDlg, CtlselPsn2, PPOBJ_PERSON, Data.ScndID, OLW_LOADDEFONOPEN, reinterpret_cast<void *>(disable_psn2 ? 0 : prev_psn2k));
			pDlg->disableCtrl(CtlselPsn2, disable_psn2);
		}
		ok = 1;
	}
	return ok;
}

void PersonOpCtrlGroup::handleEvent(TDialog * pDlg, TEvent & event)
{
	if(TVCOMMAND) {
		if(TVCMD == CmEditPsnOpList) {
			EditList(pDlg);
			pDlg->clearEvent(event);
		}
		else if(event.isCbSelected(CtlselPsnOp)) {
			Data.PrmrID = Data.ScndID = 0;
			ReplySelection(pDlg);
			pDlg->clearEvent(event);
		}
	}
}

int PersonOpCtrlGroup::setData(TDialog * pDlg, void * pData)
{
	Data = *static_cast<Rec *>(pData);
	SetupPPObjCombo(pDlg, CtlselPsnOp, PPOBJ_PERSONOPKIND, Data.PsnOpList.GetSingle(), OLW_CANSELUPLEVEL, 0);
	if(Data.PsnOpList.GetCount() > 1) {
		SetComboBoxListText(pDlg, CtlselPsnOp);
		pDlg->disableCtrl(CtlselPsnOp, true);
	}
	else {
		pDlg->setCtrlLong(CtlselPsnOp, Data.PsnOpList.GetSingle());
		pDlg->disableCtrl(CtlselPsnOp, false);
	}
	ReplySelection(pDlg);
	return 1;
}

int PersonOpCtrlGroup::getData(TDialog * pDlg, void * pData)
{
	Rec * p_rec = static_cast<Rec *>(pData);
	if(Data.PsnOpList.GetCount() <= 1) {
		pDlg->getCtrlData(CtlselPsn1, &Data.PrmrID);
		pDlg->getCtrlData(CtlselPsn2, &Data.ScndID);
		const  PPID temp_id = pDlg->getCtrlLong(CtlselPsnOp);
		Data.PsnOpList.Z().Add(temp_id);
	}
	*p_rec = Data;
	return 1;
}

SpinCtrlGroup::Rec::Rec() : Value(0)
{
}

SpinCtrlGroup::SpinCtrlGroup(uint ctlEdit, uint cmdUp, uint ctlUp, uint cmdDown, uint ctlDown, long minVal, long maxVal) :
	CtlEdit(ctlEdit), CmdUp(cmdUp), CtlUp(ctlUp), CmdDown(cmdDown), CtlDown(ctlDown), MinVal(minVal), MaxVal(maxVal)
{
}

/*virtual*/int SpinCtrlGroup::setData(TDialog * pDlg, void * pData)
{
	int    ok = -1;
	if(pDlg && pData) {
		Data = *static_cast<Rec *>(pData);
		Data.Value = (Data.Value > MaxVal) ? MaxVal : Data.Value;
		Data.Value = (Data.Value < MinVal) ? MinVal : Data.Value;
		pDlg->setCtrlLong(CtlEdit, Data.Value);
		ok = 1;
	}
	return ok;
}

/*virtual*/int SpinCtrlGroup::getData(TDialog * pDlg, void * pData)
{
	int    ok = -1;
	if(pDlg && pData) {
		Data.Value = pDlg->getCtrlLong(CtlEdit);
		Data.Value = (Data.Value > MaxVal) ? MaxVal : Data.Value;
		Data.Value = (Data.Value < MinVal) ? MinVal : Data.Value;
		*static_cast<Rec *>(pData) = Data;
		ok = 1;
	}
	return ok;
}

/*virtual*/void SpinCtrlGroup::handleEvent(TDialog * pDlg, TEvent & event)
{
	if(TVCOMMAND) {
		if(TVCMD == CmdUp && event.isCtlEvent(CtlUp)) {
			Data.Value = pDlg->getCtrlLong(CtlEdit);
			Data.Value++;
			Data.Value = (Data.Value > MaxVal) ? MaxVal : Data.Value;
			pDlg->setCtrlLong(CtlEdit, Data.Value);
		}
		else if(TVCMD == CmdDown && event.isCtlEvent(CtlDown)) {
			Data.Value = pDlg->getCtrlLong(CtlEdit);
			Data.Value--;
			Data.Value = (Data.Value < MinVal) ? MinVal : Data.Value;
			pDlg->setCtrlLong(CtlEdit, Data.Value);
		}
		else if(TVCMD == cmInputUpdatedByBtn && event.isCtlEvent(CtlEdit)) {
			long prev_val = 0;
			Data.Value = pDlg->getCtrlLong(CtlEdit);
			prev_val = Data.Value;
			Data.Value = (Data.Value < MinVal) ? MinVal : Data.Value;
			Data.Value = (Data.Value > MaxVal) ? MaxVal : Data.Value;
			if(prev_val != Data.Value)
				pDlg->setCtrlLong(CtlEdit, Data.Value);
		}
	}
}
//
// BrandCtrlGroup
//
BrandCtrlGroup::Rec::Rec(const PPIDArray * pList)
{
	RVALUEPTR(List, pList);
}

BrandCtrlGroup::BrandCtrlGroup(uint ctlsel, uint cmSelList) : Ctlsel(ctlsel), CmSelList(cmSelList)
{
}

/*virtual*/int BrandCtrlGroup::setData(TDialog * pDlg, void * pData)
{
	int    ok = -1;
	if(pDlg && pData) {
		Data = *static_cast<Rec *>(pData);
		SetupPPObjCombo(pDlg, Ctlsel, PPOBJ_BRAND, Data.List.getSingle(), OLW_LOADDEFONOPEN, 0);
		ok = Setup(pDlg);
	}
	return ok;
}

/*virtual*/int BrandCtrlGroup::getData(TDialog * pDlg, void * pData)
{
	int    ok = -1;
	if(pDlg && pData) {
		PPID   id = pDlg->getCtrlLong(Ctlsel);
		if(id)
			Data.List.addUnique(id);
		Data.List.sort();
		*static_cast<Rec *>(pData) = Data;
		ok = 1;
	}
	return ok;
}

int BrandCtrlGroup::Setup(TDialog * pDlg)
{
	int    ok = -1;
	if(pDlg) {
		PPID   id = Data.List.getSingle();
		PPID   prev_id = pDlg->getCtrlLong(Ctlsel);
		if(id != prev_id)
			pDlg->setCtrlData(Ctlsel, &id);
		if(Data.List.getCount() > 1)
			SetComboBoxListText(pDlg, Ctlsel);
		pDlg->disableCtrl(Ctlsel, (Data.List.getCount() > 1));
		ok = 1;
	}
	return ok;
}

/*virtual*/void BrandCtrlGroup::handleEvent(TDialog * pDlg, TEvent & event)
{
	if(TVCOMMAND) {
		if(TVCMD == CmSelList) {
			PPID   id = 0;
			PPIDArray brand_list(Data.List);
			pDlg->getCtrlData(Ctlsel, &id);
			if(id)
				brand_list.addUnique(id);
			ListToListData data(PPOBJ_BRAND, 0, &brand_list);
			data.TitleStrID = PPTXT_SELBRANDLIST;
			if(ListToListDialog(&data) > 0) {
				Data.List = brand_list;
				Setup(pDlg);
			}
			pDlg->clearEvent(event);
		}
		else if(event.isCbSelected(Ctlsel)) {
			if(Data.List.getSingle()) {
				const  PPID id = pDlg->getCtrlLong(Ctlsel);
				Data.List.clear();
				Data.List.addnz(id);
			}
		}
	}
}
//
//
// 
PPInputStringDialogParam::PPInputStringDialogParam(const char * pTitle, const char * pInputTitle) :
	Flags(0), Title(pTitle), InputTitle(pInputTitle), P_Wse(0), MaxTextLen(0)
{
}

PPInputStringDialogParam::~PPInputStringDialogParam()
{
	delete P_Wse;
}

int InputStringDialog(PPInputStringDialogParam & rParam, SString & rBuf)
{
	int    ok = -1;
	TDialog * dlg = new TDialog((rParam.Flags & rParam.fInputMemo) ? DLG_MEMO : DLG_INPUT);
	if(CheckDialogPtrErr(&dlg)) {
		dlg->setTitle(rParam.Title);
		if(rParam.InputTitle.NotEmpty())
			dlg->setLabelText(CTL_INPUT_STR, rParam.InputTitle);
		{
			dlg->setCtrlString(CTL_INPUT_STR, rBuf);
		}
		TInputLine * p_il = static_cast<TInputLine *>(dlg->getCtrlView(CTL_INPUT_STR));
		if(p_il) {
			if(rParam.Flags & rParam.fDisableSelection) {
				CALLPTRMEMB(p_il, disableDeleteSelection(1));
			}
			// @v12.4.1 {
			if(rParam.MaxTextLen > 0) {
				p_il->SetupMaxTextLen(rParam.MaxTextLen);
			}
			// } @v12.4.1 
		}
		if(rParam.P_Wse) {
			dlg->SetupWordSelector(CTL_INPUT_STR, rParam.P_Wse, 0, rParam.P_Wse->MinSymbCount, rParam.P_Wse->Flags);
			rParam.P_Wse = 0; // Диалог разрушит объект pParam->P_Wse
		}
		if(ExecView(dlg) == cmOK) {
			dlg->getCtrlString(CTL_INPUT_STR, rBuf);
			ok = 1;
		}
	}
	else
		ok = 0;
	delete dlg;
	return ok;
}

#if 0 // @v12.4.1 {
// @v12.4.1 @todo Объединить эту функцию с общей InputStringDialog()
static int InputStringDialog__(const char * pTitle, const char * pInpTitle, int disableSelection, int inputMemo, SString & rBuf)
{
	int    ok = -1;
	TDialog * dlg = new TDialog(inputMemo ? DLG_MEMO : DLG_INPUT);
	if(CheckDialogPtrErr(&dlg)) {
		dlg->setTitle(pTitle);
		if(pInpTitle)
			dlg->setLabelText(CTL_INPUT_STR, pInpTitle);
		{
		// @v12.4.1 @todo 
		/*int   setMaxLen(size_t maxLen)
		{
			int    ok = 1;
			if(checkirange(maxLen, static_cast<size_t>(1), static_cast<size_t>(4000)))
				SetupInputLine(CTL_BIGTXTEDIT_TEXT, MKSTYPE(S_ZSTRING, maxLen), MKSFMT(maxLen, 0));
			else
				ok = PPSetErrorSLib();
			return ok;
		}*/
		}
		dlg->setCtrlString(CTL_INPUT_STR, rBuf);
		if(disableSelection) {
			TInputLine * il = static_cast<TInputLine *>(dlg->getCtrlView(CTL_INPUT_STR));
			CALLPTRMEMB(il, disableDeleteSelection(1));
		}
		if(ExecView(dlg) == cmOK) {
			dlg->getCtrlString(CTL_INPUT_STR, rBuf);
			ok = 1;
		}
	}
	else
		ok = 0;
	delete dlg;
	return ok;
}
#endif // } 0 @v12.4.1

int InputNumberDialog(const char * pTitle, const char * pInpTitle, double & rValue)
{
	int    ok = -1;
	TDialog * dlg = new TDialog(DLG_INPUTNUM);
	if(CheckDialogPtrErr(&dlg)) {
		dlg->setTitle(pTitle);
		if(pInpTitle)
			dlg->setLabelText(CTL_INPUT_STR, pInpTitle);
		dlg->setCtrlReal(CTL_INPUTNUM_NUM, rValue);
		if(ExecView(dlg) == cmOK) {
			rValue = dlg->getCtrlReal(CTL_INPUTNUM_NUM);
			ok = 1;
		}
	}
	else
		ok = 0;
	delete dlg;
	return ok;
}

int STDCALL EditSysjFilt2(SysJournalFilt * pFilt)
{
	DIALOG_PROC_BODY_P1(SysJFiltDialog, DLG_SYSJFILT2, pFilt);
}

RemoveAllDialog::RemoveAllDialog(uint resID) : TDialog(resID)
{
}

IMPL_HANDLE_EVENT(RemoveAllDialog)
{
	TDialog::handleEvent(event);
	if(event.isClusterClk(CTL_REMOVEALL_WHAT)) {
		disableCtrl(CTLSEL_REMOVEALL_GRP, getCtrlUInt16(CTL_REMOVEALL_WHAT) != 0);
		clearEvent(event);
	}
}

int RemoveAllDialog::setDTS(const RemoveAllParam * pData)
{
	Data = *pData;
	ushort v = 0;
	if(Data.Action == RemoveAllParam::aMoveToGroup)
		v = 0;
	else if(Data.Action == RemoveAllParam::aRemoveAll)
		v = 1;
	setCtrlData(CTL_REMOVEALL_WHAT, &v);
	if(resourceID == DLG_GOODSRMVALL)
		SetupPPObjCombo(this, CTLSEL_REMOVEALL_GRP, PPOBJ_GOODSGROUP, Data.DestGrpID, 0, reinterpret_cast<void *>(GGRTYP_SEL_NORMAL));
	else
		SetupPPObjCombo(this, CTLSEL_REMOVEALL_GRP, PPOBJ_SCARDSERIES, Data.DestGrpID, 0);
	disableCtrl(CTLSEL_REMOVEALL_GRP, Data.Action != RemoveAllParam::aMoveToGroup);
	return 1;
}

int RemoveAllDialog::getDTS(RemoveAllParam * pData)
{
	int    ok = 1;
	uint   sel = 0;
	ushort v = getCtrlUInt16(sel = CTL_REMOVEALL_WHAT);
	if(v == 0)
		Data.Action = RemoveAllParam::aMoveToGroup;
	else if(v == 1)
		Data.Action = RemoveAllParam::aRemoveAll;
	else {
		CALLEXCEPT_PP(PPERR_USERINPUT);
	}
	getCtrlData(sel = CTLSEL_REMOVEALL_GRP, &Data.DestGrpID);
	THROW_PP(Data.Action != RemoveAllParam::aMoveToGroup || Data.DestGrpID,
		resourceID == DLG_GOODSRMVALL ? PPERR_GOODSGROUPNEEDED : PPERR_SCARDSERIESNEEDED);
	ASSIGN_PTR(pData, Data);
	CATCHZOKPPERRBYDLG
	return ok;
}

ResolveGoodsItem::ResolveGoodsItem(PPID goodsID /*= 0*/)
{
	THISZERO();
	GoodsID = goodsID;
}

ResolveGoodsItemList::ResolveGoodsItemList() : TSArray <ResolveGoodsItem> ()
{
}

ResolveGoodsItemList::ResolveGoodsItemList(const ResolveGoodsItemList & s) : TSArray <ResolveGoodsItem> (s)
{
}

ResolveGoodsItemList & FASTCALL ResolveGoodsItemList::operator = (const ResolveGoodsItemList & s)
{
	copy(s);
	return *this;
}

ResolveGoodsItemList & FASTCALL ResolveGoodsItemList::operator = (const PPIDArray & s)
{
	clear();
	SString goods_name;
	for(uint i = 0; i < s.getCount(); i++) {
		ResolveGoodsItem item(s.at(i));
		STRNSCPY(item.GoodsName, GetGoodsName(item.GoodsID, goods_name));
		insert(&item);
	}
	return *this;
}

class ResolveGoodsDialog : public PPListDialog {
	DECL_DIALOG_DATA(ResolveGoodsItemList);
public:
	explicit ResolveGoodsDialog(int flags) :
		PPListDialog((flags & (RESOLVEGF_SHOWRESOLVED|RESOLVEGF_SHOWEXTDLG)) ? DLG_SUBSTGL : DLG_LBXSEL, CTL_LBXSEL_LIST), Flags(flags), GoodsGrpID(0)
	{
		SString subtitle;
		PPLoadText(PPTXT_NOTIMPORTEDGOODS, subtitle);
		setSubTitle(subtitle);
		showCtrl(STDCTL_INSBUTTON,   false);
		showCtrl(STDCTL_DELBUTTON,   false);
		showCtrl(CTL_LBXSEL_UPBTN,   false);
		showCtrl(CTL_LBXSEL_DOWNBTN, false);
		GObj.ReadConfig(&GoodsCfg);
		updateList(-1);
	}
	DECL_DIALOG_SETDTS()
	{
		int    ok = 1;
		RVALUEPTR(Data, pData);
		updateList(-1);
		return ok;
	}
	DECL_DIALOG_GETDTS()
	{
		int    ok = 1;
		if(Flags & RESOLVEGF_RESOLVEALLGOODS) {
			ResolveGoodsItem * p_item = 0;
			for(uint i = 0; ok && Data.enumItems(&i, (void **)&p_item) > 0;)
				ok = p_item->ResolvedGoodsID ? 1 : PPSetError(PPERR_EXISTUNRESOLVDEDGOODS);
		}
		return (ok > 0) ? (pData->copy(Data), 1) : ok;
	}
private:
	DECL_HANDLE_EVENT;
	virtual int editItem(long pos, long id);
	virtual int setupList();
	int    CreateGoods(PPID id, PPID goodsGrpID, int editAfterAdd);
	int    CreateAllGoods();
	int    SubstGoods(long pos, long id);
	int    ResolveGoods(PPID resolveGoodsID, uint firstGoodsPos);
	int    Flags;
	PPID   GoodsGrpID;
	PPGoodsConfig GoodsCfg;
	PPObjGoods GObj;
};

IMPL_HANDLE_EVENT(ResolveGoodsDialog)
{
	PPListDialog::handleEvent(event);
	if(TVCOMMAND) {
		if(TVCMD == cmaAltInsert) {
			long   id = 0;
			getSelection(&id);
			if(!CreateGoods(id - 1, 0, 1))
				PPError();
			clearEvent(event);
		}
		else if(TVCMD == cmCreateAll) {
			if(!CreateAllGoods())
				PPError();
		}
		else if(TVCMD == cmSubstGoods) {
			long   id = 0;
			getSelection(&id);
			if(!SubstGoods(0, id))
				PPError();
		}
	}
}

int ResolveGoodsDialog::setupList()
{
	int    ok = -1;
	SString buf, word;
	ResolveGoodsItem * p_item = 0;
	for(uint i = 0; Data.enumItems(&i, (void **)&p_item) > 0;) {
		if(!p_item->ResolvedGoodsID || Flags & RESOLVEGF_SHOWRESOLVED) {
			int    barcode_added = 0, wo_name = 0, id_added = 0;
			buf.Z();
			if(p_item->GoodsID) {
				PPLoadString("id", word);
				buf.Cat(word).CatDiv(':', 2).Cat(p_item->GoodsID);
				id_added = 1;
			}
			if(sstrlen(p_item->GoodsName)) {
				PPLoadString("name", word);
				if(id_added)
					buf.CatDiv(',', 2);
				buf.Cat(word).CatDiv(':', 2).Cat(p_item->GoodsName);
			}
			else
				wo_name = 1;
			if(wo_name || (Flags & RESOLVEGF_SHOWBARCODE)) {
				PPLoadString("barcode", word);
				if(!wo_name || id_added)
					buf.CatDiv(',', 2);
				buf.Cat(word).CatDiv(':', 2).Cat(p_item->Barcode);
				barcode_added = 1;
			}
			if((wo_name || (Flags & RESOLVEGF_SHOWQTTY)) && p_item->Quantity != 0) {
				PPLoadString("qtty", word);
				if(!wo_name || barcode_added || id_added)
					buf.CatDiv(',', 2);
				buf.Cat(word).CatDiv(':', 2).Cat(p_item->Quantity, MKSFMTD(10, 2, 0));
			}
			if(Flags & RESOLVEGF_SHOWRESOLVED) {
				SString resolve_name;
				if(p_item->ResolvedGoodsID)
					buf.Cat(SLBColumnDelim).Cat(GetGoodsName(p_item->ResolvedGoodsID, resolve_name));
			}
			THROW(addStringToList(i, buf));
		}
	}
	CATCHZOKPPERR
	return ok;
}

int ResolveGoodsDialog::ResolveGoods(PPID resolveGoodsID, uint firstGoodsPos)
{
	int    ok = -1;
	if(resolveGoodsID > 0 && firstGoodsPos >= 0 && firstGoodsPos < Data.getCount()) {
		PPID   goods_id = Data.at(firstGoodsPos).GoodsID;
		char   barcode[48];
		char   goods_name[256];
		memzero(barcode, sizeof(barcode));
		memzero(goods_name, sizeof(goods_name));
		STRNSCPY(barcode, Data.at(firstGoodsPos).Barcode);
		STRNSCPY(goods_name, Data.at(firstGoodsPos).GoodsName);
		if(goods_id) {
			for(uint p = 0; Data.lsearch(&goods_id, &p, CMPF_LONG, offsetof(ResolveGoodsItem, GoodsID)); p++)
				Data.at(p).ResolvedGoodsID = resolveGoodsID;
		}
		else if(sstrlen(barcode) && strcmp(barcode, "0") != 0) {
			for(uint p = 0; Data.lsearch(barcode, &p, PTR_CMPFUNC(Pchar), offsetof(ResolveGoodsItem, Barcode)); p++)
				Data.at(p).ResolvedGoodsID = resolveGoodsID;
		}
		else if(sstrlen(goods_name)) {
			for(uint p = 0; Data.lsearch(goods_name, &p, PTR_CMPFUNC(Pchar), offsetof(ResolveGoodsItem, GoodsName)); p++)
				Data.at(p).ResolvedGoodsID = resolveGoodsID;
		}
		else
			Data.at(firstGoodsPos).ResolvedGoodsID = resolveGoodsID;
		ok = 1;
	}
	return ok;
}

int ResolveGoodsDialog::editItem(long pos, long id)
{
	int    ok = -1;
	int    valid_data = 0;
	PPID   resolve_goods_id = 0;
	ExtGoodsSelDialog * p_dlg = 0;
	if(id > 0) {
		const int maxlike_goods = BIN(Flags & RESOLVEGF_MAXLIKEGOODS);
		TIDlgInitData tidi;
		long   egsd_flags = ExtGoodsSelDialog::GetDefaultFlags();
		if(maxlike_goods)
			egsd_flags |= ExtGoodsSelDialog::fByName;
		THROW_MEM(p_dlg = new ExtGoodsSelDialog(0, maxlike_goods ? 0 : GoodsGrpID, egsd_flags));
		THROW(CheckDialogPtr(&p_dlg));
		if(maxlike_goods) {
			StrAssocArray goods_list;
			SString goods_name;
			goods_name.CatChar('!').Cat(Data.at(id - 1).GoodsName);
			PPWaitStart();
			GObj.P_Tbl->GetListBySubstring(goods_name, &goods_list, -1);
			p_dlg->setSelectionByGoodsList(&goods_list);
			PPWaitStop();
		}
		while(!valid_data && ExecView(p_dlg) == cmOK) {
			if(p_dlg->getDTS(&tidi) > 0) {
				resolve_goods_id = tidi.GoodsID;
				GoodsGrpID       = tidi.GoodsGrpID;
				valid_data = 1;
			}
			else
				PPError();
		}
		THROW(ok = ResolveGoods(resolve_goods_id, id - 1));
	}
	CATCHZOK
	delete p_dlg;
	return ok;
}

int ResolveGoodsDialog::SubstGoods(long pos, long id)
{
	int    ok = -1;
	PPID   resolve_goods_id = 0;
	if(id && SelectGoods(resolve_goods_id) > 0)
		ok = ResolveGoods(resolve_goods_id, id - 1);
	if(ok > 0)
		updateList(-1);
	return ok;
}

int ResolveGoodsDialog::CreateAllGoods()
{
	int    ok = 1;
	PPID   goods_grp_id = 0;
	if(ListBoxSelDialog::Run(PPOBJ_GOODSGROUP, &goods_grp_id, (void *)GGRTYP_SEL_NORMAL) > 0)
		for(uint i = 0; ok > 0 && i < Data.getCount(); i++)
			if(Data.at(i).ResolvedGoodsID == 0)
				ok = CreateGoods(i, goods_grp_id, 0);
	return ok;
}

int ResolveGoodsDialog::CreateGoods(long id, PPID goodsGrpID, int editAfterAdd)
{
	int    ok = 1;
	PPID   goods_id = 0;
	if(id >= 0 && id < (long)Data.getCount()) {
		PPGoodsPacket pack;
		const ResolveGoodsItem & r_item = Data.at(id);
		const SString name(r_item.GoodsName);
		THROW(GObj.InitPacket(&pack, gpkndGoods, 0, 0, Data.at(id).Barcode));
		pack.Rec.UnitID   = GoodsCfg.DefUnitID;
		pack.Rec.ParentID = goodsGrpID;
		STRNSCPY(pack.Rec.Name, name);
		name.CopyTo(pack.Rec.Abbr, sizeof(pack.Rec.Abbr));
		if(r_item.ArID && r_item.ArCode[0] && (CConfig.Flags & CCFLG_USEARGOODSCODE)) {
			PPObjArticle ar_obj;
			ArticleTbl::Rec ar_rec;
			if(ar_obj.Fetch(r_item.ArID, &ar_rec) > 0) {
				ArGoodsCodeTbl::Rec code_rec;
				if(GObj.P_Tbl->SearchByArCode(r_item.ArID, r_item.ArCode, &code_rec) > 0) {
				}
				else {
					MEMSZERO(code_rec);
					code_rec.ArID = r_item.ArID;
					STRNSCPY(code_rec.Code, r_item.ArCode);
					pack.ArCodes.insert(&code_rec);
				}
			}
		}
		if(name.Len() == 0 || editAfterAdd) {
			ok = GObj.Helper_Edit(&goods_id, &pack, gpkndGoods, 1);
			if(ok)
				ok = (ok == cmOK) ? 1 : -1;
		}
		else {
			THROW_PP(pack.Rec.UnitID, PPERR_UNITNEEDED);
			THROW(ok = GObj.PutPacket(&goods_id, &pack, 1));
		}
	}
	if(ok > 0 && goods_id)
		THROW(ResolveGoods(goods_id, id));
	CATCHZOK
	if(ok > 0)
		updateList(-1);
	return ok;
}

int ResolveGoodsDlg(ResolveGoodsItemList * pData, int flags)
{
	int    ok = -1;
	ResolveGoodsDialog * p_dlg = 0;
	if(pData && pData->getCount()) {
		THROW(CheckDialogPtr(&(p_dlg = new ResolveGoodsDialog(flags))));
		p_dlg->setDTS(pData);
		while(ok < 0 && ExecView(p_dlg) == cmOK)
			if(p_dlg->getDTS(pData) > 0)
				ok = 1;
			else
				PPError();
	}
	else
		ok = 1;
	CATCHZOKPPERR
	delete p_dlg;
	return ok;
}

int ViewImageInfo(const char * pImagePath, const char * pInfo, const char * pWarn)
{
	class ImageInfoDialog : public TDialog {
	public:
		explicit ImageInfoDialog(bool simple) : TDialog(simple ? DLG_IMAGEINFO2 : DLG_IMAGEINFO), IsSimple(simple)
		{
			if(IsSimple) {
				SetCtrlResizeParam(CTL_IMAGEINFO_IMAGE, 0, 0, 0, 0, crfResizeable);
				showCtrl(STDCTL_OKBUTTON, false);
				showCtrl(STDCTL_CANCELBUTTON, false);
				ResizeDlgToFullScreen();
			}
		}
		const bool IsSimple;
	};
	int    ok = -1;
	const  bool simple_resizeble = (!pInfo && !pWarn);
	ImageInfoDialog * p_dlg = new ImageInfoDialog(simple_resizeble);
	THROW(CheckDialogPtr(&p_dlg));
	p_dlg->setCtrlData(CTL_IMAGEINFO_IMAGE, (void *)pImagePath);
	if(!simple_resizeble) {
		p_dlg->setStaticText(CTL_IMAGEINFO_INFO, pInfo);
		p_dlg->setStaticText(CTL_IMAGEINFO_WARN, pWarn);
		if(!isempty(pWarn)) {
			p_dlg->SetCtrlBitmap(CTL_IMAGEINFO_PIC_WARN, BM_RED);
			//p_dlg->enableCommand(cmOK, 0);
		}
		else
			p_dlg->showCtrl(CTL_IMAGEINFO_PIC_WARN, false);
	}
	if(ExecView(p_dlg) == cmOK)
		ok = 1;
	CATCHZOKPPERR
	delete p_dlg;
	return ok;
}

int SetupComboByBuddyList(TDialog * pDlg, uint ctlCombo, const ObjIdListFilt & rList)
{
	pDlg->setCtrlLong(ctlCombo, rList.GetSingle());
	if(rList.GetCount() > 1) {
		SetComboBoxListText(pDlg, ctlCombo);
		pDlg->disableCtrl(ctlCombo, true);
	}
	else
		pDlg->disableCtrl(ctlCombo, false);
	return 1;
}

class EditMemosDialog : public PPListDialog {
public:
	EditMemosDialog() : PPListDialog(DLG_MEMOLIST, CTL_LBXSEL_LIST)
	{
		SString title;
		setTitle(PPLoadTextS(PPTXT_EDITMEMOS, title));
	}
	int    setDTS(const SString & rMemos)
	{
		SString buf;
		StringSet ss(PPConst::P_ObjMemoDelim);
		Memos.Z();
		ss.setBuf(rMemos);
		for(uint i = 0, j = 1; ss.get(&i, buf); j++)
			Memos.Add((long)j, 0, buf, 0);
		updateList(-1);
		return 1;
	}
	int    getDTS(SString & rMemos)
	{
		SString buf;
		rMemos.Z();
		for(uint i = 0; i < Memos.getCount(); i++) {
			buf = Memos.Get(i).Txt;
			buf.ReplaceStr(PPConst::P_ObjMemoDelim, "", 0);
			if(i != 0)
				rMemos.Cat(PPConst::P_ObjMemoDelim);
			rMemos.Cat(buf);
		}
		return 1;
	}
private:
	virtual int setupList();
	virtual int addItem(long * pPos, long * pID);
	virtual int editItem(long pos, long id);
	virtual int delItem(long pos, long id);

	StrAssocArray Memos;
};

/*virtual*/int EditMemosDialog::setupList()
{
	int    ok = 1;
	SString buf;
	for(uint i = 0; ok && i < Memos.getCount(); i++) {
		StrAssocArray::Item item = Memos.Get(i);
		(buf = item.Txt).ReplaceChar('\n', ' ').ReplaceChar('\r', ' ');
		if(!addStringToList(item.Id, buf))
			ok = PPErrorZ();
	}
	return ok;
}

/*virtual*/int EditMemosDialog::addItem(long * pPos, long * pID)
{
	int    ok = -1;
	SString buf;
	PPInputStringDialogParam isd_param;
	isd_param.Flags |= PPInputStringDialogParam::fInputMemo;
	isd_param.MaxTextLen = SKILOBYTE(4);
	if(/*InputStringDialog__(0, 0, 0, 1, buf)*/InputStringDialog(isd_param, buf) > 0) {
		Memos.Add(Memos.getCount() + 1, 0, buf, 0);
		ASSIGN_PTR(pPos, Memos.getCount() - 1);
		ASSIGN_PTR(pID,  Memos.getCount());
		ok = 1;
	}
	return ok;
}

/*virtual*/int EditMemosDialog::editItem(long pos, long id)
{
	int    ok = -1;
	SString buf;
	if(Memos.GetText(id, buf) > 0) {
		PPInputStringDialogParam isd_param;
		isd_param.Flags |= PPInputStringDialogParam::fInputMemo;
		isd_param.MaxTextLen = SKILOBYTE(4);
		if(/*InputStringDialog__(0, 0, 0, 1, buf)*/InputStringDialog(isd_param, buf) > 0) {
			Memos.Add(Memos.getCount(), 0, buf);
			ok = 1;
		}
	}
	return ok;
}

/*virtual*/int EditMemosDialog::delItem(long pos, long id)
{
	Memos.Remove(id);
	return 1;
}

int PutObjMemos(PPID objTypeID, PPID prop, PPID objID, const SString & rMemos, int useTa)
{
	int    ok = 1;
	{
		PPTransaction tra(useTa);
		THROW(tra);
		THROW(PPRef->PutPropVlrString(objTypeID, objID, prop, rMemos));
		DS.LogAction(PPACN_OBJEXTMEMOUPD, objTypeID, objID, 0, 0);
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int EditObjMemos(PPID objTypeID, PPID prop, PPID objID)
{
	int    ok = -1;
	SString memos;
	EditMemosDialog * p_dlg = 0;
	PPRef->GetPropVlrString(objTypeID, objID, prop, memos);
	PPInputStringDialogParam isd_param;
	isd_param.Flags |= PPInputStringDialogParam::fInputMemo;
	isd_param.MaxTextLen = SKILOBYTE(4);
	if(!memos.Len() && /*InputStringDialog__(0, 0, 0, 1, memos)*/InputStringDialog(isd_param, memos) > 0) {
		memos.ReplaceStr(PPConst::P_ObjMemoDelim, "", 0);
		ok = 1;
	}
	if(ok == -1 && memos.Len()) {
		THROW(CheckDialogPtr(&(p_dlg = new EditMemosDialog)));
		p_dlg->setDTS(memos);
		if(ExecView(p_dlg) == cmOK) {
			p_dlg->getDTS(memos);
			ok = 1;
		}
	}
	if(ok > 0)
		THROW(PutObjMemos(objTypeID, prop, objID, memos, 1));
	CATCHZOK
	delete p_dlg;
	return ok;
}
//
// TimePickerDialog
//
class TimePickerDialog : public TDialog {
	DECL_DIALOG_DATA(LTIME);
public:
	TimePickerDialog();
	~TimePickerDialog();
	DECL_DIALOG_SETDTS()
	{
		RVALUEPTR(Data, pData);
		return 1;
	}
	DECL_DIALOG_GETDTS()
	{
		ASSIGN_PTR(pData, Data);
		return 1;
	}
private:
	enum {
		dummyFirst = 1,
		clrClear,
		clrText,
		brClear,
		brSelected,
		brSquare,
		brMainRect,
		brWhiteRect,
		penWhite,
		penSquare,
		penBlack,
		penBlue,
		fontTexts,
		fontHours,
		fontWorkHours,
		fontMin,
		fontMin15,
		fontMin30
	};

	DECL_HANDLE_EVENT;
	void   Select(long x, long y);
	void   DrawMainRect(TCanvas *, RECT *);
	void   DrawHourText(TCanvas *);
	void   DrawHoursRect(TCanvas *);
	void   DrawMinutText(TCanvas *);
	void   DrawMinutsRect(TCanvas *);
	void   Implement_Draw();

	struct TimeRects {
		void Init(long hoursLinesCount, long minutsLinesCount, HWND hWnd)
		{
			long   offs = 0L;
			RECT   rect;
			TRect  item;
			GetClientRect(hWnd, &rect);
			{
				long hoursl_count = (hoursLinesCount > 0) ? hoursLinesCount : 2L;
				long hours_in_line = (24 / hoursl_count);
				HourDelta = (rect.right - rect.left - 20) / hours_in_line;
				offs = (rect.right - rect.left - (HourDelta * hours_in_line)) / 2;
				for(long i = 0; i < hoursl_count; i++) {
					int16 top = (i == 0) ? static_cast<int16>(rect.top + 30) : Hours.at(i - 1).b.y;
					item.a.x = static_cast<int16>(rect.left + offs);
					item.a.y = top;
					item.b.x = static_cast<int16>(item.a.x + HourDelta * hours_in_line);
					item.b.y = static_cast<int16>(item.a.y + HourDelta);
					Hours.insert(&item);
				}
				HCellLen = (item.b.x - item.a.x) / hours_in_line;
			}
			{
				long minutsl_count = (minutsLinesCount > 0) ? minutsLinesCount : 1L;
				long minuts_in_line = (12 / minutsl_count);
				MinDelta = (rect.right - rect.left - 20) / minuts_in_line;
				offs = (rect.right - rect.left - (MinDelta * minuts_in_line)) / 2;
				for(long i = 0; i < minutsl_count; i++) {
					int16 top = (i == 0) ? Hours.at(Hours.getCount() - 1).b.y + 25 : Minuts.at(i - 1).b.y;
					item.a.x = static_cast<int16>(rect.left + offs);
					item.a.y = top;
					item.b.x = static_cast<int16>(item.a.x + MinDelta * minuts_in_line);
					item.b.y = static_cast<int16>(item.a.y + MinDelta);
					Minuts.insert(&item);
				}
				MCellLen = (item.b.x - item.a.x) / minuts_in_line;
			}
		}
		int GetTimeByCoord(long x, long y, long * pHour, long * pMinut)
		{
			long   h = -1, m = -1;
			long   hours_in_line = (Hours.getCount()) ? 24 / Hours.getCount()  : 0;
			long   mins_in_line = (Minuts.getCount()) ? 12 / Minuts.getCount() : 0;
			SPoint2S p;
			p.Set(x, y);
			for(uint i = 0; h == -1 && i < Hours.getCount(); i++) {
				TRect item = Hours.at(i);
				item.b.x -= 1;
				if(item.contains(p))
					h = ((p.x - item.a.x) / HCellLen) + i * hours_in_line;
			}
			if(h == -1) {
				for(uint i = 0; m == -1 && i < Minuts.getCount(); i++) {
					TRect item = Minuts.at(i);
					item.b.x -= 1;
					if(item.contains(p))
						m = ((p.x - item.a.x) / MCellLen + i * mins_in_line) * 5;
				}
			}
			if(h != -1) {
				ASSIGN_PTR(pHour, h);
			}
			if(m != -1) {
				ASSIGN_PTR(pMinut, m);
			}
			return 1;
		}

		long HourDelta;
		long MinDelta;
		long HCellLen;
		long MCellLen;
		TSVector <TRect> Hours;
		TSVector <TRect> Minuts;
	};
	TimeRects TmRects;
	SPaintToolBox Ptb;
};

TimePickerDialog::TimePickerDialog() : TDialog(DLG_TMPICKR)
{
	const long def_font_size = 18;
	Ptb.SetColor(clrClear,        RGB(0x20, 0xAC, 0x90));
	Ptb.SetColor(clrText,         GetColorRef(SClrWhite));
	Ptb.SetBrush(brClear,         SPaintObj::psSolid, Ptb.GetColor(clrClear), 0);
	Ptb.SetBrush(brSelected,      SPaintObj::psSolid, GetColorRef(SClrBlack), 0);
	Ptb.SetBrush(brMainRect,      SPaintObj::psSolid, RGB(0xC0, 0xC0, 0xC0), 0);
	Ptb.SetBrush(brWhiteRect,     SPaintObj::psSolid, LightenColor(GetColorRef(SClrGrey), 0.8f), 0);
	Ptb.SetBrush(brSquare,        SPaintObj::psSolid, LightenColor(GetColorRef(SClrAqua), 0.5f), 0);
	Ptb.SetPen(penWhite,          SPaintObj::psSolid, 1, GetColorRef(SClrWhite));
	Ptb.SetPen(penSquare,         SPaintObj::psSolid, 2, GetColorRef(SClrBlue));
	Ptb.SetPen(penBlack,          SPaintObj::psSolid, 1, GetColorRef(SClrBlack));
	Ptb.SetPen(penBlue,           SPaintObj::psSolid, 1, GetColorRef(SClrLightblue));
	{
	 	SString temp_buf;
		LOGFONT log_font;
		MEMSZERO(log_font);
		log_font.lfCharSet = DEFAULT_CHARSET;
		PPGetSubStr(PPTXT_FONTFACE, PPFONT_TIMESNEWROMAN, temp_buf);
		STRNSCPY(log_font.lfFaceName, SUcSwitch(temp_buf));
		log_font.lfHeight = def_font_size;
		Ptb.SetFont(fontHours, ::CreateFontIndirect(&log_font));
		log_font.lfWeight = FW_BOLD;
		Ptb.SetFont(fontWorkHours, ::CreateFontIndirect(&log_font));
		Ptb.SetFont(fontMin, ::CreateFontIndirect(&log_font));
		log_font.lfHeight = 16;
		log_font.lfItalic = 1;
		Ptb.SetFont(fontTexts, ::CreateFontIndirect(&log_font));
	}
	TmRects.Init(3, 2, H());
}

TimePickerDialog::~TimePickerDialog()
{
}

IMPL_HANDLE_EVENT(TimePickerDialog)
{
	TDialog::handleEvent(event);
	if(event.isCmd(cmPaint) || event.isCmd(cmDraw)) {
		Implement_Draw();
	}
	else if(event.isCmd(cmCurTime)) {
		Data = getcurtime_();
		int    m = Data.minut();
		Data = encodetime(Data.hour(), m - (m % 5), 0, 0);
		invalidateRect(getClientRect(), true);
		::UpdateWindow(H());
	}
	else if(TVEVENT == TEvent::evMouseDown) {
		Select(event.mouse.WhereX, event.mouse.WhereY);
		if(event.mouse.doubleClick == 1)
			endModal(cmOK);
	}
	else
		return;
	clearEvent(event);
}

void TimePickerDialog::Select(long x, long y)
{
	long   h = Data.hour();
	long   m = Data.minut();
	TmRects.GetTimeByCoord(x, y, &h, &m);
	Data = encodetime(h, m, 0, 0);
	invalidateRect(getClientRect(), true);
	::UpdateWindow(H());
}

void TimePickerDialog::DrawMainRect(TCanvas * pCanv, RECT * pRect)
{
	TRect m_rect;
	if(TmRects.Minuts.getCount()) {
		m_rect = TmRects.Minuts.at(TmRects.Minuts.getCount() - 1);
		TRect draw_rect(pRect->left + 3, 3, pRect->right - 3, m_rect.b.y + 8);
		// нарисуем прямоугольник
		pCanv->FillRect(draw_rect, static_cast<HBRUSH>(Ptb.Get(brMainRect)));
		// Эффект объемного прямоугольника
		pCanv->SelectObjectAndPush(Ptb.Get(penBlack));
		pCanv->LineHorz(draw_rect.a.x + 1, draw_rect.b.x - 1, draw_rect.a.y + 1);
		pCanv->LineVert(draw_rect.a.x + 1, draw_rect.a.y + 1, draw_rect.b.y - 1);
		pCanv->PopObject();

		pCanv->SelectObjectAndPush(Ptb.Get(penWhite));
		pCanv->LineHorz(draw_rect.a.x + 1, draw_rect.b.x - 1, draw_rect.b.y - 1);
		pCanv->LineVert(draw_rect.b.x -1, draw_rect.a.y + 1, draw_rect.b.y - 1);
		pCanv->PopObject();
	}
}

void TimePickerDialog::DrawHoursRect(TCanvas * pCanv)
{
	if(TmRects.Hours.getCount()) {
		TRect  draw_rect;
		TRect  h1_rect = TmRects.Hours.at(0);
		TRect  h2_rect = TmRects.Hours.at(TmRects.Hours.getCount() - 1);
		long   x1 = h1_rect.a.x - 3;
		long   y1 = h1_rect.a.y - 4;
		long   x2 = h2_rect.b.x + 2;
		long   y2 = h2_rect.b.y + 3;

		pCanv->SelectObjectAndPush(Ptb.Get(penBlue));
		pCanv->SelectObjectAndPush(Ptb.Get(brWhiteRect));
		draw_rect.Set(x1, y1, x2, y2);
		pCanv->Rectangle(draw_rect);
		pCanv->PopObject();
		pCanv->PopObject();

		pCanv->SelectObjectAndPush(Ptb.Get(penBlack));
		pCanv->LineHorz(x1 + 1, x2 - 1, y1 + 1);
		pCanv->LineVert(x1 + 1, y1 + 1, y2 - 1);
		pCanv->PopObject();

		pCanv->SelectObjectAndPush(Ptb.Get(penWhite));
		pCanv->LineHorz(x1 + 1, x2 - 2, y2 - 2);
		pCanv->LineVert(x2 - 2, y1 + 2, y2 - 2);
		pCanv->PopObject();
	}
}

void TimePickerDialog::DrawHourText(TCanvas * pCanv)
{
	if(TmRects.Hours.getCount()) {
		SString temp_buf;
		SPoint2S text_point;
		TRect  h_rect = TmRects.Hours.at(0);
		text_point.Set(h_rect.a.x, 10);
		PPGetWord(PPWORD_HOURS, 1, temp_buf.Z());
		pCanv->SelectObjectAndPush(Ptb.Get(fontTexts));
		pCanv->SetBkColor(Ptb.GetColor(clrClear));
		pCanv->SetTextColor(GetColorRef(SClrBlack));
		pCanv->SetBkTranparent();
		pCanv->TextOut_(text_point, temp_buf);
		pCanv->PopObject();
	}
}

void TimePickerDialog::DrawMinutsRect(TCanvas * pCanv)
{
	if(TmRects.Minuts.getCount()) {
		TRect  draw_rect;
		const TRect  m1_rect = TmRects.Minuts.at(0);
		const TRect  m2_rect = TmRects.Minuts.at(TmRects.Minuts.getCount() - 1);
		const long x1 = m1_rect.a.x - 3;
		const long y1 = m1_rect.a.y - 4;
		const long x2 = m2_rect.b.x + 2;
		const long y2 = m2_rect.b.y + 3;
		pCanv->SelectObjectAndPush(Ptb.Get(penBlue));
		pCanv->SelectObjectAndPush(Ptb.Get(brWhiteRect));
		draw_rect.Set(x1, y1, x2, y2);
		pCanv->Rectangle(draw_rect);
		pCanv->PopObjectN(2);

		pCanv->SelectObjectAndPush(Ptb.Get(penBlack));
		pCanv->LineHorz(x1 + 1, x2 - 1, y1 + 1);
		pCanv->LineVert(x1 + 1, y1 + 1, y2 - 1);
		pCanv->PopObject();

		pCanv->SelectObjectAndPush(Ptb.Get(penWhite));
		pCanv->LineHorz(x1 + 1, x2 - 1, y2 - 1);
		pCanv->LineVert(x2 - 1, y1 + 1, y2 - 1);
		pCanv->PopObject();
	}
}

void TimePickerDialog::DrawMinutText(TCanvas * pCanv)
{
	if(TmRects.Minuts.getCount()) {
		SPoint2S text_point;
		SString temp_buf;
		TRect  m_rect = TmRects.Minuts.at(0);
		text_point.Set(m_rect.a.x, m_rect.a.y - 20);
		PPGetWord(PPWORD_MINUTES, 1, temp_buf.Z());
		pCanv->SelectObjectAndPush(Ptb.Get(fontTexts));
		pCanv->SetBkColor(Ptb.GetColor(clrClear));
		pCanv->SetTextColor(GetColorRef(SClrBlack));
		pCanv->SetBkTranparent();
		pCanv->TextOut_(text_point, temp_buf);
		pCanv->PopObject();
	}
}

void TimePickerDialog::Implement_Draw()
{
	const int h = Data.hour();
	const int m = Data.minut();
	RECT   rect, btn_rect;
	SString temp_buf;
	PAINTSTRUCT ps;
	::BeginPaint(H(), &ps);
	GetClientRect(H(), &rect);
	GetWindowRect(GetDlgItem(H(), STDCTL_OKBUTTON), &btn_rect);
	rect.bottom = btn_rect.top;
	if(ps.fErase) {
		::FillRect(GetDC(H()), &rect, static_cast<HBRUSH>(Ptb.Get(brClear)));
		ps.fErase = 0;
	}
	TCanvas canv(ps.hdc);
	if(TmRects.Hours.getCount()) {
		DrawMainRect(&canv, &rect);
		DrawHourText(&canv);
		DrawHoursRect(&canv);
		long   hours_in_line = (24 / TmRects.Hours.getCount());
		long   h_ = 0;
		for(uint i = 0; i < TmRects.Hours.getCount(); i++) {
			for(long j = 0; j < hours_in_line; j++) {
				uint font_id = fontHours;
				long delta = TmRects.HourDelta;
				long x = TmRects.Hours.at(i).a.x + delta * j;
				long y = TmRects.Hours.at(i).a.y;
				COLORREF color;
				SPoint2S round_pt;
				TRect text_rect(x + 1, y + 1, x + delta - 1, y + delta - 1);
				temp_buf.Z().Cat(h_);
				round_pt.Set(6, 6);
				canv.SelectObjectAndPush(Ptb.Get(penSquare));
				if(h_ == h)
					canv.SelectObjectAndPush(Ptb.Get(brSelected));
				else
					canv.SelectObjectAndPush(Ptb.Get(brSquare));
				canv.Rectangle(text_rect);
				if(h_ >= 8 && h_ <= 20)
					font_id = fontWorkHours;
				color = (h_ == h) ? GetColorRef(SClrWhite) : GetColorRef(SClrBlack);
				canv.SelectObjectAndPush(Ptb.Get(font_id));
				canv.SetBkColor(Ptb.GetColor(clrClear));
				canv.SetTextColor(color);
				canv.SetBkTranparent();
				canv.DrawText_(text_rect, temp_buf, DT_CENTER|DT_VCENTER|DT_SINGLELINE);
				canv.PopObject();
				h_++;
			}
		}
	}
	if(TmRects.Minuts.getCount()) {
		DrawMinutText(&canv);
		DrawMinutsRect(&canv);
		long minuts_in_line = (12 / TmRects.Minuts.getCount());
		long min5 = 0;
		for(uint i = 0; i < TmRects.Minuts.getCount(); i++) {
			for(long j = 0; j < minuts_in_line; j++) {
				uint font_id = fontWorkHours;
				long delta = TmRects.MinDelta;
				long x = TmRects.Minuts.at(i).a.x + delta * j;
				long y = TmRects.Minuts.at(i).a.y;
				COLORREF color;
				SPoint2S round_pt;
				TRect  text_rect(x + 1, y + 1, x + delta - 1, y + delta -1);
				temp_buf.Z().Cat(min5);
				round_pt.Set(6, 6);
				canv.SelectObjectAndPush(Ptb.Get(penSquare));
				if(m >= min5 && m < min5 + 5)
					canv.SelectObjectAndPush(Ptb.Get(brSelected));
				else
					canv.SelectObjectAndPush(Ptb.Get(brSquare));
				canv.Rectangle(text_rect);
				canv.PopObject();
				canv.PopObject();
				font_id = oneof4(min5, 0, 15, 30, 45) ? fontWorkHours : fontHours;
				color = (m >= min5 && m < min5 + 5) ? GetColorRef(SClrWhite) : GetColorRef(SClrBlack);
				canv.SelectObjectAndPush(Ptb.Get(font_id));
				canv.SetBkColor(Ptb.GetColor(clrClear));
				canv.SetTextColor(color);
				canv.SetBkTranparent();
				canv.DrawText_(text_rect, temp_buf, DT_CENTER|DT_VCENTER|DT_SINGLELINE);
				canv.PopObject();
				min5 += 5;
			}
		}
	}
	::EndPaint(H(), &ps);
}

void SetupTimePicker(TDialog * pDlg, uint editCtlID, int buttCtlID)
{
	struct TimeButtonWndEx {
		TimeButtonWndEx(TDialog * pDlg, uint editCtlId, WNDPROC fPrevWndProc) : Dlg(pDlg), EditID(editCtlId), PrevWndProc(fPrevWndProc)
		{
			STRNSCPY(Signature, "papyrusclock");
		}
		static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
		{
			TimeButtonWndEx * p_cbwe = static_cast<TimeButtonWndEx *>(TView::GetWindowUserData(hWnd));
			switch(message) {
				case WM_DESTROY:
					TView::SetWindowProp(hWnd, GWLP_USERDATA, static_cast<void *>(0));
					if(p_cbwe->PrevWndProc)
						TView::SetWindowProp(hWnd, GWLP_WNDPROC, p_cbwe->PrevWndProc);
					delete p_cbwe;
					return 0;
				case WM_LBUTTONUP:
					{
						const bool use_new_calendar = !(APPL->GetUiSettings().Flags & UserInterfaceSettings::fDateTimePickerBefore1124);						
						if(use_new_calendar) 
							SCalendarPicker::Exec(SUiCtrlSupplement::kTime, p_cbwe->Dlg, p_cbwe->EditID, 0);
						else {
							LTIME  tm = p_cbwe->Dlg->getCtrlTime(p_cbwe->EditID);
							TimePickerDialog * dlg = new TimePickerDialog;
							if(CheckDialogPtrErr(&dlg)) {
								dlg->setDTS(&tm);
								if(ExecView(dlg) == cmOK) {
									dlg->getDTS(&tm);
									p_cbwe->Dlg->setCtrlTime(p_cbwe->EditID, tm);
								}
							}
							delete dlg;
						}
					}
					break;
			}
			return CallWindowProc(p_cbwe->PrevWndProc, hWnd, message, wParam, lParam);
		}
		char   Signature[24];
		TDialog * Dlg;
		const  uint EditID;
		WNDPROC PrevWndProc;
	};
	HWND   hwnd = GetDlgItem(pDlg->H(), buttCtlID);
	if(hwnd && pDlg->getCtrlView(editCtlID)) {
		static HBITMAP hbm_clock = 0; // @global @threadsafe
		TimeButtonWndEx * p_cbwe = new TimeButtonWndEx(pDlg, editCtlID, static_cast<WNDPROC>(TView::GetWindowProp(hwnd, GWLP_WNDPROC)));
		TView::SetWindowProp(hwnd, GWLP_USERDATA, p_cbwe);
		TView::SetWindowProp(hwnd, GWLP_WNDPROC, TimeButtonWndEx::WndProc);
		if(!hbm_clock) {
			ENTER_CRITICAL_SECTION
			SETIFZ(hbm_clock, APPL->LoadBitmap_(IDB_CLOCK));
			LEAVE_CRITICAL_SECTION
		}
		::SendMessage(hwnd, BM_SETIMAGE, IMAGE_BITMAP, reinterpret_cast<LPARAM>(hbm_clock));
	}
}
#if 0 // @v11.2.4 {
//
// TimePickerCtrlGroup
//
TimePickerCtrlGroup::TimePickerCtrlGroup(uint ctl, uint ctlSel, TDialog * pDlg) : Ctl(ctl), CtlSel(ctlSel), Cmd(0)
{
	if(CtlSel && pDlg) {
		TButton * p_btn = static_cast<TButton *>(pDlg->getCtrlView(CtlSel));
		if(p_btn) {
			Cmd = p_btn->GetCommand();
			p_btn->SetBitmap(IDB_CLOCK);
		}
	}
}

TimePickerCtrlGroup::~TimePickerCtrlGroup()
{
}

int TimePickerCtrlGroup::Edit(TDialog * pDlg)
{
	int    ok = -1;
	TimePickerDialog * dlg = new TimePickerDialog;
	getData(pDlg, &Data);
	if(CheckDialogPtrErr(&dlg) && dlg->setDTS(&Data)) {
		while(ok <= 0 && ExecView(dlg) == cmOK)
			if(dlg->getDTS(&Data)) {
				setData(pDlg, &Data);
				ok = 1;
			}
			else
				PPError();
	}
	else
		ok = 0;
	delete dlg;
	return ok;
}

/*virtual*/int TimePickerCtrlGroup::setData(TDialog * pDlg, void * pData)
{
	Data = pData ? *static_cast<const LTIME *>(pData) : ZEROTIME;
	pDlg->setCtrlData(Ctl, &Data);
	return 1;
}

/*virtual*/int TimePickerCtrlGroup::getData(TDialog * pDlg, void * pData)
{
	int    ok = 0;
	pDlg->getCtrlData(Ctl, &Data);
	if(checktime(Data) > 0) {
		ASSIGN_PTR(static_cast<LTIME *>(pData), Data);
		ok = 1;
	}
	return ok;
}

/*virtual*/void TimePickerCtrlGroup::handleEvent(TDialog * pDlg, TEvent & event)
{
	if(pDlg) {
		if(event.isCmd(Cmd)) {
			Edit(pDlg);
			pDlg->clearEvent(event);
		}
	}
}

int SetupTimePicker(uint ctl, uint ctlSel, uint grpID, TDialog * pDlg)
{
	return pDlg ? pDlg->addGroup(grpID, new TimePickerCtrlGroup(ctl, ctlSel, pDlg)) : 0;
}
#endif // } 0 @v11.2.4
//
//
//
EmailCtrlGroup::Rec::Rec(StrAssocArray * pAddrList)
{
	RVALUEPTR(AddrList, pAddrList);
}

EmailCtrlGroup::EmailCtrlGroup(uint ctl, uint cm) : Ctl(ctl), Cm(cm)
{
}

EmailCtrlGroup::~EmailCtrlGroup()
{
}

int EmailCtrlGroup::setData(TDialog * pDlg, void * pData)
{
	const Rec * p_rec = static_cast<const Rec *>(pData);
	RVALUEPTR(Data, p_rec);
	SetLine(pDlg);
	return 1;
}

int EmailCtrlGroup::getData(TDialog * pDlg, void * pData)
{
	SString buf, addr_list;
	CALLPTRMEMB(pDlg, getCtrlString(Ctl, addr_list));
	Rec * p_rec = static_cast<Rec *>(pData);
	if(p_rec) {
		StringSet ss(";");
		p_rec->AddrList.Z();
		if(addr_list.NotEmptyS())
			ss.setBuf(addr_list, addr_list.Len() + 1);
		for(uint p = 0; ss.get(&p, buf);)
			p_rec->AddrList.Add(p_rec->AddrList.getCount() + 1, buf, 0);
	}
	return 1;
}

void EmailCtrlGroup::SetLine(TDialog * pDlg)
{
	SString addr_list;
	const uint alcnt = Data.AddrList.getCount();
	for(uint i = 0; i < alcnt; i++) {
		addr_list.Cat(Data.AddrList.Get(i).Txt);
		if((i + 1) < alcnt)
			addr_list.Semicol();
	}
	if(pDlg) {
		pDlg->SetupWordSelector(Ctl, (oneof2(alcnt, 0, 1) ? new TextHistorySelExtra("email-common") : 0), 0, 2, WordSel_ExtraBlock::fFreeText);
		pDlg->setCtrlString(Ctl, addr_list);
		pDlg->disableCtrl(Ctl, Data.AddrList.getCount() > 1);
	}
}

class EmailListDlg : public PPListDialog {
	DECL_DIALOG_DATA(StrAssocArray);
public:
	EmailListDlg() : PPListDialog(DLG_EMAILADDRS, CTL_EMAILADDRS_LIST)
	{
		showCtrl(CTL_LBXSEL_UPBTN,   false);
		showCtrl(CTL_LBXSEL_DOWNBTN, false);
		updateList(-1);
	}
	DECL_DIALOG_SETDTS()
	{
		RVALUEPTR(Data, pData);
		updateList(-1);
		return 1;
	}
	DECL_DIALOG_GETDTS()
	{
		ASSIGN_PTR(pData, Data);
		return 1;
	}
protected:
	virtual int setupList();
	virtual int addItem(long * pPos,  long * pID);
	virtual int editItem(long pos, long id);
	virtual int delItem(long pos,  long id);
};

// AHTOXA {
int EmailListDlg::setupList()
{
	int    ok = 1;
	for(uint i = 0; i < Data.getCount(); i++) {
		if(!addStringToList(Data.Get(i).Id, Data.Get(i).Txt))
			ok = PPErrorZ();
	}
	return ok;
}

static int IsEmailAddr(const char * pPath)
{
	return BIN(pPath && sstrchr(pPath, '@'));
}

int EmailListDlg::addItem(long * pPos, long * pID)
{
	int    ok = -1;
	long   pos = -1;
	long   id = -1;
	SString addr;
	SString title;
	PPLoadString("email", title);
	PPInputStringDialogParam isd_param(title, title);
	if(/*InputStringDialog__(title, title, 0, 0, addr)*/InputStringDialog(isd_param, addr) > 0 && IsEmailAddr(addr)) {
		if((ok = Data.Add(Data.getCount() + 1, addr, 0)) > 0) {
			pos = Data.getCount() - 1;
			id  = Data.getCount();
		}
	}
	ASSIGN_PTR(pPos, pos);
	ASSIGN_PTR(pID,  id);
	return ok;
}

int EmailListDlg::editItem(long pos, long id)
{
	int    ok = -1;
	if(pos >= 0 && pos < static_cast<long>(Data.getCount())) {
		SString title;
		SString addr(Data.Get(pos).Txt);
		PPLoadString("email", title);
		PPInputStringDialogParam isd_param(title, title);
		if(/*InputStringDialog__(title, title, 0, 0, addr)*/InputStringDialog(isd_param, addr) > 0 && IsEmailAddr(addr)) {
			Data.Add(id, addr);
			ok = 1;
		}
	}
	return ok;
}

int EmailListDlg::delItem(long pos, long id)
{
	int    ok = -1;
	if(pos >= 0 && pos < static_cast<long>(Data.getCount())) {
		Data.Remove(id);
		ok = 1;
	}
	return ok;
}

int EmailCtrlGroup::Edit(TDialog * pDlg)
{
	int    ok = -1;
	EmailListDlg * p_dlg = new EmailListDlg;
	THROW(CheckDialogPtr(&p_dlg));
	p_dlg->setDTS(&Data.AddrList);
	for(int valid_data = 0; !valid_data && ExecView(p_dlg) == cmOK;) {
		if(p_dlg->getDTS(&Data.AddrList) > 0) {
			ok = valid_data = 1;
			SetLine(pDlg);
		}
		else
			PPError();
	}
	CATCHZOKPPERR
	delete p_dlg;
	return ok;
}

void EmailCtrlGroup::handleEvent(TDialog * pDlg, TEvent & event)
{
	if(pDlg) {
		if(event.isCmd(Cm)) {
			getData(pDlg, &Data);
			pDlg->disableCtrl(Ctl, Data.AddrList.getCount() > 1);
			Edit(pDlg);
			pDlg->clearEvent(event);
		}
	}
}
//
//
//
EmailToBlock::EmailToBlock() : MailAccID(0)
{
}

int EmailToBlock::Edit(long flags)
{
	class EmailToBlockDialog : public TDialog {
		DECL_DIALOG_DATA(EmailToBlock);
	public:
		explicit EmailToBlockDialog(long flags) : TDialog(DLG_MAILTO), Flags(0)
		{
			addGroup(ctlgroupEmailList, new EmailCtrlGroup(CTL_MAILTO_ADDR, cmEMailList));
		}
		DECL_DIALOG_SETDTS()
		{
			int    ok = 1;
			RVALUEPTR(Data, pData);
			SetupPPObjCombo(this, CTLSEL_MAILTO_ACCNT, PPOBJ_INTERNETACCOUNT, Data.MailAccID, OLW_CANEDIT|OLW_CANINSERT,
				reinterpret_cast<void *>(PPObjInternetAccount::filtfMail));
			setCtrlString(CTL_MAILTO_SUBJ, Data.Subj);
			{
				EmailCtrlGroup::Rec grp_rec(&Data.AddrList);
				setGroupData(ctlgroupEmailList, &grp_rec);
			}
			return ok;
		}
		DECL_DIALOG_GETDTS()
		{
			int    ok = 1;
			getCtrlData(CTLSEL_MAILTO_ACCNT, &Data.MailAccID);
			getCtrlString(CTL_MAILTO_SUBJ, Data.Subj);
			{
				EmailCtrlGroup::Rec grp_rec;
				getGroupData(ctlgroupEmailList, &grp_rec);
				Data.AddrList = grp_rec.AddrList;
			}
			ASSIGN_PTR(pData, Data);
			return ok;
		}
	private:
		enum {
			ctlgroupEmailList = 1
		};
		long   Flags;
	};
    DIALOG_PROC_BODY_P1(EmailToBlockDialog, 0, this);
}
//
//
//
SendMailDialog::Rec::Rec() : MailAccID(0), Delay(0)
{
}

SendMailDialog::Rec & FASTCALL SendMailDialog::Rec::operator = (const SendMailDialog::Rec & rSrc)
{
	MailAccID = rSrc.MailAccID;
	Delay = rSrc.Delay;
	Subj = rSrc.Subj;
	Text = rSrc.Text;
	AddrList = rSrc.AddrList;
	char * p_item = 0;
	for(uint i = 0; rSrc.FilesList.enumItems(&i, (void **)&p_item);)
		FilesList.insert(newStr(p_item));
	return *this;
}

SendMailDialog::SendMailDialog() : PPListDialog(DLG_SENDMAIL, CTL_SENDMAIL_ATTACHLIST)
{
	addGroup(ctlgroupEmailList, new EmailCtrlGroup(CTL_SENDMAIL_ADDR, cmEMailList));
	updateList(-1);
}

int SendMailDialog::setupList()
{
	int    ok = 1;
	SString temp_buf;
	char * p_path = 0;
	for(uint i = 0; ok && Data.FilesList.enumItems(&i, (void **)&p_path);) {
		(temp_buf = p_path).Transf(CTRANSF_OUTER_TO_INNER);
		if(!addStringToList(i, temp_buf))
			ok = PPErrorZ();
	}
	return ok;
}

/*virtual*/int SendMailDialog::addItem(long * pPos, long * pId)
{
	int    ok = -1;
	long   pos = -1, id = -1;
	SString path;
	if(PPOpenFile(PPTXT_FILPAT_ALL, path, 0, 0) > 0) {
		if(!Data.FilesList.lsearch(path.cptr(), 0, PTR_CMPFUNC(PcharNoCase))) {
			Data.FilesList.insert(newStr(path));
			id = pos = Data.FilesList.getCount() - 1;
			ok = 1;
		}
		else
			PPError(PPERR_ITEMALREADYEXISTS);
	}
	ASSIGN_PTR(pPos, pos);
	ASSIGN_PTR(pId, id);
	return ok;
}

/*virtual*/int SendMailDialog::editItem(long pos, long id)
{
	int    ok = -1;
	if(pos >= 0 && pos < Data.FilesList.getCountI()) {
		SString path(Data.FilesList.at(pos));
		if(PPOpenFile(PPTXT_FILPAT_ALL, path,  0, 0) > 0) {
			uint p = 0;
			if(Data.FilesList.lsearch(path.cptr(), &p, PTR_CMPFUNC(PcharNoCase)) && p != pos)
				PPError(PPERR_ITEMALREADYEXISTS);
			else {
				Data.FilesList.atFree(pos);
				Data.FilesList.atInsert(pos, newStr(path));
				ok = 1;
			}
		}
	}
	return ok;
}

/*virtual*/int SendMailDialog::delItem(long pos, long id)
{
	int    ok = -1;
	if(pos >= 0 && pos < Data.FilesList.getCountI()) {
		Data.FilesList.atFree(pos);
		ok = 1;
	}
	return ok;
}

int SendMailDialog::setDTS(const Rec * pData)
{
	RVALUEPTR(Data, pData);
	SetupPPObjCombo(this, CTLSEL_SENDMAIL_ACCNT, PPOBJ_INTERNETACCOUNT, Data.MailAccID, OLW_CANEDIT|OLW_CANINSERT,
		reinterpret_cast<void *>(PPObjInternetAccount::filtfMail));
	setCtrlString(CTL_SENDMAIL_SUBJ, Data.Subj);
	setCtrlString(CTL_SENDMAIL_TEXT, Data.Text);
	{
		EmailCtrlGroup::Rec grp_rec(&Data.AddrList);
		setGroupData(ctlgroupEmailList, &grp_rec);
	}
	{
		int32  delay_sec = (int32)(Data.Delay / 1000);
		setCtrlData(CTL_SENDMAIL_DELAY, &delay_sec);
	}
	updateList(-1);
	return 1;
}

int SendMailDialog::getDTS(Rec * pData)
{
	int    ok = 1;
	uint   sel = 0;
	getCtrlData(sel = CTLSEL_SENDMAIL_ACCNT, &Data.MailAccID);
	THROW_PP(Data.MailAccID, PPERR_INETACCOUNTNOTDEF);
	getCtrlString(CTL_SENDMAIL_SUBJ, Data.Subj);
	getCtrlString(CTL_SENDMAIL_TEXT, Data.Text);
	{
		EmailCtrlGroup::Rec grp_rec;
		getGroupData(ctlgroupEmailList, &grp_rec);
		Data.AddrList = grp_rec.AddrList;
	}
	{
		int32  delay_sec = 0;
		getCtrlData(sel = CTL_SENDMAIL_DELAY, &delay_sec);
		THROW_PP(delay_sec >= 0 && delay_sec <= 3600, PPERR_USERINPUT);
		Data.Delay = delay_sec * 1000;
	}
	ASSIGN_PTR(pData, Data);
	CATCH
		selectCtrl(sel);
		ok = 0;
	ENDCATCH
	return ok;
}
//
//
//
int PPCallHelp(void * hWnd, uint cmd, uint ctx)
{
	int    ok = 0;
	if(oneof3(cmd, HH_HELP_CONTEXT, HH_INITIALIZE, HH_UNINITIALIZE)) {
		SString path;
		if(cmd == HH_HELP_CONTEXT) {
			const char * p_file_name = "pphelp.chm";
			PPGetFilePath(PPPATH_BIN, p_file_name, path);
			if(fileExists(path)) {
				SFsPath ps(path);
				if(ps.Flags & SFsPath::fUNC || ((ps.Flags & SFsPath::fDrv) && GetDriveType(SUcSwitch(ps.Drv.SetLastSlash())) == DRIVE_REMOTE)) {
					//
					// В связи с проблемой загрузки help'а с сетевого каталога коприруем файл в
					// локальный каталог. При этом, чтобы избежать лишних копирований, проверяем, нет ли
					// в локальном каталоге скопированного до этого актуального файла.
					//
					SString local_path;
					// @v11.8.5 if(SFile::GetSysDir(SFile::sdAppDataLocal, local_path)) {
					if(GetKnownFolderPath(UED_FSKNOWNFOLDER_LOCAL_APP_DATA, local_path)) { // @v11.8.5
						int    do_copy = 0;
						SFile::Stat st_local, st;
						local_path.SetLastSlash().Cat(SLS.GetAppName());
						if(!fileExists(local_path))
							SFile::CreateDir(local_path);
						local_path.SetLastSlash().Cat(p_file_name);
						if(!fileExists(local_path))
							do_copy = 1;
						else if(SFile::GetStat(local_path, 0, &st_local, 0) && SFile::GetStat(path, 0, &st, 0))
							if(st_local.ModTm_ < st.ModTm_)
								do_copy = 1;
							else
								path = local_path; // В локальном каталоге уже лежит актуальный файл: недо использовать его.
						else
							do_copy = 1;
						if(do_copy && copyFileByName(path, local_path))
							path = local_path;
					}
				}
				ok = BIN(HtmlHelp(static_cast<HWND>(hWnd), SUcSwitch(path), cmd, ctx));
			}
		}
		else
			ok = BIN(HtmlHelp(0, 0, cmd, ctx));
	}
	return ok;
}
//
//
//
static BOOL CALLBACK GetChildWindowsList(HWND hWnd, LPARAM lp)
{
	TSVector <HWND> * p_list = reinterpret_cast<TSVector <HWND> *>(lp);
	CALLPTRMEMB(p_list, insert(&hWnd));
	return TRUE;
}

static bool TakeInCountCtrl(HWND hWnd, const TSVector <HWND> & rCtrlList, LongArray & rSeenPosList)
{
	for(uint i = 0; i < rCtrlList.getCount(); i++) {
		if(rCtrlList.at(i) == hWnd) {
			rSeenPosList.addUnique(i);
			return true;
		}
	}
	return false;
}

static SString & _RectToLine(const RECT & rRect, SString & rBuf)
{
	return rBuf.CatChar('(').Cat(rRect.left).CatDiv(',', 2).Cat(rRect.top).CatDiv(',', 2).
		Cat(rRect.right).CatDiv(',', 2).Cat(rRect.bottom).CatChar(')');
}

static RECT & _AdjustRectToOrigin(RECT & rRect, const SPoint2I & rOrigin)
{
	rRect.left -= rOrigin.x;	
	rRect.right -= rOrigin.x;	
	rRect.top -= rOrigin.y;
	rRect.bottom -= rOrigin.y;
	return rRect;
}

static SString & _BBox(const RECT & rRect, SString & rBuf, int subType/*0 - no, 1 - label*/)
{
	//rBuf.Cat("bbox").CatDiv(':', 2);
	//return _RectToLine(rRect, rBuf);
	//
	const char * p_org_text = (subType == 1) ? "labelorigin" : "origin";
	const char * p_sz_text = (subType == 1) ? "labelsize" : "size";
	rBuf.Cat(p_org_text).CatDiv(':', 2).CatChar('(').Cat(rRect.left).CatDiv(',', 2).Cat(rRect.top).CatChar(')').Space();
	rBuf.Cat(p_sz_text).CatDiv(':', 2).CatChar('(').Cat(rRect.right-rRect.left).CatDiv(',', 2).Cat(rRect.bottom-rRect.top).CatChar(')').Space();
	return rBuf;
}

static SString & PreprocessCtrlText(const SString & rSrcText, SString & rResult)
{
	if(rSrcText.NotEmpty()) {
		SString subst;
		if(SLS.SubstString(rSrcText, 1, subst) > 0)
			rResult = subst;
		else {
			rResult = rSrcText;
			SLS.ExpandString(rResult, CTRANSF_UTF8_TO_OUTER);
		}
	}
	else
		rResult = rSrcText;
	rResult.ReplaceStr("&", "", 0);
	return rResult;
}

int ExportDialogs2(const char * pFileName)
{
	int    ok = 1;
	uint   res_id = 0;
	ulong  res_pos = 0;
	SString line_buf;
	SString symb;
	SString temp_buf;
	SString ctl_text;
	SString ctl_text_processed;
	SString label_text;
	SString text_line_buf; // Буфер вывода строк из диалогов
	SString cls_name;
	SString dlg_title_buf;
	SString dlg_symb_body;
	TDialog * dlg = 0;
	StrAssocArray prop_list;
	TSVector <HWND> child_list;
	LongArray seen_pos_list;
	SFile  f_out(pFileName, SFile::mWrite);
	{
		SFsPath ps(pFileName);
		ps.Nam.CatChar('-').Cat("text");
		ps.Ext = "tsv";
		ps.Merge(temp_buf);
	}
	SFile f_out_text(temp_buf, SFile::mWrite);
	{
		SFsPath ps(pFileName);
		ps.Nam.CatChar('-').Cat("manual");
		ps.Ext = "tex";
		ps.Merge(temp_buf);
	}
	SFile f_out_manual(temp_buf, SFile::mWrite);
	while(P_SlRez->enumResources(TV_DIALOG, &res_id, &res_pos) > 0) {
		prop_list.Z();
		WINDOWINFO wi;
		dlg = new TDialog(res_id, TDialog::coExport);
		if(CheckDialogPtr(&dlg)) {
			SPoint2I _origin;
			seen_pos_list.Z();
			child_list.clear();
			dlg_symb_body.Z();
			prop_list.Z();
			dlg->GetCtlSymb(-1000, symb);
			if(symb.IsEmpty()) {
				dlg_symb_body.Z().Cat(dlg->GetId());
				symb.Z().Cat("DLG").CatChar('_').Cat(dlg_symb_body);
			}
			else
				TDialog::GetSymbolBody(symb, dlg_symb_body);
			TView::SGetWindowText(dlg->H(), dlg_title_buf);
			{
				text_line_buf.Z().Cat(symb).Tab().Cat(dlg_title_buf).CR();
				f_out_text.WriteLine(text_line_buf);
			}
			line_buf.Z().CR().Cat("dialog").Space().Cat(symb).Space().CatChar('[');
			{
				line_buf.Cat("title").CatDiv(':', 2).CatQStr((temp_buf = dlg_title_buf).Transf(CTRANSF_OUTER_TO_UTF8));
				INITWINAPISTRUCT(wi);
				if(GetWindowInfo(dlg->H(), &wi)) {
					_BBox(wi.rcWindow, line_buf.Space(), 0);
					_origin.Set(wi.rcClient.left, wi.rcClient.top);
				}
				{
					HFONT h_f = reinterpret_cast<HFONT>(::SendMessage(dlg->H(), WM_GETFONT, 0, 0));
					if(h_f) {
						temp_buf.Z();
						LOGFONT f;
						if(::GetObject(h_f, sizeof(f), &f)) {
							SFontDescr fd(0, 0, 0);
							fd.SetLogFont(&f);
							fd.Size = (int16)MulDiv(fd.Size, 72, GetDeviceCaps(SLS.GetTLA().GetFontDC(), LOGPIXELSY));
							temp_buf.Cat(fd.Face).CatDiv(',', 0).Cat(fd.Size);
							/*if(!fd.Face.IsEqiAscii("MS Sans Serif") || fd.Size != 8 || fd.Flags || fd.Weight != 0.0f) {
								fd.ToStr(temp_buf, 0);
							}*/
						}
						if(temp_buf.NotEmptyS()) {
							line_buf.Space().Cat("font").CatDiv(':', 2).CatQStr(temp_buf);
							//prop_list.Add(DlScope::cuifFont, temp_buf.Quot('\"', '\"'));
						}
					}
				}
				//DlScope::PropListToLine(prop_list, 1, line_buf).CR();
			}
			line_buf.CatChar(']');
			line_buf.Space().CatChar('{').CR();
			f_out.WriteLine(line_buf);
			{
				// %topic(DLG_BILLSTATUS)
                line_buf.Z().Cat("%topic").CatParStr(dlg_symb_body).CR();
                f_out_manual.WriteLine(line_buf);
                // \ppypict{dlg-billstatus}{Диалог редактирования статуса документов}
                PreprocessCtrlText(dlg_title_buf, ctl_text_processed);
                (temp_buf = dlg_symb_body).ReplaceStr("_", "-", 0).ToLower();
                line_buf.Z().BSlash().Cat("ppypict").CatChar('{').Cat(temp_buf).CatChar('}');
				line_buf.CatChar('{').Cat(ctl_text_processed).CatChar('}').CR();
				f_out_manual.WriteLine(line_buf);
				// \begin{description}
				line_buf.Z().BSlash().Cat("begin").CatChar('{').Cat("description").CatChar('}').CR();
				f_out_manual.WriteLine(line_buf);
			}
			if(EnumChildWindows(dlg->H(), GetChildWindowsList, reinterpret_cast<LPARAM>(&child_list))) {
				for(uint i = 0; i < child_list.getCount(); i++) {
					if(!seen_pos_list.lsearch(static_cast<long>(i))) {
						prop_list.Z();
						line_buf.Z();
						label_text.Z();
						const HWND h = child_list.at(i);
						int   ctl_id = GetDlgCtrlID(h);
						INITWINAPISTRUCT(wi);
						if(GetWindowInfo(h, &wi)) {
							TView::SGetWindowText(h, ctl_text);
							ctl_text.ReplaceStr("\n", 0, 0);
							TView * p_view = dlg->getCtrlView(ctl_id);
							if(!p_view && ctl_id > 4096) {
								ctl_id -= 4096;
								p_view = dlg->getCtrlView(ctl_id);
							}
							symb.Z(); //p_view ? p_view->GetSymb() : 0;
							dlg->GetCtlSymb(ctl_id, symb);
							if(symb.IsEmpty())
								symb.Cat(ctl_id);
							TLabel * p_label = dlg->GetCtrlLabel(ctl_id);
							RECT   label_rect;
							if(p_label) {
								WINDOWINFO label_wi;
								INITWINAPISTRUCT(label_wi);
								if(GetWindowInfo(p_label->getHandle(), &label_wi)) {
									TView::SGetWindowText(p_label->getHandle(), label_text);
									label_text.ReplaceStr("\n", 0, 0);
									label_rect = _AdjustRectToOrigin(label_wi.rcWindow, _origin);
									if(label_text.NotEmpty()) {
										text_line_buf.Z().Cat(symb).Tab().Cat(label_text).CR();
										f_out_text.WriteLine(text_line_buf);
									}
								}
								else
									p_label = 0;
							}
							else if(ctl_text.NotEmpty() && symb != "0" && symb.ToLong() == 0) {
								text_line_buf.Z().Cat(symb).Tab().Cat(ctl_text).CR();
								f_out_text.WriteLine(text_line_buf);
							}
							TView::SGetWindowClassName(h, cls_name);
							if(cls_name.IsEqiAscii("Edit")) {
								if(TView::IsSubSign(p_view, TV_SUBSIGN_INPUTLINE)) {
									TInputLine * p_il = static_cast<TInputLine *>(p_view);
									if(p_label)
										ctl_text = label_text;
									ComboBox * p_cb = p_il->GetCombo();
									if(p_cb) {
										// T_COMBOBOX T_IDENT T_CONST_STR uirectopt uictrl_type uictrl_properties ';'
										HWND   h_combo = p_cb->getHandle();
										WINDOWINFO wi_combo;
										INITWINAPISTRUCT(wi_combo);
										if(GetWindowInfo(h_combo, &wi_combo)) {
											dlg->GetCtlSymb(p_cb->GetId(), symb);
											line_buf.Tab().Cat("combobox").Space().Cat(symb).Space().CatChar('[');
											_BBox(_AdjustRectToOrigin(wi.rcWindow, _origin), line_buf.Space(), 0);
											if(p_label) {
												line_buf.Cat("label").CatDiv(':', 2).CatQStr((temp_buf = ctl_text).Transf(CTRANSF_OUTER_TO_UTF8));
												_BBox(label_rect, line_buf.Space(), 1/*subType*/);
											}
											//DlScope::PropListToLine(prop_list, 2, line_buf).Semicol().CR();
											{
												const TInputLine * p_cb_line = p_cb->GetLink();
												if(p_cb_line) {
													dlg->GetCtlSymb(p_cb_line->GetId(), symb);
													if(symb.NotEmpty()) {
														line_buf.Cat("cblinesymb").CatDiv(':', 2).Cat(symb).Space();
													}
												}
											}
											line_buf.CatChar(']');
											line_buf.Semicol();
											f_out.WriteLine(line_buf.CR());
											TakeInCountCtrl(h_combo, child_list, seen_pos_list);
										}
										{
											// \item[\dlgcombo{Счетчик}]
											PreprocessCtrlText(ctl_text, ctl_text_processed);
											line_buf.Z().Tab().BSlash().Cat("item").CatChar('[').BSlash().Cat("dlgcombo").
												CatChar('{').Cat(ctl_text_processed).CatChar('}').CatChar(']').CR().CR();
											f_out_manual.WriteLine(line_buf);
										}
									}
									else {
										/*if(wi.dwStyle & ES_READONLY) {
											prop_list.Add(DlScope::cuifReadOnly, temp_buf.Z());
										}*/
										const TYPEID _type_id = p_il->getType();
										line_buf.Tab().Cat("input").Space().Cat(symb).Space().CatChar('[');
										_BBox(_AdjustRectToOrigin(wi.rcWindow, _origin), line_buf, 0);
										{
											static constexpr SIntToSymbTabEntry ws_symb_list[] = {
												{ WS_TABSTOP, "tabstop" },
												{ WS_DISABLED, "disabled" },
												{ ES_READONLY, "readonly" },
												{ ES_PASSWORD, "password" },
												{ ES_MULTILINE, "multiline" },
												{ ES_WANTRETURN, "wantreturn" },
											};
											for(uint i = 0; i < SIZEOFARRAY(ws_symb_list); i++) {
												if(wi.dwStyle & ws_symb_list[i].Id)
													line_buf.Space().Cat(ws_symb_list[i].P_Symb);
											}
										}
										{
											long il_format = p_il->getFormat();
											if(il_format) {
												if(il_format & STRF_PASSWORD && wi.dwStyle & ES_PASSWORD) {
													il_format &= ~STRF_PASSWORD;
												}
												if(il_format) {
													StringSet ss_format;
													SFormat_TranslateFlagsToStringSet(il_format, _type_id, ss_format);
													if(ss_format.getCount()) {
														line_buf.Space().Cat("fmtf").CatDiv(':', 2).CatChar('(');
														for(uint ssp = 0, ss_count = 0; ss_format.get(&ssp, temp_buf); ss_count++) {
															if(ss_count)
																line_buf.Space();
															line_buf.Cat(temp_buf);
														}
														line_buf.CatChar(')');
													}
													if(stbase(_type_id) == BTS_REAL) {
														line_buf.Space().Cat("fmtprec").CatDiv(':', 2).Cat(SFMTPRC(il_format));
													}
												}
											}
										}
										if(ctl_text.NotEmpty())
											line_buf.Space().Cat("label").CatDiv(':', 2).CatQStr((temp_buf = ctl_text).Transf(CTRANSF_OUTER_TO_UTF8));
										if(p_label) {
											_BBox(label_rect, line_buf.Space(), 1/*subType*/);
										}
										line_buf.CatChar(']');
										GetBinaryTypeString(_type_id, 1, temp_buf, "", 0);
										if(temp_buf.Cmp("unknown", 0) == 0) {
											(temp_buf = "string").CatBrackStr("48");
										}
										line_buf.Space().Cat(temp_buf);
										line_buf.Semicol();
										//DlScope::PropListToLine(prop_list, 2, line_buf).Semicol().CR();
										f_out.WriteLine(line_buf.CR());
										{
											// \item[Наименование]
											PreprocessCtrlText(ctl_text, ctl_text_processed);
											line_buf.Z().Tab().BSlash().Cat("item").CatBrackStr(ctl_text_processed).CR().CR();
											f_out_manual.WriteLine(line_buf);
										}
									}
									if(p_label)
										TakeInCountCtrl(p_label->getHandle(), child_list, seen_pos_list);
									TakeInCountCtrl(h, child_list, seen_pos_list);
								}
							}
							else if(cls_name.IsEqiAscii("Button")) {
								const int bt = (wi.dwStyle & BS_TYPEMASK);
								if(oneof2(bt, BS_CHECKBOX, BS_AUTOCHECKBOX)) {
									if(TView::IsSubSign(p_view, TV_SUBSIGN_CLUSTER)) {
										line_buf.Tab().Cat("checkbox").Space().Cat(symb).Space().CatChar('[');
										line_buf.Cat("title").CatDiv(':', 2).CatQStr((temp_buf = ctl_text).Transf(CTRANSF_OUTER_TO_UTF8));
										if(wi.dwStyle & WS_TABSTOP)
											line_buf.Space().Cat("tabstop");
										if(wi.dwStyle & WS_DISABLED)
											line_buf.Space().Cat("disabled");
										_BBox(_AdjustRectToOrigin(wi.rcWindow, _origin), line_buf.Space(), 0);
										line_buf.CatChar(']').Semicol();
										f_out.WriteLine(line_buf.CR());
										TakeInCountCtrl(h, child_list, seen_pos_list);
									}
								}
								else if(oneof2(bt, BS_RADIOBUTTON, BS_AUTORADIOBUTTON)) {
								}
								else if(bt == BS_GROUPBOX) {
									if(TView::IsSubSign(p_view, TV_SUBSIGN_CLUSTER)) {
										TCluster * p_clu = static_cast<TCluster *>(p_view);
										const char * p_kind = 0;
										if(p_clu->GetKind() == RADIOBUTTONS) {
											p_kind = "radiocluster";
											{
												// \item[\dlgradioc{Сортировать по}]
												PreprocessCtrlText(ctl_text, ctl_text_processed);
												line_buf.Z().Tab().BSlash().Cat("item").CatChar('[').BSlash().Cat("dlgradioc").
													CatChar('{').Cat(ctl_text_processed).CatChar('}').CatChar(']').CR().CR();
												f_out_manual.WriteLine(line_buf);
											}
										}
										else if(p_clu->GetKind() == CHECKBOXES) {
											p_kind = "checkcluster";
										}
										if(p_kind) {
											line_buf.Z().Tab().Cat(p_kind).Space().Cat(symb).Space().CatChar('[');
											if(ctl_text.NotEmpty())
												line_buf.Cat("title").CatDiv(':', 2).CatQStr((temp_buf = ctl_text).Transf(CTRANSF_OUTER_TO_UTF8)).Space();
											_BBox(_AdjustRectToOrigin(wi.rcWindow, _origin), line_buf, 0);
											line_buf.CatChar(']');
											line_buf.Space().CatChar('{');
											f_out.WriteLine(line_buf.CR());
											//
											for(uint j = 0; j < p_clu->getNumItems(); j++) {
												int    button_id = MAKE_BUTTON_ID(ctl_id, j+1);
												HWND   h_item = GetDlgItem(dlg->H(), button_id);
												if(h_item) {
													WINDOWINFO wi_item;
													INITWINAPISTRUCT(wi_item);
													if(GetWindowInfo(h_item, &wi_item)) {
														SString item_title_buf;
														TView::SGetWindowText(h_item, item_title_buf);
														if(p_clu->GetKind() == RADIOBUTTONS) {
															line_buf.Z().Tab_(2).Cat("radiobutton").Space().CatChar('[');
															line_buf.Cat("title").CatDiv(':', 2).CatQStr((temp_buf = item_title_buf).Transf(CTRANSF_OUTER_TO_UTF8));
															_BBox(_AdjustRectToOrigin(wi_item.rcWindow, _origin), line_buf.Space(), 0);
															line_buf.CatChar(']').Semicol();
														}
														else if(p_clu->GetKind() == CHECKBOXES) {
															line_buf.Z().Tab_(2).Cat("checkbox").Space().CatChar('[');
															line_buf.Cat("title").CatDiv(':', 2).CatQStr((temp_buf = item_title_buf).Transf(CTRANSF_OUTER_TO_UTF8));
															_BBox(_AdjustRectToOrigin(wi_item.rcWindow, _origin), line_buf.Space(), 0);
															line_buf.CatChar(']').Semicol();
														}
														f_out.WriteLine(line_buf.CR());
														if(item_title_buf.NotEmpty()) {
															text_line_buf.Z().Cat(symb).Tab().Cat(item_title_buf).CR();
															f_out_text.WriteLine(text_line_buf);
															{
																if(p_clu->GetKind() == RADIOBUTTONS) {
																	// \item[\dlgradioc{Сортировать по}]
																	PreprocessCtrlText(item_title_buf, ctl_text_processed);
																	line_buf.Z().Tab_(2).BSlash().Cat("item").CatChar('[').BSlash().Cat("dlgradioi").
																		CatChar('{').Cat(ctl_text_processed).CatChar('}').CatChar(']').CR().CR();
																	f_out_manual.WriteLine(line_buf);
																}
																else if(p_clu->GetKind() == CHECKBOXES) {
																	// \dlgflag{Просмотр}
																	PreprocessCtrlText(item_title_buf, ctl_text_processed);
																	line_buf.Z().Tab_(2).BSlash().Cat("dlgflag").CatChar('{').Cat(ctl_text_processed).CatChar('}').CR().CR();
																	f_out_manual.WriteLine(line_buf);
																}
															}
														}
													}
												}
												TakeInCountCtrl(h_item, child_list, seen_pos_list);
											}
											//
											line_buf.Z().Tab().CatChar('}').CR();
											f_out.WriteLine(line_buf);
										}
										TakeInCountCtrl(h, child_list, seen_pos_list);
									}
									else {
										//T_FRAME T_IDENT T_CONST_STR uirectopt uictrl_properties ';'
										line_buf.Tab().Cat("framebox").Space().CatChar('[');
										if(ctl_text.NotEmpty())
											line_buf.Cat("title").CatDiv(':', 2).CatQStr((temp_buf = ctl_text).Transf(CTRANSF_OUTER_TO_UTF8)).Space();
										_BBox(_AdjustRectToOrigin(wi.rcWindow, _origin), line_buf, 0);
										line_buf.CatChar(']').Semicol();
										f_out.WriteLine(line_buf.CR());
										TakeInCountCtrl(h, child_list, seen_pos_list);
									}
								}
								else if(oneof2(bt, BS_PUSHBUTTON, BS_DEFPUSHBUTTON)) {
									//T_BUTTON T_IDENT T_CONST_STR uirectopt T_IDENT uictrl_properties ';'
									if(TView::IsSubSign(p_view, TV_SUBSIGN_BUTTON)) {
										TButton * p_button = static_cast<TButton *>(p_view);
										uint   cmd_id = p_button->GetCommand();
										SString cmd_buf;
										if(cmd_id)
											dlg->GetCtlSymb(cmd_id+100000, cmd_buf);
										else
											cmd_buf = "cmNone";
										size_t offs = 0;
										if(cmd_buf.HasPrefix("cma"))
											offs = 3;
										else if(cmd_buf.HasPrefix("cm"))
											offs = 2;
										if(symb.ToLong() != 0) {
											symb.Z().Cat("CTL").CatChar('_').Cat("CMD").CatChar('_').Cat(cmd_buf+offs).ToUpper();
										}
										line_buf.Tab().Cat("button").Space().Cat(symb).Space().CatChar('[');
										line_buf.Cat("title").CatDiv(':', 2).CatQStr((temp_buf = ctl_text).Transf(CTRANSF_OUTER_TO_UTF8));
										if(wi.dwStyle & WS_TABSTOP)
											line_buf.Space().Cat("tabstop");
										if(wi.dwStyle & WS_DISABLED)
											line_buf.Space().Cat("disabled");
										if(bt == BS_DEFPUSHBUTTON)
											line_buf.Space().Cat("defaultitem");
										_BBox(_AdjustRectToOrigin(wi.rcWindow, _origin), line_buf.Space(), 0);
										line_buf.Space().Cat("command").CatDiv(':', 2).Cat(cmd_buf);
										line_buf.CatChar(']');
										line_buf.Semicol();
										f_out.WriteLine(line_buf.CR());
										TakeInCountCtrl(h, child_list, seen_pos_list);
									}
								}
							}
							else if(cls_name.IsEqiAscii("Static")) {
								//
								// Этикетки (TLabel) пропускаем (они обрабатываются объектами, которым принадлежат)
								//
								if(!TView::IsSubSign(p_view, TV_SUBSIGN_LABEL)) {
									const bool is_image = (p_view && p_view->IsSubSign(TV_SUBSIGN_IMAGEVIEW));
									if(!p_view)
										symb.Z();
									else if(!p_view->IsSubSign(TV_SUBSIGN_STATIC) && !is_image)
										symb.Z();
									else if(symb.ToLong()) {
										temp_buf.Z().CatLongZ(symb.ToLong(), 5);
										symb.Z().Cat("CTL").CatChar('_').Cat(dlg_symb_body).CatChar('_').Cat("ST").CatChar('_').Cat(temp_buf);
									}
									line_buf.Tab().Cat(is_image ? "imageview" : "statictext").Space();
									if(symb.NotEmpty())
										line_buf.Cat(symb).Space();
									line_buf.CatChar('[');
									if(ctl_text.NotEmpty()) {
										line_buf.Cat("title").CatDiv(':', 2).CatQStr((temp_buf = ctl_text).Transf(CTRANSF_OUTER_TO_UTF8)).Space();
									}
									_BBox(_AdjustRectToOrigin(wi.rcWindow, _origin), line_buf, 0);
									if(is_image) {
										TImageView * p_iv = static_cast<TImageView *>(p_view);
										if(p_iv->GetFigSymb().NotEmpty()) {
											line_buf.Space().Cat("imagesymb").CatDiv(':', 2).Cat(p_iv->GetFigSymb());
										}
									}
									if(wi.dwExStyle & WS_EX_STATICEDGE) {
										line_buf.Space().Cat("staticedge");
										//prop_list.Add(DlScope::cuifStaticEdge, temp_buf.Z());
									}
									line_buf.CatChar(']').Semicol();
									//DlScope::PropListToLine(prop_list, 2, line_buf).Semicol().CR();
									f_out.WriteLine(line_buf.CR());
									TakeInCountCtrl(h, child_list, seen_pos_list);
								}
							}
							else if(cls_name.IsEqiAscii("SysListView32") || cls_name.IsEqiAscii("ListBox")) {
								if(TView::IsSubSign(p_view, TV_SUBSIGN_LISTBOX)) {
									SmartListBox * p_list = static_cast<SmartListBox *>(p_view);
									if(p_label)
										ctl_text = label_text;
									line_buf.Tab().Cat("listbox").Space().Cat(symb).Space().CatChar('[');
									if(ctl_text.NotEmpty()) {
										line_buf.Cat("title").CatDiv(':', 2).CatQStr((temp_buf = ctl_text).Transf(CTRANSF_OUTER_TO_UTF8)).Space();
									}
									_BBox(_AdjustRectToOrigin(wi.rcWindow, _origin), line_buf, 0);
									p_list->GetOrgColumnsDescr(temp_buf);
									temp_buf.Transf(CTRANSF_INNER_TO_OUTER); // @v12.3.9
									if(temp_buf.NotEmptyS()) {
										line_buf.Space().Cat("columns").CatDiv(':', 2).CatQStr(temp_buf);
									}
									line_buf.CatChar(']');
									line_buf.Semicol();
									f_out.WriteLine(line_buf.CR());
									TakeInCountCtrl(h, child_list, seen_pos_list);
								}
							}
							else if(cls_name.IsEqiAscii("SysTreeView32")) {
								// T_TREELISTBOX T_IDENT T_CONST_STR uirectopt uictrl_properties
								if(TView::IsSubSign(p_view, TV_SUBSIGN_LISTBOX)) {
									SmartListBox * p_list = static_cast<SmartListBox *>(p_view);
									if(p_label)
										ctl_text = label_text;
									line_buf.Tab().Cat("treelistbox").Space().Cat(symb).Space().CatChar('[');
									if(ctl_text.NotEmpty())
										line_buf.Cat("title").CatDiv(':', 2).CatQStr((temp_buf = ctl_text).Transf(CTRANSF_OUTER_TO_UTF8)).Space();
									_BBox(_AdjustRectToOrigin(wi.rcWindow, _origin), line_buf, 0);
									line_buf.CatChar(']');
									line_buf.Semicol();
									f_out.WriteLine(line_buf.CR());
									TakeInCountCtrl(h, child_list, seen_pos_list);
								}
							}
							else {
								line_buf = cls_name;
								line_buf.Z();
							}
						}
					}
				}
			}
			line_buf.Z().CatChar('}').CR();
			f_out.WriteLine(line_buf);
			{
				// \end{description}
				line_buf.Z().BSlash().Cat("end").CatChar('{').Cat("description").CatChar('}').CR().CR();
				f_out_manual.WriteLine(line_buf);
				// %endtopic
                line_buf.Z().Cat("%endtopic").CR().CR();
                f_out_manual.WriteLine(line_buf);
			}
		}
		ZDELETE(dlg);
	}
	delete dlg;
	return ok;
}
//
//
//
// @v12.3.2 (replaced with PPConst::WrKey_RecentItems) static const char * WrSubKey_RecentItems = "Software\\Papyrus\\RecentItems";

RecentItemsStorage::RecentItemsStorage(int ident, uint maxItems, CompFunc cf) : Ident(ident), MaxItems(maxItems), Cf(cf)
{
}

RecentItemsStorage::RecentItemsStorage(const char * pIdent, uint maxItems, CompFunc cf) : Ident(0), IdentText(pIdent), MaxItems(maxItems), Cf(cf)
{
}

RecentItemsStorage::~RecentItemsStorage()
{
}

int RecentItemsStorage::CheckIn(const char * pText)
{
	int    ok = -1;
	if(!isempty(pText)) {
		SString key;
		if(IdentText.NotEmpty())
			key = IdentText;
		else if(Ident > 0)
			key.CatChar('#').Cat(Ident);
		if(key.NotEmpty()) {
			WinRegKey reg_key(HKEY_CURRENT_USER, PPConst::WrKey_RecentItems, 0);
			StringSet ss;
			SString temp_buf;
			size_t rec_size = 0;
			THROW_SL(reg_key.GetRecSize(key, &rec_size));
			if(rec_size) {
				STempBuffer buff(rec_size);
				THROW_SL(reg_key.GetBinary(key, buff, rec_size));
				ss.setBuf(buff, rec_size);
			}
			{
				uint ssc = ss.getCount();
				StringSet temp_ss;
				for(uint ssp = 0, j = 0; ss.get(&ssp, temp_buf); j++) {
					const bool is_eq = Cf ? (Cf(pText, temp_buf.cptr(), 0) == 0) : sstreq(pText, temp_buf);
					if(!is_eq && (!(MaxItems > 0 && ssc >= MaxItems) || j > (ssc - MaxItems)))
						temp_ss.add(temp_buf);
				}
				temp_ss.add(pText);
				THROW_SL(reg_key.PutBinary(key, temp_ss.getBuf(), temp_ss.getDataLen()));
				ok = 1;
			}
		}
	}
	CATCHZOK
	return ok;
}

int RecentItemsStorage::GetList(StringSet & rSs)
{
	rSs.Z();
	int    ok = -1;
	SString key;
	if(IdentText.NotEmpty())
		key = IdentText;
	else if(Ident > 0)
		key.CatChar('#').Cat(Ident);
	if(key.NotEmpty()) {
		WinRegKey reg_key(HKEY_CURRENT_USER, PPConst::WrKey_RecentItems, 0);
		StringSet ss;
		SString temp_buf;
		size_t rec_size = 0;
		THROW_SL(reg_key.GetRecSize(key, &rec_size));
		if(rec_size) {
			STempBuffer buff(rec_size);
			THROW_SL(reg_key.GetBinary(key, buff, rec_size));
			ss.setBuf(buff, rec_size);
			for(uint ssp = 0, j = 0; ss.get(&ssp, temp_buf); j++) {
				rSs.add(temp_buf);
				ok = 1;
			}
		}
	}
	CATCHZOK
	return ok;
}

EditTextFileParam::EditTextFileParam() : PPBaseFilt(PPFILT_EDITTEXTFILEPARAM, 0, 0)
{
	SetFlatChunk(offsetof(EditTextFileParam, ReserveStart),
		offsetof(EditTextFileParam, ReserveEnd)-offsetof(EditTextFileParam, ReserveStart)+sizeof(ReserveEnd));
	SetBranchSString(offsetof(EditTextFileParam, FileName));
	Init(1, 0);
}

int PPEditTextFile(const EditTextFileParam * pParam)
{
	class OpenEditFileDialog : public TDialog {
	public:
		explicit OpenEditFileDialog(RecentItemsStorage * pRis, const char * pPredefinedWildcard) : TDialog(DLG_OPENEDFILE), FileID(0), P_Ris(pRis),
			PredefinedWildcard(pPredefinedWildcard)
		{
			FileBrowseCtrlGroup::Setup(this, CTLBRW_OPENEDFILE_SELECT, CTL_OPENEDFILE_SELECT, 1, 0,
				PPTXT_FILPAT_PPYTEXT, FileBrowseCtrlGroup::fbcgfFile|FileBrowseCtrlGroup::fbcgfSaveLastPath);
			if(PredefinedWildcard.NotEmpty()) {
				FileBrowseCtrlGroup * p_fbg = static_cast<FileBrowseCtrlGroup *>(getGroup(1));
				CALLPTRMEMB(p_fbg, setInitPath(PredefinedWildcard));
			}
			SetupStrListBox(this, CTL_OPENEDFILE_RESERV);
			SetupStrListBox(this, CTL_OPENEDFILE_RECENT);
			SetupReservList();
			SetupRecentList();
		}
		int    GetFileName(SString & rBuf)
		{
			if(FileName.IsEmpty() && !FileID)
				getCtrlString(CTL_OPENEDFILE_SELECT, FileName);
			if(FileID == PPRFILE_VERHIST_LOG && FileName.NotEmpty()) {
				SString db_path, log_path;
				PPGetPath(PPPATH_LOG, log_path);
				DBS.GetDbPath(DBS.GetDbPathID(), db_path);
				PPVerHistory::Log(db_path, log_path);
			}
			rBuf = FileName;
			return BIN(rBuf.NotEmptyS());
		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(event.isCmd(cmLBDblClk)) {
				TView * p_view = TVINFOVIEW;
				if(p_view) {
					if(p_view->GetId() == CTL_OPENEDFILE_RESERV) {
						SmartListBox * p_box = static_cast<SmartListBox *>(p_view);
						if(p_box->P_Def) {
							long   id = 0;
							p_box->getCurID(&id);
							PPRFile fi;
							if(DS.GetRFileInfo(id, fi) > 0 && fi.PathID) {
								SString full_path;
								PPGetFilePath(fi.PathID, fi.Name, full_path);
								FileName = full_path;
								FileID = id;
								endModal(cmOK);
							}
						}
					}
					else if(p_view->GetId() == CTL_OPENEDFILE_RECENT) {
						SmartListBox * p_box = static_cast<SmartListBox *>(p_view);
						if(p_box->P_Def) {
							long   id = 0;
							SString full_path;
							p_box->getCurID(&id);
							if(RecentItems.GetText(id, full_path) > 0) {
								FileName = full_path;
								FileID = 0;
								endModal(cmOK);
							}
						}
					}
				}
			}
			else if(event.isCmd(cmLBItemFocused)) {
				TView * p_view = TVINFOVIEW;
				SString info_buf;
				if(p_view && p_view->GetId() == CTL_OPENEDFILE_RESERV) {
					SmartListBox * p_reserv_box = static_cast<SmartListBox *>(p_view);
					if(p_reserv_box->P_Def) {
						long   id = 0;
						p_reserv_box->getCurID(&id);
						PPRFile fi;
						if(DS.GetRFileInfo(id, fi) > 0) {
							info_buf = fi.Descr;
							if(fi.PathID) {
								PPGetFilePath(fi.PathID, fi.Name, FileName);
								FileID = id;
								setCtrlString(CTL_OPENEDFILE_SELECT, FileName);
							}
						}
					}
				}
				setStaticText(CTL_OPENEDFILE_ST_INFO, info_buf);
			}
			if(TVBROADCAST && TVCMD == cmCommitInput) {
				const uint i = TVINFOVIEW->GetId();
				if(i == CTL_OPENEDFILE_SELECT) {
					FileID = 0;
					getCtrlString(i, FileName);
				}
			}
			else
				return;
			clearEvent(event);
		}
		void   SetupRecentList()
		{
			RecentItems.Z();
			if(P_Ris) {
				SmartListBox * p_recent_box = static_cast<SmartListBox *>(getCtrlView(CTL_OPENEDFILE_RECENT));
				if(p_recent_box) {
					StringSet ss_ris;
					if(P_Ris->GetList(ss_ris) > 0) {
						ss_ris.reverse();
						SString temp_buf;
						StringSet ss(SLBColumnDelim);
						long   fid = 0;
						for(uint ssp = 0; ss_ris.get(&ssp, temp_buf);) {
							ss.Z();
							RecentItems.AddFast(++fid, temp_buf);
							ss.add(temp_buf);
							ss.add(temp_buf.Z());
							p_recent_box->addItem(fid, ss.getBuf());
						}
					}
					p_recent_box->Draw_();
				}
			}
		}
		void   SetupReservList()
		{
			PPRFile fi;
			StringSet ss(SLBColumnDelim);
			SString temp_buf, full_path;
			SmartListBox * p_reserv_box = static_cast<SmartListBox *>(getCtrlView(CTL_OPENEDFILE_RESERV));
			if(p_reserv_box) {
				for(SEnum en = DS.EnumRFileInfo(); en.Next(&fi) > 0;) {
					if(fi.Flags & PPRFILEF_TEXT) {
						ss.Z();
						ss.add(fi.Name);
						if(fi.PathID) {
							PPGetPath(fi.PathID, temp_buf);
							PPGetFilePath(fi.PathID, fi.Name, full_path);
						}
						else
							temp_buf.Z();
						ss.add(temp_buf);
						temp_buf.Z();
						if(full_path.NotEmpty() && fileExists(full_path)) {
							LDATETIME dtm;
							SFile::GetTime(full_path, 0, 0, &dtm);
							temp_buf.Cat(dtm, DATF_DMY, TIMF_HMS);
						}
						ss.add(temp_buf);
						p_reserv_box->addItem(fi.ID, ss.getBuf());
					}
				}
				p_reserv_box->Draw_();
			}
		}
		PPID   FileID;
		SString FileName;
		SString PredefinedWildcard;
		StrAssocArray RecentItems;
		RecentItemsStorage * P_Ris;
	};
	int    ok = -1;
	RecentItemsStorage ris("PPViewTextBrowser", 20, PTR_CMPFUNC(FilePathUtf8));
	OpenEditFileDialog * dlg = 0;
	SString file_name;
	if(pParam)
		file_name = pParam->FileName;
	if(!file_name.NotEmptyS() || IsWild(file_name) || SFile::IsDir(file_name) || !fileExists(file_name)) {
		const char * p_predefined_wildcard = 0;
		if(IsWild(file_name))
			p_predefined_wildcard = file_name;
		else if(SFile::IsDir(file_name)) {
			file_name.SetLastSlash().Cat("*.*");
			p_predefined_wildcard = file_name;
		}
		THROW(CheckDialogPtr(&(dlg = new OpenEditFileDialog(&ris, p_predefined_wildcard))));
		dlg->setCtrlString(CTL_OPENEDFILE_SELECT, file_name);
		if(ExecView(dlg) == cmOK) {
			dlg->GetFileName(file_name);
			ZDELETE(dlg);
			ok = 1;
		}
	}
	else
		ok = 1;
	if(ok > 0) {
		SString temp_buf = file_name;
		temp_buf.Transf(CTRANSF_OUTER_TO_UTF8);
		ris.CheckIn(temp_buf);
		PPViewTextBrowser(file_name, 0, 0, -1);
	}
	CATCHZOKPPERR
	delete dlg;
	return ok;
}
//
//
//
int BigTextDialog(uint maxLen, const char * pWindowTitle, const char * pSubTitle, SString & rText)
{
	class __BigTextDialog : public TDialog {
	public:
		explicit __BigTextDialog(uint maxLen = 0) : TDialog(DLG_BIGTXTEDIT)
		{
			setMaxLen(maxLen);
		}
		int   setMaxLen(size_t maxLen)
		{
			int    ok = 1;
			if(checkirange(maxLen, static_cast<size_t>(1), static_cast<size_t>(4000)))
				SetupInputLine(CTL_BIGTXTEDIT_TEXT, MKSTYPE(S_ZSTRING, maxLen), MKSFMT(maxLen, 0));
			else
				ok = PPSetErrorSLib();
			return ok;
		}
		int   setDTS(const SString * pText)
		{
			TInputLine * p_il = static_cast<TInputLine *>(getCtrlView(CTL_BIGTXTEDIT_TEXT));
			if(p_il) {
				if(pText) {
					setCtrlString(CTL_BIGTXTEDIT_TEXT, *pText);
				}
				p_il->selectAll(0);
			}
			return 1;
		}
		int   getDTS(SString * pText)
		{
			if(pText)
				getCtrlString(CTL_BIGTXTEDIT_TEXT, *pText);
			return 1;
		}
	};
    int    ok = -1;
    __BigTextDialog * dlg = new __BigTextDialog(maxLen);
    if(CheckDialogPtrErr(&dlg)) {
		dlg->setTitle(pWindowTitle);
		dlg->setCtrlString(CTL_BIGTXTEDIT_TITLE, SLS.AcquireRvlStr() = pSubTitle);
		dlg->setDTS(&rText);
		if(ExecView(dlg)) {
			dlg->getDTS(&rText);
			ok = 1;
		}
    }
    delete dlg;
    return ok;
}
//
//
//
ExtStrContainerListDialog::ExtStrContainerListDialog(uint dlgId, uint listCtlId, const char * pTitle, bool readOnly, const SIntToSymbTabEntry * pDescrList, size_t descrListCount) : 
	PPListDialog(dlgId, listCtlId), ReadOnly(readOnly), P_DescrList(pDescrList), DescrListCount(descrListCount)
{
	if(ReadOnly) {
		SString temp_buf;
		showCtrl(STDCTL_INSBUTTON, false);
		showCtrl(STDCTL_DELBUTTON, false);
		showCtrl(STDCTL_EDITBUTTON, false);
		showCtrl(STDCTL_OKBUTTON, false);
		SetButtonText(cmCancel, PPLoadStringS("close", temp_buf).Transf(CTRANSF_INNER_TO_OUTER));
	}
	setTitle(pTitle);
	updateList(0);
}

IMPL_DIALOG_SETDTS(ExtStrContainerListDialog)
{
	int    ok = 1;
	RVALUEPTR(Data, pData);
	//
	updateList(0);
	return ok;
}
	
IMPL_DIALOG_GETDTS(ExtStrContainerListDialog)
{
	int    ok = 1;
	//
	ASSIGN_PTR(pData, Data);
	return ok;
}

/*virtual*/int ExtStrContainerListDialog::setupList()
{
	StringSet ss(SLBColumnDelim);
	SString temp_buf;
	SString title_buf;
	for(uint i = 0; i < DescrListCount; i++) {
		const int fld_id = P_DescrList[i].Id;
		// @v12.1.0 if(Data.GetExtStrData(fld_id, temp_buf) > 0) {
			Data.GetExtStrData(fld_id, temp_buf); // @v12.1.0
			ss.Z();
			PPLoadString(P_DescrList[i].P_Symb, title_buf);
			ss.add(title_buf);
			ss.add(temp_buf);
			addStringToList(fld_id, ss.getBuf());
		// @v12.1.0 }
	}
	return 1;
}


/*virtual*/int ExtStrContainerListDialog::addItem(long * pos, long * id)
{
	int    ok = -1;
	return ok;
}

/*virtual*/int ExtStrContainerListDialog::editItem(long pos, long id)
{
	int    ok = -1;
	if(pos >= 0 && pos < static_cast<long>(DescrListCount)) {
		SString temp_buf;
		SString title_buf;		
		const int fld_id = P_DescrList[pos].Id;
		PPLoadString(P_DescrList[pos].P_Symb, title_buf);
		Data.GetExtStrData(fld_id, temp_buf);
		if(BigTextDialog(1024, 0, title_buf, temp_buf) > 0) {
			Data.PutExtStrData(fld_id, temp_buf);
			ok = 1;
		}
	}
	return ok;
}

/*virtual*/int ExtStrContainerListDialog::delItem(long pos, long id)
{
	int    ok = -1;
	return ok;
}
//
//
//
class PPDialogConstructor {
public:
	PPDialogConstructor(TDialog * pDlg, DlContext & rCtx, const char * pSymb) : Status(0)
	{
		Build(pDlg, rCtx, pSymb);
	}
	PPDialogConstructor(TDialog * pDlg, DlContext & rCtx, uint symbId) : Status(0)
	{
		Build(pDlg, rCtx, symbId);
	}
	PPDialogConstructor(TDialog * pDlg, DlContext & rCtx, const DlScope * pScope) : Status(0)
	{
		Build(pDlg, rCtx, pScope);
	}
	bool   operator !() const { return LOGIC(Status & stError); }
private:
	enum {
		insertctrlstagePreprocess = 1,
		insertctrlstageMain = 2,
		insertctrlstagePostprocess = 3,
	};
	//
	// Descr: Блок, передаваемый в рекурсивную функцию InsertControlItems для нахождения первого элемента,
	//   дабы сделать его selected при открытии диалога.
	//
	struct FIBlock {
		FIBlock() : FirstTabbedItemId(0), FirstItemId(0)
		{
		}
		uint   FirstTabbedItemId; // 
		uint   FirstItemId; //		
	};
	void   Build(TDialog * pDlg, DlContext & rCtx, const DlScope * pScope);
	void   Build(TDialog * pDlg, DlContext & rCtx, const char * pSymb);
	void   Build(TDialog * pDlg, DlContext & rCtx, uint viewId);
	void   InsertControlItems(TDialog * pDlg, DlContext & rCtx, const DlScope & rParentScope, uint & rLastDynId, FIBlock & rFiBlk, int stage);
	void   InsertControlLayouts(TDialog * pDlg, DlContext & rCtx, const DlScope & rParentScope, SUiLayout * pLoParent);
	static void __stdcall SetupLayoutItemFrameProc(SUiLayout * pItem, const SUiLayout::Result & rR);
	SUiLayout * InsertCtrlLayout(TDialog * pDlg, SUiLayout * pLoParent, TView * pView, const SUiLayoutParam & rP);
	SUiLayout * InsertCtrlLayout(TDialog * pDlg, SUiLayout * pLoParent, ushort ctlId, const SUiLayoutParam & rP);
	bool   MakeComplexLayout_InputLine(TDialog * pDlg, TView * pView, SUiLayoutParam & rLp, DlContext & rCtx, const DlScope * pScope, SUiLayout * pLoParent);
	long   GetScopeID(const DlScope * pScope) const { return pScope ? (pScope->ID + 2000) : 0; }

	enum {
		stError         = 0x0001,
		stScopeNotFound = 0x0002 // Дополнительный признак, означающий, что по переданному символу или идентификатору область определения диалога не найдена.
	};
	uint   Status;
};

void PPDialogConstructor::Build(TDialog * pDlg, DlContext & rCtx, const char * pSymb)
{
	if(pDlg && !isempty(pSymb)) {
		const DlScope * p_scope = rCtx.GetScopeByName_Const(DlScope::kUiView, pSymb);
		if(p_scope) {
			Build(pDlg, rCtx, p_scope);
		}
		else {
			Status |= (stError | stScopeNotFound);
		}
	}
	else
		Status |= stError;
	
}

void PPDialogConstructor::Build(TDialog * pDlg, DlContext & rCtx, uint viewId)
{
	if(pDlg && viewId) {
		const DlScope * p_scope = rCtx.GetDialogScopeBySymbolIdent_Const(viewId);
		if(p_scope) {
			Build(pDlg, rCtx, p_scope);
		}
		else {
			Status |= (stError | stScopeNotFound);
		}
	}
	else
		Status |= stError;
}

void PPDialogConstructor::Build(TDialog * pDlg, DlContext & rCtx, const DlScope * pScope)
{
	if(pDlg && pScope && pScope->IsKind(DlScope::kUiView)) {
		const UiDescription * p_uid = SLS.GetUiDescription();
		TDialog::BuildEmptyWindowParam bew_param;
		char   c_buf[1024];
		SString temp_buf;
		SUiLayoutParam __alb;
		const SUiLayoutParam * p_alb = rCtx.GetLayoutBlock(pScope, DlScope::cuifLayoutBlock, &__alb) ? &__alb : 0;
		{
			CtmExprConst __c = pScope->GetConst(DlScope::cuifCtrlText);
			if(!!__c) {
				if(rCtx.GetConstData(__c, c_buf, sizeof(c_buf))) {
					temp_buf = reinterpret_cast<const char *>(c_buf);
					pDlg->setTitle(temp_buf);
				}
			}
		}
		{
			FRect rect;
			const uint gnrr = p_alb ? p_alb->GetNominalRect(rect) : 0;
			if(rect.Width() <= 0.0f) {
				rect.b.x = rect.a.x + 60.0f;
			}
			if(rect.Height() <= 0.0f) {
				rect.b.y = rect.a.y + 60.0f;
			}
			pDlg->setBounds(rect);
		}
		{
			{
				CtmExprConst __c = pScope->GetConst(DlScope::cuifFont);
				if(!!__c) {
					if(rCtx.GetConstData(__c, c_buf, sizeof(c_buf))) {
						bew_param.FontFace = reinterpret_cast<const char *>(c_buf);
					}
				}
			}
			{
				CtmExprConst __c = pScope->GetConst(DlScope::cuifFontSize);
				if(!!__c) {
					if(rCtx.GetConstData(__c, c_buf, sizeof(c_buf))) {
						double font_size_real = *reinterpret_cast<const double *>(c_buf);
						bew_param.FontSize = static_cast<int>(font_size_real);
					}
				}
			}
			// @v12.3.7 {
			if(bew_param.FontFace.IsEmpty() || bew_param.FontSize == 0) {
				const SFontDescr * p_fd = p_uid ? p_uid->GetFontDescrC("DialogFont") : 0;
				if(p_fd) {
					if(bew_param.FontFace.IsEmpty()) {
						if(p_fd && p_fd->Face.NotEmpty())
							bew_param.FontFace = p_fd->Face;
						else
							bew_param.FontFace = "MS Shell Dlg 2";
					}
					if(bew_param.FontSize == 0) {
						if(p_fd && p_fd->Size != 0)
							bew_param.FontSize = p_fd->Size;
						else
							bew_param.FontSize = 8;
					}
				}
			}
			// } @v12.3.7 
			if(temp_buf.NotEmpty()) {
			}
		}
		pDlg->BuildEmptyWindow(&bew_param);
		//
		{
			const int container_direc = p_alb ? p_alb->GetContainerDirection() : DIREC_UNKN;
			SUiLayout * p_lo_main = 0;
			if(oneof2(container_direc, DIREC_HORZ, DIREC_VERT)) {
				p_lo_main = new SUiLayout(*p_alb);
				p_lo_main->SetID(GetScopeID(pScope));
				p_lo_main->SetSymb(pScope->GetName());
			}
			pDlg->SetLayout(p_lo_main);
			{
				//
				// Далее, мы должны вставить управляющие элементы в созданое окно диалога
				//
				uint last_dyn_id = 20000; 
				// (@unused) InsertControlItems(rCtx, *pScope, last_dyn_id, insertctrlstagePreprocess);
				FIBlock fi_blk;
				FIBlock fake_fi_blk;
				InsertControlItems(pDlg, rCtx, *pScope, last_dyn_id, fi_blk, insertctrlstageMain);
				InsertControlItems(pDlg, rCtx, *pScope, last_dyn_id, fake_fi_blk/*на этой стадии не надо*/, insertctrlstagePostprocess);
				{
					uint    ctl_to_select = 0;
					if(fi_blk.FirstTabbedItemId) {
						ctl_to_select = fi_blk.FirstTabbedItemId;
						
					}
					else if(fi_blk.FirstItemId) {
						ctl_to_select = fi_blk.FirstItemId;
					}
					if(ctl_to_select) {
						pDlg->selectCtrl(ctl_to_select);
					}
				}
			}
			InsertControlLayouts(pDlg, rCtx, *pScope, p_lo_main); // Теперь расставляем layout'ы
		}
		{
			HWND h_wnd = pDlg->H();
			{
				TWindow::CreateBlock cr_blk;
				MEMSZERO(cr_blk);
				RECT cr;
				::GetClientRect(h_wnd, &cr);
				cr_blk.Coord = cr;
				cr_blk.Param = this;
				cr_blk.H_Process = 0;
				cr_blk.Style = 0;
				cr_blk.ExStyle = 0;
				cr_blk.H_Parent = 0;
				cr_blk.H_Menu = 0;
				cr_blk.P_WndCls = 0;
				cr_blk.P_Title = 0;
				TView::messageCommand(pDlg, cmInit, &cr_blk);
			}
			pDlg->SetupCtrlTextProc(h_wnd, 0);
			pDlg->InitControls(h_wnd, 0/*wParam*/, reinterpret_cast<LPARAM>(this));
			EnumChildWindows(h_wnd, pDlg->SetupCtrlTextProc, 0);				
		}
		pDlg->EvaluateLayout(pDlg->getClientRect());
		pDlg->WbCapability |= TWindow::wbcStorableUserParams;
		pDlg->SetStorableUserParamsSymb(pScope->Name);
	}
}

/*static*/void __stdcall PPDialogConstructor::SetupLayoutItemFrameProc(SUiLayout * pItem, const SUiLayout::Result & rR)
{
	if(pItem) {
		TView * p = static_cast<TView *>(SUiLayout::GetManagedPtr(pItem));
		if(p)
			p->changeBounds(TRect(pItem->GetFrameAdjustedToParent()));
	}
}

SUiLayout * PPDialogConstructor::InsertCtrlLayout(TDialog * pDlg, SUiLayout * pLoParent, TView * pView, const SUiLayoutParam & rP)
{
	//
	// Если pView == 0, то мы вставляем лейаут без привязки к конкретному control'у. Обычно это
	// делается для вспомогательных разметочных лейаутов-контейнеров
	//
	SUiLayout * p_result = 0;
	if(pLoParent) {
		p_result = pLoParent->InsertItem(pView, &rP);
		if(p_result && pView)
			p_result->SetCallbacks(0, SetupLayoutItemFrameProc, pView);
	}
	return p_result;
}

SUiLayout * PPDialogConstructor::InsertCtrlLayout(TDialog * pDlg, SUiLayout * pLoParent, ushort ctlId, const SUiLayoutParam & rP)
{
	//
	// Если ctlId == 0, то вставляем лейаут без привязки к control'у
	// в противном случае по заданному идентификатору control должен существовать иначе - ошибка.
	// 
	//
	SUiLayout * p_result = 0;
	if(ctlId) {
		TView * p_view = pDlg->getCtrlView(ctlId);
		p_result = p_view ? InsertCtrlLayout(pDlg, pLoParent, p_view, rP) : 0;
	}
	else
		p_result = InsertCtrlLayout(pDlg, pLoParent, static_cast<TView *>(0), rP);
	return p_result;
}

void PPDialogConstructor::InsertControlItems(TDialog * pDlg, DlContext & rCtx, const DlScope & rParentScope, uint & rLastDynId, FIBlock & rFiBlk, int stage)
{
	SString temp_buf;
	SString ctl_text;
	const DlScopeList & r_scope_list = rParentScope.GetChildList();
	uint   def_button_ctl_id = 0;
	SUiCtrlSupplement_With_Symbols supplement; // @v12.3.7
	UserInterfaceSettings ui_cfg;
	const bool is_ui_cfg_valid = (ui_cfg.Restore() > 0);
	for(uint ci = 0; ci < r_scope_list.getCount(); ci++) {
		const DlScope * p_scope = r_scope_list.at(ci);
		if(p_scope) {
			uint32 vk = 0;
			uint32 symb_ident = 0;
			uint32 ui_flags = 0;
			rCtx.GetConst_Uint32(p_scope, DlScope::cuifViewKind, vk);
			rCtx.GetConst_Uint32(p_scope, DlScope::cucmSymbolIdent, symb_ident);
			const uint32 item_id = NZOR(symb_ident, GetScopeID(p_scope));
			// cuifFlags // UiItemKind::fXXX
			rCtx.GetConst_Uint32(p_scope, DlScope::cuifFlags, ui_flags);
			//UiItemKind::GetSymbById(vk, temp_buf);
			SUiLayoutParam lp;
			const bool glbr = rCtx.GetLayoutBlock(p_scope, DlScope::cuifLayoutBlock, &lp);
			{
				supplement.Z();
				CtmExprConst __c = p_scope->GetConst(DlScope::cuifSupplement);
				if(!!__c) {
					uint8    c_buf[256];
					if(rCtx.GetConstData(__c, c_buf, sizeof(c_buf))) {
						supplement = *reinterpret_cast<const SUiCtrlSupplement *>(c_buf);
						if(supplement.Kind) {
							rCtx.GetConst_String(p_scope, DlScope::cuifSupplementSymb, supplement.Symb);
							rCtx.GetConst_String(p_scope, DlScope::cuifSupplementCmdSymb, supplement.CmdSymb);
							rCtx.GetConst_String(p_scope, DlScope::cuifSupplementText, supplement.Text);
						}
					}
				}
			}
			bool    is_inserted = false; // Признак того, что элемен включен в окно диалога
			uint32  ctl_id_for_tab = item_id; // @v12.3.8
			switch(vk) {
				case UiItemKind::kInput:
					if(stage == insertctrlstageMain) {
						TRect rc;
						const uint gnrr = SUiLayoutParam::GetNominalRectWithDefaults(&lp, rc, 60.0f, 21.0f);
						//
						uint32 type_id = 0;
						uint32 format = 0;
						uint   spc_flags = 0;
						rCtx.GetConst_Uint32(p_scope, DlScope::cuifViewDataType, type_id);
						rCtx.GetConst_Uint32(p_scope, DlScope::cuifViewOutputFormat, format);
						if(ui_flags & UiItemKind::fReadOnly)
							spc_flags |= TInputLine::spcfReadOnly;
						if(ui_flags & UiItemKind::fMultiLine)
							spc_flags |= TInputLine::spcfMultiline;
						if(ui_flags & UiItemKind::fWantReturn)
							spc_flags |= TInputLine::spcfWantReturn;
						if(ui_flags & UiItemKind::fPassword) {
							spc_flags |= TInputLine::spcfPassword;
						}
						
						TInputLine * p_ctl = new TInputLine(rc, spc_flags, static_cast<TYPEID>(type_id), 0);
						if(format) {
							p_ctl->setFormat(format);
						}
						if(ui_flags & UiItemKind::fTabStop) {
							p_ctl->setState(sfTabStop, true);
						}
						{
							// Label надо вставить до поля ввода
							if(rCtx.GetConst_String(p_scope, DlScope::cuifCtrlText, ctl_text)) {
								//cuifCtrlLblRect,  // raw    Координаты текстового ярлыка, ассоциированного с управляющим элементом
								//cuifCtrlLblSymb,  // string Символ текстового ярлыка, ассоциированного с управляющим элементом
								//cuifLabelRect,    // raw(UiRelRect) Положение текстового ярлыка, ассоциированного с управляющим элементом
								//cuifLblLayoutBlock
								SUiLayoutParam lp_label;
								const bool glb_label_r = rCtx.GetLayoutBlock(p_scope, DlScope::cuifLblLayoutBlock, &lp_label);
								TRect rc_label;
								const uint gnrr = SUiLayoutParam::GetNominalRectWithDefaults(&lp_label, rc_label, 60.0f, 13.0f);
								TLabel * p_lbl = new TLabel(rc_label, ctl_text, p_ctl);
								pDlg->InsertCtlWithCorrespondingNativeItem(p_lbl, ++rLastDynId, 0, /*extraPtr*/0);
							}
						}
						pDlg->InsertCtlWithCorrespondingNativeItem(p_ctl, item_id, 0, /*extraPtr*/0);
						is_inserted = true;
						if(oneof6(supplement.Kind, SUiCtrlSupplement::kDateCalendar, SUiCtrlSupplement::kDateRangeCalendar,
							SUiCtrlSupplement::kTime, SUiCtrlSupplement::kCalc, SUiCtrlSupplement::kAsterisk, SUiCtrlSupplement::kFileBrowse)) { // @v12.3.10 SUiCtrlSupplement::kFileBrowse
							if(supplement.Ident) {
								TRect rc_sb;
								rc_sb.a.x = rc.b.x+1;
								rc_sb.a.y = rc.a.y;
								rc_sb.b.x = rc_sb.a.x + 20;
								rc_sb.b.y = rc_sb.a.y;
								uint   pic_id = 0;
								switch(supplement.Kind) {
									case SUiCtrlSupplement::kDateCalendar: pic_id = PPDV_CALENDARDAY01; break;
									case SUiCtrlSupplement::kDateRangeCalendar: pic_id = PPDV_CALENDAR03; break;
									case SUiCtrlSupplement::kTime: pic_id = PPDV_CLOCK02; break;
									case SUiCtrlSupplement::kCalc: pic_id = PPDV_CALCULATOR02; break;
									case SUiCtrlSupplement::kAsterisk: pic_id = PPDV_ASTERISK01; break;
									case SUiCtrlSupplement::kFileBrowse: pic_id = PPDV_FOLDER02; break;
								}
								TButton * p_sb = new TButton(rc_sb, 0, supplement.Cmd, 0, pic_id);
								p_sb->SetSupplementFactors(supplement.Kind, item_id);
								pDlg->InsertCtlWithCorrespondingNativeItem(p_sb, supplement.Ident, 0, /*extraPtr*/0);
								/*{
									if(supplement.Kind == SUiCtrlSupplement::kDateCalendarButton)
										pDlg->SetupCalDate_Internal(supplement.Ident, item_id);
									else if(supplement.Kind == SUiCtrlSupplement::kDateRangeCalendarButton)
										pDlg->SetupCalPeriod_Internal(supplement.Ident, item_id);
								}*/
							}
						}
					}
					break;
				case UiItemKind::kStatic:
					if(stage == insertctrlstageMain) {
						TRect rc;
						const uint gnrr = SUiLayoutParam::GetNominalRectWithDefaults(&lp, rc, 60.0f, 60.0f);
						uint   spc_flags = (ui_flags & UiItemKind::fStaticEdge) ? TStaticText::spcfStaticEdge : 0;
						rCtx.GetConst_String(p_scope, DlScope::cuifCtrlText, ctl_text);
						TStaticText * p_ctl = new TStaticText(rc, spc_flags, ctl_text);
						pDlg->InsertCtlWithCorrespondingNativeItem(p_ctl, item_id, 0, /*extraPtr*/0);
						is_inserted = true;
					}
					break;
				case UiItemKind::kPushbutton:
					if(stage == insertctrlstageMain) {
						TRect rc;
						const uint gnrr = SUiLayoutParam::GetNominalRectWithDefaults(&lp, rc, 60.0f, 60.0f);
						uint32 cmd_id = 0;
						uint   spc_flags = 0;
						SString cmd_symb;
						rCtx.GetConst_Uint32(p_scope, DlScope::cuifCtrlCmd, cmd_id); // uint32 ИД команды кнопки
						rCtx.GetConst_String(p_scope, DlScope::cuifCtrlCmdSymb, cmd_symb); // string Символ команды кнопки
						rCtx.GetConst_String(p_scope, DlScope::cuifCtrlText, ctl_text);
						if(ui_flags & UiItemKind::fDefault) {
							spc_flags |= TButton::spcfDefault;
							def_button_ctl_id = item_id;
						}
						TButton * p_ctl = new TButton(rc, ctl_text, cmd_id, spc_flags);
						if(ui_flags & UiItemKind::fTabStop) {
							p_ctl->setState(sfTabStop, true);
						}
						pDlg->InsertCtlWithCorrespondingNativeItem(p_ctl, item_id, 0, /*extraPtr*/0);
						is_inserted = true;
					}
					break;
				case UiItemKind::kCheckbox: 
					if(stage == insertctrlstageMain) {
						TRect rc;
						const uint gnrr = SUiLayoutParam::GetNominalRectWithDefaults(&lp, rc, 60.0f, 60.0f);
						TCluster * p_ctl = new TCluster(rc, CHECKBOXES, TCluster::spcfSingleItemWithoutFrame/*spcFlags*/, 0/*pTitle*/, 0);
						{
							if(rCtx.GetConst_String(p_scope, DlScope::cuifCtrlText, temp_buf)) {
								p_ctl->AddItem(-1, temp_buf, &rc);
							}
						}
						pDlg->InsertCtlWithCorrespondingNativeItem(p_ctl, item_id, 0, /*extraPtr*/0);
						ctl_id_for_tab = item_id; // @v12.3.9
						is_inserted = true;
					}
					break;
				case UiItemKind::kCheckCluster: 
					if(stage == insertctrlstageMain) {
						TRect rc;
						const uint gnrr = SUiLayoutParam::GetNominalRectWithDefaults(&lp, rc, 60.0f, 60.0f);
						rCtx.GetConst_String(p_scope, DlScope::cuifCtrlText, ctl_text);
						TCluster * p_ctl = new TCluster(rc, CHECKBOXES, 0/*spcFlags*/, ctl_text, 0);
						{
							const DlScopeList & r_item_scope_list = p_scope->GetChildList();
							for(uint cii = 0; cii < r_item_scope_list.getCount(); cii++) {
								const DlScope * p_ci = r_item_scope_list.at(cii);
								if(p_ci) {
									if(rCtx.GetConst_String(p_ci, DlScope::cuifCtrlText, temp_buf)) {
										TRect rc_item;
										SUiLayoutParam lp_item;
										const bool glb_item_r = rCtx.GetLayoutBlock(p_ci, DlScope::cuifLayoutBlock, &lp_item);
										const uint gnrr_item = SUiLayoutParam::GetNominalRectWithDefaults(&lp_item, rc_item, static_cast<float>(rc.width() - 2), 16.0f);
										p_ctl->AddItem(-1, temp_buf, &rc_item);
									}
								}
							}
						}
						pDlg->InsertCtlWithCorrespondingNativeItem(p_ctl, item_id, 0, /*extraPtr*/0);
						ctl_id_for_tab = item_id; // @v12.3.9
						is_inserted = true;
					}
					break;
				case UiItemKind::kRadioCluster: 
					if(stage == insertctrlstageMain) {
						TRect rc;
						const uint gnrr = SUiLayoutParam::GetNominalRectWithDefaults(&lp, rc, 60.0f, 60.0f);
						rCtx.GetConst_String(p_scope, DlScope::cuifCtrlText, ctl_text);
						TCluster * p_ctl = new TCluster(rc, RADIOBUTTONS, 0/*spcFlags*/, ctl_text, 0);
						{
							const DlScopeList & r_item_scope_list = p_scope->GetChildList();
							for(uint cii = 0; cii < r_item_scope_list.getCount(); cii++) {
								const DlScope * p_ci = r_item_scope_list.at(cii);
								if(p_ci) {
									if(rCtx.GetConst_String(p_ci, DlScope::cuifCtrlText, temp_buf)) {
										TRect rc_item;
										SUiLayoutParam lp_item;
										const bool glb_item_r = rCtx.GetLayoutBlock(p_ci, DlScope::cuifLayoutBlock, &lp_item);
										const uint gnrr_item = SUiLayoutParam::GetNominalRectWithDefaults(&lp_item, rc_item, static_cast<float>(rc.width() - 2), 16.0f);
										p_ctl->AddItem(-1, temp_buf, &rc_item);
									}
								}
							}
						}
						pDlg->InsertCtlWithCorrespondingNativeItem(p_ctl, item_id, 0, /*extraPtr*/0);
						ctl_id_for_tab = item_id; // @v12.3.9
						is_inserted = true;
					}
					break;
				case UiItemKind::kCombobox:
					if(stage == insertctrlstageMain) {
						TRect rc;
						const uint gnrr = SUiLayoutParam::GetNominalRectWithDefaults(&lp, rc, 60.0f, 60.0f);
						uint32 cb_line_id = 0;
						rCtx.GetConst_String(p_scope, DlScope::cuifCbLineSymb, temp_buf);
						rCtx.GetConst_Uint32(p_scope, DlScope::cuifCbLineSymbIdent, cb_line_id);
						TInputLine * p_il = new TInputLine(rc, 0/*spcFlags*/, S_ZSTRING, MKSFMT(128, 0));
						{
							const uint32 local_ctl_id = cb_line_id ? cb_line_id : (++rLastDynId);
							p_il->SetId(local_ctl_id);
							ctl_id_for_tab = local_ctl_id;
						}
						if(ui_flags & UiItemKind::fTabStop) {
							p_il->setState(sfTabStop, true);
						}
						TRect rc_cb;
						rc_cb.a.x = rc.b.x+1;
						rc_cb.b.x = rc_cb.a.x+1 + rc.height();
						rc_cb.a.y = rc.a.y;
						rc_cb.b.y = rc.b.y;
						ComboBox * p_cb = new ComboBox(rc_cb, cbxAllowEmpty|cbxDisposeData|cbxListOnly, p_il);
						p_cb->SetId(item_id);
						{
							// Label
							if(rCtx.GetConst_String(p_scope, DlScope::cuifCtrlText, ctl_text)) {
								//cuifCtrlLblRect,  // raw    Координаты текстового ярлыка, ассоциированного с управляющим элементом
								//cuifCtrlLblSymb,  // string Символ текстового ярлыка, ассоциированного с управляющим элементом
								//cuifLabelRect,    // raw(UiRelRect) Положение текстового ярлыка, ассоциированного с управляющим элементом
								//cuifLblLayoutBlock
								SUiLayoutParam lp_label;
								const bool glb_label_r = rCtx.GetLayoutBlock(p_scope, DlScope::cuifLblLayoutBlock, &lp_label);
								TRect rc_label;
								const uint gnrr_label = SUiLayoutParam::GetNominalRectWithDefaults(&lp_label, rc_label, 60.0f, 13.0f);
								TLabel * p_lbl = new TLabel(rc_label, ctl_text, p_il);
								pDlg->InsertCtlWithCorrespondingNativeItem(p_lbl, ++rLastDynId, 0, /*extraPtr*/0);
							}
						}
						pDlg->InsertCtlWithCorrespondingNativeItem(p_cb, item_id, 0, /*extraPtr*/0);
						is_inserted = true;
						// @v12.4.1 {
						if(supplement.Kind == SUiCtrlSupplement::kList) {
							if(supplement.Ident) {
								TRect rc_sb;
								rc_sb.a.x = rc_cb.b.x+1;
								rc_sb.a.y = rc_cb.a.y;
								rc_sb.b.x = rc_sb.a.x + 60; // Длина кнопки 60px
								rc_sb.b.y = rc_sb.a.y;
								uint   pic_id = 0;
								switch(supplement.Kind) {
									case SUiCtrlSupplement::kList: 
										pic_id = 0; 
										if(supplement.Text.IsEmpty()) {
											PPLoadStringUtf8("but_list", supplement.Text);
										}
										break; 
								}
								TButton * p_sb = new TButton(rc_sb, supplement.Text, supplement.Cmd, 0, pic_id);
								p_sb->SetSupplementFactors(supplement.Kind, item_id);
								pDlg->InsertCtlWithCorrespondingNativeItem(p_sb, supplement.Ident, 0, /*extraPtr*/0);
							}
						}
						// } @v12.4.1 
					}
					break;
				case UiItemKind::kListbox: 
					if(stage == insertctrlstageMain) {
						TRect rc;
						const uint gnrr = SUiLayoutParam::GetNominalRectWithDefaults(&lp, rc, 60.0f, 60.0f);

						SString column_description;
						ListBoxDef * p_lb_def = 0;
						rCtx.GetConst_String(p_scope, DlScope::cuifListBoxColumns, column_description);
						if(column_description.NotEmpty()) {
							PPExpandString(column_description, CTRANSF_UTF8_TO_INNER);
						}
						SmartListBox * p_lb = column_description.NotEmpty() ? new SmartListBox(rc, p_lb_def, column_description) : new SmartListBox(rc, p_lb_def, false/*is_tree*/);
						if(p_lb) {
							//LldState |= lldsDefBailed;
							if(ui_flags & UiItemKind::fTabStop) {
								p_lb->setState(sfTabStop, true);
							}
							pDlg->InsertCtlWithCorrespondingNativeItem(p_lb, item_id, 0, /*extraPtr*/0);
							// @v12.3.10 {
							if(is_ui_cfg_valid && ui_cfg.ListFont.IsDefined()) {
								pDlg->SetCtrlFont(item_id, ui_cfg.ListFont);
							}
							// } @v12.3.10 
							ctl_id_for_tab = item_id; // @v12.3.9
							is_inserted = true;
							//if(font_face.NotEmpty()) {
								//SetCtrlFont(STDCTL_SINGLELISTBOX, font_face, /*16*//*22*/12);
							//}
						}
					}
					break;
				case UiItemKind::kTreeListbox: 
					if(stage == insertctrlstageMain) {
						TRect rc;
						const uint gnrr = SUiLayoutParam::GetNominalRectWithDefaults(&lp, rc, 60.0f, 60.0f);

						SString column_description;
						StdTreeListBoxDef * p_lb_def = new StdTreeListBoxDef(0, lbtDisposeData|lbtDblClkNotify, 0);
						SmartListBox * p_lb = new SmartListBox(rc, p_lb_def, true/*is_tree*/);
						if(p_lb) {
							//LldState |= lldsDefBailed;
							if(ui_flags & UiItemKind::fTabStop) {
								p_lb->setState(sfTabStop, true);
							}
							pDlg->InsertCtlWithCorrespondingNativeItem(p_lb, item_id, 0, /*extraPtr*/0);
							// @v12.3.12 {
							if(is_ui_cfg_valid && ui_cfg.ListFont.IsDefined()) {
								pDlg->SetCtrlFont(item_id, ui_cfg.ListFont);
							}
							// } @v12.3.12 
							ctl_id_for_tab = item_id; // @v12.3.9
							is_inserted = true;
							//if(font_face.NotEmpty()) {
								//SetCtrlFont(STDCTL_SINGLELISTBOX, font_face, /*16*//*22*/12);
							//}
						}
					}
					break;
				case UiItemKind::kFrame: 
					if(stage == insertctrlstagePostprocess) {
						TRect rc;
						const uint gnrr = SUiLayoutParam::GetNominalRectWithDefaults(&lp, rc, 60.0f, 60.0f);
						//rc.b.x = rc.a.x + 20; // @debug
						//rc.b.y = rc.a.y + 20; // @debug
						rCtx.GetConst_String(p_scope, DlScope::cuifCtrlText, ctl_text);
						TGroupBox * p_gb = new TGroupBox(rc);
						p_gb->SetText(ctl_text);
						pDlg->InsertCtlWithCorrespondingNativeItem(p_gb, item_id, 0, /*extraPtr*/0);
						is_inserted = true;
					}
					InsertControlItems(pDlg, rCtx, *p_scope, rLastDynId, rFiBlk, stage); // @recursion // Внутри могут быть элементы, которые вставляются на фазе insertctrlstageMain,
						// по этому выводим данный вызов за рамки проверки if(stage == insertctrlstagePostprocess)
					break;
				case UiItemKind::kLabel: 
					if(stage == insertctrlstageMain) {
					}
					break;
				case UiItemKind::kRadiobutton: 
					if(stage == insertctrlstageMain) {
					}
					break;
				case UiItemKind::kGenericView: 
					InsertControlItems(pDlg, rCtx, *p_scope, rLastDynId, rFiBlk, stage); // @recursion
					is_inserted = true;
					break;
				case UiItemKind::kImageView: 
					if(stage == insertctrlstageMain) {
						TRect rc;
						const uint gnrr = SUiLayoutParam::GetNominalRectWithDefaults(&lp, rc, 60.0f, 60.0f);
							
						SString img_symb;
						rCtx.GetConst_String(p_scope, DlScope::cuifImageSymb, img_symb);
						rCtx.GetConst_String(p_scope, DlScope::cuifCtrlText, ctl_text);
						TImageView * p_ctl = new TImageView(rc, img_symb);
						pDlg->InsertCtlWithCorrespondingNativeItem(p_ctl, item_id, 0, /*extraPtr*/0);
						is_inserted = true;
					}
					break;
			}
			// @v12.3.7 {
			if(item_id && is_inserted) {
				if(!rFiBlk.FirstTabbedItemId && ui_flags & UiItemKind::fTabStop)
					rFiBlk.FirstTabbedItemId = ctl_id_for_tab;
				if(!rFiBlk.FirstItemId)
					rFiBlk.FirstItemId = ctl_id_for_tab;
			}
			// @v12.3.7 {
		}
	}
	if(stage == insertctrlstageMain) {
		if(def_button_ctl_id) {
			pDlg->SetDefaultButton(def_button_ctl_id, true);
		}
	}
}

static const float FixedButtonY = 21.0f;
static const float FixedButtonX = FixedButtonY;
static const float FixedInputY = 21.0f;
static const float FixedLabelY = 13.0f;

bool PPDialogConstructor::MakeComplexLayout_InputLine(TDialog * pDlg, TView * pView, SUiLayoutParam & rLp, DlContext & rCtx, const DlScope * pScope, SUiLayout * pLoParent)
{
	//
	// Descr: Варианты дизайна комплекса {строка ввода, этикетка, [supplement]}
	//
	enum {
		designA = 1, // этикетка над строкой ввода
		designB = 2, // этикетка слева от строки ввода
	};
	bool   done = false;
	SString temp_buf;
	if(TView::IsSubSign(pView, TV_SUBSIGN_INPUTLINE)) {
		int    design = designA;
		SUiCtrlSupplement_With_Symbols supplement;
		{
			rCtx.GetConst_String(pScope, DlScope::cuifDesign, temp_buf);
			if(temp_buf.IsEqiAscii("B"))
				design = designB;
			else 
				design = designA;
		}
		{
			CtmExprConst __c = pScope->GetConst(DlScope::cuifSupplement);
			if(!!__c) {
				uint8    c_buf[256];
				if(rCtx.GetConstData(__c, c_buf, sizeof(c_buf))) {
					supplement = *reinterpret_cast<const SUiCtrlSupplement *>(c_buf);
					if(supplement.Kind) {
						rCtx.GetConst_String(pScope, DlScope::cuifSupplementSymb, supplement.Symb);
						rCtx.GetConst_String(pScope, DlScope::cuifSupplementCmdSymb, supplement.CmdSymb);
						rCtx.GetConst_String(pScope, DlScope::cuifSupplementText, supplement.Text);
					}
				}
			}
		}
		TInputLine * p_il = static_cast<TInputLine *>(pView);
		TView * p_lbl = pDlg->GetCtrlLabel(p_il);
		TView * p_supplemental_view = 0; // supplemental-view
		SUiLayoutParam __lp_ib(DIREC_HORZ); // Лейаут для пары {поле ввода; supplemental-кнопка}
		SUiLayoutParam * p_lb_ib = 0; // Указатель на лейаут для пары {поле ввода; supplemental-кнопка}. if p_lb_ib != 0 then нужно воткнуть supplement-button
		//SUiLayoutParam lp_supplement_button; // Лейаут для supplemental-кнопки
		SPoint2F sb_sz; // Размер supplemental-button
		float inp_width = 0.0f;
		float inp_height = 0.0f;
		float label_width = 0.0f;
		float label_height = 0.0f;
		const int inp_szx = rLp.GetSizeX(&inp_width);
		const int inp_szy = rLp.GetSizeY(&inp_height);

		SUiLayoutParam lp_label_org; // Параметры label, созданные описанием
		const bool glb_label_org_r = rCtx.GetLayoutBlock(pScope, DlScope::cuifLblLayoutBlock, &lp_label_org);
		const int label_szx = lp_label_org.GetSizeX(&label_width);
		const int label_szy = lp_label_org.GetSizeY(&label_height);
		if(lp_label_org.LinkRelation) {
			;//debug_mark = true;
		}
		if(supplement.Kind && supplement.Ident) {
			p_supplemental_view = pDlg->getCtrlView(supplement.Ident);
			if(!TView::IsSubSign(p_supplemental_view, TV_SUBSIGN_BUTTON))
				p_supplemental_view = 0;
		}
		if(p_lbl) {
			// SUiLayoutParam lp_label(lp_label_org); // @v12.3.9 lp_label()-->lp_label(glb_label_org_r)
			SUiLayoutParam lp_label;
			//
			TRect  rc_label;
			const uint gnrr = SUiLayoutParam::GetNominalRectWithDefaults(&lp_label, rc_label, 60.0f, TInputLine::DefLabelHeight);
			if(inp_szy == SUiLayoutParam::szFixed || inp_szy == 0) {
				if(inp_height == 0.0f) {
					inp_height = TInputLine::DefHeight;
				}
			}
			else {
				; // ?
			}
			//
			if(p_supplemental_view) {
				p_lb_ib = &__lp_ib;
				sb_sz.Set(20.0f, inp_height);
			}
			if(design == designB) {
				/*
					Дизайн B предполагает следующее:
					-- весь комплекс располагается горизонтально. 
					-- полная ширина (size_x) комплекса определяется шириной контейнера!
					-- Ширина поля ввода:
						-- if ширина поля ввода (основной size_x) задана фиксированной, то принимается эта величина
						-- elseif growfactor поля ввода (основной growfactor) задан, то он применяется собственно к полю ввода внутри комплекса
						-- else ???
					-- Ширина этикетки:
						-- if ширина этикетки (labelsize_x) задана фиксированной, то принимается она
				*/ 
				SUiLayoutParam glp(DIREC_HORZ); // Группирующий лейаут для строки ввода и подписи (label)
				//
				glp.SetFixedSizeY(inp_height);
				glp.SetVariableSizeX(SUiLayoutParam::szByContainer, 1.0f);
				glp.Margin = rLp.Margin;
				rLp.Margin.Z();
				SUiLayout * p_lo_inp_grp = pLoParent->InsertItem(0, &glp);
				if(p_lo_inp_grp) {
					SUiLayoutParam lp_il; // inputline
					
					lp_label.SetVariableSizeY(SUiLayoutParam::szByContainer, 1.0f);
					lp_il.SetVariableSizeY(SUiLayoutParam::szByContainer, 1.0f);
					if(p_lb_ib) {
						// @todo Сейчас этот участок повторяет случай (!p_lb_ib). Надо supplement-button воткнуть.
						if(inp_szx == SUiLayoutParam::szFixed) {
							lp_il.SetFixedSizeX(inp_width);
							lp_label.SetFixedSizeX(0.0f);
							lp_label.GrowFactor = 1.0;
						}
						else if(inp_szx == SUiLayoutParam::szByContainer) {
							lp_il.SetVariableSizeX(inp_szx, inp_width);
							lp_il.Margin.a.x = 4.0f;
							lp_label.SetVariableSizeX(SUiLayoutParam::szByContainer, 1.0f - inp_width);
						}
						else if(label_szx == SUiLayoutParam::szFixed) {
							lp_il.SetFixedSizeX(0.0f);
							lp_il.Margin.a.x = 4.0f;
							lp_label.SetFixedSizeX(label_width);
						}
						else {
							lp_il.SetFixedSizeX(0.0f);
							lp_label.SetFixedSizeX(0.0f);
							lp_label.GrowFactor = 1.0;							
						}
						lp_il.Margin.a.x = 4.0f;
						InsertCtrlLayout(pDlg, p_lo_inp_grp, p_lbl, lp_label);
						InsertCtrlLayout(pDlg, p_lo_inp_grp, p_il, lp_il);
						done = true;
					}
					else {
						if(inp_szx == SUiLayoutParam::szFixed) {
							lp_il.SetFixedSizeX(inp_width);
							lp_label.SetFixedSizeX(0.0f);
							lp_label.GrowFactor = 1.0;
						}
						else if(inp_szx == SUiLayoutParam::szByContainer) {
							lp_il.SetVariableSizeX(SUiLayoutParam::szByContainer, inp_width);
							lp_il.Margin.a.x = 4.0f;
							lp_label.SetVariableSizeX(SUiLayoutParam::szByContainer, 1.0f - inp_width);
						}
						else if(label_szx == SUiLayoutParam::szFixed) {
							lp_il.SetFixedSizeX(0.0f);
							lp_il.Margin.a.x = 4.0f;
							lp_label.SetFixedSizeX(label_width);
						}
						else {
							lp_il.SetFixedSizeX(0.0f);
							lp_label.SetFixedSizeX(0.0f);
							lp_label.GrowFactor = 1.0;							
						}
						lp_il.Margin.a.x = 4.0f;
						InsertCtrlLayout(pDlg, p_lo_inp_grp, p_lbl, lp_label);
						InsertCtrlLayout(pDlg, p_lo_inp_grp, p_il, lp_il);
						done = true;
					}
				}
			}
			else if(design == designA) {
				SUiLayoutParam glp(DIREC_VERT); // Группирующий лейаут для строки ввода и подписи (label)
				//
				glp.SetFixedSizeY(inp_height + rc_label.height() + 1.0f);
				//
				// Пока для случая supplement сделаем отдельную ветку кода. Потом унифицируем.
				// 
				if(p_lb_ib) {
					SUiLayoutParam lp_sb; // supplement-button
					SUiLayoutParam lp_il; // inputline
					lp_sb.SetFixedSizeX(sb_sz.x);
					lp_sb.SetFixedSizeY(sb_sz.y);
					lp_sb.ShrinkFactor = 0.0f;

					lp_il.SetVariableSizeX(SUiLayoutParam::szByContainer, 1.0f);
					lp_il.SetFixedSizeY(FixedInputY);

					lp_label.SetVariableSizeX(SUiLayoutParam::szByContainer, 1.0f);
					lp_label.SetFixedSizeY(static_cast<float>(rc_label.height()));

					glp.Margin = rLp.Margin;
					rLp.CopySizeXParamTo(glp);
					glp.SetGrowFactor(rLp.GrowFactor);
					rLp.GrowFactor = 0.0f; // @v12.3.7

					SUiLayout * p_lo_inp_grp = pLoParent->InsertItem(0, &glp);
					InsertCtrlLayout(pDlg, p_lo_inp_grp, p_lbl, lp_label);
					SUiLayout * p_lo_ib_grp = p_lo_inp_grp->InsertItem(0, p_lb_ib);
					InsertCtrlLayout(pDlg, p_lo_ib_grp, p_il, lp_il);
					InsertCtrlLayout(pDlg, p_lo_ib_grp, p_supplemental_view, lp_sb);
					done = true;
				}
				else {
					if(inp_szx == SUiLayoutParam::szFixed) {
						glp.SetFixedSizeX(inp_width + 2.0f);
						rLp.SetVariableSizeX(SUiLayoutParam::szByContainer, 1.0f);
					}
					else if(inp_szx == SUiLayoutParam::szByContainer) {
						glp.SetVariableSizeX(SUiLayoutParam::szByContainer, inp_width);
						rLp.SetVariableSizeX(SUiLayoutParam::szByContainer, 1.0f);
					}
					else if(rLp.GrowFactor >= 0.0f) {
						glp.SetGrowFactor(rLp.GrowFactor);
						rLp.SetGrowFactor(0.0f);
					}
					else {
						// полная ширина контейнера
						glp.SetVariableSizeX(SUiLayoutParam::szByContainer, 1.0f);
						rLp.SetVariableSizeX(SUiLayoutParam::szByContainer, 1.0f);
					}
					//
					lp_label.SetVariableSizeX(SUiLayoutParam::szByContainer, 1.0f);
					lp_label.SetFixedSizeY((rc_label.height() > 0.0f) ? static_cast<float>(rc_label.height()) : TInputLine::DefLabelHeight);
					lp_label.ShrinkFactor = 0.0f;
					//
					//lp_supplement_button.SetFixedSizeX(20.0f); // @v12.3.7
					glp.ShrinkFactor = 0.0f;
					glp.Flags &= ~(SUiLayoutParam::fContainerWrap|SUiLayoutParam::fContainerWrapReverse);
					rLp.SetFixedSizeY(inp_height);
					rLp.ShrinkFactor = 0.0f;
					glp.Margin = rLp.Margin;
					rLp.Margin.Z();
					SUiLayout * p_lo_inp_grp = pLoParent->InsertItem(0, &glp);
					if(p_lo_inp_grp) {
						InsertCtrlLayout(pDlg, p_lo_inp_grp, p_lbl, lp_label);
						InsertCtrlLayout(pDlg, p_lo_inp_grp, p_il, rLp);
						done = true;
					}
				}
			}
		}
		else { // Этикетки нет 
			if(p_supplemental_view) {
				p_lb_ib = &__lp_ib;
				sb_sz.Set(20.0f, inp_height);
				//
				if(inp_szy == SUiLayoutParam::szFixed || inp_szy == 0) {
					if(inp_height == 0.0f) {
						inp_height = TInputLine::DefHeight;
					}
					p_lb_ib->SetFixedSizeY(inp_height);
				}
				else {
					; // ?
				}
				//
				SUiLayoutParam lp_sb;
				SUiLayoutParam lp_il;
				lp_sb.SetFixedSizeX(sb_sz.x);
				lp_sb.SetFixedSizeY(sb_sz.y);
				lp_sb.ShrinkFactor = 0.0f;

				lp_il.SetVariableSizeX(SUiLayoutParam::szByContainer, 1.0f);
				lp_il.SetFixedSizeY(FixedInputY);

				//lp_label.SetVariableSizeX(SUiLayoutParam::szByContainer, 1.0f);
				//lp_label.SetFixedSizeY(static_cast<float>(rc_label.height()));

				p_lb_ib->Margin = rLp.Margin;
				rLp.CopySizeXParamTo(*p_lb_ib);
				p_lb_ib->SetGrowFactor(rLp.GrowFactor);
				rLp.GrowFactor = 0.0f;

				SUiLayout * p_lo_inp_grp = pLoParent->InsertItem(0, p_lb_ib);
				//InsertCtrlLayout(pDlg, p_lo_inp_grp, p_lbl, lp_label);
				//SUiLayout * p_lo_ib_grp = p_lo_inp_grp->InsertItem(0, p_lb_ib);
				InsertCtrlLayout(pDlg, p_lo_inp_grp, p_il, lp_il);
				InsertCtrlLayout(pDlg, p_lo_inp_grp, p_supplemental_view, lp_sb);
				done = true;
			}
			else {
				SUiLayout * p_lo = InsertCtrlLayout(pDlg, pLoParent, p_il, rLp);
				done = true;
			}
		}
	}
	return done;
}

void PPDialogConstructor::InsertControlLayouts(TDialog * pDlg, DlContext & rCtx, const DlScope & rParentScope, SUiLayout * pLoParent)
{
	bool   debug_mark = false; // @debug
	if(pLoParent) {
		const DlScopeList & r_sc_list = rParentScope.GetChildList();
		SUiCtrlSupplement_With_Symbols supplement; // @v12.3.7
		for(uint i = 0; i < r_sc_list.getCount(); i++) {
			const DlScope * p_scope = r_sc_list.at(i);
			if(p_scope) {
				SUiLayoutParam lp;
				if(rCtx.GetLayoutBlock(p_scope, DlScope::cuifLayoutBlock, &lp)) {
					const int container_direc = lp.GetContainerDirection();
					//
					SUiLayout * p_lo = 0;
					uint32 vk = 0;
					uint32 symb_ident = 0;
					supplement.Z();
					rCtx.GetConst_Uint32(p_scope, DlScope::cuifViewKind, vk);
					rCtx.GetConst_Uint32(p_scope, DlScope::cucmSymbolIdent, symb_ident);
					{
						CtmExprConst __c = p_scope->GetConst(DlScope::cuifSupplement);
						if(!!__c) {
							uint8    c_buf[256];
							if(rCtx.GetConstData(__c, c_buf, sizeof(c_buf))) {
								supplement = *reinterpret_cast<const SUiCtrlSupplement *>(c_buf);
								if(supplement.Kind) {
									rCtx.GetConst_String(p_scope, DlScope::cuifSupplementSymb, supplement.Symb);
									rCtx.GetConst_String(p_scope, DlScope::cuifSupplementCmdSymb, supplement.CmdSymb);
									rCtx.GetConst_String(p_scope, DlScope::cuifSupplementText, supplement.Text);
								}
							}
						}
					}
					const uint32 item_id = NZOR(symb_ident, GetScopeID(p_scope));
					TView * p_view = pDlg->getCtrlView(item_id);
					if(p_view) {
						bool   done = false;
						if(oneof3(vk, UiItemKind::kCheckCluster, UiItemKind::kRadioCluster, UiItemKind::kCheckbox)) {
							if(p_view->IsSubSign(TV_SUBSIGN_CLUSTER)) {
								TCluster * p_clu = static_cast<TCluster *>(p_view);
								const bool is_single = LOGIC(p_clu->GetSpcFlags() & TCluster::spcfSingleItemWithoutFrame);
								const float fixed_item_y = TCluster::DefItemHeight;
								const float item_gap_y = TCluster::DefItemVerticalGap;
								const float padding_top = is_single ? 0.0f : TCluster::DefClusterPaddigTop;
								const float padding_bottom = is_single ? 0.0f : TCluster::DefClusterPaddigBottom;
								int clu_direction = lp.GetContainerDirection();
								if(clu_direction != DIREC_HORZ) {
									clu_direction = DIREC_VERT;
									lp.SetContainerDirection(clu_direction);
								}
								float cluster_size_y = padding_top;
								for(uint item_idx = 0; item_idx < p_clu->getNumItems(); item_idx++) {
									const TCluster::Item * p_item = p_clu->GetItemC(item_idx);
									if(p_item)
										cluster_size_y += (fixed_item_y + item_gap_y);
								}
								cluster_size_y += padding_bottom;
								lp.SetFixedSizeY(cluster_size_y);
								InsertCtrlLayout(pDlg, pLoParent, p_clu, lp);
								done = true;
							}
						}
						else if(vk == UiItemKind::kListbox) {
							SmartListBox * p_lb = static_cast<SmartListBox *>(p_view);
							if(!p_lb->IsMultiColumn() && !p_lb->IsTreeList()) // Припуск для скролл-бара не нужен для мультиколоночного списка и для treeview
								lp.Margin.b.x += 20.0f; 
							p_lo = InsertCtrlLayout(pDlg, pLoParent, p_view, lp);
							done = true;
						}
						else if(vk == UiItemKind::kTreeListbox) {
							p_lo = InsertCtrlLayout(pDlg, pLoParent, p_view, lp);
							done = true;
						}
						else if(vk == UiItemKind::kImageView) {
							p_lo = InsertCtrlLayout(pDlg, pLoParent, p_view, lp);
							done = true;
						}
						else if(vk == UiItemKind::kCombobox) {
							if(p_view->IsSubSign(TV_SUBSIGN_COMBOBOX)) {
								ComboBox * p_cb = static_cast<ComboBox *>(p_view);
								TInputLine * p_il = p_cb->GetLink();
								if(p_il) {
									// @v12.4.1 {
									TView * p_supplemental_view = 0;
									SPoint2F sb_sz; // Размер supplemental-button
									if(supplement.Kind && supplement.Ident) {
										p_supplemental_view = pDlg->getCtrlView(supplement.Ident);
										if(TView::IsSubSign(p_supplemental_view, TV_SUBSIGN_BUTTON)) {
											if(supplement.Kind == SUiCtrlSupplement::kList) {
												sb_sz.Set(60.0f, FixedButtonY);
											}
											else {
												sb_sz.Set(20.0f, FixedButtonY);
											}
										}
										else {
											p_supplemental_view = 0;
										}
									}
									// } @v12.4.1 
									TView * p_lbl = pDlg->GetCtrlLabel(p_il);
									if(p_lbl) {
										SUiLayoutParam glp(DIREC_VERT); // Общий лейаут для всей группы
										SUiLayoutParam lp_ib(DIREC_HORZ); // Лейаут для группы {поле ввода; кнопка[; supplement_button]}
										SUiLayoutParam lp_label;
										SUiLayoutParam lp_button;
										SUiLayoutParam lp_supplement_button; // @v12.4.1
										SUiLayoutParam lp_il;
										TRect  rc_label;
										glp.Margin = lp.Margin;
										lp.CopySizeXParamTo(glp);
										// @v12.3.7 {
										glp.SetGrowFactor(lp.GrowFactor); 
										lp.GrowFactor = 0.0f;
										// } @v12.3.7 
										lp_label.SetVariableSizeX(SUiLayoutParam::szByContainer, 1.0f);
										lp_label.SetFixedSizeY(FixedLabelY);
										lp_button.SetFixedSizeX(FixedButtonX);
										lp_button.SetFixedSizeY(FixedButtonY);
										lp_button.ShrinkFactor = 0.0f;
										lp_il.SetVariableSizeX(SUiLayoutParam::szByContainer, 1.0f);
										lp_il.SetFixedSizeY(FixedInputY);
										lp_ib.SetFixedSizeY(MAX(FixedInputY, FixedButtonY));
										glp.SetFixedSizeY(MAX(FixedInputY, FixedButtonY) + FixedLabelY);

										SUiLayout * p_lo_cb_grp = pLoParent->InsertItem(0, &glp);
										InsertCtrlLayout(pDlg, p_lo_cb_grp, p_lbl, lp_label);
										SUiLayout * p_lo_ib_grp = p_lo_cb_grp->InsertItem(0, &lp_ib);
										InsertCtrlLayout(pDlg, p_lo_ib_grp, p_il, lp_il);
										InsertCtrlLayout(pDlg, p_lo_ib_grp, p_cb, lp_button);
										// @v12.4.1 {
										if(p_supplemental_view) {
											lp_supplement_button.SetFixedSizeX(sb_sz.x);
											lp_supplement_button.SetFixedSizeY(sb_sz.y);
											lp_supplement_button.ShrinkFactor = 0.0f;
											InsertCtrlLayout(pDlg, p_lo_ib_grp, p_supplemental_view, lp_supplement_button);
										}
										// } @v12.4.1 
										done = true;
									}
									else {
										
									}
								}
							}
						}
						else if(vk == UiItemKind::kInput) {
							done = MakeComplexLayout_InputLine(pDlg, p_view, lp, rCtx, p_scope, pLoParent);
						}
						if(!done) {
							p_lo = InsertCtrlLayout(pDlg, pLoParent, p_view, lp);
						}
					}
					else {
						if(vk == UiItemKind::kGenericView) {
							p_lo = InsertCtrlLayout(pDlg, pLoParent, static_cast<TView *>(0), lp);
						}
					}
					//
					if(oneof2(container_direc, DIREC_HORZ, DIREC_VERT)) {
						InsertControlLayouts(pDlg, rCtx, *p_scope, p_lo); // @recursion
					}
				}
			}
		}
	}
}
//
//
//
//typedef int (*InitializeDialogFunc)(TDialog * pThis, const void * pIdent, void * extraPtr); // @v12.3.6
int PPInitializeDialogFunc(TDialog * pThis, const void * pIdent, void * extraPtr) // @v12.3.6
{
	int    ok = -1;
	const size_t ident_len = sstrnlen(static_cast<const char *>(pIdent), 128);
	if(ident_len && sisascii(static_cast<const char *>(pIdent), ident_len)) {
		const DlScope * p_scope = 0;
		DlContext * p_ctx = 0;
		SString ident_buf(static_cast<const char *>(pIdent));
		if(ident_buf.IsDec()) {
			uint   id = ident_buf.ToULong();
			if(id) {
				p_ctx = DS.GetInterfaceContext(PPSession::ctxUiViewLocal);
				if(p_ctx) {
					p_scope = p_ctx->GetDialogScopeBySymbolIdent_Const(id);
				}
				if(!p_scope) {
					p_ctx = DS.GetInterfaceContext(PPSession::ctxUiView);
					if(p_ctx)
						p_scope = p_ctx->GetDialogScopeBySymbolIdent_Const(id);
				}
			}
		}
		else {
			p_ctx = DS.GetInterfaceContext(PPSession::ctxUiViewLocal);
			if(p_ctx) {
				p_scope = p_ctx->GetScopeByName_Const(DlScope::kUiView, ident_buf);
			}
			if(!p_scope) {
				p_ctx = DS.GetInterfaceContext(PPSession::ctxUiView);
				if(p_ctx)
					p_scope = p_ctx->GetScopeByName_Const(DlScope::kUiView, ident_buf);
			}
		}
		if(p_scope) {
			assert(p_ctx);
			PPDialogConstructor ctr(pThis, *p_ctx, p_scope);
			ok = 1;
		}
	}
	return ok;
}

#if 0 // @v12.3.10 {
int Test_ExecuteDialogByDl600Description() // @construction
{
	class TDialogDL6_Construction : public TDialog {
	public:
		TDialogDL6_Construction(DlContext & rCtx, const char * pSymb) : TDialog(0, TWindow::wbcDrawBuffer|TWindow::wbcStorableUserParams, TDialog::coEmpty)
		{
			PPDialogConstructor ctr(this, rCtx, pSymb);
		}
	};
	int    ok = 0;
	const char * p_bin_file_name = "ppdlgs-local.bin"; //"ppdlg2.bin";
	TDialogDL6_Construction * dlg = 0;
	SString temp_buf;
	SString file_name;
	PPGetFilePath(PPPATH_BIN, p_bin_file_name, file_name);
	if(fileExists(file_name)) {
		DlContext ctx;
		if(ctx.Init(file_name)) {
			// "DLG_ADDRESS" "DLG_BILLFLT" "DLG_PERSON" "DLG_PASSWORD"
			const char * p_dlg_symb = "DLG_LO_EXPERIMENTAL";//"DLG_GGVIEW";
			const DlScope * p_scope = ctx.GetDialogScopeBySymbolIdent_Const(DLG_GGVIEW);
			dlg = new TDialogDL6_Construction(ctx, p_dlg_symb);
			//
			int16 nr_in = 113;
			int16 nr_out = 0;
			double capacity = 16.123456789;
			SString text_in("abc");
			SString text_out;
			dlg->setCtrlString(CTL_LOCATION_NAME, text_in);
			dlg->setCtrlData(CTL_LOCATION_NUMROWS, &nr_in);
			dlg->setCtrlData(CTL_LOCATION_CAPACITY, &capacity);
			
			if(ExecView(dlg) == cmOK) {
				dlg->getCtrlString(CTL_LOCATION_NAME, text_out);
				dlg->getCtrlData(CTL_LOCATION_NUMROWS, &nr_out);
			}
			ZDELETE(dlg);
			ok = -1;
		}
#if 0 // {
		if(Sc.GetFirstChildByKind(DlScope::kUiView, 1)) {
			SFsPath ps;
			ps.Split(InFileName);
			ps.Nam.CatChar('-').Cat("out");
			ps.Ext = "txt";
			ps.Merge(temp_buf);
			SFile f_view_out(temp_buf, SFile::mWrite);
			for(uint fscidx = 0; fscidx < file_sc_list.getCount(); fscidx++) {
				const long file_scope_id = file_sc_list.get(fscidx);
				const DlScope * p_file_scope = GetScope(file_scope_id, 0);
				if(p_file_scope) {
					scope_id_list.freeAll();
					p_file_scope->GetChildList(DlScope::kUiView, /*1*/0/*recursive*/, &scope_id_list);
					for(i = 0; i < scope_id_list.getCount(); i++) {
						const DlScope * p_scope = GetScope(scope_id_list.at(i), 0);
						line_buf.Z();
						if(Write_UiView(p_scope, 0, line_buf) > 0) {
							f_view_out.WriteLine(line_buf.CR());
						}
					}
				}
			}
		}
#endif // } 0
	}
	return ok;
}
#endif // } 0 @v12.3.10