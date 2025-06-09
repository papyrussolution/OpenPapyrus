// TREEWND.CPP
// Modified by A.Starodub 2013, 2016, 2018, 2019, 2020, 2021, 2023, 2025
// @codepage UTF-8
// Древовидный список в левой части основного окна
//
#include <slib-internal.h>
#pragma hdrstop
#include <ppdefs.h>

#define MENU_TREELIST 1014
#define BTN_CLOSE     1015

TreeWindow::ShortcutsWindow::ShortcutsWindow() : Hwnd(0), HwndTT(0)
{
}

TreeWindow::ShortcutsWindow::~ShortcutsWindow()
{
	Destroy();
}

/*static*/INT_PTR CALLBACK TreeWindow::ShortcutsWindow::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	TreeWindow::ShortcutsWindow * p_view = static_cast<TreeWindow::ShortcutsWindow *>(TView::GetWindowUserData(hWnd));
	switch(message) {
		case WM_INITDIALOG:
			TView::SetWindowUserData(hWnd, reinterpret_cast<void *>(lParam));
			break;
		case WM_NOTIFY:
			{
				const NMHDR * nm = reinterpret_cast<const NMHDR *>(lParam);
				if(wParam == CTL_SHORTCUTS_ITEMS && (nm->code == TCN_SELCHANGE)) {
					HWND tab_hwnd = GetDlgItem(hWnd, CTL_SHORTCUTS_ITEMS);
					int idx = TabCtrl_GetCurSel(tab_hwnd);
					TCITEM tci;
					tci.mask = TCIF_PARAM;
					if(TabCtrl_GetItem(tab_hwnd, idx, &tci)) {
						p_view->SelItem(reinterpret_cast<void *>(tci.lParam));
						PostMessage(GetParent(hWnd), WM_USER_SHOWTREEWNDITEM, tci.lParam, 0);
					}
					break;
				}
				else if(nm->code == NM_RCLICK) {
					SString menu_text;
					POINT  p;
					GetCursorPos(&p);
					TMenuPopup menu;
					uint   cmd = 0;
					SLS.LoadString_("close", menu_text);
					menu.Add(menu_text.Transf(CTRANSF_INNER_TO_OUTER), cmaDelete);
					if(menu.Execute(hWnd, TMenuPopup::efRet, &cmd, 0) && cmd == cmaDelete) {
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

HWND TreeWindow::ShortcutsWindow::Create(HWND parentWnd)
{
	Hwnd = APPL->CreateDlg(DLG_SHORTCUTS, parentWnd, ShortcutsWindow::WndProc, reinterpret_cast<LPARAM>(this));
	if(Hwnd) {
		TView::SetWindowProp(Hwnd, GWL_STYLE, WS_CHILD);
		SetParent(Hwnd, parentWnd);
		HwndTT = CreateWindowEx(WS_EX_TOPMOST, TOOLTIPS_CLASS, NULL, WS_POPUP|TTS_NOPREFIX|TTS_ALWAYSTIP|TTS_BALLOON,
			CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, Hwnd, NULL, TProgram::GetInst(), 0);
		SetWindowPos(HwndTT, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE | SWP_NOACTIVATE);
		TabCtrl_SetToolTips(GetDlgItem(Hwnd, CTL_SHORTCUTS_ITEMS), HwndTT);
	}
	return Hwnd;
}

void TreeWindow::ShortcutsWindow::SelItem(void * ptr)
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

void TreeWindow::ShortcutsWindow::AddItem(const char * pTitle, void * ptr)
{
	if(Hwnd) {
		TCITEM tci;
		RECT   rc_item;
		HWND   hwnd_tab = GetDlgItem(Hwnd, CTL_SHORTCUTS_ITEMS);
		int    idx = TabCtrl_GetItemCount(hwnd_tab);
		int    prev_sel = TabCtrl_GetCurSel(hwnd_tab);
		const  uint max_text_len = 40; //SHCTSTAB_MAXTEXTLEN;
		TCHAR  temp_title_buf[max_text_len * 2];
		size_t title_len = sstrlen(pTitle);
		STRNSCPY(temp_title_buf, SUcSwitch(pTitle));
		if(title_len > max_text_len) {
			temp_title_buf[max_text_len] = 0;
			for(int j = 0; j < 3; j++)
				temp_title_buf[max_text_len-j-1] = '.';
		}
		MEMSZERO(rc_item);
		tci.mask = TCIF_TEXT|TCIF_PARAM;
		tci.pszText = temp_title_buf;
		tci.cchTextMax = sizeof(temp_title_buf);
		tci.lParam = reinterpret_cast<LPARAM>(ptr);
		TabCtrl_InsertItem(hwnd_tab, idx, &tci);
		TabCtrl_SetCurSel(hwnd_tab, idx);
		TabCtrl_HighlightItem(hwnd_tab, prev_sel, 0);
		TabCtrl_HighlightItem(hwnd_tab, idx, 1);
		if(HwndTT && TabCtrl_GetItemRect(hwnd_tab, idx, &rc_item))	{
			//
			// Почему-то unicode-версия методов TOOLTIP не работает правильно. По-этому здесь явно используются multibyte-методы.
			//
			TOOLINFOA t_i;
			INITWINAPISTRUCT(t_i);
			t_i.uFlags   = TTF_SUBCLASS;
			t_i.hwnd     = hwnd_tab;
			t_i.uId      = reinterpret_cast<UINT_PTR>(ptr);
			t_i.rect     = rc_item;
			t_i.hinst    = TProgram::GetInst();
			t_i.lpszText = const_cast<char *>(pTitle); 
			::SendMessageW(HwndTT, (UINT)TTM_DELTOOLA, 0, reinterpret_cast<LPARAM>(&t_i));
			::SendMessageW(HwndTT, TTM_ADDTOOLA, 0, reinterpret_cast<LPARAM>(&t_i));
		}
		if(Hwnd)
			ShowWindow(Hwnd, SW_SHOW);
	}
}

void TreeWindow::ShortcutsWindow::UpdateItem(const char * pTitle, void * ptr)
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
			if(TabCtrl_GetItem(hwnd_tab, i, &tci) && tci.lParam == reinterpret_cast<LPARAM>(ptr)) {
				size_t title_len = sstrlen(pTitle);
				const  uint max_text_len = 40; //SHCTSTAB_MAXTEXTLEN;
				TCHAR  temp_title_buf[max_text_len * 2];
				STRNSCPY(temp_title_buf, SUcSwitch(pTitle));
				if(title_len > max_text_len) {
					temp_title_buf[max_text_len] = 0;
					for(int j = 0; j < 3; j++)
						temp_title_buf[max_text_len-j-1] = '.';
				}
				tci.mask = LVIF_TEXT;
				tci.pszText = temp_title_buf; // @unicodeproblem
				tci.cchTextMax = SIZEOFARRAY(temp_title_buf);
				TabCtrl_SetItem(hwnd_tab, i, &tci); // @unicodeproblem
				if(HwndTT && TabCtrl_GetItemRect(hwnd_tab, i, &rc_item))	{
					TOOLINFO t_i;
					INITWINAPISTRUCT(t_i);
					t_i.uFlags   = TTF_SUBCLASS;
					t_i.hwnd     = hwnd_tab;
					t_i.uId      = reinterpret_cast<UINT_PTR>(ptr);
					t_i.rect     = rc_item;
					t_i.hinst    = TProgram::GetInst();
					t_i.lpszText = temp_title_buf;
					::SendMessageW(HwndTT, (UINT)TTM_DELTOOL, 0, reinterpret_cast<LPARAM>(&t_i));
					::SendMessageW(HwndTT, TTM_ADDTOOL, 0, reinterpret_cast<LPARAM>(&t_i));
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
					INITWINAPISTRUCT(t_i);
					t_i.uFlags   = TTF_SUBCLASS;
					t_i.hwnd     = hwnd_tab;
					t_i.uId      = static_cast<UINT_PTR>(tci.lParam);
					t_i.rect     = rc_item;
					t_i.hinst    = TProgram::GetInst();
					::SendMessageW(HwndTT, (UINT)TTM_NEWTOOLRECT, 0, reinterpret_cast<LPARAM>(&t_i));
				}
			}
		}
	}
}

void TreeWindow::ShortcutsWindow::DelItem(void * ptr)
{
	if(Hwnd) {
		int     i;
		char    tooltip[80];
		RECT    rc_item;
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
				TOOLINFO t_i;
				INITWINAPISTRUCT(t_i);
				t_i.uFlags = TTF_SUBCLASS;
				t_i.hwnd   = hwnd_tab;
				t_i.uId    = reinterpret_cast<UINT_PTR>(ptr);
				t_i.rect   = rc_item;
				t_i.hinst  = TProgram::GetInst();
				::SendMessageW(HwndTT, (UINT)TTM_DELTOOL, 0, reinterpret_cast<LPARAM>(&t_i));
				count--;
				break;
			}
		}
		if(!count)
			ShowWindow(Hwnd, SW_HIDE);
		else {
			for(i = 0; i < count; i++) {
				tci.mask = TCIF_PARAM;
				if(TabCtrl_GetItem(hwnd_tab, i, &tci)) {
					memzero(tooltip, sizeof(tooltip));
					TabCtrl_GetItemRect(hwnd_tab, i, &rc_item);
					TOOLINFO t_i;
					INITWINAPISTRUCT(t_i);
					t_i.uFlags = TTF_SUBCLASS;
					t_i.hwnd   = hwnd_tab;
					t_i.uId    = static_cast<UINT>(tci.lParam);
					t_i.rect   = rc_item;
					t_i.hinst  = TProgram::GetInst();
					::SendMessageW(HwndTT, (UINT)TTM_NEWTOOLRECT, 0, reinterpret_cast<LPARAM>(&t_i));
				}
			}
		}
	}
}

