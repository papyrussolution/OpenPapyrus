// STOOLTI.CPP
// Copyright (c) A.Starodub, A.Sobolev 2008, 2009, 2010, 2011, 2016, 2017, 2018, 2019, 2020, 2021, 2023, 2025, 2026
// @codepage UTF-8
//
#include <slib-internal.h>
#pragma hdrstop

STooltip::STooltip() : HwndTT(0), Parent(0)
{
}

STooltip::~STooltip()
{
	Destroy();
}

int STooltip::Init(HWND parent)
{
	Destroy();
	Parent = parent;
	HwndTT = ::CreateWindowExW(WS_EX_TOPMOST, TOOLTIPS_CLASS, NULL, WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, parent, NULL, TProgram::GetInst(), 0);
	SetWindowPos(HwndTT, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE | SWP_NOACTIVATE);
	return BIN(HwndTT);
}

void STooltip::Destroy()
{
	::DestroyWindow(HwndTT);
	HwndTT = 0;
	Parent = 0;
}

int STooltip::Add(const char * pText, const RECT * pRect, long id)
{
	assert(pRect);
	if(pRect) {
		TCHAR  tooltip[256];
		memzero(tooltip, sizeof(tooltip));
		strnzcpy(tooltip, SUcSwitch(pText), SIZEOFARRAY(tooltip));
		TOOLINFO ti;
		INITWINAPISTRUCT(ti);
		ti.uFlags   = TTF_SUBCLASS;
		ti.hwnd     = Parent;
		ti.uId      = id;
		ti.rect     = *pRect;
		ti.hinst    = TProgram::GetInst();
		ti.lpszText = tooltip;
		return BIN(::SendMessageW(HwndTT, TTM_ADDTOOL, 0, reinterpret_cast<LPARAM>(&ti)));
	}
	else
		return 0;
}

int STooltip::Remove(long id)
{
	TOOLINFO ti;
	INITWINAPISTRUCT(ti);
	ti.uFlags = TTF_SUBCLASS;
	ti.hwnd   = Parent;
	ti.uId    = id;
	ti.hinst  = TProgram::GetInst();
	return BIN(::SendMessageW(HwndTT, (UINT)TTM_DELTOOL, 0, reinterpret_cast<LPARAM>(&ti)));
}

#define MSGWND_CLOSETIMER 1L

SMessageWindow::SMessageWindow() : HWnd(0), Cmd(0), Extra(0), Brush(0), Font(0), P_Image(0), PrevImgProc(0), Flags(0)
{
	PrevMouseCoord.x = 0;
	PrevMouseCoord.y = 0;
}

SMessageWindow::~SMessageWindow()
{
	Destroy();
}

/*
struct FindWindowStruc {
	HWND   Parent;
	HWND   FoundHwnd;
	long   ID;
};

BOOL CALLBACK FindWindowByID(HWND hwnd, LPARAM lParam)
{
	BOOL ok = TRUE;
	FindWindowStruc * p_struc = (FindWindowStruc*)lParam;
	if(p_struc && p_struc->ID != 0) {
		HWND parent = GetParent(hwnd);
		long id = GetWindowLong(hwnd, DWL_USER);
		if(parent == p_struc->Parent && p_struc->ID == id) {
			p_struc->FoundHwnd = hwnd;
			ok = FALSE;
		}
	}
	else
		ok = FALSE;
	return ok;
}
*/

static BOOL CALLBACK CloseTooltipWnd(HWND hwnd, LPARAM lParam)
{
	if(!lParam || ::GetParent(hwnd) == reinterpret_cast<HWND>(lParam))
		::SendMessageW(hwnd, WM_USER_CLOSE_TOOLTIPMSGWIN, 0, 0);
	return TRUE;
}

static BOOL CALLBACK CloseTooltipWnd2(HWND hwnd, LPARAM lParam)
{
	::SendMessageW(hwnd, WM_USER_CLOSE_TOOLTIPMSGWIN, 0, 0);
	return TRUE;
}

/*static*/void FASTCALL SMessageWindow::DestroyByParent(HWND parent)
{
	::EnumWindows(CloseTooltipWnd, reinterpret_cast<LPARAM>(parent));
	::EnumChildWindows(parent, CloseTooltipWnd2, 0);
}

