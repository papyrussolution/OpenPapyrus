// STOOLTI.CPP
// Copyright (c) A.Starodub 2008, 2009, 2010, 2011, 2016, 2017, 2018, 2019, 2020
//
#include <slib.h>
#include <tv.h>
#pragma hdrstop

#define STOOLTIP_USE_FIG

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
	HwndTT = CreateWindowEx(WS_EX_TOPMOST, TOOLTIPS_CLASS, NULL, WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, parent, NULL, TProgram::GetInst(), 0);
	SetWindowPos(HwndTT, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
	return BIN(HwndTT);
}

void STooltip::Destroy()
{
	if(HwndTT)
		::DestroyWindow(HwndTT);
	HwndTT = 0;
	Parent = 0;
}

int STooltip::Add(const char * pText, const RECT * pRect, long id)
{
	TCHAR  tooltip[256];
	memzero(tooltip, sizeof(tooltip));
	strnzcpy(tooltip, SUcSwitch(pText), SIZEOFARRAY(tooltip));
	TOOLINFO ti;
	ti.cbSize      = sizeof(TOOLINFO);
	ti.uFlags      = TTF_SUBCLASS;
	ti.hwnd        = Parent;
	ti.uId         = id;
	ti.rect.top    = pRect->top;
	ti.rect.left   = pRect->left;
	ti.rect.bottom = pRect->bottom;
	ti.rect.right  = pRect->right;
	ti.hinst       = TProgram::GetInst();
	ti.lpszText    = tooltip;
	return BIN(::SendMessage(HwndTT, TTM_ADDTOOL, 0, reinterpret_cast<LPARAM>(&ti)));
}

int STooltip::Remove(long id)
{
	TOOLINFO ti;
	MEMSZERO(ti);
	ti.cbSize = sizeof(TOOLINFO);
	ti.uFlags = TTF_SUBCLASS;
	ti.hwnd   = Parent;
	ti.uId    = id;
	ti.hinst  = TProgram::GetInst();
	return BIN(SendMessage(HwndTT, (UINT)TTM_DELTOOL, 0, reinterpret_cast<LPARAM>(&ti)));
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
		::SendMessage(hwnd, WM_USER_CLOSE_TOOLTIPMSGWIN, 0, 0);
	return TRUE;
}

static BOOL CALLBACK CloseTooltipWnd2(HWND hwnd, LPARAM lParam)
{
	::SendMessage(hwnd, WM_USER_CLOSE_TOOLTIPMSGWIN, 0, 0);
	return TRUE;
}

/*static*/void FASTCALL SMessageWindow::DestroyByParent(HWND parent)
{
	EnumWindows(CloseTooltipWnd, reinterpret_cast<LPARAM>(parent));
	EnumChildWindows(parent, CloseTooltipWnd2, 0);
}

/*static*/LRESULT CALLBACK ImgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	SMessageWindow * p_wnd = static_cast<SMessageWindow *>(TView::GetWindowUserData(hWnd));
	switch(uMsg) {
		case WM_PAINT:
			{
				PAINTSTRUCT ps;
				::BeginPaint(hWnd, &ps);
#ifdef STOOLTIP_USE_FIG
				SDrawFigure * p_fig = static_cast<SDrawFigure *>(p_wnd->GetImage());
				if(p_fig) {
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
					canv.PushTransform();
					p_fig->GetViewPort(&vp);
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
					canv.Draw(p_fig);
					canv.PopTransform();
				}
#else
				((SImage*)p_wnd->GetImage())->Draw(hWnd, 0);
#endif
				::EndPaint(hWnd, &ps);
			}
			return 0;
	}
	return CallWindowProc(p_wnd->PrevImgProc, hWnd, uMsg, wParam, lParam);
}

int SMessageWindow::SetFont(HWND hCtl)
{
	if(hCtl) {
		const long height = (Flags & SMessageWindow::fLargeText) ? 26 : 13;
		const long weight = (Flags & SMessageWindow::fLargeText) ? FW_HEAVY : FW_MEDIUM;
		LOGFONT log_font;
		MEMSZERO(log_font);
		log_font.lfCharSet = RUSSIAN_CHARSET;
		STRNSCPY(log_font.lfFaceName, _T("MS Shell Dlg"));
		log_font.lfHeight = height;
		log_font.lfWeight = weight;
		ZDeleteWinGdiObject(&Font);
		Font = CreateFontIndirect(&log_font);
		if(Font)
			::SendMessage(hCtl, WM_SETFONT, (WPARAM)Font, TRUE);
	}
	return 1;
}

