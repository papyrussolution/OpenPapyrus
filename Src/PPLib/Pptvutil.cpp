// PPTVUTIL.CPP
// Copyright (c) A.Sobolev 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019
//
#include <pp.h>
#pragma hdrstop

//int    modeless = GetModelessStatus();
int    FASTCALL GetModelessStatus(int outerModeless) { return BIN(outerModeless); }
TView * SLAPI ValidView(TView * pView) { return APPL->validView(pView); }
ushort FASTCALL ExecView(TWindow * pView) { return pView ? APPL->P_DeskTop->execView(pView) : cmError; } // @v9.0.4 TView-->TWindow

ushort FASTCALL ExecViewAndDestroy(TWindow * pView) // @v9.0.4 TView-->TWindow
{
	ushort r = pView ? APPL->P_DeskTop->execView(pView) : cmError;
	delete pView;
	return r;
}

ushort FASTCALL CheckExecAndDestroyDialog(TDialog * pDlg, int genErrMsg, int toCascade)
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
	if(v) {
		const uint last_cmd = static_cast<PPApp *>(APPL)->LastCmd;
		v->SetToolbarID(last_cmd ? (last_cmd + TOOLBAR_OFFS) : 0);
		return APPL->P_DeskTop->execView(v);
	}
	else
		return cmError;
	//return v ? (v->SetToolbarID(last_cmd ? (last_cmd + TOOLBAR_OFFS) : 0), APPL->P_DeskTop->execView(v)) : cmError;
}

int FASTCALL InsertView(TBaseBrowserWindow * v)
{
	if(v) {
		const uint last_cmd = static_cast<PPApp *>(APPL)->LastCmd;
		v->SetToolbarID(last_cmd ? (last_cmd + TOOLBAR_OFFS) : 0);
		APPL->P_DeskTop->Insert_(v);
		return v->Insert();
	}
	else
		return 0;
}