/*static*/LRESULT CALLBACK ImgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	SMessageWindow * p_wnd = static_cast<SMessageWindow *>(TView::GetWindowUserData(hWnd));
	switch(uMsg) {
		case WM_PAINT:
			{
				PAINTSTRUCT ps;
				::BeginPaint(hWnd, &ps);
				SDrawFigure * p_fig = static_cast<SDrawFigure *>(p_wnd->GetImage());
				if(p_fig) {
					RECT rc;
					::GetClientRect(hWnd, &rc);
					const  TRect rect_elem_i(rc);
					const  FRect rect_elem = rc;
					FRect pic_bounds = rect_elem;
					APPL->InitUiToolBox();
					SPaintToolBox * p_tb = APPL->GetUiToolBox();
					if(p_tb) {
						TCanvas2 canv(*p_tb, ps.hdc);
						LMatrix2D mtx;
						SViewPort vp;
						canv.PushTransform();
						p_fig->GetViewPort(&vp);
						{
							pic_bounds.a.SetZero();
							if(vp.GetSize().x <= rect_elem.Width() && vp.GetSize().y <= rect_elem.Height()) {
								pic_bounds.b = vp.GetSize();
								pic_bounds.MoveCenterTo(rect_elem.GetCenter());
							}
							else {
								;
							}
						}
						canv.AddTransform(vp.GetMatrix(pic_bounds, mtx));
						canv.Draw(p_fig);
						canv.PopTransform();
					}
				}
				::EndPaint(hWnd, &ps);
			}
			return 0;
	}
	return CallWindowProc(p_wnd->PrevImgProc, hWnd, uMsg, wParam, lParam);
}

