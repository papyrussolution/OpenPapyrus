// PPLOG.CPP
// Copyright (c) A.Sobolev, A.Osolotkin 1999, 2000, 2001, 2003, 2004, 2005, 2006, 2007, 2008, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020
// @codepage UTF-8
//
#include <pp.h>
#pragma hdrstop
#include <scintilla.h>
#include <scilexer.h>

class LogListBoxDef : public StdListBoxDef {
public:
	SLAPI  LogListBoxDef(SArray * pArray, uint aOptions, TYPEID t, TVMsgLog * pMl) : StdListBoxDef(pArray, aOptions, t), P_MsgLog(pMl)
	{
	}
	SLAPI ~LogListBoxDef()
	{
		ZDELETE(P_MsgLog);
	}
	virtual long SLAPI getRecsCount() { return P_MsgLog ? P_MsgLog->GetVisCount() : 0; }
	virtual void * FASTCALL getRow_(long r) { return P_MsgLog ? P_MsgLog->GetRow(r) : 0; }

	TVMsgLog * P_MsgLog; // private. Don't use !
};

#ifndef USE_LOGLISTWINDOWSCI

class LogListWindow : public TWindow {
public:
	LogListWindow(TRect &, LogListBoxDef *, const char *, int);
	~LogListWindow()
	{
		if(IsWindow(H()))
			DestroyWindow(H());
		def->P_MsgLog->P_LWnd = 0;
		ZDELETE(def);
		ZDeleteWinGdiObject(&hf);
	}
	void   Refresh(long);
	void   Append();
protected:
	DECL_HANDLE_EVENT;
	static BOOL CALLBACK LogListProc(HWND, UINT, WPARAM, LPARAM);
	SString & GetString(int pos, SString & rBuf, int oem = 0) const;

	LogListBoxDef * def;
	HFONT  hf;
	int    StopExec; // Признак остановки цикла исполнения //
	WNDPROC PrevLogListProc;
};

#endif // } USE_LOGLISTWINDOWSCI
//
// Новый вариант окна отображения сообщений (на платформе Scintilla)
//
class LogListWindowSCI : public TWindow, public SScEditorBase {
public:
	explicit LogListWindowSCI(TVMsgLog * pLog);
	~LogListWindowSCI()
	{
		::DestroyWindow(HwndSci);
		ZDELETE(P_MsgLog);
		delete P_Toolbar;
	}
	void   Append();
	void   Refresh(long item);
private:
	static int RegWindowClass(HINSTANCE hInst);
	static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK ScintillaWindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	virtual int ProcessCommand(uint ppvCmd, const void * pHdr, void * pBrw);
	int    WMHCreate();
	int    SLAPI LoadToolbar(uint tbId);
	HWND   GetSciWnd() const { return HwndSci; }
	int    Resize();

	static LPCTSTR WndClsName; // @global
	enum {
		sstLastKeyDownConsumed = 0x0001
	};
	long   SysState;
	HWND   HwndSci;
	TVMsgLog * P_MsgLog;
	TToolbar * P_Toolbar;
	long   ToolBarWidth;
	uint   ToolbarId;
	WNDPROC OrgScintillaWndProc;
};

// static
LPCTSTR LogListWindowSCI::WndClsName = _T("LogListWindowSCI"); // @global

// static
int LogListWindowSCI::RegWindowClass(HINSTANCE hInst)
{
	WNDCLASSEX wc;
	MEMSZERO(wc);
	wc.cbSize        = sizeof(wc);
	wc.style         = CS_HREDRAW|CS_VREDRAW|CS_OWNDC|CS_DBLCLKS;
	wc.lpfnWndProc   = LogListWindowSCI::WndProc;
	wc.cbClsExtra    = BRWCLASS_CEXTRA;
	wc.cbWndExtra    = BRWCLASS_WEXTRA;
	wc.hInstance     = hInst;
	wc.hIcon         = LoadIcon(hInst, MAKEINTRESOURCE(/*ICON_TIMEGRID*/172));
	wc.hbrBackground = ::CreateSolidBrush(RGB(0xEE, 0xEE, 0xEE));
	wc.lpszClassName = LogListWindowSCI::WndClsName;
#if !defined(_PPDLL) && !defined(_PPSERVER)
	Scintilla_RegisterClasses(hInst);
#endif
	return ::RegisterClassEx(&wc);
}

