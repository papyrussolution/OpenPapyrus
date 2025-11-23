// Toolbar.cpp
// There's a mine born by Osolotkin, 2000, 2001
// Modified by A.Sobolev, 2002, 2003, 2005, 2010, 2011, 2013, 2014, 2015, 2016, 2017, 2019, 2020, 2021, 2022, 2023, 2024
// @codepage UTF-8
//
#include <slib-internal.h>
#pragma hdrstop
#include <ppdefs.h>

#define OEMRESOURCE

TToolbar::TToolbar(HWND hWnd, DWORD style) : PrevToolProc(0), H_MainWnd(hWnd), H_Menu(0), H_Toolbar(0), CurrPos(0), Style(style)
{
	MEMSZERO(ClientRect);
	MEMSZERO(CurrRect);
	memzero(&MousePoint, sizeof(POINTS));
	{
		WNDCLASSEXW wc;
		INITWINAPISTRUCT(wc);
		wc.lpszClassName = L"TOOLBAR_FOR_PPY";
		wc.hInstance = TProgram::GetInst();
		wc.lpfnWndProc = static_cast<WNDPROC>(WndProc);
		wc.style = CS_HREDRAW|CS_VREDRAW;
		wc.hCursor = LoadCursor(0, IDC_ARROW);
		wc.hbrBackground = static_cast<HBRUSH>(GetStockObject(LTGRAY_BRUSH));
		::RegisterClassEx(&wc);
	}
	H_Wnd = ::CreateWindowExW(WS_EX_TOOLWINDOW, L"TOOLBAR_FOR_PPY", NULL, WS_CHILD|WS_CLIPSIBLINGS, 0, 0, 0, 0, hWnd, 0, TProgram::GetInst(), 0);
	TView::SetWindowProp(H_Wnd, GWLP_USERDATA, this);
	H_Toolbar = ::CreateWindowExW(WS_EX_TOOLWINDOW, TOOLBARCLASSNAME, L"", WS_CHILD|TBSTYLE_TOOLTIPS|TBSTYLE_FLAT|CCS_NORESIZE|WS_CLIPSIBLINGS, 0, 0, 0, 0, H_Wnd, 0, TProgram::GetInst(), 0);
	TView::SetWindowProp(H_Toolbar, GWLP_USERDATA, this);
	PrevToolProc = static_cast<WNDPROC>(TView::SetWindowProp(H_Toolbar, GWLP_WNDPROC, ToolbarProc));
	CurrPos = 0;
	DWORD s = static_cast<DWORD>(::SendMessageW(H_Toolbar, TB_GETBUTTONSIZE, 0, 0));
	Width  = LOWORD(s);
	Height = HIWORD(s) + 4;
	PostMessage(H_Wnd, WM_USER, 0, 0);
}

TToolbar::~TToolbar()
{
	// @vadim Приводило к зависанию при выходе из программы
	// DestroyMenu(H_Menu);
	//SAlloc::F(m_pitem);
	HIMAGELIST himl = reinterpret_cast<HIMAGELIST>(::SendMessageW(H_Toolbar, TB_GETIMAGELIST, 0, 0));
	if(himl) {
		ImageList_Destroy(himl);
		himl = 0;
	}
	TView::SetWindowProp(H_Toolbar, GWLP_WNDPROC, PrevToolProc);
	DestroyMenu(H_Menu);
	::DestroyWindow(H_Wnd);
}

// AHTOXA {
//
// Сразу после вызова данной функции уничтожать объект.
// Используется при вызове функции BrowserWindow::ChangeResource
//
void TToolbar::DestroyHWND()
{
	// Возможно сначала надо будет произвести действия из ~TToolbar
	DestroyWindow(H_Wnd);
	H_Wnd = 0;
}
// } AHTOXA

LRESULT TToolbar::OnCommand(WPARAM wParam, LPARAM lParam)
{
	::PostMessage(H_MainWnd, WM_COMMAND, wParam, lParam);
	return 0;
}

LRESULT TToolbar::OnSize(WPARAM wParam, LPARAM lParam)
{
	if(lParam)
		switch(CurrPos) {
			case TOOLBAR_ON_BOTTOM:
			case TOOLBAR_ON_TOP:
			case TOOLBAR_ON_FREE: ::MoveWindow(H_Toolbar, 9, -1, LOWORD(lParam)-9, HIWORD(lParam), 1); break;
			case TOOLBAR_ON_RIGHT:
			case TOOLBAR_ON_LEFT: ::MoveWindow(H_Toolbar, 0,  8, LOWORD(lParam), HIWORD(lParam)-9, 1); break;
			default:              ::MoveWindow(H_Toolbar, 0, -1, LOWORD(lParam), HIWORD(lParam), 1); break;
		}
	return 0;
}

LRESULT TToolbar::OnMainSize(int rightSpace/*=0*/)
{
	RECT   client_rect, status_rect;
	if(CurrPos != TOOLBAR_ON_FREE) {
		GetClientRect(H_MainWnd, &client_rect);
		MEMSZERO(status_rect);
		if(H_MainWnd == APPL->H_MainWnd)
			APPL->GetStatusBarRect(&status_rect);
		client_rect.bottom -= status_rect.bottom - status_rect.top;
		/* switch(CurrPos) {
			case TOOLBAR_ON_BOTTOM: MoveWindow(H_Wnd, 0, client_rect.bottom-Height, client_rect.right, Height, 1); break;
			case TOOLBAR_ON_TOP: MoveWindow(H_Wnd, 0, 0, client_rect.right, Height, 1); break;
			case TOOLBAR_ON_RIGHT: MoveWindow(H_Wnd, client_rect.right-Width-4, 0, Width+4, client_rect.bottom, 1); break;
			case TOOLBAR_ON_LEFT: MoveWindow(H_Wnd, 0, 0, Width+4, client_rect.bottom, 1); break;
		} */
		client_rect.right -= rightSpace;
		if(oneof2(CurrPos, TOOLBAR_ON_TOP, TOOLBAR_ON_BOTTOM)) {
			DWORD r = static_cast<DWORD>(::SendMessageW(H_Toolbar, TB_GETROWS, 0, 0));
			if(CurrPos == TOOLBAR_ON_BOTTOM)
				client_rect.top = client_rect.bottom-Height*r;
			::MoveWindow(H_Wnd, 0, client_rect.top, client_rect.right, Height*r, 1);
		}
		else if(CurrPos == TOOLBAR_ON_RIGHT)
			::MoveWindow(H_Wnd, client_rect.right-Width-4, 0, Width+4, client_rect.bottom, 1);
		else if(CurrPos == TOOLBAR_ON_LEFT)
			::MoveWindow(H_Wnd, 0, 0, Width+4, client_rect.bottom, 1);
	}
	return 1;
}