int SMessageWindow::Open(SString & rText, const char * pImgPath, HWND parent, long cmd, long timer, COLORREF color, long flags, long extra)
{
	int    ok = 0;
	int    font_init = 0;
	HWND   hwnd_parent = NZOR(parent, APPL->H_MainWnd);
	HWND   h_focus = ::GetFocus();
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
#ifdef STOOLTIP_USE_FIG
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
#else
			SImage * p_img = new SImage();
			P_Image = p_img;
			p_img->Init();
			p_img->LoadImage(ImgPath);
			img_height = p_img->GetHeight(); // @v9.5.10 
			img_width = p_img->GetWidth(); // @v9.5.10
			p_img->SetClearColor(Color);
			TView::SetWindowProp(h_img, GWLP_USERDATA, this);
			PrevImgProc = static_cast<WNDPROC>(TView::SetWindowProp(h_img, GWLP_WNDPROC, ImgProc));
			// @v9.5.10 @construction {
			if(flags & SMessageWindow::fMaxImgSize && img_height > 0.0 && img_width > 0.0) {
				const double rel = img_width / img_height;
				RECT   _pr;
				::GetWindowRect(hwnd_parent, &_pr);
				if(rel > 1.0) {
				}
				else {
				}
			}
			// } @v9.5.10 @construction 
#endif
		}
		if(h_ctl) {
			if(Flags & SMessageWindow::fTextAlignLeft) {
				RECT   ctl_rect;
				RECT   img_rect;
				RECT   parent_rect;
				long   style = TView::GetWindowStyle(h_ctl);
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
				h_ctl = ::CreateWindow(_T("STATIC"), _T(""), style|SS_LEFT, ctl_rect.left, ctl_rect.top, ctl_rect.right, ctl_rect.bottom, HWnd, 0, TProgram::GetInst(), 0);
				if(h_ctl) {
					SetFont(h_ctl);
					::SetWindowLong(h_ctl, GWL_ID, 1201/*CTL_TOOLTIP_TEXT*/);
					font_init = 1;
				}
			}
		}
		if(!font_init)
			SetFont(h_ctl);
		SETIFZ(Color, RGB(0xFF, 0xF7, 0x94));
		if(!(Flags & SMessageWindow::fOpaque))
			SetWindowTransparent(HWnd, 75);
		if(Flags & SMessageWindow::fChildWindow) {
			long   win_flags = TView::GetWindowStyle(HWnd);
			win_flags &= ~WS_POPUP;
			win_flags |= WS_CHILD;
			::SetWindowLong(HWnd, GWL_STYLE, win_flags);
			::SetWindowLong(HWnd, GWL_EXSTYLE, (LONG)0);
			::SetParent(HWnd, hwnd_parent);
		}
		Brush = ::CreateSolidBrush(Color);
		Text.ReplaceChar('\003', ' ').Strip().Transf(CTRANSF_INNER_TO_OUTER);
		Move();
		::ShowWindow(HWnd, SW_SHOWNORMAL);
		::UpdateWindow(HWnd);
		::SetTimer(HWnd, MSGWND_CLOSETIMER,  ((timer > 0) ? timer : 60000), static_cast<TIMERPROC>(0));
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
#ifdef STOOLTIP_USE_FIG
		delete static_cast<SDrawFigure *>(P_Image);
#else
		delete static_cast<SImage *>(P_Image);
#endif
	}
	HWnd  = 0;
	Text.Z();
	Cmd   = 0;
	Extra = 0;
	ZDeleteWinGdiObject(&Brush);
	ZDeleteWinGdiObject(&Font);
}