LogListWindowSCI::LogListWindowSCI(TVMsgLog * pLog) : TWindow(TRect(0, 0, 100, 20), "LOG WINDOW", 0), SScEditorBase()
{
	{
		static int is_cls_reg = 0;
		if(!is_cls_reg) {
			LogListWindowSCI::RegWindowClass(TProgram::GetInst());
			is_cls_reg = 1;
		}
	}
	P_MsgLog = pLog;
	HwndSci = 0;
	P_Toolbar = 0;
	ToolBarWidth = 0;
	OrgScintillaWndProc = 0;
	//
	SString temp_buf;
	RECT   parent, r;
	//StopExec = 0; // Признак остановки цикла исполнения //
	//PrevLogListProc = 0;
	(temp_buf = "LOG WINDOW").Transf(CTRANSF_INNER_TO_OUTER);
	APPL->GetClientRect(&parent);
	r.left   = parent.left;
	r.right  = parent.right;
	r.top    = (parent.bottom / 3) * 2 + parent.top;
	r.bottom = parent.bottom / 3;
	SendMessage(APPL->H_LogWnd, WM_SYSCOMMAND, SC_CLOSE, 0);
	APPL->H_LogWnd = HW = ::CreateWindowEx(WS_EX_TOOLWINDOW, LogListWindowSCI::WndClsName, SUcSwitch(temp_buf),
		WS_CHILD|WS_CLIPSIBLINGS|/*WS_VSCROLL|*/WS_CAPTION|WS_SYSMENU|WS_SIZEBOX|LBS_DISABLENOSCROLL|LBS_NOINTEGRALHEIGHT,
		r.left, r.top, r.right, r.bottom, APPL->H_MainWnd, 0, TProgram::GetInst(), this); // @unicodeproblem
	TView::SetWindowProp(H(), GWLP_USERDATA, this);
	//PrevLogListProc = (WNDPROC)TView::SetWindowProp(H(), GWLP_WNDPROC, LogListProc);
	//hf = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
	//::SendMessage(H(), WM_SETFONT, (long)hf, 0);
	::ShowWindow(H(), SW_SHOW);
	::UpdateWindow(H());
	::PostMessage(H(), WM_SIZE, 0, 0);
	::PostMessage(APPL->H_MainWnd, WM_SIZE, 0, 0);
	::SetWindowPos(H(), HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
}

void LogListWindowSCI::Refresh(long item)
{
	CallFunc(SCI_CLEARALL);
	int   line_no_to_select = 0;
	if(P_MsgLog) {
		SString temp_buf;
		for(long i = 0; i < P_MsgLog->GetVisCount(); i++) {
			const char * p_buf = static_cast<const char *>(P_MsgLog->GetRow(i));
			if(p_buf) {
				temp_buf = p_buf+sizeof(long);
				temp_buf.Strip().Transf(CTRANSF_INNER_TO_UTF8).CRB();
				CallFunc(SCI_APPENDTEXT, static_cast<int>(temp_buf.Len()), reinterpret_cast<int>(temp_buf.cptr()));
				line_no_to_select++;
			}
		}
	}
	CallFunc(SCI_GOTOLINE, line_no_to_select);
	//::UpdateWindow(H());
}

void LogListWindowSCI::Append()
{
	if(P_MsgLog) {
		long   vc = P_MsgLog->GetVisCount();
		if(vc > 0) {
			const char * p_buf = static_cast<const char *>(P_MsgLog->GetRow(vc-1));
			if(p_buf) {
				SString temp_buf = p_buf+sizeof(long);
				temp_buf.Strip().Transf(CTRANSF_INNER_TO_UTF8).CRB();
				CallFunc(SCI_SETREADONLY);
				CallFunc(SCI_APPENDTEXT, (int)temp_buf.Len(), (int)temp_buf.cptr());
				CallFunc(SCI_SETREADONLY, 1);
				CallFunc(SCI_GOTOLINE, vc);
			}
		}
	}
	::UpdateWindow(H());
}

int LogListWindowSCI::ProcessCommand(uint ppvCmd, const void * pHdr, void * pBrw)
{
	int    ok = -2;
	switch(ppvCmd) {
		case PPVCMD_SAVEAS:
			//ok = FileSave(0, ofInteractiveSaveAs);
			break;
		case PPVCMD_SEARCH:
			SearchAndReplace(srfUseDialog);
			break;
		case PPVCMD_SEARCHNEXT:
			SearchAndReplace(0);
			break;
		case PPVCMD_PRINT:
			if(P_MsgLog) {
				PView  pv(P_MsgLog);
				PPAlddPrint(REPORT_LOGLIST, &pv, 0);
			}
			break;
	}
	return ok;
}

int SLAPI LogListWindowSCI::LoadToolbar(uint tbId)
{
	int    r = 0;
	TVRez & rez = *P_SlRez;
	ToolbarList tb_list;
	r = rez.findResource(tbId, TV_EXPTOOLBAR, 0, 0) ? ImpLoadToolbar(rez, &tb_list) : 0;
	if(r > 0)
		setupToolbar(&tb_list);
	return r;
}

int LogListWindowSCI::WMHCreate()
{
	RECT   rc;
	uint   toolbar_id = TOOLBAR_LOGVIEW;
	GetWindowRect(HW, &rc);
	P_Toolbar = new TToolbar(HW, TBS_NOMOVE);
	if(P_Toolbar && LoadToolbar(toolbar_id) > 0) {
		P_Toolbar->Init(toolbar_id, &Toolbar);
		if(P_Toolbar->IsValid()) {
			RECT tbr;
			::GetWindowRect(P_Toolbar->H(), &tbr);
			ToolBarWidth = tbr.bottom - tbr.top;
		}
	}
	HwndSci = ::CreateWindowEx(WS_EX_CLIENTEDGE, _T("Scintilla"), _T(""), WS_CHILD|WS_VISIBLE|WS_TABSTOP|WS_CLIPCHILDREN,
		0, 0/*ToolBarWidth*/, rc.right - rc.left, rc.bottom - rc.top, HW, 0/*(HMENU)GuiID*/, APPL->GetInst(), 0);
	SScEditorBase::Init(HwndSci, 0/*preserveFileName*/);
	TView::SetWindowUserData(HwndSci, this);
	OrgScintillaWndProc = reinterpret_cast<WNDPROC>(::SetWindowLongPtr(HwndSci, GWLP_WNDPROC, reinterpret_cast<LPARAM>(ScintillaWindowProc)));
	{
		KeyAccel.clear();
		{
			KeyDownCommand k;
			k.SetTvKeyCode(kbF3);
			SetKeybAccelerator(k, PPVCMD_SEARCHNEXT);
		}
		{
			for(uint i = 0; i < OuterKeyAccel.getCount(); i++) {
				const LAssoc & r_accel_item = OuterKeyAccel.at(i);
				const KeyDownCommand & r_k = *reinterpret_cast<const KeyDownCommand *>(&r_accel_item.Key);
				KeyAccel.Set(r_k, r_accel_item.Val);
			}
		}
		if(P_Toolbar) {
			const uint tbc = P_Toolbar->getItemsCount();
			for(uint i = 0; i < tbc; i++) {
				const ToolbarItem & r_tbi = P_Toolbar->getItem(i);
				if(!(r_tbi.Flags & r_tbi.fHidden) && r_tbi.KeyCode && r_tbi.KeyCode != TV_MENUSEPARATOR && r_tbi.Cmd) {
					KeyDownCommand k;
					if(k.SetTvKeyCode(r_tbi.KeyCode))
						KeyAccel.Set(k, r_tbi.Cmd);
				}
			}
		}
		KeyAccel.Sort();
	}
	{
		Doc.SciDoc = reinterpret_cast<SScEditorBase::SciDocument>(CallFunc(SCI_CREATEDOCUMENT));
		//Setup scratchtilla for new filedata
		CallFunc(SCI_SETSTATUS, SC_STATUS_OK); // reset error status
		CallFunc(SCI_SETDOCPOINTER, 0, reinterpret_cast<intptr_t>(Doc.SciDoc));
		CallFunc(SCI_CLEARALL);
		CallFunc(SCI_ALLOCATE, (WPARAM)128*1024);
		int sci_status = CallFunc(SCI_GETSTATUS);
		CallFunc(SCI_SETREADONLY, 1);

		CallFunc(SCI_SETCODEPAGE, SC_CP_UTF8);
		CallFunc(SCI_SETEOLMODE, SC_EOL_CRLF);

		CallFunc(SCI_SETCARETLINEVISIBLE, 1);
		CallFunc(SCI_SETCARETLINEBACK, RGB(0x57,0xA8,0xFA));
		CallFunc(SCI_SETSELBACK, 1, RGB(117,217,117));
		CallFunc(SCI_SETFONTQUALITY, SC_EFF_QUALITY_ANTIALIASED);

		CallFunc(SCI_SETVSCROLLBAR, 1);
		CallFunc(SCI_SETHSCROLLBAR, 1);

		//CallFunc(SCI_SETYCARETPOLICY, CARET_STRICT);
	}
	return BIN(P_SciFn && P_SciPtr);
}

int LogListWindowSCI::Resize()
{
	if(HwndSci != 0) {
		RECT rc;
		GetWindowRect(H(), &rc);
		if(IsWindowVisible(APPL->H_ShortcutsWnd)) {
			RECT sh_rect;
			GetWindowRect(APPL->H_ShortcutsWnd, &sh_rect);
			rc.bottom -= sh_rect.bottom - sh_rect.top;
		}
		const int sb_width = 12;
		MoveWindow(HwndSci, 0, ToolBarWidth, rc.right - rc.left - sb_width, rc.bottom - rc.top - sb_width, 1);
	}
	return 1;
}

// static
LRESULT CALLBACK LogListWindowSCI::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	CREATESTRUCT * p_init_data;
	LogListWindowSCI * p_view = 0;
	switch(message) {
		case WM_CREATE:
			p_init_data = reinterpret_cast<CREATESTRUCT *>(lParam);
			if(TWindow::IsMDIClientWindow(p_init_data->hwndParent)) {
				p_view = reinterpret_cast<LogListWindowSCI *>((static_cast<LPMDICREATESTRUCT>(p_init_data->lpCreateParams))->lParam);
				//p_view->BbState |= bbsIsMDI;
			}
			else {
				p_view = static_cast<LogListWindowSCI *>(p_init_data->lpCreateParams);
				//p_view->BbState &= ~bbsIsMDI;
			}
			if(p_view) {
				p_view->HW = hWnd;
				TView::SetWindowProp(hWnd, GWLP_USERDATA, p_view);
				::SetFocus(hWnd);
				::SendMessage(hWnd, WM_NCACTIVATE, TRUE, 0L);
				p_view->WMHCreate();
				::PostMessage(hWnd, WM_PAINT, 0, 0);
				{
					SString temp_buf;
					TView::SGetWindowText(hWnd, temp_buf);
					APPL->AddItemToMenu(temp_buf, p_view);
				}
				::ShowWindow(p_view->HwndSci, SW_SHOW);
				::SetFocus(p_view->HwndSci);
				return 0;
			}
			else
				return -1;
		case WM_COMMAND:
			{
				p_view = static_cast<LogListWindowSCI *>(TView::GetWindowUserData(hWnd));
				if(p_view) {
					if(HIWORD(wParam) == 0) {
						if(p_view->KeyAccel.getCount()) {
							long   cmd = 0;
							KeyDownCommand k;
							k.SetTvKeyCode(LOWORD(wParam));
							if(p_view->KeyAccel.BSearch(*reinterpret_cast<const long *>(&k), &cmd, 0)) {
								p_view->ProcessCommand(cmd, 0, p_view);
							}
						}
					}
					/*
					if(LOWORD(wParam))
						p_view->ProcessCommand(LOWORD(wParam), 0, p_view);
					*/
				}
			}
			break;
		case WM_DESTROY:
			p_view = static_cast<LogListWindowSCI *>(TView::GetWindowUserData(hWnd));
			if(p_view) {
				p_view->CallFunc(SCI_SETREADONLY); // @v9.7.5
				p_view->CallFunc(SCI_CLEARALL); // @v9.7.5
				p_view->CallFunc(SCI_RELEASEDOCUMENT, 0, (int)p_view->Doc.SciDoc); // @v9.7.5
				SETIFZ(p_view->EndModalCmd, cmCancel);
				APPL->DelItemFromMenu(p_view);
				p_view->ResetOwnerCurrent();
				if(!p_view->IsInState(sfModal)) {
					APPL->P_DeskTop->remove(p_view);
					delete p_view;
					TView::SetWindowProp(hWnd, GWLP_USERDATA, reinterpret_cast<void *>(0));
				}
				if(!IsIconic(APPL->H_MainWnd))
					APPL->SizeMainWnd(hWnd);
				APPL->H_LogWnd = 0;
			}
			return 0;
		case WM_SETFOCUS:
			if(!(TView::GetWindowStyle(hWnd) & WS_CAPTION)) {
				SetWindowPos(hWnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE);
				APPL->NotifyFrame(0);
			}
			p_view = static_cast<LogListWindowSCI *>(TView::GetWindowUserData(hWnd));
			if(p_view) {
				::SetFocus(p_view->HwndSci);
				APPL->SelectTabItem(p_view);
				TView::messageBroadcast(p_view, cmReceivedFocus);
				p_view->select();
			}
			break;
		case WM_KILLFOCUS:
			if(!(TView::GetWindowStyle(hWnd) & WS_CAPTION))
				APPL->NotifyFrame(0);
			p_view = static_cast<LogListWindowSCI *>(TView::GetWindowUserData(hWnd));
			if(p_view) {
				TView::messageBroadcast(p_view, cmReleasedFocus);
				p_view->ResetOwnerCurrent();
			}
			break;
		case WM_KEYDOWN:
			if(wParam == VK_ESCAPE) {
				p_view = static_cast<LogListWindowSCI *>(TView::GetWindowUserData(hWnd));
				if(p_view) {
					p_view->endModal(cmCancel);
					return 0;
				}
			}
			else if(wParam == VK_TAB) {
				p_view = static_cast<LogListWindowSCI *>(TView::GetWindowUserData(hWnd));
				if(p_view && GetKeyState(VK_CONTROL) & 0x8000 && !p_view->IsInState(sfModal)) {
					SetFocus(GetNextBrowser(hWnd, (GetKeyState(VK_SHIFT) & 0x8000) ? 0 : 1));
					return 0;
				}
			}
			return 0;
		case WM_SIZE:
			p_view = static_cast<LogListWindowSCI *>(TView::GetWindowUserData(hWnd));
			if(lParam && p_view) {
				HWND   hw = 0;
				int    toolbar_height = 0;
				if(p_view->P_Toolbar) {
					hw = p_view->P_Toolbar->H();
					RECT tbr;
					::GetWindowRect(hw, &tbr);
					toolbar_height = tbr.bottom - tbr.top;
				}
				if(IsWindowVisible(hw)) {
					MoveWindow(hw, 0, 0, LOWORD(lParam), toolbar_height, 0);
					TView::messageCommand(p_view, cmResize);
				}
				p_view->Resize();
				if(!IsIconic(APPL->H_MainWnd))
					APPL->SizeMainWnd(hWnd);
			}
			break;
		case WM_NOTIFY:
			{
				LPNMHDR lpnmhdr = reinterpret_cast<LPNMHDR>(lParam);
				p_view = static_cast<LogListWindowSCI *>(TView::GetWindowUserData(hWnd));
				if(p_view && lpnmhdr->hwndFrom == p_view->GetSciWnd()) {
					switch(lpnmhdr->code) {
						case SCN_CHARADDED:
						case SCN_MODIFIED:
							p_view->Doc.SetState(STextBrowser::Document::stDirty, 1);
							break;
					}
				}
			}
			break;
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}

/*static*/LRESULT CALLBACK LogListWindowSCI::ScintillaWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	LogListWindowSCI * p_this = reinterpret_cast<LogListWindowSCI *>(::GetWindowLongPtr(hwnd, GWLP_USERDATA));
	if(p_this) {
		switch(msg) {
			case WM_DESTROY:
				{
					//SetWindowLongPtr(p_this->HwndSci, GWLP_WNDPROC, (LPARAM)p_this->OrgScintillaWndProc);
					//SetWindowLongPtr(p_this->HwndSci, GWLP_USERDATA, 0);
					//return ::CallWindowProc(p_this->OrgScintillaWndProc, hwnd, msg, wParam, lParam);
					return ::DefWindowProc(hwnd, msg, wParam, lParam);
				}
				break;
			case WM_CHAR:
				if(p_this->SysState & p_this->sstLastKeyDownConsumed)
					return ::DefWindowProc(hwnd, msg, wParam, lParam);
				else
					return ::CallWindowProc(p_this->OrgScintillaWndProc, hwnd, msg, wParam, lParam);
			case WM_SYSKEYDOWN:
			case WM_KEYDOWN:
				{
					p_this->SysState &= ~p_this->sstLastKeyDownConsumed;
					int    processed = 0;
					KeyDownCommand k;
					k.SetWinMsgCode(wParam);
					if(k.Code == VK_TAB && k.State & k.stateCtrl) {
						SendMessage(p_this->HW, WM_KEYDOWN, wParam, lParam);
						p_this->SysState |= p_this->sstLastKeyDownConsumed;
						processed = 1;
					}
					else if(p_this->KeyAccel.getCount()) {
						long   cmd = 0;
						if(p_this->KeyAccel.BSearch(*reinterpret_cast<const long *>(&k), &cmd, 0)) {
							p_this->SysState |= p_this->sstLastKeyDownConsumed;
							p_this->ProcessCommand(cmd, 0, p_this);
							processed = 1;
						}
					}
					return processed ? ::DefWindowProc(hwnd, msg, wParam, lParam) : ::CallWindowProc(p_this->OrgScintillaWndProc, hwnd, msg, wParam, lParam);
				}
				break;
			default:
				if(p_this && p_this->IsConsistent())
					return ::CallWindowProc(p_this->OrgScintillaWndProc, hwnd, msg, wParam, lParam);
				else
					return ::DefWindowProc(hwnd, msg, wParam, lParam);
		}
	}
	else
		return ::DefWindowProc(hwnd, msg, wParam, lParam);
};
//
// PPMsgLog
//
SLAPI PPMsgLog::PPMsgLog() : Valid(0), Stream(-1), InStream(-1), P_Index(0), NextStrOffset(0)
{
}