LRESULT TToolbar::OnNotify(WPARAM wParam, LPARAM lParam)
{
	NMHDR * phm = reinterpret_cast<NMHDR *>(lParam);
	if(phm->code == TTN_NEEDTEXT) {
		uint idx = 0;
		if(Items.searchKeyCode(static_cast<ushort>(wParam), &idx))
			STRNSCPY(reinterpret_cast<TOOLTIPTEXT *>(lParam)->szText, SUcSwitch(Items.getItem(idx).ToolTipText)); // @unicodeproblem
	}
	// @v9.7.11 (experimental) {
	/*
	else if(phm->code == NM_CUSTOMDRAW) {
		LPNMCUSTOMDRAW lpNMCustomDraw = (LPNMCUSTOMDRAW)lParam;
		return CDRF_DODEFAULT;
	}
	*/
	// } @v9.7.11 
	return 0;
}

LRESULT TToolbar::OnLButtonDown(WPARAM wParam, LPARAM lParam)
{
	if(!(Style & TBS_NOMOVE)) {
		RECT   status_rect;
		long   fl = TView::SGetWindowStyle(H_Wnd);
		fl |= WS_THICKFRAME;
		TView::SetWindowProp(H_Wnd, GWL_STYLE, fl);
		PostMessage(H_Wnd, WM_NCLBUTTONDOWN, HTCAPTION, 0);
		GetClientRect(H_MainWnd, &ClientRect);
		MEMSZERO(status_rect);
		if(H_MainWnd == APPL->H_MainWnd)
			APPL->GetStatusBarRect(&status_rect);
		ClientRect.bottom += status_rect.top-status_rect.bottom;
		POINT pnt;
		pnt.x = ClientRect.left;
		pnt.y = ClientRect.top;
		ClientToScreen(H_MainWnd, &pnt);
		ClientRect.left    = pnt.x;
		ClientRect.top     = pnt.y;
		ClientRect.right  += pnt.x;
		ClientRect.bottom += pnt.y;
		GetWindowRect(H_Wnd, &CurrRect);
		MousePoint = MAKEPOINTS(lParam);
	}
	return 0;
}

LRESULT TToolbar::OnLButtonDblclk(WPARAM wParam, LPARAM lParam)
{
	if(APPL->DlgBoxParam(DLGW_CUSTOMIZETOOLBAR, H_Wnd, TuneToolsDlgProc, reinterpret_cast<LPARAM>(this)))
		APPL->SizeMainWnd(H_MainWnd);
	return 0;
}

LRESULT TToolbar::OnMove(WPARAM wParam, LPARAM lParam)
{
	if(ClientRect.right || ClientRect.left) {
		long   fl = TView::SGetWindowStyle(H_Wnd);
		fl &= ~WS_THICKFRAME;
		TView::SetWindowProp(H_Wnd, GWL_STYLE, fl);
		CurrRect.right  -= CurrRect.left;
		CurrRect.bottom -= CurrRect.top;
		CurrRect.left   -= ClientRect.left;
		CurrRect.top    -= ClientRect.top;
		MEMSZERO(ClientRect);
		fl = TView::SGetWindowStyle(H_Toolbar);
		SETFLAG(fl, TBSTYLE_WRAPABLE, CurrPos != TOOLBAR_ON_TOP && CurrPos != TOOLBAR_ON_BOTTOM);
		TView::SetWindowProp(H_Toolbar, GWL_STYLE, fl);
		MoveWindow(H_Wnd, CurrRect.left, CurrRect.top, CurrRect.right, CurrRect.bottom, 1);
		if(CurrPos == TOOLBAR_ON_FREE) {
			DWORD r = static_cast<DWORD>(::SendMessageW(H_Toolbar, TB_GETROWS, 0, 0));
			CurrRect.bottom = (Height + 2) * r;
			MoveWindow(H_Wnd, CurrRect.left, CurrRect.top, CurrRect.right, CurrRect.bottom, 1);
		}
		if(H_MainWnd == APPL->H_MainWnd)
			APPL->SizeMainWnd(H_Wnd);
		else {
			RECT rc;
			::GetClientRect(H_MainWnd, &rc);
			::PostMessage(H_MainWnd, WM_SIZE, 0, MAKELPARAM(rc.right, rc.bottom));
		}
	}
	return 0;
}