int SMessageWindow::Move()
{
	if(HWnd) {
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
				const  int  max_w = (parent_rect.right - parent_rect.left) / 5;
				int    w = 0;
				int    h = 0;
				HDC    hdc = GetDC(h_ctl);
				RECT   ctl_rect;
				SString buf, buf2;
				StringSet ss('\n', Text);
				Text.Z();
				if(Font)
					SelectObject(hdc, Font);
				for(uint i = 0; ss.get(&i, buf) > 0;) {
					SIZE size;
					if(buf.Len() == 0)
						buf.Space();
					GetTextExtentPoint32(hdc, SUcSwitch(buf), static_cast<int>(buf.Len()), &size); // @unicodeproblem
					w = MAX(w, size.cx);
					if(w > max_w) {
						SplitBuf(hdc, buf, max_w, 10); // максимум 10 строчек для 1-ой подстроки
						StringSet ss2('\n', buf);
						buf.Z();
						uint j = 0, k = 0;
						while(ss2.get(&j, buf2) > 0 && buf2.Len() > 0) {
							buf.Cat(buf2).CR();
							k++;
						}
						h += size.cy * k;
						w = max_w;
					}
					else {
						h += size.cy;
						buf.CatChar('\n');
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
		else if(h_ctl == 0)
			toolt_h = img_rect.bottom - img_rect.top + 20;
		if(Flags & SMessageWindow::fShowOnCenter) {
			toolt_rect.top  = parent_rect.top  + (parent_rect.bottom - parent_rect.top)  / 2 - toolt_h / 2;
			toolt_rect.left = parent_rect.left + (parent_rect.right  - parent_rect.left) / 2 - toolt_w / 2;
		}
		else if(Flags & SMessageWindow::fShowOnCursor) {
			int    delta = GetSystemMetrics(SM_CXVSCROLL) + GetSystemMetrics(SM_CXBORDER);
			POINT  p;
			GetCursorPos(&p);
			toolt_rect.top  = p.y - toolt_h + 2;
			toolt_rect.left = p.x - 2;
			toolt_rect.top  = (toolt_rect.top  < parent_rect.top)  ? parent_rect.top  + 1 : toolt_rect.top;
			toolt_rect.left = (toolt_rect.left < parent_rect.left) ? parent_rect.left + 1 : toolt_rect.left;
			toolt_rect.top  = (toolt_rect.top + toolt_h  > parent_rect.bottom) ? (parent_rect.bottom - toolt_h - 1) : toolt_rect.top;
			toolt_rect.left = (toolt_rect.left + toolt_w > parent_rect.right)  ? (parent_rect.right - toolt_w + 2) : toolt_rect.left;

			toolt_rect.top  += 8; // @v10.5.5 5-->8
			toolt_rect.left -= 8; // @v10.5.5 5-->8
		}
		else if(Flags & SMessageWindow::fShowOnRUCorner) { // @v10.9.0
			toolt_rect.bottom = parent_rect.top + toolt_h + 64 + 128;
			toolt_rect.left   = parent_rect.right - toolt_w - 32;
		}
		else {
			toolt_rect.top  = parent_rect.bottom - toolt_h - 64; // @v10.5.5 5-->64
			toolt_rect.left = parent_rect.right  - toolt_w - 32; // @v10.5.5 5-->32
		}
		if(Flags & SMessageWindow::fChildWindow) {
			// toolt_rect.left -= parent_rect.left;
			toolt_rect.top  -= parent_rect.top;
		}
		::SetWindowPos(HWnd, (Flags & fTopmost) ? HWND_TOP : 0, toolt_rect.left, toolt_rect.top, toolt_w, toolt_h, (Flags & fTopmost) ? 0 : SWP_NOZORDER);
		TView::SSetWindowText(h_ctl, Text);
	}
	return 1;
}

int SMessageWindow::DoCommand(TPoint p)
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
				TPoint p;
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
				HDC hdc = reinterpret_cast<HDC>(wParam);
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
			DestroyWindow(hWnd);
			break;
		case WM_TIMER:
			if(wParam == MSGWND_CLOSETIMER) {
				DestroyWindow(hWnd);
				return 0;
			}
			break;
		 case WM_MOUSEMOVE:
			if(p_win->Flags & SMessageWindow::fCloseOnMouseLeave) {
				POINT pnt;
				GetCursorPos(&pnt);
				if(p_win->PrevMouseCoord.x != pnt.x || p_win->PrevMouseCoord.y != pnt.y)
					DestroyWindow(hWnd);
				else {
					p_win->PrevMouseCoord = pnt;
					//
					TRACKMOUSEEVENT tme;
					MEMSZERO(tme);
					tme.cbSize      = sizeof(tme);
					tme.dwFlags     = TME_LEAVE;
					tme.hwndTrack   = hWnd;
					tme.dwHoverTime = 0;
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
