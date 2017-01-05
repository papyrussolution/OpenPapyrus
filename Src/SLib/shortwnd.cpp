// SHORTWND.CPP
//
// Modified by A.Starodub 2013, 2016
//
#include <slib.h>
#include <tv.h>
#pragma hdrstop
// @v9.1.12 #include <ppdefs.h>
// @v9.1.12 #include <crpe.h>

ShortcutsWindow::ShortcutsWindow()
{
	Hwnd   = 0;
	HwndTT = 0;
}

ShortcutsWindow::~ShortcutsWindow()
{
	Destroy();
}

// static
BOOL CALLBACK ShortcutsWindow::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	ShortcutsWindow * p_view = (ShortcutsWindow*)TView::GetWindowUserData(hWnd);
	switch(message) {
		case WM_INITDIALOG:
			TView::SetWindowUserData(hWnd, (void *)lParam);
			break;
		case WM_NOTIFY:
			{
				NMHDR * nm = (LPNMHDR)lParam;
				if(wParam == CTL_SHORTCUTS_ITEMS && (nm->code == TCN_SELCHANGE)) {
					HWND tab_hwnd = GetDlgItem(hWnd, CTL_SHORTCUTS_ITEMS);
					int idx = TabCtrl_GetCurSel(tab_hwnd);
					TCITEM tci;
					tci.mask = TCIF_PARAM;
					if(TabCtrl_GetItem(tab_hwnd, idx, &tci)) {
						p_view->SelItem((void *)tci.lParam);
						PostMessage(GetParent(hWnd), WM_USER_SHOWTREEWNDITEM, tci.lParam, 0);
					}
					break;
				}
				else if(nm->code == NM_RCLICK) {
					SString menu_text;
					POINT  p;
					GetCursorPos(&p);
					TMenuPopup menu;
					SLS.LoadString("close", menu_text);
					menu.Add(menu_text.Transf(CTRANSF_INNER_TO_OUTER), cmaDelete);
					if(menu.Execute(hWnd, TMenuPopup::efRet) == cmaDelete) {
						int    idx = 0;
						HWND   tab_hwnd = GetDlgItem(hWnd, CTL_SHORTCUTS_ITEMS);
						TCITEM tci;
						TCHITTESTINFO ti;
						RECT rect;
						GetWindowRect(hWnd, &rect);
						p.x -= rect.left;
						p.y -= rect.top;
						ti.pt = p;
						tci.mask = TCIF_PARAM;
						idx = TabCtrl_HitTest(tab_hwnd, &ti);
						if(TabCtrl_GetItem(tab_hwnd, idx, &tci))
							PostMessage(GetParent(hWnd), WM_USER_CLOSETREEWNDITEM, tci.lParam, 0);
					}
					break;
				}
				return 0;
			}
		case WM_SHOWWINDOW:
			// PostMessage(APPL->H_MainWnd, WM_SIZE, 0, 0);
			break;
		case WM_SIZE:
			if(!IsIconic(APPL->H_MainWnd)) {
				/*
				APPL->SizeMainWnd(hWnd);
				RECT rc;
				GetClientRect(hWnd, &rc);
				MoveWindow(GetDlgItem(hWnd, MENUTREE_LIST), 0, 0, rc.right, rc.bottom, 1);
				*/
			}
			break;
		default:
			return 0;
	}
	return 1;
}