void TreeWindow::ShortcutsWindow::Destroy()
{
	if(Hwnd) {
		TView::SetWindowUserData(Hwnd, 0);
		::DestroyWindow(HwndTT);
		::DestroyWindow(Hwnd);
	}
	Hwnd   = 0;
	HwndTT = 0;
}

int TreeWindow::ShortcutsWindow::IsVisible() const
{
	return Hwnd ? IsWindowVisible(Hwnd) : 0;
}

void TreeWindow::ShortcutsWindow::GetRect(RECT & rRect)
{
	if(IsVisible())
		::GetWindowRect(Hwnd, &rRect);
}

void TreeWindow::ShortcutsWindow::MoveWindow(const RECT & rRect)
{
	::MoveWindow(Hwnd, rRect.left, rRect.top, rRect.right, rRect.bottom, 1);
}
//
//
//
TreeWindow::ListWindowItem::ListWindowItem(long cmd, ListWindow * pLw) : Cmd(cmd), P_Lw(pLw)
{
}

TreeWindow::ListWindowItem::~ListWindowItem()
{
	ZDELETE(P_Lw);
}

TreeWindow::TreeWindow(HWND parentWnd) : P_CurLw(0), P_Toolbar(0)
{
	Hwnd = APPL->CreateDlg(4100, parentWnd, TreeWindow::WndProc, reinterpret_cast<LPARAM>(this));
	H_CmdList = GetDlgItem(Hwnd, MENU_TREELIST);
	APPL->SetWindowViewByKind(Hwnd, TProgram::wndtypNone);
	ShortcWnd.Create(Hwnd);
}