LRESULT TToolbar::OnMoving(WPARAM wParam, LPARAM lParam)
{
	RECT * p_rect = reinterpret_cast<RECT *>(lParam);
	CurrRect = *p_rect;
	if((p_rect->top + MousePoint.y) < (ClientRect.top + Height)) {
		p_rect->left   = ClientRect.left;
		p_rect->top    = ClientRect.top;
		p_rect->bottom = ClientRect.top + Height;
		p_rect->right  = ClientRect.right;
		CurrPos  = TOOLBAR_ON_TOP;
	}
	else if((p_rect->top + MousePoint.y) > (ClientRect.bottom - Height)) {
		p_rect->left   = ClientRect.left;
		p_rect->top    = ClientRect.bottom - Height;
		p_rect->bottom = ClientRect.bottom;
		p_rect->right  = ClientRect.right;
		CurrPos  = TOOLBAR_ON_BOTTOM;
	}
	else if((p_rect->left + MousePoint.x) < (ClientRect.left + Width + 4)) {
		p_rect->left   = ClientRect.left;
		p_rect->top    = ClientRect.top;
		p_rect->bottom = ClientRect.bottom;
		p_rect->right  = ClientRect.left + Width + 4;
		CurrPos  = TOOLBAR_ON_LEFT;
	}
	else if((p_rect->left + MousePoint.x) > (ClientRect.right - Width - 4)) {
		p_rect->left   = ClientRect.right - Width - 4;
		p_rect->top    = ClientRect.top;
		p_rect->bottom = ClientRect.bottom;
		p_rect->right  = ClientRect.right;
		CurrPos  = TOOLBAR_ON_RIGHT;
	}
	else if(CurrPos != TOOLBAR_ON_FREE) {
		int    free_height = 2 * Height;
		int    free_width = MIN(8, VisibleCount) * Width + 13;
		switch(CurrPos) {
			case TOOLBAR_ON_BOTTOM:
				p_rect->top = ClientRect.bottom + MousePoint.y - free_height;
				p_rect->left = ClientRect.left + MousePoint.x - free_width / 2;
				break;
			case TOOLBAR_ON_TOP:
				p_rect->left = ClientRect.left + MousePoint.x - free_width / 2;
				break;
			case TOOLBAR_ON_RIGHT:
				p_rect->left = ClientRect.right + MousePoint.x - free_width;
				p_rect->top = ClientRect.top + MousePoint.y - free_height / 2;
				break;
			case TOOLBAR_ON_LEFT:
				p_rect->top = ClientRect.top + MousePoint.y - free_height / 2;
				break;
		}
		p_rect->bottom = p_rect->top + free_height;
		p_rect->right  = p_rect->left + free_width;
		CurrPos  = TOOLBAR_ON_FREE;
	}
	MousePoint.x += static_cast<short>(CurrRect.left - p_rect->left);
	MousePoint.y += static_cast<short>(CurrRect.top - p_rect->top);
	CurrRect = *p_rect;
	return TRUE;
}

LRESULT CALLBACK TToolbar::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	TToolbar * pTbWnd = static_cast<TToolbar *>(TView::GetWindowUserData(hWnd));
	if(!pTbWnd)
		return DefWindowProc(hWnd, message, wParam, lParam);
	LRESULT ok = 0;
	switch(message) {
		case WM_COMMAND: ok = pTbWnd->OnCommand(wParam, lParam); break;
		case WM_SIZE: ok = pTbWnd->OnSize(wParam, lParam); break;
		case WM_NOTIFY: ok = pTbWnd->OnNotify(wParam, lParam); break;
		case WM_LBUTTONDOWN: ok = pTbWnd->OnLButtonDown(wParam, lParam); break;
		case WM_RBUTTONDOWN:
		case WM_LBUTTONDBLCLK: ok = pTbWnd->OnLButtonDblclk(wParam, lParam); break;
		case WM_MOVE: ok = pTbWnd->OnMove(wParam, lParam); break;
		case WM_MOVING: ok = pTbWnd->OnMoving(wParam, lParam); break;
		case WM_PAINT:
			{
				PAINTSTRUCT ps;
				RECT rc;
				GetClientRect(hWnd, &rc);
				HDC  hdc = ::BeginPaint(hWnd, &ps);
				HPEN hPen = ::CreatePen(PS_SOLID, 1, RGB(0x70,0x70,0x70));
				HPEN oldPen = static_cast<HPEN>(::SelectObject(hdc, hPen));
				switch(pTbWnd->CurrPos) {
					case TOOLBAR_ON_BOTTOM:
					case TOOLBAR_ON_TOP:
					case TOOLBAR_ON_FREE:
						MoveToEx(hdc, 4, 2, 0);
						LineTo(hdc, 4, rc.bottom-3);
						MoveToEx(hdc, 8, 2, 0);
						LineTo(hdc, 8, rc.bottom-3);
						break;
					case TOOLBAR_ON_RIGHT:
					case TOOLBAR_ON_LEFT:
						MoveToEx(hdc, 2, 4, 0);
						LineTo(hdc, rc.right-3, 4);
						MoveToEx(hdc, 2, 8, 0);
						LineTo(hdc, rc.right-3, 8);
						break;
				}
				SelectObject(hdc, oldPen);
				ZDeleteWinGdiObject(&hPen);
				hPen = CreatePen(PS_SOLID, 2, RGB(0xF0,0xF0,0xF0));
				SelectObject(hdc, hPen);
				switch(pTbWnd->CurrPos) {
					case TOOLBAR_ON_BOTTOM:
					case TOOLBAR_ON_TOP:
					case TOOLBAR_ON_FREE:
						MoveToEx(hdc, 2, 3, 0);
						LineTo(hdc, 2, rc.bottom-2);
						MoveToEx(hdc, 6, 3, 0);
						LineTo(hdc, 6, rc.bottom-2);
						break;
					case TOOLBAR_ON_RIGHT:
					case TOOLBAR_ON_LEFT:
						MoveToEx(hdc, 3, 2, 0);
						LineTo(hdc, rc.right-2, 2);
						MoveToEx(hdc, 3, 6, 0);
						LineTo(hdc, rc.right-2, 6);
						break;
				}
				SelectObject(hdc, oldPen);
				ZDeleteWinGdiObject(&hPen);
				EndPaint(hWnd, &ps);
				ok = 0;
			}
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return ok;
}

LRESULT CALLBACK TToolbar::ToolbarProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if(oneof4(msg, WM_LBUTTONDOWN, WM_LBUTTONDBLCLK, WM_LBUTTONUP, WM_NCLBUTTONUP))
		PostMessage(GetParent(hWnd), msg, wParam, lParam);
	// @v9.7.11 (experimental) {
	/*
	else if(msg == WM_NOTIFY) {
		NMHDR * phm = (NMHDR *)lParam;
		if(phm->code == NM_CUSTOMDRAW) {
			LPNMCUSTOMDRAW lpNMCustomDraw = (LPNMCUSTOMDRAW)lParam;
			return CDRF_DODEFAULT;
		}
	}
	*/
	// } @v9.7.11 
	TToolbar * p_tb = static_cast<TToolbar *>(TView::GetWindowUserData(hWnd));
	return CallWindowProc(p_tb->PrevToolProc, hWnd, msg, wParam, lParam);
}

