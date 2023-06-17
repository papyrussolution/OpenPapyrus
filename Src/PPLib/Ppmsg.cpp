// PPMSG.CPP
// Copyright (c) A.Sobolev 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2008, 2009, 2010, 2012, 2013, 2015, 2016, 2017, 2018, 2019, 2020, 2021, 2022, 2023
// @codepage UTF-8
//
#include <pp.h>
#pragma hdrstop
#include <strstore.h>

static StringStore2 * _PPStrStore = 0; // @global @threadsafe

int PPInitStrings(const char * pFileName)
{
	ENTER_CRITICAL_SECTION
	if(_PPStrStore == 0) {
		SString name;
		SString temp_buf;
		StrAssocArray file_lang_list;
		if(pFileName == 0)
			PPGetFilePath(PPPATH_BIN, "ppstr.bin", name);
		else
			name = pFileName;
        {
            SPathStruc ps(name);
			if(!ps.Nam.HasChr('-')) {
				const SString org_nam = ps.Nam;
				const SString org_ext = ps.Ext;
				(ps.Nam = org_nam).CatChar('-').CatChar('*');
				ps.Merge(temp_buf);
				{
					SString lang_symb;
					SDirEntry de;
					SPathStruc ps_lang;
					for(SDirec direc(temp_buf, 0); direc.Next(&de) > 0;) {
						if(!de.IsFolder()) {
							de.GetNameA(temp_buf);
							size_t hyphen_pos = 0;
							if(temp_buf.SearchChar('-', &hyphen_pos)) {
								temp_buf.Sub(hyphen_pos+1, temp_buf.Len(), lang_symb.Z());
								const int slang = RecognizeLinguaSymb(lang_symb, 0);
								if(slang > 0) {
									de.GetNameA(temp_buf);
									ps_lang.Split(temp_buf);
									ps.Nam = ps_lang.Nam;
									ps.Ext = ps_lang.Ext;
									ps.Merge(temp_buf);
									file_lang_list.Add(slang, temp_buf);
								}
							}
						}
					}
				}
			}
			file_lang_list.Add(0, name); // Базовый файл должен идти последним в списке языков
        }
		_PPStrStore = new StringStore2();
		{
	#ifdef NDEBUG
			const int self_test = 0;
	#else
			const int self_test = 1;
	#endif
			int    _done = 0;
			for(uint i = 0; i < file_lang_list.getCount(); i++) {
				name = file_lang_list.at_WithoutParent(i).Txt;
				if(_PPStrStore->Load(name, self_test)) {
					_done = 1;
				}
				else {
					char err_msg[1024];
					sprintf(err_msg, "Unable to load string resource '%s'", name.cptr());
					::MessageBox(0, SUcSwitch(err_msg), _T("Error"), MB_OK|MB_ICONERROR); // @unicodeproblem
					ZDELETE(_PPStrStore);
				}
			}
			if(self_test && _PPStrStore) {
				assert(_PPStrStore->GetString(PPSTR_TEXT, PPTXT_TESTSTRING, temp_buf));
				assert(temp_buf == "abc @def ghi");
				_PPStrStore->GetDescription(PPSTR_TEXT, PPTXT_TESTSTRING, temp_buf);
				assert(temp_buf == "description for teststring");
			}
		}
	}
	return BIN(_PPStrStore);
	LEAVE_CRITICAL_SECTION
}

void PPReleaseStrings()
{
	if(_PPStrStore)
		DO_CRITICAL(ZDELETE(_PPStrStore));
}

const SymbHashTable * FASTCALL PPGetStringHash(int group)
{
	return _PPStrStore ? _PPStrStore->GetStringHash(group) : 0;
}

int STDCALL PPLoadStringUtf8(int group, int code, SString & s) // @cs
{
	//
	// Эта функция @threadsafe поскольку StrStore2::GetString является const-функцией
	//
	int    ok = 1;
	s.Z();
	if(group && code) {
		PROFILE_START
		ok = _PPStrStore ? _PPStrStore->GetString(group, code, s) : 0;
		PROFILE_END
		if(!ok)
			PPSetErrorSLib();
	}
	return ok;
}

int STDCALL PPLoadString(int group, int code, SString & s)
{
	int    ok = PPLoadStringUtf8(group, code, s);
	if(s.Len() && !sisascii(s.cptr(), s.Len()))
		s.Transf(CTRANSF_UTF8_TO_INNER);
	return ok;
}