TreeWindow::~TreeWindow()
{
	ZDELETE(P_Toolbar);
	TView::SetWindowProp(Hwnd, GWLP_USERDATA, static_cast<void *>(0));
	DestroyWindow(Hwnd);
}

/*static*/INT_PTR CALLBACK TreeWindow::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int    r = 1;
	TreeWindow * p_view = static_cast<TreeWindow *>(TView::GetWindowUserData(hWnd));
	switch(message) {
		case WM_INITDIALOG:
			{
				TView::SetWindowProp(hWnd, GWLP_USERDATA, lParam);
				HWND   h_tv = GetDlgItem(hWnd, MENU_TREELIST);
				if(h_tv) {
					TreeView_SetBkColor(h_tv, RGB(0xdd, 0xf3, 0xf5));
				}
			}
 			break;
		case WM_NOTIFY: {
			NMHDR * nm = reinterpret_cast<NMHDR *>(lParam);
			if(wParam == MENU_TREELIST && (nm->code == NM_DBLCLK || (nm->code == TVN_KEYDOWN && ((LPNMTVKEYDOWN)nm)->wVKey == 13))) {
				HWND   h_tv = GetDlgItem(hWnd, MENU_TREELIST);
				HTREEITEM hI = TreeView_GetSelection(h_tv);
				TVITEM item;
				TCHAR  menu_name[256];
				item.mask = TVIF_HANDLE|TVIF_TEXT|TVIF_CHILDREN|TVIF_PARAM;
				item.pszText = menu_name; // @unicodeproblem
				item.hItem = hI;
				item.cchTextMax = SIZEOFARRAY(menu_name);
				TreeView_GetItem(h_tv, &item); // @unicodeproblem
				if(!item.cChildren)
					PostMessage(APPL->H_MainWnd, WM_COMMAND, item.lParam, 0);
				break;
			}
			return 0;
		}
		case WM_NCLBUTTONDOWN:
			if(wParam != HTRIGHT)
				SetCapture(hWnd);
			return 0;
		case WM_LBUTTONUP:
			if(hWnd == GetCapture())
				ReleaseCapture();
			break;
		case WM_SHOWWINDOW: 
			::PostMessage(APPL->H_MainWnd, WM_SIZE, 0, 0); 
			break;
		case WM_SIZE:
			if(!IsIconic(APPL->H_MainWnd)) {
				APPL->SizeMainWnd(hWnd);
				if(p_view) {
					RECT rc;
					::GetClientRect(hWnd, &rc);
					p_view->MoveChildren(rc);
				}
			}
			r = 0; // @v11.0.3
			break;
		case WM_GETMINMAXINFO:
			if(!IsIconic(APPL->H_MainWnd)) {
				LPMINMAXINFO lpMinMax = reinterpret_cast<LPMINMAXINFO>(lParam);
				RECT rcClient;
				::GetClientRect(APPL->H_MainWnd, &rcClient);
				lpMinMax->ptMinTrackSize.x = 40;
				lpMinMax->ptMaxTrackSize.x = rcClient.right/2;
				return 0;
			}
			else
				break;
		case WM_USER_SHOWTREEWNDITEM: CALLPTRMEMB(p_view, SelItem(reinterpret_cast<HWND>(wParam))); break;
		case WM_USER_CLOSETREEWNDITEM: CALLPTRMEMB(p_view, CloseItem(reinterpret_cast<HWND>(wParam))); break;
		case WM_SYSCOMMAND:
			if(wParam != SC_CLOSE)
				return 0;
			else {
				::ShowWindow(hWnd, SW_HIDE);
				MENUITEMINFO mii;
				INITWINAPISTRUCT(mii);
				HMENU  h_menu = GetMenu(APPL->H_MainWnd);
				HMENU  h_sub = GetSubMenu(h_menu, GetMenuItemCount(h_menu)-1);
				mii.fMask = MIIM_STATE;
				mii.fState &= ~MFS_CHECKED;
				mii.fState |= MFS_UNCHECKED;
				SetMenuItemInfo(h_sub, cmShowTree, FALSE, &mii);
				break;
			}
		case WM_COMMAND:
			if(p_view)
				r = p_view->OnCommand(wParam, lParam);
			break;
		default:
			return 0;
	}
	return r;
}