HMENU SetLocalMenu(HMENU * pMenu, HWND hToolbar)
{
	HMENU  h_menu = ::CreateMenu();
	uint   cnt = static_cast<uint>(::SendMessageW(hToolbar, TB_BUTTONCOUNT, 0, 0));
	for(uint i = 0; i < cnt; i++) {
		TBBUTTON tb;
		::SendMessageW(hToolbar, TB_GETBUTTON, i, reinterpret_cast<LPARAM>(&tb));
		if(!(tb.fsState & TBSTATE_HIDDEN))
			if(tb.fsStyle & TBSTYLE_SEP)
				AppendMenu(h_menu, MF_ENABLED|MF_SEPARATOR, 0, 0);
			else
				AppendMenu(h_menu, MF_ENABLED|MF_STRING, tb.idCommand, reinterpret_cast<LPTSTR>(tb.dwData));
	}
	ASSIGN_PTR(pMenu, h_menu);
	return h_menu;
}

int isFindMenuID(long id, HMENU hm)
{
	const   int cnt = GetMenuItemCount(hm);
	int     i = 0;
	for(i = 0; i < cnt; i++) {
		MENUITEMINFO mii;
		INITWINAPISTRUCT(mii);
		mii.fMask = MIIM_DATA|MIIM_SUBMENU|MIIM_STATE|MIIM_ID;
		GetMenuItemInfo(hm, i, TRUE, &mii);
		if(mii.hSubMenu && isFindMenuID(id, mii.hSubMenu)) // @recursion
			break;
		else if(mii.wID == (uint)id)
			break;
	}
	return (i < cnt);
}

HICON BitmapToIcon(HBITMAP hBitmap)
{
	HDC hDC        = ::GetDC(NULL);
	HDC hMainDC    = ::CreateCompatibleDC(hDC);
	HDC hAndMaskDC = ::CreateCompatibleDC(hDC);
	HDC hXorMaskDC = ::CreateCompatibleDC(hDC);
	//
	// Получаем размеры битмапа
	//
	BITMAP bm;
	::GetObject(hBitmap, sizeof(BITMAP), &bm);
	HBITMAP hAndMaskBitmap  = ::CreateCompatibleBitmap(hDC, bm.bmWidth, bm.bmHeight);
	HBITMAP hXorMaskBitmap  = ::CreateCompatibleBitmap(hDC, bm.bmWidth, bm.bmHeight);

	//Select the bitmaps to DC

	HBITMAP hOldMainBitmap     = static_cast<HBITMAP>(::SelectObject(hMainDC,    hBitmap));
	HBITMAP hOldAndMaskBitmap  = static_cast<HBITMAP>(::SelectObject(hAndMaskDC, hAndMaskBitmap));
	HBITMAP hOldXorMaskBitmap  = static_cast<HBITMAP>(::SelectObject(hXorMaskDC, hXorMaskBitmap));
	//Scan each pixel of the souce bitmap and create the masks
	COLORREF MainBitPixel;
	for(int x = 0; x < bm.bmWidth; ++x) {
		for(int y = 0; y < bm.bmHeight; ++y) {
			MainBitPixel = ::GetPixel(hMainDC, x, y);
			::SetPixel(hAndMaskDC, x, y, RGB(0, 0, 0));
			::SetPixel(hXorMaskDC, x, y, MainBitPixel);
		}
	}
	ICONINFO iconinfo = {0};
	iconinfo.fIcon    = FALSE;
	iconinfo.xHotspot = 0;
	iconinfo.yHotspot = 0;
	iconinfo.hbmMask  = hAndMaskBitmap;
	iconinfo.hbmColor = hXorMaskBitmap;

	HICON hIcon = ::CreateIconIndirect(&iconinfo);

	::SelectObject(hMainDC,    hOldMainBitmap);
	::SelectObject(hAndMaskDC, hOldAndMaskBitmap);
	::SelectObject(hXorMaskDC, hOldXorMaskBitmap);

	::DeleteDC(hXorMaskDC);
	::DeleteDC(hAndMaskDC);
	::DeleteDC(hMainDC);

	::ReleaseDC(NULL,hDC);

	::DeleteObject(hAndMaskBitmap);
	::DeleteObject(hXorMaskBitmap);
	::DeleteObject(hOldMainBitmap);
	::DeleteObject(hOldAndMaskBitmap);
	::DeleteObject(hOldXorMaskBitmap);
	return hIcon;
}