SString & STDCALL PPLoadStringS(int group, int code, SString & s)
{
	PPLoadString(group, code, s);
	return s;
}

int FASTCALL PPExpandString(SString & rS, int ctransf)
{
	return _PPStrStore ? _PPStrStore->ExpandString(rS, ctransf) : 0;
}

static FORCEINLINE int Implement_PPLoadString(const char * pSignature, bool utf8, SString & rBuf)
{
	//
	// Эта функция @threadsafe поскольку StrStore2::GetString является const-функцией
	//
	int    ok = 1;
	rBuf.Z();
	if(isempty(pSignature))
		ok = -1;
	else {
		PROFILE_START
		ok = _PPStrStore ? _PPStrStore->GetString(pSignature, rBuf) : 0;
		PROFILE_END
		if(!utf8)
			rBuf.Transf(CTRANSF_UTF8_TO_INNER);
		if(!ok)
			PPSetErrorSLib();
	}
	return ok;
}

int FASTCALL PPLoadString(const char * pSignature, SString & rBuf) { return Implement_PPLoadString(pSignature, false, rBuf); }
int FASTCALL PPLoadStringUtf8(const char * pSignature, SString & rBuf) { return Implement_PPLoadString(pSignature, true, rBuf); }

SString & FASTCALL PPLoadStringS(const char * pSignature, SString & rBuf)
{
	Implement_PPLoadString(pSignature, false, rBuf);
	return rBuf;
}

SString & FASTCALL PPLoadStringUtf8S(const char * pSignature, SString & rBuf)
{
	Implement_PPLoadString(pSignature, true, rBuf);
	return rBuf;
}

int FASTCALL PPLoadStringDescription(const char * pSignature, SString & rBuf)
{
	rBuf.Z();
	int    ok = 0;
	if(isempty(pSignature))
		ok = -1;
	else {
		if(_PPStrStore) {
			ok = _PPStrStore->GetDescription(pSignature, rBuf);
			if(ok > 0) {
				rBuf.Transf(CTRANSF_UTF8_TO_INNER);
				_PPStrStore->ExpandString(rBuf, CTRANSF_UTF8_TO_INNER); // @v10.9.6 
			}
		}
		if(!ok)
			PPSetErrorSLib();
	}
	return ok;
}

int FASTCALL PPLoadText(int code, SString & s)
{
	return PPLoadString(PPSTR_TEXT, code, s);
}

SString & FASTCALL PPLoadTextS(int code, SString & s)
{
	PPLoadString(PPSTR_TEXT, code, s);
	return s;
}

int FASTCALL PPLoadTextWin(int code, SString & s)
{
	int    ok = PPLoadString(PPSTR_TEXT, code, s);
	s.Transf(CTRANSF_INNER_TO_OUTER);
	return ok;
}

int STDCALL PPLoadError(int code, SString & s, const char * pAddInfo)
{
	return PPGetMessage(mfError, code, pAddInfo, DS.CheckExtFlag(ECF_SYSSERVICE), s);
}

int FASTCALL PPSetError(int errCode)
{
	PPThreadLocalArea & tla = DS.GetTLA();
	if(&tla && tla.IsConsistent()) {
		tla.LastErr = errCode;
		tla.AddedMsgString.Z();
	}
	return 0;
}

int FASTCALL PPSetLibXmlError(const xmlParserCtxt * pCtx)
{
	PPThreadLocalArea & tla = DS.GetTLA();
	if(&tla && tla.IsConsistent()) {
		tla.LastErr = PPERR_LIBXML;
		tla.AddedMsgString = 0;
		if(pCtx) {
			if(pCtx->lastError.code)
				tla.AddedMsgString.CatDivIfNotEmpty(' ', 0).Cat(pCtx->lastError.code);
			if(!isempty(pCtx->lastError.message))
				tla.AddedMsgString.CatDivIfNotEmpty(' ', 0).Cat(pCtx->lastError.message);
			if(!isempty(pCtx->lastError.file))
				tla.AddedMsgString.CatDivIfNotEmpty(' ', 0).Cat(pCtx->lastError.file);
		}
		if(!tla.AddedMsgString.NotEmptyS())
			tla.AddedMsgString = "unknown";
	}
	return 0;
}