int TreeWindow::TranslateKeyCode(ushort keyCode, uint * pCmd) const
{
	return (P_Toolbar) ? P_Toolbar->TranslateKeyCode(keyCode, pCmd) : 0;
}

void TreeWindow::SetupCmdList(HMENU hMenu, /*HTREEITEM*/void * hP)
{
	HTREEITEM hti;
	SString temp_buf;
	HWND   h_tv = H_CmdList;
	if(!hP || hP == TVI_ROOT)
		TreeView_DeleteAllItems(h_tv);
	int    cnt = GetMenuItemCount(hMenu);
	if(hP == TVI_ROOT)
		cnt--;
	for(int i = 0; i < cnt; i++) {
		TCHAR  menu_name_buf[256];
		MENUITEMINFO mii;
		INITWINAPISTRUCT(mii);
		mii.fMask = MIIM_DATA|MIIM_SUBMENU|MIIM_TYPE|MIIM_STATE|MIIM_ID;
		mii.dwTypeData = menu_name_buf;
		mii.cch = SIZEOFARRAY(menu_name_buf);
		GetMenuItemInfo(hMenu, i, TRUE, &mii);
		if(menu_name_buf[0] != 0) {
			TVINSERTSTRUCT is;
			is.hParent = static_cast<HTREEITEM>(hP);
			is.hInsertAfter = TVI_LAST;
			is.item.mask = TVIF_TEXT;
			if(mii.hSubMenu) {
				is.item.mask |= TVIF_CHILDREN;
				is.item.cChildren = 1;
			}
			else {
				is.item.mask |= TVIF_PARAM;
				is.item.lParam = mii.wID;
				is.item.cChildren = 0;
			}
			{
				TCHAR * chr = sstrchr(menu_name_buf, '&');
				if(chr)
					memmove(chr, chr+1, sstrlen(chr)*sizeof(TCHAR));
			}
			is.item.pszText = menu_name_buf;
			is.item.cchTextMax = mii.cch;
	  		if(mii.fType != MFT_SEPARATOR) {
				hti = TreeView_InsertItem(h_tv, &is);
				if(is.item.cChildren)
					SetupCmdList(mii.hSubMenu, hti); // @recursion
			}
		}
	}
}