int TToolbar::SetupToolbarWnd(DWORD style, const ToolbarList * pList)
{
	RVALUEPTR(Items, pList);
	Style = style;
	VisibleCount = 0;
	uint   i;
	HIMAGELIST himl = reinterpret_cast<HIMAGELIST>(::SendMessageW(H_Toolbar, TB_GETIMAGELIST, 0, 0));
	{
		long _c = static_cast<long>(::SendMessageW(H_Toolbar, TB_BUTTONCOUNT, 0, 0));
		if(_c) do {
			::SendMessageW(H_Toolbar, TB_DELETEBUTTON, --_c, 0);
		} while(_c);
		if(himl) {
			ImageList_RemoveAll(himl);
			ImageList_Destroy(himl);
			himl = 0;
		}
	}
	if(Items.getItemsCount()) {
		himl = ::ImageList_Create(GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), ILC_COLORDDB, 0, 64);
		HMENU  h_menu = ::GetMenu(H_MainWnd);
		ToolbarItem item;
		int    prev_separator = 0;
		TBBUTTON * p_btns = new TBBUTTON[Items.getItemsCount()];
		long img_count = 0;
		for(i = 0; Items.enumItems(&i, &item); VisibleCount++) {
			if(!(item.Flags & ToolbarItem::fHidden)) {
				int    skip = 0;
				TBBUTTON btns;
				MEMSZERO(btns);
				if(item.KeyCode != TV_MENUSEPARATOR) {
					btns.idCommand = item.KeyCode;
					btns.dwData  = reinterpret_cast<DWORD_PTR>(item.ToolTipText);
					btns.fsStyle = TBSTYLE_BUTTON;
					if(!h_menu || oneof3(item.KeyCode, cmEventsForCurrentUser, cmOpenTextFile, cmUniFindObj) || isFindMenuID(item.KeyCode, h_menu))
						btns.fsState |= TBSTATE_ENABLED;
					prev_separator = 0;
				}
				else if(!prev_separator && i < Items.getItemsCount()) { // Последовательный и последний разделитель не заносим
					btns.fsStyle = TBSTYLE_SEP | TBSTYLE_AUTOSIZE;
					btns.fsState = TBSTATE_ENABLED;
					prev_separator = 1;
				}
				else
					skip = 1;
				if(!skip) {
					long   image_idx = -1;
					if(item.BitmapIndex & SlConst::VectorImageMask) {
						//SPaintToolBox * p_tb = APPL->GetUiToolBox();
						/*if(p_tb)*/{
							TWhatmanToolArray::Item tool_item;
							const SDrawFigure * p_fig = APPL->LoadDrawFigureById(item.BitmapIndex, &tool_item);
							// @v11.9.2 {
							SPtrHandle h_icon = TCanvas2::TransformDrawFigureToIcon(APPL->GetUiToolBox(), p_fig, SPoint2S(16, 16), tool_item.ReplacedColor, SColor(192, 192, 192, 255));
							if(h_icon) {
								image_idx = ImageList_AddIcon(himl, reinterpret_cast<HICON>((void *)h_icon));
								//added_list.Add(image_id, image_idx, 0);
								::DestroyIcon(reinterpret_cast<HICON>((void *)h_icon));
							}
							// } @v11.9.2 
							/* @v11.9.2
							const uint _w = 16;
							const uint _h = 16;
							SImageBuffer ib(_w, _h);
							{
								TCanvas2 canv(*p_tb, ib);
								if(!tool_item.ReplacedColor.IsEmpty()) {
									SColor replacement_color = p_tb->GetColor(TProgram::tbiIconRegColor);
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
							HICON h_icon = static_cast<HICON>(ib.TransformToIcon());
							if(h_icon) {
								image_idx = ImageList_AddIcon(himl, h_icon);
								//added_list.Add(image_id, image_idx, 0);
								DestroyIcon(h_icon);
							}*/
						}
					}
					else {
						HBITMAP h_bmp = item.BitmapIndex ? APPL->FetchBitmap(item.BitmapIndex) : 0;
						if(h_bmp) {
							HICON h_icon = BitmapToIcon(h_bmp);
							// На некоторых машинах не отображаются картинки
							// img_idx = ImageList_Add(himl, h_bmp, (HBITMAP)0);
							image_idx = ImageList_AddIcon(himl, h_icon);
							DestroyIcon(h_icon);
						}
					}
					btns.iBitmap = MAKELONG(image_idx, 0);
					p_btns[img_count++] = btns;
				}
			}
		}
		::SendMessageW(H_Toolbar, TB_BUTTONSTRUCTSIZE, static_cast<WPARAM>(sizeof(TBBUTTON)), 0);
		::SendMessageW(H_Toolbar, TB_SETIMAGELIST, 0, reinterpret_cast<LPARAM>(himl));
		::SendMessageW(H_Toolbar, TB_ADDBUTTONS, img_count, reinterpret_cast<LPARAM>(p_btns));
		delete [] p_btns;
	}
	if(Style & (TBS_LIST | TBS_MENU))
		SetLocalMenu(&H_Menu, H_Toolbar);
	return Items.getItemsCount();
}

int TToolbar::Init(uint resID, uint typeID)
{
	int    ok = 0;
	TVRez * p_slrez = P_SlRez;
	if(p_slrez) {
		ToolbarList list;
		if(LoadToolbar(p_slrez, typeID, resID, &list)) {
			int    r = RestoreUserSettings(typeID, &list);
			ok = (r > 0) ? r : Init(&list);
		}
	}
	return ok;
}

int TToolbar::Init(uint cmdID, ToolbarList * pList)
{
	const int r = cmdID ? RestoreUserSettings(cmdID, pList) : -1;
	return (r > 0) ? r : Init(pList);
}

int TToolbar::Init(const ToolbarList * pList)
{
	int    ok = 0;
	if(SetupToolbarWnd(Style, pList)) {
		RECT client_rect;
		GetClientRect(H_MainWnd, &client_rect);
		MoveWindow(H_Wnd, 0, -1, client_rect.right, Height, 1);
		ok = Show();
	}
	return ok;
}

bool  TToolbar::IsValid() const { return (H_Wnd && H_Toolbar); }
int   TToolbar::GetCurrPos() const { return CurrPos; }
HWND  TToolbar::H() const { return H_Wnd; }
HWND  TToolbar::GetToolbarHWND() const { return H_Toolbar; }
uint  TToolbar::getItemsCount() const { return Items.getItemsCount(); }
const ToolbarItem & TToolbar::getItem(uint idx/* 0.. */) const { return Items.getItem(idx); }

int TToolbar::SelectMode()
{
	if(H_Menu && (Style & TBS_MENU || CurrPos == TOOLBAR_ON_FREE)) {
		ShowWindow(H_Toolbar, SW_HIDE);
		int i = SetMenu(H_Wnd, H_Menu);
		i = GetLastError();
	}
	else {
		SetMenu(H_Wnd, 0);
		ShowWindow(H_Toolbar, SW_SHOW);
	}
	return 1;
}

int TToolbar::Show()
{
	int    ok = 0;
	if(IsValid()) {
		::ShowWindow(H_Wnd, SW_SHOW);
		RECT rc;
		::GetClientRect(H_Wnd, &rc);
		PostMessage(H_Wnd, WM_SIZE, SIZE_RESTORED, (rc.bottom<<16) & rc.right);
		SelectMode();
		ok = 1;
	}
	return ok;
}

void TToolbar::Hide()
{
	::ShowWindow(H_Wnd, SW_HIDE);
}

#define TBI_CHECKED   (TB_GLOBAL_CHECKED-TB_ICON_USER)
#define TBI_UNCHECKED (TB_GLOBAL_UNCHECKED-TB_ICON_USER)

