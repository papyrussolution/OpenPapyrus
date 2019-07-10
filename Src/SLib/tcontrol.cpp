// TCONTROL.CPP
// Copyright (c) A.Sobolev 2011, 2012, 2013, 2014, 2016, 2017, 2018, 2019
//
#include <slib.h>
#include <tv.h>
#pragma hdrstop
//
// TStaticText
//
TStaticText::TStaticText(const TRect & bounds, const char * pText) : TView(bounds)
{
	SubSign = TV_SUBSIGN_STATIC;
	if(pText && *pText == 3)
		pText++;
	Text = pText;
}

int TStaticText::handleWindowsMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if(uMsg == WM_INITDIALOG) {
		SetupText(&Text);
		return 1;
	}
	else
		return 0;
}

SString & TStaticText::getText(SString & rBuf) const
{
	// @v9.1.5 char   temp_buf[1024];
	// @v9.1.5 SendDlgItemMessage(Parent, Id, WM_GETTEXT, sizeof(temp_buf), (long)temp_buf);
	// @v9.1.5 (rBuf = temp_buf).Transf(CTRANSF_OUTER_TO_INNER);
	// @v9.1.5 return rBuf;
	TView::SGetWindowText(GetDlgItem(Parent, Id), rBuf); // @v9.1.5
	return rBuf.Transf(CTRANSF_OUTER_TO_INNER); // @v9.1.5
}

int TStaticText::setText(const char * s)
{
	int    ok = -1;
	Text = s;
	Text.ShiftLeftChr('\003');
	if(Parent) {
		/* @v9.1.5
		char   temp_buf[1024];
		size_t p;
		const char * t = Text.SearchChar('~', &p);
		if(t && t[1] && t[2] == '~') {
			memcpy(temp_buf, (const char *)Text, p);
			temp_buf[p] = '&';
			temp_buf[p+1] = t[1];
			strnzcpy(temp_buf+p+2, t+3, sizeof(temp_buf)-p-2);
		}
		else
			Text.CopyTo(temp_buf, sizeof(temp_buf));
		// @v9.1.5 SOemToChar(temp_buf);
		// @v9.1.5 ::SendDlgItemMessage(Parent, Id, WM_SETTEXT, 0, (LPARAM)temp_buf);
		*/
		// @v9.1.5 {
		//
		// Замещаем конструкцию '~c~' на '$c'
		//
		SString temp_buf = Text;
		size_t _p = 0;
		while(temp_buf.SearchChar('~', &_p) && temp_buf.C(_p+1) && temp_buf.C(_p+2) == '~') {
			char new_item[4];
			new_item[0] = '&';
			new_item[1] = temp_buf.C(_p+1);
			new_item[2] = 0;
			temp_buf.Excise(_p, 3);
			temp_buf.Insert(_p, new_item);
		}
		temp_buf.Transf(CTRANSF_INNER_TO_OUTER);
		TView::SSetWindowText(GetDlgItem(Parent, Id), temp_buf);
		// } @v9.1.5
		ok = 1;
	}
	return ok;
}
//
// TLabel
//
TLabel::TLabel(const TRect & bounds, const char *aText, TView * aLink) : TStaticText(bounds, aText), link(aLink)
{
	SubSign = TV_SUBSIGN_LABEL;
	ViewOptions |= (ofPreProcess|ofPostProcess);
}

IMPL_HANDLE_EVENT(TLabel)
{
	TStaticText::handleEvent(event);
	if(TVBROADCAST)
		if(TVCMD == cmSearchLabel && link && TVINFOVIEW == link)
			clearEvent(event);
}
//
// TButton
//
static BOOL CALLBACK ButtonDialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	TButton * p_view = static_cast<TButton *>(TView::GetWindowUserData(hWnd));
	switch(uMsg) {
		case WM_DESTROY:
			p_view->OnDestroy(hWnd);
			return 0;
		case WM_SYSKEYDOWN:
		case WM_KEYDOWN:
			if((wParam >= VK_F1 && wParam <= VK_F12) || wParam == VK_ESCAPE || (wParam == VK_RETURN && (0x8000 & GetKeyState(VK_CONTROL))))  {
				p_view->SendToParent(hWnd, WM_VKEYTOITEM, MAKELPARAM((WORD)wParam, 0), reinterpret_cast<LPARAM>(hWnd));
				return 0;
			}
			else {
				if(!(GetKeyState(VK_CONTROL) & 0x8000) && !(GetKeyState(VK_MENU) & 0x8000)) {
					WORD   buf[12];
					BYTE   kb_state[256];
					GetKeyboardState(kb_state);
					ToAscii(MAKEWPARAM((WORD)wParam, 0), lParam, kb_state, buf, 0);
					p_view->SendToParent(hWnd, WM_CHAR, buf[0], reinterpret_cast<LPARAM>(hWnd));
				}
				if(wParam != VK_ESCAPE && wParam != VK_RETURN)
					p_view->SendToParent(hWnd, WM_USER_KEYDOWN, MAKELPARAM((WORD)wParam, 0), reinterpret_cast<LPARAM>(hWnd));
				return 0;
			}
			break;
		case WM_SETFOCUS:
		case WM_KILLFOCUS:
			if(p_view->IsInState(sfMsgToParent))
				TDialog::DialogProc(GetParent(hWnd), uMsg, wParam, reinterpret_cast<LPARAM>(hWnd));
			break;
		case WM_ERASEBKGND:
			/*
			if(p_view && APPL->EraseBackground(p_view, hWnd, reinterpret_cast<HDC>(wParam), ODT_BUTTON) > 0)
				return 1;
			*/
			return 0;
			break;
		case WM_SETTEXT: // @debug
			{
				SString text;
				text = reinterpret_cast<const char *>(lParam);
			}
			break;
		case WM_GETDLGCODE:
		case BM_SETSTYLE:
			if(p_view && oneof2(APPL->UICfg.WindowViewStyle, UserInterfaceSettings::wndVKFancy, UserInterfaceSettings::wndVKVector)) {
				if(uMsg == WM_GETDLGCODE) {
					return (p_view->IsDefault() ? DLGC_DEFPUSHBUTTON : DLGC_UNDEFPUSHBUTTON);
				}
				else {
					p_view->makeDefault(BIN(wParam & BS_DEFPUSHBUTTON), 0);
					if(LOWORD(lParam)) { // do redraw
						InvalidateRect(hWnd, NULL, TRUE);
						UpdateWindow(hWnd);
					}
					return 0;
				}
			}
			break;
		/*
		case WM_INPUTLANGCHANGE: // @v6.4.4 AHTOXA
			if(p_view->IsInState(sfMsgToParent))
				PostMessage(GetParent(hWnd), uMsg, wParam, lParam);
			break;
		*/
	}
	return CallWindowProc(p_view->PrevWindowProc, hWnd, uMsg, wParam, lParam);
}

const int cmGrabDefault    = 61;
const int cmReleaseDefault = 62;

TButton::TButton(const TRect& bounds, const char *aTitle, ushort aCommand, ushort aFlags, uint bmpID) : TView(bounds), flags(aFlags), command(aCommand)
{
	SubSign = TV_SUBSIGN_BUTTON;
	ViewOptions |= (ofSelectable|ofPreProcess|ofPostProcess);
	Title = aTitle;
	if(!commandEnabled(aCommand))
		Sf |= sfDisabled;
	BmpID = bmpID;
	HBmp  = 0;
}

TButton::~TButton()
{
	ZDeleteWinGdiObject(&HBmp);
	RestoreOnDestruction();
}

HBITMAP TButton::GetBitmap() const
{
	return HBmp;
}

uint TButton::GetBmpID() const
{
	return BmpID;
}

ushort TButton::GetCommand() const
{
	return command;
}

int TButton::IsDefault() const
{
	return BIN(flags & bfDefault);
}

int TButton::LoadBitmap_(uint bmpID)
{
	BmpID = bmpID;
	ZDeleteWinGdiObject(&HBmp);
	HBmp = (BmpID > 32000) ? ::LoadBitmap(0, MAKEINTRESOURCE(BmpID)) : APPL->LoadBitmap_(BmpID);
	return 1;
}

int TButton::SetBitmap(uint bmpID)
{
	int    ok = LoadBitmap_(bmpID);
	if(ok)
		::SendMessage(getHandle(), BM_SETIMAGE, IMAGE_BITMAP, reinterpret_cast<LPARAM>(HBmp));
	return ok;
}

int TButton::handleWindowsMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	int    result = 1;
	HWND   h_wnd;
	switch(uMsg) {
		case WM_INITDIALOG:
			h_wnd = getHandle();
			EnableWindow(h_wnd, !IsInState(sfDisabled));
			TView::SetWindowProp(h_wnd, GWLP_USERDATA, this);
			PrevWindowProc = static_cast<WNDPROC>(TView::SetWindowProp(h_wnd, GWLP_WNDPROC, ButtonDialogProc));
			if(BmpID > 0 && TView::GetWindowStyle(h_wnd) & BS_BITMAP) {
				LoadBitmap_(BmpID);
				::SendDlgItemMessage(Parent, Id, BM_SETIMAGE, IMAGE_BITMAP, reinterpret_cast<LPARAM>(HBmp));
			}
			SetupText(&Title);
			break;
		case WM_COMMAND:
			if(HIWORD(wParam) == BN_CLICKED)
				press(LOWORD(wParam));
			break;
		case WM_CHAR:
		case WM_VKEYTOITEM:
			result = 0;
			break;
	 }
	 return result;
}

void TButton::drawState(bool down)
{
	TView::SSetWindowText(GetDlgItem(Parent, Id), Title);
}