void TreeWindow::Setup(HMENU hMenu)
{
	SetupCmdList(hMenu, 0);
	SString temp_buf;
	SLS.LoadString_("cmd_pl", temp_buf);
	Insert(0, temp_buf.Transf(CTRANSF_INNER_TO_OUTER), 0);
}

TreeWindow::ListWindowItem * TreeWindow::GetListWinByCmd(long cmd, uint * pPos)
{
	uint   pos = 0;
	ListWindowItem * p_lwi = Items.lsearch(&cmd, &pos, CMPF_LONG) ? Items.at(pos) : 0;
	ASSIGN_PTR(pPos, pos);
	return p_lwi;
}

TreeWindow::ListWindowItem * TreeWindow::GetListWinByHwnd(HWND hWnd, uint * pPos)
{
	uint  pos = 0;
	ListWindowItem * p_lwi = 0;
	for(uint i = 0; !p_lwi && i < Items.getCount(); i++) {
		ListWindowItem * p_cur_lwi = Items.at(i);
		if(p_cur_lwi && p_cur_lwi->P_Lw->H() == hWnd) {
			p_lwi = p_cur_lwi;//Items.at(i);
			pos = i;
		}
	}
	ASSIGN_PTR(pPos, pos);
	return p_lwi;
}

void TreeWindow::Insert(long cmd, const char * pTitle, ListWindow * pLw)
{
	uint pos = 0;
	if(pLw == 0)
		ShortcWnd.AddItem(pTitle, H_CmdList);
	else {
		ListWindowItem * p_lwi = GetListWinByCmd(cmd, &pos);
		if(p_lwi != 0)
			CloseItem(p_lwi->P_Lw->H());
		{
			ListWindowItem * p_lwi = new ListWindowItem(cmd, pLw);
			Items.insert(p_lwi);
			pLw->executeNM(Hwnd);
			ShortcWnd.AddItem(pTitle, pLw->H());
		}
	}
	ShowList(pLw);
}

void TreeWindow::CloseItem(HWND hWnd)
{
	uint pos = 0;
	ListWindowItem * p_lwi = GetListWinByHwnd(hWnd, &pos);
	if(p_lwi && p_lwi->P_Lw) {
		ShortcWnd.DelItem(hWnd);
		Items.atFree(pos);
		P_CurLw = 0;
		const uint count = Items.getCount();
		ShowList((count > 0) ? Items.at(count-1)->P_Lw : 0);
	}
}