static HIMAGELIST CreateTuneToolsImageList()
{
	// Create a masked image list large enough to hold the icons.
	HIMAGELIST himglist = ImageList_Create(16, 16, ILC_COLOR, 2, 0);
	// Load the icon resources, and add the icons to the image list.
	HBITMAP    hbitmap = LoadBitmap(NULL, MAKEINTRESOURCE(OBM_CHECKBOXES));
	ImageList_Add(himglist, hbitmap, 0);
	hbitmap = LoadBitmap(NULL, MAKEINTRESOURCE(OBM_CHECK));
	ImageList_Add(himglist, hbitmap, 0);
	return himglist;
}

class TuneToolsDialog {
public:
	TuneToolsDialog(HWND hWnd, TToolbar *);
	~TuneToolsDialog();
	int    ToggleMarker();
	int    OnUpDownArrow(int up /* 1 - up, 0 - down */);
	int    Accept();
private:
	static LRESULT CALLBACK ListViewProc(HWND, UINT, WPARAM, LPARAM);

	HIMAGELIST hImages;
	TToolbar * P_Toolbar;
	HWND   H_List;
	TBBUTTON * P_Buttons;
	WNDPROC PrevListViewProc;
};

TuneToolsDialog::TuneToolsDialog(HWND hWnd, TToolbar * pTb) : P_Toolbar(pTb), hImages(0), H_List(0), P_Buttons(0), PrevListViewProc(0)
{
	uint   cnt = static_cast<uint>(::SendMessageW(P_Toolbar->H_Toolbar, TB_BUTTONCOUNT, 0, 0));
	LVITEM lvi;
	P_Buttons = static_cast<TBBUTTON *>(SAlloc::C(cnt, sizeof(TBBUTTON)));
	if(!P_Buttons) {
		EndDialog(hWnd, FALSE);
	}
	else {
		H_List = GetDlgItem(hWnd, CTL_CUSTOMIZETOOLBAR_LIST);
		RECT   rc;
		SString str_buf;
		GetClientRect(H_List, &rc);
		LVCOLUMN lv;
		lv.mask = LVCF_FMT | LVCF_WIDTH;
		lv.cx   = rc.right - GetSystemMetrics(SM_CXVSCROLL);
		lv.fmt  = LVCFMT_LEFT;
		ListView_InsertColumn(H_List, 0, &lv);
		lvi.iSubItem = 0;
		lvi.mask = LVIF_TEXT | LVIF_PARAM | LVIF_IMAGE;
		hImages = ImageList_LoadImage(TProgram::GetInst(), MAKEINTRESOURCE(TB_GLOBAL), 16, 0, CLR_NONE, IMAGE_BITMAP, LR_DEFAULTCOLOR|LR_LOADTRANSPARENT);
		ListView_SetImageList(H_List, hImages, LVSIL_SMALL);
		for(uint i = 0; i < cnt; i++) {
			TBBUTTON tb;
			TCHAR  temp_buf[256];
			TCHAR  div_text_buf[128];
			int    ret = static_cast<int>(::SendMessageW(P_Toolbar->H_Toolbar, TB_GETBUTTON, i, reinterpret_cast<LPARAM>(&tb)));
			P_Buttons[i] = tb;
			lvi.iItem = i;
			if(!(tb.fsStyle & TBSTYLE_SEP)) {
				uint   idx = 0;
				P_Toolbar->Items.searchKeyCode(tb.idCommand, &idx);
				STRNSCPY(temp_buf, SUcSwitch(P_Toolbar->Items.getItem(idx).ToolTipText));
				TCHAR * p = sstrchr(temp_buf, '\t');
				if(p)
					*p = 0;
				lvi.pszText = temp_buf; // @unicodeproblem
			}
			else {
				div_text_buf[0] = 0;
				SLS.SubstString("@menudivider", 1, str_buf.Z());
				str_buf.Quot(' ', ' ');
				str_buf.Quot('-', '-');
				str_buf.Quot('-', '-');
				str_buf.Quot('-', '-');
				STRNSCPY(div_text_buf, SUcSwitch(str_buf));
				lvi.pszText = div_text_buf; // @unicodeproblem
			}
			lvi.lParam = i;
			lvi.iImage = (tb.fsState & TBSTATE_HIDDEN) ? TBI_UNCHECKED : TBI_CHECKED;
			ListView_InsertItem(H_List, &lvi); // @unicodeproblem
		}
		lvi.mask = LVIF_STATE;
		lvi.stateMask = LVIS_SELECTED | LVIS_FOCUSED;
		lvi.state = LVIS_SELECTED | LVIS_FOCUSED;
		lvi.iItem = 0;
		ListView_SetItem(H_List, &lvi);
		HBITMAP h_up = LoadBitmap(0, MAKEINTRESOURCE(OBM_UPARROWD));
		::SendMessageW(::GetDlgItem(hWnd, CTL_CUSTOMIZETOOLBAR_UP), BM_SETIMAGE, IMAGE_BITMAP, reinterpret_cast<LPARAM>(h_up));
		HBITMAP h_dn = LoadBitmap(0, MAKEINTRESOURCE(OBM_DNARROWD));
		::SendMessageW(::GetDlgItem(hWnd, CTL_CUSTOMIZETOOLBAR_DOWN), BM_SETIMAGE, IMAGE_BITMAP, reinterpret_cast<LPARAM>(h_dn));
		PrevListViewProc = static_cast<WNDPROC>(TView::SetWindowProp(H_List, GWLP_WNDPROC, ListViewProc));
		TView::SetWindowProp(H_List, GWLP_USERDATA, this);
	}
}

TuneToolsDialog::~TuneToolsDialog()
{
	TView::SetWindowProp(H_List, GWLP_WNDPROC, PrevListViewProc);
	SAlloc::F(P_Buttons);
}

int TuneToolsDialog::ToggleMarker()
{
	LVITEM lvi;
	lvi.iItem = static_cast<int>(::SendMessageW(H_List, LVM_GETNEXTITEM, -1, LVNI_SELECTED));
	if(lvi.iItem >= 0) {
		lvi.iSubItem = 0;
		lvi.mask  = LVIF_IMAGE;
		ListView_GetItem(H_List, &lvi);
		if(lvi.iImage == TBI_CHECKED)
			lvi.iImage = TBI_UNCHECKED;
		else
			lvi.iImage = TBI_CHECKED;
		ListView_SetItem(H_List, &lvi);
	}
	return 1;
}