IMPL_HANDLE_EVENT(TButton)
{
	TView::handleEvent(event);
	switch(event.what) {
		case TEvent::evBroadcast:
			switch(TVCMD) {
				case cmDefault:
					if(flags & bfDefault) {
						press();
						clearEvent(event);
					}
					break;
				case cmGrabDefault:
				case cmReleaseDefault:
					if(flags & bfDefault) {
						SETFLAG(flags, bfDefault, TVCMD == cmReleaseDefault);
						Draw_();
					}
					break;
				case cmCommandSetChanged:
					{
						bool   is_enabled = LOGIC(P_Owner ? P_Owner->commandEnabled(command) : commandEnabled(command));
						if((is_enabled && IsInState(sfDisabled)) || (!is_enabled && !IsInState(sfDisabled))) {
							setState(sfDisabled, !is_enabled);
							EnableWindow(getHandle(), !IsInState(sfDisabled));
							Draw_();
						}
					}
					break;
				case cmSearchButton:
					// @v9.5.5 if(command && event.message.infoWord == command)
					if(command && event.message.infoPtr == reinterpret_cast<void *>(command)) // @v9.5.5
						clearEvent(event);
					break;
			}
			break;
	}
}

int TButton::makeDefault(int enable, int sendMsg)
{
	if(sendMsg) {
		if(!(flags & bfDefault))
			TView::messageBroadcast(P_Owner, enable ? cmGrabDefault : cmReleaseDefault, this);
		if(enable)
			::SendMessage(Parent, DM_SETDEFID, (WPARAM)Id, 0);
	}
	SETFLAG(flags, bfDefault, enable);
	return 1;
}

void TButton::setState(uint aState, bool enable)
{
	TView::setState(aState, enable);
	if(aState & (sfSelected | sfActive))
		Draw_();
	if(aState & sfFocused)
		makeDefault(enable);
}

int TButton::TransmitData(int dir, void * pData)
{
	int    s = 0;
	if(dir > 0) {
		Title = static_cast<const char *>(pData);
		// @v9.1.5 SendDlgItemMessage(Parent, Id, WM_SETTEXT, 0, (LPARAM)(const char *)Title);
		TView::SSetWindowText(GetDlgItem(Parent, Id), Title); // @v9.1.5
	}
	else
		s = TView::TransmitData(dir, pData);
	return s;
}

void TButton::press(ushort item)
{
	if(!IsInState(sfDisabled)) {
		// @v9.5.5 TView::message(owner, (flags & bfBroadcast) ? evBroadcast : evCommand, command, this);
		// @v9.5.5 {
		if(flags & bfBroadcast)
			TView::messageBroadcast(P_Owner, command, this);
		else
			MessageCommandToOwner(command);
		// } @v9.5.5
	}
}
//
// TInputLine
//
// static
LPCTSTR TInputLine::WndClsName = _T("Edit");

TInputLine::InputStat::InputStat()
{
	Reset();
}

void TInputLine::InputStat::Reset()
{
	Last = 0;
	TmSum = 0.0;
	TmSqSum = 0.0;
}

void TInputLine::InputStat::CheckIn()
{
	clock_t c = clock();
	if(!Last) {
		Last = c;
		TmSum = 0.0;
		TmSqSum = 0.0;
	}
	else {
		clock_t diff = (c - Last);
		TmSum += diff;
		TmSqSum += (diff*diff);
		Last = c;
	}
}

//static
LRESULT CALLBACK TInputLine::DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	TInputLine * p_view = static_cast<TInputLine *>(TView::GetWindowUserData(hWnd));
	switch(uMsg) {
		case WM_DESTROY: p_view->OnDestroy(hWnd); return 0;
		case WM_COMMAND:
			if(HIWORD(wParam) == 1)
				SendMessage(APPL->H_TopOfStack, uMsg, wParam, lParam);
			break;
		case WM_CHAR:
			if(p_view->GetCombo() || p_view->hasWordSelector()) {
				if(!oneof2(wParam, VK_ESCAPE, VK_RETURN)) {
					if(!oneof2(wParam, '+', '-'))
						p_view->SendToParent(hWnd, uMsg, wParam, reinterpret_cast<LPARAM>(hWnd));
					return 0;
				}
			}
			break;
		case WM_KEYUP: p_view->SendToParent(hWnd, uMsg, wParam, reinterpret_cast<LPARAM>(hWnd)); break;
		case WM_SYSKEYDOWN:
		case WM_KEYDOWN:
			{
				const long _style = TView::GetWindowStyle(hWnd);
				const int _ml = BIN(_style & ES_MULTILINE);
				const int _k = wParam;
				if((_k >= VK_F1 && _k <= VK_F12) ||
					(!_ml && oneof6(_k, VK_ADD, VK_SUBTRACT, VK_DOWN, VK_UP, VK_PRIOR, VK_NEXT)) ||
					((p_view->GetCombo() || p_view->hasWordSelector()) && _k == VK_DELETE) ||
					(_k == VK_RETURN && (0x8000 & GetKeyState(VK_CONTROL)))) {
					p_view->SendToParent(hWnd, WM_VKEYTOITEM, MAKELPARAM((WORD)_k, 0), reinterpret_cast<LPARAM>(hWnd));
					return 0;
				}
				else if(!oneof2(_k, VK_ESCAPE, VK_RETURN))
					p_view->SendToParent(hWnd, WM_USER_KEYDOWN, MAKELPARAM((WORD)_k, 0), reinterpret_cast<LPARAM>(hWnd));
			}
			break;
		case WM_LBUTTONDBLCLK:
			CALLPTRMEMB(p_view, MessageCommandToOwner(cmInputDblClk));
			break;
		case WM_MBUTTONDOWN:
			if(p_view->GetCombo() || p_view->hasWordSelector()) {
				p_view->SendToParent(hWnd, WM_VKEYTOITEM, MAKELPARAM((WORD)VK_DELETE, 0), reinterpret_cast<LPARAM>(hWnd));
				return 0;
			}
			break;
		case WM_MOUSEWHEEL:
			if(p_view->GetCombo())
				p_view->SendToParent(hWnd, WM_VKEYTOITEM, MAKELPARAM((WORD)VK_DOWN, 0), reinterpret_cast<LPARAM>(hWnd));
			else
				p_view->OnMouseWheel((short)HIWORD(wParam));
			break;
		case WM_SETFOCUS:
		case WM_KILLFOCUS:
			if(p_view->IsInState(sfMsgToParent))
				TDialog::DialogProc(GetParent(hWnd), uMsg, wParam, reinterpret_cast<LPARAM>(hWnd));
			if(oneof2(APPL->UICfg.WindowViewStyle, UserInterfaceSettings::wndVKFancy, UserInterfaceSettings::wndVKVector))
				SetWindowPos(hWnd, 0, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER|SWP_NOOWNERZORDER|SWP_NOACTIVATE|SWP_FRAMECHANGED|SWP_DRAWFRAME);
			break;
		case WM_PASTE:
			CALLPTRMEMB(p_view, OnPaste());
			break;
		case WM_NCPAINT:
			{
				HWND   focus_hwnd = GetFocus();
				DRAWITEMSTRUCT di;
				MEMSZERO(di);
				di.CtlID = GetDlgCtrlID(hWnd);
				di.CtlType = ODT_EDIT;
				di.hDC = GetWindowDC(hWnd);
				GetWindowRect(hWnd, &di.rcItem);
				di.rcItem.right  -= di.rcItem.left;
				di.rcItem.bottom -= di.rcItem.top;
				di.rcItem.left    = 0;
				di.rcItem.top     = 0;
				SETFLAG(di.itemState, ODS_FOCUS, focus_hwnd == hWnd);
				SETFLAG(di.itemState, ODS_DISABLED, !IsWindowEnabled(hWnd));
				lParam = reinterpret_cast<LPARAM>(&di);
				if(APPL->DrawControl(hWnd, uMsg, wParam, reinterpret_cast<LPARAM>(&di)) > 0) {
					ReleaseDC(hWnd, di.hDC);
					/* Если в DrawControl используется RoundRect, то этот кусок необходимо включить в код
					InvalidateRect(hWnd, 0, TRUE);
					UpdateWindow(hWnd);
					*/
					return 0;
				}
				ReleaseDC(hWnd, di.hDC);
			}
			break;
		/*
		case WM_INPUTLANGCHANGE: // @v6.4.4 AHTOXA
			if(p_view->IsInState(sfMsgToParent))
				PostMessage(GetParent(hWnd), uMsg, wParam, lParam);
			break;
		*/
	}
	return CallWindowProc(p_view->PrevWindowProc, hWnd, uMsg, wParam, lParam);
}

int TInputLine::OnPaste()
{
	InlSt |= stPaste;
	if(hasWordSelector()) {
		SString symb;
		if(::OpenClipboard(0)) {
			HANDLE h_cb = ::GetClipboardData(CF_TEXT);
			if(h_cb) {
				LPTSTR p_str = static_cast<LPTSTR>(::GlobalLock(h_cb)); // @unicodeproblem
				if(p_str) {
					(symb = SUcSwitch(p_str)).Transf(CTRANSF_OUTER_TO_INNER); // @unicodeproblem
					::GlobalUnlock(h_cb);
				}
			}
			::CloseClipboard();
		}
		UiSearchTextBlock::ExecDialog(getHandle(), 0, symb, 1, P_WordSelBlk, 0);
	}
	return -1;
}