HWND ShortcutsWindow::Create(HWND parentWnd)
{
	Hwnd = APPL->CreateDlg(DLG_SHORTCUTS, parentWnd, ShortcutsWindow::WndProc, (long)this);
	if(Hwnd) {
		TView::SetWindowProp(Hwnd, GWL_STYLE, WS_CHILD);
		SetParent(Hwnd, parentWnd);
		HwndTT = CreateWindowEx(WS_EX_TOPMOST, TOOLTIPS_CLASS,
						NULL, WS_POPUP|TTS_NOPREFIX|TTS_ALWAYSTIP|TTS_BALLOON,
						CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
						Hwnd, NULL, TProgram::GetInst(), NULL);
		SetWindowPos(HwndTT, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
		TabCtrl_SetToolTips(GetDlgItem(Hwnd, CTL_SHORTCUTS_ITEMS), HwndTT);
	}
	return Hwnd;
}

void ShortcutsWindow::SelItem(void * ptr)
{
	if(Hwnd) {
		HWND   hwnd_tab = GetDlgItem(Hwnd, CTL_SHORTCUTS_ITEMS);
		int    count = TabCtrl_GetItemCount(hwnd_tab);
		for(int i = 0; i < count; i++) {
			TCITEM tci;
			tci.mask = TCIF_PARAM;
			if(TabCtrl_GetItem(hwnd_tab, i, &tci)) {
				if(LOWORD(tci.lParam) == LOWORD(ptr)) {
					TabCtrl_SetCurSel(hwnd_tab, i);
					TabCtrl_HighlightItem(hwnd_tab, i, 1);
				}
				else
					TabCtrl_HighlightItem(hwnd_tab, i, 0);
			}
		}
	}
}

void ShortcutsWindow::AddItem(const char * pTitle, void * ptr)
{
	if(Hwnd) {
		TCITEM tci;
		RECT   rc_item;
		HWND   hwnd_tab = GetDlgItem(Hwnd, CTL_SHORTCUTS_ITEMS);
		int    idx = TabCtrl_GetItemCount(hwnd_tab);
		int    prev_sel = TabCtrl_GetCurSel(hwnd_tab);
		char   temp_title_buf[SHCTSTAB_MAXTEXTLEN * 2];
		size_t title_len = (pTitle) ? strlen(pTitle) : 0;

		STRNSCPY(temp_title_buf, pTitle);
		if(title_len > SHCTSTAB_MAXTEXTLEN) {
			temp_title_buf[SHCTSTAB_MAXTEXTLEN] = 0;
			for(int j = 0; j < 3; j++)
				temp_title_buf[SHCTSTAB_MAXTEXTLEN - j - 1] = '.';
		}
		MEMSZERO(rc_item);
		tci.mask = TCIF_TEXT|TCIF_PARAM;
		tci.pszText = temp_title_buf;
		tci.cchTextMax = sizeof(temp_title_buf);
		tci.lParam = (LPARAM)ptr;
		TabCtrl_InsertItem(hwnd_tab, idx, &tci); // @unicodeproblem
		TabCtrl_SetCurSel(hwnd_tab, idx);
		TabCtrl_HighlightItem(hwnd_tab, prev_sel, 0);
		TabCtrl_HighlightItem(hwnd_tab, idx, 1);
		if(HwndTT && TabCtrl_GetItemRect(hwnd_tab, idx, &rc_item))	{
			TOOLINFO t_i;
			t_i.cbSize      = sizeof(TOOLINFO);
			t_i.uFlags      = TTF_SUBCLASS;
			t_i.hwnd        = hwnd_tab;
			t_i.uId         = (UINT_PTR)ptr;
			t_i.rect        = rc_item;
			t_i.hinst       = TProgram::GetInst();
			t_i.lpszText    = temp_title_buf;
			SendMessage(HwndTT, (UINT)TTM_DELTOOL, 0, (LPARAM)(LPTOOLINFO)&t_i); // @unicodeproblem
			SendMessage(HwndTT, TTM_ADDTOOL, 0, (LPARAM)(LPTOOLINFO)&t_i); // @unicodeproblem
		}
		if(Hwnd)
			ShowWindow(Hwnd, SW_SHOW);
	}
}

void ShortcutsWindow::UpdateItem(const char * pTitle, void * ptr)
{
	if(Hwnd) {
		int     i;
		RECT    rc_item;
		TCITEM tci;
		HWND   hwnd_tab = GetDlgItem(Hwnd, CTL_SHORTCUTS_ITEMS);
		int    count = TabCtrl_GetItemCount(hwnd_tab);
		int    _upd = 0;
		for(i = 0; i < count; i++) {
			tci.mask = TCIF_PARAM;
			if(TabCtrl_GetItem(hwnd_tab, i, &tci) && tci.lParam == (LPARAM)ptr) {
				size_t title_len = (pTitle) ? strlen(pTitle) : 0;
				char   temp_title_buf[SHCTSTAB_MAXTEXTLEN * 2];
				STRNSCPY(temp_title_buf, pTitle);
				if(title_len > SHCTSTAB_MAXTEXTLEN) {
					temp_title_buf[SHCTSTAB_MAXTEXTLEN] = 0;
					for(int j = 0; j < 3; j++)
						temp_title_buf[SHCTSTAB_MAXTEXTLEN - j - 1] = '.';
				}
				tci.mask = LVIF_TEXT;
				tci.pszText = temp_title_buf;
				tci.cchTextMax = sizeof(temp_title_buf);
				TabCtrl_SetItem(hwnd_tab, i, &tci); // @unicodeproblem
				if(HwndTT && TabCtrl_GetItemRect(hwnd_tab, i, &rc_item))	{
					TOOLINFO t_i;
					t_i.cbSize      = sizeof(TOOLINFO);
					t_i.uFlags      = TTF_SUBCLASS;
					t_i.hwnd        = hwnd_tab;
					t_i.uId         = (UINT_PTR)ptr;
					t_i.rect        = rc_item;
					t_i.hinst       = TProgram::GetInst();
					t_i.lpszText    = temp_title_buf;
					SendMessage(HwndTT, (UINT)TTM_DELTOOL, 0, (LPARAM)(LPTOOLINFO)&t_i); // @unicodeproblem
					SendMessage(HwndTT, TTM_ADDTOOL, 0, (LPARAM)(LPTOOLINFO)&t_i); // @unicodeproblem
				}
				_upd = 1;
				break;
			}
		}
		if(_upd) {
			for(i = 0; i < count; i++) {
				tci.mask = TCIF_PARAM;
				if(TabCtrl_GetItem(hwnd_tab, i, &tci)) {
					TOOLINFO t_i;
					TabCtrl_GetItemRect(hwnd_tab, i, &rc_item);
					t_i.cbSize      = sizeof(TOOLINFO);
					t_i.uFlags      = TTF_SUBCLASS;
					t_i.hwnd        = hwnd_tab;
					t_i.uId         = (UINT)tci.lParam;
					t_i.rect        = rc_item;
					t_i.hinst       = TProgram::GetInst();
					t_i.lpszText    = 0;
					SendMessage(HwndTT, (UINT)TTM_NEWTOOLRECT, 0, (LPARAM)(LPTOOLINFO)&t_i);
				}
			}
		}
	}
}

void ShortcutsWindow::DelItem(void * ptr)
{
	if(Hwnd) {
		int     i;
		char    tooltip[80];
		RECT    rc_item;
		TOOLINFO t_i;
		TCITEM tci;
		HWND   hwnd_tab = GetDlgItem(Hwnd, CTL_SHORTCUTS_ITEMS);
		int    count = TabCtrl_GetItemCount(hwnd_tab);
		for(i = 0; i < count; i++) {
			tci.mask = TCIF_PARAM;
			if(TabCtrl_GetItem(hwnd_tab, i, &tci) && LOWORD(tci.lParam) == LOWORD(ptr)) {
				MEMSZERO(rc_item);
				TabCtrl_GetItemRect(hwnd_tab, i, &rc_item);
				TabCtrl_DeleteItem(hwnd_tab, i);
				memzero(tooltip, sizeof(tooltip));
				t_i.cbSize      = sizeof(TOOLINFO);
				t_i.uFlags      = TTF_SUBCLASS;
				t_i.hwnd        = hwnd_tab;
				t_i.uId         = (UINT_PTR)ptr;
				t_i.rect        = rc_item;
				t_i.hinst       = TProgram::GetInst();
				t_i.lpszText    = 0;
				SendMessage(HwndTT, (UINT)TTM_DELTOOL, 0, (LPARAM)(LPTOOLINFO)&t_i);
				count--;
				break;
			}
		}
		if(count == 0)
			ShowWindow(Hwnd, SW_HIDE);
		else {
			for(i = 0; i < count; i++) {
				tci.mask = TCIF_PARAM;
				if(TabCtrl_GetItem(hwnd_tab, i, &tci)) {
					TOOLINFO t_i;
					memzero(tooltip, sizeof(tooltip));
					TabCtrl_GetItemRect(hwnd_tab, i, &rc_item);
					t_i.cbSize      = sizeof(TOOLINFO);
					t_i.uFlags      = TTF_SUBCLASS;
					t_i.hwnd        = hwnd_tab;
					t_i.uId         = (UINT)tci.lParam;
					t_i.rect        = rc_item;
					t_i.hinst       = TProgram::GetInst();
					t_i.lpszText    = 0;
					SendMessage(HwndTT, (UINT)TTM_NEWTOOLRECT, 0, (LPARAM)(LPTOOLINFO)&t_i);
				}
			}
		}
	}
}

void ShortcutsWindow::Destroy()
{
	if(Hwnd) {
		TView::SetWindowUserData(Hwnd, (void *)0);
		if(HwndTT)
			DestroyWindow(HwndTT);
		DestroyWindow(Hwnd);
	}
	Hwnd   = 0;
	HwndTT = 0;
}

int ShortcutsWindow::IsVisible() const
{
	return (Hwnd) ? IsWindowVisible(Hwnd) : 0;
}

void ShortcutsWindow::GetRect(RECT & rRect)
{
	if(IsVisible())
		GetWindowRect(Hwnd, &rRect);
}

int ShortcutsWindow::MoveWindow(const RECT & rRect)
{
	::MoveWindow(Hwnd, rRect.left, rRect.top, rRect.right, rRect.bottom, 1);
	return 1;
}