int PPSetErrorNoMem() { return PPSetError(PPERR_NOMEM); }
int PPSetErrorInvParam() { return PPSetError(PPERR_INVPARAM); }
int PPSetErrorSLib() { return PPSetError(PPERR_SLIB); }
int PPSetErrorDB() { return PPSetError(PPERR_DBENGINE); }

int FASTCALL PPSetError(int errCode, const char * pAddedMsg)
{
	PPThreadLocalArea & tla = DS.GetTLA();
	if(&tla && tla.IsConsistent()) {
		tla.LastErr = errCode;
		tla.AddedMsgString.Z().CatN(pAddedMsg, 256);
	}
	return 0;
}

int FASTCALL PPSetError(int errCode, long val)
{
	PPThreadLocalArea & tla = DS.GetTLA();
	if(&tla && tla.IsConsistent()) {
		tla.LastErr = errCode;
		tla.AddedMsgString.Z().Cat(val);
	}
	return 0;
}

int STDCALL PPSetObjError(int errCode, PPID objType, PPID objID)
{
	PPThreadLocalArea & tla = DS.GetTLA();
	if(&tla && tla.IsConsistent()) {
		tla.LastErrObj.Obj = objType;
		tla.LastErrObj.Id  = objID;
		tla.LastErr = errCode;
	}
	return 0;
}

int FASTCALL PPCheckGetObjPacketID(PPID objType, PPID id)
{
	int    ok = 1;
	if(!id) {
		SString & r_temp_buf = SLS.AcquireRvlStr();
		GetObjectTitle(objType, r_temp_buf);
		ok = PPSetError(PPERR_QUERYOBJPACKBYZEROID, r_temp_buf);
	}
	return ok;
}

void FASTCALL PPSetAddedMsgString(const char * pStr)
{
	DS.GetTLA().AddedMsgString = pStr;
}

void FASTCALL PPSetAddedMsgObjName(PPID objType, PPID objID)
{
	SString obj_name;
	GetObjectName(objType, objID, obj_name);
	PPSetAddedMsgString(obj_name);
}

int FASTCALL PPGetLastErrorMessage(int rmvSpcChrs, SString & rBuf)
{
	return PPGetMessage(mfError, /**/PPErrCode, 0, rmvSpcChrs, rBuf);
}