int TInputLine::OnMouseWheel(int delta)
{
	int    ok = -1;
	if(delta && !IsInState(sfReadOnly)) {
		if(GETSTYPE(type) == S_DATE) {
			LDATE dt;
			TransmitData(-1, &dt);
			SETIFZ(dt, getcurdate_());
			dt = plusdate(dt, (delta > 0) ? +1 : -1);
			TransmitData(+1, &dt);
			ok = 1;
		}
		else if(GETSTYPE(type) == S_INT) {
			size_t sz = GETSSIZE(type);
			long   i = 0;
			TransmitData(-1, &i);
			i += (delta > 0) ? +1 : -1;
			TransmitData(+1, &i);
			ok = 1;
		}
		else if(GETSTYPE(type) == S_FLOAT) {
			int p = GETSPRECD(type);
			if(p == 0) {
				double v = 0.0;
				if(GETSSIZED(type) == 8)
					TransmitData(-1, &v);
				else if(GETSSIZED(type) == 4) {
					float f = 0.0f;
					TransmitData(-1, &f);
					v = (double)f;
				}
				v += (delta > 0) ? +1.0 : -1.0;
				if(GETSSIZED(type) == 8) {
					TransmitData(+1, &v);
					ok = 1;
				}
				else if(GETSSIZED(type) == 4) {
					float f = (float)v;
					TransmitData(+1, &f);
					ok = 1;
				}
			}
		}
	}
	return ok;
}

#if 0 // @v9.1.3 {
// virtual
int TInputLine::Paint_(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	int    r = 0;
	if(APPL->UICfg.WindowViewStyle >= UserInterfaceSettings::wndVKFancy) {
		RECT   cli_rect;
		GetClientRect(hWnd, &cli_rect);
		if(uMsg == WM_LBUTTONDOWN) {
			// @v9.1.5 char   buf[1024];
			POINT  c_pos;
			long   exstyle = TView::GetWindowExStyle(hWnd);
			// @v9.1.5 memzero(buf, sizeof(buf));
			// @v9.1.5 ::GetWindowText(hWnd, buf, sizeof(buf));
			SString text_buf;
			TView::SGetWindowText(hWnd, text_buf);
			::SetFocus(hWnd);
			size_t buf_len = text_buf.Len();
			if(buf_len) {
				int    pos_found = 0;
				c_pos.x  = LOWORD(lParam);
				if(c_pos.x <= (cli_rect.left + 5)) {
					c_pos.x = (cli_rect.left + 5);
					pos_found = 1;
				}
				if(c_pos.x >= cli_rect.right + 5) {
					c_pos.x  = cli_rect.right - 5;
					pos_found = 1;
				}
				if(!pos_found) {
					int    len = cli_rect.left + 5;
					int    prev_len = 0;
					SIZE   size;
					HDC    hdc = GetDC(hWnd);
					for(uint i = 0; i < buf_len; i++) {
						GetTextExtentPoint32(hdc, onecstr(text_buf.C(i)), 1, &size);
						prev_len = len;
						len += size.cx;
						if(len >= c_pos.x) {
							c_pos.x = (abs(prev_len - c_pos.x) > abs(len - c_pos.x)) ? len : prev_len;
							pos_found = 1;
							break;
						}
						prev_len = len;
					}
					if(!pos_found)
						c_pos.x = len;
					::ReleaseDC(hWnd, hdc);
				}
			}
			else
				c_pos.x = (exstyle & ES_RIGHT) ? cli_rect.right - 5 : cli_rect.left + 5;
			c_pos.y  = cli_rect.top + 3;
			SetCaretPos(c_pos.x, c_pos.y);
			InvalidateRect(hWnd, NULL, TRUE);
			UpdateWindow(hWnd);
			r = 1;
		}
		else {
			int    focused = BIN(GetFocus() == hWnd);
			PAINTSTRUCT ps;
			DRAWITEMSTRUCT di;
			memzero(&di, sizeof(di));
			di.CtlType = ODT_EDIT;
			di.CtlID   = Id;
			di.itemID  = Id;
			di.itemAction = (GetFocus() == hWnd) ? ODA_FOCUS : ODA_DRAWENTIRE;
			SETFLAG(di.itemState, ODS_FOCUS, focused);
			SETFLAG(di.itemState, ODS_DISABLED, !IsWindowEnabled(hWnd));
			di.hwndItem = hWnd;
			di.rcItem = cli_rect;
			BeginPaint(hWnd, (LPPAINTSTRUCT)&ps);
			di.hDC = ps.hdc;
			r = APPL->DrawControl(GetParent(hWnd), WM_DRAWITEM, 0, (LPARAM)&di);
			EndPaint(hWnd, (LPPAINTSTRUCT)&ps);
			ReleaseDC(hWnd, di.hDC);
		}
	}
	return r;
}
#endif // } 0 @v9.1.3

int TInputLine::Implement_GetText()
{
	int    ok = 1;
	/* @v9.1.5
	char   st_buf[1024];
	char * ptr = 0;
	STempBuffer t_buf(0);
	size_t buflen = (maxLen > 0) ? (maxLen+1) : 4096;
	if(buflen < sizeof(st_buf))
		ptr = st_buf;
	else {
		t_buf.Alloc(buflen);
		ptr = t_buf;
	}
	if(ptr) {
		// @v9.1.5 SendDlgItemMessage(Parent, Id, WM_GETTEXT, buflen, (long)ptr);
		(Data = ptr).Transf(CTRANSF_OUTER_TO_INNER);
	}
	else
		ok = 0;
	*/
	TView::SGetWindowText(GetDlgItem(Parent, Id), Data); // @v9.1.5
	Data.Transf(CTRANSF_OUTER_TO_INNER); // @v9.1.5
	Data.Trim((maxLen > 0) ? maxLen : (4096-1)); // @v9.1.5
	return ok;
}

int TInputLine::handleWindowsMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg) {
		case WM_INITDIALOG:
			{
				SendDlgItemMessage(Parent, Id, EM_SETLIMITTEXT, maxLen ? (maxLen-1) : 0, 0);
				if(format & STRF_PASSWORD)
					SendDlgItemMessage(Parent, Id, EM_SETPASSWORDCHAR, DEFAULT_PASSWORD_SYMB, 0);
				Draw_();
				HWND h_wnd = getHandle();
				TView::SetWindowProp(h_wnd, GWLP_USERDATA, this);
				PrevWindowProc = static_cast<WNDPROC>(TView::SetWindowProp(h_wnd, GWLP_WNDPROC, TInputLine::DlgProc));
				if(TView::GetWindowStyle(h_wnd) & ES_READONLY)
					Sf |= sfReadOnly;
			}
			break;
		case WM_VKEYTOITEM:
			if(wParam == VK_DOWN) {
				if(P_Owner) {
					TEvent t;
					t.what = TEvent::evKeyDown;
					t.keyDown.keyCode = kbDown;
					P_Owner->handleEvent(t);
				}
				CALLPTRMEMB(P_Combo, handleWindowsMessage(WM_COMMAND, 0, 0));
			}
			else if(oneof3(wParam, VK_UP, VK_PRIOR, VK_NEXT) && P_Owner) {
				TEvent t;
				t.what = TEvent::evKeyDown;
				t.keyDown.keyCode = (wParam == VK_UP) ? kbUp : ((wParam == VK_PRIOR) ? kbPgUp : kbPgDn);
				P_Owner->handleEvent(t);
			}
			else if(wParam == VK_DELETE) {
				if(P_Combo)
					P_Combo->handleWindowsMessage(WM_USER_COMBO_CLEAR, wParam, lParam);
				else if(hasWordSelector())
					P_WordSelBlk->SetupData(0);
				else
					return 0;
			}
			else
				return 0;
			return -2;
		case WM_KEYUP:
			Implement_GetText();
			MessageCommandToOwner(cmInputUpdatedByBtn);
			return 0;
		case WM_CHAR:
			if(P_Combo)
				P_Combo->handleWindowsMessage(WM_USER_COMBO_ACTIVATEBYCHAR, wParam, lParam);
			else if(hasWordSelector()) {
				SString symb;
				symb.CatChar((char)wParam);
				UiSearchTextBlock::ExecDialog(getHandle(), 0, symb, 1, P_WordSelBlk, 0);
			}
			break;
		case WM_COMMAND:
			if(HIWORD(wParam) == EN_CHANGE) {
				const uint prev_len = Data.Len();
				Implement_GetText();
				if(Data.Len() == (prev_len+1)) {
					if(InlSt & stSerialized) {
						Stat.CheckIn();
					}
					else if(prev_len == 0) {
						InlSt |= stSerialized;
						Stat.Reset();
						Stat.CheckIn();
					}
				}
				else {
					InlSt &= ~stSerialized;
					Stat.Reset();
					if(Data.Len() == 0)
						InlSt &= ~stPaste;
				}
				MessageCommandToOwner(cmInputUpdated);
			}
			break;
	 }
	 return 1;
}

void TInputLine::Init()
{
	SubSign = TV_SUBSIGN_INPUTLINE;
	maxLen = DEFAULT_MAX_LEN;
	Sf      |= sfMsgToParent;
	ViewOptions |= ofSelectable;
	format   = 0;
	type     = 0;
	P_Combo = 0;
	InlSt = stValidStr;
}

TInputLine::TInputLine(const TRect & bounds, TYPEID typ, long fmt) : TView(bounds)
{
	TInputLine::Init();
	maxLen = SFMTLEN(fmt);
	format   = fmt;
	type     = typ;
}

TInputLine::~TInputLine() { RestoreOnDestruction(); }
const char * TInputLine::getText() { return Data.cptr(); }
ComboBox * TInputLine::GetCombo() { return P_Combo; }

void TInputLine::setMaxLen(int newMaxLen)
{
	maxLen = newMaxLen;
	::SendDlgItemMessage(Parent, Id, EM_SETLIMITTEXT, (maxLen > 0) ? (maxLen-1) : 0, 0);
}

void TInputLine::selectAll(int enable)
{
	::SendDlgItemMessage(Parent, Id, EM_SETSEL, enable ? 0 : -1, -1);
}

void TInputLine::setupCombo(ComboBox * pCombo)
{
	P_Combo = pCombo;
	CALLPTRMEMB(pCombo, SetLink(this));
}