SLAPI PPMsgLog::~PPMsgLog()
{
	Destroy();
}

void SLAPI PPMsgLog::Destroy()
{
	if(Stream >= 0) {
		close(Stream);
		Stream = -1;
	}
	if(InStream >= 0) {
		close(InStream);
		InStream = -1;
	}
	SFile::Remove(FileName);
	SFile::Remove(InFileName);
	ZDELETE(P_Index);
}

int SLAPI PPMsgLog::ShowLogWnd(const char * pTitle)
{
	return -1;
}

long SLAPI PPMsgLog::ImplPutMsg(const char * pText, long flags)
{
	return 0;
}

long SLAPI PPMsgLog::GetCount() const { return AllCount; }
long SLAPI PPMsgLog::GetCurMsg() const { return CurMsg; }
long SLAPI PPMsgLog::GetVisCount() const { return SVectorBase::GetCount(P_Index); }

void * SLAPI PPMsgLog::GetRow(long r)
{
	int16  rr, hh;
	if(r >= GetVisCount() || r < 0)
		return 0;
	CurMsg = 0;
	EnumMessages(GetVisibleMessage(r), TmpText+sizeof(long), LF_BUFFSIZE-sizeof(long), &rr, &hh);
	*reinterpret_cast<long *>(TmpText+hh) = r;
	if((rr = (int16)sstrlen(TmpText+hh+4)) >= 256)
		TmpText[hh+259] = '\0';
	return (TmpText+hh);
}