int STDCALL PPGetMessage(uint options, int msgcode, const char * pAddInfo, int rmvSpcChrs, SString & rBuf)
{
	const PPThreadLocalArea & r_ds_tla = DS.GetConstTLA();
	SString temp_buf;
	char   btr_err_code[16];
	char   fname[MAXPATH];
	int    group = 0;
	int    addcode = 0;
	int    is_win_msg = 0; // 1 as win32 msg, 2 as socket msg
	rBuf.Z();
	switch(options & 0x00ff) {
		case mfError:
			group = PPMSG_ERROR;
			switch(msgcode) {
				case PPERR_DBENGINE:
					{
						const int _btr_err_code = BtrError;
						if(_btr_err_code) {
							if(_btr_err_code == BE_SLIB) {
								; // @fallthrough : управление передается ветке {case PPERR_SLIB}
							}
							else {
								group   = addcode = PPSTR_DBENGINE;
								msgcode = _btr_err_code;
								if(msgcode == BE_ORA_TEXT)
									pAddInfo = DBS.GetAddedMsgString();
								else if(!pAddInfo && DBTable::GetLastErrorFileName())
									pAddInfo = DBTable::GetLastErrorFileName();
								break;
							}
						}
						else
							break;
					}
				case PPERR_SLIB:
					{
						const int _sl_err_code = SLibError; // Обращение к SLibError довольно дорогое
						if(_sl_err_code) {
							if(_sl_err_code == SLERR_WINDOWS)
								is_win_msg = 1;
							else if(_sl_err_code == SLERR_SOCK_WINSOCK) {
								is_win_msg = 2;
								group = addcode = PPSTR_SLIBERR;
								msgcode = _sl_err_code;
							}
							else if(_sl_err_code == SLERR_CURL) {
								const int ce = SLS.GetConstTLA().LastCurlErr;
								group = addcode = PPSTR_CURLERR;
								msgcode = ce;
							}
							else {
								group   = addcode = PPSTR_SLIBERR;
								msgcode = _sl_err_code;
							}
						}
					}
					break;
				case PPERR_DBLIB:
					{
						const int _db_err_code = DBErrCode;
						if(_db_err_code) {
							group = addcode = msgcode;
							if(_db_err_code == SDBERR_SLIB) {
								group = PPSTR_SLIBERR;
								msgcode = SLibError;
							}
							else if(_db_err_code == SDBERR_BTRIEVE) {
								group = PPSTR_DBENGINE;
								msgcode = BtrError;
							}
							else {
								group = PPSTR_DBLIB; // @v10.8.2
								msgcode = _db_err_code;
							}
						}
					}
					break;
				case PPERR_REFSEXISTS:
				case PPERR_REFISBUSY:
				case PPERR_OBJNFOUND:
				case PPERR_DUPSYMB:
				case PPERR_LOTRESTBOUND:
					{
						const PPObjID last_err_obj = r_ds_tla.LastErrObj;
						GetObjectTitle(last_err_obj.Obj, temp_buf.Z()).CatCharN(' ', 2);
						ideqvalstr(last_err_obj.Id, temp_buf).Space().CatChar('(');
						GetObjectName(last_err_obj.Obj, last_err_obj.Id, temp_buf, 1);
						temp_buf.CatChar(')');
						pAddInfo = STRNSCPY(fname, temp_buf);
					}
					break;
				case PPERR_DBQUERY:
					break;
				case PPERR_CRYSTAL_REPORT:
					group = addcode = PPSTR_CRYSTAL_REPORT;
					msgcode = CrwError;
					break;
				case PPERR_NORIGHTS:
					if(r_ds_tla.AddedMsgStrNoRights.IsEmpty()) {
						GetCurUserName(temp_buf.Z());
						STRNSCPY(fname, temp_buf);
					}
					else
						STRNSCPY(fname, r_ds_tla.AddedMsgStrNoRights);
					pAddInfo = fname;
					break;
			}
			break;
		case mfWarn: group = PPMSG_WARNING;      break;
		case mfInfo: group = PPMSG_INFORMATION;  break;
		case mfConf: group = PPMSG_CONFIRMATION; break;
		case mfCritWarn: group = PPMSG_CONFIRMATION; break;
		default:
			return 0;
	}
	{
		SString base_msg_buf;
		temp_buf.Z();
		if(!pAddInfo) {
			if(oneof3(group, PPSTR_DBENGINE, PPERR_DBLIB, PPSTR_DBLIB)) // @v10.8.2 PPSTR_DBLIB
				pAddInfo = DBS.GetConstTLA().AddedMsgString;
			else if(group == PPSTR_SLIBERR)
				pAddInfo = SLS.GetConstTLA().AddedMsgString;
			else
				pAddInfo = r_ds_tla.AddedMsgString;
		}
		if(is_win_msg) {
			const int c = (is_win_msg == 2) ? SLS.GetConstTLA().LastSockErr : SLS.GetOsError();
			SSystem::SFormatMessage(c, temp_buf);
			temp_buf.Chomp().Transf(CTRANSF_OUTER_TO_INNER); 
		}
		else if(msgcode) {
	__loadstring:
			const int lsr = PPLoadString(group, msgcode, base_msg_buf);
			if(lsr) {
				if(pAddInfo)
					temp_buf.Printf(base_msg_buf, pAddInfo);
				else
					temp_buf = base_msg_buf;
			}
			else if(SLibError == SLERR_NOFOUND && addcode) {
				if(addcode == PPERR_DBENGINE) {
					group   = PPMSG_ERROR;
					msgcode = PPERR_DBENGINE;
					addcode = 0;
					pAddInfo = itoa(BtrError, btr_err_code, 10);
				}
				else {
					group   = PPMSG_ERROR;
					msgcode = addcode;
					addcode = 0;
				}
				goto __loadstring;
			}
			else if(addcode == PPSTR_CRYSTAL_REPORT) {
				PPLoadString("err_crpe", base_msg_buf);
				base_msg_buf.Space().CatParStr(msgcode);
				temp_buf.CatN(base_msg_buf, PP_MSGLEN);
			}
			else {
				PPLoadString(PPMSG_ERROR, PPERR_TEXTLOADINGFAULT, temp_buf);
				temp_buf.CatDiv(':', 2).CatParStr(group).Space().Cat(msgcode);
				pAddInfo = temp_buf;
				msgcode = 0;
			}
		}
		rBuf = msgcode ? temp_buf : pAddInfo;
		if(rmvSpcChrs)
			rBuf.ReplaceChar('\003', ' ').ReplaceChar('\n', ' ').ReplaceStr("  ", " ", 0); // @v10.6.7 .ReplaceStr("  ", " ", 0)
	}
	return 1;
}