void TInputLine::disableDeleteSelection(int _disable)
{
	SETFLAG(InlSt, stDisableDelSel, _disable);
	::SendDlgItemMessage(Parent, Id, EM_SETSEL, -1, -1);
}

void TInputLine::Implement_Draw()
{
	// @v9.1.5 char   buf[4096]; // @v8.3.11 [1024]-->[4096]
	// @v9.1.5 SendDlgItemMessage(Parent, Id, WM_SETTEXT, 0, (long)SOemToChar(Data.CopyTo(buf, sizeof(buf))));
	SString text_buf; // @v9.1.5
	(text_buf = Data).Transf(CTRANSF_INNER_TO_OUTER); // @v9.1.5
	TView::SSetWindowText(GetDlgItem(Parent, Id), text_buf); // @v9.1.5
	if(IsInState(sfSelected))
		::SendDlgItemMessage(Parent, Id, EM_SETSEL, (InlSt & stDisableDelSel) ? -1 : 0, -1);
	if(InlSt & stDisableDelSel)
		::SendDlgItemMessage(Parent, Id, WM_KEYDOWN, VK_END, 0);
}

void TInputLine::setType(TYPEID typ)
{
	type = typ;
	setMaxLen(SFMTLEN(format));
}

void TInputLine::setFormat(long f)
{
	if(f != format) {
		char   buf[1024];
		TransmitData(-1, buf);
		format = f;
		setMaxLen(SFMTLEN(format));
		if(format & STRF_PASSWORD)
			::SendDlgItemMessage(Parent, Id, EM_SETPASSWORDCHAR, DEFAULT_PASSWORD_SYMB, 0);
		TransmitData(+1, buf);
	}
}

int TInputLine::TransmitData(int dir, void * pData)
{
	int    s = stsize(type);
	if(dir > 0) {
		char   temp[4096];
		if(hasWordSelector() && !P_WordSelBlk->IsTextMode()) {
			P_WordSelBlk->SetupData(pData ? *static_cast<long *>(pData) : 0);
		}
		else {
			if(pData == 0)
				temp[0] = 0;
			else {
				long f = MKSFMTD(0, SFMTPRC(format), SFMTFLAG(format)) & ~(SFALIGNMASK|STRF_PASSWORD);
				sttostr(type, pData, f, temp);
			}
			setText(temp);
		}
	}
	else if(dir < 0) {
		if(P_Combo)
			s = P_Combo->TransmitData(dir, pData);
		else if(hasWordSelector() && !P_WordSelBlk->IsTextMode()) {
			long   id = 0L;
			SString buf;
			P_WordSelBlk->GetData(&id, buf);
			s = 4;
			ASSIGN_PTR(static_cast<long *>(pData), id);
		}
		else {
			if(Data.cptr() == 0)
				Data.Space() = 0;
			SETFLAG(InlSt, stValidStr, stfromstr(type, pData, format, Data));
		}
	}
	return s;
}

size_t TInputLine::getCaret()
{
	POINT p;
	GetCaretPos(&p);
	DWORD c = ::SendMessage(getHandle(), EM_CHARFROMPOS, 0, MAKELPARAM(p.x, p.y));
	return LoWord(c);
}

void TInputLine::setCaret(size_t pos)
{
	DWORD c = ::SendMessage(getHandle(), EM_POSFROMCHAR, pos, pos /**/);
	SetCaretPos(LoWord(c), HiWord(c));
}

void TInputLine::getText(SString & rBuf) const
{
	rBuf = Data;
}

void TInputLine::setText(const char * b)
{
	(Data = b).Strip();
	if(maxLen)
		Data.Trim(maxLen).Strip();
	Draw_();
}

IMPL_HANDLE_EVENT(TInputLine)
{
	TView::handleEvent(event);
	if(event.isCmd(cmDraw)) {
		Implement_Draw();
	}
	else if(TVCOMMAND && IsInState(sfSelected)) {
		if(event.message.infoPtr)
			if(event.message.command == cmGetFocusedNumber)
				*static_cast<double *>(event.message.infoPtr) = Data.ToReal();
			else if(event.message.command == cmGetFocusedText)
				Data.CopyTo(static_cast<char *>(event.message.infoPtr), 0);
			else
				return;
		else
			return;
	}
	else
		return;
	clearEvent(event);
}

void TInputLine::setState(uint aState, bool enable)
{
	TView::setState(aState, enable);
	if(aState == sfReadOnly)
		::SendDlgItemMessage(Parent, Id, EM_SETREADONLY, BIN(enable), 0);
}

int TInputLine::GetStatistics(Statistics * pStat) const
{
	int    ok = 1;
	if(pStat) {
		memzero(pStat, sizeof(*pStat));
		pStat->SymbCount = static_cast<int>(Data.Len());
		SETFLAG(pStat->Flags, Statistics::fSerialized, InlSt & stSerialized);
		SETFLAG(pStat->Flags, Statistics::fPaste, InlSt & stPaste);
		const int count = (pStat->SymbCount-1);
		if(count) {
			const double sum = Stat.TmSum;
			const double sq_sum = Stat.TmSqSum;
			const double mean = sum / count;
			pStat->IntervalMean = (1000.0 * mean) / CLOCKS_PER_SEC;
			if(count > 1) {
				const double var = sq_sum / count - mean * (2.0 * sum / count - mean);
		 		pStat->IntervalStdDev = (1000.0 * sqrt(var * count / (count-1))) / CLOCKS_PER_SEC;
			}
		}
	}
	else
		ok = 0;
	return ok;
}
//
// ComboBoxInputLine
//
ComboBoxInputLine::ComboBoxInputLine(ushort aId) : TInputLine(TRect(), 0, 0)
{
	Id = aId;
}

int ComboBoxInputLine::TransmitData(int dir, void * pData)
{
	int    s = 0;
	if(dir == 0)
		s = TInputLine::TransmitData(dir, pData);
	else if(dir > 0) {
		Data = static_cast<const char *>(pData);
		if(maxLen)
			Data.Trim(maxLen-1);
	}
	else if(dir < 0) {
		Implement_GetText();
	}
	return s;
}

#if 0 // @v9.1.5 @unused {
//
// TInfoPane
//
TInfoPane::TInfoPane(TRect & r) : TView(r)
{
	text = 0;
}

TInfoPane::~TInfoPane()
{
	delete text;
}

void TInfoPane::draw()
{
	SendDlgItemMessage(Parent, Id, WM_SETTEXT, 0, (long)text);
}

void TInfoPane::setText(char * str)
{
	delete text;
	text = newStr(str);
	SOemToChar(text);
	Draw_();
}

int TInfoPane::handleWindowsMessage(UINT  uMsg, WPARAM  wParam, LPARAM  lParam)
{
	if(uMsg == WM_INITDIALOG)
		Draw_();
	return 1;
}
#endif // } 0 @v9.1.5
//
// TCluster
//
static BOOL CALLBACK ClusterDialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	TCluster * p_view = static_cast<TCluster *>(TView::GetWindowUserData(hWnd));
	switch(uMsg) {
		case WM_DESTROY:
			p_view->OnDestroy(hWnd);
			return 0;
		case WM_SYSKEYDOWN:
		case WM_KEYDOWN:
			if(wParam >= VK_F1 && wParam <= VK_F12 || wParam==VK_ESCAPE || (wParam == VK_RETURN && (0x8000 & GetKeyState(VK_CONTROL)))) {
				::SendMessage(GetParent(hWnd), WM_VKEYTOITEM, MAKELPARAM((WORD)wParam, 0), reinterpret_cast<LPARAM>(hWnd));
				return 0;
			}
			else if(wParam != VK_ESCAPE && wParam != VK_RETURN)
				p_view->SendToParent(hWnd, WM_USER_KEYDOWN, MAKELPARAM((WORD)wParam, 0), reinterpret_cast<LPARAM>(hWnd));
			break;
		case WM_SETFOCUS:
		case WM_KILLFOCUS:
			TDialog::DialogProc(GetParent(hWnd), uMsg, wParam, reinterpret_cast<LPARAM>(hWnd));
			break;
		case WM_NCPAINT:
		case BM_SETCHECK:
		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_PAINT:
		case WM_CANCELMODE:
		case WM_ENABLE:
			/* if(p_view && p_view->Paint_(hWnd, uMsg, wParam, lParam) > 0)
				return 0;
			*/
			break;
		/*
		case WM_INPUTLANGCHANGE:
			if(p_view->IsInState(sfMsgToParent))
				PostMessage(GetParent(hWnd), uMsg, wParam, lParam);
			break;
		*/
	}
	return ::CallWindowProc(p_view->PrevWindowProc, hWnd, uMsg, wParam, lParam);
}

TCluster::TCluster(const TRect & bounds, int aClusterKind, const StringSet * pStrings) : TView(bounds), Value(0), Sel(0), Kind(aClusterKind), DisableMask(0)
{
	assert(oneof2(aClusterKind, RADIOBUTTONS, CHECKBOXES));
	SubSign = TV_SUBSIGN_CLUSTER;
	ViewOptions |= (ofSelectable|ofPreProcess|ofPostProcess);
	if(pStrings) {
		SString temp_buf;
		for(uint i = 0; pStrings->get(&i, temp_buf);)
			addItem(-1, temp_buf);
	}
}

TCluster::~TCluster()
{
	//
	// Так как иногда экземпляр этого объекта разрушается до того, как будет
	// разрушено собственно окно, возвращаем назад подставную оконную процедуру.
	//
	// К сожалению, те же действия дублируются одновременно в самой подставной
	// процедуре (по сообщению WM_DESTROY). Это приводит к некоторой запутанности кода.
	//
	if(PrevWindowProc)
		for(int i = 0; i < 33; i++) {
			HWND   h_wnd = GetDlgItem(Parent, MAKE_BUTTON_ID(Id, i+1));
			TView::SetWindowProp(h_wnd, GWLP_WNDPROC, PrevWindowProc);
		}
}

