// TPROGRAM.CPP  Turbo Vision 1.0
// Copyright (c) 1991 by Borland International
// Modified by A.Sobolev 1996, 1997, 2003, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021, 2022
// @codepage UTF-8
//
#include <slib-internal.h>
#pragma hdrstop
#include <ppdefs.h>
//
#define CLOSEBTN_BITMAPID  132 // defined in ppdefs.h as IDB_CLOSE
#define MENUTREE_LIST     1014
#define ROUNDRECT_RADIUS     2
//
//
//
BOOL CALLBACK StatusWinDialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	TStatusWin * p_view = static_cast<TStatusWin *>(TView::GetWindowUserData(hWnd));
	switch(uMsg) {
		case WM_DESTROY:
			if(p_view) {
				p_view->OnDestroy(hWnd);
				delete p_view;
			}
			return 0;
		case WM_LBUTTONDBLCLK:
			{
				POINT  coord;
				coord.x = LOWORD(lParam);
				coord.y = HIWORD(lParam);
				TView::messageCommand(p_view, cmaEdit, static_cast<void *>(&coord));
			}
			return 0;
		default:
			break;
	}
	return CallWindowProc(p_view->PrevWindowProc, hWnd, uMsg, wParam, lParam);
}

TStatusWin::TStatusWin() : TWindow(TRect(1,1,50,20), 0, 1)
{
	HW = ::CreateWindowEx(WS_EX_TOPMOST, STATUSCLASSNAME, 0,
		WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS|SBT_TOOLTIPS, 0, 0, 0, 0,
		APPL->H_MainWnd, static_cast<HMENU>(0)/*111*/, TProgram::GetInst(), 0);
	TView::SetWindowProp(H(), GWLP_USERDATA, this);
	PrevWindowProc = static_cast<WNDPROC>(TView::SetWindowProp(H(), GWLP_WNDPROC, StatusWinDialogProc));
}
//
// Returns:
//   !0 - Success
//   0  - Error. To get extended error information, callGetLastError.
//
int TStatusWin::GetRect(RECT * pRcStatus)
{
	return pRcStatus ? GetWindowRect(H(), pRcStatus) : 0;
}

int TStatusWin::Update()
{
	int    ok = 1;
	ENTER_CRITICAL_SECTION
	int    i;
	int    l_parts[64];
	const  int n_parts = MIN(Items.getCount(), SIZEOFARRAY(l_parts));
	int    n_width = 4;
	SString temp_buf;
	HWND   hw = H();
	::SendMessage(hw, WM_SIZE, 0, 0);
	HDC    hdc = ::GetDC(hw);
	::SendMessage(hw, SB_SETBKCOLOR, 0, static_cast<LPARAM>(RGB(0xD4, 0xD0, 0xC8)));
	for(i = 0; i < n_parts; i++) {
		SIZE local_size;
		const StItem & r_item = Items.at(i);
		temp_buf = r_item.str;
		if(r_item.Icon)
			n_width += 24; // icon size + borders
        else if(temp_buf.NotEmpty() && GetTextExtentPoint32(hdc, SUcSwitch(temp_buf), static_cast<int>(temp_buf.Len()), &local_size))
			n_width += local_size.cx;
		l_parts[i] = n_width;
	}
	::SendMessage(hw, SB_SETPARTS, static_cast<WPARAM>(n_parts), reinterpret_cast<LPARAM>(l_parts));
	for(i = 0; i < n_parts; i++) {
		const StItem & r_item = Items.at(i);
		temp_buf = r_item.str;
		const long icon_id = r_item.Icon;
		if(icon_id) {
			HICON h_icon = 0;
			if(icon_id & _SlConst.VectorImageMask) {
				TWhatmanToolArray::Item tool_item;
				const SDrawFigure * p_fig = APPL->LoadDrawFigureById(icon_id, &tool_item);
				const uint _w = 16;
				const uint _h = 16;
				SImageBuffer ib(_w, _h);
				{
					TCanvas2 canv(APPL->GetUiToolBox(), ib);
					if(!tool_item.ReplacedColor.IsEmpty()) {
						SColor replacement_color;
						replacement_color = APPL->GetUiToolBox().GetColor(TProgram::tbiIconRegColor);
						canv.SetColorReplacement(tool_item.ReplacedColor, replacement_color);
					}
					LMatrix2D mtx;
					SViewPort vp;
					FRect pic_bounds(static_cast<float>(_w), static_cast<float>(_h));
					//
					canv.Rect(pic_bounds);
					//canv.Fill(SColor(255, 255, 255, 255), 0); // Прозрачный фон
					canv.Fill(SColor(192, 192, 192, 255), 0); // Прозрачный фон
					canv.PushTransform();
					p_fig->GetViewPort(&vp);
					canv.AddTransform(vp.GetMatrix(pic_bounds, mtx));
					canv.Draw(p_fig);
				}
				h_icon = static_cast<HICON>(ib.TransformToIcon());
			}
			else {
				h_icon = LoadIcon(TProgram::GetInst(), MAKEINTRESOURCE(icon_id)); // @1
				// @construction
				// Вместо строки выше (@1) следует использовать этот блок, однако, предварительно
				// решив проблему кэширования изображения (DestroyIcon (@2) не дает отрисовать изображение)
				// HICON h_icon = (HICON)::LoadImage(TProgram::GetInst(), MAKEINTRESOURCE(icon_id), IMAGE_ICON, 0, 0, 0);
			}
			if(h_icon) {
				::SendMessage(hw, SB_SETICON, i, reinterpret_cast<LPARAM>(h_icon));
				::DestroyIcon(h_icon); // @2
				::SendMessage(hw, SB_SETTIPTEXT, i, reinterpret_cast<LPARAM>(SUcSwitch(temp_buf.cptr())));
			}
		}
		else {
			COLORREF color = r_item.Color;
			if(color || r_item.TextColor)
				::SendMessage(hw, SB_SETTEXT, static_cast<WPARAM>(SBT_OWNERDRAW|i), reinterpret_cast<LPARAM>(&r_item));
			else
				::SendMessage(hw, SB_SETTEXT, i, reinterpret_cast<LPARAM>(SUcSwitch(temp_buf.cptr())));
		}
	}
	::ReleaseDC(hw, hdc);
	invalidateAll(true);
	LEAVE_CRITICAL_SECTION
	return ok;
}

int TStatusWin::AddItem(const char * pStr, long icon /* = 0 */, COLORREF color /*=0*/, uint cmd/*=0*/, COLORREF textColor /*=0*/)
{
	int    ok = 0;
	if(!isempty(pStr)) {
		StItem item;
		STRNSCPY(item.str, pStr);
		padleft(item.str, ' ', 1);
		item.Icon      = icon;
		item.Color     = color;
		item.Cmd       = cmd;
		item.TextColor = textColor;
		ok = Items.insert(&item);
	}
	return ok;
}

int TStatusWin::RemoveItem(int pos)
{
	int    ok = 1;
	if(pos == -1)
		Items.clear();
	else if(pos >= 0 && (uint)pos < Items.getCount())
		ok = Items.atFree(static_cast<uint>(pos));
	return ok;
}

uint TStatusWin::GetCmdByCoord(POINT coord, TStatusWin::StItem * pItem /*=0*/)
{
	uint   cmd = 0;
	uint   n_parts = Items.getCount();
	for(uint i = 0; !cmd && i < n_parts; i++) {
		RECT rect;
		::SendMessage(H(), SB_GETRECT, i, reinterpret_cast<LPARAM>(&rect));
		if(coord.x >= rect.left && coord.x <= rect.right)
			cmd = Items.at(i).Cmd;
	}
	return cmd;
}

IMPL_HANDLE_EVENT(TStatusWin)
{
	TWindow::handleEvent(event);
	if(event.isCmd(cmaEdit)) {
		if(APPL->P_DeskTop) {
			uint cmd = GetCmdByCoord(*static_cast<const POINT *>(event.message.infoPtr));
			if(cmd)
				TView::messageCommand(APPL, cmd);
		}
		clearEvent(event);
	}
}
//
//
//
TProgram * TProgram::application;     // @global
HINSTANCE  TProgram::hInstance;       // @global @threadsafe

const UserInterfaceSettings & TProgram::GetUiSettings()
{
	if(!(State & stUiSettingInited)) {
		ENTER_CRITICAL_SECTION
		if(!(State & stUiSettingInited)) {
			UICfg.Restore();
		}
		LEAVE_CRITICAL_SECTION
	}
	return UICfg;
}

int TProgram::UpdateUiSettings(const UserInterfaceSettings & rS)
{
	UICfg = rS;
	return UICfg.Save();
}