int TuneToolsDialog::OnUpDownArrow(int up)
{
	int    ok = -1;
	LVITEM lvi, lvi1;
	lvi.iItem = ListView_GetNextItem(H_List, -1, LVNI_SELECTED);
	if(lvi.iItem >= 0 && !(up && lvi.iItem == 0) && !(!up && lvi.iItem == ListView_GetItemCount(H_List)-1)) {
		TCHAR  buf[128];
		TCHAR  buf1[128];
		lvi.iSubItem = 0;
		lvi1.iSubItem = 0;
		lvi.mask = lvi1.mask  = LVIF_TEXT | LVIF_PARAM | LVIF_IMAGE | LVIF_STATE;
		lvi1.pszText = buf1;
		lvi1.cchTextMax = SIZEOFARRAY(buf1);
		lvi.stateMask = lvi1.stateMask = LVIS_SELECTED | LVIS_FOCUSED;
		lvi.pszText = buf;
		lvi.cchTextMax = SIZEOFARRAY(buf);
		lvi1.iItem = up ? (lvi.iItem-1) : (lvi.iItem+1);
		ListView_GetItem(H_List, &lvi);
		ListView_GetItem(H_List, &lvi1);
		if(up) {
			P_Toolbar->Items.moveItem(lvi.iItem, 1);
			lvi1.iItem++;
			lvi.iItem--;
		}
		else {
			P_Toolbar->Items.moveItem(lvi.iItem, 0);
			lvi1.iItem--;
			lvi.iItem++;
		}
		ListView_SetItem(H_List, &lvi1);
		ListView_SetItem(H_List, &lvi);
		ok = 1;
	}
	return ok;
}

int TuneToolsDialog::Accept()
{
	HWND   h_toolbar = P_Toolbar->H_Toolbar;
	uint   cnt = static_cast<uint>(::SendMessageW(h_toolbar, TB_BUTTONCOUNT, 0, 0));
	LVITEM lvi;
	lvi.iSubItem = 0;
	lvi.mask  = LVIF_PARAM | LVIF_IMAGE;
	P_Toolbar->VisibleCount = 0;
	for(uint i = 0; i < cnt; i++) {
		lvi.iItem = i;
		ListView_GetItem(H_List, &lvi);
		if(lvi.iImage == TBI_CHECKED) {
			P_Buttons[lvi.lParam].fsState &= ~TBSTATE_HIDDEN;
			P_Toolbar->VisibleCount++;
		}
		else
			P_Buttons[lvi.lParam].fsState |= TBSTATE_HIDDEN;
		::SendMessageW(h_toolbar, TB_DELETEBUTTON, i, 0);
		::SendMessageW(h_toolbar, TB_INSERTBUTTON, i, reinterpret_cast<LPARAM>(&P_Buttons[lvi.lParam]));
	}
	return 1;
}
//
//
/*static*/LRESULT CALLBACK TuneToolsDialog::ListViewProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	TuneToolsDialog * p_param = static_cast<TuneToolsDialog *>(TView::GetWindowUserData(hWnd));
	switch(message) {
		case WM_LBUTTONDOWN:
			if(LOWORD(lParam) > 0 && LOWORD(lParam) < 16)
				p_param->ToggleMarker();
			break;
		case WM_LBUTTONDBLCLK:
			if(LOWORD(lParam) >= 16)
				p_param->ToggleMarker();
			break;
		case WM_CHAR:
			if(wParam == 32 /* whitespace */)
				p_param->ToggleMarker();
			break;
	}
	return CallWindowProc(p_param->PrevListViewProc, hWnd, message, wParam, lParam);
}

static BOOL CALLBACK SetupCtrlTextProc(HWND hwnd, LPARAM lParam)
{
	SString temp_buf;
	TView::SGetWindowText(hwnd, temp_buf);
	if(temp_buf.NotEmpty()) {
		SString subst;
		if(SLS.SubstString(temp_buf, 1, subst) > 0)
			TView::SSetWindowText(hwnd, subst);
		else if(SLS.ExpandString(temp_buf, CTRANSF_UTF8_TO_OUTER) > 0)
			TView::SSetWindowText(hwnd, temp_buf);
	}
	return TRUE;
}

INT_PTR CALLBACK TToolbar::TuneToolsDlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) // @callback(DLGPROC)
{
	TuneToolsDialog * p_param = 0;
	switch(message) {
		case WM_INITDIALOG:
			p_param = new TuneToolsDialog(hWnd, reinterpret_cast<TToolbar *>(lParam));
			TView::SetWindowProp(hWnd, GWLP_USERDATA, p_param);
			SetupCtrlTextProc(hWnd, 0); // @v11.0.1
			EnumChildWindows(hWnd, SetupCtrlTextProc, 0);
			return 1;
		case WM_COMMAND:
			p_param = static_cast<TuneToolsDialog *>(TView::GetWindowUserData(hWnd));
			if(LOWORD(wParam) == IDOK) {
				p_param->Accept();
				EndDialog(hWnd, TRUE);
			}
			else if(LOWORD(wParam) == IDCANCEL)
				EndDialog(hWnd, FALSE);
			else if(LOWORD(wParam) == CTL_CUSTOMIZETOOLBAR_UP) {
				p_param->OnUpDownArrow(1);
				SetFocus(GetDlgItem(hWnd, CTL_CUSTOMIZETOOLBAR_LIST));
			}
			else if(LOWORD(wParam) == CTL_CUSTOMIZETOOLBAR_DOWN) {
				p_param->OnUpDownArrow(0);
				SetFocus(GetDlgItem(hWnd, CTL_CUSTOMIZETOOLBAR_LIST));
			}
			break;
		case WM_DESTROY:
			p_param = static_cast<TuneToolsDialog *>(TView::GetWindowUserData(hWnd));
			delete p_param;
			TView::SetWindowProp(hWnd, GWLP_USERDATA, static_cast<void *>(0));
			break;
	}
	return 0;
}
//
//
//
ToolbarCfg::ToolbarCfg() : P_Buttons(0), Count(0), Reserve(0)
{
}

ToolbarCfg::~ToolbarCfg()
{
	delete [] P_Buttons;
}