#if 0 // @v9.1.3 {
// virtual
int TCluster::Paint_(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	int    r = 0;
	if(APPL->UICfg.WindowViewStyle >= UserInterfaceSettings::wndVKFancy) {
		int    kind = getKind();
		ushort btn_id = (ushort)GetWindowLong(hWnd, GWL_ID);
		if(uMsg == WM_LBUTTONDOWN || uMsg == BM_SETCHECK || uMsg == WM_CANCELMODE || uMsg == WM_ENABLE) {
			int updated = 0;
			if(uMsg == WM_LBUTTONDOWN) {
				press(btn_id);
				SetFocus(hWnd);
				if(kind == RADIOBUTTONS) {
					for(int i = 0; i < 33; i++) {
						int    button_id = MAKE_BUTTON_ID(Id, i+1);
						HWND   h_wnd = GetDlgItem(Parent, button_id);
						if(h_wnd) {
							InvalidateRect(h_wnd, NULL, TRUE);
							UpdateWindow(h_wnd);
						}
					}
					updated = 1;
				}
			}
			if(!updated) {
				InvalidateRect(hWnd, NULL, TRUE);
				UpdateWindow(hWnd);
			}
			r = 1;
		}
		else {
			int    checked = 0;
			int    focused = BIN(GetFocus() == hWnd);
			PAINTSTRUCT ps;
			DRAWITEMSTRUCT di;
			memzero(&di, sizeof(di));
			if(kind == RADIOBUTTONS)
				di.CtlType = ODT_RADIOBTN;
			else if(kind == CHECKBOXES)
				di.CtlType = ODT_CHECKBOX;
			checked = isChecked(btn_id);
			di.CtlID   = Id;
			di.itemID  = Id;
			di.itemAction = (GetFocus() == hWnd) ? ODA_FOCUS : ODA_DRAWENTIRE;
			SETFLAG(di.itemState, ODS_SELECTED, checked);
			SETFLAG(di.itemState, ODS_FOCUS, focused);
			SETFLAG(di.itemState, ODS_DISABLED, !isEnabled(btn_id));
			di.hwndItem = hWnd;
			GetClientRect(hWnd, &di.rcItem);
			BeginPaint(hWnd, (LPPAINTSTRUCT)&ps);
			di.hDC = ps.hdc;
			r = APPL->DrawControl(GetParent(hWnd), WM_DRAWITEM, 0, (LPARAM)&di);
			EndPaint(hWnd, (LPPAINTSTRUCT)&ps);
			ReleaseDC(hWnd, di.hDC);
		}
	}
	return r;
}
#endif // } 0 @v9.1.3

int TCluster::handleWindowsMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	int    result = 1;
	int    i;
	switch(uMsg) {
		case WM_INITDIALOG:
			{
				HWND hw_cluster = getHandle();
				if(hw_cluster) {
					HWND hw_parent = ::GetParent(hw_cluster);
					if(IsInState(sfDisabled)) {
						EnableWindow(hw_cluster, 0);
						DisableMask = -1;
					}
					SetupText(0);
					RECT  rc_temp;
					::GetWindowRect(hw_cluster, &rc_temp);
					const  TRect rc_cluster = rc_temp;
					::GetWindowRect(hw_parent, &rc_temp);
					const  TRect  rc_parent = rc_temp;
					TRect  rc_prev; // Координаты предыдущего элемента
					int    dispersion_left = 0; // Разброс left-координат
					int    dispersion_top = 0;  // Разброс top-координат
                    int    direction = DIREC_UNKN; //DIREC_XXX
					int    items_count = 0;
					for(i = 0; i < 33; i++) {
						int    button_id = MAKE_BUTTON_ID(Id, i+1);
						HWND   h_wnd = GetDlgItem(Parent, button_id);
						if(h_wnd) {
							items_count++;
							{
								::GetWindowRect(h_wnd, &rc_temp);
								TRect rc_item = rc_temp;
								if(i > 0) {
									dispersion_left = MAX(dispersion_left, abs(rc_item.a.x - rc_prev.a.x));
									dispersion_top  = MAX(dispersion_top,  abs(rc_item.a.y - rc_prev.a.y));
								}
								rc_prev = rc_item;
							}
							TView::SetWindowProp(h_wnd, GWLP_USERDATA, this);
							WNDPROC prev_proc = static_cast<WNDPROC>(TView::SetWindowProp(h_wnd, GWLP_WNDPROC, ClusterDialogProc));
							SETIFZ(PrevWindowProc, prev_proc);
							if(Kind == RADIOBUTTONS) {
								WPARAM state = (i == Value) ? BST_CHECKED : BST_UNCHECKED;
								SendDlgItemMessage(Parent, button_id, BM_SETCHECK, state, 0);
							}
							else if(Value & (1<<i))
								SendDlgItemMessage(Parent, button_id, BM_SETCHECK, BST_CHECKED, 0);
							EnableWindow(h_wnd, (DisableMask & (1<<i)) == 0);
						}
					}
					//
					// Arrange
					//
					if(dispersion_left <= 4)
						direction = DIREC_VERT;
					else if(dispersion_top <= 4)
						direction = DIREC_HORZ;
					if(direction == DIREC_VERT) {
						int16  first_left = 0;
						for(i = 0; i < items_count; i++) {
							int    button_id = MAKE_BUTTON_ID(Id, i+1);
							HWND   h_wnd = GetDlgItem(Parent, button_id);
							if(h_wnd) {
								int    y_offs = 0;
								::GetWindowRect(h_wnd, &rc_temp);
								TRect rc_item = rc_temp;
								if(i == 0)
									first_left = rc_item.a.x;
								else {
									const int min_gap_y = 1;
									if((rc_item.a.y - rc_prev.b.y) < min_gap_y)
										y_offs = rc_prev.b.y + min_gap_y - rc_item.a.y;
									if(y_offs || rc_item.a.x != first_left) {
										POINT   pt_lu;
										pt_lu.x = first_left; // rc_item.a.x;
										pt_lu.y = rc_item.a.y + y_offs;
										MapWindowPoints(NULL, hw_parent, &pt_lu, 1);
										::MoveWindow(h_wnd, pt_lu.x, pt_lu.y, rc_item.width(), rc_item.height(), FALSE);
										::GetWindowRect(h_wnd, &rc_temp);
										rc_item = rc_temp;
									}
								}
								rc_prev = rc_item;
							}
						}
					}
				}
			}
			break;
		case WM_COMMAND:
			if(HIWORD(wParam) == BN_CLICKED)
				press(LOWORD(wParam));
			break;
		case WM_VKEYTOITEM:
			result = 0;
			break;
	 }
	 return result;
}

int TCluster::TransmitData(int dir, void * pData)
{
	int    s = sizeof(Value);
	if(dir > 0) {
		Value = *(ushort *)pData;
		Draw_();
		if(Kind == RADIOBUTTONS)
			Sel = Value;
		WPARAM state;
		for(uint i = 0; i < Strings.getCount(); i++) {
			if(Kind == RADIOBUTTONS)
				state = (static_cast<int>(i) == Sel) ? BST_CHECKED : BST_UNCHECKED;
			else
				state = (Value & (1 << i)) ? BST_CHECKED : BST_UNCHECKED;
			SendDlgItemMessage(Parent, MAKE_BUTTON_ID(Id, i+1), BM_SETCHECK, state, 0);
		}
	}
	else if(dir < 0) {
		*(ushort *)pData = Value;
		Draw_();
	}
	return s;
}

void TCluster::setState(uint aState, bool enable)
{
	TView::setState(aState, enable);
	if(aState == sfDisabled) {
		DWORD dis = DisableMask;
		if(IsInState(sfDisabled))
			dis = 0xffffffff;
		for(uint i = 0; i < Strings.getCount(); i++)
			EnableWindow(GetDlgItem(Parent, MAKE_BUTTON_ID(Id, i+1)), (dis &(1<<i))==0);
	}
	else if(aState == sfVisible) {
		for(uint i = 0; i < Strings.getCount(); i++)
			ShowWindow(GetDlgItem(Parent, MAKE_BUTTON_ID(Id, i+1)), enable);
	}
	else if(aState == sfSelected)
		Draw_();
}

bool TCluster::mark(int item) { return LOGIC((Kind == RADIOBUTTONS) ? (item == Value) : (Value & (1 << item))); }

void TCluster::press(ushort item)
{
	const short citem = BUTTON_ID(item)-1;
	Value = (Kind == RADIOBUTTONS) ? citem : Value ^ (1 << citem);
	MessageCommandToOwner(cmClusterClk);
}

int TCluster::isEnabled(ushort item) const
{
	int    enabled = 1;
	short  citem = BUTTON_ID(item) - 1;
	DWORD  dis = DisableMask;
	if(IsInState(sfDisabled))
		dis = 0xffffffff;
	if(dis & (1 << citem))
		enabled = 0;
	return enabled;
}

int TCluster::isChecked(ushort item) const
{
	int    checked = 0;
	short  citem = BUTTON_ID(item) - 1;
	if(Kind == RADIOBUTTONS)
		checked = (Value == citem) ? 1 : 0;
	else
		if(Value & (1 << citem))
			checked = 1;
	return checked;
}

int TCluster::column(int item) const
{
	int    col = 0;
	if(item >= ViewSize.y) {
		col = -6;
		for(int i = 0, l = 0, width = 0; i <= item; i++) {
			if((i % ViewSize.y) == 0) {
				col += width + 6;
				width = 0;
			}
			if(i < static_cast<int>(Strings.getCount()))
				l = sstrlen(Strings.at(i));
			if(l > width)
				width = l;
		}
	}
	return col;
}

int TCluster::row(int item) const
{
	return (item % ViewSize.y);
}