int TProgram::SelectTabItem(const void * ptr)
{
	if(H_ShortcutsWnd) {
		HWND   hwnd_tab = GetDlgItem(H_ShortcutsWnd, CTL_SHORTCUTS_ITEMS);
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
	return 1;
}

int TProgram::AddListToTree(long cmd, const char * pTitle, ListWindow * pLw)
{
	int    ok = -1;
	if(P_TreeWnd) {
		if(!P_TreeWnd->IsVisible()) {
			// @v11.3.8 P_TreeWnd->Show(1);
			ShowLeftTree(true); // @v11.3.8 
			PostMessage(H_MainWnd, WM_COMMAND, cmShowTree, 0);
		}
		if(cmd && pLw) // @v10.1.3
			P_TreeWnd->Insert(cmd, pTitle, pLw);
		ok = 1;
	}
	return ok;
}

void TProgram::DelItemFromMenu(void * ptr)
{
	if(ptr) {
		HMENU  h_menu = GetMenu(H_MainWnd);
		h_menu = GetSubMenu(h_menu, GetMenuItemCount(h_menu) - 1);
		if(h_menu) {
			CALLPTRMEMB(P_TreeWnd, DelItemCmdList(ptr));
			if(H_ShortcutsWnd) {
				int     i;
				//char    tooltip[80];
				RECT    rc_item;
				TCITEM tci;
				HWND   hwnd_tab = GetDlgItem(H_ShortcutsWnd, CTL_SHORTCUTS_ITEMS);
				HWND   hwnd_tt  = TabCtrl_GetToolTips(hwnd_tab);
				int    count = TabCtrl_GetItemCount(hwnd_tab);
				for(i = 0; i < count; i++) {
					tci.mask = TCIF_PARAM;
					if(TabCtrl_GetItem(hwnd_tab, i, &tci) && LOWORD(tci.lParam) == LOWORD(ptr)) {
						MEMSZERO(rc_item);
						TabCtrl_GetItemRect(hwnd_tab, i, &rc_item);
						TabCtrl_DeleteItem(hwnd_tab, i);
						//memzero(tooltip, sizeof(tooltip));
						TOOLINFO t_i;
						INITWINAPISTRUCT(t_i);
						t_i.uFlags      = TTF_SUBCLASS;
						t_i.hwnd        = hwnd_tab;
						t_i.uId = reinterpret_cast<UINT_PTR>(ptr);
						t_i.rect        = rc_item;
						t_i.hinst       = TProgram::GetInst();
						::SendMessage(hwnd_tt, (UINT)TTM_DELTOOL, 0, reinterpret_cast<LPARAM>(&t_i));
						count--;
						break;
					}
				}
				if(!count)
					ShowWindow(H_ShortcutsWnd, SW_HIDE);
				else {
					for(i = 0; i < count; i++) {
						tci.mask = TCIF_PARAM;
						if(TabCtrl_GetItem(hwnd_tab, i, &tci)) {
							TOOLINFO t_i;
							//memzero(tooltip, sizeof(tooltip));
							TabCtrl_GetItemRect(hwnd_tab, i, &rc_item);
							INITWINAPISTRUCT(t_i);
							t_i.uFlags = TTF_SUBCLASS;
							t_i.hwnd   = hwnd_tab;
							t_i.uId    = static_cast<UINT>(tci.lParam);
							t_i.rect   = rc_item;
							t_i.hinst  = TProgram::GetInst();
							::SendMessage(hwnd_tt, (UINT)TTM_NEWTOOLRECT, 0, reinterpret_cast<LPARAM>(&t_i));
						}
					}
				}
				UpdateWindow(H_MainWnd);
			}
			DeleteMenu(h_menu, (UINT)ptr, MF_BYCOMMAND);
		}
	}
}

int TProgram::UpdateItemInMenu(const char * pTitle, void * ptr)
{
	if(ptr) {
		char   title_buf[512];
		STRNSCPY(title_buf, pTitle);
		const  size_t title_len = sstrlen(title_buf);
		HMENU  h_menu = GetMenu(H_MainWnd);
		h_menu = GetSubMenu(h_menu, GetMenuItemCount(h_menu) - 1);
		if(h_menu) {
			CALLPTRMEMB(P_TreeWnd, UpdateItemCmdList(pTitle, ptr));
			if(H_ShortcutsWnd) {
				int     i;
				RECT    rc_item;
				TCITEM tci;
				HWND   hwnd_tab = GetDlgItem(H_ShortcutsWnd, CTL_SHORTCUTS_ITEMS);
				HWND   hwnd_tt  = TabCtrl_GetToolTips(hwnd_tab);
				int    count = TabCtrl_GetItemCount(hwnd_tab);
				int    _upd = 0;
				for(i = 0; i < count; i++) {
					tci.mask = TCIF_PARAM;
					if(TabCtrl_GetItem(hwnd_tab, i, &tci) && tci.lParam == reinterpret_cast<LPARAM>(ptr)) {
						const  uint max_text_len = 40;// SHCTSTAB_MAXTEXTLEN;
						TCHAR  temp_org_title_buf[max_text_len * 2];
						TCHAR  temp_title_buf[max_text_len * 2];
						STRNSCPY(temp_org_title_buf, SUcSwitch(title_buf));
						STRNSCPY(temp_title_buf, temp_org_title_buf);
						if(title_len > max_text_len) {
							temp_title_buf[max_text_len] = 0;
							for(int j = 0; j < 3; j++)
								temp_title_buf[max_text_len-j-1] = '.';
						}
						tci.mask = LVIF_TEXT;
						tci.pszText = temp_title_buf;
						tci.cchTextMax = SIZEOFARRAY(temp_title_buf);
						TabCtrl_SetItem(hwnd_tab, i, &tci);
						if(hwnd_tt && TabCtrl_GetItemRect(hwnd_tab, i, &rc_item))	{
							TOOLINFO t_i;
							INITWINAPISTRUCT(t_i);
							t_i.uFlags   = TTF_SUBCLASS;
							t_i.hwnd     = hwnd_tab;
							t_i.uId      = reinterpret_cast<UINT_PTR>(ptr);
							t_i.rect     = rc_item;
							t_i.hinst    = TProgram::GetInst();
							t_i.lpszText = temp_org_title_buf/*title_buf*/;
							::SendMessage(hwnd_tt, (UINT)TTM_DELTOOL, 0, reinterpret_cast<LPARAM>(&t_i));
							::SendMessage(hwnd_tt, TTM_ADDTOOL, 0, reinterpret_cast<LPARAM>(&t_i));
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
							t_i.uFlags = TTF_SUBCLASS;
							t_i.hwnd   = hwnd_tab;
							t_i.uId    = static_cast<UINT>(tci.lParam);
							t_i.rect   = rc_item;
							t_i.hinst  = TProgram::GetInst();
							::SendMessage(hwnd_tt, (UINT)TTM_NEWTOOLRECT, 0, reinterpret_cast<LPARAM>(&t_i));
						}
					}
				}
				UpdateWindow(H_MainWnd);
			}
		}
	}
	return 1;
}

int TProgram::AddItemToMenu(const char * pTitle, void * ptr)
{
	if(ptr) {
		TCHAR  title_buf[512];
		char   mb_title_buf[512];
		STRNSCPY(title_buf, SUcSwitch(pTitle));
		STRNSCPY(mb_title_buf, pTitle);
		const  size_t title_len = sstrlen(title_buf);
		HMENU h_menu = GetMenu(H_MainWnd);
		h_menu = GetSubMenu(h_menu, GetMenuItemCount(h_menu) - 1);
		DelItemFromMenu(ptr);
		if(h_menu) {
			MENUITEMINFO mii;
			INITWINAPISTRUCT(mii);
			mii.fMask = MIIM_TYPE|MIIM_DATA|MIIM_ID;
			mii.wID   = reinterpret_cast<UINT>(ptr);
			mii.dwItemData = reinterpret_cast<ULONG_PTR>(ptr);
			mii.dwTypeData = title_buf;
			InsertMenuItem(h_menu, reinterpret_cast<UINT>(ptr), FALSE, &mii);
			CALLPTRMEMB(P_TreeWnd, AddItemCmdList(pTitle, ptr));
			if(H_ShortcutsWnd) {
				TCITEM tci;
				RECT   rc_item;
				HWND   hwnd_tab = GetDlgItem(H_ShortcutsWnd, CTL_SHORTCUTS_ITEMS);
				HWND   hwnd_tt  = TabCtrl_GetToolTips(hwnd_tab);
				int    idx = TabCtrl_GetItemCount(hwnd_tab);
				int    prev_sel = TabCtrl_GetCurSel(hwnd_tab);
				const  uint max_text_len = 40;// SHCTSTAB_MAXTEXTLEN;
				TCHAR  temp_title_buf[max_text_len * 2];
				TCHAR  temp_org_title_buf[max_text_len * 2];
				STRNSCPY(temp_org_title_buf, title_buf);
				STRNSCPY(temp_title_buf, title_buf);
				if(title_len > max_text_len) {
					temp_title_buf[max_text_len] = 0;
					for(int j = 0; j < 3; j++)
						temp_title_buf[max_text_len-j-1] = '.';
				}
				MEMSZERO(rc_item);
				tci.mask = TCIF_TEXT|TCIF_PARAM;
				tci.pszText = temp_title_buf;
				tci.cchTextMax = SIZEOFARRAY(temp_title_buf);
				tci.lParam = reinterpret_cast<LPARAM>(ptr);
				TabCtrl_InsertItem(hwnd_tab, idx, &tci);
				TabCtrl_SetCurSel(hwnd_tab, idx);
				TabCtrl_HighlightItem(hwnd_tab, prev_sel, 0);
				TabCtrl_HighlightItem(hwnd_tab, idx, 1);
				if(hwnd_tt && TabCtrl_GetItemRect(hwnd_tab, idx, &rc_item))	{
					TOOLINFOA t_i;
					INITWINAPISTRUCT(t_i);
					t_i.uFlags   = TTF_SUBCLASS;
					t_i.hwnd     = hwnd_tab;
					t_i.uId      = reinterpret_cast<UINT_PTR>(ptr);
					t_i.rect     = rc_item;
					t_i.hinst    = TProgram::GetInst();
					t_i.lpszText = mb_title_buf/*temp_org_title_buf*/;
					SendMessage(hwnd_tt, TTM_DELTOOLA, 0, reinterpret_cast<LPARAM>(&t_i));
					SendMessage(hwnd_tt, TTM_ADDTOOLA, 0, reinterpret_cast<LPARAM>(&t_i));
				}
				::ShowWindow(H_ShortcutsWnd, SW_SHOW);
				::UpdateWindow(H_MainWnd);
			}
		}
	}
	return 1;
}

HWND TProgram::CreateDlg(uint dlgID, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam)
	{ return ::CreateDialogParam(GetInst(), MAKEINTRESOURCE(dlgID), hWndParent, lpDialogFunc, dwInitParam); }
INT_PTR TProgram::DlgBoxParam(uint dlgID, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam)
	{ return ::DialogBoxParam(GetInst(), MAKEINTRESOURCE(dlgID), hWndParent, lpDialogFunc, dwInitParam); }
HBITMAP FASTCALL TProgram::LoadBitmap_(uint bmID)
	{ return static_cast<HBITMAP>(::LoadImage(GetInst(), MAKEINTRESOURCE(bmID), IMAGE_BITMAP, 0, 0, 0)); }
HBITMAP FASTCALL TProgram::FetchBitmap(uint bmID)
	{ return BmH.Get(bmID); }
HBITMAP FASTCALL TProgram::FetchSystemBitmap(uint bmID)
	{ return BmH.GetSystem(bmID); }

/*virtual*/int TProgram::InitStatusBar()
{
	SETIFZ(P_Stw, new TStatusWin);
	return BIN(P_Stw);
}

/*virtual*/int TProgram::LoadVectorTools(TWhatmanToolArray * pT)
{
	return -1;
}

int TProgram::GetStatusBarRect(RECT * pRect)
{
	memzero(pRect, sizeof(*pRect));
	return P_Stw ? P_Stw->GetRect(pRect) : 0;
}

int TProgram::ClearStatusBar()
	{ return P_Stw ? P_Stw->RemoveItem(-1) : 0; }
int TProgram::AddStatusBarItem(const char * pStr, long icon /* = 0 */, COLORREF color /*=0*/, uint cmd /*=0*/, COLORREF textColor /*=0*/)
	{ return P_Stw ? P_Stw->AddItem(pStr, icon, color, cmd, textColor) : 0; }
int TProgram::UpdateStatusBar()
	{ return P_Stw ? P_Stw->Update() : 0; }
void TProgram::SetupTreeWnd(HMENU hMenu, void * hP) // @v10.9.4 HTREEITEM-->(void *)
	{ CALLPTRMEMB(P_TreeWnd, Setup(hMenu)); }

int TProgram::GetClientRect(RECT * pClientRC)
{
	int    ok = -1;
	if(pClientRC) {
		RECT   rc_toolbar, rc_status;
		MEMSZERO(rc_toolbar);
		MEMSZERO(rc_status);
		::GetClientRect(H_MainWnd, pClientRC);
		if(P_Stw) {
			P_Stw->GetRect(&rc_status);
			pClientRC->bottom -= rc_status.bottom-rc_status.top;
		}
		if(P_Toolbar && IsWindowVisible(P_Toolbar->H())) {
			GetWindowRect(P_Toolbar->H(), &rc_toolbar);
			rc_toolbar.bottom -= rc_toolbar.top;
			rc_toolbar.right  -= rc_toolbar.left;
			switch(P_Toolbar->GetCurrPos()) {
				case TOOLBAR_ON_TOP: pClientRC->top += rc_toolbar.bottom;
					// @fallthrough
				case TOOLBAR_ON_BOTTOM: pClientRC->bottom -= rc_toolbar.bottom; break;
				case TOOLBAR_ON_LEFT: pClientRC->left += rc_toolbar.right;
					// @fallthrough
				case TOOLBAR_ON_RIGHT: pClientRC->right -= rc_toolbar.right; break;
			}
		}
		ok = 1;
	}
	return ok;
}

void TProgram::SizeMainWnd(HWND hw)
{
	// @todo Use layout
	if(P_Toolbar && hw == H_MainWnd) {
		P_Toolbar->OnMainSize();
	}
	RECT   rc_client;
	MEMSZERO(rc_client);
	CALLPTRMEMB(P_Stw, Update());
	GetClientRect(&rc_client);
	// @v11.4.2 const RECT org_rc_client(rc_client);
	int _width  = rc_client.right - rc_client.left;
	int _height = rc_client.bottom - rc_client.top;
	if(IsWindowVisible(H_ShortcutsWnd)) {
		RECT _rc;
		GetWindowRect(H_ShortcutsWnd, &_rc);
		_rc.bottom -= _rc.top;
		if(hw != H_ShortcutsWnd) {
			_rc.top = rc_client.top + rc_client.bottom - _rc.bottom;
			SetWindowPos(H_ShortcutsWnd, 0, rc_client.left, _rc.top, rc_client.right, _rc.bottom, SWP_NOZORDER); // @debug (0 -> SWP_NOZORDER)
		}
		rc_client.bottom -= _rc.bottom;
	}
	if(IsWindowVisible(H_LogWnd)) {
		RECT _rc;
		GetWindowRect(H_LogWnd, &_rc);
		_rc.bottom -= _rc.top;
		if(hw != H_LogWnd) {
			SETMIN(_rc.bottom, rc_client.bottom / 2);
			_rc.top = rc_client.top + rc_client.bottom - _rc.bottom;
			::MoveWindow(H_LogWnd, rc_client.left, _rc.top, rc_client.right, _rc.bottom, 1);
			//SetWindowPos(H_LogWnd, 0, rc_client.left, rc_log.top, rc_client.right, rc_log.bottom, SWP_NOZORDER); // @debug (0 -> SWP_NOZORDER)
		}
		rc_client.bottom -= _rc.bottom;
	}
	if(IsTreeVisible()) {
		HWND h_tree = GetTreeHWND();
		RECT _rc;
		GetTreeRect(_rc);
		_rc.right -= _rc.left;
		if(hw != h_tree) {
			//const long tree_wnd_style = TView::GetWindowStyle(h_tree);
			int  tw_top = rc_client.top;
			int  tw_bottom = rc_client.bottom;
			/* @v11.3.8 {
				RECT rc_dt;
				if(H_Desktop && GetWindowRect(H_Desktop, &rc_dt)) {
					tw_top = rc_dt.top;
					//tw_bottom = rc_dt.bottom;
				}
			}*/
			//if(tree_wnd_style & WS_POPUP) {
			//}
			SETMIN(_rc.right, rc_client.right / 2);
			::MoveWindow(h_tree, rc_client.left, tw_top, _rc.right, tw_bottom, 1);
		}
		rc_client.left  += _rc.right;
		rc_client.right -= _rc.right;
	}
	if(H_FrameWnd)
		::MoveWindow(H_FrameWnd, rc_client.left, rc_client.top, rc_client.right, rc_client.bottom, 1);
	///*
	if(IsWindowVisible(H_Desktop))
		MoveWindow(H_Desktop, 0, 0, rc_client.right - 18, rc_client.bottom - 2, 1);
	//*/
}

int  TProgram::IsTreeVisible() const { return BIN(P_TreeWnd && P_TreeWnd->IsVisible()); }
void TProgram::GetTreeRect(RECT & rRect) { CALLPTRMEMB(P_TreeWnd, GetRect(rRect)); }
HWND TProgram::GetTreeHWND() const { return (P_TreeWnd) ? P_TreeWnd->Hwnd : 0; }

void TProgram::ShowLeftTree(bool visible)
{
	if(P_TreeWnd) {
		if(visible) {
			P_TreeWnd->Show(1);
			::InvalidateRect(H_MainWnd, 0, true);
			::UpdateWindow(H_MainWnd);
		}
		else {
			P_TreeWnd->Show(0);
		}
	}
}

INT_PTR CALLBACK ShortcutsWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message) {
		case WM_INITDIALOG:
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
						APPL->SelectTabItem(reinterpret_cast<const void *>(tci.lParam));
						PostMessage(APPL->H_MainWnd, WM_COMMAND, tci.lParam, 0);
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
							::DestroyWindow(reinterpret_cast<BrowserWindow *>(tci.lParam)->H());
					}
					break;
				}
				return 0;
			}
		case WM_SHOWWINDOW:
			PostMessage(APPL->H_MainWnd, WM_SIZE, 0, 0);
			break;
		case WM_SIZE:
			if(!IsIconic(APPL->H_MainWnd)) {
				APPL->SizeMainWnd(hWnd);
				RECT rc;
				::GetClientRect(hWnd, &rc);
				::MoveWindow(GetDlgItem(hWnd, MENUTREE_LIST), 0, 0, rc.right, rc.bottom, 1);
			}
			break;
		default:
			return 0;
	}
	return 1;
}

/*static*/BOOL CALLBACK TProgram::CloseWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	TProgram * p_pgm = static_cast<TProgram *>(TView::GetWindowUserData(hWnd));
	switch(message) {
		case WM_SETFOCUS:
			return 0;
		case WM_LBUTTONDOWN:
			SetCapture(hWnd);
			break;
		case WM_LBUTTONUP:
			ReleaseCapture();
			RECT rc;
			::GetClientRect(hWnd, &rc);
			if(checkirange(static_cast<long>(LOWORD(lParam)), 0L, rc.right) && checkirange(static_cast<long>(HIWORD(lParam)), 0L, rc.bottom))
				::SendMessage(p_pgm->GetFrameWindow(), WM_USER_CLOSEBROWSER, 0, 0);
			SetFocus(p_pgm->H_MainWnd);
			break;
	}
	return p_pgm->PrevCloseWndProc(hWnd, message, wParam, lParam);
}

BOOL CALLBACK SendMainWndSizeMessage(HWND hwnd, LPARAM lParam)
{
	if(GetParent(hwnd) == reinterpret_cast<HWND>(lParam))
		::SendMessage(hwnd, WM_USER_MAINWND_MOVE_SIZE, 0, 0);
	return TRUE;
}

INT_PTR CALLBACK FrameWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int    is_desktop = 0;
	HWND   h_close_wnd = APPL->H_CloseWnd;
	HWND   h_tree_wnd = APPL->P_TreeWnd ? APPL->P_TreeWnd->Hwnd : 0; // @v11.0.0
	{
		HWND hwnd_tw = GetTopWindow(hWnd);
		while(hwnd_tw && oneof2(hwnd_tw, h_close_wnd, h_tree_wnd))
			hwnd_tw = GetNextWindow(hwnd_tw, GW_HWNDNEXT);
		//if(hwnd_tw == h_close_wnd) hwnd_tw = GetNextWindow(hwnd_tw, GW_HWNDNEXT);
		is_desktop = BIN(hwnd_tw == APPL->H_Desktop);
	}
	switch(message) {
		case WM_SIZE:
			{
				HWND hW = GetTopWindow(hWnd);
				HWND hWt = hW;
				while(hW) {
					if(!oneof2(hW, h_close_wnd, h_tree_wnd))
						::MoveWindow(hW, 0, 0, LOWORD(lParam)-16, HIWORD(lParam), 0);
					else {
						::ShowWindow(hW, SW_HIDE);
						::MoveWindow(hW, LOWORD(lParam)-14, 2, 12, 12, 0);
						::ShowWindow(hW, SW_SHOW);
					}
					hW = ::GetNextWindow(hW, GW_HWNDNEXT);
				}
				while(hWt && oneof2(hWt, h_close_wnd, h_tree_wnd))
					hWt = ::GetNextWindow(hWt, GW_HWNDNEXT);
				//if(hWt == h_close_wnd) hWt = ::GetNextWindow(hWt, GW_HWNDNEXT);
				if(hWt) {
					::SetWindowPos(hWt, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
					::SendMessage(hWnd, WM_USER, 0, 0);
				}
			}
			return 0; // @v11.0.3 1-->0
		case WM_USER_NOTIFYBRWFRAME:
			{
				HWND   hb = ::GetTopWindow(hWnd);
				while(hb && oneof2(hb, h_close_wnd, h_tree_wnd))
					hb = ::GetNextWindow(hb, GW_HWNDNEXT);
				SString main_text_buf;
				TView::SGetWindowText(APPL->H_MainWnd, main_text_buf);
				size_t div_pos = 0;
				if(main_text_buf.Search(" : ", 0, 0, &div_pos))
					main_text_buf.Trim(div_pos);
				if(hb) {
					SString child_text_buf;
					TView::SGetWindowText(hb, child_text_buf);
					main_text_buf.CatDiv(':', 1).Cat(child_text_buf);
					::ShowWindow(hWnd, SW_SHOWNA);
					::UpdateWindow(hWnd);
				}
				else {
					::ShowWindow(hWnd, SW_HIDE);
					::SetFocus(APPL->H_MainWnd);
				}
				TView::SSetWindowText(APPL->H_MainWnd, main_text_buf);
			}
			break;
		case WM_USER_CLOSEBROWSER:
			{
				HWND hb = GetTopWindow(hWnd);
				while(hb && oneof2(hb, h_close_wnd, h_tree_wnd))
					hb = GetNextWindow(hb, GW_HWNDNEXT);
				if(!is_desktop)
					DestroyWindow(hb);
			}
			break;
		case WM_PAINT:
			{
				PAINTSTRUCT ps;
				RECT   rc;
				GetClientRect(hWnd, &rc);
				rc.left = rc.right-16;
				HDC  hdc = BeginPaint(hWnd, &ps);
				if(SIntersectRect(rc, ps.rcPaint)) {
					TCanvas canv(hdc);
					HPEN h_pen1 = CreatePen(PS_SOLID, 1, GetGrayColorRef(0.44f));
					HPEN h_pen2 = CreatePen(PS_SOLID, 2, GetGrayColorRef(0.94f));
					canv.SelectObjectAndPush(h_pen1);
					canv.LineVert(rc.right-9, 18, rc.bottom-5);
					canv.LineVert(rc.right-5, 18, rc.bottom-5);
					canv.SelectObjectAndPush(h_pen2);
					canv.LineVert(rc.right-11, 19, rc.bottom-5);
					canv.LineVert(rc.right- 7, 19, rc.bottom-5);
					canv.PopObjectN(2);
					ZDeleteWinGdiObject(&h_pen1);
					ZDeleteWinGdiObject(&h_pen2);
				}
				EndPaint(hWnd, &ps);
			}
			break;
	}
	return 0;
}

