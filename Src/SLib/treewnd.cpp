// TREEWND.CPP
// Modified by A.Starodub 2013, 2016, 2018
//
#include <slib.h>
#include <tv.h>
#pragma hdrstop
#include <ppdefs.h>
#include <crpe.h>

#define MENU_TREELIST 1014
#define BTN_CLOSE     1015

TreeWindow::ListWindowItem::ListWindowItem(long cmd, ListWindow * pLw) : Cmd(cmd), P_Lw(pLw)
{
}

TreeWindow::ListWindowItem::~ListWindowItem()
{
	ZDELETE(P_Lw);
}

TreeWindow::TreeWindow(HWND parentWnd) : P_CurLw(0), P_Toolbar(0)
{
	Hwnd = APPL->CreateDlg(4100, parentWnd, TreeWindow::WndProc, (long)this);
	H_CmdList = GetDlgItem(Hwnd, MENU_TREELIST);
	APPL->SetWindowViewByKind(Hwnd, TProgram::wndtypNone);
	ShortcWnd.Create(Hwnd);
}

TreeWindow::~TreeWindow()
{
	ZDELETE(P_Toolbar);
	//SetWindowLong(Hwnd, GWLP_USERDATA, 0);
	TView::SetWindowProp(Hwnd, GWLP_USERDATA, (void *)0);
	DestroyWindow(Hwnd);
}