PPLogIdx SLAPI PPMsgLog::GetLogIdx(long row)
{
	PPLogIdx li;
	li.address = 0;
	li.flags = 0;
	if(row >= 0 && row <= GetCount()) {
		lseek(InStream, row*sizeof(PPLogIdx), SEEK_SET);
		_read(InStream, &li, sizeof(PPLogIdx));
	}
	return li;
}

void SLAPI PPMsgLog::SetLogIdx(long row, const PPLogIdx * pLi)
{
	if(row >= 0 && row <= GetCount()) {
		lseek(InStream, row*sizeof(PPLogIdx), SEEK_SET);
		_write(InStream, pLi, sizeof(PPLogIdx));
	}
}

long SLAPI PPMsgLog::PutMessage(const char * pBody, long flags, const void * head, size_t hsize)
{
	long   rval = 0;
	if(Valid) {
		PPLogIdx st;
		SString body;
		body = pBody;
		body.Chomp();
		st.flags    = flags;
		st.address  = hsize + sizeof(int16) + body.Len();
		st.address += GetLogIdx(GetCount()).address;
		lseek(Stream, 0, SEEK_END);
		lseek(InStream, 0, SEEK_END);
		_write(InStream, &st, sizeof(PPLogIdx));
		_write(Stream, &hsize, sizeof(int16));
		_write(Stream, head, hsize);
		_write(Stream, body.cptr(), body.Len());
		AllCount++;
		rval = ImplPutMsg(body, flags);
	}
	return rval;
}

long SLAPI PPMsgLog::GetVisibleMessage(long nrow)
{
	return (Valid && nrow < GetVisCount()) ? *static_cast<const long *>(P_Index->at((uint)nrow)) : 0;
}

// static
int SLAPI PPMsgLog::RemoveTempFiles()
{
	int    ok = 0;
	SString src_path, src_file_name;
	PPGetPath(PPPATH_TEMP, src_path) || PPGetPath(PPPATH_OUT, src_path);
	src_path.SetLastSlash();
	(src_file_name = src_path).Cat("log?????.");
	SDirEntry sde;
	for(SDirec sd(src_file_name); sd.Next(&sde) > 0;)
		if(SFile::Remove((src_file_name = src_path).Cat(sde.FileName)) == 0)
			ok++;
	return ok;
}

long SLAPI PPMsgLog::Init()
{
	Valid = 0;
	Destroy();
	AllCount = 0;
	CurMsg   = 0;
	P_Index = new SArray(sizeof(long));
	if(!P_Index)
		return 0;
	{
		SString fname;
		PPMakeTempFileName("logl", 0, 0, FileName);
		PPMakeTempFileName("logi", 0, 0, InFileName);
	}
	Stream = creat(FileName, S_IWRITE);
	if(Stream < 0) {
		FileName = 0;
		ZDELETE(P_Index);
		return 0;
	}
	else {
		close(Stream);
		Stream = open(FileName, O_RDWR|O_BINARY);
	}
	InStream = creat(InFileName, S_IWRITE);
	if(InStream < 0) {
		InFileName = 0;
		close(Stream);
		Stream = -1;
		SFile::Remove(FileName);
		ZDELETE(P_Index);
		return 0;
	}
	else {
		close(InStream);
		InStream = open(InFileName, O_RDWR|O_BINARY);
	}
	PPLogIdx li;
	li.flags = 0;
	li.address = 0;
	_write(InStream, &li, sizeof(PPLogIdx));
	Valid = 1;
	return Valid;
}

void SLAPI PPMsgLog::DeleteVisibleMessage(long nrow)
{
	if(Valid && nrow < GetVisCount() && GetVisCount() || nrow >= 0) {
		const long row = GetVisibleMessage(nrow);
		PPLogIdx li = GetLogIdx(row);
		li.flags &= ~(LF_SHOW);
		SetLogIdx(row, &li);
		P_Index->atFree((uint)nrow);
	}
}

long SLAPI PPMsgLog::EnumMessages(long nmsg, void * buff, int16 bsize, int16 * rsize, int16 * hsize)
{
	if((!nmsg && CurMsg > GetCount()) || nmsg > GetCount())
		return 0;
	CurMsg = NZOR(nmsg, (CurMsg+1));
	if(CurMsg > AllCount)
		return 0;
	lseek(Stream, GetLogIdx(CurMsg-1).address, SEEK_SET);
	_read(Stream, hsize, sizeof(int16));
	int    len = (int)(GetLogIdx(CurMsg).address - GetLogIdx(CurMsg-1).address) - sizeof(int16);
	*rsize = _read(Stream, buff, len > bsize ? bsize : len); // exception was here!
	char * bb = static_cast<char *>(buff);
	bb[((*rsize >= bsize) ? (*rsize - 1) : *rsize)] = 0;
	return CurMsg;
}

int SLAPI PPMsgLog::SaveLogFile(const char * pFileName, long options)
{
	int    ok = 1;
	if(Valid) {
		long   rr;
		int16  r, h;
		SString path;
		{
			SPathStruc ps(pFileName);
			if(!ps.Dir.NotEmptyS() && !ps.Drv.NotEmptyS() && PPGetPath(PPPATH_LOG, path) > 0)
				path.SetLastSlash().Cat(pFileName);
			else
				path = pFileName;
		}
		CurMsg = 0;
		while((rr = EnumMessages(0l, TmpText, LF_BUFFSIZE, &r, &h)) != 0)
			if(!(GetLogIdx(rr).flags & LF_DONTWRITE))
				PPLogMessage(path, TmpText+h, options);
	}
	else
		ok = 0;
	return ok;
}