int SMessageWindow::SetFont(HWND hCtl)
{
	if(hCtl) {
		const  UiDescription * p_uid = SLS.GetUiDescription();
		LOGFONTW log_font;
		MEMSZERO(log_font);
		// @v12.7.1 {
		if(Flags & SMessageWindow::fLargeText) {
			const  SFontDescr * p_fd = p_uid ? p_uid->GetFontDescrC("PopUpMsgWin_Large") : 0;
			if(p_fd && p_fd->MakeLogFont(&log_font)) {
				log_font.lfHeight = abs(log_font.lfHeight);
			}
			else {
				log_font.lfCharSet = RUSSIAN_CHARSET;
				STRNSCPY(log_font.lfFaceName, L"MS Shell Dlg");
				log_font.lfHeight = 26;
				log_font.lfWeight = FW_HEAVY;
			}
		}
		else {
			const  SFontDescr * p_fd = p_uid ? p_uid->GetFontDescrC("PopUpMsgWin_Normal") : 0;
			if(p_fd && p_fd->MakeLogFont(&log_font)) {
				log_font.lfHeight = abs(log_font.lfHeight);
			}
			else {
				log_font.lfCharSet = RUSSIAN_CHARSET;
				STRNSCPY(log_font.lfFaceName, L"MS Shell Dlg");
				log_font.lfHeight = 13;
				log_font.lfWeight = FW_MEDIUM;
			}
		}
		// } @v12.7.1 
		/* @v12.7.1
		log_font.lfCharSet = RUSSIAN_CHARSET;
		STRNSCPY(log_font.lfFaceName, L"MS Shell Dlg");
		log_font.lfHeight = (Flags & SMessageWindow::fLargeText) ? 26 : 13;
		log_font.lfWeight = (Flags & SMessageWindow::fLargeText) ? FW_HEAVY : FW_MEDIUM;
		*/
		ZDeleteWinGdiObject(&Font);
		Font = ::CreateFontIndirectW(&log_font);
		if(Font) {
			::SendMessageW(hCtl, WM_SETFONT, reinterpret_cast<WPARAM>(Font), TRUE);
		}
	}
	return 1;
}
/*
	popupmsgwin_transparency        [75] // 0..100 прозрачность всплывающего окна сообщений
	popupmsgwin_deftimerms          [60000] // время, в течении которого всплывающее окно сообщений висит на экране (в миллисекундах)
	popupmsgwin_maxwidthtoparentrel [2] // Максимальное отношение ширины окна к ширине родительского окна
	popupmsgwin_maxwraplines        [10] // Максимальное количество выводимых строк, на которое разбивается длинная строка

	papyrus_style/popupmsgwin_bg defcolor RGB(0xFF, 0xF7, 0x94)
*/ 
int SMessageWindow::Open(SString & rText, const char * pImgPath, HWND parent, long cmd, long timer, COLORREF color, long flags, long extra)
{
	int    ok = 0;
	int    font_init = 0;
	HWND   h_focus = ::GetFocus();
	HWND   hwnd_parent = NZOR(parent, APPL->H_MainWnd);
	const  UiDescription * p_uid = SLS.GetUiDescription();
	/* @construction if(parent)
		hwnd_parent = parent;
	else if(APPL->H_TopOfStack)
		hwnd_parent = APPL->H_TopOfStack;
	else
		hwnd_parent = APPL->H_MainWnd; */
	Destroy();
	Color   = color;
	Flags   = flags;
	Text    = rText;
	ImgPath = pImgPath;
	Cmd     = cmd;
	Extra   = extra;
	SMessageWindow::DestroyByParent(hwnd_parent);
	HWnd = APPL->CreateDlg(1013/*DLG_TOOLTIP*/, hwnd_parent, SMessageWindow::Proc, reinterpret_cast<LPARAM>(this));
	::GetCursorPos(&PrevMouseCoord);
	if(HWnd) {
		HWND   h_ctl = ::GetDlgItem(HWnd, 1201/*CTL_TOOLTIP_TEXT*/);
		HWND   h_img = ::GetDlgItem(HWnd, 1202/*CTL_TOOLTIP_IMAGE*/);
		double img_height = 0.0;
		double img_width = 0.0;
		if(Text.Len() == 0) {
			::DestroyWindow(h_ctl);
			h_ctl = 0;
		}
		if(ImgPath.Len() == 0) {
			::DestroyWindow(h_img);
			h_img = 0;
		}
		else {
			SDrawFigure * p_fig = SDrawFigure::CreateFromFile(ImgPath, 0);
			P_Image = p_fig;
			if(p_fig) {
				SViewPort vp;
				p_fig->GetViewPort(&vp);
				img_height = vp.Height();
				img_width = vp.Width();
				TView::SetWindowProp(h_img, GWLP_USERDATA, this);
				PrevImgProc = static_cast<WNDPROC>(TView::SetWindowProp(h_img, GWLP_WNDPROC, ImgProc));
			}
		}
		if(h_ctl) {
			if(Flags & SMessageWindow::fTextAlignLeft) {
				//
				// @v12.7.1 Как я (или кто-то другой) мог сделать такое (закомментированное) уродство?
				// 
				long   style = TView::SGetWindowStyle(h_ctl);
				::SetWindowLongW(h_ctl, GWL_STYLE, ((style|SS_LEFT)&~SS_CENTER)); // @v12.7.1 
				/* @v12.7.1 
				RECT   ctl_rect;
				RECT   img_rect;
				RECT   parent_rect;
				::GetWindowRect(h_ctl, &ctl_rect);
				if(h_img)
					::GetWindowRect(h_img, &img_rect);
				else
					MEMSZERO(img_rect);
				::GetWindowRect(HWnd, &parent_rect);
				ctl_rect.bottom -= ctl_rect.top;
				ctl_rect.right  -= ctl_rect.left;
				ctl_rect.left   -= parent_rect.left;
				ctl_rect.top    -= (parent_rect.top + img_rect.top);
				DestroyWindow(h_ctl);
				style &= ~SS_CENTER;
				h_ctl = ::CreateWindowExW(0, L"STATIC", L"", style|SS_LEFT, 
					ctl_rect.left, ctl_rect.top, ctl_rect.right, ctl_rect.bottom, HWnd, 0, TProgram::GetInst(), 0);
				if(h_ctl) {
					SetFont(h_ctl);
					::SetWindowLongW(h_ctl, GWL_ID, 1201); // CTL_TOOLTIP_TEXT==1201
					font_init = 1;
				}
				*/
			}
		}
		if(!font_init)
			SetFont(h_ctl);
		if(!(Flags & SMessageWindow::fOpaque)) {
			int   transp = 75; // default value = 75
			// @v12.7.1 {
			if(p_uid) {
				int    uid_transp = 0;
				if(p_uid->VList.Get(UiValueList::vPopUpMsgWinTransparency, uid_transp) && checkirange(uid_transp, 0, 100)) {
					transp = uid_transp;
				}
			}
			// } @v12.7.1 
			SetWindowTransparent(HWnd, transp);
		}
		if(Flags & SMessageWindow::fChildWindow) {
			const  long win_flags = TView::SGetWindowStyle(HWnd);
			::SetWindowLongW(HWnd, GWL_STYLE, ((win_flags|WS_CHILD)&~WS_POPUP));
			::SetWindowLongW(HWnd, GWL_EXSTYLE, 0L);
			::SetParent(HWnd, hwnd_parent);
		}
		{
			if(!Color) {
				SColor def_bg_color(RGB(0xFF, 0xF7, 0x94));	
				const SColorSet * p_cs = p_uid ? p_uid->GetColorSetC("papyrus_style") : 0;
				SColor sc = UiDescription::GetColorR(p_uid, p_cs, "popupmsgwin_bg", def_bg_color);
				Color = static_cast<COLORREF>(sc);
			}
			Brush = ::CreateSolidBrush(Color);
		}
		Text.ReplaceChar('\003', ' ').Strip();
		if(Flags & SMessageWindow::fUtf8) {
			Text.Transf(CTRANSF_UTF8_TO_OUTER);
		}
		else {
			Text.Transf(CTRANSF_INNER_TO_OUTER);
		}
		Move();
		::ShowWindow(HWnd, SW_SHOWNORMAL);
		::UpdateWindow(HWnd);
		{
			uint   elapse = checkirange(timer, 100L, 3600L*1000L) ? static_cast<uint>(timer) : 0;
			if(!elapse) {
				elapse = 60000U;
				// @v12.7.1 {
				if(p_uid) {
					int    uid_timer = 0;
					if(p_uid->VList.Get(UiValueList::vPopUpMsgWinDefTimerMs, uid_timer) && checkirange(uid_timer, 100, 3600*1000)) {
						elapse = static_cast<uint>(uid_timer);
					}
				}
				// } @v12.7.1
			}
			::SetTimer(HWnd, MSGWND_CLOSETIMER, elapse, static_cast<TIMERPROC>(0));
		}
		// SetCapture(HWnd);
		ok = 1;
	}
	if(Flags & fPreserveFocus && h_focus)
		::SetFocus(h_focus);
	return ok;
}