uint TCluster::getNumItems() const
{
	return Strings.getCount();
}

void TCluster::addItem(int item, const char * pStr)
{
	char * p_dup_str = newStr(pStr);
	if(p_dup_str) {
		if(item == -1)
			Strings.insert(p_dup_str);
		else
			Strings.atInsert(item, p_dup_str);
		ViewSize.y = Strings.getCount();
	}
}

int TCluster::isItemEnabled(int item) const
{
	return (DisableMask & (1<<item)) ? 0 : 1;
}

void TCluster::deleteItem(int item)
{
	Strings.atFree(item);
	ViewSize.y = Strings.getCount();
}

void TCluster::disableItem(int item, int disable)
{
	SETFLAG(DisableMask, (1<<item), disable);
	EnableWindow(GetDlgItem(Parent, MAKE_BUTTON_ID(Id, item+1)), disable == 0);
}

int TCluster::getText(int pos, char * buf, uint bufLen)
{
	int    ok = 0;
	if(pos >= 0 && pos < static_cast<int>(getNumItems())) {
		strnzcpy(buf, Strings.at(pos), bufLen);
		ok = 1;
	}
	return ok;
}

int TCluster::setText(int pos, const char * pText)
{
	int    ok = 0;
	if(pos >= 0 && pos < static_cast<int>(getNumItems())) {
		// @v9.1.5 char   temp_buf[512];
		// @v9.1.5 OemToChar(buf, temp_buf);
		// @v9.1.5 SendDlgItemMessage(Parent, MAKE_BUTTON_ID(Id, pos+1), WM_SETTEXT, 0, (long)temp_buf);
		SString temp_buf = pText;
		TView::SSetWindowText(GetDlgItem(Parent, MAKE_BUTTON_ID(Id, pos+1)), temp_buf.Transf(CTRANSF_INNER_TO_OUTER));
		deleteItem(pos);
		addItem(pos, pText);
		ok = 1;
	}
	return ok;
}

void TCluster::deleteAll()
{
	Strings.freeAll();
}

int TCluster::addAssoc(long pos, long val)
{
	return ValAssoc.Update(pos, val, 0);
}

int TCluster::getItemByAssoc(long val, int * pItem) const
{
	int    ok = 0;
	LongArray key_list;
	ValAssoc.GetListByVal(val, key_list);
	for(uint i = 0; !ok && i < key_list.getCount(); i++) {
		const long key = key_list.get(i);
		if(key >= 0) {
			ASSIGN_PTR(pItem, key);
			ok = 1;
		}
	}
	return ok;
}

int TCluster::setDataAssoc(long val)
{
	ushort v = 0;
	LAssoc * p_assoc;
	if(Kind == RADIOBUTTONS) {
		uint   i;
		int    found = 0;
		long   def_val = 0, def_key = 0;
		// Находим значение по умолчанию
		for(i = 0; ValAssoc.enumItems(&i, (void **)&p_assoc);)
			if(p_assoc->Key == -1) {
				def_val = p_assoc->Val;
				break;
			}
		for(i = 0; !found && ValAssoc.enumItems(&i, (void **)&p_assoc);)
			if(p_assoc->Key != -1)
				if(p_assoc->Val == val) {
					v = (ushort)p_assoc->Key;
					found = 1;
				}
				else if(p_assoc->Val == def_val)
					def_key = p_assoc->Key;
		if(!found)
			v = (ushort)def_key;
	}
	else if(Kind == CHECKBOXES) {
		for(uint i = 0; ValAssoc.enumItems(&i, (void **)&p_assoc);)
			if(val & p_assoc->Val)
				v |= (ushort)(1 << p_assoc->Key);
	}
	TransmitData(+1, &v);
	return 1;
}

int TCluster::getDataAssoc(long * pVal)
{
	int    ok = 0;
	ushort v = 0;
	long   val = *pVal;
	TransmitData(-1, &v);
	if(Kind == RADIOBUTTONS) {
		if(ValAssoc.Search(static_cast<long>(v), &val, 0) || ValAssoc.Search(-1, &val, 0)) {
			*pVal = val;
			ok = 1;
		}
	}
	else if(Kind == CHECKBOXES) {
		for(int i = 0; i < 16; i++) {
			long temp_val = 0;
			if(ValAssoc.Search(i, &temp_val, 0))
				SETFLAG(val, temp_val, (v & (1 << i)));
		}
		*pVal = val;
		ok = 1;
	}
	return ok;
}
//
// ComboBox
//
#define OEMRESOURCE // OBM_COMBO definition

DECL_CMPFUNC(_PcharNoCase);

void ComboBox::Init(long flags)
{
	SubSign = TV_SUBSIGN_COMBOBOX;
	State = 0;
	State |= stUndef;
	NoDefID  = 0;
	SrchFunc = PTR_CMPFUNC(_PcharNoCase);
	P_Def = 0;
	Top = 0;
	Range = 0;
	P_ListWin = 0;
	hScrollBar = 0;
	Flags = flags;
}

ComboBox::ComboBox(const TRect & bounds, ListBoxDef * aDef) : TView(bounds)
{
	Init(0);
	setDef(aDef);
}

ComboBox::ComboBox(const TRect & bounds, ushort aFlags) : TView(bounds)
{
	Init(aFlags);
}

ComboBox::~ComboBox()
{
	RestoreOnDestruction();
	if(!P_ListWin || P_ListWin->P_Def != P_Def)
		delete P_Def;
	delete P_ListWin;
}

void ComboBox::setupListWindow(int noUpdateSize)
{
	if(P_ListWin) {
		HWND   h_box = P_ListWin->H();
		HWND   h_list = GetDlgItem(h_box, CTL_LBX_LIST);
		RECT   link_rect, list_rect;
		GetWindowRect(P_ILink->getHandle(), &link_rect);
		GetWindowRect(getHandle(), &list_rect);
		link_rect.right = list_rect.right;
		GetWindowRect(h_box, &list_rect);
		int    h = P_Def ? ((P_Def->ViewHight + 1) * ::SendMessage(h_list, LB_GETITEMHEIGHT, 0, 0)) : (list_rect.bottom - list_rect.top);
		int    screen_y = GetSystemMetrics(SM_CYFULLSCREEN);
		int    top = ((link_rect.bottom + h) < screen_y) ? link_rect.bottom : screen_y-h;
		MoveWindow(h_box, link_rect.left, top, (link_rect.right - link_rect.left), h, 1);
		GetClientRect(h_box, &list_rect);
		list_rect.right -= GetSystemMetrics(SM_CXVSCROLL);
		MoveWindow(h_list, 0, 0, list_rect.right, list_rect.bottom, 1);
		MoveWindow(GetDlgItem(h_box, MAKE_BUTTON_ID(CTL_LBX_LIST, 1)), list_rect.right, 0,
			GetSystemMetrics(SM_CXVSCROLL), list_rect.bottom, 1);
	}
}

void ComboBox::setupTreeListWindow(int noUpdateSize)
{
	if(P_ListWin) {
		RECT   rect;
		GetWindowRect(getHandle(), &rect);
		P_ListWin->MoveWindow(P_ILink->getHandle(), rect.right);
	}
}

int ComboBox::setListWindow(ListWindow * pListWin)
{
	int    ok = 1;
	if(pListWin) {
		setDef(pListWin->P_Def);
		//
		// @todo В этом месте необходимо принудительно завершить модальность
		// окна P_ListWin если оно активно и модально
		//
		if(P_ListWin && P_ListWin->IsInState(sfModal))
			P_ListWin = 0;
		else
			delete P_ListWin;
		P_ListWin = pListWin;
		P_ListWin->P_Lb->combo = this;
	}
	else
		ok = 0;
	return ok;
}

int ComboBox::setListWindow(ListWindow * pListWin, long dataVal)
{
	int    r = setListWindow(pListWin);
	TransmitData(+1, &dataVal);
	return r;
}

// static
LRESULT CALLBACK ComboBox::DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	ComboBox * p_view = static_cast<ComboBox *>(TView::GetWindowUserData(hWnd));
	switch(uMsg) {
		case WM_DESTROY: p_view->OnDestroy(hWnd); return 0;
		case WM_ENABLE:
			if(!lParam)
				::PostMessage(GetParent(hWnd), WM_CHAR, 0x9, 0);
			break;
		case WM_LBUTTONUP:
			if(p_view && p_view->P_ListWin/*&& !p_view->IsInState(sfMsgToParent)*/) {
				p_view->handleWindowsMessage(WM_COMMAND, 0, 0);
				return 0;
			}
			break;
		case WM_SYSKEYDOWN:
		case WM_KEYDOWN:
			if(!oneof2(wParam, VK_ESCAPE, VK_RETURN))
				p_view->SendToParent(hWnd, WM_USER_KEYDOWN, MAKELPARAM((WORD)wParam, 0), reinterpret_cast<LPARAM>(hWnd));
			break;
		case WM_ERASEBKGND:
			/*
			if(p_view && APPL->EraseBackground(p_view, hWnd, reinterpret_cast<HDC>(wParam), ODT_BUTTON) > 0)
				return 1;
			*/
			break;
	}
	return CallWindowProc(p_view->PrevWindowProc, hWnd, uMsg, wParam, lParam);
}