int SLAPI PPMsgLog::Print()
{
	PView  pv(this);
	return PPAlddPrint(REPORT_LOGLIST, &pv, 0);
}

int SLAPI PPMsgLog::InitIteration()
{
	int    ok = 1;
	if(Valid) {
		CurMsg = 0;
		NextStrOffset = 0;
	}
	else
		ok = 0;
	return ok;
}

int FASTCALL PPMsgLog::NextIteration(MsgLogItem * pItem)
{
	int    ok = 1;
	int16  r, h;
	int    max_str_len = LOGLIST_MAXSTRLEN;
	if(EnumMessages(NextStrOffset ? CurMsg : 0L, TmpText, LF_BUFFSIZE, &r, &h) > 0) {
		int    len = r - h - NextStrOffset;
		char * p_str = TmpText + h + NextStrOffset;
		char * p_next_str = 0;
		char   first_sym = 0;
		if(len >= max_str_len) {
			for(p_next_str = p_str + max_str_len; *p_next_str != ' ' && p_next_str > p_str; p_next_str--) {
				;
			}
			if(p_next_str == p_str)
				p_next_str = p_str + max_str_len - 1;
			first_sym = *p_next_str;
			*p_next_str = '\0';
			NextStrOffset += (int)(p_next_str - p_str) + (first_sym == ' ' ? 1 : 0);
		}
		else
			NextStrOffset = 0;
		STRNSCPY(pItem->LogListStr, p_str);
		if(NextStrOffset)
			*p_next_str = first_sym;
	}
	else
		ok = -1;
	return ok;
}
//
// TVMsgLog
//
SLAPI TVMsgLog::TVMsgLog() : PPMsgLog(), P_LWnd(0), HorzRange(0)
{
}

// static
void TVMsgLog::Delete_(TVMsgLog * pMsgLog, int winDestroy)
{
	if(pMsgLog && winDestroy)
		ZDELETE(pMsgLog);
}

int SLAPI TVMsgLog::ShowLogWnd(const char * pTitle)
{
	int    ok = 1;
	if(Valid) {
		if(!P_LWnd) {
			TRect rect(0, 15, 80, 23);
#ifdef USE_LOGLISTWINDOWSCI
			P_LWnd = new LogListWindowSCI(this); // @todo invalid size 512 (> 255)
			APPL->P_DeskTop->Insert_(P_LWnd);
			::ShowWindow(P_LWnd->HW, SW_SHOW);
			//P_LWnd->Refresh(GetVisCount());
#else
			P_LWnd = new LogListWindow(rect, new LogListBoxDef(P_Index, 0, (TYPEID)MKSTYPE(S_ZSTRING, 255), this), pTitle, 0); // @todo invalid size 512 (> 255)
			APPL->P_DeskTop->Insert_(P_LWnd);
			P_LWnd->Refresh(GetVisCount());
#endif
		}
	}
	else
		ok = 0;
	return ok;
}

SLAPI TVMsgLog::~TVMsgLog()
{
#ifdef USE_LOGLISTWINDOWSCI
#else
	delete P_LWnd;
#endif
}

long SLAPI TVMsgLog::ImplPutMsg(const char * pText, long flags)
{
	if(flags & LF_SHOW) {
		if(GetVisCount() >= LF_MAXMSG)
			P_Index->atFree(0);
		while(!P_Index->insert(&AllCount))
			P_Index->atFree(0);
		const long max_horz_range = 256;
		const long tlen = sstrleni(pText);
		if(HorzRange < tlen)
			HorzRange = (tlen > max_horz_range) ? max_horz_range : tlen;
		RefreshList();
	}
	return 1;
}

void SLAPI TVMsgLog::RefreshList()
{
#ifdef USE_LOGLISTWINDOWSCI
	CALLPTRMEMB(P_LWnd, Append());
#else
	CALLPTRMEMB(P_LWnd, Append());
#endif
}
//
//
//
SLAPI PPEmbeddedLogger::PPEmbeddedLogger(long ctrflags, PPLogger * pOuterLogger, uint fileNameId, uint defLogOptions) : 
	ElState(0), LogFileNameId(fileNameId), DefLogOptions(defLogOptions), P_Logger(0)
{
	if(pOuterLogger) {
		P_Logger = pOuterLogger;
		ElState |= elstOuterLogger;
	}
	else if(ctrflags & ctrfDirectLogging) {
		ElState |= elstDirectLogging;
	}
	else {
		P_Logger = new PPLogger();
	}
}

SLAPI PPEmbeddedLogger::~PPEmbeddedLogger()
{
	if(!(ElState & elstOuterLogger)) {
		ZDELETE(P_Logger);
	}
}

void FASTCALL PPEmbeddedLogger::Log(const SString & rMsg)
{
	if(P_Logger)
		P_Logger->Log(rMsg);
	else if(ElState & elstDirectLogging) {
		if(LogFileNameId) {
			PPLogMessage(LogFileNameId, rMsg, DefLogOptions);
		}
	}
}

void SLAPI PPEmbeddedLogger::LogTextWithAddendum(int msgCode, const SString & rAddendum)
{
	if(msgCode) {
		SString fmt_buf, msg_buf;
		Log(msg_buf.Printf(PPLoadTextS(msgCode, fmt_buf), rAddendum.cptr()));
	}
}

void SLAPI PPEmbeddedLogger::LogLastError()
{
	if(P_Logger)
		P_Logger->LogLastError();
	else if(ElState & elstDirectLogging) {
		if(LogFileNameId)
			PPLogMessage(LogFileNameId, 0, LOGMSGF_LASTERR|DefLogOptions);
	}
}

int SLAPI PPEmbeddedLogger::Save(uint fileNameId, long options)
{
	int    ok = -1;
	if(P_Logger) {
		SETIFZ(fileNameId, LogFileNameId);
		if(fileNameId)
			ok = P_Logger->Save(fileNameId, options);
	}
	return ok;
}
//
//
//
SLAPI PPLogger::PPLogger() : Flags(0), P_Log(0)
{
}

SLAPI PPLogger::PPLogger(long flags) : Flags(flags), P_Log(0)
{
}

SLAPI PPLogger::~PPLogger()
{
	TVMsgLog::Delete_(static_cast<TVMsgLog *>(P_Log), BIN(CS_SERVER));
}

void SLAPI PPLogger::Clear()
{
	TVMsgLog::Delete_(static_cast<TVMsgLog *>(P_Log));
}

int FASTCALL PPLogger::Log(const char * pMsg)
{
	int    ok = 1;
	if(!(Flags & fDisableOutput)) {
		SString buf = pMsg;
		if(!P_Log) {
			THROW_MEM(P_Log = new TVMsgLog);
			P_Log->Init();
			if(!(Flags & fDisableWindow) && !CS_SERVER) // @v10.6.8 !(Flags & fDisableWindow)
				P_Log->ShowLogWnd();
		}
		P_Log->PutMessage(buf.Chomp(), LF_SHOW);
	}
	CATCHZOK
	return ok;
}

int SLAPI PPLogger::LogMsgCode(uint msgOptions, uint msgId, const char * pAddedInfo)
{
	int    ok = -1;
	SString buf;
	if(PPGetMessage(msgOptions, msgId, pAddedInfo, 1, buf) > 0)
		ok = Log(buf);
	return ok;
}