static int FASTCALL Helper_PPError(int errcode, const char * pAddInfo, uint extraMfOptions)
{
	int    r = 0;
	//
	// Так как функция PPMessage может изменить контекст ошибки, сохраняем его
	// для вывода в журнал последней отображенной ошибки
	//
	PPSaveErrContext();
	int    ok = PPMessage(mfError|mfOK|extraMfOptions, ((errcode >= 0) ? errcode : PPErrCode), pAddInfo);
	if(ok > 0) {
		if(DS.IsThreadInteractive()) {
			PPRestoreErrContext();
			r = 1;
			PPLogMessage(PPFILNAM_ERRMSG_LOG, 0, LOGMSGF_TIME|LOGMSGF_USER|LOGMSGF_LASTERR|LOGMSGF_DBINFO);
		}
	}
	if(!r)
		PPRestoreErrContext();
	return ok;
}

int FASTCALL PPError(int errcode, const char * pAddInfo)
	{ return Helper_PPError(errcode, pAddInfo, 0); }
int FASTCALL PPError(int errcode)
	{ return Helper_PPError(errcode, 0, 0); }
int STDCALL  PPError(int errcode, const char * pAddInfo, uint extraMfOptions)
	{ return Helper_PPError(errcode, pAddInfo, extraMfOptions); }
int PPError()
	{ return Helper_PPError(-1, 0, 0); }

int FASTCALL PPErrorTooltip(int errcode, const char * pAddInfo)
{
	int    ok = 0;
	int    r = 0;
	//
	// Так как функция PPMessage может изменить контекст ошибки, сохраняем его
	// для вывода в журнал последней отображенной ошибки
	//
	PPSaveErrContext();
	ok = PPTooltipMessage(mfError|mfOK, ((errcode >= 0) ? errcode : PPErrCode), pAddInfo);
	if(ok > 0) {
		if(DS.IsThreadInteractive()) {
			PPRestoreErrContext();
			r = 1;
			PPLogMessage(PPFILNAM_ERRMSG_LOG, 0, LOGMSGF_TIME|LOGMSGF_USER|LOGMSGF_LASTERR|LOGMSGF_DBINFO);
		}
	}
	if(!r)
		PPRestoreErrContext();
	return ok;
}

int PPErrorZ()
{
	return (PPError(-1, 0), 0);
}

int PPDbSearchError()
{
	return (BTROKORNFOUND) /**/ ? -1 : PPSetErrorDB();
}

static int PPCriticalWarning(SString & rMsg, uint /*options*/)
{
	if(DS.IsThreadInteractive()) {
		int    yes = cmCancel;
		SString answ;
		rMsg.ReplaceChar('\003', ' ');
		TDialog * dlg = new TDialog(DLG_CRITWARN);
		if(CheckDialogPtrErr(&dlg)){
			dlg->setStaticText(CTL_CRITWARN_HEAD, rMsg);
			if(ExecView(dlg) == cmOK) {
				dlg->getCtrlString(CTL_CRITWARN_ANSWER, answ);
				SString yes_str;
				PPLoadString("yes", yes_str);
				yes = (stricmp866(answ, yes_str) != 0) ? cmCancel : cmYes;
			}
		}
		else
			yes = 0;
		delete dlg;
		return yes;
	}
	else
		return cmYes;
}

int STDCALL PPMessage(uint options, int msgcode, const char * pAddInfo)
{
	int    ok = 0;
	SString buf;
	if(PPGetMessage(options, msgcode, pAddInfo, DS.CheckExtFlag(ECF_SYSSERVICE), buf)) {
		PPWaitStop();
		ok = ((options & mfCritWarn) == mfCritWarn) ? PPCriticalWarning(buf, options) : PPOutputMessage(buf, options);
	}
	return ok;
}

int FASTCALL PPMessage(uint options, int msgcode)
{
	int    ok = 0;
	SString buf;
	if(PPGetMessage(options, msgcode, 0, DS.CheckExtFlag(ECF_SYSSERVICE), buf)) {
		PPWaitStop();
		ok = ((options & mfCritWarn) == mfCritWarn) ? PPCriticalWarning(buf, options) : PPOutputMessage(buf, options);
	}
	return ok;
}