void TreeWindow::SelItem(HWND hWnd)
{
	ListWindowItem * p_lwi = GetListWinByHwnd(hWnd, 0);
	ListWindow * p_lw = p_lwi ? p_lwi->P_Lw : 0;
	ShowList(p_lw);
}

void TreeWindow::ShowList(ListWindow * pLw)
{
	HWND prev_hwnd = P_CurLw ? P_CurLw->H() : H_CmdList;
	HWND cur_hwnd  = pLw ? pLw->H() : H_CmdList;
	if(cur_hwnd) {
		if(prev_hwnd)
			::ShowWindow(prev_hwnd, SW_HIDE);
		P_CurLw = pLw;
		RECT rc;
		GetClientRect(Hwnd, &rc);
		ShortcWnd.SelItem(cur_hwnd);
		ZDELETE(P_Toolbar);
		uint tb_id = pLw ? pLw->GetToolbar() : 0;
		if(tb_id) {
			P_Toolbar = new TToolbar(Hwnd, TBS_NOMOVE);
			if(P_Toolbar->Init(tb_id, TV_EXPTOOLBAR) <= 0)
				ZDELETE(P_Toolbar);
		}
		MoveChildren(rc);
		::ShowWindow(cur_hwnd, SW_SHOWNORMAL);
		::UpdateWindow(Hwnd);
	}
}

int TreeWindow::OnCommand(WPARAM wParam, LPARAM lParam)
{
	int    r = 1;
	if(P_CurLw && P_CurLw->listBox()) {
		TView::messageCommand(P_CurLw->listBox()->P_Owner, wParam, this);
		r = 0;
	}
	return r;
}

void TreeWindow::MoveChildren(const RECT & rRect)
{
	RECT rect = rRect;
	if(P_Toolbar) {
		RECT tb_rect;
		P_Toolbar->OnMainSize();
		GetWindowRect(P_Toolbar->H(), &tb_rect);
		rect.top = tb_rect.bottom - tb_rect.top;
		rect.bottom -= rect.top;
		/* По какой-то причине кнопки не отображаются в данном окне
		int tb_h = (tb_rect.bottom - tb_rect.top) - 4;
		RECT btn_rect;
		btn_rect.left = tb_rect.right + 2;
		btn_rect.right = btn_rect.left + tb_h;
		btn_rect.top = tb_rect.top + 2;
		btn_rect.bottom = btn_rect.top + tb_h;
		HWND hwnd_btn = GetDlgItem(Hwnd, BTN_CLOSE);
		::MoveWindow(hwnd_btn, btn_rect.left, btn_rect.top, btn_rect.right, btn_rect.bottom, 1);
		*/
	}
	if(ShortcWnd.IsVisible()) {
		RECT   sh_rect;
		ShortcWnd.GetRect(sh_rect);
		const int sh_h = sh_rect.bottom - sh_rect.top;
		rect.bottom -= sh_h;
		sh_rect.top    = rect.top + rect.bottom;
		sh_rect.left   = 0;
		sh_rect.right  = (rect.right - rect.left);
		sh_rect.bottom = sh_h;
		ShortcWnd.MoveWindow(sh_rect);
	}
	if(P_CurLw)
		P_CurLw->Move_(rect);
	else
		::MoveWindow(H_CmdList, rect.left, rect.top, rect.right, rect.bottom, 1);
	//ShowWindow(GetDlgItem(Hwnd, BTN_CLOSE), (P_CurLw) ? SW_SHOWNORMAL : SW_HIDE);
}