void SMessageWindow::Destroy()
{
	if(P_Image) {
		HWND h_img = GetDlgItem(HWnd, 1202/*CTL_TOOLTIP_IMAGE*/);
		TView::SetWindowProp(h_img, GWLP_WNDPROC, PrevImgProc);
		delete static_cast<SDrawFigure *>(P_Image);
	}
	HWnd  = 0;
	Text.Z();
	Cmd   = 0;
	Extra = 0;
	ZDeleteWinGdiObject(&Brush);
	ZDeleteWinGdiObject(&Font);
}

void SMessageWindow::Move()
{
	if(HWnd) {
		const  UiDescription * p_uid = SLS.GetUiDescription();
		RECT   toolt_rect;
		RECT   parent_rect;
		RECT   img_rect;
		HWND   h_ctl = GetDlgItem(HWnd, 1201/*CTL_TOOLTIP_TEXT*/);
		HWND   h_img = GetDlgItem(HWnd, 1202/*CTL_TOOLTIP_IMAGE*/);
		::GetWindowRect(HWnd, &toolt_rect);
		::GetWindowRect(GetParent(HWnd), &parent_rect);
		::GetWindowRect(h_img, &img_rect);
		int    top_delta = h_img ? (img_rect.bottom - img_rect.top) : 0;
		int    toolt_h = toolt_rect.bottom - toolt_rect.top;
		int    toolt_w = toolt_rect.right  - toolt_rect.left;
		if(h_img == 0) {
			toolt_h = 100;
			toolt_w = 100;
			if(Flags & SMessageWindow::fSizeByText) {
				double max_width_to_parent_rel = 2.0; // default=2.0 // @v12.3.10 (/5)-->(/4) // @v12.7.1 (/4)-->(/2)
				// @v12.7.1 {
				if(p_uid) {
					double uid_max_width_to_parent_rel = 0.0;
					if(p_uid->VList.Get(UiValueList::vPopUpMsgWinMaxWidthToParentRel, uid_max_width_to_parent_rel)) {
						if(uid_max_width_to_parent_rel > 0.0 && uid_max_width_to_parent_rel < 100.0) {
							max_width_to_parent_rel = uid_max_width_to_parent_rel;
						}
					}
				}
				// } @v12.7.1
				const  int  max_w = static_cast<int>((parent_rect.right - parent_rect.left) / max_width_to_parent_rel); 
				int    w = 0;
				int    h = 0;
				HDC    hdc = GetDC(h_ctl);
				RECT   ctl_rect;
				SString buf;
				SString buf2;
				StringSet ss('\n', Text);
				Text.Z();
				if(Font) {
					SelectObject(hdc, Font);
				}
				int   max_wrap_lines = 10; // // максимум строчек для 1-ой подстроки // default=10
				// @v12.7.1 {
				if(p_uid) {
					int    uid_max_wrap_lines = 0;
					if(p_uid->VList.Get(UiValueList::vPopUpMsgWinMaxWrapLines, uid_max_wrap_lines) && checkirange(uid_max_wrap_lines, 1, 1000)) {
						max_wrap_lines = uid_max_wrap_lines;
					}
				}
				// } @v12.7.1
				for(uint i = 0; ss.get(&i, buf);) {
					SIZE size;
					if(buf.Len() == 0)
						buf.Space();
					::GetTextExtentPoint32W(hdc, SUcSwitchW(buf), buf.LenI(), &size);
					w = MAX(w, size.cx);
					if(w > max_w) {
						SplitBuf(hdc, buf, max_w, max_wrap_lines); 
						StringSet ss2('\n', buf);
						uint   j = 0;
						uint   k = 0;
						buf.Z();
						while(ss2.get(&j, buf2) && buf2.Len()) {
							buf.Cat(buf2).CR();
							k++;
						}
						h += size.cy * k;
						w = max_w;
					}
					else {
						h += size.cy;
						buf.CR();
					}
					Text.Cat(buf);
				}
				toolt_h = h + 16; 
				toolt_w = w + 16; 
				ctl_rect.left = 5;
				ctl_rect.top  = 5 + top_delta;
				ctl_rect.right  = toolt_w -10;
				ctl_rect.bottom = toolt_h -10;
				::MoveWindow(h_ctl, ctl_rect.left, ctl_rect.top, ctl_rect.right, ctl_rect.bottom, FALSE);
				::ReleaseDC(h_ctl, hdc);
			}
			else {
				RECT ctl_rect;
				toolt_h = 100;
				toolt_w = 200;
				ctl_rect.top    = 5;
				ctl_rect.left   = 5;
				ctl_rect.bottom = toolt_h -10;
				ctl_rect.right  = toolt_w -10;
				::MoveWindow(h_ctl, ctl_rect.left, ctl_rect.top, ctl_rect.right, ctl_rect.bottom, FALSE);
			}
		}
		else if(h_ctl == 0) {
			toolt_h = img_rect.bottom - img_rect.top + 20;
		}
		if(Flags & SMessageWindow::fShowOnCenter) {
			toolt_rect.top  = parent_rect.top  + (parent_rect.bottom - parent_rect.top)  / 2 - toolt_h / 2;
			toolt_rect.left = parent_rect.left + (parent_rect.right  - parent_rect.left) / 2 - toolt_w / 2;
		}
		else if(Flags & SMessageWindow::fShowOnCursor) {
			//int    delta = GetSystemMetrics(SM_CXVSCROLL) + GetSystemMetrics(SM_CXBORDER);
			POINT  p;
			POINT  p_;
			::GetCursorPos(&p);
			// @v12.7.5 {
			p_ = p;
			if(::ScreenToClient(GetParent(HWnd), &p_)) { 
				p.x = p_.x;
			}
			// } @v12.7.5 
			toolt_rect.top  = p.y - toolt_h + 2;
			toolt_rect.left = p.x - 2;
			toolt_rect.top  = (toolt_rect.top  < parent_rect.top)  ? parent_rect.top  + 1 : toolt_rect.top;
			toolt_rect.left = (toolt_rect.left < parent_rect.left) ? parent_rect.left + 1 : toolt_rect.left;
			toolt_rect.top  = (toolt_rect.top + toolt_h  > parent_rect.bottom) ? (parent_rect.bottom - toolt_h - 1) : toolt_rect.top;
			toolt_rect.left = (toolt_rect.left + toolt_w > parent_rect.right)  ? (parent_rect.right - toolt_w + 2) : toolt_rect.left;

			toolt_rect.top  += 8;
			toolt_rect.left -= 8;
		}
		else if(Flags & SMessageWindow::fShowOnRUCorner) {
			toolt_rect.bottom = parent_rect.top + toolt_h + 64 + 128;
			toolt_rect.left   = parent_rect.right - toolt_w - 32;
		}
		else {
			toolt_rect.top  = parent_rect.bottom - toolt_h - 64;
			toolt_rect.left = parent_rect.right  - toolt_w - 32;
		}
		if(Flags & SMessageWindow::fChildWindow) {
			// toolt_rect.left -= parent_rect.left;
			toolt_rect.top  -= parent_rect.top;
		}
		::SetWindowPos(HWnd, (Flags & fTopmost) ? HWND_TOP : 0, toolt_rect.left, toolt_rect.top, toolt_w, toolt_h, (Flags & fTopmost) ? 0 : SWP_NOZORDER);
		TView::SSetWindowText(h_ctl, Text);
	}
}