int SLAPI PPLogger::LogSubString(uint strId, int idx)
{
	int    ok = 0;
	SString buf;
	if(PPGetSubStr(strId, idx, buf) > 0)
		ok = Log(buf);
	return ok;
}

int SLAPI PPLogger::LogString(uint strId, const char * pAddedInfo)
{
	int    ok = 0;
	SString fmt_buf, msg_buf;
	if(PPLoadText(strId, fmt_buf))
		ok = pAddedInfo ? Log(msg_buf.Printf(fmt_buf, pAddedInfo)) : Log(fmt_buf);
	return ok;
}

int SLAPI PPLogger::LogAcceptMsg(PPID objType, PPID objID, int upd)
{
	SString log_msg;
	return Log(PPObject::GetAcceptMsg(objType, objID, upd, log_msg));
}

int SLAPI PPLogger::LogLastError()
{
	int    ok = -1;
	SString buf;
	if(PPGetLastErrorMessage(1, buf))
		ok = Log(buf);
	return ok;
}

int SLAPI PPLogger::Save(uint fileId, long options)
{
	int    ok = 0;
	if(P_Log && fileId) {
		SString file_name;
		if(PPGetFileName(fileId, file_name).NotEmptyS())
			if(P_Log->GetCount()) {
				const long f = NZOR(options, LOGMSGF_TIME|LOGMSGF_USER);
				P_Log->SaveLogFile(file_name, f);
				ok = 1;
			}
			else
				ok = -1;
	}
	return ok;
}

int SLAPI PPLogger::Save(const char * pFileName, long options)
{
	if(P_Log && !isempty(pFileName) && P_Log->GetCount()) {
		const long f = NZOR(options, LOGMSGF_TIME|LOGMSGF_USER);
		P_Log->SaveLogFile(pFileName, f);
	}
	return 1;
}

#ifndef USE_LOGLISTWINDOWSCI

// static
BOOL CALLBACK LogListWindow::LogListProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LogListWindow * p_data = (LogListWindow *)TView::GetWindowUserData(hWnd);
	switch(uMsg) {
		case WM_SIZE:
			if(!IsIconic(APPL->H_MainWnd))
				APPL->SizeMainWnd(hWnd);
			break;
		case WM_GETMINMAXINFO:
			if(!IsIconic(APPL->H_MainWnd)) {
				LPMINMAXINFO p_min_max = (LPMINMAXINFO) lParam;
				RECT rc_client;
				APPL->GetClientRect(&rc_client);
				p_min_max->ptMinTrackSize.y = 40;
				p_min_max->ptMaxTrackSize.y = rc_client.bottom / 2;
				return 0;
			}
			else
				break;
		case WM_SHOWWINDOW:
			PostMessage(APPL->H_MainWnd, WM_SIZE, 0, 0);
			break;
		case WM_NCLBUTTONDOWN:
			if(wParam != HTTOP)
				SetCapture(hWnd);
			break;
		case WM_LBUTTONUP:
			if(hWnd == GetCapture())
				ReleaseCapture();
			break;
		case WM_SYSCOMMAND:
			if(wParam != SC_CLOSE)
				break;
			else {
				::DestroyWindow(hWnd);
				ZDELETE(p_data);
				return 0;
			}
		case WM_DESTROY:
			p_data->StopExec = 1;
			TView::SetWindowProp(hWnd, GWLP_WNDPROC, p_data->PrevLogListProc);
			break;
		case WM_KEYDOWN:
			if(wParam == VK_ESCAPE) {
				::DestroyWindow(hWnd);
				return 0;
			}
			else if(wParam == VK_F7)
				p_data->def->P_MsgLog->Print();
			break;
	}
	return CallWindowProc(p_data->PrevLogListProc, hWnd, uMsg, wParam, lParam);
}