int PPOutputMessage(const char * pMsg, uint options)
{
	if(DS.IsThreadInteractive()) {
		if(SLS.CheckUiFlag(sluifUseLargeDialogs))
			options |= mfLargeBox;
		return messageBox(pMsg, options);
	}
	else {
		if((options & mfConf) != mfConf)
			PPLogMessage((options & mfError) ? PPFILNAM_ERR_LOG : PPFILNAM_INFO_LOG, pMsg, LOGMSGF_TIME);
		return 1;
	}
}

int PPTooltipMessage(const char * pMsg, const char * pImgPath, HWND parent, long timer, COLORREF color, long flags)
{
	int    ok = 0;
	if(DS.IsThreadInteractive()) {
		if(pMsg || pImgPath) {
			SMessageWindow * p_win = new SMessageWindow;
			if(p_win) {
				SString buf(pMsg);
				buf.ReplaceChar('\003', ' ').Strip();
				ok = p_win->Open(buf, pImgPath, parent, 0, timer, color, flags, 0);
			}
		}
	}
	return ok;
}

int PPTooltipMessage(uint options, int msgcode, const char * pAddInfo)
{
	int    ok = 0;
	if(DS.IsThreadInteractive()) {
		SString buf;
		if(PPGetMessage(options, msgcode, pAddInfo, DS.CheckExtFlag(ECF_SYSSERVICE), buf)) {
			SMessageWindow * p_win = new SMessageWindow;
			if(p_win) {
				buf.ReplaceChar('\003', ' ').Strip();
				COLORREF color = GetColorRef(SClrSteelblue);
				long   flags = SMessageWindow::fSizeByText|SMessageWindow::fOpaque|SMessageWindow::fPreserveFocus;
				if(options & mfError) {
					color = GetColorRef(SClrRed);
					// @v11.2.9 flags |= SMessageWindow::fShowOnCenter; 
					flags |= SMessageWindow::fShowOnRUCorner|SMessageWindow::fTopmost; // @v11.2.9 
				}
				ok = p_win->Open(buf, 0, 0, 0, 30000, color, flags, 0);
			}
		}
	}
	return ok;
}
//
//
static void AlignWaitDlg(HWND hw); // Prototype

PPThreadLocalArea::WaitBlock::WaitBlock() : State(stValid), PrevView(0), WaitDlg(0), OrgCur(0), hwndPB(0), hwndST(0), PrevPercent(-1), PrevPromille(-1),
	WaitCur(::LoadCursor(TProgram::GetInst(), MAKEINTRESOURCE(IDC_PPYWAIT))), IdleTimer(500)
{
}

PPThreadLocalArea::WaitBlock::~WaitBlock()
{
	Stop();
	DestroyCursor(WaitCur);
	State &= ~stValid; // @v10.5.3
}

bool PPThreadLocalArea::WaitBlock::IsValid() const { return LOGIC(State & stValid); }
HWND PPThreadLocalArea::WaitBlock::GetWindowHandle() const { return WaitDlg; }

static INT_PTR CALLBACK WaitDialogWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	PPThreadLocalArea::WaitBlock * p_blk = static_cast<PPThreadLocalArea::WaitBlock *>(TView::GetWindowUserData(hWnd));
	switch(message) {
		case WM_INITDIALOG:
			{
				TView::SetWindowUserData(hWnd, reinterpret_cast<void *>(lParam));
				HWND hw_text_ctrl = GetDlgItem(hWnd, CTL_WAIT_TEXT);
				if(hw_text_ctrl) {
					SString temp_buf;
					PPLoadString("wait", temp_buf);
					temp_buf.Transf(CTRANSF_INNER_TO_OUTER);
					SetWindowText(hw_text_ctrl, SUcSwitch(temp_buf));
				}
			}
			break;
		default:
			return 0;
	}
	return 1;
}