static BOOL CALLBACK IsBrowsersExists(HWND hwnd, LPARAM lParam)
{
	SString cls_name;
	TView::SGetWindowClassName(hwnd, cls_name);
	if(cls_name.Cmp(SUcSwitch(BrowserWindow::WndClsName), 0) == 0) {
		*reinterpret_cast<long *>(lParam) = 1;
		return FALSE;
	}
	else
		return TRUE;
}

/*static*/LRESULT CALLBACK TProgram::MainWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	TProgram * p_pgm = 0;
	switch(message) {
		case WM_CREATE:
			{
				LPCREATESTRUCT p_create_data = reinterpret_cast<LPCREATESTRUCT>(lParam);
				p_pgm = static_cast<TProgram *>(p_create_data->lpCreateParams);
				TView::SetWindowProp(hWnd, GWLP_USERDATA, p_pgm);
				p_pgm->H_TopOfStack = hWnd;
				p_pgm->H_MainWnd = hWnd;
				BrowserWindow::RegWindowClass(TProgram::GetInst());
				STimeChunkBrowser::RegWindowClass(TProgram::GetInst());
				SetTimer(hWnd, 1, 500, 0);
				p_pgm->H_FrameWnd = APPL->CreateDlg(4101, hWnd, FrameWndProc, 0);
				p_pgm->P_TreeWnd = new TreeWindow(hWnd);
				{
					if(p_pgm->UICfg.Flags & UserInterfaceSettings::fShowShortcuts) {
						p_pgm->H_ShortcutsWnd = APPL->CreateDlg(DLG_SHORTCUTS, hWnd, ShortcutsWndProc, 0);
						if(p_pgm->H_ShortcutsWnd) {
							HWND hwnd_tt = ::CreateWindowEx(WS_EX_TOPMOST, TOOLTIPS_CLASS,
								NULL, WS_POPUP|TTS_NOPREFIX|TTS_ALWAYSTIP|TTS_BALLOON,
								CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
								hWnd, NULL, TProgram::GetInst(), 0);
							SetWindowPos(hwnd_tt, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
							TabCtrl_SetToolTips(GetDlgItem(p_pgm->H_ShortcutsWnd, CTL_SHORTCUTS_ITEMS), hwnd_tt);
						}
					}
				}
				p_pgm->H_CloseWnd = CreateWindowEx(0, _T("BUTTON"), _T("X"),
					WS_VISIBLE|WS_CHILD|WS_CLIPSIBLINGS, 2, 2, 12, 12, p_pgm->GetFrameWindow(), 0, TProgram::GetInst(), 0);
				p_pgm->SetWindowViewByKind(p_pgm->H_ShortcutsWnd, TProgram::wndtypNone);
				p_pgm->SetWindowViewByKind(p_pgm->H_CloseWnd, TProgram::wndtypNone);
				p_pgm->PrevCloseWndProc = static_cast<WNDPROC>(TView::SetWindowProp(p_pgm->H_CloseWnd, GWLP_WNDPROC, CloseWndProc));
				TView::SetWindowProp(p_pgm->H_CloseWnd, GWLP_USERDATA, p_pgm);
				p_pgm->P_Toolbar = new TToolbar(hWnd, 0);
			}
 			return 0;
		case WM_ERASEBKGND:
			{
				long brw_exists = 0;
				EnumChildWindows(APPL->H_FrameWnd,	IsBrowsersExists, reinterpret_cast<LPARAM>(&brw_exists));
				return brw_exists ? 1 : DefWindowProc(hWnd, message, wParam, lParam);
			}
			break;
		case WM_USER:
			p_pgm = static_cast<TProgram *>(TView::GetWindowUserData(hWnd));
			p_pgm->SetupTreeWnd(GetMenu(hWnd), TVI_ROOT);
			CALLPTRMEMB(p_pgm->P_Toolbar, Init(TOOLBAR_MAIN, TV_GLBTOOLBAR));
			// @v11.2.4 CALLPTRMEMB(p_pgm->P_TreeWnd, Show(1));
			::PostMessage(hWnd, WM_COMMAND, cmShowTree, 0);
			::PostMessage(hWnd, WM_SIZE, 0, 0);
			break;
		case WM_COMMAND:
			{
				p_pgm = static_cast<TProgram *>(TView::GetWindowUserData(hWnd));
				MENUITEMINFO mii;
				INITWINAPISTRUCT(mii);
				int   cnt = GetMenuItemCount(GetMenu(hWnd));
				HMENU hmW = GetSubMenu(GetMenu(hWnd), cnt-1);
				if(LOWORD(wParam) == cmShowToolbar) {
					mii.fMask = MIIM_STATE;
					GetMenuItemInfo(hmW, cmShowToolbar, FALSE, &mii);
					if(mii.fState & MFS_CHECKED) {
						mii.fState &= ~MFS_CHECKED;
						mii.fState |= MFS_UNCHECKED;
						CALLPTRMEMB(p_pgm->P_Toolbar, Hide());
					}
					else {
						mii.fState &= ~MFS_UNCHECKED;
						mii.fState |= MFS_CHECKED;
						CALLPTRMEMB(p_pgm->P_Toolbar, Show());
					}
					SetMenuItemInfo(hmW, cmShowToolbar, FALSE, &mii);
					PostMessage(hWnd, WM_SIZE, 0, 0);
					break;
				}
				if(LOWORD(wParam) == cmShowTree) {
					mii.fMask = MIIM_STATE;
					GetMenuItemInfo(hmW, cmShowTree, FALSE, &mii);
					if(mii.fState & MFS_CHECKED) {
						mii.fState &= ~MFS_CHECKED;
						mii.fState |= MFS_UNCHECKED;
					}
					else {
						mii.fState &= ~MFS_UNCHECKED;
						mii.fState |= MFS_CHECKED;
					}
					// @v11.3.8 CALLPTRMEMB(p_pgm->P_TreeWnd, Show(BIN(mii.fState & MFS_CHECKED)));
					p_pgm->ShowLeftTree(LOGIC(mii.fState & MFS_CHECKED)); // @v11.3.8
					SetMenuItemInfo(hmW, cmShowTree, FALSE, &mii);
					break;
				}
				if(LOWORD(wParam) && hmW) {
					/*
					// Все броузеры имеют ZOrder одинаковый, поэтому при перерисовке происходят некоторые неприятные коллизии
					// Для их устранения существует данный кусок кода, но он вызывает некоторые не менее неприятные события:
					// невозможность переключения между броузерами по Ctrl+Tab, поэтому пока закоментирован.
					int brw_count = GetMenuItemCount(hmW);
					for(int idx = 0; idx < brw_count; idx++) {
						mii.fMask = MIIM_DATA|MIIM_ID;
						GetMenuItemInfo(hmW, idx, TRUE, &mii);
						if(mii.dwItemData)
							if(LOWORD(mii.wID) != LOWORD(wParam))
								EnableWindow(((TWindow *)mii.dwItemData)->hWnd, FALSE);
							else
								EnableWindow(((TWindow *)mii.dwItemData)->hWnd, TRUE);
					}
					*/
					mii.fMask = MIIM_DATA|MIIM_ID;
					mii.wID = LOWORD(wParam);
					GetMenuItemInfo(hmW, wParam, FALSE, &mii);
					if(LOWORD(mii.wID) == LOWORD(mii.dwItemData)) {
						HWND   hwnd = reinterpret_cast<const TWindow *>(mii.dwItemData)->H();
						SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE|SWP_NOMOVE);
						SetFocus(hwnd);
						break;
					}
				}
				TEvent event;
				p_pgm->handleEvent(event.setCmd(LOWORD(wParam), 0));
			}
			break;
		case WM_SIZE:
			if(wParam != SIZE_MINIMIZED) {
				p_pgm = static_cast<TProgram *>(TView::GetWindowUserData(hWnd));
				p_pgm->SizeMainWnd(hWnd);
				EnumWindows(SendMainWndSizeMessage, reinterpret_cast<LPARAM>(hWnd));
			}
			break;
		case WM_DRAWITEM:
			{
				DRAWITEMSTRUCT * p_di = reinterpret_cast<DRAWITEMSTRUCT *>(lParam);
				if(p_di) {
					p_pgm = static_cast<TProgram *>(TView::GetWindowUserData(hWnd));
					if(p_pgm->DrawControl(p_di->hwndItem, message, wParam, lParam) > 0)
						return TRUE;
				}
			}
			break;
		case WM_TIMER:
		case WM_ENTERIDLE:
			//APPL->idle(); // @v10.0.02
			// @v10.0.02 {
			{
				TGroup * targets[] = { APPL->P_DeskTop, APPL };
				for(uint i = 0; i < SIZEOFARRAY(targets); i++) {
					TGroup * p_tgt = targets[i];
					TView::messageBroadcast(p_tgt, cmIdle);
					//
					// Далее следует специальный цикл, призванный закрыть те окна, которые об этом
					// "попросили" выставив флаг состояния sfCloseMe
					//
					for(int is_there_closeme = 1; is_there_closeme;) {
						is_there_closeme = 0;
						TView * p_term = p_tgt->GetLastView();
						TView * p_temp = p_term;
						if(p_temp) do {
							p_temp = p_temp->P_Next;
							if(p_temp->IsInState(sfCloseMe)) {
								TView::messageCommand(p_temp, cmClose);
								is_there_closeme = 1;
								break;
							}
						} while(p_temp != p_term);
					}
				}
			}
			// } @v10.0.02
			break;
		case WM_TIMECHANGE:
			TView::messageCommand(static_cast<TProgram *>(TView::GetWindowUserData(hWnd)), cmTimeChange);
			break;
		case WM_DESTROY:
			p_pgm = static_cast<TProgram *>(TView::GetWindowUserData(hWnd));
			::DestroyWindow(p_pgm->H_FrameWnd);
			p_pgm->H_FrameWnd = 0;
			if(p_pgm->H_ShortcutsWnd) {
				HWND hwnd_tt = TabCtrl_GetToolTips(GetDlgItem(p_pgm->H_ShortcutsWnd, CTL_SHORTCUTS_ITEMS));
				::DestroyWindow(hwnd_tt);
				::DestroyWindow(p_pgm->H_ShortcutsWnd);
				p_pgm->H_ShortcutsWnd = 0;
			}
			TView::SetWindowProp(p_pgm->H_CloseWnd, GWLP_WNDPROC, p_pgm->PrevCloseWndProc);
			ZDELETE(p_pgm->P_Toolbar);
			PostQuitMessage(0);
			break;
		case WM_SETFOCUS:
			{
				p_pgm = static_cast<TProgram *>(TView::GetWindowUserData(hWnd));
				HWND hb = GetTopWindow(p_pgm->GetFrameWindow());
				if(hb == p_pgm->H_CloseWnd)
					hb = GetNextWindow(hb, GW_HWNDNEXT);
				if(hb)
					SetFocus(hb);
			}
			break;
		case WM_GETMINMAXINFO:
			{
				LPMINMAXINFO p_min_max = reinterpret_cast<LPMINMAXINFO>(lParam);
				p_min_max->ptMinTrackSize.x = 420;
				p_min_max->ptMinTrackSize.y = 300;
			}
			break;
		case WM_MOVE:
			p_pgm = static_cast<TProgram *>(TView::GetWindowUserData(hWnd));
			if(p_pgm->H_FrameWnd)
				PostMessage(p_pgm->H_FrameWnd, WM_MOVE, 0, 0);
			EnumWindows(SendMainWndSizeMessage, reinterpret_cast<LPARAM>(hWnd));
			return DefWindowProc(hWnd, message, wParam, lParam);
		//case WM_INPUTLANGCHANGE: {} break; // @v6.4.4 AHTOXA
		case WM_SYSCOMMAND:
			if(wParam == SC_CLOSE) {
				TView::messageCommand(static_cast<TProgram *>(TView::GetWindowUserData(hWnd)), cmQuit, &lParam);
				break;
			}
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

void TProgram::NotifyFrame(int post)
{
	HWND   h_frame = GetFrameWindow();
	if(IsWindow(h_frame))
		if(post)
			::PostMessage(h_frame, WM_USER_NOTIFYBRWFRAME, 0, 0);
		else
			::SendMessage(h_frame, WM_USER_NOTIFYBRWFRAME, 0, 0);
}

/* @v10.0.02
void TProgram::idle()
{
	TView::messageBroadcast(P_DeskTop, cmIdle);
	TView::messageBroadcast(this, cmIdle);
}*/

// Public variables

TProgram::TProgram(HINSTANCE hInst, const char * pAppSymb, const char * pAppTitle) : TGroup(TRect()),
	State(0), H_MainWnd(0), H_FrameWnd(0), H_CloseWnd(0), H_LogWnd(0), H_Desktop(0), H_ShortcutsWnd(0),
	H_TopOfStack(0), H_Accel(0), P_Stw(0), P_DeskTop(0), P_TopView(0), P_Toolbar(0), P_TreeWnd(0), AppSymbol(pAppSymb)
{
	hInstance = hInst;
	UICfg.Restore();
	(AppTitle = pAppTitle).SetIfEmpty(AppSymbol);
	Sf = (sfVisible | sfSelected | sfFocused | sfModal);
	ViewOptions = 0;
	P_DeskTop = new TGroup(TRect());
	application = this;
	H_Icon = LoadIcon(hInstance, MAKEINTRESOURCE(ICON_MAIN_P2));
	{
		WNDCLASSEX wc;
		INITWINAPISTRUCT(wc);
		wc.lpszClassName = SUcSwitch(AppSymbol);
		wc.hInstance     = hInstance;
		wc.lpfnWndProc   = static_cast<WNDPROC>(MainWndProc);
		wc.style = CS_HREDRAW | CS_VREDRAW;
		wc.hIcon = H_Icon;
		{
			// @v10.7.8 wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_GRAYTEXT);
			HBRUSH hbr_bkg = ::CreateSolidBrush(RGB(0x20, 0x63, 0x9b)); // @v10.7.8
			wc.hbrBackground = hbr_bkg; // @v10.7.8
		}
		wc.cbClsExtra    = sizeof(long);
		wc.cbWndExtra    = sizeof(long);
		::RegisterClassEx(&wc);
	}
	// @v11.2.4 WS_EX_COMPOSITED
	HWND   hWnd = ::CreateWindowEx(/*WS_EX_COMPOSITED*/0, SUcSwitch(AppSymbol), SUcSwitch(AppTitle),
		WS_OVERLAPPEDWINDOW|WS_EX_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, this);
	ShowWindow(hWnd, SW_SHOWMAXIMIZED/*SW_SHOWDEFAULT*/);
	UpdateWindow(hWnd);
}

TProgram::~TProgram()
{
	DestroyIcon(H_Icon);
	delete P_DeskTop;
	delete P_Toolbar;
}

HWND TProgram::GetFrameWindow() const
{
	return NZOR(H_FrameWnd, H_MainWnd);
}

IMPL_HANDLE_EVENT(TProgram)
{
	TGroup::handleEvent(event);
	if(event.what == TEvent::evCommand && event.message.command == cmQuit) {
		::DestroyWindow(H_MainWnd);
		clearEvent(event);
	}
}

int TProgram::MsgLoop(TWindow * pV, int & rExitSignal)
{
	rExitSignal = 0;
	MSG    msg;
	while(GetMessage(&msg, 0, 0, 0)) {
		if(!TranslateAccelerator(msg.hwnd, H_Accel, &msg)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		if(rExitSignal) {
			if(!pV || TestWindowForEndModal(pV))
				return 1;
			else
				rExitSignal = 0;
		}
	}
	return 0;
}

void TProgram::run()
{
	MSG    msg;
	H_Accel = LoadAccelerators(hInstance, MAKEINTRESOURCE(101));
	while(GetMessage(&msg, 0, 0, 0)) {
		if(!TranslateAccelerator(msg.hwnd, H_Accel, &msg)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
}

TRect TProgram::MakeCenterRect(int width, int height) const
{
	const int dx = (P_DeskTop->ViewSize.x - width) / 2;
	const int dy = (P_DeskTop->ViewSize.y - height) / 2;
	TRect r(dx, dy, width + dx, height + dy);
	return r;
}

TView * TProgram::validView(TView * p)
{
	if(p && !p->valid(cmValid)) {
		ZDELETE(p);
	}
	return p;
}

/*static*/HINSTANCE TProgram::GetInst()
{
	return TProgram::hInstance;
}

/*static*/void TProgram::IdlePaint()
{
	MSG msg;
	while(PeekMessage(&msg, NULL, WM_PAINT, WM_PAINT, PM_REMOVE)) { // @v9.9.5 (WM_PAINT, WM_PAINT)
		if(msg.message == WM_PAINT) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
}

struct Entry {
	HWND   FirstChild;
	HWND   Parent;
	int    ViewKind;
	int    WindowType;
	int    CaptionHeight;
	RECT   ParentRect;
};

BOOL CALLBACK EnumCtrls(HWND hWnd, LPARAM lParam)
{
	BOOL   r = true;
	Entry * p_e = reinterpret_cast<Entry *>(lParam);
	if(p_e && p_e->FirstChild != hWnd) {
		TCHAR  cls_name[64];
		long   style    = TView::GetWindowStyle(hWnd);
		long   ex_style = TView::GetWindowExStyle(hWnd);
		memzero(cls_name, sizeof(cls_name));
		RealGetWindowClass(hWnd, cls_name, SIZEOFARRAY(cls_name));
		if(p_e->ViewKind == UserInterfaceSettings::wndVKFlat) {
			ex_style |= WS_EX_STATICEDGE;
			if(sstreqi_ascii(cls_name, "BUTTON"))
				style |= BS_FLAT;
			else if(sstreqi_ascii(cls_name, "EDIT")) {
				TView::SetWindowProp(hWnd, GWL_STYLE, style | WS_BORDER);
				style &= ~WS_BORDER;
				ex_style &= ~WS_EX_DLGMODALFRAME;
			}
			TView::SetWindowProp(hWnd, GWL_STYLE, style);
			TView::SetWindowProp(hWnd, GWL_EXSTYLE, ex_style);
			if(p_e->WindowType == TProgram::wndtypDialog) {
				if(GetParent(hWnd) == p_e->Parent) {
					RECT r;
					MEMSZERO(r);
					GetWindowRect(hWnd, &r);
					r.top    += (p_e->CaptionHeight - p_e->ParentRect.top);
					r.left   -= p_e->ParentRect.left;
					r.right  -= p_e->ParentRect.left;
					r.bottom += (p_e->CaptionHeight - p_e->ParentRect.top);
					MoveWindow(hWnd, r.left, r.top, r.right - r.left, r.bottom - r.top, 1);
				}
			}
		}
		else if(oneof2(p_e->ViewKind, UserInterfaceSettings::wndVKFancy, UserInterfaceSettings::wndVKVector)) {
			if(sstreqi_ascii(cls_name, "BUTTON") && (style & BS_AUTOCHECKBOX) != BS_AUTOCHECKBOX && (style & BS_AUTORADIOBUTTON) != BS_AUTORADIOBUTTON)
				style |= BS_OWNERDRAW;
			else if(sstreqi_ascii(cls_name, "EDIT")) {
				ex_style &= ~WS_EX_STATICEDGE;
				TView::SetWindowProp(hWnd, GWL_EXSTYLE, ex_style);
			}
			TView::SetWindowProp(hWnd, GWL_STYLE, style);
		}
		else
			r = false;
		SETIFZ(p_e->FirstChild, hWnd);
	}
	else
		r = false;
	return r;
}

/*static*/static LRESULT CALLBACK SpecTitleWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	WNDPROC prev_wnd_proc = reinterpret_cast<WNDPROC>(TView::GetWindowUserData(hWnd));
	switch(message) {
		case WM_DESTROY:
			TView::SetWindowProp(hWnd, GWLP_WNDPROC, prev_wnd_proc);
			break;
		case WM_SETFOCUS:
			return 0;
		case WM_CTLCOLORSTATIC:
		    {
	       		LOGBRUSH lb;
         		lb.lbColor = RGB(255, 0, 0);
         		lb.lbStyle = BS_SOLID;
         		return reinterpret_cast<LRESULT>(CreateBrushIndirect(&lb));
      		}
	}
	return prev_wnd_proc(hWnd, message, wParam, lParam);
}

int TProgram::SetWindowViewByKind(HWND hWnd, int wndType)
{
	int    ok = 1;
	if(hWnd) {
		Entry e;
		MEMSZERO(e);
		e.ViewKind      = UICfg.WindowViewStyle;
		e.WindowType    = wndType;
		e.CaptionHeight = GetSystemMetrics(SM_CYCAPTION);
		e.Parent = hWnd;
		if(UICfg.WindowViewStyle == UserInterfaceSettings::wndVKFlat) {
			long ex_style = TView::GetWindowExStyle(hWnd) | WS_EX_STATICEDGE;
			long style = TView::GetWindowStyle(hWnd);
			if(oneof2(wndType, TProgram::wndtypDialog, TProgram::wndtypListDialog)) {
				style &= ~(WS_CAPTION|DS_MODALFRAME|WS_SYSMENU);
				style |= WS_BORDER;
				TView::SetWindowProp(hWnd, GWL_STYLE, style);
				ex_style &= ~WS_EX_DLGMODALFRAME;
				if(wndType == TProgram::wndtypDialog) {
					HWND btn_hwnd = 0, title_hwnd = 0;
					TDialog * p_dlg = reinterpret_cast<TDialog *>(TView::GetWindowUserData(hWnd));
					RECT r;
					MEMSZERO(r);
					GetWindowRect(hWnd, &r);
					e.ParentRect = r;
					r.bottom += e.CaptionHeight;
					MoveWindow(hWnd, r.left, r.top, r.right - r.left, r.bottom - r.top, 1);
					// add title text
					{
						SString title_buf;
						SString font_face;
						RECT title_rect;
						HDC hdc = 0;
						TView::SGetWindowText(hWnd, title_buf);
						title_rect.top = -e.CaptionHeight;
						title_rect.left    = 10;
						title_rect.right   = r.right - r.left - 50;
						title_rect.bottom  = 24;
						title_hwnd = ::CreateWindowEx(0, _T("STATIC"), SUcSwitch(title_buf), WS_CHILD,
							title_rect.left, title_rect.top, title_rect.right, title_rect.bottom, hWnd, 0, TProgram::hInstance, 0);
						font_face = "MS Sans Serif";
						TView::setFont(title_hwnd, font_face, 24);
						TView::SetWindowProp(title_hwnd, GWL_STYLE, TView::GetWindowStyle(title_hwnd) & ~WS_TABSTOP);
						TView::SetWindowProp(title_hwnd, GWL_ID, SPEC_TITLEWND_ID);
						TView::SetWindowProp(title_hwnd, GWLP_USERDATA, TView::SetWindowProp(title_hwnd, GWLP_WNDPROC, SpecTitleWndProc));
						ShowWindow(title_hwnd, SW_SHOWNORMAL);
						ReleaseDC(title_hwnd, hdc);
					}
					// add close button
					{
						TButton * p_btn  = new TButton(TRect(10, 10, 10, 10), 0, cmCancel, 0, CLOSEBTN_BITMAPID);
						p_btn->Parent = hWnd;
						p_dlg->Insert_(&p_btn->SetId(SPEC_TITLEWND_ID + 1));
						btn_hwnd = ::CreateWindowEx(0, _T("BUTTON"), NULL, WS_CHILD|BS_NOTIFY|BS_BITMAP,
							r.right - r.left - 30, -e.CaptionHeight, 16, 16, hWnd, 0, TProgram::hInstance, 0);
						{
							HBITMAP h_bm = APPL->FetchBitmap(CLOSEBTN_BITMAPID);
							::SendMessage(btn_hwnd, BM_SETIMAGE, IMAGE_BITMAP, reinterpret_cast<LPARAM>(h_bm));
						}
						TView::SetWindowProp(btn_hwnd, GWL_STYLE, TView::GetWindowStyle(btn_hwnd) & ~WS_TABSTOP);
						TView::SetWindowProp(btn_hwnd, GWL_ID, SPEC_TITLEWND_ID + 1);
						ShowWindow(btn_hwnd, SW_SHOWNORMAL);
						MoveWindow(p_btn->getHandle(), r.right - r.left - 30, -e.CaptionHeight, 16, 16, 1);
					}
				}
			}
			TView::SetWindowProp(hWnd, GWL_EXSTYLE, ex_style);
			while(EnumChildWindows(hWnd, EnumCtrls, reinterpret_cast<LPARAM>(&e)) != 0)
				;
		}
		else if(oneof2(APPL->UICfg.WindowViewStyle, UserInterfaceSettings::wndVKFancy, UserInterfaceSettings::wndVKVector)) {
			HDC dc = GetDC(hWnd);
			SetBkColor(dc, RGB(0xDD, 0xDD, 0xF1));
			ReleaseDC(hWnd, dc);
			while(EnumChildWindows(hWnd, EnumCtrls, reinterpret_cast<LPARAM>(&e)) != 0)
				;
		}
	}
	return ok;
}

/*
	enum {
        tbiDummyFirst = 1,
        //
        tbiButtonBrush,
        tbiButtonFocBrush,
        tbiButtonSelBrush,
        tbiButtonDsblBrush,
        tbiButtonHvrBrush,
        tbiButtonPen,
        tbiButtonFocPen,
        tbiButtonSelPen,
        tbiButtonDsblPen,
        tbiButtonHvrPen,
        tbiButtonFont
        //
	};
*/
int TProgram::InitUiToolBox()
{
	int    ok = 1;
	if(!(State & stUiToolBoxInited)) {
		ENTER_CRITICAL_SECTION
		LoadVectorTools(&DvToolList);
		if(!(State & stUiToolBoxInited)) {
			UiToolBox.CreateColor(tbiButtonTextColor, SColor(SClrBlack));
			UiToolBox.CreateColor(tbiButtonTextColor+tbisDisable, SColor(SClrWhite));
			UiToolBox.CreateColor(tbiIconRegColor,     SColor(/*0x06, 0xAE, 0xD5*/0x00, 0x49, 0x82)); // 004982
			UiToolBox.CreateColor(tbiIconAlertColor,   SColor(0xDD, 0x1C, 0x1A));
			UiToolBox.CreateColor(tbiIconAccentColor,  SColor(0x2A, 0x9D, 0x8F));
			UiToolBox.CreateColor(tbiIconPassiveColor, SColor(0xFF, 0xF1, 0xD0));
			UiToolBox.CreatePen(tbiBlackPen,         SPaintObj::psSolid, 1.0f, SClrBlack); // @v10.3.0
			UiToolBox.CreatePen(tbiWhitePen,         SPaintObj::psSolid, 1.0f, SClrWhite); // @v10.3.0
			UiToolBox.CreateBrush(tbiInvalInpBrush,  SPaintObj::bsSolid, SClrCrimson, 0); // @v10.2.4
			UiToolBox.CreateBrush(tbiInvalInp2Brush, SPaintObj::bsSolid, SColor(0xff, 0x99, 0x00) /*https://www.colorhexa.com/ff9900*/, 0); // @v10.3.0
			UiToolBox.CreateBrush(tbiInvalInp3Brush, SPaintObj::bsSolid, SColor(0xff, 0x33, 0xcc) /*https://www.colorhexa.com/ff33cc*/, 0); // @v10.3.0
			UiToolBox.CreateBrush(tbiListBkgBrush,   SPaintObj::bsSolid, SClrWhite, 0); // @v10.3.0
			UiToolBox.CreatePen(tbiListBkgPen,       SPaintObj::psSolid, 1.0f, SClrWhite); // @v10.3.0
			UiToolBox.CreateBrush(tbiListFocBrush,   SPaintObj::bsSolid, SColor(0x00, 0x66, 0xcc) /*https://www.colorhexa.com/0066cc*/, 0); // @v10.3.0
			UiToolBox.CreatePen(tbiListFocPen,       SPaintObj::psSolid, 1.0f, SColor(0x00, 0x66, 0xcc) /*https://www.colorhexa.com/0066cc*/); // @v10.3.0
			UiToolBox.CreateBrush(tbiListSelBrush,   SPaintObj::bsSolid, SColor(0xa2, 0xd2, 0xff) /*https://www.colorhexa.com/0066cc*/, 0); // @v10.3.0
			UiToolBox.CreatePen(tbiListSelPen,       SPaintObj::psDot, 1.0f, SColor(0x00, 0x66, 0xcc) /*https://www.colorhexa.com/0066cc*/); // @v10.3.0
			{
				// linear-gradient(to bottom, #f0f9ff 0%,#cbebff 47%,#a1dbff 100%)
				/*
				FRect gr;
				gr.a.Set(0.0f, 0.0f);
				gr.b.Set(0.0f, 10.0f);
				int   gradient = UiToolBox.CreateGradientLinear(0, gr);
				UiToolBox.AddGradientStop(gradient, 0.00f, SColor(0xf0, 0xf9, 0xff));
				UiToolBox.AddGradientStop(gradient, 0.47f, SColor(0xcb, 0xeb, 0xff));
				UiToolBox.AddGradientStop(gradient, 1.00f, SColor(0xa1, 0xdb, 0xff));
				UiToolBox.CreateBrush(tbiButtonBrush, SPaintObj::bsPattern, SColor(0xDC, 0xD9, 0xD1), 0, gradient);
				*/
				UiToolBox.CreateBrush(tbiButtonBrush, SPaintObj::bsSolid, SColor(0xDC, 0xD9, 0xD1), 0);
			}
			UiToolBox.CreateBrush(tbiButtonBrush+tbisSelect, SPaintObj::bsSolid, SColor(0xBA, 0xBA, 0xC9), 0);
			UiToolBox.CreatePen(tbiButtonPen,             SPaintObj::psSolid, 1, UiToolBox.GetColor(tbiIconRegColor)/*SColor(0x47, 0x47, 0x3D)*/);
			UiToolBox.CreatePen(tbiButtonPen+tbisDefault, SPaintObj::psSolid, 1, SClrGreen);
			UiToolBox.CreatePen(tbiButtonPen+tbisFocus,   SPaintObj::psSolid, 1, /*SColor(0x15, 0x20, 0xEA)*/SClrOrange); // SColor(0xE5, 0xC3, 0x65)
			UiToolBox.CreatePen(tbiButtonPen+tbisSelect,  SPaintObj::psSolid, 1, /*SColor(0x15, 0x20, 0xEA)*/SClrOrange);
			UiToolBox.CreatePen(tbiButtonPen+tbisDisable, SPaintObj::psSolid, 1, SColor(SClrWhite));
			UiToolBox.SetBrush(tbiButtonBrush_F, SPaintObj::bsSolid, SColor(0xDC, 0xD9, 0xD1), 0);
			UiToolBox.SetBrush(tbiButtonBrush_F+tbisSelect, SPaintObj::bsSolid, SColor(0xBA, 0xBA, 0xC9), 0);
			UiToolBox.SetPen(tbiButtonPen_F, SPaintObj::psSolid, 1, SColor(0x47, 0x47, 0x3D));
			UiToolBox.SetPen(tbiButtonPen_F+tbisFocus,  SPaintObj::psSolid, 1, SColor(0x15, 0x20, 0xEA));
			UiToolBox.SetPen(tbiButtonPen_F+tbisSelect, SPaintObj::psSolid, 1, SColor(0x15, 0x20, 0xEA));
			// @v11.2.3 {
			{
				UiToolBox.CreateFont_(tbiControlFont, "Verdana", 11, 0); // ! Не использовать "MS Sans Serif"
				/*
				HFONT hf = 0;
				{
					LOGFONT lf;
					MEMSZERO(lf);
					lf.lfCharSet = DEFAULT_CHARSET;
					lf.lfHeight  = 9;
					STRNSCPY(lf.lfFaceName, _T("MS Sans Serif"));
					hf = ::CreateFontIndirect(&lf);
				}
				if(hf) {
					UiToolBox.SetFont(tbiControlFont, hf);
				}
				*/
			}
			// } @v11.2.3
			State |= stUiToolBoxInited;
		}
        LEAVE_CRITICAL_SECTION
	}
	else
		ok = -1;
	return ok;
}

/*static*/void TProgram::DrawTransparentBitmap(HDC hdc, HBITMAP hBitmap, const RECT & rDestRect, long xOffs, long yOffs, COLORREF cTransparentColor, COLORREF newBkgndColor, long fmt, POINT * pBmpSize)
{
	long   x_pos = 0, y_pos = 0;
	BITMAP     bm;
	COLORREF   cColor;
	HBITMAP    bmAndBack, bmAndObject, bmAndMem, bmSave;
	HBITMAP    bmBackOld, bmObjectOld, bmMemOld, bmSaveOld;
	HDC    hdcMem, hdcBack, hdcObject, hdcSave;
	POINT  ptSize;
	HDC    hdcTemp = CreateCompatibleDC(hdc);
	SelectObject(hdcTemp, hBitmap);
	// Выбираем битмап
	GetObject(hBitmap, sizeof(BITMAP), (LPSTR)&bm);
	ptSize.x = bm.bmWidth;            // Получаем ширину битмапа
	ptSize.y = bm.bmHeight;           // Получаем высоту битмапа
	DPtoLP(hdcTemp, &ptSize, 1);      // Конвертируем из координат устройства в логические точки
	y_pos = ((fmt & DT_VCENTER) ? ((rDestRect.bottom - rDestRect.top  - ptSize.y) / 2) : rDestRect.top)  + yOffs;
	x_pos = ((fmt & DT_CENTER)  ? ((rDestRect.right  - rDestRect.left - ptSize.x) / 2) : rDestRect.left) + xOffs;
	//
	// Создаём несколько DC для хранения временных данных.
	hdcBack   = CreateCompatibleDC(hdc);
	hdcObject = CreateCompatibleDC(hdc);
	hdcMem    = CreateCompatibleDC(hdc);
	hdcSave   = CreateCompatibleDC(hdc);
	// Создаём битмап для каждого DC.
	// Монохромный DC
	bmAndBack   = CreateBitmap(ptSize.x, ptSize.y, 1, 1, 0);
	// Монохромный DC
	bmAndObject = CreateBitmap(ptSize.x, ptSize.y, 1, 1, 0);
	bmAndMem    = CreateCompatibleBitmap(hdc, ptSize.x, ptSize.y);
	bmSave      = CreateCompatibleBitmap(hdc, ptSize.x, ptSize.y);
	// В каждом DC должен быть выбран объект битмапа для хранения пикселей.
	bmBackOld   = static_cast<HBITMAP>(SelectObject(hdcBack, bmAndBack));
	bmObjectOld = static_cast<HBITMAP>(SelectObject(hdcObject, bmAndObject));
	bmMemOld    = static_cast<HBITMAP>(SelectObject(hdcMem, bmAndMem));
	bmSaveOld   = static_cast<HBITMAP>(SelectObject(hdcSave, bmSave));
	SetMapMode(hdcTemp, GetMapMode(hdc)); // Устанавливаем режим маппинга.
	BitBlt(hdcSave, 0, 0, ptSize.x, ptSize.y, hdcTemp, 0, 0, SRCCOPY); // Сохраняем битмап, переданный в параметре функции, так как он будет изменён.
	cColor = SetBkColor(hdcTemp, cTransparentColor); // Устанавливаем фоновый цвет (в исходном DC) тех частей, которые будут прозрачными.
	/*
	if(newBkgndColor >= 0)
		cColor = newBkgndColor;
	*/
	BitBlt(hdcObject, 0, 0, ptSize.x, ptSize.y, hdcTemp, 0, 0, SRCCOPY); // Создаём маску для битмапа путём вызова BitBlt из исходного битмапа на монохромный битмап.
	SetBkColor(hdcTemp, cColor); // Устанавливаем фоновый цвет исходного DC обратно в оригинальный цвет.
	BitBlt(hdcBack, 0, 0, ptSize.x, ptSize.y, hdcObject, 0, 0, NOTSRCCOPY); // Создаём инверсию маски.
	BitBlt(hdcMem, 0, 0, ptSize.x, ptSize.y, hdc, x_pos, y_pos, SRCCOPY); // Копируем фон главного DC в конечный.
	BitBlt(hdcMem, 0, 0, ptSize.x, ptSize.y, hdcObject, 0, 0, SRCAND); // Накладываем маску на те места, где будет помещён битмап.
	BitBlt(hdcTemp, 0, 0, ptSize.x, ptSize.y, hdcBack, 0, 0, SRCAND); // Накладываем маску на прозрачные пиксели битмапа.
	BitBlt(hdcMem, 0, 0, ptSize.x, ptSize.y, hdcTemp, 0, 0, SRCPAINT); // XOR-им битмап с фоном на конечном DC.
	BitBlt(hdc, x_pos, y_pos, ptSize.x, ptSize.y, hdcMem, 0, 0, SRCCOPY); // Копируем на экран.
	BitBlt(hdcTemp, 0, 0, ptSize.x, ptSize.y, hdcSave, 0, 0, SRCCOPY); // Помещаем оригинальный битмап обратно в битмап, переданный в параметре функции.
	// Удаляем битмапы из памяти.
	::DeleteObject(SelectObject(hdcBack, bmBackOld));
	::DeleteObject(SelectObject(hdcObject, bmObjectOld));
	::DeleteObject(SelectObject(hdcMem, bmMemOld));
	::DeleteObject(SelectObject(hdcSave, bmSaveOld));
	// Удаляем DC из памяти.
	DeleteDC(hdcMem);
	DeleteDC(hdcBack);
	DeleteDC(hdcObject);
	DeleteDC(hdcSave);
	DeleteDC(hdcTemp);
	ASSIGN_PTR(pBmpSize, ptSize);
}

enum _Asset {
    _assetCtrlBorderColor = 1,
    _assetCtrlFocusedBorderColor = 2
};

static COLORREF _GetAssetColor(int _asset)
{
	if(_asset == _assetCtrlBorderColor)
		return RGB(0x47, 0x47, 0x3D); // RGB(0x9C, 0x9C, 0xA8);
	else if(_asset == _assetCtrlFocusedBorderColor)
		return RGB(0x15, 0x20, 0xEA); // RGB(0x66, 0x33, 0xFF);
	else
		return RGB(0xff, 0xff, 0xff);
}

int DrawCluster(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	int    ok = -1;
	int    draw_checkbox = 0;
	int    draw_radiobtn = 0;
	long   checkbox_size = 14;
	const DRAWITEMSTRUCT * p_di = reinterpret_cast<const DRAWITEMSTRUCT *>(lParam);
	int    focused = BIN(p_di->itemAction == ODA_FOCUS || (p_di->itemState & ODS_FOCUS));
	int    selected = BIN(p_di->itemState & ODS_SELECTED && p_di->itemAction == ODA_SELECT);
	int    disabled = BIN(p_di->itemState & ODS_DISABLED);
	long   style = TView::GetWindowStyle(p_di->hwndItem);
	long   text_out_fmt = DT_SINGLELINE|DT_VCENTER|DT_EXTERNALLEADING|DT_LEFT;
	RECT   out_r = p_di->rcItem, elem_r = p_di->rcItem;
	COLORREF brush_color = RGB(0xDD, 0xDD, 0xF1);
	COLORREF pen_color   = RGB(0xDD, 0xDD, 0xF1);
	COLORREF text_color  = GetColorRef(SClrBlack);
	COLORREF adv_brush_color = RGB(0xF2, 0xF2, 0xF7);
	COLORREF adv_pen_color = _GetAssetColor(_assetCtrlBorderColor);
	HBRUSH brush = 0, old_brush = 0;
	HPEN   pen   = 0, old_pen   = 0;
	SString text_buf;
	TView::SGetWindowText(p_di->hwndItem, text_buf);
	if(disabled) {
		adv_brush_color = RGB(0xDD, 0xCC, 0xCC);
		text_color = RGB(0xFF, 0xFF, 0xFF);
	}
	else if(focused)
		pen_color = _GetAssetColor(_assetCtrlFocusedBorderColor);
	else if(selected)
		adv_brush_color = RGB(0xBA, 0xBA, 0xC9);
	out_r.left += checkbox_size + 5;
	if(p_di->CtlType == ODT_RADIOBTN)
		draw_radiobtn = 1;
	else
		draw_checkbox = 1;
	SETIFZ(brush, CreateSolidBrush(brush_color));
	old_brush = static_cast<HBRUSH>(::SelectObject(p_di->hDC, brush));
	SETIFZ(pen, CreatePen(PS_SOLID, 1, pen_color));
	old_pen   = static_cast<HPEN>(SelectObject(p_di->hDC, pen));
	ok = Rectangle(p_di->hDC, elem_r.left, elem_r.top, elem_r.right, elem_r.bottom);
	if(ok > 0) {
		elem_r.left =+ 2;
		elem_r.top = (elem_r.bottom - elem_r.top) / 2 - checkbox_size / 2;
		ZDeleteWinGdiObject(&brush);
		ZDeleteWinGdiObject(&pen);
		SETIFZ((brush = 0), CreateSolidBrush(adv_brush_color));
		SelectObject(p_di->hDC, brush);
		SETIFZ((pen = 0), CreatePen(PS_SOLID, 1, adv_pen_color));
		SelectObject(p_di->hDC, pen);
		if(draw_checkbox) {
			ok = Rectangle(p_di->hDC, elem_r.left, elem_r.top, elem_r.left + checkbox_size, elem_r.top + checkbox_size);
			if(p_di->itemState & ODS_SELECTED) {
				ZDeleteWinGdiObject(&pen);
				SETIFZ((pen = 0), CreatePen(PS_SOLID, 2, RGB(0x00, 0x00, 0x00)));
				SelectObject(p_di->hDC, pen);
				MoveToEx(p_di->hDC, elem_r.left + 4,     elem_r.top + 4, (LPPOINT) NULL);
       			LineTo(p_di->hDC,   elem_r.left + checkbox_size - 4, elem_r.top + checkbox_size - 4);
       			MoveToEx(p_di->hDC, elem_r.left + checkbox_size - 4, elem_r.top + 4, (LPPOINT) NULL);
       			LineTo(p_di->hDC,   elem_r.left + 4,     elem_r.top + checkbox_size - 4);
			}
		}
		else {
			ok = Ellipse(p_di->hDC, elem_r.left, elem_r.top, elem_r.left + checkbox_size, elem_r.top + checkbox_size);
			if(p_di->itemState & ODS_SELECTED) {
				ZDeleteWinGdiObject(&pen);
				SETIFZ((pen = 0), CreatePen(PS_SOLID, 3, GetColorRef(SClrBlack)));
				SelectObject(p_di->hDC, pen);
				ok = Ellipse(p_di->hDC, elem_r.left + checkbox_size / 2, elem_r.top + checkbox_size / 2,  elem_r.left + checkbox_size / 2 + 2, elem_r.top + checkbox_size / 2 + 2);
			}
		}
	}
	{
		HFONT old_font = 0;
		old_font = static_cast<HFONT>(::SelectObject(p_di->hDC, reinterpret_cast<HFONT>(::SendMessage(p_di->hwndItem, WM_GETFONT, 0, 0))));
		SetBkMode(p_di->hDC, TRANSPARENT);
		SetTextColor(p_di->hDC, text_color);
		::DrawText(p_di->hDC, SUcSwitch(text_buf), (int)text_buf.Len(), &out_r, text_out_fmt);
		if(old_font)
			SelectObject(p_di->hDC, old_font);
	}
	if(old_brush)
		SelectObject(p_di->hDC, old_brush);
	ZDeleteWinGdiObject(&brush);
	if(old_pen)
		SelectObject(p_di->hDC, old_pen);
	ZDeleteWinGdiObject(&pen);
	return ok;
}

int DrawButton(HWND hwnd, DRAWITEMSTRUCT * pDi)
{
	int    ok = -1;
	int    draw_bitmap = 0;
	const int  focused  = BIN(pDi->itemAction == ODA_FOCUS || (pDi->itemState & ODS_FOCUS));
	const int  selected = BIN(pDi->itemState & ODS_SELECTED && pDi->itemAction == ODA_SELECT);
	const int  disabled = BIN(pDi->itemState & ODS_DISABLED);
	const long style = TView::GetWindowStyle(pDi->hwndItem);
	long   text_out_fmt = /*DT_SINGLELINE|*/DT_VCENTER|DT_EXTERNALLEADING|DT_END_ELLIPSIS;
	RECT   out_r = pDi->rcItem;
	RECT   elem_r = pDi->rcItem;
	COLORREF brush_color = RGB(0xDC, 0xD9, 0xD1);
	COLORREF pen_color = _GetAssetColor(_assetCtrlBorderColor);
	COLORREF text_color = GetColorRef(SClrBlack);
	HBITMAP hbmp = 0;
	HBRUSH brush = 0, old_brush = 0;
	HPEN   pen   = 0, old_pen   = 0;
	SString text_buf;
	TView::SGetWindowText(pDi->hwndItem, text_buf);
	const int draw_text = BIN(text_buf.Len());
	int    erase_text = 0;
	{
		TButton * p_btn = static_cast<TButton *>(TView::GetWindowUserData(pDi->hwndItem));
		if(disabled)
			text_color = GetColorRef(SClrWhite);
		else if(selected) {
			out_r.top  += (style & BS_BITMAP) ? 1 : 2;
			out_r.left += (style & BS_BITMAP) ? 1 : 2;
			brush_color = RGB(0xBA, 0xBA, 0xC9);
			pen_color = _GetAssetColor(_assetCtrlFocusedBorderColor);
		}
		else if(focused)
			pen_color = _GetAssetColor(_assetCtrlFocusedBorderColor);//RGB(0x66, 0x33, 0xFF);
		if(style & BS_BITMAP)
			hbmp = reinterpret_cast<HBITMAP>(::SendMessage(pDi->hwndItem, BM_GETIMAGE, IMAGE_BITMAP, 0));
		else if(p_btn) {
			uint bmp_id = p_btn->GetBmpID();
			if(!bmp_id) {
				const uint button_id = p_btn->GetId();
				switch(button_id) {
					case STDCTL_CANCELBUTTON:    bmp_id = IDB_CLOSE; break;
					case STDCTL_OKBUTTON:        bmp_id = IDB_OK; break;
					case STDCTL_INSBUTTON:       bmp_id = IDB_INS_448; break;
					case STDCTL_DELBUTTON:       bmp_id = IDB_DEL_448; break;
					case STDCTL_EDITBUTTON:      bmp_id = IDB_EDIT_448; break;
					case STDCTL_PRINT:
					case STDCTL_PRINTBUTTON:     bmp_id = IDB_PRINT_448; break;
					case STDCTL_AGREEMENTBUTTON: bmp_id = BM_AGREEMENT; break;
					case STDCTL_TAGSBUTTON:      bmp_id = TBBM_TAGS; break;
					case STDCTL_REGISTERSBUTTON: bmp_id = TBBM_REGISTER; break;
					case STDCTL_VIEWOPTBUTTON:   bmp_id = TBBM_VIEWOPTIONS; break;
					case STDCTL_PERSONBUTTON:    bmp_id = BM_PERSON; break;
					default:
						if(text_buf == "*") {
							bmp_id = BM_ASTERISK;
							erase_text = 1;
						}
						else if(text_buf == "...") {
							bmp_id = BM_ELLIPSIS;
							erase_text = 1;
						}
						break;
				}
			}
			if(bmp_id && !p_btn->GetBitmap())
				p_btn->LoadBitmap_(bmp_id);
			hbmp = p_btn->GetBitmap();
		}
		if(draw_text) {
			out_r.left += 4;
			out_r.top  += 2;
		}
		draw_bitmap = BIN(hbmp);
	}
	SETIFZ(brush, CreateSolidBrush(brush_color));
	old_brush = static_cast<HBRUSH>(::SelectObject(pDi->hDC, brush));
	SETIFZ(pen, CreatePen(PS_SOLID, 1, pen_color));
	old_pen   = static_cast<HPEN>(SelectObject(pDi->hDC, pen));
	ok = RoundRect(pDi->hDC, elem_r.left, elem_r.top, elem_r.right, elem_r.bottom, ROUNDRECT_RADIUS * 2, ROUNDRECT_RADIUS * 2);
	if(draw_text || draw_bitmap) {
		SETFLAG(text_out_fmt, DT_CENTER, !(draw_text * draw_bitmap));
		SETFLAG(text_out_fmt, DT_LEFT, draw_text && draw_bitmap);
	}
	if(draw_bitmap) {
        if(!disabled) {
			POINT bmp_size;
			TProgram::DrawTransparentBitmap(pDi->hDC, hbmp, elem_r, out_r.left, out_r.top, RGB(0xD4, 0xD0, 0xC8), -1, text_out_fmt, &bmp_size);
			out_r.left += bmp_size.x + 2;
		}
	}
	if(erase_text) {
		TView::SSetWindowText(pDi->hwndItem, text_buf.Z());
	}
	else if(draw_text) {
		HFONT old_font = static_cast<HFONT>(::SelectObject(pDi->hDC, reinterpret_cast<HFONT>(::SendMessage(pDi->hwndItem, WM_GETFONT, 0, 0))));
		SetBkMode(pDi->hDC, TRANSPARENT);
		SetTextColor(pDi->hDC, text_color);
		if(style & BS_MULTILINE) {
			long text_h = 0, height = 0;
			RECT text_rect;
			TEXTMETRIC tm;
			GetTextMetrics(pDi->hDC, &tm);
			height = tm.tmHeight + 2;
			text_h = (out_r.bottom - out_r.top) / height;
			SplitBuf(pDi->hDC, text_buf, out_r.right - out_r.left, text_h);
			StringSet ss('\n', text_buf);
			text_rect.top    = out_r.top;
			text_rect.bottom = height;
			text_rect.left   = out_r.left;
			text_rect.right  = out_r.right;
			for(uint i = 0; ss.get(&i, text_buf); text_rect.top += height, text_rect.bottom += height)
				::DrawText(pDi->hDC, SUcSwitch(text_buf), (int)text_buf.Len(), &text_rect, text_out_fmt);
		}
		else {
			text_out_fmt |= DT_SINGLELINE;
			::DrawText(pDi->hDC, SUcSwitch(text_buf), (int)text_buf.Len(), &out_r, text_out_fmt);
		}
		if(old_font)
			SelectObject(pDi->hDC, old_font);
	}
	if(old_brush)
		SelectObject(pDi->hDC, old_brush);
	ZDeleteWinGdiObject(&brush);
	if(old_pen)
		SelectObject(pDi->hDC, old_pen);
	ZDeleteWinGdiObject(&pen);
	return ok;
}

int TProgram::DrawButton2(HWND hwnd, DRAWITEMSTRUCT * pDi)
{
    InitUiToolBox();

	int    ok = -1;
	int    draw_bitmap = 0;
	int    item_state = tbisBase;
	if(pDi->itemState & ODS_DISABLED)
		item_state = tbisDisable;
	else if(pDi->itemState & ODS_SELECTED && pDi->itemAction == ODA_SELECT)
		item_state = tbisSelect;
	else if(pDi->itemAction == ODA_FOCUS || (pDi->itemState & ODS_FOCUS))
		item_state = tbisFocus;
	const long style = TView::GetWindowStyle(pDi->hwndItem);
	long   text_out_fmt = /*DT_SINGLELINE|*/DT_VCENTER|DT_EXTERNALLEADING|DT_END_ELLIPSIS;
	const  TRect rect_elem = pDi->rcItem;
	RECT   out_r = pDi->rcItem;
	//COLORREF brush_color = RGB(0xDC, 0xD9, 0xD1);
	//COLORREF pen_color = _GetAssetColor(_assetCtrlBorderColor);
	HBITMAP hbmp = 0;
	SString text_buf;
	TView::SGetWindowText(pDi->hwndItem, text_buf);
	const int draw_text = BIN(text_buf.Len());
	int    erase_text = 0;
	{
		TButton * p_btn = static_cast<TButton *>(TView::GetWindowUserData(pDi->hwndItem));
		if(item_state == tbisSelect) {
			out_r.top  += (style & BS_BITMAP) ? 1 : 2;
			out_r.left += (style & BS_BITMAP) ? 1 : 2;
		}
		if(style & BS_BITMAP)
			hbmp = reinterpret_cast<HBITMAP>(::SendMessage(pDi->hwndItem, BM_GETIMAGE, IMAGE_BITMAP, 0));
		else if(p_btn) {
			uint bmp_id = p_btn->GetBmpID();
			if(!bmp_id) {
				const uint button_id = p_btn->GetId();
				switch(button_id) {
					case STDCTL_CANCELBUTTON:    bmp_id = IDB_CLOSE; break;
					case STDCTL_OKBUTTON:        bmp_id = IDB_OK; break;
					case STDCTL_INSBUTTON:       bmp_id = IDB_INS_448; break;
					case STDCTL_DELBUTTON:       bmp_id = IDB_DEL_448; break;
					case STDCTL_EDITBUTTON:      bmp_id = IDB_EDIT_448; break;
					case STDCTL_PRINT:
					case STDCTL_PRINTBUTTON:     bmp_id = IDB_PRINT_448; break;
					case STDCTL_AGREEMENTBUTTON: bmp_id = BM_AGREEMENT; break;
					case STDCTL_TAGSBUTTON:      bmp_id = TBBM_TAGS; break;
					case STDCTL_REGISTERSBUTTON: bmp_id = TBBM_REGISTER; break;
					case STDCTL_VIEWOPTBUTTON:   bmp_id = TBBM_VIEWOPTIONS; break;
					case STDCTL_PERSONBUTTON:    bmp_id = BM_PERSON; break;
					default:
						if(text_buf == "*") {
							bmp_id = BM_ASTERISK;
							erase_text = 1;
						}
						else if(text_buf == "...") {
							bmp_id = BM_ELLIPSIS;
							erase_text = 1;
						}
						break;
				}
			}
			if(bmp_id && !p_btn->GetBitmap())
				p_btn->LoadBitmap_(bmp_id);
			hbmp = p_btn->GetBitmap();
		}
		if(draw_text) {
			out_r.left += 4;
			out_r.top  += 2;
		}
		draw_bitmap = BIN(hbmp);
	}
	{
		TCanvas canv(pDi->hDC);
		HGDIOBJ brush = UiToolBox.Get(tbiButtonBrush_F + item_state);
		if(!brush && item_state)
			brush = UiToolBox.Get(tbiButtonBrush_F);
		HGDIOBJ pen = UiToolBox.Get(tbiButtonPen_F + item_state);
		if(!pen && item_state)
			pen = UiToolBox.Get(tbiButtonPen_F);
		canv.SelectObjectAndPush(brush);
		canv.SelectObjectAndPush(pen);
		SPoint2S pt_round;
		pt_round = 4; // ROUNDRECT_RADIUS * 2;
		canv.RoundRect(rect_elem, pt_round);
		if(draw_text || draw_bitmap) {
			SETFLAG(text_out_fmt, DT_CENTER, !(draw_text * draw_bitmap));
			SETFLAG(text_out_fmt, DT_LEFT, draw_text && draw_bitmap);
		}
		if(draw_bitmap) {
			if(item_state != tbisDisable) {
				POINT bmp_size;
				TProgram::DrawTransparentBitmap(canv, hbmp, rect_elem, out_r.left, out_r.top, RGB(0xD4, 0xD0, 0xC8), -1, text_out_fmt, &bmp_size);
				out_r.left += bmp_size.x + 2;
			}
		}
		if(erase_text) {
			TView::SSetWindowText(pDi->hwndItem, text_buf.Z());
		}
		else if(draw_text) {
			HFONT hf = reinterpret_cast<HFONT>(::SendMessage(pDi->hwndItem, WM_GETFONT, 0, 0));
			canv.SelectObjectAndPush(hf);
			canv.SetBkTranparent();
			COLORREF text_color;
            if(!UiToolBox.GetColor(tbiButtonTextColor + item_state, &text_color) && item_state)
				text_color = UiToolBox.GetColor(tbiButtonTextColor);
			canv.SetTextColor(text_color);
			TRect  rect_text;
			if(style & BS_MULTILINE) {
				long   text_h = 0;
				long   height = 0;
				TEXTMETRIC tm;
				::GetTextMetrics(canv, &tm);
				height = tm.tmHeight + 2;
				text_h = (out_r.bottom - out_r.top) / height;
				SplitBuf(canv, text_buf, out_r.right - out_r.left, text_h);
				StringSet ss('\n', text_buf);
				rect_text.set(out_r.left, out_r.top, out_r.right, height);
				for(uint i = 0; ss.get(&i, text_buf); rect_text.move(0, height)) {
					canv.DrawText_(rect_text, text_buf, text_out_fmt);
				}
			}
			else {
				rect_text = out_r;
				canv.DrawText_(rect_text, text_buf, text_out_fmt | DT_SINGLELINE);
			}
			canv.PopObject(); // font
		}
		canv.PopObjectN(2); // pen && brush
	}
	return ok;
}

int TProgram::GetDialogTextLayout(const SString & rText, int fontId, int penId, STextLayout & rTlo, int adj)
{
	int    ok = -1;
	rTlo.Reset();
	if(fontId && rText.NotEmpty()) {
		SString temp_buf;
		int    tool_text_brush_id = 0; //SPaintToolBox::rbr3DFace;
		int    tid_cs = UiToolBox.CreateCStyle(0, fontId, penId, tool_text_brush_id);
		SParaDescr pd;
		if(adj == ADJ_CENTER)
			pd.Flags |= SParaDescr::fJustCenter;
		else if(adj == ADJ_RIGHT)
			pd.Flags |= SParaDescr::fJustRight;
		int    tid_para = UiToolBox.CreateParagraph(0, &pd);
		rTlo.SetText(rText);
		rTlo.SetOptions(STextLayout::fWrap, tid_para, tid_cs);
		ok = 1;
	}
	return ok;
}

/*int TProgram::DrawCtrlImage(HWND hwnd, DRAWITEMSTRUCT * pDi)
{
	int   ok = 1;
	return ok;
}*/

int TProgram::DrawButton3(HWND hwnd, DRAWITEMSTRUCT * pDi)
{
	MemLeakTracer mlt; // @debug
    InitUiToolBox();

	int    ok = -1;
	int    draw_bitmap = 0;
	uint   dv_id = 0;
	int    item_state = tbisBase;
	if(pDi->itemState & ODS_DISABLED)
		item_state = tbisDisable;
	else if(pDi->itemState & ODS_SELECTED && pDi->itemAction == ODA_SELECT)
		item_state = tbisSelect;
	else if(pDi->itemAction == ODA_FOCUS || (pDi->itemState & ODS_FOCUS))
		item_state = tbisFocus;
	/* if(pDi->itemState & ODS_HOTLIGHT) {
		item_state = tbisHover;
	} */
	const long style = TView::GetWindowStyle(pDi->hwndItem);
	long   text_out_fmt = /*DT_SINGLELINE|*/DT_VCENTER|DT_EXTERNALLEADING|DT_END_ELLIPSIS;
	const  TRect rect_elem_i = pDi->rcItem;
	const  FRect rect_elem = pDi->rcItem;
	RECT   out_r = pDi->rcItem;
	//COLORREF brush_color = RGB(0xDC, 0xD9, 0xD1);
	//COLORREF pen_color = _GetAssetColor(_assetCtrlBorderColor);
	HBITMAP hbmp = 0;
	SString text_buf;
	TView::SGetWindowText(pDi->hwndItem, text_buf);
	text_buf.Strip();
	const int draw_text = BIN(text_buf.Len());
	int    erase_text = 0;
	{
		void * p_user_data = TView::GetWindowUserData(pDi->hwndItem);
		if(item_state == tbisSelect) {
			out_r.top  += (style & BS_BITMAP) ? 1 : 2;
			out_r.left += (style & BS_BITMAP) ? 1 : 2;
		}
		/*
		if(style & BS_BITMAP)
			hbmp = (HBITMAP)SendMessage(pDi->hwndItem, BM_GETIMAGE, IMAGE_BITMAP, 0);
		else */
		if(p_user_data) {
			TButton * p_btn = static_cast<TButton *>(p_user_data);
			if(p_btn->IsConsistent()) {
				if(item_state == 0 && p_btn->IsDefault())
					item_state = tbisDefault;
				const uint bmp_id = p_btn->GetBmpID();
				switch(bmp_id) {
					case IDB_MENU:          dv_id = PPDV_FUNCTION01; break;
					case IDB_FILEBROWSE:    dv_id = PPDV_FOLDER02; break;
					case IDB_INLNCALC:      dv_id = PPDV_CALCULATOR01; break;
					case IDB_RTABLE:        dv_id = PPDV_TABLE02; break;
					case IDB_CRDCARD:       dv_id = PPDV_CARD01; break;
					case IDB_QUANTITY:      dv_id = PPDV_QUANTITY01; break;
					case IDB_DIVISION:      dv_id = PPDV_DIVISION01; break;
					case IDB_CASH01S:       dv_id = PPDV_CASHBOX02; break;
					case IDB_PRINTDEF:      dv_id = PPDV_CLIENTPRINTER01; break;
					case IDB_PRINTDEFMARK:  dv_id = PPDV_CLIENTPRINTER01MARK; break;
					case IDB_PRINTER:       dv_id = PPDV_PRINTER01; break;
					case IDB_RETURN:        dv_id = PPDV_RETURN01; break;
					case IDB_ARROW_LEVELUP: dv_id = PPDV_BACK01; break;
					case IDB_ENTER:         dv_id = PPDV_ENTER01; break;
					case IDB_CANCEL:        dv_id = PPDV_CANCEL02; break;
					case IDB_SELMODIFIER:   dv_id = PPDV_BOX01; break;
					case IDB_PASTE:         dv_id = PPDV_CLIPBOARDPASTE01; break;
					case IDB_BYCASH:        dv_id = PPDV_PAYMCASH01; break;
					case IDB_BANKING:       dv_id = PPDV_PAYMBANK01; break;
					case IDB_BANKINGDISABLED: dv_id = PPDV_PAYMBANKDISABLED01; break;
					case IDB_SUSCHECK:      dv_id = PPDV_CHANGE01; break;
					case IDB_ARROW_UP:      dv_id = PPDV_UP01; break;
					case IDB_ARROW_DOWN:    dv_id = PPDV_DOWN01; break;
					case IDB_BYPRICE:       dv_id = PPDV_PRICE01; break;
					case IDB_GDSGRP:        dv_id = PPDV_CATEGORY02; break;
					case IDB_DEFGRP:        dv_id = /*PPDV_CATEGORYHEART01*/PPDV_CATEGORYFAW02; break;
					case IDB_GUESTS:        dv_id = PPDV_GUESTCOUNT01; break;
					case IDB_TABLE_ORDERS:  dv_id = PPDV_TABLEORDER02; break;
					case IDB_DELIVERY:      dv_id = PPDV_DELIVERY01; break;
					case IDB_CLOCK:         dv_id = PPDV_CLOCK02; break; // @v9.2.11
					case IDB_PHONE:         dv_id = PPDV_PHONE01; break; // @v10.0.04
					case IDB_PHONEFORWARDED: dv_id = PPDV_PHONEFORWARDED01; break; // @v10.0.04
					case IDB_GEAR:           dv_id = PPDV_GEAR01; break; // @v10.4.11
					default:
						if(bmp_id & _SlConst.VectorImageMask) { // @v10.5.5
							dv_id = bmp_id;
						}
						else {
							const uint button_id = p_btn->GetId();
							switch(button_id) {
								case STDCTL_CANCELBUTTON:    dv_id = PPDV_CANCEL01; break;
								case STDCTL_OKBUTTON:        dv_id = PPDV_OK01; break;
								case STDCTL_UPBUTTON:        dv_id = PPDV_UP01; break;
								case STDCTL_DOWNBUTTON:      dv_id = PPDV_DOWN01; break;
								case STDCTL_EDITBUTTON:      dv_id = PPDV_EDITFILE02; break;
								case STDCTL_PRINT:
								case STDCTL_PRINTBUTTON:     dv_id = PPDV_PRINTER02; break;
								case STDCTL_AGREEMENTBUTTON: dv_id = PPDV_AGREEMENT01; break;
								case STDCTL_TAGSBUTTON:      dv_id = PPDV_TAGS01; break;
								case STDCTL_REGISTERSBUTTON: dv_id = PPDV_REGISTERS01; break;
								case STDCTL_VIEWOPTBUTTON:   dv_id = PPDV_VIEWOPTIONS02; break;
								case STDCTL_PERSONBUTTON:    dv_id = PPDV_PERSON01; break;
								case STDCTL_IMGADDBUTTON:    dv_id = PPDV_FOLDER02; break; // @v11.3.4
								case STDCTL_IMGDELBUTTON:    dv_id = PPDV_CANCEL02; break; // @v11.3.4
								case STDCTL_IMGPSTBUTTON:    dv_id = PPDV_CLIPBOARDPASTE01; break;
								case STDCTL_SJBUTTON:        dv_id = PPDV_SYSJOURNAL; break; // @v10.5.3
								case STDCTL_TRANSMITBUTTON:  dv_id = PPDV_SYNC01; break; // @v10.5.3
								case STDCTL_INSBUTTON:
									dv_id = PPDV_ADDFILE02;
									if(text_buf == "+") // Специальный случай: иногда кнопка "Добавить" содержит текст "+" - его надо элиминировать
										erase_text = 1;
									break;
								case STDCTL_DELBUTTON:
									dv_id = PPDV_DELETEFILE02;
									if(text_buf == "-") // Специальный случай: иногда кнопка "Удалить" содержит текст "-" - его надо элиминировать
										erase_text = 1;
									break;
								default:
									if(p_btn->GetSubSign() == TV_SUBSIGN_COMBOBOX) {
										dv_id = PPDV_DROPDOWN01;
									}
									else if(bmp_id == OBM_UPARROWD) {
										dv_id = PPDV_UP01;
										erase_text = 1;
									}
									else if(bmp_id == OBM_DNARROWD) {
										dv_id = PPDV_DOWN01;
										erase_text = 1;
									}
									else if(text_buf.NotEmpty()) {
										if(text_buf == "*") {
											dv_id = PPDV_ASTERISK01;
											erase_text = 1;
										}
										else if(text_buf == "...") {
											dv_id = PPDV_ELLIPSIS;
											erase_text = 1;
										}
										else if(text_buf == ">") {
											dv_id = PPDV_ANGLEARROWRIGHT01;
											erase_text = 1;
										}
										else if(text_buf == ">>") {
											dv_id = PPDV_DOUBLEANGLEARROWRIGHT01;
											erase_text = 1;
										}
										else if(text_buf == "<") {
											dv_id = PPDV_ANGLEARROWLEFT01;
											erase_text = 1;
										}
										else if(text_buf == "<<") {
											dv_id = PPDV_DOUBLEANGLEARROWLEFT01;
											erase_text = 1;
										}
										/*else if(text_buf == "1") {
											dv_id = PPDV_NUMBERONE01;
											erase_text = 1;
										}
										else if(text_buf == "2") {
											dv_id = PPDV_NUMBERTWO01;
											erase_text = 1;
										}
										else if(text_buf == "3") {
											dv_id = PPDV_NUMBERTHREE01;
											erase_text = 1;
										}
										else if(text_buf == "4") {
											dv_id = PPDV_NUMBERFOUR01;
											erase_text = 1;
										}
										else if(text_buf == "5") {
											dv_id = PPDV_NUMBERFIVE01;
											erase_text = 1;
										}
										else if(text_buf == "6") {
											dv_id = PPDV_NUMBERSIX01;
											erase_text = 1;
										}
										else if(text_buf == "7") {
											dv_id = PPDV_NUMBERSEVEN01;
											erase_text = 1;
										}
										else if(text_buf == "8") {
											dv_id = PPDV_NUMBEREIGHT01;
											erase_text = 1;
										}
										else if(text_buf == "9") {
											dv_id = PPDV_NUMBERNINE01;
											erase_text = 1;
										}
										else if(text_buf == "0") {
											dv_id = PPDV_NUMBERZERO01;
											erase_text = 1;
										}
										else if(text_buf == "+") {
											dv_id = PPDV_PLUS01;
											erase_text = 1;
										}
										else if(text_buf == "-") {
											dv_id = PPDV_MINUS01;
											erase_text = 1;
										}
										else if(text_buf == "/") {
											dv_id = PPDV_DIVIDE01;
											erase_text = 1;
										}
										else if(text_buf == "=") {
											dv_id = PPDV_EQUAL01;
											erase_text = 1;
										}*/
										else if(text_buf == "+/-") {
											dv_id = PPDV_PLUSMINUS01;
											erase_text = 1;
										}
										/*else if(text_buf == ".") {
											dv_id = PPDV_DOT01;
											erase_text = 1;
										}*/
										/*else if(text_buf == "C") {
											dv_id = PPDV_C;
											erase_text = 1;
										}*/
										/*else if(text_buf == "CE") {
											dv_id = PPDV_CE;
											erase_text = 1;
										}*/
									}
									break;
							}
						}
						break;
				}
			}
			else {
				if(sstreq(static_cast<const char *>(p_user_data), "papyruscalendarperiod")) // GetCalCtrlSignature(1)
					dv_id = PPDV_CALENDAR03;
				else if(sstreq(static_cast<const char *>(p_user_data), "papyruscalendardate")) // GetCalCtrlSignature(0)
					dv_id = PPDV_CALENDARDAY01;
				else if(sstreq(static_cast<const char *>(p_user_data), "papyruscalculator"))
					dv_id = PPDV_CALCULATOR02;
				else if(sstreq(static_cast<const char *>(p_user_data), "papyrusclock"))
					dv_id = PPDV_CLOCK02;
			}
		}
		if(!erase_text && draw_text) {
			out_r.left += 4;
			// out_r.top  += 2;
		}
		draw_bitmap = BIN(hbmp);
	}
	{
		TCanvas2 canv(UiToolBox, pDi->hDC);
		int   brush_id = tbiButtonBrush + item_state;
		int   pen_id = tbiButtonPen + item_state;
		int   text_pen_id = tbiButtonTextColor + item_state;
		if(item_state) {
			if(!UiToolBox.GetObj(brush_id))
				brush_id -= item_state;
			if(!UiToolBox.GetObj(pen_id))
				pen_id -= item_state;
			if(!UiToolBox.GetObj(text_pen_id))
				text_pen_id -= item_state;
		}
		{
			FRect rect_elem_f(rect_elem);
			rect_elem_f.Grow(-0.5f, -0.5f);
			// canv.RoundRect(rect_elem_f, 3, pen_id, brush_id);
			canv.Rect(rect_elem_f, pen_id, brush_id);
		}
		if(draw_text || draw_bitmap) {
			SETFLAG(text_out_fmt, DT_CENTER, !(draw_text * draw_bitmap));
			SETFLAG(text_out_fmt, DT_LEFT, draw_text && draw_bitmap);
		}
		//
		{
            draw_bitmap = 0;
			{
				//SETIFZ(dv_id, PPDV_TESTFLOWER);
				TWhatmanToolArray::Item tool_item;
				const SDrawFigure * p_fig = DvToolList.GetFigById(1, dv_id, &tool_item);
				if(p_fig) {
					if(!tool_item.ReplacedColor.IsEmpty()) {
						SColor replacement_color;
						if(item_state == tbisDisable)
							replacement_color = UiToolBox.GetColor(tbiIconPassiveColor);
						else
							replacement_color = UiToolBox.GetColor(tbiIconRegColor);
						canv.SetColorReplacement(tool_item.ReplacedColor, replacement_color);
					}
					FRect pic_bounds(rect_elem);
					if(!erase_text && draw_text) {
						pic_bounds.a.x = rect_elem.a.x + 2.5f;
						pic_bounds.a.y = rect_elem.a.y + 2.5f;
						pic_bounds.b.y = rect_elem.b.y - 2.5f;
						pic_bounds.b.x = pic_bounds.a.x + pic_bounds.Height();
						out_r.left += (LONG)pic_bounds.Width();
					}
					else {
						const float min_side = MIN(rect_elem.Width(), rect_elem.Height());
                        pic_bounds.a.SetZero();
                        pic_bounds.b.Set(min_side, min_side);
                        pic_bounds.Grow(/*-2.5f, -2.5f*/-3.5f, -3.5f);
						pic_bounds.MoveCenterTo(rect_elem.GetCenter());
					}
					LMatrix2D mtx;
					SViewPort vp;
					canv.PushTransform();
					p_fig->GetViewPort(&vp);
					canv.AddTransform(vp.GetMatrix(pic_bounds, mtx));
					canv.Draw(p_fig);
					canv.PopTransform();
					canv.ResetColorReplacement();
				}
			}
		}
		//
		/*
		if(draw_bitmap) {
			if(item_state != tbisDisable) {
				POINT bmp_size;
				TProgram::DrawTransparentBitmap(canv, hbmp, rect_elem_i, out_r.left, out_r.top, RGB(0xD4, 0xD0, 0xC8), -1, text_out_fmt, &bmp_size);
				out_r.left += bmp_size.x + 2;
			}
		}
		*/
		if(erase_text) {
			// @v9.2.4 TView::SSetWindowText(pDi->hwndItem, text_buf.Z());
		}
		else if(draw_text) {
			HFONT  hf = reinterpret_cast<HFONT>(::SendMessage(pDi->hwndItem, WM_GETFONT, 0, 0));
			if(!hf)
				hf = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
			int    temp_font_id = 0;
			if(hf) {
				LOGFONT f;
				if(::GetObject(hf, sizeof(f), &f)) {
					SFontDescr fd(0, 0, 0);
					fd.SetLogFont(&f);
					temp_font_id = UiToolBox.CreateFont_(0, fd.Face, fd.Size, fd.Flags);
				}
			}
			if(temp_font_id) {
				STextLayout tlo;
				SDrawContext dctx = canv;
				text_buf.Transf(CTRANSF_OUTER_TO_INNER);
				if(GetDialogTextLayout(text_buf, temp_font_id, text_pen_id, tlo, draw_bitmap ? ADJ_LEFT : ADJ_CENTER) > 0) {
					FRect fr(out_r);
					tlo.SetBounds(fr);
					tlo.SetOptions(tlo.fVCenter, -1, -1);
					tlo.Arrange(dctx, UiToolBox);
					canv.DrawTextLayout(&tlo);
				}
			}
		}
	}
	return ok;
}

const SDrawFigure * TProgram::LoadDrawFigureBySymb(const char * pSymb, TWhatmanToolArray::Item * pInfo) const
{
	const SDrawFigure * p_fig = DvToolList.GetFig(1, pSymb, pInfo);
	return p_fig;
}

const SDrawFigure * TProgram::LoadDrawFigureById(uint id, TWhatmanToolArray::Item * pInfo) const
{
	const SDrawFigure * p_fig = DvToolList.GetFigById(1, id, pInfo);
	return p_fig;
}

int TProgram::DrawInputLine3(HWND hwnd, DRAWITEMSTRUCT * pDi)
{
	int    ok = 1;
	InitUiToolBox();
	int    item_state = tbisBase;
	if(pDi->itemState & ODS_DISABLED)
		item_state = tbisDisable;
	else if(pDi->itemState & ODS_SELECTED && pDi->itemAction == ODA_SELECT)
		item_state = tbisSelect;
	else if(pDi->itemAction == ODA_FOCUS || (pDi->itemState & ODS_FOCUS))
		item_state = tbisFocus;
	const  long style = TView::GetWindowStyle(pDi->hwndItem);
	const  TRect rect_elem_i = pDi->rcItem;
	const  FRect rect_elem = pDi->rcItem;
	{
		TCanvas2 canv(UiToolBox, pDi->hDC);

		int   brush_id = tbiButtonBrush + item_state;
		int   pen_id = tbiButtonPen + item_state;
		int   text_pen_id = tbiButtonTextColor + item_state;
		if(item_state) {
			if(!UiToolBox.GetObj(brush_id))
				brush_id -= item_state;
			if(!UiToolBox.GetObj(pen_id))
				pen_id -= item_state;
			if(!UiToolBox.GetObj(text_pen_id))
				text_pen_id -= item_state;
		}
		{
			FRect rect_elem_f(rect_elem);
			rect_elem_f.Grow(-0.5f, -0.5f);
			canv.RoundRect(rect_elem_f, /*3*/0, pen_id, 0/*brush_id*/);
			//
			/* test
			{
				canv.MoveTo(rect_elem_f.a.AddY(1.0f));
				canv.LineH(rect_elem_f.b.X);
				canv.Stroke(tbiIconAlertColor, 0);
			}
			*/
		}
	}
	return ok;
}

int DrawInputLine(HWND hwnd, DRAWITEMSTRUCT * pDi)
{
	int    ok = 0;
	if(oneof2(APPL->GetUiSettings().WindowViewStyle, UserInterfaceSettings::wndVKFancy, UserInterfaceSettings::wndVKVector)) {
		int    focused  = BIN(pDi->itemAction == ODA_FOCUS || (pDi->itemState & ODS_FOCUS));
		int    disabled = BIN(pDi->itemState & ODS_DISABLED);
		COLORREF brush_color = RGB(0xDC, 0xD9, 0xD1);
		COLORREF pen_color = focused ? _GetAssetColor(_assetCtrlFocusedBorderColor) : _GetAssetColor(_assetCtrlBorderColor);
		HPEN   pen   = CreatePen(PS_SOLID, 1, pen_color);
		HBRUSH brush = CreateSolidBrush(brush_color);
		HPEN   old_pen   = (HPEN)SelectObject(pDi->hDC, pen);
		HBRUSH old_brush = static_cast<HBRUSH>(::SelectObject(pDi->hDC, brush));
		// RoundRect(pDi->hDC, pDi->rcItem.left, pDi->rcItem.top, pDi->rcItem.right, pDi->rcItem.bottom, ROUNDRECT_RADIUS, ROUNDRECT_RADIUS);
		{
			HDC hdc = pDi->hDC;
			int start_x = 0, start_y = 0, end_x = 0, end_y = 0;
			RECT arc_rect;
			arc_rect.left   = pDi->rcItem.left;
			arc_rect.top    = pDi->rcItem.top;
			arc_rect.right  = arc_rect.left + ROUNDRECT_RADIUS * 2;
			arc_rect.bottom = arc_rect.top + ROUNDRECT_RADIUS * 2;
			start_x = pDi->rcItem.left + ROUNDRECT_RADIUS;
			start_y = pDi->rcItem.top;
			end_x   = pDi->rcItem.left;
			end_y   = pDi->rcItem.top + ROUNDRECT_RADIUS;
			Arc(hdc, arc_rect.left, arc_rect.top, arc_rect.right, arc_rect.bottom, start_x, start_y, end_x, end_y);

			MoveToEx(hdc, end_x, end_y - 1, 0);
			end_x = pDi->rcItem.left;
			end_y = pDi->rcItem.bottom - ROUNDRECT_RADIUS / 2 - 1;
			LineTo(hdc, end_x, end_y);

			arc_rect.left   = pDi->rcItem.left;
			arc_rect.top    = pDi->rcItem.bottom - ROUNDRECT_RADIUS * 2;
			arc_rect.right  = arc_rect.left + ROUNDRECT_RADIUS * 2;
			arc_rect.bottom = pDi->rcItem.bottom;
			start_x = pDi->rcItem.left;
			start_y = pDi->rcItem.bottom - ROUNDRECT_RADIUS;
			end_x   = pDi->rcItem.left + ROUNDRECT_RADIUS;
			end_y   = pDi->rcItem.bottom;
			Arc(hdc, arc_rect.left, arc_rect.top, arc_rect.right, arc_rect.bottom, start_x, start_y, end_x, end_y);

			MoveToEx(hdc, end_x - 1, end_y - 1, 0);
			end_x = pDi->rcItem.right - ROUNDRECT_RADIUS;
			end_y = pDi->rcItem.bottom - 1;
			LineTo(hdc, end_x, end_y);

			arc_rect.left   = pDi->rcItem.right - ROUNDRECT_RADIUS * 2;
			arc_rect.top    = pDi->rcItem.bottom - ROUNDRECT_RADIUS * 2;
			arc_rect.right  = pDi->rcItem.right;
			arc_rect.bottom = pDi->rcItem.bottom;
			start_x = pDi->rcItem.right - ROUNDRECT_RADIUS;
			start_y = pDi->rcItem.bottom;
			end_x   = pDi->rcItem.right;
			end_y   = pDi->rcItem.bottom - ROUNDRECT_RADIUS;
			Arc(hdc, arc_rect.left, arc_rect.top, arc_rect.right, arc_rect.bottom, start_x, start_y, end_x, end_y);

			MoveToEx(hdc, end_x - 1, end_y - 1, 0);
			end_x = pDi->rcItem.right - 1;
			end_y = pDi->rcItem.top + ROUNDRECT_RADIUS - 1;
			LineTo(hdc, end_x, end_y);

			arc_rect.left   = pDi->rcItem.right - ROUNDRECT_RADIUS * 2;
			arc_rect.top    = pDi->rcItem.top;
			arc_rect.right  = pDi->rcItem.right;
			arc_rect.bottom = pDi->rcItem.top + ROUNDRECT_RADIUS * 2;
			start_x = pDi->rcItem.right;
			start_y = pDi->rcItem.top + ROUNDRECT_RADIUS;
			end_x   = pDi->rcItem.right - ROUNDRECT_RADIUS;
			end_y   = pDi->rcItem.top;
			Arc(hdc, arc_rect.left, arc_rect.top, arc_rect.right, arc_rect.bottom, start_x, start_y, end_x, end_y);

			MoveToEx(hdc, end_x, end_y, 0);
			end_x = pDi->rcItem.left + ROUNDRECT_RADIUS - 1;
			end_y = pDi->rcItem.top;
			LineTo(hdc, end_x, end_y);
		}
		if(old_brush)
			SelectObject(pDi->hDC, old_brush);
		ZDeleteWinGdiObject(&brush);
		if(old_pen)
			SelectObject(pDi->hDC, old_pen);
		ZDeleteWinGdiObject(&pen);
		ok = 1;
	}
	return ok;
}

int DrawStatusBarItem(HWND hwnd, DRAWITEMSTRUCT * pDi)
{
	int    ok = -1;
	const TStatusWin::StItem * p_item = reinterpret_cast<const TStatusWin::StItem *>(pDi->itemData);
	if(p_item) {
		int    hotlight = BIN(pDi->itemState & ODS_HOTLIGHT);
		HBRUSH brush = 0, old_brush = 0;
		HPEN   pen = 0, old_pen = 0;
		RECT   out_r;
		if(p_item->Color) {
			brush = ::CreateSolidBrush((COLORREF)p_item->Color);
			old_brush = static_cast<HBRUSH>(::SelectObject(pDi->hDC, brush));
			pen = ::CreatePen(PS_SOLID, 1, p_item->Color);
			old_pen = static_cast<HPEN>(::SelectObject(pDi->hDC, pen));
		}
		out_r = pDi->rcItem;
		if(!hotlight) {
			RECT rect;
			POINT mouse_pos;
			GetCursorPos(&mouse_pos);
			GetWindowRect(hwnd, &rect);
			if(mouse_pos.y >= rect.top && mouse_pos.y <= rect.bottom) {
				mouse_pos.x -= rect.left;
				hotlight = BIN(mouse_pos.x >= out_r.left && mouse_pos.x <= out_r.right);
			}
		}
		Rectangle(pDi->hDC, out_r.left, out_r.top, out_r.right, out_r.bottom);
		if(sstrlen(p_item->str)) {
			int    delete_font = 0;
			HFONT  font = reinterpret_cast<HFONT>(SendMessage(pDi->hwndItem, WM_GETFONT, 0, 0));
			HFONT  old_font = 0;
			long   text_out_fmt = DT_SINGLELINE|DT_VCENTER|DT_EXTERNALLEADING;
			SetBkMode(pDi->hDC, TRANSPARENT);
			if(hotlight) {
				LOGFONT log_font;
				TEXTMETRIC tm;
				MEMSZERO(tm);
				MEMSZERO(log_font);
				GetTextMetrics(pDi->hDC, &tm);
				GetTextFace(pDi->hDC, sizeof(log_font.lfFaceName), log_font.lfFaceName);
				log_font.lfCharSet   = RUSSIAN_CHARSET;
				log_font.lfHeight    = tm.tmHeight;
				log_font.lfWeight    = FW_MEDIUM;
				log_font.lfUnderline = 1;
				font = ::CreateFontIndirect(&log_font);
				delete_font = 1;
			}
			else
				::SetCursor(::LoadCursor(0, IDC_ARROW));
			SetTextColor(pDi->hDC, p_item->TextColor);
			old_font = static_cast<HFONT>(::SelectObject(pDi->hDC, font));
			InflateRect(&out_r, -1, -1);
			::DrawText(pDi->hDC, SUcSwitch(p_item->str), sstrleni(p_item->str), &out_r, text_out_fmt);
			if(old_font)
				::SelectObject(pDi->hDC, old_font);
			if(delete_font)
				::DeleteObject(font);
		}
		ZDeleteWinGdiObject(&brush);
		if(old_brush)
			::SelectObject(pDi->hDC, old_brush);
		ZDeleteWinGdiObject(&pen);
		if(old_pen)
			::SelectObject(pDi->hDC, old_pen);
		ok = 1;
	}
	return ok;
}

int TProgram::EraseBackground(TView * pView, HWND hWnd, HDC hDC, int ctlType)
{
	int    ok = -1;
	if(pView && oneof2(UICfg.WindowViewStyle, UserInterfaceSettings::wndVKFancy, UserInterfaceSettings::wndVKVector)) {
		DRAWITEMSTRUCT di;
		memzero(&di, sizeof(di));
		const uint ctl_id = pView->GetId();
		di.CtlID      = ctl_id;
		di.itemID     = ctl_id;
		di.itemAction = ODA_DRAWENTIRE;
		di.CtlType    = ctlType; // ODT_BUTTON;
		di.hwndItem   = hWnd;
		di.hDC        = hDC;
		::GetClientRect(hWnd, &di.rcItem);
		ok = DrawControl(GetParent(hWnd), WM_ERASEBKGND, (WPARAM)&di, 0);
	}
	return ok;
}

int TProgram::DrawControl(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	int    ok = -1;
	switch(msg) {
		case WM_DRAWITEM:
		case WM_PAINT:
		case WM_NCPAINT:
			{
				DRAWITEMSTRUCT * p_di = reinterpret_cast<DRAWITEMSTRUCT *>(lParam);
				const int wvs = GetUiSettings().WindowViewStyle;
				if(oneof2(wvs, UserInterfaceSettings::wndVKFancy, UserInterfaceSettings::wndVKVector)) {
					if(p_di->CtlType == ODT_BUTTON && msg != WM_NCPAINT) {
						if(UICfg.WindowViewStyle == UserInterfaceSettings::wndVKFancy) {
							ok = DrawButton2(hwnd, p_di);
						}
						else if(UICfg.WindowViewStyle == UserInterfaceSettings::wndVKVector) {
							ok = DrawButton3(hwnd, p_di);
						}
					}
					else if(oneof2(p_di->CtlType, ODT_CHECKBOX, ODT_RADIOBTN))
						ok = -1;
					else if(p_di->CtlType == ODT_EDIT && msg == WM_NCPAINT) {
						if(UICfg.WindowViewStyle == UserInterfaceSettings::wndVKFancy) {
							ok = DrawInputLine(hwnd, p_di);
						}
						else if(UICfg.WindowViewStyle == UserInterfaceSettings::wndVKVector) {
							ok = DrawInputLine3(hwnd, p_di);
						}
					}
				}
				if(msg == WM_DRAWITEM && p_di->CtlID == 0) // Status window
					ok = DrawStatusBarItem(hwnd, p_di);
			}
			break;
		case WM_ERASEBKGND:
			{
				const int wvs = GetUiSettings().WindowViewStyle;
				if(oneof2(wvs, UserInterfaceSettings::wndVKFancy, UserInterfaceSettings::wndVKVector)) {
					DRAWITEMSTRUCT * p_di = reinterpret_cast<DRAWITEMSTRUCT *>(wParam);
					HBRUSH brush = 0, old_brush = 0;
					SETIFZ(brush, CreateSolidBrush(RGB(0xDD, 0xDD, 0xF1)));
					old_brush = static_cast<HBRUSH>(::SelectObject(p_di->hDC, brush));
					FillRect(p_di->hDC, &p_di->rcItem, brush);
					if(old_brush)
						SelectObject(p_di->hDC, old_brush);
					ZDeleteWinGdiObject(&brush);
					ok = 1;
				}
			}
			break;
	}
	return ok;
}

void TProgram::CloseAllBrowsers()
{
	HWND   hw = 0;
	do {
		hw = GetTopWindow(GetFrameWindow());
		if(hw == H_CloseWnd)
			hw = GetNextWindow(hw, GW_HWNDNEXT);
		if(hw) {
			if(hw != APPL->H_Desktop) {
				TBaseBrowserWindow * p_brw = static_cast<TBaseBrowserWindow *>(TView::GetWindowUserData(hw));
				p_brw->endModal(cmCancel);
			}
			else
				DestroyWindow(hw);
		}
	} while(hw);
	ZDELETE(P_TreeWnd);
}

void TProgram::GotoSite()
{
	SString url("http://www.petroglif.ru/");
	ShellExecute(0, _T("open"), SUcSwitch(url), NULL, NULL, SW_SHOWNORMAL);
}
//
//
//
static const char * UserInterfaceParamName  = "UserInterface";
static const char * UserInterfaceParamName2 = "UserInterface2";
/*static*/const char * UserInterfaceSettings::SubKey  = "Software\\Papyrus\\UI";

UserInterfaceSettings::UserInterfaceSettings() : TableFont(0, 0, 0), ListFont(0, 0, 0)
{
	Init();
}

void UserInterfaceSettings::Init()
{
	SetVersion();
	Flags = fShowShortcuts;
	WindowViewStyle = wndVKVector;
	TableViewStyle = 1;
	ListElemCount = 0;
	TableFont.Init();
	ListFont.Init();
	SupportMail.Z();
	SpecialInputDeviceSymb.Z();
}

uint32 UserInterfaceSettings::GetBrwColorSchema() const
{
	return (TableViewStyle >= 0 && TableViewStyle < NUMBRWCOLORSCHEMA) ? TableViewStyle : 0;
}

void UserInterfaceSettings::SetVersion()
{
	Ver = (UISETTINGS_VERSION_MINOR|(UISETTINGS_VERSION_MAJOR << 16));
}

int UserInterfaceSettings::Restore()
{
	int    ok = -1;
	Init();
	{
		size_t sz = 0;
		WinRegKey reg_key(HKEY_CURRENT_USER, UserInterfaceSettings::SubKey, 1);
		if(reg_key.GetRecSize(UserInterfaceParamName2, &sz) > 0 && sz > 0) {
			STempBuffer temp_buf(sz);
			SBuffer buffer;
			THROW(temp_buf.IsValid());
			ok = reg_key.GetBinary(UserInterfaceParamName2, temp_buf, sz);
			if(ok > 0) {
				SSerializeContext sctx;
				THROW(buffer.Write(temp_buf, sz));
				THROW(Serialize(-1, buffer, &sctx));
			}
		}
	}
	if(ok < 0) {
		struct UserInterfaceSettings_Before7909 {
			uint32 Size;                         //  4
			int32  Ver;                          //  4
			int32  Flags;                        //  4
			int8   ShowTreeMenu;                 //  1
			char   BrwFontName[32];              // 32
			int32  BrwFontHeight;                //  4
			int8   BrowserExitByEsc;             //  1 @obsolete
			uint32 BrwColorsSchemaNum;           //  4
			int16  ListElemCount;                //  2
			uint32 WndViewKindID;                // 32
			char   SupportMail[64];              //  5
			int32  BrwFontWeight;                //  4
			char   Reserve[128];
		};
		UserInterfaceSettings_Before7909 blk_b7909;
		WinRegKey reg_key(HKEY_CURRENT_USER, UserInterfaceSettings::SubKey, 1);
		ok = reg_key.GetBinary(UserInterfaceParamName, &blk_b7909, sizeof(blk_b7909));
		if(ok > 0) {
			if(blk_b7909.Ver < 0x00010002L) // 1.2
				blk_b7909.BrowserExitByEsc = 1;

			SetVersion();
			Flags = blk_b7909.Flags;
			SETFLAG(Flags, fShowLeftTree, blk_b7909.ShowTreeMenu);
			WindowViewStyle = blk_b7909.WndViewKindID;
			TableViewStyle = blk_b7909.BrwColorsSchemaNum;
			ListElemCount = blk_b7909.ListElemCount;
			TableFont.Init();
			TableFont.Face = blk_b7909.BrwFontName;
			TableFont.Size = (int16)blk_b7909.BrwFontHeight;
			TableFont.Weight = (float)blk_b7909.BrwFontWeight;
			ListFont.Init(); // В предыдущих версиях не было определения этого шрифта.
			SupportMail = blk_b7909.SupportMail;
		}
		else
			Init();
	}
	CATCHZOK
	return ok;
}

int UserInterfaceSettings::Save()
{
	int    ok = 1;
	SetVersion();
	SSerializeContext sctx;
	SBuffer buffer;
	THROW(Serialize(+1, buffer, &sctx));
	{
		WinRegKey reg_key(HKEY_CURRENT_USER, UserInterfaceSettings::SubKey, 0);
		ok = reg_key.PutBinary(UserInterfaceParamName2, buffer.GetBuf(buffer.GetRdOffs()), buffer.GetAvailableSize());
	}
	CATCHZOK
	return ok;
}

int UserInterfaceSettings::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx)
{
	int    ok = 1;
	SetVersion();
	THROW(pCtx->Serialize(dir, Ver, rBuf));
	THROW(pCtx->Serialize(dir, Flags, rBuf));
	THROW(pCtx->Serialize(dir, WindowViewStyle, rBuf));
	THROW(pCtx->Serialize(dir, TableViewStyle, rBuf));
	THROW(pCtx->Serialize(dir, ListElemCount, rBuf));
	THROW(TableFont.Serialize(dir, rBuf, pCtx));
	THROW(ListFont.Serialize(dir, rBuf, pCtx));
	THROW(pCtx->Serialize(dir, SupportMail, rBuf));
	if(dir < 0) {
		if(Ver >= (9 | (1 << 16))) {
			THROW(pCtx->Serialize(dir, SpecialInputDeviceSymb, rBuf));
			// @v11.5.11 {
			if(Ver >= (10 | (1 << 16))) {
				THROW(pCtx->Serialize(dir, BillItemTableFlags, rBuf));
			}
			// } @v11.5.11 
		}
		else
			SpecialInputDeviceSymb.Z();
	}
	else if(dir > 0) {
		THROW(pCtx->Serialize(dir, SpecialInputDeviceSymb, rBuf));
		THROW(pCtx->Serialize(dir, BillItemTableFlags, rBuf)); // @v11.5.11
	}
	CATCHZOK
	return ok;
}