LogListWindow::LogListWindow(TRect & rct, LogListBoxDef * aDef, const char * pTitle, int aNum) : TWindow(rct, pTitle, aNum)
{
	def = aDef;

	SString temp_buf;
	RECT   parent, r;
	StopExec = 0; // Признак остановки цикла исполнения //
	PrevLogListProc = 0;
	(temp_buf = pTitle).Transf(CTRANSF_INNER_TO_OUTER);
	APPL->GetClientRect(&parent);
	r.left   = parent.left;
	r.right  = parent.right;
	r.top    = (parent.bottom / 3) * 2 + parent.top;
	r.bottom = parent.bottom / 3;
	SendMessage(APPL->H_LogWnd, WM_SYSCOMMAND, SC_CLOSE, 0);
	APPL->H_LogWnd = HW = ::CreateWindowEx(WS_EX_TOOLWINDOW, _T("LISTBOX"), temp_buf,
		WS_CHILD|WS_CLIPSIBLINGS|WS_VSCROLL|WS_CAPTION|WS_SYSMENU|WS_THICKFRAME|LBS_DISABLENOSCROLL|LBS_NOINTEGRALHEIGHT,
		r.left, r.top, r.right, r.bottom, APPL->H_MainWnd, 0, TProgram::GetInst(), 0);
	TView::SetWindowProp(H(), GWLP_USERDATA, this);
	PrevLogListProc = (WNDPROC)TView::SetWindowProp(H(), GWLP_WNDPROC, LogListProc);
	hf = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
	::SendMessage(H(), WM_SETFONT, (WPARAM)hf, 0);
	::ShowWindow(H(), SW_SHOW);
	::UpdateWindow(H());
	::PostMessage(H(), WM_SIZE, 0, 0);
	::PostMessage(APPL->H_MainWnd, WM_SIZE, 0, 0);
	::SetWindowPos(H(), HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
}

SString & LogListWindow::GetString(int pos, SString & rBuf, int oem) const
{
	if(pos < def->getRecsCount()) {
		rBuf = (const char *)def->getRow_(pos)+sizeof(long);
		if(!oem)
			rBuf.Transf(CTRANSF_INNER_TO_OUTER);
	}
	else
		rBuf.Space() = 0; // Чтобы быть уверенным в том, что буфер не будет нулевым
	return rBuf;
}

void LogListWindow::Refresh(long item)
{
	SString buf;
	for(int i = 0; i < def->getRecsCount(); i++)
		::SendMessage(H(), LB_ADDSTRING, 0, (LPARAM)GetString(i, buf).cptr());
	::SendMessage(H(), LB_SETCARETINDEX, item-1, 0);
	::UpdateWindow(H());
}

void LogListWindow::Append()
{
	SString buf;
	int    i = def->getRecsCount();
	if(i)
		::SendMessage(H(), LB_ADDSTRING, 0, (LPARAM)GetString(i-1, buf).cptr());
	::SendMessage(H(), LB_SETCARETINDEX, i-1, 0);
	::UpdateWindow(H());
}

IMPL_HANDLE_EVENT(LogListWindow)
{
	if(event.isCmd(cmExecute)) {
		::ShowWindow(H(), SW_SHOW);
		::UpdateWindow(H());
		::PostMessage(H(), WM_SIZE, 0, 0);
		::PostMessage(APPL->H_MainWnd, WM_SIZE, 0, 0);
		if(APPL->PushModalWindow(this, H())) {
			::UpdateWindow(PrevInStack);
			::EnableWindow(PrevInStack, 0);
			APPL->MsgLoop(this, StopExec);
			APPL->PopModalWindow(this, 0);
		}
		clearEvent(event);
		event.message.infoLong = cmCancel;
	}
	else
		TWindow::handleEvent(event);
}

#endif // } USE_LOGLISTWINDOWSCI
//
//
//
void SLAPI PPLogMsgItem::Clear()
{
	Options = 0;
	FileName.Z();
	DupFileName.Z();
	Text.Z();
	Prefix.Z();
}

SLAPI PPLogMsgQueue::Stat::Stat()
{
	THISZERO();
}

SLAPI PPLogMsgQueue::PPLogMsgQueue() : Q(sizeof(PPLogMsgQueue::InnerItem), 1024*1024), NonEmptyEv(Evnt::modeCreateAutoReset)
{
}

SLAPI PPLogMsgQueue::~PPLogMsgQueue()
{
}

void FASTCALL PPLogMsgQueue::GetStat(PPLogMsgQueue::Stat & rS)
{
    L.Lock();
    rS = S;
    L.Unlock();
}

int FASTCALL PPLogMsgQueue::Push(const PPLogMsgItem & rItem)
{
	int    ok = 1;
	const  uint prev_count = Q.getNumItems();
    L.Lock();
	InnerItem new_item;
	MEMSZERO(new_item);
	new_item.Options = rItem.Options;
	THROW_SL(AddS(rItem.FileName, &new_item.FileNameP));
	THROW_SL(AddS(rItem.DupFileName, &new_item.DupFileNameP));
	THROW_SL(AddS(rItem.Text, &new_item.TextP));
	THROW_SL(AddS(rItem.Prefix, &new_item.PrefixP));
	THROW_SL(Q.push(&new_item));
	{
		S.PushCount++;
		if((prev_count+1) > S.MaxLenght)
			S.MaxLenght = prev_count+1;
		const size_t pool_size = Pool.getDataLen();
		if(pool_size > S.MaxStrPoolSize)
			S.MaxStrPoolSize = pool_size;
		if(prev_count == 0) {
			S.NonEmptyEvCount++;
			NonEmptyEv.Signal();
		}
	}
	CATCHZOK
    L.Unlock();
	return ok;
}

int FASTCALL PPLogMsgQueue::Pop(PPLogMsgItem & rItem)
{
	int    ok = -1;
    L.Lock();
    InnerItem * p_item = static_cast<InnerItem *>(Q.pop());
    if(p_item) {
        rItem.Options = p_item->Options;
        GetS(p_item->FileNameP, rItem.FileName);
        GetS(p_item->DupFileNameP, rItem.DupFileName);
        GetS(p_item->TextP, rItem.Text);
        GetS(p_item->PrefixP, rItem.Prefix);
		if(Q.getNumItems() == 0) {
			Q.clear();
			DestroyS(); // @v9.0.0 ClearS-->DestroyS
		}
        S.PopCount++;
        ok = 1;
    }
    L.Unlock();
	return ok;
}

SLAPI PPLogMsgSession::Stat::Stat() : PPLogMsgQueue::Stat(), MaxSingleOutputCount(0), OutputCount(0), FalseNonEmptyEvSwitchCount(0)
{
}

SLAPI PPLogMsgSession::PPLogMsgSession(PPLogMsgQueue * pQueue) : PPThread(PPThread::kLogger, "Logger Thread", pQueue), P_Queue(pQueue)
{
}

/*virtual*/void PPLogMsgSession::Run()
{
	PPLogMsgItem msg_item;
	SString diag_msg_buf;
	if(P_Queue) {
		PPSession::LoggerIntermediateBlock lb(DS);
		Evnt   stop_event(SLS.GetStopEventName(lb.TempBuf), Evnt::modeOpen);
		for(int stop = 0; !stop;) {
			uint   h_count = 0;
			HANDLE h_list[32];
			h_list[h_count++] = P_Queue->NonEmptyEv;
			h_list[h_count++] = stop_event;
			uint   r = ::WaitForMultipleObjects(h_count, h_list, 0, INFINITE);
			int    do_check_queue = 0;
			if(r == WAIT_OBJECT_0 + 0) { // NonEmptyEv
				do_check_queue = 1;
			}
			else if(r == WAIT_OBJECT_0 + 1) { // stop event // @v9.1.12 @fix (+2)-->(+1)
				stop = 1; // quit loop
				do_check_queue = 1; // @v9.1.12 Перед завершением сбросим все, что есть в очереди
			}
			else if(r == WAIT_FAILED) {
				// error
			}
			if(do_check_queue) {
                uint32 single_ev_count = 0;
				while(P_Queue->Pop(msg_item) > 0) {
					single_ev_count++;
					if(PPSession::Helper_Log(msg_item, lb) > 0) {
						S.OutputCount++;
					}
				}
				if(single_ev_count) {
					if(single_ev_count > S.MaxSingleOutputCount)
						S.MaxSingleOutputCount = single_ev_count;
				}
				else {
					S.FalseNonEmptyEvSwitchCount++;
				}
				if(!stop) {
					/*
						uint32  PushCount;       // Количество запросов Push
						uint32  PopCount;        // Количество запросов Pop
						uint32  MaxLenght;       // Максимальное количество сообщений в очереди
						size_t  MaxStrPoolSize;  // Максимальный объем пула строк
						uint32  NonEmptyEvCount; // Количество установок события NonEmptyEv

						uint32  MaxSingleOutputCount;       // Максимальное количество сообщений в очереди обработанное по одному событию PPLogMsgQueue::NonEmptyEv
						uint32  OutputCount;                // Количество выведенных сообщений
						uint32  FalseNonEmptyEvSwitchCount; // Количество срабатываний события PPLogMsgQueue::NonEmptyEv при которых очередь была пуста
					*/
					P_Queue->GetStat(S);
					diag_msg_buf.Z().Cat(getcurdatetime_(), DATF_ISO8601|DATF_CENTURY, 0).Space().CatEq("push", S.PushCount).Space().CatEq("output", S.OutputCount).Space().
                        CatEq("max count", S.MaxLenght).Space().CatEq("max size", S.MaxStrPoolSize).Space().
                        CatEq("false switch count", S.FalseNonEmptyEvSwitchCount);
					DS.SetThreadNotification(PPSession::stntMessage, diag_msg_buf);
				}
			}
		}
	}
	/*
	// @v10.4.0 {
	diag_msg_buf.Z().Cat("PPLogMsgSession is out");
	if(P_Queue) {
		P_Queue->GetStat(S);
		diag_msg_buf.Space().Cat(getcurdatetime_(), DATF_ISO8601, 0).Space().CatEq("push", S.PushCount).Space().CatEq("output", S.OutputCount).Space().
			CatEq("max count", S.MaxLenght).Space().CatEq("max size", S.MaxStrPoolSize).Space().
			CatEq("false switch count", S.FalseNonEmptyEvSwitchCount);
	}
	DS.SetThreadNotification(PPSession::stntMessage, diag_msg_buf);
	// } @v10.4.0 
	*/
}

/*static*/int SLAPI PPSession::Helper_Log(PPLogMsgItem & rMsgItem, PPSession::LoggerIntermediateBlock & rLb)
{
	const long max_file_size = (rMsgItem.Options & LOGMSGF_UNLIMITSIZE) ? 0 : rLb.CfgMaxFileSize;
	int   ok = 1;
	long  current_size = 0;
	FILE * f = fopen(rMsgItem.FileName, "r");
	if(f) {
		if(max_file_size > 0) {
			fseek(f, 0, SEEK_END);
			current_size = ftell(f);
		}
		SFile::ZClose(&f);
	}
	else {
		f = fopen(rMsgItem.FileName, "w");
		if(f)
			fclose(f);
		else
			ok = 0;
	}
	if(ok) {
		const int added_size = rMsgItem.Prefix.Len() + rMsgItem.Text.Len();
		if((max_file_size > 0) && (current_size + added_size) >= max_file_size*1024) {
			int    num_dig = 3;
			long   counter = 0;
			rLb.NewFileName.Z();
			rLb.TempBuf.Z(); // Используется для расширения файла
			do {
				if(counter >= (((int)fpow10i(num_dig))-1)) {
					num_dig++;
				}
				SPathStruc::ReplaceExt(rLb.NewFileName = rMsgItem.FileName, rLb.TempBuf.Z().CatLongZ(++counter, num_dig), 1);
			} while(fileExists(rLb.NewFileName));
			SFile::Rename(rMsgItem.FileName, rLb.NewFileName);
		}
		int    timeout = 30;
		do {
			f = fopen(rMsgItem.FileName, "a+");
			if(!f) {
				Sleep(10);
			}
			timeout--;
		} while(!f && timeout);
		if(f) {
			rLb.TempBuf.Z().Cat(rMsgItem.Prefix).Cat(rMsgItem.Text).CR().Transf(CTRANSF_INNER_TO_OUTER);
			fputs(rLb.TempBuf, f);
			SFile::ZClose(&f);
			if(rMsgItem.DupFileName.NotEmpty()) {
				//
				// Запись в дублирующий файл
				//
				f = fopen(rMsgItem.DupFileName, "a+");
				if(f) {
					fputs(rLb.TempBuf, f);
					SFile::ZClose(&f);
				}
			}
		}
	}
	return ok;
}

int SLAPI PPSession::Log(const char * pFileName, const char * pStr, long options)
{
	int    ok = 1;
	PPLogMsgItem item;
	item.Clear();
	item.Options = options;
	item.FileName = pFileName;
	SString temp_buf;
	if(pStr == 0 && options & LOGMSGF_LASTERR) {
		PPGetLastErrorMessage(1, item.Text);
	}
	else
		item.Text = pStr;
	if(item.FileName.NotEmptyS() || PPGetFilePath(PPPATH_LOG, "pp.log", item.FileName)) {
		if(options & LOGMSGF_TIME)
			item.Prefix.Cat(getcurdatetime_()).Tab();
		if(options & LOGMSGF_DBINFO) {
			DbProvider * p_dict = CurDict;
			if(p_dict)
				p_dict->GetDbSymb(temp_buf.Z());
			else
				temp_buf = "nologin";
			item.Prefix.Cat(temp_buf).Tab();
		}
		if(options & LOGMSGF_USER) {
			item.Prefix.Cat(GetCurUserName(temp_buf.Z())).Tab();
		}
		if(options & LOGMSGF_COMP) {
			if(!SGetComputerName(temp_buf.Z()))
				temp_buf = "?COMP?";
			item.Prefix.Cat(temp_buf).Tab();
		}
		if(options & LOGMSGF_THREADID) {
			item.Prefix.Cat(GetConstTLA().GetThreadID()).Tab();
		}
		if(options & LOGMSGF_SLSSESSGUID) { // @v10.5.7
			item.Prefix.Cat(SLS.GetSessUuid(), S_GUID::fmtIDL).Tab();
		}
		if(options & LOGMSGF_THREADINFO) {
			SetThreadNotification(PPSession::stntMessage, item.Text);
		}
		if(!(options & LOGMSGF_NODUPFORJOB))
			item.DupFileName = GetConstTLA().TempLogFile;
		if(P_LogQueue && !(options & LOGMSGF_DIRECTOUTP)) {
			ok = PushLogMsgToQueue(item);
		}
		else {
			LoggerIntermediateBlock lb(*this);
			ENTER_CRITICAL_SECTION
			ok = Helper_Log(item, lb);
			LEAVE_CRITICAL_SECTION
		}
	}
	else
		ok = 0;
	return ok;
}

int FASTCALL PPLogMessage(const char * pFileName, const char * pStr, long options)
{
	return DS.Log(pFileName, pStr, options);
}

int FASTCALL PPLogMessage(uint fileId, const char * pStr, long options)
{
	SString & r_file_name = SLS.AcquireRvlStr(); // @v9.9.10
	return PPGetFilePath(PPPATH_LOG, fileId, r_file_name) ? PPLogMessage(r_file_name, pStr, options) : 0;
}

int FASTCALL PPLogMessageList(uint fileId, const SStrCollection & rList, long options)
{
	int    ok = 0;
	const  uint c = rList.getCount();
	if(c) {
		SString file_name;
		if(PPGetFilePath(PPPATH_LOG, fileId, file_name)) {
			for(uint i = 0; i < c; i++)
				PPLogMessage(file_name, rList.at(i), options);
			ok = 1;
		}
	}
	else
		ok = -1;
	return ok;
}

int FASTCALL PPLogMessage(uint fileId, uint strGroup, uint strId, long options)
{
	SString & r_msg_buf = SLS.AcquireRvlStr(); // @v9.9.10
	PPLoadString(strGroup, strId, r_msg_buf);
	return PPLogMessage(fileId, r_msg_buf, options);
}
//
// Implementation of PPALDD_LogList
//
PPALDD_CONSTRUCTOR(LogList)
{
	if(Valid) {
		AssignHeadData(&H, sizeof(H));
		AssignDefIterData(&I, sizeof(I));
	}
}

PPALDD_DESTRUCTOR(LogList) { Destroy(); }

int PPALDD_LogList::InitData(PPFilt & rFilt, long rsrv)
{
	PPMsgLog * p_ml = 0;
	if(rsrv) {
		Extra[1].Ptr = p_ml = static_cast<PPMsgLog *>(rFilt.Ptr);
	}
	else {
		Extra[0].Ptr = p_ml = new PPMsgLog;
		p_ml->Init();
	}
	return DlRtm::InitData(rFilt, rsrv);
}

int PPALDD_LogList::InitIteration(PPIterID iterId, int sortId, long /*rsrv*/)
{
	PPMsgLog * p_ml = static_cast<PPMsgLog *>(NZOR(Extra[1].Ptr, Extra[0].Ptr));
	IterProlog(iterId, 1);
	if(sortId >= 0)
		SortIdx = sortId;
	return BIN(p_ml->InitIteration());
}

int PPALDD_LogList::NextIteration(PPIterID iterId)
{
	int    ok = -1;
	IterProlog(iterId, 0);
	PPMsgLog * p_ml = static_cast<PPMsgLog *>(NZOR(Extra[1].Ptr, Extra[0].Ptr));
	MsgLogItem item;
	if(p_ml->NextIteration(&item) > 0) {
		STRNSCPY(I.LogListStr, item.LogListStr);
		ok = DlRtm::NextIteration(iterId);
	}
	return ok;
}

void PPALDD_LogList::Destroy()
{
	delete static_cast<PPMsgLog *>(Extra[0].Ptr);
	Extra[0].Ptr = Extra[1].Ptr = 0;
}
//
//
//
int SLAPI TestLogWindow()
{
	const uint max_msg_count = 1000;
	PPLogger logger;
	SString msg_buf;
	SString temp_buf;
	for(uint i = 0; i < max_msg_count; i++) {
        PPLoadText((i & 1) ? PPTXT_TESTLOG_TEXT1 : PPTXT_TESTLOG_TEXT2, temp_buf);
        msg_buf.Z().Cat(i+1).CatDiv(':', 2).Cat(temp_buf);
        logger.Log(msg_buf);
	}
    return 1;
}