int PPThreadLocalArea::WaitBlock::Start()
{
	int    ok = 1;
	State |= stValid;
	if(!WaitDlg) {
		PrevView = 0;
		WaitDlg = APPL->CreateDlg(DLG_WAIT, APPL->H_MainWnd, WaitDialogWndProc, reinterpret_cast<LPARAM>(this));
		PrevPercent = -1;
		PrevPromille = -1;
		IdleTimer.Restart(1000);
		if(WaitDlg) {
			TView * p_cur = APPL->P_DeskTop->GetCurrentView();
			PrevView = (p_cur && p_cur->IsConsistent()) ? p_cur : 0;
			AlignWaitDlg(WaitDlg);
			::ShowWindow(WaitDlg, SW_SHOWNA);
			hwndPB = ::GetDlgItem(WaitDlg, 101);
			hwndST = ::GetDlgItem(WaitDlg, CTL_WAIT_TEXT);
			::UpdateWindow(hwndST);
			if(WaitCur)
				OrgCur = SetCursor(WaitCur);
		}
		else {
			State &= stValid;
			ok = 0;
		}
	}
	else
		ok = -1;
	return ok;
}

int PPThreadLocalArea::WaitBlock::Stop()
{
	int    ok = 1;
	if(WaitDlg) {
	   	uint32 save;
		if(PrevView && PrevView->IsConsistent()) {
			save = PrevView->ViewOptions;
			PrevView->ViewOptions |= ofSelectable;
		}
		::DestroyWindow(WaitDlg);
		WaitDlg = 0;
		::SetActiveWindow(APPL->H_TopOfStack);
		if(OrgCur)
			::SetCursor(OrgCur);
		if(PrevView && PrevView->IsConsistent())
			PrevView->ViewOptions = save;
	}
	else
		ok = -1;
	return ok;
}

int PPThreadLocalArea::WaitBlock::Hide()
{
	int    ok = 0;
	if(WaitDlg) {
		if(!(State & stHide)) {
			::ShowWindow(WaitDlg, SW_HIDE);
			State |= stHide;
			ok = 1;
		}
		else
			ok = -1;
	}
	return ok;
}

int PPThreadLocalArea::WaitBlock::Show()
{
	int    ok = 0;
	if(WaitDlg) {
		if(State & stHide) {
			::ShowWindow(WaitDlg, SW_SHOWNA);
			State &= ~stHide;
			ok = 1;
		}
		else
			ok = -1;
	}
	return ok;
}

void FASTCALL PPThreadLocalArea::WaitBlock::SetMessage(const char * pMsg)
{
	PROFILE_START
	if(IdleTimer.Check(0) && APPL && APPL->H_MainWnd)
		::SendMessage(APPL->H_MainWnd, WM_ENTERIDLE, 0, 0);
	DS.SetThreadNotification(PPSession::stntMessage, pMsg);
	//
	PPAdviseList adv_list;
	if(DS.GetAdviseList(PPAdviseBlock::evWaitMsg, 0, adv_list) > 0) {
		PPNotifyEvent ev;
		PPAdviseBlock adv_blk;
		for(uint j = 0; adv_list.Enum(&j, &adv_blk);) {
			if(adv_blk.Proc) {
				ev.Clear();
				ev.PutExtStrData(PPNotifyEvent::extssMessage, pMsg);
				adv_blk.Proc(PPAdviseBlock::evWaitMsg, &ev, adv_blk.ProcExtPtr);
			}
		}
	}
	PROFILE_END
	if(hwndST) {
		if(pMsg) {
			if(Text != pMsg) {
				(Text = pMsg).Transf(CTRANSF_INNER_TO_OUTER);
				TView::SSetWindowText(hwndST, Text);
			}
		}
		else if(Text.NotEmpty()) {
			TView::SSetWindowText(hwndST, Text.Z());
		}
		TProgram::IdlePaint();
	}
}

void STDCALL PPThreadLocalArea::WaitBlock::SetPercent(ulong p, ulong t, const char * msg)
{
	const  ulong  promille = static_cast<ulong>(t ? (1000.0 * fdivui(p, t)) : 1000.0);
	const  ulong  percent = promille / 10;//static_cast<ulong>(t ? (100.0 * fdivui(p, t)) : 100.0);
	if(percent != PrevPercent || (msg && msg[0] && PrevMsg.Cmp(msg, 0) != 0)) {
		PrevPercent = percent;
		PrevPromille = promille;
		PrevMsg = msg;
		if(hwndPB) {
			ShowWindow(hwndPB, SW_SHOWNA);
			SendMessage(hwndPB, PBM_SETPOS, percent, 0);
		}
		char b[1024], * s;
		if(msg) {
			s = stpcpy(b, msg);
			*s++ = ' ';
		}
		else
			s = b;
		ultoa(percent, s, 10);
		s = b + sstrlen(b);
		*s++ = '%';
		*s = 0;
		SetMessage(b);
	}
	else if(promille != PrevPromille) {
		PrevPromille = promille;
		SetMessage(Text);
	}
}