void TreeWindow::MenuToList(HMENU hMenu, long parentId, StrAssocArray * pList)
{
	SString temp_buf;
	int    cnt = GetMenuItemCount(hMenu);
	if(parentId == 0)
		cnt--;
	for(int i = 0; i < cnt; i++) {
		TCHAR  menu_name_buf[256];
		MENUITEMINFO mii;
		INITWINAPISTRUCT(mii);
		mii.fMask = MIIM_DATA|MIIM_SUBMENU|MIIM_TYPE|MIIM_STATE|MIIM_ID;
		mii.dwTypeData = menu_name_buf;
		mii.cch = SIZEOFARRAY(menu_name_buf);
		GetMenuItemInfo(hMenu, i, TRUE, &mii);
		if(menu_name_buf[0] != 0) {
			temp_buf = SUcSwitch(menu_name_buf);
			temp_buf.ReplaceStr("&", 0, 1); 
			temp_buf.Transf(CTRANSF_OUTER_TO_INNER);
	  		if(mii.fType != MFT_SEPARATOR) {
				pList->Add(mii.wID, parentId, temp_buf);
				if(mii.hSubMenu)
					MenuToList(mii.hSubMenu, mii.wID, pList); // @recursion
			}
		}
	}
}

void TreeWindow::AddItemCmdList(const char * pTitle, void * ptr)
{
	if(ptr) {
		TCHAR  title_buf[512];
		strnzcpy(title_buf, SUcSwitch(pTitle), SIZEOFARRAY(title_buf));
		const  size_t title_len = sstrlen(title_buf);
		TVINSERTSTRUCT is;
		is.hParent = TVI_ROOT;
		is.hInsertAfter    = TVI_LAST;
		is.item.mask       = TVIF_TEXT | TVIF_PARAM;
		is.item.lParam     = reinterpret_cast<LPARAM>(ptr);
		is.item.cChildren  = 0;
		is.item.pszText    = title_buf;
		is.item.cchTextMax = SIZEOFARRAY(title_buf);
		TreeView_InsertItem(H_CmdList, &is);
	}
}

void TreeWindow::UpdateItemCmdList(const char * pTitle, void * ptr)
{
	if(ptr) {
		TCHAR  title_buf[512];
		strnzcpy(title_buf, SUcSwitch(pTitle), SIZEOFARRAY(title_buf));
		const  size_t title_len = sstrlen(title_buf);
		HWND   hw_tree = H_CmdList;
		for(HTREEITEM h_item = TreeView_GetRoot(hw_tree); h_item; h_item = TreeView_GetNextSibling(hw_tree, h_item)) {
			TVITEM is;
			MEMSZERO(is);
			is.mask = TVIF_CHILDREN | TVIF_PARAM | TVIF_HANDLE;
			is.hItem = h_item;
			TreeView_GetItem(hw_tree, &is);
			if(!is.cChildren && LOWORD(is.lParam) == LOWORD(ptr)) {
				is.hItem      = h_item;
				is.mask       = TVIF_TEXT;
				is.pszText    = title_buf;
				is.cchTextMax = SIZEOFARRAY(title_buf);
				TreeView_SetItem(hw_tree, &is);
				break;
			}
		}
	}
}

void TreeWindow::DelItemCmdList(void * ptr)
{
	if(ptr) {
		HWND hw_tree = H_CmdList;
		for(HTREEITEM h_item = TreeView_GetRoot(hw_tree); h_item; h_item = TreeView_GetNextSibling(hw_tree, h_item)) {
			TVITEM is;
			MEMSZERO(is);
			is.mask = TVIF_CHILDREN | TVIF_PARAM | TVIF_HANDLE;
			is.hItem = h_item;
			TreeView_GetItem(hw_tree, &is);
			if(!is.cChildren && LOWORD(is.lParam) == LOWORD(ptr)) {
				TreeView_DeleteItem(hw_tree, h_item);
				break;
			}
		}
	}
}

int  TreeWindow::IsVisible() {return IsWindowVisible(Hwnd);}
void TreeWindow::Move_(const RECT &rRect) {::MoveWindow(Hwnd, rRect.left, rRect.top, rRect.right, rRect.bottom, 1);}
void TreeWindow::GetRect(RECT &rRect) {GetWindowRect(Hwnd, &rRect);}
void TreeWindow::Show(int show) { ShowWindow(Hwnd, (show) ? SW_SHOW : SW_HIDE); }