int ToolbarCfg::Init()
{
	delete [] P_Buttons;
	return BIN(P_Buttons = new TBButtonCfg[Count]);
}

int ToolbarCfg::Init(const void * pBuf)
{
	int    r = -1;
	const char * p = static_cast<const char *>(pBuf);
	if(p) {
		Count = *static_cast<const uint16 *>(pBuf);
		p += sizeof(uint16);
		if(Count > 0 && Init()) {
			r = 1;
			for(uint i = 0; i < Count; i++) {
				P_Buttons[i] = *reinterpret_cast<const TBButtonCfg *>(p);
				p += sizeof(TBButtonCfg);
			}
		}
	}
	return r;
}

int ToolbarCfg::GetBuf(void ** ppBuf, size_t bufLen) const
{
	int    r = -1;
	char * p = static_cast<char *>(*ppBuf);
	if(p && bufLen >= GetSize()) {
		r = 1;
		*reinterpret_cast<uint16 *>(p) = Count;
		p += sizeof(uint16);
		for(uint i = 0; i < Count; i++) {
			*reinterpret_cast<TBButtonCfg *>(p) = P_Buttons[i];
			p += sizeof(TBButtonCfg);
		}
	}
	return r;
}

static const char * GlobalToolbarParamName = "GlobalToolbar";
static const char * ToolbarParamName = "Toolbar";

int TToolbar::GetRegTbParam(uint typeID, char * pBuf, size_t bufLen)
{
	int r = 1;
	if(pBuf) {
		if(typeID == TV_GLBTOOLBAR)
			strnzcpy(pBuf, GlobalToolbarParamName, bufLen);
		else {
			SString toolbar_name;
			(toolbar_name = ToolbarParamName).Cat(typeID);
			toolbar_name.CopyTo(pBuf, bufLen);
		}
	}
	else
		r = 0;
	return r;
}

int TToolbar::SaveUserSettings(uint typeID)
{
	int    r = -1;
	char * p = 0;
	/* Не будем записывать данные в реестр {
	if(typeID) {
		char   param[48];
		if(GetRegTbParam(typeID, param, sizeof(param)) > 0) {
			ToolbarCfg tb_cfg;
			WinRegKey reg_key(HKEY_CURRENT_USER, UserInterfaceSettings::SubKey, 0);
			tb_cfg.Count = (uint16)::SendMessageW(H_Toolbar, TB_BUTTONCOUNT, 0, 0);
			tb_cfg.Init();
			for(uint i = 0; i < tb_cfg.Count; i++) {
				TBBUTTON tb;
				::SendMessageW(H_Toolbar, TB_GETBUTTON, i, (LPARAM)&tb);
				tb_cfg.P_Buttons[i].KeyCode = (tb.fsStyle & TBSTYLE_SEP) ? TV_MENUSEPARATOR : tb.idCommand;
				tb_cfg.P_Buttons[i].State   = (tb.fsState & ~TBSTATE_WRAP);
				tb_cfg.P_Buttons[i].Style   = tb.fsStyle;
			}
			p = new char[tb_cfg.GetSize()];
			if(tb_cfg.GetBuf((void **)&p, tb_cfg.GetSize()) > 0)
				r = reg_key.PutBinary(param, p, tb_cfg.GetSize());
		}
	}
	} Не будем записывать данные в реестр */
	delete p;
	return r;
}

int TToolbar::RestoreUserSettings(uint typeID, ToolbarList * pTbList)
{
	int    r = -1;
	char * p = 0;
	char   param[48];
	size_t size = 0;
	ToolbarCfg tb_cfg;
 	WinRegKey reg_key(HKEY_CURRENT_USER, UserInterfaceSettings::SubKey, 1);
	if(GetRegTbParam(typeID, param, sizeof(param)) > 0 && reg_key.GetRecSize(param, &size) > 0 && size >= (sizeof(uint16) + sizeof(TBButtonCfg))) {
		p = new char[size];
		if(reg_key.GetBinary(param, p, size) > 0 && tb_cfg.Init(p) > 0) {
			ToolbarList new_tb_list;
			if(pTbList && pTbList->getVisibleItemsCount() == tb_cfg.Count) {
				uint    i;
				for(i = 0; i < tb_cfg.Count; i++) {
					uint idx = 0;
					if(pTbList->searchKeyCode(tb_cfg.P_Buttons[i].KeyCode, &idx) > 0) {
						ToolbarItem tb_item = pTbList->getItem(idx);
						new_tb_list.addItem(&tb_item);
					}
				}
				/* Не будем восстанавливать данные из реестра
				if((r = Init(&new_tb_list)) > 0)
					SetupToolbarWnd(0, &new_tb_list);
					// todo Перенести в SetupToolbarWnd {
					for(i = 0; i < new_tb_list.getItemsCount(); i++) {
						ToolbarItem tb_item = new_tb_list.getItem(i);
						if(!(tb_item.Flags & ToolbarItem::fHidden)) {
							TBBUTTON btn;
							::SendMessageW(H_Toolbar, TB_GETBUTTON, i, (LPARAM)&btn);
							::SendMessageW(H_Toolbar, TB_DELETEBUTTON, i, 0);
							SETFLAG(tb_cfg.P_Buttons[i].State, TBSTATE_ENABLED, btn.fsState & TBSTATE_ENABLED);
							btn.fsState = tb_cfg.P_Buttons[i].State;
							btn.fsStyle = tb_cfg.P_Buttons[i].Style;
							::SendMessageW(H_Toolbar, TB_INSERTBUTTON, i, (LPARAM) &btn);
						}
					}
					// } todo Перенести в SetupToolbarWnd
				*/
			}
		}
	}
	delete p;
	return r;
}

int TToolbar::TranslateKeyCode(ushort keyCode, uint * pCmd) const
{
	int    ok = 0;
	uint   idx = 0;
	if(Items.searchKeyCode(keyCode, &idx)) {
		const ToolbarItem & r_item = Items.getItem(idx);
		if(r_item.Cmd) {
			ASSIGN_PTR(pCmd, (uint)r_item.Cmd);
			ok = 1;
		}
	}
	return ok;
}