int SLAPI InitSTimeChunkBrowserParam(const char * pSymbol, STimeChunkBrowser::Param * pParam)
{
	int    ok = 1;
	if(pParam) {
		pParam->Z();
		pParam->RegSaveParam = pSymbol;
		{
			UserInterfaceSettings ui_cfg;
			if(ui_cfg.Restore() > 0 && ui_cfg.Flags & UserInterfaceSettings::fTcbInterlaced)
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
BrowserWindow * SLAPI PPFindLastBrowser() { return APPL ? static_cast<BrowserWindow *>(APPL->FindBrowser(static_cast<PPApp *>(APPL)->LastCmd, 0)) : 0; }
STimeChunkBrowser * SLAPI PPFindLastTimeChunkBrowser() { return APPL ? static_cast<STimeChunkBrowser *>(APPL->FindBrowser(static_cast<PPApp *>(APPL)->LastCmd, 1)) : 0; }
PPPaintCloth * SLAPI PPFindLastPaintCloth() { return APPL ? (PPPaintCloth*)APPL->FindBrowser(static_cast<PPApp *>(APPL)->LastCmd, 2) : 0; }
static STextBrowser * SLAPI PPFindLastTextBrowser(const char * pFileName) { return APPL ? static_cast<STextBrowser *>(APPL->FindBrowser(static_cast<PPApp *>(APPL)->LastCmd, 3, pFileName)) : 0; }

void SLAPI PPViewTextBrowser(const char * pFileName, const char * pTitle, const char * pLexerSymb, int toolbarId)
{
	STextBrowser * p_brw = PPFindLastTextBrowser(pFileName);
	if(p_brw) {
		PPCloseBrowser(p_brw);
		p_brw = 0;
	}
	p_brw = new STextBrowser(pFileName, pLexerSymb, toolbarId);
	{
		SString title_buf;
		if(!isempty(pTitle)) {
			title_buf = pTitle;
		}
		else {
			SPathStruc ps(pFileName);
			ps.Merge(SPathStruc::fNam|SPathStruc::fExt, title_buf);
		}
		p_brw->setTitle(title_buf);
	}
#ifndef NDEBUG
	p_brw->SetSpecialMode(STextBrowser::spcmSartrTest); // @v9.2.0 @debug
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
uint SLAPI GetComboBoxLinkID(TDialog * dlg, uint comboBoxCtlID) { return dlg->getCtrlView(comboBoxCtlID)->GetId(); }

int FASTCALL SetComboBoxLinkText(TDialog * dlg, uint comboBoxCtlID, const char * pText)
{
	ComboBox * p_combo = static_cast<ComboBox *>(dlg->getCtrlView(comboBoxCtlID));
	if(p_combo) {
		p_combo->setInputLineText(pText);
		return 1;
	}
	else
		return 0;
}

int FASTCALL SetComboBoxListText(TDialog * dlg, uint comboBoxCtlID)
{
	SString temp_buf;
	PPLoadString("list", temp_buf);
	return SetComboBoxLinkText(dlg, comboBoxCtlID, temp_buf);
}

SString & FASTCALL PPFormatPeriod(const DateRange * pPeriod, SString & rBuf)
{
	rBuf.Z();
	if(pPeriod) {
		LDATE  beg = pPeriod->low;
		LDATE  end = pPeriod->upp;
		if(beg) {
			if(beg != end)
				rBuf.CatChar('с').Space();
			rBuf.Cat(beg, DATF_DMY);
		}
		if(end && beg != end) {
			if(beg)
				rBuf.Space();
			rBuf.Cat("по").Space().Cat(end, DATF_DMY);
		}
	}
	return rBuf.Transf(CTRANSF_OUTER_TO_INNER);
}

SString & FASTCALL PPFormatPeriod(const LDATETIME & rBeg, const LDATETIME & rEnd, SString & rBuf)
{
	rBuf.Z();
	if(rBeg.d) {
		if(rBeg.d != rEnd.d)
			rBuf.CatChar('с').Space();
		rBuf.Cat(rBeg.d, DATF_DMY);
		if(rBeg.t)
			rBuf.Space().Cat(rBeg.t, TIMF_HMS);
	}
	if(rEnd.d && rBeg.d != rEnd.d) {
		if(rBeg.d)
			rBuf.Space();
		rBuf.Cat("по").Space().Cat(rEnd.d, DATF_DMY);
		if(rEnd.t)
			rBuf.Space().Cat(rEnd.t, TIMF_HMS);
	}
	return rBuf.Transf(CTRANSF_OUTER_TO_INNER);
}

/*
int FASTCALL SetPeriodInput(TDialog * dlg, uint fldID, char * buf, const DateRange * rng)
{
	char   b[64];
	char * c = buf ? buf : &(b[0] = 0);
	if(*strip(c) == 0)
		periodfmt(rng, c);
	dlg->setCtrlData(fldID, c);
	return 1;
}
*/

void FASTCALL SetPeriodInput(TDialog * dlg, uint fldID, const DateRange * rng)
{
	if(dlg) {
		char   b[64];
		b[0] = 0;
		periodfmt(rng, b);
		dlg->setCtrlData(fldID, b);
	}
}

static int SLAPI Helper_GetPeriodInput(TDialog * dlg, uint fldID, DateRange * pPeriod, long strtoperiodFlags)
{
	int    ok = -1;
	char   b[64];
	b[0] = 0;
	if(dlg && dlg->getCtrlData(fldID, b)) {
		if(strtoperiod(b, pPeriod, strtoperiodFlags)) {
			if(checkdate(pPeriod->low, 1) && checkdate(pPeriod->upp, 1)) {
				LDATE a_low = pPeriod->low.getactual(ZERODATE);
				LDATE a_upp = pPeriod->upp.getactual(ZERODATE);
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

int    FASTCALL GetPeriodInput(TDialog * dlg, uint fldID, DateRange * pPeriod) { return Helper_GetPeriodInput(dlg, fldID, pPeriod, 0); }
int    FASTCALL GetPeriodInput(TDialog * dlg, uint fldID, DateRange * pPeriod, long strtoperiodFlags) { return Helper_GetPeriodInput(dlg, fldID, pPeriod, strtoperiodFlags); }

void FASTCALL SetTimeRangeInput(TDialog * pDlg, uint ctl, long fmt, const TimeRange * pTimePeriod)
{
	SString buf;
	if(pTimePeriod && !pTimePeriod->IsZero())
		pTimePeriod->ToStr(fmt, buf);
	CALLPTRMEMB(pDlg, setCtrlString(ctl, buf));
}

void FASTCALL SetTimeRangeInput(TDialog * pDlg, uint ctl, long fmt, const LTIME * pLow, const LTIME * pUpp)
{
	TimeRange tr;
	tr.Set((pLow ? *pLow : ZEROTIME), (pUpp ? *pUpp : ZEROTIME));
	SetTimeRangeInput(pDlg, ctl, fmt, &tr);
}

int FASTCALL GetTimeRangeInput(TDialog * pDlg, uint ctl, long fmt, TimeRange * pTimePeriod)
{
	int    ok = -1;
	TimeRange prd;
	if(GetTimeRangeInput(pDlg, ctl, fmt, &prd.low, &prd.upp) > 0) {
		ok = 1;
		ASSIGN_PTR(pTimePeriod, prd);
	}
	return ok;
}

int FASTCALL GetTimeRangeInput(TDialog * pDlg, uint ctl, long fmt, LTIME * pLow, LTIME * pUpp)
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
	double low = 0, upp = 0;
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
		// @v9.6.0 strtoirng(temp_buf, (long *)&pR->low, (long *)&pR->upp);
		temp_buf.ToIntRange(*pR, SString::torfAny); // @v9.6.0
		return 1;
	}
	else
		return 0;
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
				SString descr;
				THROW_PP(p_rez->findResource(ctrlMenuID, PP_RCDECLCTRLMENU), PPERR_RESFAULT);
				cnt = p_rez->getUINT();
				for(uint i = 0; i < cnt; i++) {
					p_rez->getString(descr, 2);
					SLS.ExpandString(descr, CTRANSF_UTF8_TO_INNER); // @v9.2.1
					key_code = static_cast<long>(p_rez->getUINT());
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
		dlg->SetDefaultButton(STDCTL_CANCELBUTTON, 1);
	}
}

int FASTCALL SetupPhoneButton(TDialog * pDlg, uint inputCtlId, uint btnCmd)
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
				clearEvent(event);
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
		}
		SCycleTimer T;
	};
	AsyncEventQueueStatDialog * dlg = new AsyncEventQueueStatDialog;
	if(CheckDialogPtrErr(&dlg)) {
		ExecViewAndDestroy(dlg);
	}
}

int SLAPI ViewStatus()
{
	class StatusDialog : public TDialog {
	public:
		enum {
			dummyFirst = 1,
			brushValidPath,
			brushInvalidPath
		};
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
					static const uint16 ctl_list[] = {CTL_STATUS_BINPATH, CTL_STATUS_INPATH, CTL_STATUS_OUTPATH, CTL_STATUS_TEMPPATH};
					SString path;
					for(uint i = 0; i < SIZEOFARRAY(ctl_list); i++) {
						uint16 ctl_id = ctl_list[i];
						if(p_dc->H_Ctl == getCtrlHandle(ctl_id)) {
							getCtrlString(ctl_id, path);
							if(IsDirectory(path.RmvLastSlash())) {
								::SetBkMode(p_dc->H_DC, TRANSPARENT);
								p_dc->H_Br = (HBRUSH)Ptb.Get(brushValidPath);
							}
							else {
								::SetBkMode(p_dc->H_DC, TRANSPARENT);
								p_dc->H_Br = (HBRUSH)Ptb.Get(brushInvalidPath);
							}
							clearEvent(event);
							break;
						}
					}
				}
			}
		}
		SPaintToolBox  Ptb;
	};
	int    ok = 1;
	DbProvider * p_dict = CurDict;
	PPID   main_org_id = 0;
	SString sbuf, datapath;
	//SString accbuf;
	//SString accbufno;
	LDATE  oper_dt = LConfig.OperDate;
	StatusDialog * dlg = new StatusDialog();
	THROW(CheckDialogPtr(&dlg));
	dlg->setCtrlString(CTL_STATUS_USR, GetCurUserName(sbuf));
	GetLocationName(LConfig.Location, sbuf);
	dlg->setCtrlString(CTL_STATUS_LOC, sbuf);
	GetMainOrgID(&main_org_id);
	SetupPPObjCombo(dlg, CTLSEL_STATUS_MAINORG, PPOBJ_PERSON, main_org_id, 0, reinterpret_cast<void *>(PPPRK_MAIN));
	dlg->setCtrlData(CTL_STATUS_DATE, &oper_dt);
	GetObjectName(PPOBJ_DBDIV, LConfig.DBDiv, sbuf, 0);
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

	//PPLoadText(PPTXT_PATHACCESS, accbuf);
	//PPLoadText(PPTXT_PATHACCESSNO, accbufno);

	PPGetPath(PPPATH_BIN, datapath);
	dlg->setCtrlString(CTL_STATUS_BINPATH, (sbuf = datapath).Transf(CTRANSF_OUTER_TO_INNER));
	//dlg->setCtrlString(CTL_STATUS_ACCESSBIN, fileExists(datapath) ? accbuf : accbufno);

	PPGetPath(PPPATH_IN, datapath);
    dlg->setCtrlString(CTL_STATUS_INPATH, (sbuf = datapath).Transf(CTRANSF_OUTER_TO_INNER));
	//dlg->setCtrlString(CTL_STATUS_ACCESSIN, fileExists(datapath) ? accbuf : accbufno);

	PPGetPath(PPPATH_OUT, datapath);
	dlg->setCtrlString(CTL_STATUS_OUTPATH, (sbuf = datapath).Transf(CTRANSF_OUTER_TO_INNER));
	//dlg->setCtrlString(CTL_STATUS_ACCESSOUT, fileExists(datapath) ? accbuf : accbufno);

	PPGetPath(PPPATH_TEMP, datapath);
	dlg->setCtrlString(CTL_STATUS_TEMPPATH, (sbuf = datapath).Transf(CTRANSF_OUTER_TO_INNER));
	//dlg->setCtrlString(CTL_STATUS_ACCESSTEMP, fileExists(datapath) ? accbuf : accbufno);
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

int FASTCALL SetupDBEntryComboBox(TDialog * dlg, uint ctl, PPDbEntrySet2 * pDbes)
{
	int    ok = 1;
	ComboBox * cb = static_cast<ComboBox *>(dlg->getCtrlView(ctl));
	if(cb && pDbes && pDbes->GetCount()) {
		StrAssocArray * p_list = new StrAssocArray;
		pDbes->MakeList(p_list, DbLoginBlockArray::loUseFriendlyName);
		ListWindow * p_lw = CreateListWindow(p_list, lbtDblClkNotify | lbtFocNotify | lbtDisposeData);
		cb->setListWindow(p_lw, pDbes->GetSelection());
	}
	return ok;
}

int FASTCALL SetupDBTableComboBox(TDialog * dlg, uint ctl, PPDbEntrySet2 * pDbes, long dbID, BTBLID tblID)
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

int SLAPI InputDateDialog(const char * pTitle, const char * pInputTitle, LDATE * pDate)
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
SLAPI DateAddDialogParam::DateAddDialogParam() : BaseDate(ZERODATE), Period(PRD_MONTH), PeriodCount(1), ResultDate(ZERODATE)
{
}

int SLAPI DateAddDialogParam::Recalc()
{
	if(!checkdate(BaseDate))
		BaseDate = getcurdate_();
	LDATE   td = BaseDate;
	plusperiod(&td, Period, PeriodCount, 0);
	ResultDate = td;
	return (ResultDate != BaseDate) ? 1 : -1;
}

int SLAPI DateAddDialog(DateAddDialogParam * pData)
{
	class __DateAddDialog : public TDialog {
	public:
		__DateAddDialog() : TDialog(DLG_DATEADD)
		{
		}
		int    setDTS(const DateAddDialogParam * pData)
		{
			int    ok = 1;
			RVALUEPTR(Data, pData);
			Data.Recalc();
			SetupStringCombo(this, CTLSEL_DATEADD_PRD, PPTXT_CYCLELIST, Data.Period);
			setCtrlLong(CTL_DATEADD_PRDCOUNT, Data.PeriodCount);
			Update();
			return ok;
		}
		int    getDTS(DateAddDialogParam * pData)
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
		DateAddDialogParam Data;
	};
	DIALOG_PROC_BODY(__DateAddDialog, pData);
}

int SLAPI DateRangeDialog(const char * pTitle, const char * pInputTitle, DateRange * pPeriod)
{
	int    ok = -1;
	TDialog * dlg = new TDialog(DLG_DATERNG);
	if(CheckDialogPtrErr(&dlg)) {
		dlg->setTitle(pTitle);
		if(pInputTitle)
			dlg->setLabelText(CTL_DATERNG_PERIOD, pInputTitle);
		dlg->SetupCalPeriod(CTLCAL_DATERNG_PERIOD, CTL_DATERNG_PERIOD);
		SetPeriodInput(dlg, CTL_DATERNG_PERIOD, pPeriod);
		while(ok < 0 && ExecView(dlg) == cmOK)
			if(GetPeriodInput(dlg, CTL_DATERNG_PERIOD, pPeriod))
				ok = 1;
			else
				PPError();
	}
	else
		ok = 0;
	delete dlg;
	return ok;
}

int SLAPI InputQttyDialog(const char * pTitle, const char * pInputTitle, double * pQtty)
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
	TDialog ** dlg = (TDialog**)ppDlg;
	if(ValidView(*dlg) == 0) {
		*(void **)ppDlg = 0;
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

int FASTCALL PPErrorByDialog(TDialog * dlg, uint ctlID, int err)
{
	PPError(err, 0);
	CALLPTRMEMB(dlg, selectCtrl(ctlID));
	return 0;
}

int FASTCALL PPErrorByDialog(TDialog * dlg, uint ctlID)
{
	PPError(-1, 0);
	CALLPTRMEMB(dlg, selectCtrl(ctlID));
	return 0;
}

int SLAPI PasswordDialog(uint dlgID, char * pBuf, size_t pwSize, size_t minLen, int withoutEncrypt)
{
	int    ok = -1, valid_data = 0;
	char   b1[32], b2[32];
	TDialog * dlg = new TDialog(NZOR(dlgID, DLG_PASSWORD));
	if(CheckDialogPtrErr(&dlg)) {
		b1[0] = 0;
		// @v9.6.6 dlg->SetCtrlBitmap(CTL_PASSWORD_IMG, BM_KEYS);
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
	if(pList && pList->IsSubSign(TV_SUBSIGN_LISTBOX)) {
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

int SLAPI SetupTreeListBox(TDialog * dlg, uint ctl, StrAssocArray * pData, uint fl, uint lbfl)
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

int Lst2LstDialogUI::setup()
{
	if(Data.TitleStrID) {
		SString temp;
		PPLoadText(Data.TitleStrID, temp);
		setTitle(temp);
	}
	else
		setTitle(Data.P_Title);
	SETIFZ(Data.LeftCtlId, CTL_LST2LST_LST1);
	SETIFZ(Data.RightCtlId, CTL_LST2LST_LST2);
	enableCommand(cmForward, 1);
	enableCommand(cmAllForward, 1);
	enableCommand(cmaInsert, Data.Flags & ListToListUIData::fCanInsertNewItem);
	SetDefaultButton(CTL_LST2LST_FW, 1);
	SetDefaultButton(CTL_LST2LST_BW, 0);
	return 1;
}

IMPL_HANDLE_EVENT(Lst2LstDialogUI)
{
	TDialog::handleEvent(event);
	if(TVCOMMAND)
		switch(TVCMD) {
			case cmaInsert:     addNewItem(); break;
			case cmForward:     addItem();    break;
			case cmAllForward:  addAll();     break;
			case cmBackward:    removeItem(); break;
			case cmAllBackward: removeAll();  break;
			case cmLBDblClk:
				if(P_Current) {
  					int    action = 1;
					SmartListBox * list = static_cast<SmartListBox *>(getCtrlView(GetCurrId()));
					if(list && list->isTreeList()) {
						PPID cur_id = 0;
						list->def->getCurID(&cur_id);
						if(static_cast<StdTreeListBoxDef *>(list->def)->HasChild(cur_id))
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
SLAPI ListToListAryData::ListToListAryData(uint rezID, SArray * pLList, SArray * pRList) : ListToListUIData(), RezID(rezID), P_LList(pLList), P_RList(pRList)
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

int Lst2LstAryDialog::SetupList(SArray *pA, SmartListBox * pL)
{
	if(pL) {
		long pos = pL->def ? pL->def->_curItem() : 0L;
		StdListBoxDef * def = new StdListBoxDef(pA, lbtFocNotify | lbtDblClkNotify, MKSTYPE(S_ZSTRING, 64));
		pL->setDef(def);
		pL->def->go(pos);
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
		const TaggedString * current = static_cast<const TaggedString *>(P_Left->at(idx));
		THROW(P_Right->insert(current));
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
	SmartListBox * l = GetRightList();
	if(l && l->getCurID(&tmp)) {
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

int FASTCALL ListToListAryDialog(ListToListAryData * pData)
{
	if(pData) {
		pData->Flags &= ~ListToListData::fIsTreeList;
		int    r;
		uint   rez_id = pData->RezID ? pData->RezID : ((pData->Flags & ListToListData::fIsTreeList) ? DLG_TLST2TLST : DLG_LST2LST);
		Lst2LstAryDialog * dlg = new Lst2LstAryDialog(rez_id, pData, pData->P_LList, pData->P_RList);
		if(CheckDialogPtr(&dlg)) {
			if((r = ExecView(dlg)) == cmOK)
				if(!dlg->getDTS(pData->P_RList))
					r = 0;
			delete dlg;
			if(r)
				return (r == cmOK) ? 1 : -1;
		}
	}
	return 0;
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
	//Data = *aData;
	Data.P_List = new PPIDArray(*aData->P_List);
	setup();
}

Lst2LstObjDialog::~Lst2LstObjDialog()
{
	delete P_Object;
	delete Data.P_List;
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
		if(rBuf.Empty())
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
	StdTreeListBoxDef * p_l_def = static_cast<StdTreeListBoxDef *>(GetLeftList()->def);
	SmartListBox      * p_r_lbx = GetRightList();
	StrAssocArray * p_list = new StrAssocArray;
	THROW_MEM(p_list);
	for(i = 0; Data.P_List->enumItems(&i, (void **)&p_id);) {
		const  long id = *p_id;
		long   parent_id = 0;
		p_l_def->GetParent(id, &parent_id);
		parent_id = (parent_id && Data.P_List->lsearch(parent_id)) ? parent_id : 0;
		/* @v9.2.1
		THROW(P_Object->GetName(id, &name_buf));
		if(name_buf.Empty())
			ideqvalstr(id, name_buf);
		*/
		GetItemText(id, name_buf); // @v9.2.1
		THROW_SL(p_list->Add(id, parent_id, name_buf));
	}
	p_list->SortByText();
	p_def = new StdTreeListBoxDef(p_list, lbtDisposeData | lbtDblClkNotify, 0);
	THROW_MEM(p_def);
	p_r_lbx->setDef(p_def);
	p_r_lbx->def->go(0);
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
	StdListBoxDef * p_def = 0;
	TaggedStringArray * p_ary = 0;
	if(Data.Flags & ListToListData::fIsTreeList) {
		return setupRightTList();
	}
	else {
		SmartListBox * p_lb = GetRightList();
		if(p_lb) {
			PPID * p_id;
			SString name_buf;
			const long pos = p_lb->def ? p_lb->def->_curItem() : 0L;
			THROW_MEM(p_ary = new TaggedStringArray);
			for(uint i = 0; Data.P_List->enumItems(&i, (void **)&p_id);) {
				/* @v9.2.1
				P_Object->GetName(*p_id, &name_buf);
				if(name_buf.Empty())
					name_buf.Cat(*p_id);
				*/
				GetItemText(*p_id, name_buf); // @v9.2.1
				THROW_SL(p_ary->Add(*p_id, name_buf));
			}
			p_ary->SortByText();
			THROW_MEM(p_def = new StdListBoxDef(p_ary, lbtDisposeData|lbtDblClkNotify, TaggedString::BufType()));
			p_lb->setDef(p_def);
			p_lb->def->go(pos);
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
		p_def = P_Object->Selector(Data.ExtraPtr);
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

int Lst2LstObjDialog::setup()
{
	int    ok = 1;
	if(Data.ObjType) {
		THROW(P_Object = GetPPObject(Data.ObjType, Data.ExtraPtr));
	}
	else {
		ZDELETE(P_Object);
	}
	THROW(setupLeftList());
	THROW(setupRightList());
	CATCHZOKPPERR
	return ok;
}

int Lst2LstObjDialog::addNewItem()
{
	int    ok = -1;
	if(Data.Flags & ListToListData::fCanInsertNewItem && P_Object ) {
		SmartListBox * p_view = GetLeftList();
		PPID   obj_id = 0;
		if(p_view && P_Object->Edit(&obj_id, Data.ExtraPtr) > 0) {
			P_Object->UpdateSelector(p_view->def, Data.ExtraPtr);
			p_view->search(&obj_id, 0, srchFirst|lbSrchByID);
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
	PPID   id = 0;
	SmartListBox * p_view = GetLeftList();
	if(p_view && p_view->getCurID(&id) && id && !Data.P_List->lsearch(id)) {
		if(Data.Flags & ListToListData::fIsTreeList) {
			THROW(Helper_AddItemRecursive(id, (StdTreeListBoxDef*)p_view->def));
		}
		else {
			THROW(Data.P_List->add(id));
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
	PPID   id;
	SmartListBox * p_list = GetRightList();
	if(p_list && p_list->getCurID(&id)) {
		if(Data.P_List->freeByKey(id, 0) > 0)
			THROW(setupRightList());
	}
	CATCHZOKPPERR
	return ok;
}

int Lst2LstObjDialog::addAll()
{
	int    ok = -1;
	SmartListBox * p_l = GetLeftList();
	if(p_l && p_l->def) {
		LongArray id_list;
		if(p_l->def->getIdList(id_list) > 0) {
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

int SLAPI GetDeviceTypeName(uint dvcClass, PPID deviceTypeID, SString & rBuf)
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
				// @v10.3.12 rBuf = drv_name.Transf(CTRANSF_OUTER_TO_INNER);
				rBuf = drv_name; // @v10.3.12
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
int SLAPI SetupStringComboDevice(TDialog * dlg, uint ctlID, uint dvcClass, long initID, uint /*flags*/)
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
					if(!txt_buf.IsEqiAscii("Unused")) { // @v10.4.5
						THROW_SL(p_list->Add(id, txt_buf));
					}
				}
				//list_count = p_list->getCount();
			}
			for(int i = /*(idx + 1)*/PPCMT_FIRST_DYN_DVC; GetStrFromDrvIni(ini_file, ini_sect_id, i, /*list_count*/PPCMT_FIRST_DYN_DVC, line_buf) > 0; i++) {
				int    drv_impl = 0;
				if(PPAbstractDevice::ParseRegEntry(line_buf, symbol, drv_name, path, &drv_impl)) {
					THROW_SL(p_list->Add((int)i, drv_name/* @v10.3.12 .Transf(CTRANSF_OUTER_TO_INNER)*/));
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

static int SLAPI Helper_SetupStringCombo(TDialog * dlg, uint ctlID, const SString & rLineBuf, const StrAssocArray * pAddendumList, long initID)
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
			// @v9.5.0 {
			if(pAddendumList && pAddendumList->getCount()) {
				for(uint i = 0; i < pAddendumList->getCount(); i++) {
					StrAssocArray::Item ai = pAddendumList->Get(i);
					THROW_SL(p_list->Add(ai.Id, ai.Txt, 0));
				}
			}
			// } @v9.5.0
			p_cb->setListWindow(CreateListWindow(p_list, lbtDisposeData|lbtDblClkNotify), initID);
		}
	}
	CATCH
		ZDELETE(p_list);
		ok = 0;
	ENDCATCH
	return ok;
}

int FASTCALL SetupStringCombo(TDialog * dlg, uint ctlID, int strID, long initID)
{
	SString line_buf;
	return PPLoadText(strID, line_buf) ? Helper_SetupStringCombo(dlg, ctlID, line_buf, 0, initID) : 0;
}

int FASTCALL SetupStringCombo(TDialog * dlg, uint ctlID, const char * pStrSignature, long initID)
{
	SString line_buf;
	return PPLoadString(pStrSignature, line_buf) ? Helper_SetupStringCombo(dlg, ctlID, line_buf, 0, initID) : 0;
}

int FASTCALL SetupStringComboWithAddendum(TDialog * dlg, uint ctlID, const char * pStrSignature, const StrAssocArray * pAddendumList, long initID)
{
	SString line_buf;
	return PPLoadString(pStrSignature, line_buf) ? Helper_SetupStringCombo(dlg, ctlID, line_buf, pAddendumList, initID) : 0;
}

/* @v9.5.0 // id = <string offset> + 1
int SLAPI SetupStringCombo(TDialog * dlg, uint ctlID, StringSet * pSs, long initID, uint flags)
{
	int    ok = 1;
	ComboBox   * p_cb = 0;
	ListWindow * p_lw = 0;
	if((p_cb = static_cast<ComboBox *>(dlg->getCtrlView(ctlID))) != 0) {
		uint   idx = 0;
		long   id = 1;
		SString item_buf;
		THROW(p_lw = CreateListWindow(48, lbtDisposeData | lbtDblClkNotify));
		for(idx = 0; pSs->get(&idx, item_buf);) {
			p_lw->listBox()->addItem(id, item_buf);
			id = idx + 1;
		}
		p_cb->setListWindow(p_lw, initID);
	}
	CATCHZOK
	return ok;
} @v9.5.0 */

int FASTCALL SetupStrAssocCombo(TDialog * dlg, uint ctlID, const StrAssocArray * pList, long initID, uint flags, size_t offs, int ownerDrawListBox)
{
	int    ok = 1;
	ComboBox   * p_cb = 0;
	ListWindow * p_lw = 0;
	if((p_cb = static_cast<ComboBox *>(dlg->getCtrlView(ctlID))) != 0) {
		uint options = ownerDrawListBox ? (lbtOwnerDraw|lbtDisposeData|lbtDblClkNotify) : (lbtDisposeData|lbtDblClkNotify);
		StrAssocArray * p_list = new StrAssocArray;
		THROW_MEM(p_list);
		if(offs) {
			for(uint i = 0; i < pList->getCount(); i++) {
				StrAssocArray::Item item = pList->at_WithoutParent(i);
				const size_t len = sstrlen(item.Txt);
				if(offs < len)
					p_list->Add(item.Id, item.Txt+offs);
				else
					p_list->Add(item.Id, 0);
			}
		}
		else
			*p_list = *pList;
		THROW_MEM(p_lw = new ListWindow(new StrAssocListBoxDef(p_list, options), 0, 0));
		p_cb->setListWindow(p_lw, initID);
	}
	CATCHZOK
	return ok;
}

int FASTCALL SetupSCollectionComboBox(TDialog * dlg, uint ctl, SCollection * pSC, long initID)
{
	int    ok = 1;
	ComboBox * cb = static_cast<ComboBox *>(dlg->getCtrlView(ctl));
	if(cb && pSC && pSC->getCount()) {
		ListWindow * lw = CreateListWindow(128, lbtDisposeData);
		for(uint i = 0; i < pSC->getCount(); i++) {
			char * p_buf = (char *)pSC->at(i);
			if(p_buf && p_buf[0] != '\0') {
				char   temp_buf[128];
				long   id = 0;
				uint   pos = 0;
				StringSet ss(',', p_buf);
				if(ss.getCount() >= 2) {
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

int SLAPI SetupSubstDateCombo(TDialog * dlg, uint ctlID, long initID)
{
	return SetupStringCombo(dlg, ctlID, PPTXT_SUBSTDATELIST, initID);
}

int SLAPI SetupSubstPersonCombo(TDialog * pDlg, uint ctlID, SubstGrpPerson sgp)
{
	PPID   id = 0, init_id = 0;
	SString buf, word_rel, id_buf, txt_buf;
	PPPersonRelType item;
	StrAssocArray ary;
	PPObjPersonRelType relt_obj;
	PPLoadText(PPTXT_SUBSTPERSONLIST, buf);
	StringSet ss(';', buf);
	for(uint i = 0; ss.get(&i, buf.Z()) > 0;)
		if(buf.Divide(',', id_buf, txt_buf) > 0)
			ary.Add(id_buf.ToLong(), txt_buf);
	PPGetWord(PPWORD_RELATION, 0, word_rel);
	for(id = 0; relt_obj.EnumItems(&id, &item) > 0;)
		if(item.Cardinality & (PPPersonRelType::cOneToOne | PPPersonRelType::cManyToOne)) {
			(buf = word_rel).CatChar(':').Cat(item.Name);
			ary.Add(id + (long)sgpFirstRelation, buf);
		}
	init_id = (long)sgp;
	return SetupStrAssocCombo(pDlg, ctlID, &ary, init_id, 0);
}

int SLAPI SetupSubstGoodsCombo(TDialog * dlg, uint ctlID, long initID)
{
	int    ok = 1;
	ComboBox   * p_cb = 0;
	ListWindow * p_lw = 0;
	if((p_cb = static_cast<ComboBox *>(dlg->getCtrlView(ctlID))) != 0) {
		int    idx = 0;
		PrcssrAlcReport::Config alr_cfg;
		SString item_buf, id_buf, txt_buf;
		THROW(p_lw = CreateListWindow(48, lbtDisposeData | lbtDblClkNotify));
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
			MEMSZERO(ggrp_total);
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

int SLAPI SetupSubstBillCombo(TDialog * pDlg, uint ctlID, SubstGrpBill sgb)
{
	SString buf, id_buf, txt_buf;
	StrAssocArray ary;
	PPLoadText(PPTXT_SUBSTBILLLIST, buf);
	StringSet ss(';', buf);
	for(uint i = 0; ss.get(&i, buf) > 0;)
		if(buf.Divide(',', id_buf, txt_buf) > 0)
			ary.Add(id_buf.ToLong(), txt_buf);
	return SetupStrAssocCombo(pDlg, ctlID, &ary, (long)sgb.S, 0);
}

int SLAPI SetupSubstSCardCombo(TDialog * pDlg, uint ctlID, SubstGrpSCard sgc)
{
	SString buf, id_buf, txt_buf;
	StrAssocArray ary;
	PPLoadText(PPTXT_SUBSTSCARDLIST, buf);
	StringSet ss(';', buf);
	for(uint i = 0; ss.get(&i, buf) > 0;)
		if(buf.Divide(',', id_buf, txt_buf) > 0)
			ary.Add(id_buf.ToLong(), txt_buf);
	return SetupStrAssocCombo(pDlg, ctlID, &ary, (long)sgc, 0);
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
	cf.Init();
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
		if(!prd.IsEqual(prev_prd) && (leaderCtl && leaderCtl != CtlPeriod))
			SetPeriodInput(NZOR(P_PrdDialog, pDlg), CtlPeriod, &prd);
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
				cf.Init();
				long   c = pDlg->getCtrlLong(CtlSelCycle);
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
	else
		rec.C.Init();
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
	rec.C.Init();
	pDlg->getCtrlData(CtlSelCycle, &c/*rec.Cycle*/);
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
		SPathStruc  ps(pDirNFile);
		ps.Merge(0, SPathStruc::fNam|SPathStruc::fExt, rDir);
		ps.Split(pDirNFile);
		ps.Merge(0, SPathStruc::fDrv|SPathStruc::fDir, rFile);
		ok = 1;
	}
	return ok;
}
//
//
//
// static
int SLAPI FileBrowseCtrlGroup::Setup(TDialog * dlg, uint btnCtlID, uint inputCtlID, uint grpID,
	int titleTextId, int patternId, long flags)
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
			// @v9.2.5 {
			if(btnCtlID) {
				TButton * p_btn = static_cast<TButton *>(dlg->getCtrlView(btnCtlID));
				CALLPTRMEMB(p_btn, SetBitmap(IDB_FILEBROWSE));
			}
			// } @v9.2.5
		}
		else
			ok = PPSetErrorNoMem();
	}
	else
		ok = -1;
	return ok;
}

FileBrowseCtrlGroup::FileBrowseCtrlGroup(uint buttonId, uint inputId, const char * pTitle, long flags) :
	ButtonCtlId(buttonId), InputCtlId(inputId), Flags(flags), Title(pTitle)
{
	if(Title.Strip().Empty()) {
		PPGetSubStr(PPTXT_FILE_OR_PATH_SELECTION, (Flags & fbcgfPath) ? PPFOPS_PATH : PPFOPS_FILE, Title);
		Title.Transf(CTRANSF_INNER_TO_OUTER);
	}
	MEMSZERO(Data);
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
	int      not_exist = 1;
	SString  dir, fname;
	if(!isempty(pInitPath)) {
		not_exist = access(pInitPath, 0);
		if(not_exist) {
			SplitPath(pInitPath, dir, fname);
			not_exist = access(dir, 0);
			fname.Z();
		}
		else if(Flags & fbcgfPath) {
			dir = pInitPath;
			fname.Z();
		}
		else
			SplitPath(pInitPath, dir, fname);
	}
	if(not_exist && Data.FilePath[0]) {
		not_exist = access(Data.FilePath, 0);
		if(not_exist) {
			SplitPath(Data.FilePath, dir, fname);
			not_exist = access(dir, 0);
			fname.Z();
		}
		else if(Flags & fbcgfPath) {
			dir = Data.FilePath;
			fname.Z();
		}
		else
			SplitPath(Data.FilePath, dir, fname);
	}
	if(not_exist)
		if(Flags)
			PPGetPath((Flags & fbcgfLogFile) ? PPPATH_LOG : PPPATH_SYSROOT, dir);
		else
			dir.Z();
	InitDir  = dir;
	fname.ShiftLeftChr('\\').ShiftLeftChr('\\');
	InitFile = (Flags & fbcgfPath) ? "*.*" : fname;
}

static const TCHAR * MakeOpenFileInitPattern(const StringSet & rPattern, STempBuffer & rResult)
{
	const size_t src_pattern_len = rPattern.getDataLen();
	if(src_pattern_len == 0)
		return 0;
	else if(sizeof(TCHAR) == sizeof(wchar_t)) {
		SString & r_temp_buf = SLS.AcquireRvlStr();
		rResult.Alloc((src_pattern_len * sizeof(TCHAR)) + 64); // @safe(64) // @v10.4.5 @fix (* sizeof(TCHAR))
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
	// @v10.2.1 const char * WrSubKey_Dialog = "Software\\Papyrus\\Dialog";
	int    ok = -1;
	OPENFILENAME sofn;
	TCHAR  file_name[1024];
	SString temp_buf;
	SString result_file_name;
	SString reg_key_buf;
	STempBuffer filter_buf(16);
	reg_key_buf.Cat("FileBrowseLastPath").CatChar('(').Cat(pDlg->GetId()).CatDiv(',', 2).Cat(InputCtlId).CatChar(')');
	RecentItemsStorage ris(reg_key_buf, 20, PTR_CMPFUNC(FilePathUtf8)); // @v10.2.1
	StringSet ss_ris; // @v10.2.1
	file_name[0] = 0;
	pDlg->getCtrlString(InputCtlId, temp_buf);
	temp_buf.Transf(CTRANSF_INNER_TO_OUTER);
	if(temp_buf.Empty() || !fileExists(temp_buf))
		temp_buf = InitFile;
	STRNSCPY(file_name, SUcSwitch(temp_buf));
	memzero(&sofn, sizeof(sofn));
	sofn.lStructSize = sizeof(sofn);
	sofn.hwndOwner   = pDlg->H();
	// @v10.4.0 sofn.lpstrFilter = SUcSwitch(Patterns.getBuf()); // @unicodeproblem
	sofn.lpstrFilter = MakeOpenFileInitPattern(Patterns, filter_buf); // @v10.4.0
	sofn.lpstrFile   = file_name; // @unicodeproblem
	sofn.nMaxFile    = SIZEOFARRAY(file_name);
	sofn.lpstrTitle  = SUcSwitch(Title); // @unicodeproblem
	sofn.Flags       = (OFN_EXPLORER|OFN_HIDEREADONLY|OFN_LONGNAMES|OFN_NOCHANGEDIR);
	if(!(Flags & fbcgfAllowNExists))
		sofn.Flags |= OFN_FILEMUSTEXIST;
	if(Flags & fbcgfPath)
		sofn.Flags  |= (OFN_NOVALIDATE|OFN_PATHMUSTEXIST);
	if(!InitDir.NotEmptyS()) {
		if(Flags & fbcgfSaveLastPath) {
			/* @v10.2.1
			WinRegKey reg_key(HKEY_CURRENT_USER, WrSubKey_Dialog, 1);
			reg_key.GetString(reg_key_buf, InitDir);
			*/
			// @v10.2.1 {
			if(ris.GetList(ss_ris) > 0) {
				ss_ris.reverse();
				for(uint ssp = 0; ss_ris.get(&ssp, temp_buf);) {
					temp_buf.Transf(CTRANSF_UTF8_TO_OUTER);
					if(IsDirectory(temp_buf)) {
						InitDir = temp_buf;
						break;
					}
				}
			}
			// } @v10.2.1
		}
		if(!InitDir.NotEmptyS())
			setInitPath(SUcSwitch(file_name));
	}
	sofn.lpstrInitialDir = SUcSwitch(InitDir); // @unicodeproblem
	if((ok = ::GetOpenFileName(/*(LPOPENFILENAME)*/&sofn)) != 0) { // @unicodeproblem
		result_file_name = SUcSwitch(file_name);
		SPathStruc ps(result_file_name);
		ps.Merge(SPathStruc::fDrv|SPathStruc::fDir, InitDir);
		if(Flags & fbcgfSaveLastPath) {
			/* @v10.2.1
			WinRegKey reg_key(HKEY_CURRENT_USER, WrSubKey_Dialog, 0);
			reg_key.PutString(reg_key_buf, InitDir);
			*/
			// @v10.2.1 {
			(temp_buf = InitDir).Transf(CTRANSF_OUTER_TO_UTF8);
			ris.CheckIn(temp_buf);
			// } @v10.2.1
		}
		if(Flags & fbcgfPath) {
			if(sofn.nFileExtension != 0)
				file_name[sofn.nFileOffset] = 0;
			result_file_name.RmvLastSlash();
		}
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
			/* @v9.2.5
			static HBITMAP hbmp = 0;
			SETIFZ(hbmp, APPL->LoadBitmap(IDB_FILEBROWSE));
			SendDlgItemMessage(pDlg->H(), ButtonCtlId, BM_SETIMAGE, IMAGE_BITMAP, (long)hbmp);
			*/
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

int SLAPI PPOpenFile(uint strID, SString & rPath, long flags, HWND owner)
{
	int    ok = -1;
	SString temp_buf, name, pattern;
	if(PPLoadTextWin(strID, temp_buf)) {
		StringSet ss_pat;
		StringSet ss(',', temp_buf);
		for(uint i = 0; ss.get(&i, temp_buf) > 0;) {
			if(temp_buf.Divide(':', name, pattern) > 0) {
				ss_pat.add(name);
				ss_pat.add(pattern);
			}
		}
		ok = PPOpenFile(rPath, ss_pat, flags, owner);
	}
	return ok;
}

int SLAPI PPOpenFile(SString & rPath, const StringSet & rPatterns, long flags, HWND owner)
{
	int    ok = -1;
	OPENFILENAME sofn;
	TCHAR  file_name[MAXPATH];
	SString  title_buf;
	SString  dir, fname;
	STempBuffer filter_buf(64);
	SplitPath(rPath, dir, fname);
	STRNSCPY(file_name, SUcSwitch(fname));
	memzero(&sofn, sizeof(sofn));
	sofn.lStructSize = sizeof(sofn);
	sofn.hwndOwner   = NZOR(owner, GetForegroundWindow());
	// @v10.4.0 sofn.lpstrFilter = SUcSwitch(rPatterns.getBuf()); // @unicodeproblem
	sofn.lpstrFilter = MakeOpenFileInitPattern(rPatterns, filter_buf); // @v10.4.0
	sofn.lpstrFile   = file_name; // @unicodeproblem
	sofn.nMaxFile    = SIZEOFARRAY(file_name);
	PPLoadString("fileopen", title_buf);
	title_buf.Transf(CTRANSF_INNER_TO_OUTER); // @v9.0.12
	sofn.lpstrTitle  = SUcSwitch(title_buf); // @unicodeproblem
	sofn.Flags = (OFN_EXPLORER|OFN_HIDEREADONLY|OFN_LONGNAMES|OFN_NOCHANGEDIR);
	if(!(flags & ofilfNExist))
		sofn.Flags |= OFN_FILEMUSTEXIST;
	sofn.lpstrInitialDir = SUcSwitch(dir); // @unicodeproblem
	ok = GetOpenFileName(&sofn); // @unicodeproblem
	if(!ok) 
		PTR32(file_name)[0] = 0;
	rPath = SUcSwitch(file_name);
	return ok;
}

int SLAPI PPOpenDir(SString & rPath, const char * pTitle, HWND owner /*=0*/)
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
	// @v10.4.0 sofn.lpstrFilter = SUcSwitch(patterns.getBuf()); // @unicodeproblem
	sofn.lpstrFilter = MakeOpenFileInitPattern(patterns, filter_buf); // @v10.4.0
	sofn.lpstrFile   = file_name; // @unicodeproblem
	sofn.nMaxFile    = SIZEOFARRAY(file_name);
	sofn.lpstrTitle  = SUcSwitch(title); // @unicodeproblem
	sofn.Flags       = (OFN_EXPLORER|OFN_HIDEREADONLY|OFN_LONGNAMES|OFN_NOCHANGEDIR|OFN_NOVALIDATE|OFN_FILEMUSTEXIST);
	sofn.lpstrInitialDir = SUcSwitch(rPath); // @unicodeproblem
	ok = GetOpenFileName(/*(LPOPENFILENAME)*/&sofn); // @unicodeproblem
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
				PPWait(1);
				if(dir.Len() > 0 && WaitNewFile(dir, file_name) > 0) {
					setCtrlString(CTL_OPENFILE_PATH, file_name);
					SDelay(1000);
					PPWait(0);
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
		SPathStruc sp(Data);
		sp.Merge(0, SPathStruc::fNam|SPathStruc::fExt, WaitFolder);
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
	int    ok = 0, valid_data = 0;
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
SLAPI ImageBrowseCtrlGroup::Rec::Rec(const SString * pBuf) : Flags(0), Path(pBuf ? *pBuf : 0)
{
}

ImageBrowseCtrlGroup::ImageBrowseCtrlGroup(/* @v9.5.6 uint patternsID,*/ uint ctlImage,
	uint cmChgImage, uint cmDeleteImage, int allowChangeImage /*=1*/, long flags /*=0*/) :
	CtlImage(ctlImage), CmChgImage(cmChgImage), CmDelImage(cmDeleteImage), AllowChangeImage(allowChangeImage), Flags(flags)
{
	SString buf, name, ext;
	uint patterns_id = PPTXT_PICFILESEXTS; // can be PPTXT_FILPAT_PICT
	PPLoadTextWin(/*patternsID*/patterns_id, buf);
	StringSet ss(',', buf);
	for(uint i = 0; ss.get(&i, buf) > 0;) {
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
	if(!RVALUEPTR(Data, static_cast<Rec *>(pData)))
		Data.Path.Z();
	pDlg->setCtrlString(CtlImage, Data.Path);
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
				ViewImageInfo(Data.Path, 0, 0);
			}
		}
	}
	else if(event.isCmd(CmChgImage)) {
		int    r = 0;
		SString path;
		if(Flags & fUseExtOpenDlg) {
			if((r = ExtOpenFileDialog(path = Data.Path, &Patterns, &DefWaitFolder)) > 0) {
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
			Data.Flags |= Rec::fUpdated;
			pDlg->setCtrlString(CtlImage, Data.Path = path);
		}
		pDlg->clearEvent(event);
	}
	else if(event.isCmd(CmDelImage)) {
		pDlg->setCtrlString(CtlImage, Data.Path.Z());
		Data.Flags |= Rec::fUpdated;
		pDlg->clearEvent(event);
	}
	else if(event.isCmd(cmPasteImage)) {
		long   start = 0;
		SString temp_dir, temp_path;
		PPGetPath(PPPATH_TEMP, temp_dir);
		temp_dir.SetLastSlash().Cat("IMG");
		if(!IsDirectory(temp_dir))
			createDir(temp_dir);
		MakeTempFileName(temp_dir, "pst", "jpg", &start, temp_path);
		if(SClipboard::CopyPaste(GetDlgItem(pDlg->H(), CtlImage), 0, temp_path) > 0) {
			Data.Flags |= Rec::fUpdated;
			pDlg->setCtrlString(CtlImage, Data.Path = temp_path);
			pDlg->clearEvent(event);
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
int SLAPI BarcodeInputDialog(int initChar, SString & rBuf)
{
	class BarcodeSrchDialog : public TDialog {
	public:
		BarcodeSrchDialog() : TDialog(DLG_SRCHBCODE)
		{
			disableCtrl(CTL_SRCHBCODE_CHKDIG, 1);
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

int FASTCALL SelectorDialog(uint dlgID, uint ctlID, uint * pVal /* IN,OUT */, const char * pTitle /*=0*/)
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
ListBoxSelDlg::ListBoxSelDlg(ListBoxDef * pDef, uint dlgID /*=0*/) :
	PPListDialog((dlgID) ? dlgID : ((pDef && pDef->_isTreeList()) ? DLG_LBXSELT : DLG_LBXSEL), CTL_LBXSEL_LIST), P_Def(pDef)
{
	showCtrl(STDCTL_INSBUTTON,  0);
	showCtrl(STDCTL_EDITBUTTON, 0);
	showCtrl(STDCTL_DELBUTTON,  0);
	showCtrl(CTL_LBXSEL_UPBTN,   0);
	showCtrl(CTL_LBXSEL_DOWNBTN, 0);
	updateList(-1);
}

// AHTOXA {
int ListBoxSelDlg::setupList()
{
	CALLPTRMEMB(P_Box, setDef(P_Def));
	return 1;
}

int ListBoxSelDlg::editItem(long pos, long id)
{
	if(pos >= 0 && IsInState(sfModal))
		endModal(cmOK);
	return -1;
}

int ListBoxSelDlg::getDTS(PPID * pID)
{
	long   pos = 0, id = 0;
	getCurItem(&pos, &id);
	ASSIGN_PTR(pID, id);
	return 1;
}

int ListBoxSelDlg::setDTS(PPID id)
{
	CALLPTRMEMB(P_Box, TransmitData(+1, &id));
	return 1;
}

int FASTCALL ListBoxSelDialog(PPID objID, PPID * pID, void * extraPtr)
{
	int    ok = -1;
	PPObject * ppobj = GetPPObject(objID, extraPtr);
	ListBoxDef * p_def = ppobj ? ppobj->Selector(extraPtr) : 0;
	ListBoxSelDlg * p_dlg = 0;
	if(p_def) {
		p_dlg = new ListBoxSelDlg(p_def);
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

int FASTCALL ListBoxSelDialog(uint dlgID, StrAssocArray * pAry, PPID * pID, uint flags)
{
	int    ok = -1;
	ListBoxSelDlg * p_dlg = 0;
	ListBoxDef * p_def = pAry ? new StrAssocListBoxDef(pAry, lbtDblClkNotify|lbtFocNotify) : 0;
	if(p_def && CheckDialogPtrErr(&(p_dlg = new ListBoxSelDlg(p_def, dlgID)))) {
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

/* @v9.2.1 int SLAPI ComboBoxSelDialog(PPID objID, PPID * pID, uint flags, long extra)
{
	int    ok = -1;
	TDialog * p_dlg = new TDialog(DLG_CBXSEL);
	if(CheckDialogPtrErr(&p_dlg)) {
		PPID   id = (pID) ? *pID : 0;
		SString obj_title;
		p_dlg->setSubTitle(GetObjectTitle(objID, obj_title));
		p_dlg->setLabelText(CTL_CBXSEL_COMBO, obj_title);
		SetupPPObjCombo(p_dlg, CTLSEL_CBXSEL_COMBO, objID, id, flags, (void *)extra);
		if(ExecView(p_dlg) == cmOK) {
			p_dlg->getCtrlData(CTLSEL_CBXSEL_COMBO, &id);
			ASSIGN_PTR(pID, id);
			ok = 1;
		}
	}
	else
		ok = 0;
	delete p_dlg;
	return ok;
}*/

int SLAPI ComboBoxSelDialog2(const StrAssocArray * pAry, uint subTitleStrId, uint labelStrId, long * pSelectedId, uint flags)
{
	int    ok = -1;
	long   sel_id = DEREFPTRORZ(pSelectedId);
	TDialog * p_dlg = 0;
	if(pAry && CheckDialogPtrErr(&(p_dlg = new TDialog(DLG_CBXSEL)))) {
		SString subtitle, label;
		if(subTitleStrId) {
			PPLoadText(subTitleStrId, subtitle);
			p_dlg->setSubTitle(subtitle);
		}
		if(labelStrId) {
			PPLoadText(labelStrId, label);
			p_dlg->setLabelText(CTL_CBXSEL_COMBO, label);
		}
		SetupStrAssocCombo(p_dlg, CTLSEL_CBXSEL_COMBO, pAry, sel_id, flags);
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

int  SLAPI AdvComboBoxSeldialog(const StrAssocArray * pAry, SString & rTitle, SString & rLabel, PPID * pID, SString * pName, uint flags)
{
	int    ok = -1;
	TDialog * p_dlg = 0;
	if(pAry && CheckDialogPtrErr(&(p_dlg = new TDialog(DLG_ADVCBXSEL)))) {
		PPID   id = DEREFPTRORZ(pID);
		SString subtitle, label;
		if(rTitle.Len())
			p_dlg->setSubTitle(rTitle); // @v10.4.6 setSubTitle-->setTitle
		if(rLabel.Len())
			p_dlg->setLabelText(CTL_CBXSEL_COMBO, rLabel);
		SetupStrAssocCombo(p_dlg, CTLSEL_CBXSEL_COMBO, pAry, id, flags);
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

int FASTCALL ListBoxSelDialog(StrAssocArray * pAry, uint titleStrId, PPID * pID, uint flags)
{
	SString title;
	if(titleStrId)
		PPLoadText(titleStrId, title);
	return ListBoxSelDialog(pAry, title, pID, flags);
}

int FASTCALL ListBoxSelDialog(StrAssocArray * pAry, const char * pTitle, PPID * pID, uint flags)
{
	int    ok = -1;
	ListBoxSelDlg * p_dlg = 0;
	ListBoxDef * p_def = pAry ? new StrAssocListBoxDef(pAry, lbtDblClkNotify|lbtFocNotify) : 0;
	if(p_def && CheckDialogPtrErr(&(p_dlg = new ListBoxSelDlg(p_def)))) {
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
public:
	UICfgDialog() : TDialog(DLG_UICFG)
	{
		SetupStringCombo(this, CTLSEL_UICFG_SELBCLRSHM, PPTXT_BRWCOLORSSCHEMASNAMES, 1);
		SetupStringCombo(this, CTLSEL_UICFG_WNDVIEWKIND, PPTXT_WINDOWSVIEWKINDS, 1);
	}
	int    setDTS(const UserInterfaceSettings * pUICfg);
	int    getDTS(UserInterfaceSettings * pUICfg);
private:
	DECL_HANDLE_EVENT;
	void   SelectFont(SFontDescr & rFd, uint indCtlId);
	UserInterfaceSettings UICfg;
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

IMPL_HANDLE_EVENT(UICfgDialog)
{
	TDialog::handleEvent(event);
	if(event.isCmd(cmSelBrowserFont)) {
		SelectFont(UICfg.TableFont, CTL_UICFG_ST_TABLEFONT);
	}
	else if(event.isCmd(cmSelListFont)) {
		SelectFont(UICfg.ListFont, CTL_UICFG_ST_LISTFONT);
	}
	else
		return;
	clearEvent(event);
}

int UICfgDialog::setDTS(const UserInterfaceSettings * pUICfg)
{
	long   cmbb_pos = 1;
	SString temp_buf;
	UICfg = *pUICfg;
	cmbb_pos = (UICfg.TableViewStyle >= 0 && UICfg.TableViewStyle < NUMBRWCOLORSCHEMA) ? (UICfg.TableViewStyle + 1) : 1;
	static_cast<ComboBox *>(getCtrlView(CTLSEL_UICFG_SELBCLRSHM))->TransmitData(+1, &cmbb_pos);
	cmbb_pos = (UICfg.WindowViewStyle >= 0) ? (UICfg.WindowViewStyle + 1) : 1;
	static_cast<ComboBox *>(getCtrlView(CTLSEL_UICFG_WNDVIEWKIND))->TransmitData(+1, &cmbb_pos);
	setCtrlData(CTL_UICFG_LISTELEMCOUNT, &UICfg.ListElemCount);
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
	AddClusterAssoc(CTL_UICFG_FLAGS, 10, UserInterfaceSettings::fExtGoodsSelMainName); // @v9.9.1
	AddClusterAssoc(CTL_UICFG_FLAGS, 11, UserInterfaceSettings::fPollVoipService); // @v9.8.11
	INVERSEFLAG(UICfg.Flags, UserInterfaceSettings::fDontExitBrowserByEsc);
	SetClusterData(CTL_UICFG_FLAGS, UICfg.Flags);
	// @v10.3.0 {
	{
		const  long t = CHKXORFLAGS(UICfg.Flags, UserInterfaceSettings::fEnalbeBillMultiPrint, UserInterfaceSettings::fDisableBillMultiPrint);
		AddClusterAssocDef(CTL_UICFG_MULTBILLPRINT, 0, 0);
		AddClusterAssoc(CTL_UICFG_MULTBILLPRINT, 1, UserInterfaceSettings::fEnalbeBillMultiPrint);
		AddClusterAssoc(CTL_UICFG_MULTBILLPRINT, 2, UserInterfaceSettings::fDisableBillMultiPrint);
		SetClusterData(CTL_UICFG_MULTBILLPRINT, t);
	}
	// } @v10.3.0 
	{
		UICfg.TableFont.ToStr(temp_buf.Z(), 0);
		setStaticText(CTL_UICFG_ST_TABLEFONT, temp_buf);
	}
	{
		UICfg.ListFont.ToStr(temp_buf.Z(), 0);
		setStaticText(CTL_UICFG_ST_LISTFONT, temp_buf);
	}
	setCtrlString(CTL_UICFG_SPCINPDRV, UICfg.SpecialInputDeviceSymb); // @v8.1.11
	return 1;
}

int UICfgDialog::getDTS(UserInterfaceSettings * pUICfg)
{
	long   id = 0;
	getCtrlData(CTLSEL_UICFG_SELBCLRSHM, &id);
	id--;
	UICfg.TableViewStyle = (id >= 0 && id < NUMBRWCOLORSCHEMA) ? id : 0;
	getCtrlData(CTLSEL_UICFG_WNDVIEWKIND, &(id = 0));
	UICfg.WindowViewStyle = (id > 0) ? (id - 1) : 0;
	getCtrlData(CTL_UICFG_LISTELEMCOUNT, &UICfg.ListElemCount);
	GetClusterData(CTL_UICFG_FLAGS, &UICfg.Flags);
	INVERSEFLAG(UICfg.Flags, UserInterfaceSettings::fDontExitBrowserByEsc);
	// @v10.3.0 {
	{
		const long t = GetClusterData(CTL_UICFG_MULTBILLPRINT);
		UICfg.Flags &= ~(UserInterfaceSettings::fEnalbeBillMultiPrint | UserInterfaceSettings::fDisableBillMultiPrint);
		if(t & UserInterfaceSettings::fEnalbeBillMultiPrint)
			UICfg.Flags |= UserInterfaceSettings::fEnalbeBillMultiPrint;
		else if(t & UserInterfaceSettings::fDisableBillMultiPrint)
			UICfg.Flags |= UserInterfaceSettings::fDisableBillMultiPrint;
	}
	// } @v10.3.0 
	getCtrlString(CTL_UICFG_SPCINPDRV, UICfg.SpecialInputDeviceSymb);
	ASSIGN_PTR(pUICfg, UICfg);
	return 1;
}

int SLAPI UISettingsDialog()
{
	int    r = 0;
	UICfgDialog	* p_dlg = new UICfgDialog();
	if(CheckDialogPtrErr(&p_dlg)) {
		uint   v = 0;
		UserInterfaceSettings uiset;
		uiset.Restore();
		p_dlg->setDTS(&uiset);
		if(ExecView(p_dlg) == cmOK) {
			p_dlg->getDTS(&uiset);
			uiset.Save();
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

// virtual
void EmbedDialog::setChildPos(uint neighbourCtl)
{
	if(P_ChildDlg && P_ChildDlg->H()) {
		RECT   child_rect, ctl_rect, dlg_rect;
		long   parent_style = TView::GetWindowStyle(H());
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
	UserInterfaceSettings uicfg;
	if(uicfg.Restore() > 0 && uicfg.SpecialInputDeviceSymb.NotEmptyS()) {
		PPObjGenericDevice gd_obj;
		PPID   dvc_id = 0;
		if(gd_obj.SearchBySymb(uicfg.SpecialInputDeviceSymb, &dvc_id) > 0) {
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
			if(p_il && p_il->IsSubSign(TV_SUBSIGN_INPUTLINE)) {
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
SLAPI LocationCtrlGroup::Rec::Rec(const ObjIdListFilt * pLocList, PPID parentID) : ParentID(parentID)
{
	RVALUEPTR(LocList, pLocList);
}

void SLAPI LocationCtrlGroup::Helper_Construct()
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

int SLAPI LocationCtrlGroup::EditLocList(TDialog * pDlg, uint ctlID, ObjIdListFilt * pLocList)
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
		LocationFilt loc_filt(LOCTYP_WAREHOUSE);
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
					pDlg->disableCtrl(ctlID, 1);
				}
				else {
					pDlg->setCtrlLong(ctlID, pLocList->GetSingle());
					pDlg->disableCtrl(ctlID, 0);
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

void SLAPI LocationCtrlGroup::SetupInfo(TDialog * pDlg, PPID locID)
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
	LocationFilt filt(((Flags & fWarehouseCell) ? LOCTYP_WHZONE : LOCTYP_WAREHOUSE), 0, Data.ParentID);
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
		pDlg->disableCtrl(CtlselLoc, 1);
	}
	else {
		pDlg->setCtrlLong(CtlselLoc, single_id);
		pDlg->disableCtrl(CtlselLoc, 0);
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
	const int by_lot_serial = BIN(ObjType == PPOBJ_LOT && TagID == PPTAG_LOT_SN);
	PPObjBill * p_bobj = BillObj;
	SString pattern, temp_buf, obj_name_buf;
	StrAssocArray * p_list = 0;
	pattern = pText;
	if(pattern.Len()) {
		int    srch_substr = BIN(pattern.C(0) == '*');
		size_t len = pattern.ShiftLeftChr('*').Len();
		if(len >= MinSymbCount) {
			const StrAssocArray * p_full_list = 0;
			if(by_lot_serial)
				p_full_list = p_bobj->GetFullSerialList();
			else
				p_full_list = &TextBlock;
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
			const PPID obj_id = obj_id_list.get(i);
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
	SString pattern = pText;
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
			reg_flt.Oid.Obj = PPOBJ_PERSON; // @v10.0.1
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
				pattern.Transf(CTRANSF_INNER_TO_UTF8).Utf8ToLower(); // @v9.9.11
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
						const PPID reg_id = reg_list.get(i);
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
					PPID   id     = 0;
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
	SString pattern = pText;
	if(pattern.NotEmptyS()) {
		SString phone_buf;
		const int srch_substr = BIN(pattern.C(0) == '*');
		pattern.ShiftLeftChr('*');
		pattern.Transf(CTRANSF_INNER_TO_UTF8).Utf8ToLower(); // @v9.9.11
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
					const PPID ea_id = src_ea_list.Get(i).Id;
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
								if(name.Empty() && loc_rec.Name[0])
									name = loc_rec.Name;
								if(name.Empty())
									LocationCore::GetExField(&loc_rec, LOCEXSTR_CONTACT, name);
								name.SetIfEmpty(loc_rec.Code);
								if(name.Empty())
									LocationCore::GetExField(&loc_rec, LOCEXSTR_FULLADDR, name);
								if(name.Empty())
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
	if(p_combo && p_combo->link()) {
		PPPersonKind psn_kind_rec;
		PPID   reg_type_id = 0;
		if(SearchObject(PPOBJ_PRSNKIND, Data.PsnKindID, &psn_kind_rec) > 0)
			reg_type_id = psn_kind_rec.CodeRegTypeID;
		if(reg_type_id > 0) {
			SString code, title;
			PPRegisterType reg_type_rec;
			SearchObject(PPOBJ_REGISTERTYPE, reg_type_id, &reg_type_rec);
			PPLoadText(PPTXT_SEARCHPERSON, title);
			PPInputStringDialogParam isd_param(title, reg_type_rec.Name);
			if(InputStringDialog(&isd_param, code) > 0) {
				PPIDArray   psn_list;
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
		if(p_combo && pDlg->IsCurrentView(p_combo->link())) {
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
#if 0 // {

PersonListCtrlGroup::Rec::Rec(PPID psnKindID, const PPIDArray * pPersonList)
{
	Init(psnKindID, pPersonList);
}

void PersonListCtrlGroup::Rec::Init(PPID psnKindID, const PPIDArray * pPersonList)
{
	PsnKindID = psnKindID;
	if(pPersonList)
		List = *pPersonList;
	else
		List.freeAll();
}

PersonListCtrlGroup::PersonListCtrlGroup(uint ctlsel, uint ctlSelPsnKind, uint cmPsnList, long flags) : CtrlGroup()
{
	Ctlsel        = ctlsel;
	CtlselPsnKind = ctlSelPsnKind;
	CmPsnList     = cmPsnList;
	Flags         = flags;
}

PersonListCtrlGroup::~PersonListCtrlGroup()
{
	ZDELETE(ListData.P_LList);
	ZDELETE(ListData.P_RList);
}

int PersonListCtrlGroup::Setup(TDialog * pDlg, PPID psnKindID, int force /*=0*/)
{
	int     new_psn_list = 0;
	SString buf;
	if(Data.PsnKindID != psnKindID || force) {
		Data.PsnKindID = psnKindID;
		SETIFZ(ListData.P_LList, new TaggedStringArray());
		SETIFZ(ListData.P_RList, new TaggedStringArray());
		ListData.P_LList->freeAll();
		ListData.P_RList->freeAll();
		if(Data.PsnKindID) {
			StrAssocArray _list;
			TaggedStringArray * p_psn_list = static_cast<TaggedStringArray *>(ListData.P_LList);
			PsnObj.GetListByKind(Data.PsnKindID, 0, &_list);
			for(uint i = 0; i < _list.getCount(); i++)
				p_psn_list->Add(_list.at(i).Id, _list.at(i).Txt);
		}
		if(force) {
			TaggedStringArray * p_sel_list = (TaggedStringArray*)ListData.P_RList;
			for(uint i = 0; i < Data.List.getCount(); i++) {
				PPID id = Data.List.at(i);
				GetObjectName(PPOBJ_PERSON, id, buf);
				p_sel_list->Add(id, buf);
			}
		}
	}
	pDlg->enableCommand(CmPsnList, BIN(Data.PsnKindID));
	{
		PPID   id = (ListData.P_RList && ListData.P_RList->getCount() == 1) ? ((TaggedString*)ListData.P_RList->at(0))->Id : 0;
		int    is_list = BIN(ListData.P_RList && ListData.P_RList->getCount() > 1);
		if(is_list) {
			PPLoadString("list", buf);
			SetComboBoxLinkText(pDlg, Ctlsel, buf);
		}
		else
			SetupPPObjCombo(pDlg, Ctlsel, PPOBJ_PERSON, id, Flags, Data.PsnKindID);
		pDlg->disableCtrl(Ctlsel, is_list);
	}
	return 1;
}

int PersonListCtrlGroup::setData(TDialog * pDlg, void * pRec)
{
	if(pRec)
		Data = *(Rec *)pRec;
	else
		Data.Init();
	SetupPPObjCombo(pDlg, CtlselPsnKind, PPOBJ_PRSNKIND, Data.PsnKindID, 0, 0);
	Setup(pDlg, Data.PsnKindID, 1);
	return 1;
}

int PersonListCtrlGroup::getData(TDialog * pDlg, void * pRec)
{
	Data.PsnKindID = pDlg->getCtrlLong(CtlselPsnKind);
	Data.List.freeAll();
	if(ListData.P_RList && ListData.P_RList->getCount()) {
		TaggedStringArray * p_psn_list = (TaggedStringArray*)ListData.P_RList;
		for(uint i = 0; i < p_psn_list->getCount(); i++)
			Data.List.add(p_psn_list->at(i).Id);
	}
	else {
		PPID psn_id = pDlg->getCtrlLong(Ctlsel);
		if(psn_id)
			Data.List.add(psn_id);
	}
	ASSIGN_PTR((Rec *)pRec, Data);
	return 1;
}

int PersonListCtrlGroup::selectByCode(TDialog * pDlg)
{
	int    ok = -1;
	ComboBox * p_combo = static_cast<ComboBox *>(pDlg->getCtrlView(Ctlsel));
	if(p_combo && p_combo->link()) {
		PPPersonKind psn_kind_rec;
		PPID   reg_type_id = 0;
		if(SearchObject(PPOBJ_PRSNKIND, Data.PsnKindID, &psn_kind_rec) > 0)
			reg_type_id = psn_kind_rec.CodeRegTypeID;
		if(reg_type_id > 0) {
			SString code, title;
			PPRegisterType reg_type_rec;
			SearchObject(PPOBJ_REGISTERTYPE, reg_type_id, &reg_type_rec);
			PPLoadText(PPTXT_SEARCHPERSON, title);
			if(InputStringDialog(title, reg_type_rec.Name, 0, 0, code) > 0) {
				PPIDArray   psn_list;
				PPObjPerson psn_obj;
				if(psn_obj.GetListByRegNumber(reg_type_id, 0, (const char *)code, psn_list) > 0)
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
	if(event.isKeyDown(kbF2)) {
		ComboBox * p_combo = static_cast<ComboBox *>(pDlg->getCtrlView(Ctlsel));
		if(p_combo && p_combo->link() && pDlg->current == p_combo->link()) {
			selectByCode(pDlg);
			pDlg->clearEvent(event);
		}
	}
	else if(TVCOMMAND) {
		if(TVCMD == CmPsnList) {
			if(ListData.P_RList && ListData.P_RList->getCount() == 0) {
				TaggedString item;
				if((item.Id = pDlg->getCtrlLong(Ctlsel))) {
					GetObjectName(PPOBJ_PERSON, item.Id, item.Txt, sizeof(item.Txt));
					ListData.P_RList->insert(&item);
				}
			}
			if(ListToListAryDialog(&ListData) > 0)
				Setup(pDlg, Data.PsnKindID);
			pDlg->clearEvent(event);
		}
		else if(TVCMD == cmCBSelected && CtlselPsnKind && event.isCtlEvent(CtlselPsnKind))
			Setup(pDlg, pDlg->getCtrlLong(CtlselPsnKind));
	}
}

#else // }{

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
		pDlg->enableCommand(CmPsnList, BIN(Data.PsnKindID));

		PPID   id = Data.List.getSingle();
		PPID   prev_id = pDlg->getCtrlLong(Ctlsel);
		if(id != prev_id)
			pDlg->setCtrlData(Ctlsel, &id);
		if(Data.List.getCount() > 1)
			SetComboBoxListText(pDlg, Ctlsel);
		pDlg->disableCtrl(Ctlsel, BIN(Data.List.getCount() > 1));
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
	SetupPPObjCombo(pDlg, CtlselPsnKind, PPOBJ_PRSNKIND, Data.PsnKindID, 0, 0);
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
	if(p_combo && p_combo->link()) {
		PPPersonKind psn_kind_rec;
		PPID   reg_type_id = 0;
		if(SearchObject(PPOBJ_PRSNKIND, Data.PsnKindID, &psn_kind_rec) > 0)
			reg_type_id = psn_kind_rec.CodeRegTypeID;
		if(reg_type_id > 0) {
			SString code, title;
			PPRegisterType reg_type_rec;
			SearchObject(PPOBJ_REGISTERTYPE, reg_type_id, &reg_type_rec);
			PPLoadText(PPTXT_SEARCHPERSON, title);
			PPInputStringDialogParam isd_param(title, reg_type_rec.Name);
			if(InputStringDialog(&isd_param, code) > 0) {
				PPIDArray   psn_list;
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
			ListToListData data(PPOBJ_PERSON, (void *)Data.PsnKindID, &result_list);
			data.TitleStrID = PPTXT_SELPERSONLIST;
			if(ListToListDialog(&data) > 0) {
				Data.List = result_list;
				Setup(pDlg, Data.PsnKindID);
			}
		}
	}
	else if(TVCMD == cmCBSelected && event.isCtlEvent(Ctlsel)) {
		if(Data.List.getSingle()) {
			const PPID id = pDlg->getCtrlLong(Ctlsel);
			Data.List.freeAll();
			if(id)
				Data.List.add(id);
		}
	}
	else if(TVCMD == cmCBSelected && CtlselPsnKind && event.isCtlEvent(CtlselPsnKind)) {
		Setup(pDlg, pDlg->getCtrlLong(CtlselPsnKind));
	}
	else if(event.isKeyDown(kbF2)) {
		ComboBox * p_combo = static_cast<ComboBox *>(pDlg->getCtrlView(Ctlsel));
		if(p_combo && pDlg->IsCurrentView(p_combo->link())) {
			SelectByCode(pDlg);
			pDlg->clearEvent(event);
		}
	}
	else
		return;
	pDlg->clearEvent(event);
}

#endif // }
//
//
//
#define SCARD_MIN_SYMBS 4

SLAPI SCardCtrlGroup::Rec::Rec() : SCardID(0)
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
		pDlg->disableCtrl(CtlselSCardSer, BIN(Data.SCardSerList.getCount() > 1));
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
		data.Flags |= ListToListData::fIsTreeList; // @v9.8.12 @fix
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
		SString pattern = pText;
		if(pattern.Len()) {
			size_t len = pattern.Len();
			PPIDArray id_list;
			SString temp_buf;
			if(len == 6 && pattern.IsDigit()) {
				P_Fr->FT.GetHouseListByZIP(pattern, id_list);
				if(id_list.getCount()) {
					p_list = new StrAssocArray;
					for(uint i = 0; i < id_list.getCount(); i++) {
						const PPID _id = id_list.get(i);
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
						const PPID _id = id_list.get(i);
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
SLAPI FiasAddressCtrlGroup::Rec::Rec() : TerminalFiasID(0)
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
SLAPI PosNodeCtrlGroup::Rec::Rec(const ObjIdListFilt * pList)
{
	RVALUEPTR(List, pList);
}

PosNodeCtrlGroup::PosNodeCtrlGroup(uint ctlselLoc, uint cmEditList) : Ctlsel(ctlselLoc), CmEditList(cmEditList)
{
}

void PosNodeCtrlGroup::handleEvent(TDialog * pDlg, TEvent & event)
{
	if(TVCOMMAND && TVCMD == CmEditList) {
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
		else if(r == 0)
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
		PPID   temp_id = pDlg->getCtrlLong(Ctlsel);
		Data.List.FreeAll();
        if(temp_id)
			Data.List.Add(temp_id);
	}
	*p_rec = Data;
	return 1;
}
//
//
//
SLAPI QuotKindCtrlGroup::Rec::Rec(const ObjIdListFilt * pList)
{
	RVALUEPTR(List, pList);
}

QuotKindCtrlGroup::QuotKindCtrlGroup(uint ctlsel, uint cmEditList) : Ctlsel(ctlsel), CmEditList(cmEditList)
{
}

void QuotKindCtrlGroup::handleEvent(TDialog * pDlg, TEvent & event)
{
	if(TVCOMMAND && TVCMD == CmEditList) {
		//EditQuotKindList(pDlg, Ctlsel, &Data.List);
		//static int SLAPI EditQuotKindList(TDialog * pDlg, uint ctlID, ObjIdListFilt * pList)
		{
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
					pDlg->disableCtrl(Ctlsel, 1);
				}
				else {
					pDlg->setCtrlLong(Ctlsel, Data.List.GetSingle());
					pDlg->disableCtrl(Ctlsel, 0);
				}
			}
		}
		pDlg->clearEvent(event);
	}
}

int QuotKindCtrlGroup::setData(TDialog * pDlg, void * pData)
{
	Data = *static_cast<Rec *>(pData);
	SetupPPObjCombo(pDlg, Ctlsel, PPOBJ_QUOTKIND, Data.List.GetSingle(), 0, 0);
	if(Data.List.GetCount() > 1) {
		SetComboBoxListText(pDlg, Ctlsel);
		pDlg->disableCtrl(Ctlsel, 1);
	}
	else {
		pDlg->setCtrlLong(Ctlsel, Data.List.GetSingle());
		pDlg->disableCtrl(Ctlsel, 0);
	}
	return 1;
}

int QuotKindCtrlGroup::getData(TDialog * pDlg, void * pData)
{
	Rec * p_rec = static_cast<Rec *>(pData);
	if(Data.List.GetCount() <= 1) {
		const PPID temp_id = pDlg->getCtrlLong(Ctlsel);
		Data.List.FreeAll();
		Data.List.Add(temp_id);
	}
	*p_rec = Data;
	return 1;
}
//
//
//
SLAPI StaffCalCtrlGroup::Rec::Rec(const ObjIdListFilt * pList)
{
	RVALUEPTR(List, pList);
}

StaffCalCtrlGroup::StaffCalCtrlGroup(uint ctlsel, uint cmEditList) : Ctlsel(ctlsel), CmEditList(cmEditList)
{
}

/* @v9.5.5 (inlined)
static int SLAPI EditStaffCalList(TDialog * pDlg, uint ctlID, ObjIdListFilt * pList)
{
	int    ok = -1;
	if(pList && pDlg) {
		PPIDArray ary;
		if(pList->IsExists())
			ary = pList->Get();
		if(!ary.getCount())
			ary.setSingleNZ(pDlg->getCtrlLong(ctlID));
		ListToListData lst(PPOBJ_STAFFCAL, 0, &ary);
		lst.TitleStrID = 0; // PPTXT_XXX;
		if(ListToListDialog(&lst) > 0) {
			pList->Set(&ary);
			if(pList->GetCount() > 1) {
				SetComboBoxListText(pDlg, ctlID);
				pDlg->disableCtrl(ctlID, 1);
			}
			else {
				pDlg->setCtrlLong(ctlID, pList->GetSingle());
				pDlg->disableCtrl(ctlID, 0);
			}
			ok = 1;
		}
	}
	else
		ok = 0;
	return ok;
}*/

void StaffCalCtrlGroup::handleEvent(TDialog * pDlg, TEvent & event)
{
	if(TVCOMMAND && TVCMD == CmEditList) {
		//EditStaffCalList(pDlg, Ctlsel, &Data.List);
		//static int SLAPI EditStaffCalList(TDialog * pDlg, uint ctlID, ObjIdListFilt * pList)
		{
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
					pDlg->disableCtrl(Ctlsel, 1);
				}
				else {
					pDlg->setCtrlLong(Ctlsel, Data.List.GetSingle());
					pDlg->disableCtrl(Ctlsel, 0);
				}
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
		pDlg->disableCtrl(Ctlsel, 1);
	}
	else {
		pDlg->setCtrlLong(Ctlsel, Data.List.GetSingle());
		pDlg->disableCtrl(Ctlsel, 0);
	}
	return 1;
}

int StaffCalCtrlGroup::getData(TDialog * pDlg, void * pData)
{
	Rec * p_rec = static_cast<Rec *>(pData);
	if(Data.List.GetCount() <= 1) {
		const PPID temp_id = pDlg->getCtrlLong(Ctlsel);
		Data.List.FreeAll();
		Data.List.Add(temp_id);
	}
	*p_rec = Data;
	return 1;
}
//
//
//
SLAPI PersonOpCtrlGroup::Rec::Rec(const ObjIdListFilt * pPsnOpList, PPID prmrID, PPID scndID) : PrmrID(prmrID), ScndID(scndID)
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
			const PPID temp_id = pDlg->getCtrlLong(CtlselPsnOp);
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
				pDlg->disableCtrl(CtlselPsnOp, 1);
			}
			else {
				pDlg->setCtrlLong(CtlselPsnOp, Data.PsnOpList.GetSingle());
				pDlg->disableCtrl(CtlselPsnOp, 0);
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
		int    disable_psn1 = 0, disable_psn2 = 0;
		PPID   prev_psn1k = 0, prev_psn2k = 0;
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
					disable_psn1 = 1;
				SETIFZ(prev_psn2k, psn2k);
				if(psn2k && prev_psn2k != psn2k)
					disable_psn2 = 1;
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
		pDlg->disableCtrl(CtlselPsnOp, 1);
	}
	else {
		pDlg->setCtrlLong(CtlselPsnOp, Data.PsnOpList.GetSingle());
		pDlg->disableCtrl(CtlselPsnOp, 0);
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
		const PPID temp_id = pDlg->getCtrlLong(CtlselPsnOp);
		Data.PsnOpList.FreeAll();
		Data.PsnOpList.Add(temp_id);
	}
	*p_rec = Data;
	return 1;
}

SpinCtrlGroup::SpinCtrlGroup(uint ctlEdit, uint cmdUp, uint ctlUp, uint cmdDown, uint ctlDown, long minVal, long maxVal) :
	CtlEdit(ctlEdit), CmdUp(cmdUp), CtlUp(ctlUp), CmdDown(cmdDown), CtlDown(ctlDown), MinVal(minVal), MaxVal(maxVal)
{
}

// virtual
int SpinCtrlGroup::setData(TDialog * pDlg, void * pData)
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

// virtual
int SpinCtrlGroup::getData(TDialog * pDlg, void * pData)
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

// virtual
void SpinCtrlGroup::handleEvent(TDialog * pDlg, TEvent & event)
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

// virtual
int BrandCtrlGroup::setData(TDialog * pDlg, void * pData)
{
	int    ok = -1;
	if(pDlg && pData) {
		Data = *static_cast<Rec *>(pData);
		SetupPPObjCombo(pDlg, Ctlsel, PPOBJ_BRAND, Data.List.getSingle(), OLW_LOADDEFONOPEN, 0);
		ok = Setup(pDlg);
	}
	return ok;
}

// virtual
int BrandCtrlGroup::getData(TDialog * pDlg, void * pData)
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
		pDlg->disableCtrl(Ctlsel, BIN(Data.List.getCount() > 1));
		ok = 1;
	}
	return ok;
}

// virtual
void BrandCtrlGroup::handleEvent(TDialog * pDlg, TEvent & event)
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
		else if(TVCMD == cmCBSelected && event.isCtlEvent(Ctlsel)) {
			if(Data.List.getSingle()) {
				const PPID id = pDlg->getCtrlLong(Ctlsel);
				Data.List.clear();
				Data.List.addnz(id);
			}
		}
	}
}
//
//
//
static int InputStringDialog(const char * pTitle, const char * pInpTitle, int disableSelection, int inputMemo, SString & rBuf)
{
	int    ok = -1;
	TDialog * dlg = new TDialog((inputMemo) ? DLG_MEMO : DLG_INPUT);
	if(CheckDialogPtrErr(&dlg)) {
		dlg->setTitle(pTitle);
		if(pInpTitle)
			dlg->setLabelText(CTL_INPUT_STR, pInpTitle);
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

SLAPI PPInputStringDialogParam::PPInputStringDialogParam(const char * pTitle, const char * pInputTitle) :
	Flags(0), Title(pTitle), InputTitle(pInputTitle), P_Wse(0)
{
}

SLAPI PPInputStringDialogParam::~PPInputStringDialogParam()
{
	delete P_Wse;
}

int FASTCALL InputStringDialog(PPInputStringDialogParam * pParam, SString & rBuf)
{
	int    ok = -1;
	TDialog * dlg = new TDialog((pParam && pParam->Flags & pParam->fInputMemo) ? DLG_MEMO : DLG_INPUT);
	if(CheckDialogPtrErr(&dlg)) {
		if(pParam) {
			dlg->setTitle(pParam->Title);
			if(pParam->InputTitle.NotEmpty())
				dlg->setLabelText(CTL_INPUT_STR, pParam->InputTitle);
		}
		{
			dlg->setCtrlString(CTL_INPUT_STR, rBuf);
		}
		if(pParam) {
			if(pParam->Flags & pParam->fDisableSelection) {
				TInputLine * il = static_cast<TInputLine *>(dlg->getCtrlView(CTL_INPUT_STR));
				CALLPTRMEMB(il, disableDeleteSelection(1));
			}
			if(pParam->P_Wse) {
				dlg->SetupWordSelector(CTL_INPUT_STR, pParam->P_Wse, 0, pParam->P_Wse->MinSymbCount, 0);
				pParam->P_Wse = 0; // Диалог разрушит объект pParam->P_Wse
			}
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

int SLAPI InputNumberDialog(const char * pTitle, const char * pInpTitle, double & rValue)
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

int FASTCALL EditSysjFilt2(SysJournalFilt * pFilt)
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
	CATCH
		ok = PPErrorByDialog(this, sel);
	ENDCATCH
	return ok;
}

SLAPI ResolveGoodsItem::ResolveGoodsItem(PPID goodsID /*= 0*/)
{
	THISZERO();
	GoodsID = goodsID;
}

SLAPI ResolveGoodsItemList::ResolveGoodsItemList() : TSArray <ResolveGoodsItem> ()
{
}

SLAPI ResolveGoodsItemList::ResolveGoodsItemList(const ResolveGoodsItemList & s) : TSArray <ResolveGoodsItem> (s)
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
public:
	explicit ResolveGoodsDialog(int flags) :
		PPListDialog((flags & (RESOLVEGF_SHOWRESOLVED|RESOLVEGF_SHOWEXTDLG)) ? DLG_SUBSTGL : DLG_LBXSEL, CTL_LBXSEL_LIST), Flags(flags), GoodsGrpID(0)
	{
		SString subtitle;
		PPLoadText(PPTXT_NOTIMPORTEDGOODS, subtitle);
		setSubTitle(subtitle);
		showCtrl(STDCTL_INSBUTTON,   0);
		showCtrl(STDCTL_DELBUTTON,   0);
		showCtrl(CTL_LBXSEL_UPBTN,   0);
		showCtrl(CTL_LBXSEL_DOWNBTN, 0);
		GObj.ReadConfig(&GoodsCfg);
		updateList(-1);
	}
	int    setDTS(const ResolveGoodsItemList * pData);
	int    getDTS(ResolveGoodsItemList * pData);
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
	ResolveGoodsItemList Data;
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
				// @v9.0.2 PPGetWord(PPWORD_ID, 0, word);
				PPLoadString("id", word); // @v9.0.2
				buf.Cat(word).CatDiv(':', 2).Cat(p_item->GoodsID);
				id_added = 1;
			}
			if(sstrlen(p_item->GoodsName)) {
				// @v9.0.2 PPGetWord(PPWORD_NAME, 0, word);
				PPLoadString("name", word); // @v9.0.2
				if(id_added)
					buf.CatDiv(',', 2);
				buf.Cat(word).CatDiv(':', 2).Cat(p_item->GoodsName);
			}
			else
				wo_name = 1;
			if(wo_name || (Flags & RESOLVEGF_SHOWBARCODE)) {
				// @v9.0.2 PPGetWord(PPWORD_BARCODE, 0, word);
				PPLoadString("barcode", word); // @v9.0.2
				if(!wo_name || id_added)
					buf.CatDiv(',', 2);
				buf.Cat(word).CatDiv(':', 2).Cat(p_item->Barcode);
				barcode_added = 1;
			}
			if((wo_name || (Flags & RESOLVEGF_SHOWQTTY)) && p_item->Quantity != 0) {
				// @v9.1.4 PPGetWord(PPWORD_QTTY, 0, word);
				PPLoadString("qtty", word); // @v9.1.4
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

int ResolveGoodsDialog::setDTS(const ResolveGoodsItemList * pData)
{
	int    ok = 1;
	if(pData)
		Data.copy(*pData);
	updateList(-1);
	return ok;
}

int ResolveGoodsDialog::getDTS(ResolveGoodsItemList * pData)
{
	int    ok = 1;
	if(Flags & RESOLVEGF_RESOLVEALLGOODS) {
		ResolveGoodsItem * p_item = 0;
		for(uint i = 0; ok && Data.enumItems(&i, (void **)&p_item) > 0;)
			ok = p_item->ResolvedGoodsID ? 1 : PPSetError(PPERR_EXISTUNRESOLVDEDGOODS);
	}
	return (ok > 0) ? (pData->copy(Data), 1) : ok;
}

int ResolveGoodsDialog::ResolveGoods(PPID resolveGoodsID, uint firstGoodsPos)
{
	int    ok = -1;
	if(resolveGoodsID > 0 && firstGoodsPos >= 0 && firstGoodsPos < Data.getCount()) {
		PPID   goods_id = Data.at(firstGoodsPos).GoodsID;
		char   barcode[24], goods_name[128];
		memzero(barcode, sizeof(barcode));
		memzero(goods_name, sizeof(goods_name));
		STRNSCPY(barcode, Data.at(firstGoodsPos).Barcode);
		STRNSCPY(goods_name, Data.at(firstGoodsPos).GoodsName);
		if(goods_id) {
			for(uint p = 0; Data.lsearch(&goods_id, &p, PTR_CMPFUNC(long), offsetof(ResolveGoodsItem, GoodsID)) > 0; p++)
				Data.at(p).ResolvedGoodsID = resolveGoodsID;
		}
		else if(sstrlen(barcode) && strcmp(barcode, "0") != 0) {
			for(uint p = 0; Data.lsearch(barcode, &p, PTR_CMPFUNC(Pchar), offsetof(ResolveGoodsItem, Barcode)) > 0; p++)
				Data.at(p).ResolvedGoodsID = resolveGoodsID;
		}
		else if(sstrlen(goods_name)) {
			for(uint p = 0; Data.lsearch(goods_name, &p, PTR_CMPFUNC(Pchar), offsetof(ResolveGoodsItem, GoodsName)) > 0; p++)
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
	int    ok = -1, valid_data = 0;
	PPID   resolve_goods_id = 0;
	ExtGoodsSelDialog * p_dlg = 0;
	if(id > 0) {
		int    maxlike_goods = (Flags & RESOLVEGF_MAXLIKEGOODS) ? 1 : 0;
		TIDlgInitData tidi;
		THROW_MEM(p_dlg = new ExtGoodsSelDialog(0, maxlike_goods ? 0 : GoodsGrpID,
			maxlike_goods ? ExtGoodsSelDialog::fByName : 0));
		THROW(CheckDialogPtr(&p_dlg));
		if(maxlike_goods) {
			StrAssocArray goods_list;
			SString goods_name;
			goods_name.CatChar('!').Cat(Data.at(id - 1).GoodsName);
			PPWait(1);
			GObj.P_Tbl->GetListBySubstring(goods_name, &goods_list, -1);
			p_dlg->setSelectionByGoodsList(&goods_list);
			PPWait(0);
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
	if(ListBoxSelDialog(PPOBJ_GOODSGROUP, &goods_grp_id, (void *)GGRTYP_SEL_NORMAL) > 0)
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
		SString name = r_item.GoodsName;
		THROW(GObj.InitPacket(&pack, gpkndGoods, 0, 0, Data.at(id).Barcode));
		pack.Rec.UnitID   = GoodsCfg.DefUnitID;
		pack.Rec.ParentID = goodsGrpID;
		name.CopyTo(pack.Rec.Name, sizeof(pack.Rec.Name));
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

int SLAPI ResolveGoodsDlg(ResolveGoodsItemList * pData, int flags)
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

int SLAPI ViewImageInfo(const char * pImagePath, const char * pInfo, const char * pWarn)
{
	class ImageInfoDialog : public TDialog {
	public:
		explicit ImageInfoDialog(int simple) : TDialog(simple ? DLG_IMAGEINFO2 : DLG_IMAGEINFO), IsSimple(simple)
		{
			if(IsSimple) {
				SetCtrlResizeParam(CTL_IMAGEINFO_IMAGE, 0, 0, 0, 0, crfResizeable);
				showCtrl(STDCTL_OKBUTTON, 0);
				showCtrl(STDCTL_CANCELBUTTON, 0);
				ResizeDlgToFullScreen();
			}
		}
		int    IsSimple;
	};
	int    ok = -1;
	const  int simple_resizeble = BIN(!pInfo && !pWarn);
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
			p_dlg->showCtrl(CTL_IMAGEINFO_PIC_WARN, 0);
	}
	if(ExecView(p_dlg) == cmOK)
		ok = 1;
	CATCHZOKPPERR
	delete p_dlg;
	return ok;
}

int SLAPI SetupComboByBuddyList(TDialog * pDlg, uint ctlCombo, const ObjIdListFilt & rList)
{
	pDlg->setCtrlLong(ctlCombo, rList.GetSingle());
	if(rList.GetCount() > 1) {
		SetComboBoxListText(pDlg, ctlCombo);
		pDlg->disableCtrl(ctlCombo, 1);
	}
	else
		pDlg->disableCtrl(ctlCombo, 0);
	return 1;
}

/* @v9.1.1 Заменено на SLS.CheckUiFlag(sluifUseLargeDialogs)
int SLAPI IsLargeDlg()
{
	return BIN(DS.GetTLA().Lc.Flags & CCFLG_USELARGEDIALOG);
}
*/

class EditMemosDialog : public PPListDialog {
public:
	EditMemosDialog() : PPListDialog(DLG_MEMOLIST, CTL_LBXSEL_LIST)
	{
		SString title;
		setTitle(PPLoadTextS(PPTXT_EDITMEMOS, title));
	}
	int    setDTS(const char * pMemos);
	int    getDTS(SString & rMemos);
private:
	virtual int setupList();
	virtual int addItem(long * pPos, long * pID);
	virtual int editItem(long pos, long id);
	virtual int delItem(long pos, long id);

	StrAssocArray Memos;
};

int EditMemosDialog::setDTS(const char * pMemos)
{
	SString buf;
	StringSet ss(MemosDelim);
	Memos.Z();
	const size_t mlen = sstrlen(pMemos);
	if(mlen)
		ss.setBuf(pMemos, mlen + 1);
	for(uint i = 0, j = 1; ss.get(&i, buf) > 0; j++)
		Memos.Add((long)j, 0, buf, 0);
	updateList(-1);
	return 1;
}

int EditMemosDialog::getDTS(SString & rMemos)
{
	SString buf;
	rMemos.Z();
	for(uint i = 0; i < Memos.getCount(); i++) {
		buf = Memos.Get(i).Txt;
		buf.ReplaceStr(MemosDelim, "", 0);
		if(i != 0)
			rMemos.Cat(MemosDelim);
		rMemos.Cat(buf);
	}
	return 1;
}

// virtual
int EditMemosDialog::setupList()
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

// virtual
int EditMemosDialog::addItem(long * pPos, long * pID)
{
	int    ok = -1;
	SString buf;
	if(InputStringDialog(0, 0, 0, 1, buf) > 0) {
		Memos.Add(Memos.getCount() + 1, 0, buf, 0);
		ASSIGN_PTR(pPos, Memos.getCount() - 1);
		ASSIGN_PTR(pID,  Memos.getCount());
		ok = 1;
	}
	return ok;
}

// virtual
int EditMemosDialog::editItem(long pos, long id)
{
	int    ok = -1;
	SString buf;
	if(Memos.GetText(id, buf) > 0) {
		if(InputStringDialog(0, 0, 0, 1, buf) > 0) {
			Memos.Add(Memos.getCount(), 0, buf);
			ok = 1;
		}
	}
	return ok;
}

// virtual
int EditMemosDialog::delItem(long pos, long id)
{
	Memos.Remove(id);
	return 1;
}

int SLAPI PutObjMemos(PPID objTypeID, PPID prop, PPID objID, const SString & rMemos, int useTa)
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

int SLAPI EditObjMemos(PPID objTypeID, PPID prop, PPID objID)
{
	int    ok = -1;
	SString memos;
	EditMemosDialog * p_dlg = 0;
	PPRef->GetPropVlrString(objTypeID, objID, prop, memos);
	if(!memos.Len() && InputStringDialog(0, 0, 0, 1, memos) > 0) {
		memos.ReplaceStr(MemosDelim, "", 0);
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
public:
	TimePickerDialog();
	~TimePickerDialog();
	int    setDTS(LTIME data);
	int    getDTS(LTIME * pData);
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
	// @v9.6.2 virtual void draw();
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
					int16 top = (i == 0) ? (int16)(rect.top + 30) : Hours.at(i - 1).b.y;
					item.a.x = (int16)(rect.left + offs);
					item.a.y = top;
					item.b.x = (int16)(item.a.x + HourDelta * hours_in_line);
					item.b.y = (int16)(item.a.y + HourDelta);
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
					item.a.x = (int16)(rect.left + offs);
					item.a.y = top;
					item.b.x = (int16)(item.a.x + MinDelta * minuts_in_line);
					item.b.y = (int16)(item.a.y + MinDelta);
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
			TPoint p;
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
			if(pHour && h != -1)
				*pHour = h;
			if(pMinut && m != -1)
				*pMinut = m;
			return 1;
		}

		long HourDelta;
		long MinDelta;
		long HCellLen;
		long MCellLen;
		TSVector <TRect> Hours; // @v9.8.4 TSArray-->TSVector
		TSVector <TRect> Minuts; // @v9.8.4 TSArray-->TSVector
	};
	TimeRects TmRects;
	LTIME  Data;
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
	Ptb.SetBrush(brSquare,        SPaintObj::psSolid, LightenColor(GetColorRef(SClrBrown), 0.5f), 0);
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
		STRNSCPY(log_font.lfFaceName, SUcSwitch(temp_buf)); // @unicodeproblem
		log_font.lfHeight = def_font_size;
		Ptb.SetFont(fontHours, ::CreateFontIndirect(&log_font)); // @unicodeproblem
		log_font.lfWeight = FW_BOLD;
		Ptb.SetFont(fontWorkHours, ::CreateFontIndirect(&log_font)); // @unicodeproblem
		Ptb.SetFont(fontMin, ::CreateFontIndirect(&log_font)); // @unicodeproblem
		log_font.lfHeight = 16;
		log_font.lfItalic = 1;
		Ptb.SetFont(fontTexts, ::CreateFontIndirect(&log_font)); // @unicodeproblem
	}
	TmRects.Init(3, 2, H());
}

TimePickerDialog::~TimePickerDialog()
{
}

IMPL_HANDLE_EVENT(TimePickerDialog)
{
	TDialog::handleEvent(event);
	if(event.isCmd(cmPaint)) {
		Implement_Draw();
	}
	else if(event.isCmd(cmDraw)) {
		Implement_Draw();
	}
	else if(event.isCmd(cmCurTime)) {
		Data = getcurtime_();
		int    m = Data.minut();
		Data = encodetime(Data.hour(), m - (m % 5), 0, 0);
		invalidateRect(getClientRect(), 1);
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
	long   h = Data.hour(), m = Data.minut();
	TmRects.GetTimeByCoord(x, y, &h, &m);
	Data = encodetime(h, m, 0, 0);
	invalidateRect(getClientRect(), 1);
	::UpdateWindow(H());
}

void TimePickerDialog::DrawMainRect(TCanvas * pCanv, RECT * pRect)
{
	TRect m_rect;
	MEMSZERO(m_rect);
	if(TmRects.Minuts.getCount()) {
		m_rect = TmRects.Minuts.at(TmRects.Minuts.getCount() - 1);
		TRect draw_rect(pRect->left + 3, 3, pRect->right - 3, m_rect.b.y + 8);
		// нарисуем прямоугольник
		pCanv->FillRect(draw_rect, (HBRUSH)Ptb.Get(brMainRect));
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
		draw_rect.set(x1, y1, x2, y2);
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
		TPoint text_point;
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
		draw_rect.set(x1, y1, x2, y2);
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
		TPoint text_point;
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
				TPoint round_pt;
				TRect text_rect(x + 1, y + 1, x + delta - 1, y + delta - 1);
				temp_buf.Z().Cat(h_);
				round_pt.Set(6, 6);
				canv.SelectObjectAndPush(Ptb.Get(penSquare));
				if(h_ == h)
					canv.SelectObjectAndPush(Ptb.Get(brSelected));
				else
					canv.SelectObjectAndPush(Ptb.Get(brSquare));
				canv.RoundRect(text_rect, round_pt);
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
				TPoint round_pt;
				TRect  text_rect(x + 1, y + 1, x + delta - 1, y + delta -1);
				temp_buf.Z().Cat(min5);
				round_pt.Set(6, 6);
				canv.SelectObjectAndPush(Ptb.Get(penSquare));
				if(m >= min5 && m < min5 + 5)
					canv.SelectObjectAndPush(Ptb.Get(brSelected));
				else
					canv.SelectObjectAndPush(Ptb.Get(brSquare));
				canv.RoundRect(text_rect, round_pt);
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

int TimePickerDialog::setDTS(LTIME data)
{
	Data = data;
	return 1;
}

int TimePickerDialog::getDTS(LTIME * pData)
{
	ASSIGN_PTR(pData, Data);
	return 1;
}

void SLAPI SetupTimePicker(TDialog * pDlg, uint editCtlID, int buttCtlID)
{
	struct TimeButtonWndEx {
		TimeButtonWndEx(TDialog * pDlg, uint editCtlId, WNDPROC fPrevWndProc) : Dlg(pDlg), EditID(editCtlId), PrevWndProc(fPrevWndProc)
		{
			STRNSCPY(Signature, "papyrusclock");
		}
		static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
		{
			TimeButtonWndEx * p_cbwe = (TimeButtonWndEx *)TView::GetWindowUserData(hWnd);
			switch(message) {
				case WM_DESTROY:
					TView::SetWindowProp(hWnd, GWLP_USERDATA, static_cast<void *>(0));
					if(p_cbwe->PrevWndProc)
						TView::SetWindowProp(hWnd, GWLP_WNDPROC, p_cbwe->PrevWndProc);
					delete p_cbwe;
					return 0;
				case WM_LBUTTONUP:
					{
						LTIME  tm = p_cbwe->Dlg->getCtrlTime(p_cbwe->EditID);
						TimePickerDialog * dlg = new TimePickerDialog;
						if(CheckDialogPtrErr(&dlg)) {
							dlg->setDTS(tm);
							if(ExecView(dlg) == cmOK) {
								dlg->getDTS(&tm);
								p_cbwe->Dlg->setCtrlTime(p_cbwe->EditID, tm);
							}
						}
						delete dlg;
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
	if(CheckDialogPtrErr(&dlg) && dlg->setDTS(Data)) {
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

// virtual
int TimePickerCtrlGroup::setData(TDialog * pDlg, void * pData)
{
	Data = (pData) ? *static_cast<const LTIME *>(pData) : ZEROTIME;
	pDlg->setCtrlData(Ctl, &Data);
	return 1;
}

// virtual
int TimePickerCtrlGroup::getData(TDialog * pDlg, void * pData)
{
	int    ok = 0;
	pDlg->getCtrlData(Ctl, &Data);
	if(checktime(Data) > 0) {
		ASSIGN_PTR(static_cast<LTIME *>(pData), Data);
		ok = 1;
	}
	return ok;
}

// virtual
void TimePickerCtrlGroup::handleEvent(TDialog * pDlg, TEvent & event)
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
		for(uint p = 0; ss.get(&p, buf) > 0;)
			p_rec->AddrList.Add(p_rec->AddrList.getCount() + 1, buf, 0);
	}
	return 1;
}

int EmailCtrlGroup::SetLine(TDialog * pDlg)
{
	SString addr_list;
	for(uint i = 0; i < Data.AddrList.getCount(); i++) {
		addr_list.Cat(Data.AddrList.Get(i).Txt);
		if(i + 1 < Data.AddrList.getCount())
			addr_list.Semicol();
	}
	if(pDlg) {
		pDlg->setCtrlString(Ctl, addr_list);
		pDlg->disableCtrl(Ctl, Data.AddrList.getCount() > 1);
	}
	return 1;
}

class EmailListDlg : public PPListDialog {
public:
	EmailListDlg();
	int    setDTS(const StrAssocArray * pData);
	int    getDTS(StrAssocArray * pData);
protected:
	virtual int setupList();
	virtual int addItem(long * pPos,  long * pID);
	virtual int editItem(long pos, long id);
	virtual int delItem(long pos,  long id);

	StrAssocArray Data;
};

EmailListDlg::EmailListDlg() : PPListDialog(DLG_EMAILADDRS, CTL_EMAILADDRS_LIST)
{
	showCtrl(CTL_LBXSEL_UPBTN,   0);
	showCtrl(CTL_LBXSEL_DOWNBTN, 0);
	updateList(-1);
}

// AHTOXA {
int EmailListDlg::setupList()
{
	int    ok = 1;
	SString temp_buf;
	for(uint i = 0; i < Data.getCount(); i++) {
		long   id = Data.Get(i).Id;
		temp_buf = Data.Get(i).Txt;
		if(!addStringToList(id, temp_buf))
			ok = PPErrorZ();
	}
	return ok;
}

static int SLAPI IsEmailAddr(const char * pPath)
{
	return BIN(pPath && sstrchr(pPath, '@'));
}

int EmailListDlg::addItem(long * pPos, long * pID)
{
	int    ok = -1;
	long   pos = -1, id = -1;
	SString addr, title;
	PPLoadString("email", title);
	if(InputStringDialog(title, title, 0, 0, addr) > 0 && IsEmailAddr(addr)) {
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
	if(pos >= 0 && pos < (long)Data.getCount()) {
		SString title;
		SString addr = Data.Get(pos).Txt;
		PPLoadString("email", title);
		if(InputStringDialog(title, title, 0, 0, addr) > 0 && IsEmailAddr(addr)) {
			Data.Add(id, addr);
			ok = 1;
		}
	}
	return ok;
}

int EmailListDlg::delItem(long pos, long id)
{
	int    ok = -1;
	if(pos >= 0 && pos < (long)Data.getCount()) {
		Data.Remove(id);
		ok = 1;
	}
	return ok;
}

int EmailListDlg::getDTS(StrAssocArray * pData)
{
	ASSIGN_PTR(pData, Data);
	return 1;
}

int EmailListDlg::setDTS(const StrAssocArray * pData)
{
	RVALUEPTR(Data, pData);
	updateList(-1);
	return 1;
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
SLAPI EmailToBlock::EmailToBlock() : MailAccID(0)
{
}

int SLAPI EmailToBlock::Edit(long flags)
{
	class EmailToBlockDialog : public TDialog {
	public:
		explicit EmailToBlockDialog(long flags) : TDialog(DLG_MAILTO), Flags(0)
		{
			addGroup(ctlgroupEmailList, new EmailCtrlGroup(CTL_MAILTO_ADDR, cmEMailList));
		}
		int    setDTS(const EmailToBlock * pData)
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
		int    getDTS(EmailToBlock * pData)
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
		EmailToBlock Data;
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
		(temp_buf = p_path).ToOem();
		if(!addStringToList(i, temp_buf))
			ok = PPErrorZ();
	}
	return ok;
}

// virtual
int SendMailDialog::addItem(long * pPos, long * pId)
{
	int    ok = -1;
	long   pos = -1, id = -1;
	SString path;
	if(PPOpenFile(PPTXT_FILPAT_ALL, path, 0, 0) > 0) {
		if(Data.FilesList.lsearch(path.cptr(), 0, PTR_CMPFUNC(PcharNoCase)) <= 0) {
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

// virtual
int SendMailDialog::editItem(long pos, long id)
{
	int    ok = -1;
	if(pos >= 0 && pos < (long)Data.FilesList.getCount()) {
		SString path = Data.FilesList.at(pos);
		if(PPOpenFile(PPTXT_FILPAT_ALL, path,  0, 0) > 0) {
			uint p = 0;
			if(Data.FilesList.lsearch(path.cptr(), &p, PTR_CMPFUNC(PcharNoCase)) > 0 && p != pos)
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

// virtual
int SendMailDialog::delItem(long pos, long id)
{
	int    ok = -1;
	if(pos >= 0 && pos < (long)Data.FilesList.getCount()) {
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
// HtmlHelp
#include <htmlhelp.h>
#pragma comment (lib, "htmlhelp.lib")

int PPCallHelp(void * hWnd, uint cmd, uint ctx)
{
	int    ok = 0;
	if(oneof3(cmd, HH_HELP_CONTEXT, HH_INITIALIZE, HH_UNINITIALIZE)) {
		SString path;
		if(cmd == HH_HELP_CONTEXT) {
			const char * p_file_name = "pphelp.chm";
			PPGetFilePath(PPPATH_BIN, p_file_name, path);
			if(fileExists(path)) {
				SPathStruc ps(path);
				if(ps.Flags & SPathStruc::fUNC || ((ps.Flags & SPathStruc::fDrv) && GetDriveType(SUcSwitch(ps.Drv.SetLastSlash())) == DRIVE_REMOTE)) { // @unicodeproblem
					//
					// В связи с проблемой загрузки help'а с сетевого каталога коприруем файла в
					// локальный каталог. При этом, чтобы избежать лишних копирований, проверяем, нет ли
					// в локальном каталоге скопированного до этого актуального файла.
					//
					SString local_path;
					if(SFileUtil::GetSysDir(SFileUtil::sdAppDataLocal, local_path)) {
						int    do_copy = 0;
						SFileUtil::Stat st_local, st;
						local_path.SetLastSlash().Cat(SLS.GetAppName());
						if(!fileExists(local_path))
							createDir(local_path);
						local_path.SetLastSlash().Cat(p_file_name);
						if(!fileExists(local_path))
							do_copy = 1;
						else if(SFileUtil::GetStat(local_path, &st_local) && SFileUtil::GetStat(path, &st))
							if(cmp(st_local.ModTime, st.ModTime) < 0)
								do_copy = 1;
							else
								path = local_path; // В локальном каталоге уже лежит актуальный файл: недо использовать его.
						else
							do_copy = 1;
						if(do_copy && copyFileByName(path, local_path))
							path = local_path;
					}
				}
				ok = BIN(HtmlHelp(static_cast<HWND>(hWnd), SUcSwitch(path), cmd, ctx)); // @unicodeproblem
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
	TSArray <HWND> * p_list = (TSArray <HWND> *)lp;
	CALLPTRMEMB(p_list, insert(&hWnd));
	return TRUE;
}

static int TakeInCountCtrl(HWND hWnd, const TSArray <HWND> & rCtrlList, LongArray & rSeenPosList)
{
	for(uint i = 0; i < rCtrlList.getCount(); i++)
		if(rCtrlList.at(i) == hWnd) {
			rSeenPosList.addUnique(i);
			return 1;
		}
	return 0;
}

static SString & _RectToLine(const RECT & rRect, SString & rBuf)
{
	return rBuf.CatChar('(').Cat(rRect.left).CatDiv(',', 2).Cat(rRect.top).CatDiv(',', 2).
		Cat(rRect.right).CatDiv(',', 2).Cat(rRect.bottom).CatChar(')');
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

int SLAPI ExportDialogs(const char * pFileName)
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
	TDialog * dlg = 0;
	StrAssocArray prop_list;
	SFile  f_out(pFileName, SFile::mWrite);
	{
		SPathStruc ps(pFileName);
		ps.Nam.CatChar('-').Cat("text");
		ps.Ext = "tsv";
		ps.Merge(temp_buf);
	}
	SFile f_out_text(temp_buf, SFile::mWrite);
	{
		SPathStruc ps(pFileName);
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
			LongArray seen_pos_list;
			//char   text_buf[1024];
			SString dlg_symb_body;
			TSArray <HWND> child_list;
			prop_list.Z();
			dlg->GetCtlSymb(-1000, symb);
			if(symb.Empty()) {
				dlg_symb_body.Z().Cat(dlg->GetId());
				symb.Z().Cat("DLG").CatChar('_').Cat(dlg_symb_body);
			}
			else
				TDialog::GetSymbolBody(symb, dlg_symb_body);
			// @v9.1.5 SendMessage(dlg->H(), WM_GETTEXT, sizeof(text_buf), (long)text_buf);
			TView::SGetWindowText(dlg->H(), dlg_title_buf); // @v9.1.5
			{
				text_line_buf.Z().Cat(symb).Tab().Cat(dlg_title_buf).CR();
				f_out_text.WriteLine(text_line_buf);
			}
			line_buf.Z().CR().Cat("dialog").Space().Cat(symb).Space().CatQStr(dlg_title_buf);
			if(GetWindowInfo(dlg->H(), &wi))
				_RectToLine(wi.rcWindow, line_buf.Space());
			{
				HFONT h_f = (HFONT)::SendMessage(dlg->H(), WM_GETFONT, 0, 0);
				if(h_f) {
					temp_buf.Z();
					LOGFONT f;
					if(::GetObject(h_f, sizeof(f), &f)) {
						SFontDescr fd(0, 0, 0);
						fd.SetLogFont(&f);
						fd.Size = (int16)MulDiv(fd.Size, 72, GetDeviceCaps(SLS.GetTLA().GetFontDC(), LOGPIXELSY));
						if(!fd.Face.IsEqiAscii("MS Sans Serif") || fd.Size != 8 || fd.Flags || fd.Weight != 0.0f) {
							fd.ToStr(temp_buf, 0);
						}
					}
					if(temp_buf.NotEmptyS()) {
						prop_list.Add(DlScope::cuifFont, temp_buf.Quot('\"', '\"'));
					}
				}
			}
			DlScope::PropListToLine(prop_list, 1, line_buf).CR();
			line_buf.CatChar('{').CR();
			f_out.WriteLine(line_buf);
			{
				// %topic(DLG_BILLSTATUS)
                line_buf.Z().Cat("%topic").CatParStr(dlg_symb_body).CR();
                f_out_manual.WriteLine(line_buf);
                // \ppypict{dlg-billstatus}{Диалог редактирования статуса документов}
                PreprocessCtrlText(dlg_title_buf, ctl_text_processed);
                (temp_buf = dlg_symb_body).ReplaceStr("_", "-", 0).ToLower();
                line_buf.Z().CatChar('\\').Cat("ppypict").CatChar('{').Cat(temp_buf).CatChar('}');
				line_buf.CatChar('{').Cat(ctl_text_processed).CatChar('}').CR();
				f_out_manual.WriteLine(line_buf);
				// \begin{description}
				line_buf.Z().CatChar('\\').Cat("begin").CatChar('{').Cat("description").CatChar('}').CR();
				f_out_manual.WriteLine(line_buf);
			}
			if(EnumChildWindows(dlg->H(), GetChildWindowsList, reinterpret_cast<LPARAM>(&child_list))) {
				for(uint i = 0; i < child_list.getCount(); i++) {
					if(!seen_pos_list.lsearch((long)i)) {
						prop_list.Z();
						line_buf.Z();
						label_text.Z();
						const HWND h = child_list.at(i);
						int   ctl_id = GetDlgCtrlID(h);
						if(GetWindowInfo(h, &wi)) {
							// @v9.1.5 SendMessage(h, WM_GETTEXT, sizeof(text_buf), (long)text_buf);
							TView::SGetWindowText(h, ctl_text); // @v9.1.5
							ctl_text.ReplaceStr("\n", 0, 0);
							TView * p_view = dlg->getCtrlView(ctl_id);
							if(!p_view && ctl_id > 4096) {
								ctl_id -= 4096;
								p_view = dlg->getCtrlView(ctl_id);
							}
							symb = 0; //p_view ? p_view->GetSymb() : 0;
							dlg->GetCtlSymb(ctl_id, symb);
							if(symb.Empty())
								symb.Z().Cat(ctl_id);
							TLabel * p_label = dlg->getCtlLabel(ctl_id);
							RECT   label_rect;
							if(p_label) {
								WINDOWINFO label_wi;
								if(GetWindowInfo(p_label->getHandle(), &label_wi)) {
									// @v9.1.5 SendMessage(p_label->getHandle(), WM_GETTEXT, sizeof(text_buf), (long)text_buf);
									TView::SGetWindowText(p_label->getHandle(), label_text); // @v9.1.5
									label_text.ReplaceStr("\n", 0, 0);
									label_rect = label_wi.rcWindow;
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
								if(p_view && p_view->IsSubSign(TV_SUBSIGN_INPUTLINE)) {
									TInputLine * p_il = static_cast<TInputLine *>(p_view);
									if(p_label)
										ctl_text = label_text;
									ComboBox * p_cb = p_il->GetCombo();
									if(p_cb) {
										// T_COMBOBOX T_IDENT T_CONST_STR uirectopt uictrl_type uictrl_properties ';'
										HWND   h_combo = p_cb->getHandle();
										WINDOWINFO wi_combo;
										if(GetWindowInfo(h_combo, &wi_combo)) {
											dlg->GetCtlSymb(p_cb->GetId(), symb);
											line_buf.Tab().Cat("combobox").Space().Cat(symb).Space().CatQStr(ctl_text);
											_RectToLine(wi.rcWindow, line_buf.Space());
											if(p_label) {
												_RectToLine(label_rect, temp_buf.Z());
												prop_list.Add(DlScope::cuifLabelRect, temp_buf);
											}
											DlScope::PropListToLine(prop_list, 2, line_buf).Semicol().CR();
											f_out.WriteLine(line_buf);
											TakeInCountCtrl(h_combo, child_list, seen_pos_list);
										}
										{
											// \item[\dlgcombo{Счетчик}]
											PreprocessCtrlText(ctl_text, ctl_text_processed);
											line_buf.Z().Tab().CatChar('\\').Cat("item").CatChar('[').CatChar('\\').Cat("dlgcombo").
												CatChar('{').Cat(ctl_text_processed).CatChar('}').CatChar(']').CR().CR();
											f_out_manual.WriteLine(line_buf);
										}
									}
									else {
										if(wi.dwStyle & ES_READONLY) {
											prop_list.Add(DlScope::cuifReadOnly, temp_buf.Z());
										}
										line_buf.Tab().Cat("input").Space().Cat(symb).Space().CatQStr(ctl_text);
										_RectToLine(wi.rcWindow, line_buf.Space());
										GetBinaryTypeString(p_il->getType(), 1, temp_buf, "", 0);
										if(temp_buf.Cmp("unknown", 0) == 0) {
											(temp_buf = "string").CatBrackStr("48");
										}
										line_buf.Space().Cat(temp_buf);
										if(p_label) {
											_RectToLine(label_rect, temp_buf.Z());
											prop_list.Add(DlScope::cuifLabelRect, temp_buf);
										}
										DlScope::PropListToLine(prop_list, 2, line_buf).Semicol().CR();
										f_out.WriteLine(line_buf);
										{
											// \item[Наименование]
											PreprocessCtrlText(ctl_text, ctl_text_processed);
											line_buf.Z().Tab().CatChar('\\').Cat("item").CatBrackStr(ctl_text_processed).CR().CR();
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
									if(p_view && p_view->IsSubSign(TV_SUBSIGN_CLUSTER)) {
										line_buf.Tab().Cat("checkbox").Space().Cat(symb).Space().CatQStr(ctl_text);
										_RectToLine(wi.rcWindow, line_buf.Space()).Semicol().CR();
										f_out.WriteLine(line_buf);
										TakeInCountCtrl(h, child_list, seen_pos_list);
									}
								}
								else if(oneof2(bt, BS_RADIOBUTTON, BS_AUTORADIOBUTTON)) {
								}
								else if(bt == BS_GROUPBOX) {
									if(p_view && p_view->IsSubSign(TV_SUBSIGN_CLUSTER)) {
										TCluster * p_clu = static_cast<TCluster *>(p_view);
										const char * p_kind = 0;
										if(p_clu->getKind() == RADIOBUTTONS) {
											p_kind = "radiocluster";
											{
												// \item[\dlgradioc{Сортировать по}]
												PreprocessCtrlText(ctl_text, ctl_text_processed);
												line_buf.Z().Tab().CatChar('\\').Cat("item").CatChar('[').CatChar('\\').Cat("dlgradioc").
													CatChar('{').Cat(ctl_text_processed).CatChar('}').CatChar(']').CR().CR();
												f_out_manual.WriteLine(line_buf);
											}
										}
										else if(p_clu->getKind() == CHECKBOXES) {
											p_kind = "checkcluster";
										}
										if(p_kind) {
											line_buf.Z().Tab().Cat(p_kind).Space().Cat(symb).Space().CatQStr(ctl_text);
											_RectToLine(wi.rcWindow, line_buf.Space());
											line_buf.Space().CatChar('{').CR();
											f_out.WriteLine(line_buf);
											//
											for(uint j = 0; j < p_clu->getNumItems(); j++) {
												int    button_id = MAKE_BUTTON_ID(ctl_id, j+1);
												HWND   h_item = GetDlgItem(dlg->H(), button_id);
												if(h_item) {
													WINDOWINFO wi_item;
													if(GetWindowInfo(h_item, &wi_item)) {
														// @v9.1.5 SendMessage(h_item, WM_GETTEXT, sizeof(text_buf), (long)text_buf);
														TView::SGetWindowText(h_item, temp_buf); // @v9.1.5
														line_buf.Z().Tab(2).CatQStr(temp_buf);
														_RectToLine(wi_item.rcWindow, line_buf.Space());
														line_buf.Semicol().CR();
														f_out.WriteLine(line_buf);
														if(temp_buf.NotEmpty()) {
															text_line_buf.Z().Cat(symb).Tab().Cat(temp_buf).CR();
															f_out_text.WriteLine(text_line_buf);
															{
																if(p_clu->getKind() == RADIOBUTTONS) {
																	// \item[\dlgradioc{Сортировать по}]
																	PreprocessCtrlText(temp_buf, ctl_text_processed);
																	line_buf.Z().Tab(2).CatChar('\\').Cat("item").CatChar('[').CatChar('\\').Cat("dlgradioi").
																		CatChar('{').Cat(ctl_text_processed).CatChar('}').CatChar(']').CR().CR();
																	f_out_manual.WriteLine(line_buf);
																}
																else if(p_clu->getKind() == CHECKBOXES) {
																	// \dlgflag{Просмотр}
																	PreprocessCtrlText(temp_buf, ctl_text_processed);
																	line_buf.Z().Tab(2).CatChar('\\').Cat("dlgflag").CatChar('{').Cat(ctl_text_processed).CatChar('}').CR().CR();
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
										line_buf.Tab().Cat("framebox").Space().CatQStr(ctl_text);
										_RectToLine(wi.rcWindow, line_buf.Space());
										line_buf.Semicol().CR();
										f_out.WriteLine(line_buf);
										TakeInCountCtrl(h, child_list, seen_pos_list);
									}
								}
								else if(oneof2(bt, BS_PUSHBUTTON, BS_DEFPUSHBUTTON)) {
									//T_BUTTON T_IDENT T_CONST_STR uirectopt T_IDENT uictrl_properties ';'
									if(p_view && p_view->IsSubSign(TV_SUBSIGN_BUTTON)) {
										TButton * p_button = static_cast<TButton *>(p_view);
										uint   cmd_id = p_button->GetCommand();
										temp_buf.Z();
										if(cmd_id)
											dlg->GetCtlSymb(cmd_id+100000, temp_buf);
										else
											temp_buf = "cmNone";
										size_t offs = 0;
										if(temp_buf.CmpPrefix("cma", 0) == 0)
											offs = 3;
										else if(temp_buf.CmpPrefix("cm", 0) == 0)
											offs = 2;
										if(symb.ToLong() != 0) {
											symb.Z().Cat("CTL").CatChar('_').Cat("CMD").CatChar('_').Cat(temp_buf+offs).ToUpper();
										}
										line_buf.Tab().Cat("button").Space().Cat(symb).Space().CatQStr(ctl_text);
										_RectToLine(wi.rcWindow, line_buf.Space());
										line_buf.Space().Cat(temp_buf);
										line_buf.Semicol().CR();
										f_out.WriteLine(line_buf);
										TakeInCountCtrl(h, child_list, seen_pos_list);
									}
								}
							}
							else if(cls_name.IsEqiAscii("Static")) {
								//
								// Этикетки (TLabel) пропускаем (они обрабатываются объектами, которым принадлежат)
								//
								if(!(p_view && p_view->IsSubSign(TV_SUBSIGN_LABEL))) {
									if(wi.dwExStyle & WS_EX_STATICEDGE) {
										prop_list.Add(DlScope::cuifStaticEdge, temp_buf.Z());
									}
									if(!p_view || !p_view->IsSubSign(TV_SUBSIGN_STATIC))
										symb = 0;
									else if(symb.ToLong())
										symb.Z().Cat("CTL").CatChar('_').Cat(dlg_symb_body).CatChar('_').Cat("ST").CatChar('_').CatLongZ(symb.ToLong(), 3);
									line_buf.Tab().Cat("statictext").Space();
									if(symb.NotEmpty())
										line_buf.Cat(symb).Space();
									line_buf.CatQStr(ctl_text);
									_RectToLine(wi.rcWindow, line_buf.Space());
									DlScope::PropListToLine(prop_list, 2, line_buf).Semicol().CR();
									f_out.WriteLine(line_buf);
									TakeInCountCtrl(h, child_list, seen_pos_list);
								}
							}
							else if(cls_name.IsEqiAscii("SysListView32") || cls_name.IsEqiAscii("ListBox")) {
								if(p_view && p_view->IsSubSign(TV_SUBSIGN_LISTBOX)) {
									SmartListBox * p_list = static_cast<SmartListBox *>(p_view);
									if(p_label)
										ctl_text = label_text;
									line_buf.Tab().Cat("listbox").Space().Cat(symb).Space().CatQStr(ctl_text);
									_RectToLine(wi.rcWindow, line_buf.Space());
									p_list->GetOrgColumnsDescr(temp_buf);
									if(temp_buf.NotEmptyS())
										line_buf.Space().Cat("columns").Space().CatQStr(temp_buf);
									line_buf.Semicol().CR();
									f_out.WriteLine(line_buf);
									TakeInCountCtrl(h, child_list, seen_pos_list);
								}
							}
							else if(cls_name.IsEqiAscii("SysTreeView32")) {
								// T_TREELISTBOX T_IDENT T_CONST_STR uirectopt uictrl_properties
								if(p_view && p_view->IsSubSign(TV_SUBSIGN_LISTBOX)) {
									SmartListBox * p_list = static_cast<SmartListBox *>(p_view);
									if(p_label)
										ctl_text = label_text;
									line_buf.Tab().Cat("treelistbox").Space().Cat(symb).Space().CatQStr(ctl_text);
									_RectToLine(wi.rcWindow, line_buf.Space());
									line_buf.Semicol().CR();
									f_out.WriteLine(line_buf);
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
				line_buf.Z().CatChar('\\').Cat("end").CatChar('{').Cat("description").CatChar('}').CR().CR();
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
static const char * WrSubKey_RecentItems = "Software\\Papyrus\\RecentItems";

SLAPI RecentItemsStorage::RecentItemsStorage(int ident, uint maxItems, CompFunc cf) : Ident(ident), MaxItems(maxItems), Cf(cf)
{
}

SLAPI RecentItemsStorage::RecentItemsStorage(const char * pIdent, uint maxItems, CompFunc cf) : Ident(0), IdentText(pIdent), MaxItems(maxItems), Cf(cf)
{
}

SLAPI RecentItemsStorage::~RecentItemsStorage()
{
}

int SLAPI RecentItemsStorage::CheckIn(const char * pText)
{
	int    ok = -1;
	if(!isempty(pText)) {
		SString key;
		if(IdentText.NotEmpty())
			key = IdentText;
		else if(Ident > 0)
			key.CatChar('#').Cat(Ident);
		if(key.NotEmpty()) {
			WinRegKey reg_key(HKEY_CURRENT_USER, WrSubKey_RecentItems, 0);
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
					const int is_eq = Cf ? BIN(Cf(pText, temp_buf.cptr(), 0) == 0) : BIN(strcmp(pText, temp_buf) == 0);
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

int SLAPI RecentItemsStorage::GetList(StringSet & rSs)
{
	rSs.clear();
	int    ok = -1;
	SString key;
	if(IdentText.NotEmpty())
		key = IdentText;
	else if(Ident > 0)
		key.CatChar('#').Cat(Ident);
	if(key.NotEmpty()) {
		WinRegKey reg_key(HKEY_CURRENT_USER, WrSubKey_RecentItems, 0);
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

int SLAPI PPEditTextFile(const char * pFileName)
{
	class OpenEditFileDialog : public TDialog {
	public:
		explicit OpenEditFileDialog(RecentItemsStorage * pRis) : TDialog(DLG_OPENEDFILE), FileID(0), P_Ris(pRis)
		{
			FileBrowseCtrlGroup::Setup(this, CTLBRW_OPENEDFILE_SELECT, CTL_OPENEDFILE_SELECT, 1, 0,
				PPTXT_FILPAT_PPYTEXT, FileBrowseCtrlGroup::fbcgfFile|FileBrowseCtrlGroup::fbcgfSaveLastPath);
			SetupStrListBox(this, CTL_OPENEDFILE_RESERV);
			SetupStrListBox(this, CTL_OPENEDFILE_RECENT);
			SetupReservList();
			SetupRecentList();
		}
		int    GetFileName(SString & rBuf)
		{
			// @v10.2.0 {
			if(FileName.Empty() && !FileID)
				getCtrlString(CTL_OPENEDFILE_SELECT, FileName);
			// } @v10.2.0
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
						if(p_box->def) {
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
						if(p_box->def) {
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
					if(p_reserv_box->def) {
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
							ss.clear();
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
						ss.clear();
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
		StrAssocArray RecentItems;
		RecentItemsStorage * P_Ris;
	};
	int    ok = -1;
	RecentItemsStorage ris("PPViewTextBrowser", 20, PTR_CMPFUNC(FilePathUtf8));
	OpenEditFileDialog * dlg = 0;
	SString file_name = pFileName;
	if(!file_name.NotEmptyS() || !fileExists(file_name)) {
		THROW(CheckDialogPtr(&(dlg = new OpenEditFileDialog(&ris))));
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
int SLAPI BigTextDialog(uint maxLen, const char * pTitle, SString & rText)
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
			if(maxLen > 0 && maxLen <= 4000)
				SetupInputLine(CTL_BIGTXTEDIT_TEXT, MKSTYPE(S_ZSTRING, maxLen), MKSFMT(maxLen, 0));
			else
				ok = 0;
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
		dlg->setTitle(pTitle);
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
#if 0 // @construction {

class ProxiAuthDialog : public TDialog {
public:
	ProxiAuthDialog() : TDialog(DLG_NETPROXI)
	{
	}
	int    setDTS(const SProxiAuthParam * pData)
	{
		int    ok = 1;
		Data = *pData;

		return ok;
	}
	int    getDTS(SProxiAuthParah * pData)
	{
		int    ok = 1;

		ASSIGN_PTR(pData, Data);
		return ok;
	}
private:
	DECL_HANDLE_EVENT
	{
		TDialog::handleEvent(event);
	}
	SProxiAuthParam Data;
};

#endif // } 0 @construction