#define __WD DS.GetTLA().WD

static void AlignWaitDlg(HWND hw)
{
	SETIFZ(hw, __WD.GetWindowHandle());
	if(hw) {
		RECT   r1, r2;
		GetWindowRect(hw, &r1);
		GetWindowRect(APPL->H_MainWnd, &r2);
		r1.bottom -= r1.top;
		r1.right -= r1.left;
		r1.top = r2.top+80;
		r1.left = r2.left+40;
		::MoveWindow(hw, r1.left, r1.top, r1.right, r1.bottom, 1);
	}
}

int FASTCALL PPWait(int begin)
{
	int    ok = 1;
	if(begin != 1)
		DS.SetThreadNotification(PPSession::stntMessage, 0);
	if(DS.IsThreadInteractive()) {
		switch(begin) {
			case 0: __WD.Stop(); break;
			case 1: __WD.Start(); break;
			case 2: __WD.Hide(); break;
			case 3: __WD.Show(); break;
		}
	}
	return ok;
}

void PPWaitStart() // @v11.0.3 PPWait(1)
{
	if(DS.IsThreadInteractive())
		__WD.Start();
}

void PPWaitStop() // @v11.0.3 PPWait(0)
{
	DS.SetThreadNotification(PPSession::stntMessage, 0);
	if(DS.IsThreadInteractive())
		__WD.Stop();
}

void FASTCALL PPWaitMsg(const char * pMsg) { __WD.SetMessage(pMsg); }
void STDCALL  PPWaitPercent(long  p, long  t, const char * pMsg) { __WD.SetPercent(p, t, pMsg); }
void STDCALL  PPWaitPercent(ulong p, ulong t, const char * pMsg) { __WD.SetPercent(p, t, pMsg); }
void STDCALL  PPWaitPercent(uint  p, uint  t, const char * pMsg) { __WD.SetPercent(p, t, pMsg); }

void STDCALL  PPWaitPercent(int64 p, int64 t, const char * pMsg) 
{ 
	__WD.SetPercent(static_cast<ulong>(R0i((100.0 * static_cast<double>(p)) / static_cast<double>(t))), 100UL, pMsg); 
}

void FASTCALL PPWaitPercent(const IterCounter & cntr, const char * pMsg) { PPWaitPercent(cntr, cntr.GetTotal(), pMsg); }
void FASTCALL PPWaitPercent(ulong v, const char * pMsg) { PPWaitPercent(v, 100UL, pMsg); }

void STDCALL  PPWaitMsg(int msgGrpID, int msgID, const char * addInfo)
{
	SString fmt_buf;
	if(PPLoadString(msgGrpID, msgID, fmt_buf)) {
		if(addInfo) {
			SString msg_buf;
			PPWaitMsg(msg_buf.Printf(fmt_buf, addInfo));
		}
		else
			PPWaitMsg(fmt_buf);
	}
}

void FASTCALL PPWaitLong(long v)
{
	char b[32];
	PPWaitMsg(ltoa(v, b, 10));
}

void FASTCALL PPWaitDate(LDATE dt)
{
	char   b[32];
	PPWaitMsg(datefmt(&dt, DATF_DMY, b));
}

static int FASTCALL CheckEscKey(int cmd)
{
	MSG    msg;
	return PeekMessage(&msg, 0, WM_KEYDOWN, WM_KEYDOWN, cmd ? PM_NOREMOVE : PM_REMOVE) ? ((msg.wParam == VK_ESCAPE) ? 1 : 0) : 0;
}

int PPCheckUserBreak()
{
	int    ok = 1;
	PROFILE_START
	if(SLS.CheckStopFlag())
		ok = PPSetError(PPERR_PROCESSWASSTOPPED);
	else if(DS.IsThreadStopped())
		ok = PPSetError(PPERR_THREADWASSTOPPED);
	else if(DS.IsThreadInteractive()) {
		if(__WD.GetWindowHandle() && CheckEscKey(1)) {
			CheckEscKey(0);
			PPWaitStop();
			if(PPMessage(mfConf|mfYesNo, PPCFM_USERBREAK) == cmYes)
                ok = PPSetError(PPERR_USERBREAK);
			else
				ok = (PPWaitStart(), -1);
		}
	}
	PROFILE_END
	return ok;
}