int ComboBox::handleWindowsMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg) {
		case WM_INITDIALOG:
			{
				long   prev_item_selected = 0;
				HWND   hcb  = getHandle();
				if(P_Def) {
					P_Def->TransmitData(-1, &prev_item_selected);
					setRange(P_Def->getRecsCount());
				}
				TView::SetWindowProp(hcb, GWLP_USERDATA, this);
				PrevWindowProc = static_cast<WNDPROC>(TView::SetWindowProp(hcb, GWLP_WNDPROC, ComboBox::DlgProc)); 
				{
					HBITMAP h_bm = APPL->FetchSystemBitmap(OBM_COMBO);
					::SendMessage(hcb, BM_SETIMAGE, IMAGE_BITMAP, reinterpret_cast<LPARAM>(h_bm));
				}
				SetWindowLong(hcb, GWL_STYLE, TView::GetWindowStyle(hcb) & ~WS_TABSTOP);
				if(P_ILink) {
					RECT r, cr;
					if(::GetWindowRect(P_ILink->getHandle(), &r)) {
						::GetWindowRect(hcb, &cr);
						POINT p;
						p.x = r.right;
						p.y = r.top;
						::ScreenToClient(Parent, &p);
						::MoveWindow(hcb, p.x, p.y, cr.right-cr.left, r.bottom-r.top, TRUE);
					}
				}
			}
			break;
		case WM_USER_COMBO_CLEAR:
			if(Flags & cbxAllowEmpty) {
				TransmitData(+1, 0);
				MessageCommandToOwner(cmCBSelected);
			}
			break;
		case WM_USER_COMBO_ACTIVATEBYCHAR:
			if(!P_ListWin)
				break;
			P_ListWin->prepareForSearching(static_cast<int>(wParam));
		case WM_COMMAND:
			{
				ListWindow * p_list_win = P_ListWin;
				if(!(State & stExecSemaphore) && p_list_win) {
					State |= stExecSemaphore;
					long   v = 0;
					int    res = cmCancel;
					HWND   w_link = GetDlgItem(Parent, link()->GetId());
					if(GetFocus() != w_link)
						SetFocus(w_link);
					if(!(State & stUndef))
						p_list_win->getResult(&v);
					else
						v = NoDefID;
					if((res = APPL->P_DeskTop->execView(p_list_win)) == cmOK)
						p_list_win->getResult(&v);
					//
					// Список P_ListWin был анулирован в функции ComboBox::setListWindow(ListWindow *)
					// Во избежании утечки памяти здесь необходимо разрушить оставшийся висячим p_list_win
					//
					if(p_list_win != P_ListWin)
						delete p_list_win;
					//
					TransmitData(+1, (v > 0) ? &v : 0);
					State &= ~stExecSemaphore;
					if(res == cmOK)
						MessageCommandToOwner(cmCBSelected);
				}
			}
			break;
	 }
	 return 1;
}

int ComboBox::search(const void * pPattern, CompFunc fcmp, int srchMode)
{
	int    ok = 0;
	if(P_Def && P_Def->search(pPattern, fcmp, srchMode)) {
		long   scroll_delta, scroll_pos;
		P_Def->getScrollData(&scroll_delta, &scroll_pos);
		SendDlgItemMessage(Parent, MAKE_BUTTON_ID(Id, 1), SBM_SETPOS, scroll_pos, 1);
		Draw_();
		ok = 1;
	}
	return ok;
}

void ComboBox::search(const char * pFirstLetter, int srchMode)
{
	int    r = -1;
	if((srchMode & ~srchFlags) == srchFirst) {
		if(pFirstLetter) {
			if(*pFirstLetter == 0x08) {
				size_t x = SearchPattern.Len();
				SearchPattern.Trim(x ? x-1 : 0);
			}
			else
				SearchPattern.Cat(pFirstLetter);
			if(SearchPattern.C(0) != '*' || SearchPattern.C(1) != 0)
				r = search(SearchPattern, SrchFunc, (SearchPattern.C(0) == '*') ? srchNext : srchFirst);
		}
	}
	else if((srchMode & ~srchFlags) == srchNext && SearchPattern.NotEmpty())
		r = search(SearchPattern, SrchFunc, srchNext);
	if(r >= 0)
		Draw_();
}

void FASTCALL ComboBox::setDef(ListBoxDef * pDef)
{
	if(pDef) {
		P_Def = pDef;
		setRange(P_Def->getRecsCount());
		State |= stUndef;
		Draw_();
	}
	else
		P_Def = 0;
}

void ComboBox::setUndefTag(int set)
{
	SETFLAG(State, stUndef, set);
}

void ComboBox::setUndefID(long undefID)
{
	NoDefID = undefID;
}

int ComboBox::setDataByUndefID()
{
	int    ok = -1;
	if(NoDefID) {
		TransmitData(+1, &NoDefID);
		ok = 1;
	}
	return ok;
}

int ComboBox::getInputLineText(char * pBuf, size_t bufLen)
{
	ASSIGN_PTR(pBuf, 0);
	// @v9.1.5 char   temp_buf[256];
	// @v9.1.5 SendDlgItemMessage(Parent, P_ILink->GetId(), WM_GETTEXT, sizeof(temp_buf)-1, (long)temp_buf);
	// @v9.1.5 (Text = temp_buf).Transf(CTRANSF_OUTER_TO_INNER).CopyTo(pBuf, bufLen);
	TView::SGetWindowText(GetDlgItem(Parent, P_ILink->GetId()), Text); // @v9.1.5
	Text.Transf(CTRANSF_OUTER_TO_INNER).CopyTo(pBuf, bufLen); // @v9.1.5
	return Text.NotEmpty() ? 1 : -1;
}

void ComboBox::setInputLineText(const char * pBuf)
{
	(Text = pBuf).Transf(CTRANSF_INNER_TO_OUTER);
	// @v9.1.5 SendDlgItemMessage(Parent, P_ILink->GetId(), WM_SETTEXT, 0, (long)(const char *)Text);
	TView::SSetWindowText(GetDlgItem(Parent, P_ILink->GetId()), Text); // @v9.1.5
}

int ComboBox::TransmitData(int dir, void * pData)
{
	int    s = 0;
	if(dir == 0) {
		s = P_Def ? P_Def->TransmitData(dir, pData) : 0;
	}
	else if(dir > 0) {
		if(P_Def == 0 && NoDefID) {
			NoDefID = pData ? *static_cast<long *>(pData) : 0;
			SETFLAG(State, stNoDefZero, !NoDefID);
		}
		else if(P_ListWin) {
			SmartListBox * p_lb = P_ListWin->listBox();
			p_lb->TransmitData(dir, pData);
			SETFLAG(State, stUndef, !(p_lb->State & SmartListBox::stDataFounded));
			if(P_ILink) {
				char   d[512];
				if(State & stUndef) {
					memzero(d, sizeof(d));
					// @v10.4.12 {
					/* @construction 
					if(!(p_lb->State & SmartListBox::stDataFounded)) {
						long temp_id = 0;
						if(p_lb->getCurID(&temp_id) && temp_id) {
							SString & r_temp_buf = SLS.AcquireRvlStr();
							r_temp_buf.CatEq("#NF", temp_id);
							STRNSCPY(d, r_temp_buf);
						}
					}*/
					// } @v10.4.12 
				}
				else
					P_ListWin->getListData(d);
				P_ILink->TransmitData(dir, d);
			}
		}
	}
	else if(dir < 0) {
		if(P_ListWin && !(State & stUndef) && P_ListWin->listBox()->def)
			P_ListWin->listBox()->TransmitData(dir, pData);
		else if(P_Def == 0 && NoDefID)
			*static_cast<long *>(pData) = NoDefID;
		else if(P_Def == 0 && State & stNoDefZero)
			*static_cast<long *>(pData) = 0;
		else {
			const int ds = TransmitData(0, 0); // @recursion
			if(ds == sizeof(short))
				*static_cast<short *>(pData) = 0;
			else if(ds == sizeof(long))
				*static_cast<long *>(pData) = 0;
			else if(ds)
				*static_cast<char *>(pData) = 0;
		}
	}
	return s;
}

IMPL_HANDLE_EVENT(ComboBox)
{
	if(event.isCmd(cmCBActivate)) {
		if(Parent) {
			HWND   h_link = GetDlgItem(Parent, P_ILink->GetId());
			const  uint preserve_state = Sf;
			setState(sfDisabled, false);
			SetFocus(h_link);
			Sf = preserve_state;
			EnableWindow(h_link, 1);
			PostMessage(h_link, WM_KEYDOWN, VK_DOWN, 0);
		}
		clearEvent(event);
	}
}

void ComboBox::selectItem(long)
{
	MessageCommandToOwner(cmLBItemSelected);
}

void ComboBox::setRange(long aRange)
{
	Range = aRange;
}

void ComboBox::setState(uint aState, bool enable)
{
	if(aState & sfDisabled)
		P_ILink->setState(sfDisabled, enable);
	if(aState & sfVisible)
		P_ILink->setState(sfVisible, enable);
	TView::setState(aState, enable);
	if(aState & (sfSelected | sfActive))
		Draw_();
}

int ComboBox::addItem(long id, const char * pS, long * pPos)
{
	int    r = -1;
	if(P_Def && (r = P_Def->addItem(id, pS, pPos)) > 0)
		setRange(P_Def->getRecsCount());
	return r;
}

int ComboBox::removeItem(long pos)
{
	int    r = -1;
	if(P_Def && (r = P_Def->removeItem(pos)) > 0)
		setRange(P_Def->getRecsCount());
	return r;
}

void ComboBox::freeAll()
{
	if(P_Def) {
		P_Def->freeAll();
		setRange(P_Def->getRecsCount());
	}
}

TInputLine * ComboBox::link() const
{
	return P_ILink;
}