int SMessageWindow::DoCommand(SPoint2S p)
{
	int    ok = -1;
	/*
	if(Cmd) {
		static_cast<PPApp *>(APPL)->processCommand(Cmd);
		ok = 1;
	}
	*/
	return ok;
}

/*static*/INT_PTR CALLBACK SMessageWindow::Proc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	SMessageWindow * p_win = static_cast<SMessageWindow *>(TView::GetWindowUserData(hWnd));
	switch(message) {
		case WM_INITDIALOG:
			SetWindowLong(hWnd, GWLP_USERDATA, static_cast<LONG>(lParam));
			SetWindowLong(hWnd, DWLP_USER, 1013/*DLG_TOOLTIP*/);
			break;
		case WM_DESTROY:
			KillTimer(hWnd, MSGWND_CLOSETIMER);
			TView::SetWindowProp(hWnd, GWLP_USERDATA, static_cast<void *>(0));
			ZDELETE(p_win);
			break;
		case WM_LBUTTONDBLCLK:
			if(p_win) {
				SPoint2S p;
				p_win->DoCommand(p.setwparam(static_cast<uint32>(lParam)));
				::DestroyWindow(hWnd);
			}
			break;
		case WM_RBUTTONDOWN:
			if(p_win) {
				SString menu_text;
				TMenuPopup menu;
				uint   cmd = 0;
				SLS.LoadString_("close", menu_text);
				menu.Add(menu_text.Transf(CTRANSF_INNER_TO_OUTER), cmaDelete);
				if(menu.Execute(hWnd, TMenuPopup::efRet, &cmd, 0) && cmd == cmaDelete)
					::DestroyWindow(hWnd);
			}
			break;
		case WM_CTLCOLORSTATIC:
		case WM_CTLCOLORDLG:
			if(p_win) {
				HDC    hdc = reinterpret_cast<HDC>(wParam);
				TCanvas canv(hdc);
				COLORREF text_color = (labs(p_win->Color - SClrBlack) > labs(p_win->Color - SClrWhite)) ? SClrBlack : SClrWhite;
				canv.SetTextColor(text_color);
				SetBkMode(hdc, TRANSPARENT);
				return reinterpret_cast<INT_PTR>(p_win->Brush);
			}
			break;
		case WM_USER_MAINWND_MOVE_SIZE:
			CALLPTRMEMB(p_win, Move());
			break;
		case WM_USER_CLOSE_TOOLTIPMSGWIN:
			::DestroyWindow(hWnd);
			break;
		case WM_TIMER:
			if(wParam == MSGWND_CLOSETIMER) {
				::DestroyWindow(hWnd);
				return 0;
			}
			break;
		 case WM_MOUSEMOVE:
			if(p_win->Flags & SMessageWindow::fCloseOnMouseLeave) {
				POINT pnt;
				::GetCursorPos(&pnt);
				if(p_win->PrevMouseCoord.x != pnt.x || p_win->PrevMouseCoord.y != pnt.y)
					::DestroyWindow(hWnd);
				else {
					p_win->PrevMouseCoord = pnt;
					//
					TRACKMOUSEEVENT tme;
					INITWINAPISTRUCT(tme);
					tme.dwFlags     = TME_LEAVE;
					tme.hwndTrack   = hWnd;
					::_TrackMouseEvent(&tme);
				}
			}
			return 0;
		 case WM_MOUSELEAVE:
			if(p_win && (p_win->Flags & SMessageWindow::fCloseOnMouseLeave))
				DestroyWindow(hWnd);
			break;
		default:
			break;
	}
	return FALSE;
}