// static
INT_PTR CALLBACK TreeWindow::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int    r = 1;
	char   menu_name[256];
	TreeWindow * p_view = (TreeWindow*)TView::GetWindowUserData(hWnd);
	switch(message) {
		case WM_INITDIALOG:
			{
				//SetWindowLong(hWnd, GWLP_USERDATA, (long)lParam);
				TView::SetWindowProp(hWnd, GWLP_USERDATA, lParam);
				HWND   h_tv = GetDlgItem(hWnd, MENU_TREELIST);
				if(h_tv)
					TreeView_SetBkColor(h_tv, GetGrayColorRef(0.8f));
			}
 			break;
		case WM_NOTIFY: {
			NMHDR * nm = (LPNMHDR)lParam;
			if(wParam == MENU_TREELIST && (nm->code == NM_DBLCLK || nm->code == TVN_KEYDOWN && ((LPNMTVKEYDOWN)nm)->wVKey == 13)) {
				HWND   h_tv = GetDlgItem(hWnd, MENU_TREELIST);
				HTREEITEM hI = TreeView_GetSelection(h_tv);
				TVITEM item;
				item.mask = TVIF_HANDLE|TVIF_TEXT|TVIF_CHILDREN|TVIF_PARAM;
				item.pszText = menu_name;
				item.hItem = hI;
				item.cchTextMax = sizeof(menu_name);
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
			PostMessage(APPL->H_MainWnd, WM_SIZE, 0, 0);
			break;
		case WM_SIZE:
			if(!IsIconic(APPL->H_MainWnd)) {
				APPL->SizeMainWnd(hWnd);
				RECT rc;
				::GetClientRect(hWnd, &rc);
				CALLPTRMEMB(p_view, MoveChilds(rc));
			}
			break;
		case WM_GETMINMAXINFO:
			if(!IsIconic(APPL->H_MainWnd)) {
				LPMINMAXINFO lpMinMax = (LPMINMAXINFO) lParam;
				RECT rcClient;
				::GetClientRect(APPL->H_MainWnd, &rcClient);
				lpMinMax->ptMinTrackSize.x = 40;
				lpMinMax->ptMaxTrackSize.x = rcClient.right/2;
				return 0;
			}
			else
				break;
		case WM_USER_SHOWTREEWNDITEM:
			CALLPTRMEMB(p_view, SelItem((HWND)wParam));
			break;
		case WM_USER_CLOSETREEWNDITEM:
			CALLPTRMEMB(p_view, CloseItem((HWND)wParam));
			break;
		case WM_SYSCOMMAND:
			if(wParam != SC_CLOSE)
				return 0;
			else {
				ShowWindow(hWnd, SW_HIDE);
				MENUITEMINFO mii;
				MEMSZERO(mii);
				mii.cbSize = sizeof(MENUITEMINFO);
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

int SLAPI TreeWindow::TranslateKeyCode(ushort keyCode, uint * pCmd) const
{
	return (P_Toolbar) ? P_Toolbar->TranslateKeyCode(keyCode, pCmd) : 0;
}

void TreeWindow::SetupCmdList(HMENU hMenu, HTREEITEM hP)
{
	HTREEITEM hti;
	HWND   h_tv = H_CmdList;
	if(!hP || hP == TVI_ROOT)
		TreeView_DeleteAllItems(h_tv);
	char   menu_name[256];
	int    cnt = GetMenuItemCount(hMenu);
	if(hP == TVI_ROOT)
		cnt--;
	for(int i = 0; i < cnt; i++) {
		MENUITEMINFO mii;
		mii.cbSize = sizeof(MENUITEMINFO);
		mii.fMask = MIIM_DATA|MIIM_SUBMENU|MIIM_TYPE|MIIM_STATE|MIIM_ID;
		mii.dwTypeData = menu_name;
		mii.cch = sizeof(menu_name);
		GetMenuItemInfo(hMenu, i, TRUE, &mii); // @unicodeproblem
		if(menu_name[0] != 0) {
			TVINSERTSTRUCT is;
			is.hParent = hP;
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
			char * chr = strchr(menu_name, '&');
			if(chr)
				memmove(chr, chr+1, strlen(chr));
			is.item.pszText = menu_name;
			is.item.cchTextMax = mii.cch;
	  		if(mii.fType != MFT_SEPARATOR) {
				hti = TreeView_InsertItem(h_tv, &is); // @unicodeproblem
				if(is.item.cChildren)
					SetupCmdList(mii.hSubMenu, hti); // @recursion
			}
		}
	}
}

/*
void TreeWindow::Setup(HMENU hMenu)
{
	StrAssocArray * p_list = new StrAssocArray();
	MenuToList(hMenu, 0, p_list);
	P_CurLw = new ListWindow(new StdTreeListBoxDef(p_list, lbtDblClkNotify|lbtFocNotify|lbtDisposeData, MKSTYPE(S_ZSTRING, 64)), 0, 0);
	Insert(0, "Команды", P_CurLw);

}
*/

void TreeWindow::Setup(HMENU hMenu)
{
	SetupCmdList(hMenu, 0);
	Insert(0, "Команды", 0);
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
	uint pos = 0, count = Items.getCount();
	ListWindowItem * p_lwi = 0;
	for(uint i = 0; !p_lwi && i < count; i++) {
		ListWindowItem * p_cur_lwi = Items.at(i);
		if(p_cur_lwi && p_cur_lwi->P_Lw->H() == hWnd) {
			p_lwi = Items.at(i);
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
	ListWindow * p_lw = (p_lwi) ? p_lwi->P_Lw : 0;
	ShowList(p_lw);
}

void TreeWindow::ShowList(ListWindow * pLw)
{
	HWND prev_hwnd = (P_CurLw) ? P_CurLw->H() : H_CmdList;
	HWND cur_hwnd  = (pLw)     ? pLw->H()     : H_CmdList;
	if(cur_hwnd) {
		if(prev_hwnd)
			ShowWindow(prev_hwnd, SW_HIDE);
		P_CurLw = pLw;
		RECT rc;
		GetClientRect(Hwnd, &rc);
		ShortcWnd.SelItem(cur_hwnd);
		ZDELETE(P_Toolbar);
		uint tb_id = (pLw) ? pLw->GetToolbar() : 0;
		if(tb_id) {
			P_Toolbar = new TToolbar(Hwnd, TBS_NOMOVE);
			if(P_Toolbar->Init(tb_id, TV_EXPTOOLBAR) <= 0)
				ZDELETE(P_Toolbar);
		}
		MoveChilds(rc);
		ShowWindow(cur_hwnd, SW_SHOWNORMAL);
		UpdateWindow(Hwnd);
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

void TreeWindow::MoveChilds(const RECT & rRect)
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
		sh_rect.right  = rect.right - rect.left;
		sh_rect.bottom = sh_h;
		ShortcWnd.MoveWindow(sh_rect);
	}
	if(P_CurLw)
		P_CurLw->MoveWindow(rect);
	else
		::MoveWindow(H_CmdList, rect.left, rect.top, rect.right, rect.bottom, 1);
	//ShowWindow(GetDlgItem(Hwnd, BTN_CLOSE), (P_CurLw) ? SW_SHOWNORMAL : SW_HIDE);
}

void TreeWindow::MenuToList(HMENU hMenu, long parentId, StrAssocArray * pList)
{
	char   menu_name[256];
	int    cnt = GetMenuItemCount(hMenu);
	if(parentId == 0)
		cnt--;
	for(int i = 0; i < cnt; i++) {
		MENUITEMINFO mii;
		mii.cbSize = sizeof(MENUITEMINFO);
		mii.fMask = MIIM_DATA|MIIM_SUBMENU|MIIM_TYPE|MIIM_STATE|MIIM_ID;
		mii.dwTypeData = menu_name;
		mii.cch = sizeof(menu_name);
		GetMenuItemInfo(hMenu, i, TRUE, &mii); // @unicodeproblem
		if(menu_name[0] != 0) {
			char * chr = strchr(menu_name, '&');
			if(chr)
				memmove(chr, chr+1, strlen(chr));
			SCharToOem(menu_name);
	  		if(mii.fType != MFT_SEPARATOR) {
				pList->Add(mii.wID, parentId, menu_name);
				if(mii.hSubMenu)
					MenuToList(mii.hSubMenu, mii.wID, pList); // @recursion
			}
		}
	}
}

void TreeWindow::AddItemCmdList(const char * pTitle, void * ptr)
{
	if(ptr) {
		char   title_buf[512];
		STRNSCPY(title_buf, pTitle);
		const  size_t title_len = strlen(title_buf);

		TVINSERTSTRUCT is;
		is.hParent         = TVI_ROOT;
		is.hInsertAfter    = TVI_LAST;
		is.item.mask       = TVIF_TEXT | TVIF_PARAM;
		is.item.lParam     = (UINT)ptr;
		is.item.cChildren  = 0;
		is.item.pszText    = title_buf;
		is.item.cchTextMax = sizeof(title_buf);
		TreeView_InsertItem(H_CmdList, &is); // @unicodeproblem
	}
}

void TreeWindow::UpdateItemCmdList(const char * pTitle, void * ptr)
{
	if(ptr) {
		char   title_buf[512];
		STRNSCPY(title_buf, pTitle);
		const  size_t title_len = strlen(title_buf);

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
				is.cchTextMax = sizeof(title_buf);
				TreeView_SetItem(hw_tree, &is); // @unicodeproblem
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
void TreeWindow::MoveWindow(const RECT &rRect) {::MoveWindow(Hwnd, rRect.left, rRect.top, rRect.right, rRect.bottom, 1);}
void TreeWindow::GetRect(RECT &rRect) {GetWindowRect(Hwnd, &rRect);}
void TreeWindow::Show(int show) {ShowWindow(Hwnd, (show) ? SW_SHOW : SW_HIDE);}