void ComboBox::SetLink(TInputLine * pLink)
{
	P_ILink = pLink;
}
//
//
//
// static
LRESULT CALLBACK TImageView::DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	TImageView * p_view = static_cast<TImageView *>(TView::GetWindowUserData(hWnd));
	switch(uMsg) {
		case WM_DESTROY:
			if(p_view && p_view->IsSubSign(TV_SUBSIGN_IMAGEVIEW)) {
				p_view->OnDestroy(hWnd);
			}
			return 0;
		case WM_COMMAND:
			if(HIWORD(wParam) == 1)
				SendMessage(APPL->H_TopOfStack, uMsg, wParam, lParam);
			break;
		case WM_LBUTTONDBLCLK:
			CALLPTRMEMB(p_view, MessageCommandToOwner(cmImageDblClk));
			break;
		case WM_PAINT:
			if(p_view && p_view->IsSubSign(TV_SUBSIGN_IMAGEVIEW)) {
				PAINTSTRUCT ps;
				::BeginPaint(hWnd, &ps);
				//p_view->draw();
				if(p_view->P_Fig) {
					RECT rc;
					::GetClientRect(hWnd, &rc);
					const  TRect rect_elem_i = rc;
					const  FRect rect_elem = rc;
					FRect pic_bounds = rect_elem;
					APPL->InitUiToolBox();
					SPaintToolBox & r_tb = APPL->GetUiToolBox();
					TCanvas2 canv(r_tb, ps.hdc);
					LMatrix2D mtx;
					SViewPort vp;
					// @v9.6.5 {
					if(!p_view->ReplacedColor.IsEmpty()) {
						SColor replacement_color;
						replacement_color = r_tb.GetColor(TProgram::tbiIconRegColor);
						canv.SetColorReplacement(p_view->ReplacedColor, replacement_color);
					}
					// } @v9.6.5
					canv.PushTransform();
					p_view->P_Fig->GetViewPort(&vp);
					{
                        pic_bounds.a.X = 0.0f;
                        pic_bounds.a.Y = 0.0f;
						if(vp.GetSize().X <= rect_elem.Width() && vp.GetSize().Y <= rect_elem.Height()) {
							pic_bounds.b.X = vp.GetSize().X;
							pic_bounds.b.Y = vp.GetSize().Y;
							pic_bounds.MoveCenterTo(rect_elem.GetCenter());
						}
						else {

						}
					}
					canv.AddTransform(vp.GetMatrix(pic_bounds, mtx));
					canv.Draw(p_view->P_Fig);
					canv.PopTransform();
				}
#ifndef TIMAGEVIEW_USE_FIG
				else if(p_view->P_Image_GDIP) {
					static_cast<SImage *>(p_view->P_Image_GDIP)->Draw(hWnd, 0);
				}
#endif
				::EndPaint(hWnd, &ps);
			}
			return 0;
		case WM_SETFOCUS:
		case WM_KILLFOCUS:
			TDialog::DialogProc(GetParent(hWnd), uMsg, wParam, reinterpret_cast<LPARAM>(hWnd));
			break;
	}
	return CallWindowProc(p_view->PrevWindowProc, hWnd, uMsg, wParam, lParam);
}

int TImageView::handleWindowsMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if(uMsg == WM_INITDIALOG) {
		HWND   h_wnd = getHandle();
		TView::SetWindowUserData(h_wnd, this);
		PrevWindowProc = static_cast<WNDPROC>(TView::SetWindowProp(h_wnd, GWLP_WNDPROC, TImageView::DlgProc));
	 }
	 return 1;
}

TImageView::TImageView(const TRect & rBounds, const char * pFigSymb) : TView(rBounds), P_Fig(0), FigSymb(pFigSymb)
#ifndef TIMAGEVIEW_USE_FIG
	, P_Image_GDIP(0)
#endif
{
	SubSign = TV_SUBSIGN_IMAGEVIEW; // @v8.3.11
	ReplacedColor.Set(0); // @v9.6.5
	ReplacedColor.Alpha = 0; // @v9.6.5
	if(FigSymb.NotEmpty()) {
		TWhatmanToolArray::Item tool_item;
		const SDrawFigure * p_fig = APPL->LoadDrawFigureBySymb(FigSymb, &tool_item);
		if(p_fig) {
			P_Fig = p_fig->Dup();
			ReplacedColor = tool_item.ReplacedColor;
		}
	}
	else {
#ifndef TIMAGEVIEW_USE_FIG
		P_Image = new SImage;
		((SImage*)P_Image)->Init();
#endif
	}
}

TImageView::~TImageView()
{
	delete P_Fig;
#ifndef TIMAGEVIEW_USE_FIG
	if(P_Image_GDIP) {
		delete (SImage *)P_Image_GDIP;
		P_Image_GDIP = 0;
	}
#endif
	RestoreOnDestruction();
}

int TImageView::TransmitData(int dir, void * pData)
{
	int    s = 0;
	if(dir > 0) {
		const char * p_path = static_cast<const char *>(pData);
		{
			HWND hw = getHandle();
#ifdef TIMAGEVIEW_USE_FIG
			DELETEANDASSIGN(P_Fig, SDrawFigure::CreateFromFile(p_path, 0));
#else
			((SImage*)P_Image)->LoadImage(p_path);
#endif
			::InvalidateRect(hw, 0, /*erase=*/TRUE);
			::UpdateWindow(hw);
		}
	}
	return s;
}

/*void TImageView::draw()
{
#ifdef TIMAGEVIEW_USE_FIG
#else
	((SImage*)P_Image)->Draw(getHandle(), 0);
#endif
	TView::draw();
}*/
//
//
//
TToolTip::ToolItem::ToolItem() : Id(0), H(0), Param(0)
{
}

TToolTip::TToolTip(HWND hParent, uint maxWidthPix) : H(0), MaxWidthPix(maxWidthPix)
{
	if(hParent)
		Create(hParent);
}

TToolTip::~TToolTip()
{
	if(H)
		::DestroyWindow(H);
}

int TToolTip::Create(HWND hParent)
{
	H = ::CreateWindowEx(WS_EX_TOPMOST, TOOLTIPS_CLASS, NULL, WS_POPUP|TTS_NOPREFIX|TTS_ALWAYSTIP,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, hParent, NULL, SLS.GetHInst(), 0);
	if(H) {
		SetWindowPos(H, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);
		if(MaxWidthPix)
			::SendMessage(H, TTM_SETMAXTIPWIDTH, 0, MaxWidthPix);
		::SendMessage(H, TTM_SETTIPBKCOLOR, GetColorRef(SClrYellow), 0);
	}
	return BIN(H);
}

int TToolTip::AddTool(ToolItem & rItem)
{
	int    ok = 1;
	if(H) {
		const uint _cur_count = GetToolsCount();
		TOOLINFO ti;
		MEMSZERO(ti);
		ti.cbSize = sizeof(TOOLINFO);
		ti.uFlags = TTF_SUBCLASS;
		ti.hwnd = rItem.H;
		ti.hinst = SLS.GetHInst();
		ti.uId = _cur_count+1; //++Counter;
		if(rItem.Text.NotEmpty()) {
			ti.lpszText = const_cast<TCHAR *>(SUcSwitch(rItem.Text)); // @badcast // @unicodeproblem
		}
		if(rItem.R.IsEmpty() && rItem.H) {
			GetClientRect(rItem.H, &ti.rect);
		}
		else {
			ti.rect = static_cast<RECT>(rItem.R);
		}
		ti.lParam = rItem.Param;
		::SendMessage(H, TTM_ADDTOOL, 0, reinterpret_cast<LPARAM>(&ti)); // @unicodeproblem
	}
	else
		ok = 0;
	return ok;
}

uint TToolTip::GetToolsCount()
{
	uint   c = 0;
	if(H)
		c = static_cast<uint>(::SendMessage(H, TTM_GETTOOLCOUNT, 0, 0));
	return c;
}

int TToolTip::GetTool(uint idx, ToolItem & rItem)
{
	// @v9.8.0 Сообщение TTM_ENUMTOOLS почему-то не работает (я перепроверил максимум, пробовал явный вызов TTM_ENUMTOOLSW,  
	// пробовал увеличивать значение, передваемое в TOOLINFO::cbSize - не помогает. Функция всегда возвращает 0
	int    ok = 0;
	if(H) {
		STempBuffer text_buf(2048);
		TOOLINFO ti;
		MEMSZERO(ti);
		ti.lpszText = static_cast<TCHAR *>(text_buf.vptr()); // @unicodeproblem
		ti.cbSize = sizeof(TOOLINFO);
		if(::SendMessage(H, TTM_ENUMTOOLS, idx, reinterpret_cast<LPARAM>(&ti))) { // @unicodeproblem
			rItem.Id = ti.uId;
			rItem.H = ti.hwnd;
			rItem.Param = ti.lParam;
			rItem.R = ti.rect;
			rItem.Text = SUcSwitch(static_cast<TCHAR *>(text_buf.vptr()));
			ok = 1;
		}
	}
	return ok;
}

int TToolTip::RemoveTool(uint idx)
{
	int    ok = 0;
	if(H) {
		ToolItem item;
		/*if(GetTool(idx, item))*/ { // @v9.8.0 С WIN-функцией TTM_ENUMTOOLS что-то не так
			TOOLINFO ti;
			MEMSZERO(ti);
			ti.cbSize = sizeof(TOOLINFO);
			ti.hwnd = item.H;
			ti.uId = idx+1; //item.Id;
			::SendMessage(H, TTM_DELTOOL, 0, reinterpret_cast<LPARAM>(&ti));
			ok = 1;
		}
	}
	return ok;
}

int TToolTip::RemoveAllTools()
{
	//
	// @v9.8.0 Не понятно: не работают сообщения WINDOWS TTM_DELTOOL и TTM_ENUMTOOLS. 
	// Пришлось очищать все элементы через колено: удалять и создавать окно снова.
	//
	int    ok = 0;
	if(H) {
		HWND h_parent = GetParent(H);
		if(h_parent) {
			::DestroyWindow(H);
			H = 0;
			ok = Create(h_parent);
		}
		/*
		uint c = GetToolsCount();
		for(uint i = 0; i < c; i++) {
			RemoveTool(i);
		}
		uint test_c = GetToolsCount(); // @debug
		*/
		ok = 1;
	}
	return ok;
}
