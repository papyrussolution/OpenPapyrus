// SMRTLBX.CPP
// Copyright (c) Sobolev A. 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2015, 2016, 2017, 2018, 2019, 2020, 2021, 2022, 2023, 2025
// @codepage UTF-8
// Release for WIN32
//
#include <slib-internal.h>
#pragma hdrstop

const char * SLBColumnDelim = "/^";

IMPL_CMPFUNC(_PcharNoCase, i1, i2)
{
	const char * p = static_cast<const char *>(i1);
	const char * c = static_cast<const char *>(i2);
	while(*p && !isprint(static_cast<uchar>(*p)) && !IsLetter866(*p))
		p++;
	if(*c == '*')
		return stristr866(p, c+1) ? 0 : 1;
	else {
		const size_t p_len = sstrlen(p);
		const size_t c_len = sstrlen(c);
		const int    r = strnicmp866(p, c, MIN(p_len, c_len));
		return (r == 0 && p_len < c_len) ? -1 : r;
	}
}

IMPL_CMPFUNC(FilePathUtf8, i1, i2)
{
	const char * p1 = static_cast<const char *>(i1);
	const char * p2 = static_cast<const char *>(i2);
	SString & r_s1 = SLS.AcquireRvlStr();
	SStringU & r_su1 = SLS.AcquireRvlStrU();
	SString & r_s2 = SLS.AcquireRvlStr();
	SStringU & r_su2 = SLS.AcquireRvlStrU();
	SFsPath::NormalizePath(p1, SFsPath::npfKeepCase, r_s1);
	r_su1.CopyFromUtf8Strict(r_s1, r_s1.Len());
	r_su1.ToLower();
	SFsPath::NormalizePath(p2, SFsPath::npfKeepCase, r_s2);
	r_su2.CopyFromUtf8Strict(r_s2, r_s2.Len());
	r_su2.ToLower();
	return r_su1.Cmp(r_su2);
}
//
//
//
INT_PTR CALLBACK TreeListBoxDialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) // @callback(DLGPROC)
{
	SmartListBox * p_view = static_cast<SmartListBox *>(TView::GetWindowUserData(hWnd));
	const WNDPROC prev_wnd_proc = p_view ? p_view->PrevWindowProc : 0;
	switch(uMsg) {
		case WM_DESTROY: CALLPTRMEMB(p_view, OnDestroy(hWnd)); return 0; // Не вызывается. Непонятно правда почему.
		case WM_KEYDOWN:
		case WM_SYSKEYDOWN: ::SendMessageW(GetParent(hWnd), WM_VKEYTOITEM, MAKEWPARAM((WORD)wParam, 0), reinterpret_cast<LPARAM>(hWnd)); break; // Process by default
		case WM_SETFOCUS:
		case WM_KILLFOCUS:  ::SendMessageW(GetParent(hWnd), uMsg, wParam, reinterpret_cast<LPARAM>(hWnd)); break;
		case WM_CHAR:       ::SendMessageW(GetParent(hWnd), uMsg, wParam, reinterpret_cast<LPARAM>(hWnd)); return 0;
		case WM_NOTIFY:     ::SendMessageW(GetParent(hWnd), uMsg, wParam, lParam); return 0;
		case WM_ERASEBKGND:
			if(p_view && p_view->HasState(SmartListBox::stOwnerDraw)) {
				TDrawItemData di;
				MEMSZERO(di);
				di.CtlType = ODT_LISTBOX;
				di.CtlID = p_view->GetId();
				di.ItemAction = TDrawItemData::iaBackground;
				di.H_Item = hWnd;
				di.H_DC = reinterpret_cast<HDC>(wParam);
				GetClientRect(hWnd, &di.ItemRect);
				di.P_View = p_view;
				TView::messageCommand(p_view->P_Owner, cmDrawItem, &di);
				if(di.ItemAction == 0)
					return 1;
			}
			break;
		// @v7.7.7 case WM_LBUTTONDBLCLK:
			// @v7.7.7 return p_view ? CallWindowProc(p_view->PrevWindowProc, hWnd, uMsg, wParam, lParam) : 1;
		/*
		case WM_INPUTLANGCHANGE: // @v6.4.4 AHTOXA
			PostMessage(GetParent(hWnd), uMsg, wParam, lParam);
			break;
		*/
		default:
			break;
	}
	return prev_wnd_proc ? CallWindowProc(prev_wnd_proc, hWnd, uMsg, wParam, lParam) : 1;
}

INT_PTR CALLBACK ListBoxDialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) // @callback(DLGPROC)
{
	SmartListBox * p_view = static_cast<SmartListBox *>(TView::GetWindowUserData(hWnd));
	if(p_view) {
		switch(uMsg) {
			case WM_DESTROY: // Не вызывается. Непонятно, правда, почему.
				CALLPTRMEMB(p_view, OnDestroy(hWnd));
				return 0;
			case WM_KEYDOWN:
			case WM_SYSKEYDOWN: ::SendMessageW(GetParent(hWnd), WM_VKEYTOITEM, MAKEWPARAM((WORD)wParam, 0), reinterpret_cast<LPARAM>(hWnd)); return 0;
			case WM_SETFOCUS:
			case WM_KILLFOCUS: ::SendMessageW(GetParent(hWnd), uMsg, wParam, reinterpret_cast<LPARAM>(hWnd)); break;
			case WM_CHAR: ::SendMessageW(GetParent(hWnd), uMsg, wParam, reinterpret_cast<LPARAM>(hWnd)); return 0;
			case WM_MOUSEWHEEL: ::SendMessageW(GetParent(hWnd), uMsg, wParam, reinterpret_cast<LPARAM>(hWnd)); return 0;
				/*
				{
					short delta = (short)HIWORD(wParam);
					int   scroll_code = (delta > 0) ? SB_LINEUP : SB_LINEDOWN;
					for(int i = 0; i < 3; i++)
						p_view->Scroll(scroll_code, 0);
				}
				return 0;
				*/
			case WM_RBUTTONDOWN: ::SendMessageW(GetParent(hWnd), uMsg, MAKEWPARAM(LOWORD(wParam), 1), lParam); return 0;
			case WM_ERASEBKGND:
				if(p_view->HasState(SmartListBox::stOwnerDraw)) {
					TDrawItemData di;
					MEMSZERO(di);
					di.CtlType = ODT_LISTBOX;
					di.CtlID = p_view->GetId();
					di.ItemAction = TDrawItemData::iaBackground;
					di.H_Item = hWnd;
					di.H_DC = reinterpret_cast<HDC>(wParam);
					GetClientRect(hWnd, &di.ItemRect);
					di.P_View = p_view;
					TView::messageCommand(p_view->P_Owner, cmDrawItem, &di);
					if(di.ItemAction == 0)
						return 1;
				}
				break;
			/*
			case WM_INPUTLANGCHANGE: // @v6.4.4 AHTOXA
				PostMessage(GetParent(hWnd), uMsg, wParam, lParam);
				break;
			*/
			default:
				break;
		}
		return p_view ? CallWindowProc(p_view->PrevWindowProc, hWnd, uMsg, wParam, lParam) : 1;
	}
	else
		return TRUE;
}

INT_PTR CALLBACK ListViewDialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) // @callback(DLGPROC)
{
	SmartListBox * p_view = static_cast<SmartListBox *>(TView::GetWindowUserData(hWnd));
	switch(uMsg) {
		case WM_DESTROY: // Не вызывается. Непонятно правда почему.
			CALLPTRMEMB(p_view, OnDestroy(hWnd));
			return 0;
		case WM_KEYDOWN:
		case WM_SYSKEYDOWN: ::SendMessageW(GetParent(hWnd), WM_VKEYTOITEM, MAKEWPARAM((WORD)wParam, 0), reinterpret_cast<LPARAM>(hWnd)); break; // Process by default
		case WM_SETFOCUS:
		case WM_KILLFOCUS: ::SendMessageW(GetParent(hWnd), uMsg, wParam, reinterpret_cast<LPARAM>(hWnd)); break;
		case WM_CHAR: ::SendMessageW(GetParent(hWnd), uMsg, wParam, reinterpret_cast<LPARAM>(hWnd)); return 0;
		case WM_LBUTTONDOWN: ::PostMessageW(GetParent(hWnd), WM_COMMAND, MAKEWPARAM((WORD)GetDlgCtrlID(hWnd), (WORD)LBN_SELCHANGE), reinterpret_cast<LPARAM>(hWnd)); break;
		case WM_LBUTTONDBLCLK: ::SendMessageW(GetParent(hWnd), WM_COMMAND, MAKEWPARAM((WORD)GetDlgCtrlID(hWnd), (WORD)LBN_DBLCLK), reinterpret_cast<LPARAM>(hWnd)); return 0;
		case WM_RBUTTONDOWN:
			CALLPTRMEMB(p_view, handleWindowsMessage(uMsg, wParam, lParam));
			//::SendMessageW(GetParent(hWnd), uMsg, MAKEWPARAM(LOWORD(wParam), 1), lParam);
			break;
		/*
		case WM_INPUTLANGCHANGE: // @v6.4.4 AHTOXA
			::PostMessageW(GetParent(hWnd), uMsg, wParam, lParam);
			break;
		*/
	}
	INT_PTR result = p_view ? CallWindowProc(p_view->PrevWindowProc, hWnd, uMsg, wParam, lParam) : 1;
	return result;
}
//
//
//
SmartListBox::SmartListBox(const TRect & rRect, ListBoxDef * pDef, bool isTreeList) : TView(rRect), Columns(sizeof(ColumnDescr)),
	State(0), SrchPatternPos(0), ColumnsSpcPos(0), P_Def(0), Range(0), ItemHeight(0), Top(0), HIML(0), SrchFunc(PTR_CMPFUNC(_PcharNoCase))
{
	SubSign = TV_SUBSIGN_LISTBOX;
	StrPool.add("$"); // zero index - is empty string
	SetTreeListState(isTreeList);
	ViewOptions |= ofSelectable | ofFirstClick;
	setDef(pDef);
}

SmartListBox::SmartListBox(const TRect & rRect, ListBoxDef * pDef, const char * pColumnsDeclaration) : TView(rRect), Columns(sizeof(ColumnDescr)),
	State(0), SrchPatternPos(0), ColumnsSpcPos(0), P_Def(0), Range(0), ItemHeight(0), Top(0), HIML(0), SrchFunc(PTR_CMPFUNC(_PcharNoCase))
{
	SubSign = TV_SUBSIGN_LISTBOX;
	StrPool.add("$"); // zero index - is empty string
	SetTreeListState(false);
	ViewOptions |= ofSelectable|ofFirstClick;
	setDef(pDef);
	SetupColumns(pColumnsDeclaration); // @v12.2.4
}

SmartListBox::~SmartListBox()
{
	// @v12.2.4 {
	{
		const  HWND h_lb = getHandle();
		if(IsWindow(h_lb)) {
			const  uint cc = Columns.getCount();
			if(cc)
				ListView_DeleteAllItems(h_lb);
			else
				::SendMessageW(h_lb, LB_RESETCONTENT, 0, 0);
		}
	}
	// } @v12.2.4 
	RestoreOnDestruction();
	delete P_Def;
	if(HIML)
		ImageList_Destroy(static_cast<HIMAGELIST>(HIML));
}

int SmartListBox::SearchColumnByIdent(long ident, uint * pPos) const
{
	int    ok = 0;
	uint   pos = 0;
	for(uint i = 0; !ok && i < Columns.getCount(); i++) {
		const ColumnDescr * p_item = static_cast<const ColumnDescr *>(Columns.at(i));
		if(p_item->Ident == ident) {
			pos = i;
			ok = 1;
		}
	}
	ASSIGN_PTR(pPos, pos);
	return ok;
}

int SmartListBox::AddColumn(int pos, const char * pTitle, uint width, uint format, long ident)
{
	int    ok = 1;
	SString temp_buf;
	SString title(pTitle);
	if(!title.NotEmptyS()) {
		title.Space();
	}
	uint cf = CHKXORFLAGS(format, STRF_ANSI, STRF_OEM);
	if(title.C(0) == '@') {
		if(cf == STRF_ANSI) {
			title.Transf(CTRANSF_OUTER_TO_INNER);
			cf = STRF_OEM;
		}
		if(SLS.LoadString_(title.ShiftLeft(), temp_buf) > 0) {
			title = temp_buf.Strip();
			cf = STRF_OEM;
		}
	}
	if(oneof2(cf, STRF_OEM, 0))
		title.Transf(CTRANSF_INNER_TO_OUTER);
	SETIFZ(width, title.Len());
	{
		ColumnDescr item;
		MEMSZERO(item);
		item.Width = MIN(255, MAX(width, 2));
		if(format & ALIGN_LEFT)
			item.Format |= ALIGN_LEFT;
		else if(format & ALIGN_RIGHT)
			item.Format |= ALIGN_RIGHT;
		else if(format & ALIGN_CENTER)
			item.Format |= ALIGN_CENTER;
		else
			item.Format |= ALIGN_LEFT;
		item.Ident = ident;
		if(title.Len())
			StrPool.add(title, &item.TitlePos);
		if(pos < 0 || pos >= Columns.getCountI())
			pos = Columns.getCount();
		ok = Columns.atInsert((uint)pos, &item);
	}
	if(ok)
		Helper_InsertColumn(static_cast<uint>(pos));
	return ok;
}

int SmartListBox::RemoveColumn(int pos)
{
	uint   _p = Columns.getCount();
	if(pos < 0 || pos >= (int)_p) {
		if(_p)
			_p--;
	}
	else
		_p = (uint)pos;
	if(_p < Columns.getCount()) {
		ListView_DeleteColumn(getHandle(), _p);
		Columns.atFree(_p);
		return 1;
	}
	else
		return 0;
}

bool SmartListBox::GetOrgColumnsDescr(SString & rBuf) const
{
	return StrPool.getnz(ColumnsSpcPos, rBuf.Z());
}

int SmartListBox::SetupColumns(const char * pColsBuf)
{
	constexpr uint default_column_width = 2; // @v12.2.4
	RemoveColumns();
	SString columns_buf(pColsBuf);
	if(columns_buf.NotEmptyS()) {
		SString title_buf;
		SString cstr;
		SString citem;
		SString left;
		SString right;
		if(columns_buf.Strip().C(0) == '@' && SLS.LoadString_(columns_buf.ShiftLeft(), cstr) > 0)
			columns_buf = cstr;
		columns_buf.Transf(CTRANSF_INNER_TO_OUTER);
		StringSet columns_ss(';', columns_buf);
		StringSet ss(",");
		for(uint columns_pos = 0; columns_ss.get(&columns_pos, cstr);) {
			uint   pos = 0;
			ss.setBuf(cstr);
			uint   width = default_column_width; // @v12.2.4 0-->default_column_width
			uint   format = ALIGN_LEFT; // @v12.2.4 0-->ALIGN_LEFT
			title_buf.Z();
			if(ss.get(&pos, citem)) {
				width = static_cast<uint16>((citem.Divide('w', left, right) > 0) ? right.ToLong() : citem.ToLong());
				SETIFZ(width, default_column_width); // @v12.2.4
				if(ss.get(&pos, citem)) {
					switch(toupper(citem.Strip().C(0))) {
						case 'L': format = ALIGN_LEFT; break;
						case 'R': format = ALIGN_RIGHT; break;
						case 'C': format = ALIGN_CENTER; break;
						default: format = ALIGN_LEFT; break;
					}
					ss.get(&pos, title_buf);
				}
			}
			AddColumn(-1, title_buf, width, format|STRF_ANSI, 0);
		}
		StrPool.add(columns_buf, &ColumnsSpcPos);
	}
	return 1;
}

void SmartListBox::RemoveColumns()
{
	State &= ~stInited;
	for(long i = static_cast<long>(Columns.getCount()); i >= 0; i--)
		RemoveColumn(i);
}

int    FASTCALL SmartListBox::getCurID(long * pId) { return P_Def ? P_Def->getCurID(pId) : 0; }
int    FASTCALL SmartListBox::getCurData(void * pData) { return P_Def ? P_Def->getCurData(pData) : 0; }
int    FASTCALL SmartListBox::getCurString(SString & rBuf) { return P_Def ? P_Def->getCurString(rBuf) : (rBuf.Z(), 0); }
void   SmartListBox::setHorzRange(int) {}

uint SmartListBox::GetSelectionList(LongArray * pList)
{
	uint   result = 0;
	CALLPTRMEMB(pList, Z());
	if(P_Def) {
		if(IsTreeList()) {
			long   cur_id = 0;
			if(getCurID(&cur_id)) {
				CALLPTRMEMB(pList, add(cur_id));
				result = 1;
			}
		}
		else {
			HWND hw = getHandle();
			if(hw) {
				long   cur_id__ = 0;
				const  int gcir = getCurID(&cur_id__);
				const long s = TView::SGetWindowStyle(hw);
				if(s & LBS_EXTENDEDSEL) {
					LRESULT c = ::SendMessageW(hw, LB_GETSELCOUNT, 0, 0);
					if(c > 0) {
						if(pList) {
							int32 * p_buffer = 0;
							int32   stbuf[128];
							bool is_buf_allocated = false;
							if(c > SIZEOFARRAY(stbuf)) {
								p_buffer = new int32[c+8];
								is_buf_allocated = true;
							}
							else {
								p_buffer = stbuf;
							}
							LRESULT c2 = ::SendMessageW(hw, LB_GETSELITEMS, c, reinterpret_cast<LPARAM>(p_buffer));
							if(c2 > 0) {
								for(uint i = 0; i < static_cast<uint>(c2); i++) {
									int32 idx = p_buffer[i];
									if(idx >= 0 && idx < P_Def->GetRecsCount()) {
										const void * p_row = P_Def->getRow_(idx);
										if(p_row)
											pList->add(*static_cast<const long *>(p_row));
									}
								}
							}
							if(is_buf_allocated) {
								delete [] p_buffer;
							}
							p_buffer = 0;
						}
					}
					result = static_cast<uint>(c);
				}
				else {
					if(gcir) {
						CALLPTRMEMB(pList, add(cur_id__));
						result = 1;
					}
				}
			}
		}
	}
	return result;
}

int  SmartListBox::getText(long itemN  /* 0.. */, SString & rBuf)
{
	int    ok = 0;
	rBuf.Z();
	if(P_Def) {
		P_Def->getText(itemN, rBuf);
		ok = 1;
	}
	return ok;
}

int  SmartListBox::getID(long itemN, long * pID)
{
	int    ok = 0;
	if(P_Def) {
		const char * p = static_cast<const char *>(P_Def->getRow_(itemN));
		if(p) {
			uint   h = (P_Def->Options & lbtAutoID) ? 0 : ((P_Def->Options & lbtWordID) ? 2 : 4);
			if(h == 2) {
				int16  id = *reinterpret_cast<const int16 *>(p);
				ASSIGN_PTR(pID, static_cast<long>(id));
			}
			else {
				long   id = *reinterpret_cast<const long *>(p);
				ASSIGN_PTR(pID, id);
			}
			ok = 1;
		}
	}
	return ok;
}

void SmartListBox::Helper_InsertColumn(uint pos)
{
	if(pos < Columns.getCount()) {
		ColumnDescr & slbc = *static_cast<ColumnDescr *>(Columns.at(pos));
		SString title_buf;
		LVCOLUMN lv;
		lv.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH;
		lv.cx = 6 * slbc.Width;
		StrPool.getnz(slbc.TitlePos, title_buf);
		lv.pszText = const_cast<TCHAR *>(SUcSwitch(title_buf.cptr())); // @badcast
		if(slbc.Format & ALIGN_LEFT)
			lv.fmt = LVCFMT_LEFT;
		else if(slbc.Format & ALIGN_RIGHT)
			lv.fmt = LVCFMT_RIGHT;
		else if(slbc.Format & ALIGN_CENTER)
			lv.fmt = LVCFMT_CENTER;
		else
			lv.fmt = LVCFMT_LEFT;
		ListView_InsertColumn(getHandle(), pos, &lv);
	}
}

void SmartListBox::MoveScrollBar(int autoHeight)
{
	long   scroll_delta = 0;
	long   scroll_pos = 0;
	CreateScrollBar(1);
	if(P_Def) {
		if(autoHeight) {
			P_Def->SetOption(lbtHSizeAlreadyDef, 0);
			P_Def->setViewHight(GetMaxListHeight());
		}
		P_Def->getScrollData(&scroll_delta, &scroll_pos);
		if(P_Def->_curItem() == 0)
			scroll_pos = 0;
		setRange(P_Def->GetRecsCount());
	}
	SetScrollBarPos(scroll_pos, 1);
	Draw_();
}

void SmartListBox::CreateScrollBar(int create)
{
	HWND h_wnd = GetDlgItem(Parent, MAKE_BUTTON_ID(Id, 1));
	::DestroyWindow(h_wnd);
	if(create) {
		int    tabbed = TView::SGetWindowStyle(Parent) & WS_CHILD;
		const  HWND h_lb = getHandle();
		RECT   rc_list;
		::GetWindowRect(h_lb, &rc_list);
		int   sc_width = GetSystemMetrics(SM_CXVSCROLL);
		int   sc_height = rc_list.bottom - rc_list.top;
		POINT sc_lu;
		sc_lu.x = rc_list.right;
		sc_lu.y = rc_list.top;
		::MapWindowPoints(NULL, Parent, &sc_lu, 1);
		h_wnd = ::CreateWindowExW(0, L"SCROLLBAR", L"", WS_CHILD|SBS_LEFTALIGN|SBS_VERT, sc_lu.x, sc_lu.y, sc_width, sc_height, Parent, 
			reinterpret_cast<HMENU>(MAKE_BUTTON_ID(Id, 1)), TProgram::GetInst(), 0);
		::ShowWindow(h_wnd, SW_SHOWNORMAL);
	}
}

void SmartListBox::SetScrollBarPos(long pos, LPARAM lParam)
{
	HWND h_wnd = GetDlgItem(Parent, MAKE_BUTTON_ID(Id, 1));
	if(h_wnd)
		::SendMessageW(h_wnd, SBM_SETPOS, pos, lParam);
}

int  SmartListBox::GetMaxListHeight()
{
	const  HWND h_lb = getHandle();
	RECT   list_rect;
	::GetClientRect(Parent, &list_rect);
	const int list_height = list_rect.bottom - list_rect.top;
	int item_height = ::SendMessageW(h_lb, LB_GETITEMHEIGHT, 0, 0);
	// @v12.2.4 {
	if(item_height <= 0) {
		int sihr = ::SendMessageW(h_lb, LB_SETITEMHEIGHT, 0, static_cast<LPARAM>(20));
		item_height = ::SendMessageW(h_lb, LB_GETITEMHEIGHT, 0, 0);
	}
	// } @v12.2.4 
	return (P_Def && P_Def->Options & lbtHSizeAlreadyDef) ? P_Def->ViewHight : (item_height ? (list_height-5) / item_height : 0);
}

void SmartListBox::onInitDialog(int useScrollBar)
{
	const  HWND h_lb = getHandle();
	DLGPROC dlg_proc = 0;
	// @v11.2.4 {
	if(Parent) {
		const long exstyle = TView::SGetWindowExStyle(Parent);
		if(exstyle & WS_EX_COMPOSITED)
			TView::SetWindowProp(Parent, GWL_EXSTYLE, (exstyle & ~WS_EX_COMPOSITED));
	}
	// } @v11.2.4 
	if(State & stTreeList) {
		dlg_proc = TreeListBoxDialogProc;
	}
	else {
		RECT   rc_cli_parent;
		RECT   rc_list;
		RECT   rc_parent;
		//const bool is_multi_col = LOGIC(Columns.getCount());
		const bool is_tabbed = LOGIC(TView::SGetWindowStyle(Parent) & WS_CHILD);
		::GetClientRect(Parent, &rc_cli_parent);
		if(!IsMultiColumn() && !GetDlgItem(Parent, MAKE_BUTTON_ID(Id, 1))) {
			::GetWindowRect(Parent, &rc_parent);
			::GetWindowRect(h_lb, &rc_list);
			POINT  list_lu;
			list_lu.x = rc_list.left;
			list_lu.y = rc_list.top;
			::MapWindowPoints(NULL, Parent, &list_lu, 1);
			int    list_width = rc_list.right - rc_list.left - GetSystemMetrics(SM_CXVSCROLL);
			int    list_height = rc_list.bottom - rc_list.top;
			::MoveWindow(h_lb, list_lu.x, list_lu.y, list_width, list_height, 1);
			::GetWindowRect(h_lb, &rc_list);
			CreateScrollBar(useScrollBar);
		}
		if(P_Def)
			setRange(P_Def->GetRecsCount());
		//
		::GetWindowRect(h_lb, &rc_list);
		int    item_height;
		int    list_height = (rc_list.bottom - rc_list.top);
		if(IsMultiColumn()) {
			RECT   rc;
			HWND   hwh = ListView_GetHeader(h_lb);
			::GetWindowRect(hwh, &rc);
			item_height = rc.bottom - rc.top - 4;
			SForEachVectorItem(Columns, i) { Helper_InsertColumn(i); }
			ItemHeight = ((list_height-3) / item_height) - 1;
			ListView_SetExtendedListViewStyle(h_lb, LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES);
		}
		else {
			ItemHeight = GetMaxListHeight();
			/*
			item_height = ::SendMessageW(h_lb, LB_GETITEMHEIGHT, 0, 0);
			ItemHeight = (def && def->Options & lbtHSizeAlreadyDef) ? def->ViewHight : (item_height ? (list_height-5) / item_height : 0);
			*/
			if(P_Def) {
				long   scroll_delta = 0;
				long   scroll_pos = 0;
				Top = P_Def->_topItem();
				P_Def->getScrollData(&scroll_delta, &scroll_pos);
				if(P_Def->_curItem() == 0)
					scroll_pos = 0;
				SetScrollBarPos(scroll_pos, 1);
			}
		}
		CALLPTRMEMB(P_Def, setViewHight(ItemHeight));
		dlg_proc = IsMultiColumn() ? ListViewDialogProc : ListBoxDialogProc;
	}
	State |= stInited;
	Draw_();
	PrevWindowProc = static_cast<WNDPROC>(TView::SetWindowProp(h_lb, GWLP_WNDPROC, dlg_proc));
	TView::SetWindowUserData(h_lb, this);
}

void SmartListBox::Helper_ClearTreeWnd()
{
	HWND   h_lb = getHandle();
	if(h_lb)
		TreeView_DeleteAllItems(h_lb);
}

int  SmartListBox::SetupTreeWnd2(void * pParent)
{
	int    ok = -1;
	HWND   h_lb = getHandle();
	if(P_Def && h_lb) {
		HTREEITEM h_parent = 0;
		long   save_pos = 0;
		SString err_msg;
		StdTreeListBoxDef * p_def = 0;
		StdTreeListBoxDef2_ * p_def2 = 0;
		if(P_Def->GetSignature() == SlConst::ListBoxDefSignature_STDTREE)
			p_def = static_cast<StdTreeListBoxDef *>(P_Def);
		else if(P_Def->GetSignature() == SlConst::ListBoxDefSignature_STDTREE2)
			p_def2 = static_cast<StdTreeListBoxDef2_ *>(P_Def);
		if(p_def2) {
			StrAssocTree * p_tree = p_def2->P_SaList;
			if(p_tree) {
				if(pParent) {
					//SPtrHandle h = p_tree->Search(parentP);
					h_parent = reinterpret_cast<HTREEITEM>(p_tree->GetNodeExtraPtr(SPtrHandle(pParent)));
				}
				else {
					save_pos = p_def2->_curItem();
					Helper_ClearTreeWnd();
					//
					// добавляем список картинок
					//
					{
						if(HIML)
							ImageList_Destroy(static_cast<HIMAGELIST>(HIML));
						HIML = p_def2->CreateImageList(TProgram::GetInst());
						if(HIML)
							::SendMessageW(static_cast<HWND>(h_lb), (UINT)TVM_SETIMAGELIST, static_cast<WPARAM>(TVSIL_NORMAL), reinterpret_cast<LPARAM>(HIML));
					}
				}
				{
					//LongArray item_id_list;
					//p_tree->GetListByParent(parentP, false, item_id_list);
					TSVector <SPtrHandle> item_h_list;
					p_tree->GetListByParent_Unsafe(SPtrHandle(pParent), false, item_h_list);
					//for(uint i = 0; i < item_id_list.getCount(); i++) {
					for(uint i = 0; i < item_h_list.getCount(); i++) {
						//const long current_id = item_id_list.get(i);
						//SPtrHandle current_h = p_tree->Search(current_id);
						SPtrHandle current_h = item_h_list.at(i);
						if(current_h) {
							const long current_id = p_tree->GetNodeKey(current_h);
							if(pParent || p_def2->BelongToTopLevelResriction(current_id)) {
								TVINSERTSTRUCT is;
								is.hParent      = NZOR(h_parent, TVI_ROOT);
								is.hInsertAfter = TVI_LAST;
								is.item.mask    = TVIF_TEXT | TVIF_PARAM | TVIF_CHILDREN;
								if(p_def2->GetImageIdxByID(current_id, 0) > 0) {
									is.item.iImage = I_IMAGECALLBACK;
									is.item.iSelectedImage = I_IMAGECALLBACK;
									is.item.mask |= (TVIF_IMAGE|TVIF_SELECTEDIMAGE);
								}
								//const  uint32 first_child_p = p_tree->GetFirstChildP(t_iter.GetCurrentPos());
								bool has_children = p_tree->HasNodeChildren(current_h);
								is.item.cChildren = has_children ? 1 : 0;
								is.item.pszText = LPSTR_TEXTCALLBACK;
								is.item.lParam  = current_id;
								HTREEITEM h_tree = TreeView_InsertItem(h_lb, &is);
								if(h_tree) {
									p_tree->SetNodeExtraPtr(current_h, reinterpret_cast<uintptr_t>(h_tree));
									//p_item->H = h_tree;
									if(has_children /*&& !(parentP == 0 && p_item->Id == 0)*/)
										SetupTreeWnd2(current_h); // @recursion
								}
								else {
									//
									// @? Здесь программа получает сообщение от системы, которое нигде ни используется!
									//
									SSystem::SFormatMessage(err_msg); 
									err_msg.Chomp();
								}
							}
						}
					}
				}
			}
		}
		else if(p_def) {
			uint32 parent_idx = reinterpret_cast<uint32>(pParent);
			if(parent_idx) {
				const StdTreeListBoxDef::TreeItem * p_item = static_cast<const StdTreeListBoxDef::TreeItem *>(p_def->T.GetData(parent_idx));
				if(p_item)
					h_parent = static_cast<HTREEITEM>(p_item->H);
			}
			else {
				save_pos = p_def->_curItem();
				Helper_ClearTreeWnd();
				//
				// добавляем список картинок
				//
				{
					if(HIML)
						ImageList_Destroy(static_cast<HIMAGELIST>(HIML));
					HIML = p_def->CreateImageList(TProgram::GetInst());
					if(HIML)
						::SendMessageW(static_cast<HWND>(h_lb), (UINT)TVM_SETIMAGELIST, static_cast<WPARAM>(TVSIL_NORMAL), reinterpret_cast<LPARAM>(HIML));
				}
			}
			{
				for(STree::Iter t_iter(parent_idx); p_def->T.Enum(t_iter);) {
					StdTreeListBoxDef::TreeItem * p_item = static_cast<StdTreeListBoxDef::TreeItem *>(t_iter.GetData());
					if(p_item) {
						TVINSERTSTRUCT is;
						is.hParent      = NZOR(h_parent, TVI_ROOT);
						is.hInsertAfter = TVI_LAST;
						is.item.mask    = TVIF_TEXT | TVIF_PARAM | TVIF_CHILDREN;
						if(p_def->GetImageIdxByID(p_item->Id, 0) > 0) {
							is.item.iImage = I_IMAGECALLBACK;
							is.item.iSelectedImage = I_IMAGECALLBACK;
							is.item.mask |= (TVIF_IMAGE|TVIF_SELECTEDIMAGE);
						}
						const  uint32 first_child_p = p_def->T.GetFirstChildP(t_iter.GetCurrentPos());
						is.item.cChildren = first_child_p ? 1 : 0;
						is.item.pszText = LPSTR_TEXTCALLBACK;
						is.item.lParam  = p_item->Id;
						HTREEITEM h_tree = TreeView_InsertItem(h_lb, &is);
						if(h_tree) {
							p_item->H = h_tree;
							if(first_child_p && !(parent_idx == 0 && p_item->Id == 0))
								SetupTreeWnd2(reinterpret_cast<void *>(t_iter.GetCurrentPos())); // @recursion
						}
						else {
							//
							// @? Здесь программа получает сообщение от системы, которое нигде ни используется!
							//
							SSystem::SFormatMessage(err_msg); 
							err_msg.Chomp();
						}
						ok = 1;
					}
				}
			}
			if(pParent == 0)
				P_Def->go(save_pos);
		}
	}
	return ok;
}

int  SmartListBox::GetStringByID(long id, SString & rBuf)
{
	rBuf.Z();
	int    ok = 0;
	if(P_Def) {
		if(P_Def->GetSignature() == SlConst::ListBoxDefSignature_STDTREE) {
			StdTreeListBoxDef * p_def = static_cast<StdTreeListBoxDef *>(P_Def);
			ok = p_def->GetStringByID(id, rBuf);
		}
		else if(P_Def->GetSignature() == SlConst::ListBoxDefSignature_STDTREE2) {
			StdTreeListBoxDef2_ * p_def = static_cast<StdTreeListBoxDef2_ *>(P_Def);
			ok = p_def->GetStringByID(id, rBuf);
		}
	}
	return ok;
}

int  SmartListBox::GetImageIdxByID(long id, long * pIdx) 
{
	int    ok = 0;
	if(P_Def) {
		if(P_Def->GetSignature() == SlConst::ListBoxDefSignature_STDTREE) {
			StdTreeListBoxDef * p_def = static_cast<StdTreeListBoxDef *>(P_Def);
			ok = p_def->GetImageIdxByID(id, pIdx);
		}
		else if(P_Def->GetSignature() == SlConst::ListBoxDefSignature_STDTREE2) {
			StdTreeListBoxDef2_ * p_def = static_cast<StdTreeListBoxDef2_ *>(P_Def);
			ok = p_def->GetImageIdxByID(id, pIdx);
		}
		else if(P_Def->GetSignature() == SlConst::ListBoxDefSignature_STD) {
			StdListBoxDef * p_def = static_cast<StdListBoxDef *>(P_Def);
			ok = p_def->GetImageIdxByID(id, pIdx);
		}
	}
	return ok;
}

/* @v11.4.4 (inlined) int  FASTCALL SmartListBox::onVKeyToItem(WPARAM wParam)
{
	int    nScrollCode;
	int    sf = 0;
	switch(LOWORD(wParam)) {
		case VK_UP:    nScrollCode = SB_LINEUP;   break;
		case VK_DOWN:  nScrollCode = SB_LINEDOWN; break;
		case VK_NEXT:  nScrollCode = SB_PAGEDOWN; break;
		case VK_PRIOR: nScrollCode = SB_PAGEUP;   break;
		case VK_HOME:  nScrollCode = SB_TOP;      break;
		case VK_END:   nScrollCode = SB_BOTTOM;   break;
		case VK_INSERT:
			if(IsInState(sfSelected) || State & stTreeList) {
				MessageCommandToOwner(cmaInsert);
				sf = 1;
			}
			break;
		case VK_DELETE:
			if(IsInState(sfSelected) || State & stTreeList) {
				MessageCommandToOwner(cmaDelete);
				sf = 1;
			}
			break;
		case VK_ADD:
			MessageCommandToOwner(cmaLevelDown);
			sf = 1;
			break;
		case VK_SUBTRACT:
			MessageCommandToOwner(cmaLevelUp);
			sf = 1;
			break;
		default:
			return 0;
	}
	if(!(State & stTreeList)) {
		if(sf) {
			SetFocus(getHandle());
			return 1;
		}
		Scroll(nScrollCode, 0);
	}
	return -1;
}*/

int SmartListBox::handleWindowsMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg) {
		case WM_INITDIALOG:
			onInitDialog(1);
			break;
		case WM_DESTROY: // данная команда практически никогда не вызывается. См. WM_DESTROY в DialogProc.
			// Из-за этого, если список картинок валиден, то на каждом вызове диалога с таким списком теряется 4 объекта GDI.
			/*{ // @v6.0.14 AHTOXA
				HIMAGELIST himl = (HIMAGELIST)::SendMessageW(::GetDlgItem(Parent, Id), (UINT)TVM_SETIMAGELIST, (WPARAM)TVSIL_NORMAL, (LPARAM)0);
				if(himl) {
					ImageList_RemoveAll(himl);
					ImageList_Destroy(himl);
				}
			}*/ // } @v6.0.14 AHTOXA
			OnDestroy(getHandle());
			break;
		case WM_COMMAND: {
			switch(HIWORD(wParam)) {
				case LBN_DBLCLK:
					MessageCommandToOwner(cmLBDblClk);
					break;
				case LBN_SELCHANGE:
					if(P_Def) {
						const int index = IsMultiColumn() ?
							SendDlgItemMessage(Parent, Id, LVM_GETNEXTITEM, -1, MAKELPARAM(LVNI_SELECTED, 0)) :
							SendDlgItemMessage(Parent, Id, LB_GETCURSEL, 0, 0) + P_Def->_topItem();
						if(index >= 0) {
							long   prev_top_item = Top;
							P_Def->go(index);
							if(!IsMultiColumn() && Top != prev_top_item)
								Draw_();
							if(P_Def->Options & lbtFocNotify)
								MessageCommandToOwner(cmLBItemFocused);
							Scroll(-1, 0);
							if(P_Def->Options & lbtSelNotify)
								MessageCommandToOwner(cmLBItemSelected);
						}
					}
					break;
			}
			break;
		}
		case WM_RBUTTONDOWN:
			{
				if(!IsMultiColumn()) {
					const long hw_clarea_lw_index = SendDlgItemMessage(Parent, Id, LB_ITEMFROMPOINT, 0, lParam);
					if(HIWORD(hw_clarea_lw_index) == 0 && LOWORD(hw_clarea_lw_index) >= 0) {
						const long prev_top_item = Top;
						P_Def->go(LOWORD(hw_clarea_lw_index) + P_Def->_topItem());
						if(Top != prev_top_item)
							Draw_();
						if(P_Def->Options & lbtFocNotify)
							MessageCommandToOwner(cmLBItemFocused);
						Scroll(-1, 0);
					}
				}
				MessageCommandToOwner(cmRightClick);
			}
			break;
		case WM_VKEYTOITEM:
			{
				int    n_scroll_code = 0;
				int    sf = 0;
				switch(LOWORD(wParam)) {
					case VK_UP:    n_scroll_code = SB_LINEUP;   break;
					case VK_DOWN:  n_scroll_code = SB_LINEDOWN; break;
					case VK_NEXT:  n_scroll_code = SB_PAGEDOWN; break;
					case VK_PRIOR: n_scroll_code = SB_PAGEUP;   break;
					case VK_HOME:  n_scroll_code = SB_TOP;      break;
					case VK_END:   n_scroll_code = SB_BOTTOM;   break;
					case VK_INSERT:
						if(IsInState(sfSelected) || State & stTreeList) {
							MessageCommandToOwner(cmaInsert);
							sf = 1;
						}
						break;
					case VK_DELETE:
						if(IsInState(sfSelected) || State & stTreeList) {
							MessageCommandToOwner(cmaDelete);
							sf = 1;
						}
						break;
					case VK_ADD:
						MessageCommandToOwner(cmaLevelDown);
						sf = 1;
						break;
					case VK_SUBTRACT:
						MessageCommandToOwner(cmaLevelUp);
						sf = 1;
						break;
					default:
						return 0;
						break;
				}
				if(!(State & stTreeList)) {
					if(sf) {
						SetFocus(getHandle());
						return 1;
					}
					else
						Scroll(n_scroll_code, 0);
				}
			}
			break;
		case WM_CHAR:
			if(wParam == kbCtrlF)
				search(0, srchFirst);
			else if(wParam == kbCtrlG)
				search(0, srchNext);
			else {
				const char ac = SSystem::TranslateWmCharToAnsi(wParam);
				char b[2];
				b[0] = ac;
				b[1] = 0;
				SCharToOem(b);
				const uchar ub = b[0];
				if(!(State & stOmitSearchByFirstChar)) {
					if(isalnum(ub) || IsLetter866(ub) || ub == '*')
						search(b, srchFirst);
					else
						return 0;
				}
				else {
					if(isalnum(ub) || IsLetter866(ub))
						return 0;
					else
						return 1;
				}
			}
			break;
		case WM_MOUSEWHEEL:
			{
				const short delta = static_cast<short>(HIWORD(wParam));
				const int scroll_code = (delta > 0) ? SB_LINEUP : SB_LINEDOWN;
				for(int i = 0; i < 3; i++)
					Scroll(scroll_code, 0);
			}
			break;
		case WM_VSCROLL:
			Scroll(LOWORD(wParam), static_cast<int16>(HIWORD(wParam)));
			break;
		case WM_NOTIFY:
			const NMHDR * p_nm = reinterpret_cast<const NMHDR *>(lParam);
			/* @construction
			if(p_nm->code == LVN_BEGINDRAG) {
				NMLISTVIEW * p_nmlv = (NMLISTVIEW *)lParam;
			}
			*/
			if(State & stTreeList) {
				switch(p_nm->code) {
					case TVN_GETDISPINFO:
						{
							LPNMTVDISPINFO lptvdi = reinterpret_cast<LPNMTVDISPINFO>(lParam);
							if(lptvdi->item.mask & TVIF_TEXT) {
								SString & r_temp_buf = SLS.AcquireRvlStr();
								GetStringByID(static_cast<long>(lptvdi->item.lParam), r_temp_buf);
								// @debug {
								if(!r_temp_buf.NotEmptyS())
									r_temp_buf.Z().CatChar('#').Cat(lptvdi->item.lParam);
								// } @debug
								r_temp_buf.Transf(CTRANSF_INNER_TO_OUTER);
								strnzcpy(lptvdi->item.pszText, SUcSwitch(r_temp_buf), lptvdi->item.cchTextMax);
							}
							if(lptvdi->item.mask & (TVIF_IMAGE|TVIF_SELECTEDIMAGE)) {
								long idx = 0;
								lptvdi->item.iImage = -1;
								lptvdi->item.iSelectedImage = -1;
								GetImageIdxByID(lptvdi->item.lParam, &idx);
								if(lptvdi->item.mask & TVIF_IMAGE)
									lptvdi->item.iImage = static_cast<int>(idx);
								if(lptvdi->item.mask & TVIF_SELECTEDIMAGE)
									lptvdi->item.iSelectedImage = static_cast<int>(idx);
							}
						}
						break;
					case TVN_SELCHANGED:
						{
							LPNMTREEVIEW pnmtv = reinterpret_cast<LPNMTREEVIEW>(lParam);
							if(P_Def->GetSignature() == SlConst::ListBoxDefSignature_STDTREE) {
								StdTreeListBoxDef * p_def = static_cast<StdTreeListBoxDef *>(P_Def);
								p_def->GoByID(pnmtv->itemNew.lParam);
							}
							else if(P_Def->GetSignature() == SlConst::ListBoxDefSignature_STDTREE2) {
								StdTreeListBoxDef2_ * p_def = static_cast<StdTreeListBoxDef2_ *>(P_Def);
								p_def->GoByID(pnmtv->itemNew.lParam);
							}
							if(P_Def->Options & lbtFocNotify)
								MessageCommandToOwner(cmLBItemFocused);
							if(State & stLButtonDown) {
								if(P_Def->Options & lbtSelNotify)
									MessageCommandToOwner(cmLBItemSelected);
								State &= ~stLButtonDown;
							}
						}
						break;
					case NM_RCLICK:
					case NM_DBLCLK:
						{
							RECT list_rect;
							TVHITTESTINFO ht;
							HWND   h_tlist = getHandle();
							GetWindowRect(h_tlist, &list_rect);
							GetCursorPos(&ht.pt);
							ht.pt.x -= list_rect.left;
							ht.pt.y -= list_rect.top;
							TVITEM t_item;
							t_item.mask = TVIF_PARAM;
							t_item.hItem = TreeView_HitTest(h_tlist, &ht);
							TreeView_GetItem(h_tlist, &t_item);
							if(P_Def->GetSignature() == SlConst::ListBoxDefSignature_STDTREE) {
								StdTreeListBoxDef * p_def = static_cast<StdTreeListBoxDef *>(P_Def);
								p_def->GoByID(t_item.lParam);
							}
							else if(P_Def->GetSignature() == SlConst::ListBoxDefSignature_STDTREE2) {
								StdTreeListBoxDef2_ * p_def = static_cast<StdTreeListBoxDef2_ *>(P_Def);
								p_def->GoByID(t_item.lParam);
							}
							SelectTreeItem();
							MessageCommandToOwner((p_nm->code == NM_RCLICK) ? cmRightClick : cmLBDblClk);
						}
						break;
					case NM_CLICK:
						State |= stLButtonDown;
						break;
					/*
					case NM_CUSTOMDRAW:
						{
							long lvn_res = 0;
							NMLVCUSTOMDRAW * p_nmlvcd = (NMLVCUSTOMDRAW*)lParam;
							switch(p_nmlvcd->nmcd.dwDrawStage) {
								case CDDS_PREPAINT:
									lvn_res = CDRF_NOTIFYITEMDRAW;
									break;
								case CDDS_ITEMPREPAINT:
									if(TView::messageCommand(owner, cmLBItemDraw, this))
										lvn_res = CDRF_NEWFONT;
									break;
							}
							//SetWindowLong(hWnd, DWL_MSGRESULT, lvn_res);
							TView::SetWindowProp(hWnd, DWL_MSGRESULT, lvn_res);
						}
						break;
					*/
					default:
						return 0;
				}
			}
			else {
				if(p_nm && p_nm->code == NM_CUSTOMDRAW) {
					NMTVCUSTOMDRAW * p_cd = (NMTVCUSTOMDRAW *)p_nm;
					//HWND   h_ctl = p_nm->hwndFrom;
					long   result = CDRF_DODEFAULT;
					if(!p_cd)
						result = -1;
					else {
						switch(p_cd->nmcd.dwDrawStage) {
	    					case CDDS_PREPAINT:
								if(P_Def && P_Def->HasItemColorSpec())
									result = CDRF_NOTIFYITEMDRAW;
								else
									result = -1;
								break;
							case CDDS_ITEMPREPAINT:
								{
									long   _top = 0; //def->_topItem();
									long   item_pos = _top + p_cd->nmcd.dwItemSpec;
									long   item_id = 0;
									if((p_cd->nmcd.uItemState&CDIS_FOCUS) != CDIS_FOCUS) {
										if(getID(item_pos, &item_id)) {
											SColor bc, fc;
											if(P_Def->GetItemColor(item_id, &fc, &bc) > 0) {
												p_cd->clrText = static_cast<COLORREF>(fc);
												p_cd->clrTextBk = static_cast<COLORREF>(bc);
											}
										}
									}
									else {
										;
									}
								}
								break;
							}
					}
					TView::SetWindowProp(Parent, DWLP_MSGRESULT, result);
					return 0; // @v11.2.4
				}
				else
					return 0;
			}
			break;
	}
	return 1;
}

void SmartListBox::Scroll(short Code, int value)
{
	if(P_Def) {
		long   scroll_delta;
		long   scroll_pos;
		const  long prev_top_item = Top;
		int    to_draw = 0;
		int    need_sel = 1;
		int    fsize  = P_Def->GetFrameSize() - 1;
		int    fstate = P_Def->GetFrameState();
		switch(Code) {
			case SB_BOTTOM:
				if(fstate == -1)
					fstate = 2;
				if(fstate == 2 && P_Def->_curItem() >= fsize)
					need_sel = 0;
				else {
					P_Def->bottom();
					to_draw = 1;
					Top = P_Def->_curItem() - P_Def->ViewHight + 1;
				}
				break;
			case SB_LINEDOWN:
				if(fstate == -1)
					fstate = 2;
				if(fstate == 2 && P_Def->_curItem() >= fsize)
					need_sel = 0;
				else {
					P_Def->step(1);
					if((P_Def->_curItem() - Top) >= P_Def->ViewHight)
						Top++;
					to_draw = (P_Def->_curItem() < P_Def->ViewHight) ? 0 : 1;
				}
				break;
			case SB_LINEUP:
				if(fstate == -1)
					fstate = 1;
				if(fstate == 1 && P_Def->_curItem() == 0)
					need_sel = 0;
				else {
					P_Def->step(-1);
					to_draw = Top ? 0 : 1;
					if(Top > P_Def->_curItem())
						Top--;
					// @vmiller {
					else if((P_Def->_curItem() - P_Def->ViewHight) >= Top)
						Top = P_Def->_curItem();
					// } @vmiller
				}
				break;
			case SB_PAGEDOWN:
				if(fstate == -1)
					fstate = 2;
				if(fstate == 2 && P_Def->_curItem() >= fsize)
					need_sel = 0;
				else {
					P_Def->step(P_Def->ViewHight);
					to_draw = 1;
					Top = P_Def->_curItem() - P_Def->ViewHight + 1;
				}
				break;
			case SB_PAGEUP:
				if(fstate == -1)
					fstate = 1;
				if(fstate == 1 && P_Def->_curItem() == 0)
					need_sel = 0;
				else {
					P_Def->step(-P_Def->ViewHight);
					to_draw = 1;
					Top = P_Def->_curItem();
				}
				break;
			case SB_TOP:
				if(fstate == -1)
					fstate = 1;
				if(fstate == 1 && P_Def->_curItem() == 0)
					need_sel = 0;
				else {
					P_Def->top();
					to_draw = 1;
					Top = 0;
				}
				break;
			case SB_THUMBPOSITION:
			case SB_THUMBTRACK:
				if(P_Def->HasCapability(ListBoxDef::cCountable)) {
					if(value < 0)
						P_Def->top();
					else if(value >= P_Def->GetRecsCount())
						P_Def->bottom();
					else
						P_Def->go(value);
					to_draw = 1;
				}
				break;
			default:
				break;
		}
		P_Def->getScrollData(&scroll_delta, &scroll_pos);
		if(P_Def->_curItem() == 0)
			scroll_pos = 0;
		if(!IsMultiColumn()) {
			SetScrollBarPos(scroll_pos, 1);
			if(Top != prev_top_item || to_draw)
				Draw_();
			else if(need_sel) {
				const HWND h_lb = getHandle();
				/* @v11.4.5 (противоречие в условием !IsMultiColumn() above) if(Columns.getCount()) {
					LVITEM lvi;
					lvi.mask  = LVIF_STATE;
					lvi.state = LVIS_FOCUSED | LVIS_SELECTED;
					lvi.stateMask = LVIS_FOCUSED | LVIS_SELECTED;
					::SendMessageW(h_lb, LVM_SETITEMSTATE, P_Def ? P_Def->_curItem() : 0, (LPARAM)&lvi);
				}
				else*/{
					const long cur_sel_idx = P_Def ? (P_Def->_curItem() - P_Def->_topItem()) : 0;
					::SendMessageW(h_lb, LB_SETCURSEL, cur_sel_idx, 0);
				}
			}
		}
		if(P_Def->Options & lbtFocNotify)
			MessageCommandToOwner(cmLBItemFocused);
	}
}

bool SmartListBox::SetTreeListState(bool yes)
{
	SETFLAG(State, stTreeList, yes);
	return yes;
}

void   SmartListBox::SetOwnerDrawState() { State |= stOwnerDraw; }
void   SmartListBox::SetLBLnkToUISrchState() { State |= stLBIsLinkedUISrchTextBlock; }
void   SmartListBox::SetOmitSearchByFirstChar() { State |= stOmitSearchByFirstChar; }
int    SmartListBox::HasState(long s) const { return BIN(State & s); }

void SmartListBox::SelectTreeItem()
{
	if(P_Def) {
		long   scroll_delta = 0, scroll_pos = 0;
		P_Def->getScrollData(&scroll_delta, &scroll_pos);
		if(scroll_pos != 0)
			TreeView_SelectItem(getHandle(), (HTREEITEM)scroll_pos);
	}
}

void FASTCALL SmartListBox::focusItem(long item)
{
	if(P_Def && P_Def->go(item)) {
		if(P_Def->_isTreeList())
			SelectTreeItem();
		else {
			const  HWND h_lb = getHandle();
			const long cur__item = P_Def->_curItem();
			if(IsMultiColumn()) {
				LVITEM lvi;
				lvi.mask  = LVIF_STATE;
				lvi.state = LVIS_FOCUSED | LVIS_SELECTED;
				lvi.stateMask = LVIS_FOCUSED | LVIS_SELECTED;
				::SendMessageW(h_lb, LVM_SETITEMSTATE, cur__item, (LPARAM)&lvi);
				ListView_EnsureVisible(h_lb, cur__item, 0); // @AHTOXA
			}
			else
				::SendMessageW(h_lb, LB_SETCURSEL, cur__item - P_Def->_topItem(), 0);
		}
		if(P_Def->Options & lbtFocNotify)
			MessageCommandToOwner(cmLBItemFocused);
	}
}

bool SmartListBox::Search_(const void * pPattern, CompFunc fcmp, int srchMode)
{
	bool result = false;
	if(P_Def && P_Def->search(pPattern, fcmp, srchMode)) {
		if(!(State & stTreeList)) {
			long   scroll_delta;
			long   scroll_pos;
			P_Def->getScrollData(&scroll_delta, &scroll_pos);
			SetScrollBarPos(scroll_pos, 1);
			Draw_();
		}
		else
			SelectTreeItem();
		result = true;
	}
	return result;
}
//
//
//
Helper_WordSelector::Helper_WordSelector(WordSel_ExtraBlock * pBlk, uint inputCtlId) : P_WordSel(0), P_OuterWordSelBlk(pBlk), InputCtlId(inputCtlId)
{
}

Helper_WordSelector::~Helper_WordSelector()
{
	HWND hw = P_WordSel ? P_WordSel->H() : 0;
	if(hw)
		::EndDialog(hw, cmCancel);
	ZDELETE(P_WordSel);
}

int Helper_WordSelector::IsWsVisible() const
{
	return P_WordSel ? P_WordSel->CheckVisible() : 0;
}
//
//
//
#define UISEARCHTEXTBLOCK_MAXLEN 64

UiSearchTextBlock::UiSearchTextBlock(HWND h, uint ctlId, char * pText, int firstLetter, WordSel_ExtraBlock * pBlk, int linkToList) : 
	Helper_WordSelector(pBlk, CTL_LBX_LIST), H_Wnd(h), Id(ctlId), Text(pText), PrevInputCtlProc(0), LinkToList(linkToList), FirstLetter(firstLetter), IsBnClicked(0)
{
}

UiSearchTextBlock::~UiSearchTextBlock()
{
}

/*static*/int UiSearchTextBlock::ExecDialog(HWND hWnd, uint ctlId, SString & rText, int isFirstLetter, WordSel_ExtraBlock * pBlk, int linkToList)
{
	int    r = cmCancel;
	long   id = 0;
	SString result_text;
	if(pBlk && rText.Len() >= pBlk->MinSymbCount && pBlk->SearchText(rText, &id, result_text) > 0) {
		pBlk->SetData(id, result_text);
		rText = result_text;
		r = cmOK;
	}
	else {
		char   text[512];
		rText.CopyTo(text, sizeof(text));
		UiSearchTextBlock sd(hWnd, ctlId, text, (isFirstLetter ? text[0] : 0), pBlk, linkToList);
		r = APPL->DlgBoxParam(MAKE_BUTTON_ID(0,1)-2, hWnd, reinterpret_cast<DLGPROC>(UiSearchTextBlock::DialogProc), reinterpret_cast<LPARAM>(&sd));
		rText = sd.Text;
	}
	return r;
}

/*static*/LRESULT CALLBACK UiSearchTextBlock::InputCtlProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) // @callback
{
	UiSearchTextBlock * p_slb = static_cast<UiSearchTextBlock *>(TView::GetWindowUserData(hWnd));
	switch(uMsg) {
		case WM_KEYDOWN:
			::SendMessageW(GetParent(hWnd), uMsg, wParam, lParam);
			break;
	}
	return p_slb ? CallWindowProc(p_slb->PrevInputCtlProc, hWnd, uMsg, wParam, lParam) : DefWindowProc(hWnd, uMsg, wParam, lParam);
}

/*static*/INT_PTR CALLBACK UiSearchTextBlock::DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) // @callback(DLGPROC)
{
	SString temp_buf;
	switch(uMsg) {
		case WM_INITDIALOG:
			{
				TView::SetWindowUserData(hwndDlg, reinterpret_cast<void *>(lParam));
				UiSearchTextBlock * p_slb = reinterpret_cast<UiSearchTextBlock *>(lParam);
				{
					int    is_browser = 0;
					RECT   rc_parent;
					RECT   rc_chld;
					::GetWindowRect(hwndDlg, &rc_chld);
					{
						HWND   h_parent = 0;
						SString cls_name;
						TView::SGetWindowClassName(p_slb->H_Wnd, cls_name);
						if(cls_name.Cmp(SUcSwitch(BrowserWindow::WndClsName), 0) == 0) {
							h_parent = p_slb->H_Wnd;
							is_browser = 1;
						}
						else if(cls_name.Cmp(SUcSwitch(TInputLine::WndClsName), 0) == 0)
							h_parent = p_slb->H_Wnd;
						else
							h_parent = p_slb->LinkToList ? p_slb->H_Wnd : GetWindow(hwndDlg, GW_OWNER);
						::GetWindowRect(h_parent, &rc_parent);
					}
					int    wd = rc_parent.right - rc_parent.left;
					int    ht = rc_chld.bottom - rc_chld.top;
					int    x  = rc_parent.left;
					int    y  = is_browser ? rc_parent.top : (rc_parent.top - ht);
					::MoveWindow(hwndDlg, x, y, wd, ht, 0);
					{
						HWND hw_input = GetDlgItem(hwndDlg, p_slb->InputCtlId);
						if(hw_input) {
							::GetClientRect(hwndDlg, &rc_parent);
							::GetWindowRect(hw_input, &rc_chld);
							::MoveWindow(hw_input, rc_parent.left, rc_parent.top, rc_parent.right-rc_parent.left, rc_chld.bottom-rc_chld.top, 0);
						}
					}
				}
				SendDlgItemMessage(hwndDlg, p_slb->InputCtlId, EM_SETLIMITTEXT, UISEARCHTEXTBLOCK_MAXLEN, 0);
				TView::SSetWindowText(GetDlgItem(hwndDlg, p_slb->InputCtlId), (temp_buf = p_slb->Text).Transf(CTRANSF_INNER_TO_OUTER));
				p_slb->IsBnClicked = 0;
				if(p_slb->P_OuterWordSelBlk) {
					p_slb->P_OuterWordSelBlk->H_InputDlg = hwndDlg;
					p_slb->P_WordSel = new WordSelector(p_slb->P_OuterWordSelBlk);
					HWND h_ctl = GetDlgItem(hwndDlg, p_slb->InputCtlId);
					TView::SetWindowUserData(h_ctl, p_slb);
					p_slb->PrevInputCtlProc = static_cast<WNDPROC>(TView::SetWindowProp(h_ctl, GWLP_WNDPROC, UiSearchTextBlock::InputCtlProc));
				}
			}
			return 1;
		case WM_SHOWWINDOW:
			if(wParam == 1) {
				UiSearchTextBlock * p_slb = static_cast<UiSearchTextBlock *>(TView::GetWindowUserData(hwndDlg));
				if(p_slb && p_slb->P_WordSel) {
					TView::SGetWindowText(GetDlgItem(hwndDlg, p_slb->InputCtlId), p_slb->Text);
					p_slb->Text.Transf(CTRANSF_OUTER_TO_INNER);
					p_slb->P_WordSel->Refresh(p_slb->Text);
				}
			}
			break;
		case WM_DESTROY:
			{
				UiSearchTextBlock * p_slb = static_cast<UiSearchTextBlock *>(TView::GetWindowUserData(hwndDlg));
				TView::SetWindowProp(GetDlgItem(hwndDlg, p_slb->InputCtlId), GWLP_WNDPROC, p_slb->PrevInputCtlProc);
			}
			break;
		case WM_COMMAND:
			if(HIWORD(wParam) == BN_CLICKED) {
				UiSearchTextBlock * p_slb = static_cast<UiSearchTextBlock *>(TView::GetWindowUserData(hwndDlg));
				if(p_slb) {
					TView::SGetWindowText(GetDlgItem(hwndDlg, p_slb->InputCtlId), p_slb->Text);
					p_slb->Text.Transf(CTRANSF_OUTER_TO_INNER);
					p_slb->IsBnClicked = 1;
					TView::messageCommand(p_slb->P_WordSel, cmCancel);
				}
				EndDialog(hwndDlg, (LOWORD(wParam) == IDOK) ? cmOK : cmCancel);
			}
			else if(HIWORD(wParam) == EN_SETFOCUS) {
				UiSearchTextBlock * p_slb = static_cast<UiSearchTextBlock *>(TView::GetWindowUserData(hwndDlg));
				if(!p_slb->IsWsVisible()) {
					SendDlgItemMessage(hwndDlg, p_slb->InputCtlId, WM_KEYDOWN, VK_END, 1);
					SendDlgItemMessage(hwndDlg, p_slb->InputCtlId, WM_KEYUP,   VK_END, 1);
					SendDlgItemMessage(hwndDlg, p_slb->InputCtlId, EM_SETSEL,  0xffff, 0xffff);
#if 0 // @construction {
					if(p_slb && p_slb->FirstLetter) {
						uint   n = 0;
						SString buf;
						SStringU ubuf;
						buf.CatChar(p_slb->FirstLetter).Transf(CTRANSF_INNER_TO_UTF8);
						ubuf.CopyFromUtf8Strict(buf);
						if(ubuf.Len()) {
							const uint c = ubuf.Len();
							const size_t sz = sizeof(INPUT) * c;
							INPUT * p_inp = (INPUT *)SAlloc::M(sz);
							if(p_inp) {
								memzero(p_inp, sz);
								for(uint i = 0; i < c; i++) {
									p_inp[i].type = INPUT_KEYBOARD;
									p_inp[i].ki.dwFlags = KEYEVENTF_KEYUP|KEYEVENTF_UNICODE;
									p_inp[i].ki.wScan = ((const wchar_t *)ubuf)[i];
								}
								n = ::SendInput(c, p_inp, sizeof(INPUT));
							}
						}
						p_slb->FirstLetter = 0;
					}
#endif // } 0 @construction
				}
			}
			else if(HIWORD(wParam) == EN_KILLFOCUS) {
				UiSearchTextBlock * p_slb = static_cast<UiSearchTextBlock *>(TView::GetWindowUserData(hwndDlg));
				const int send_cancel = BIN(!p_slb->IsBnClicked && !p_slb->IsWsVisible());
				if(send_cancel || p_slb->LinkToList) {
					HWND   h_parent = p_slb->LinkToList ? p_slb->H_Wnd : GetWindow(hwndDlg, GW_OWNER);
					if(send_cancel)
						EndDialog(hwndDlg, cmCancel);
					if(h_parent)
						SetFocus(h_parent);
						//::SendMessageW(h_parent, WM_SETFOCUS, 0, 0);
				}
			}
			if(HIWORD(wParam) == EN_CHANGE) {
				UiSearchTextBlock * p_slb = static_cast<UiSearchTextBlock *>(TView::GetWindowUserData(hwndDlg));
				if(p_slb->P_WordSel) {
					long   id = 0;
					SString result_text;
					TView::SGetWindowText(GetDlgItem(hwndDlg, p_slb->InputCtlId), p_slb->Text);
					p_slb->Text.Transf(CTRANSF_OUTER_TO_INNER);
					if(p_slb->P_OuterWordSelBlk && p_slb->Text.Len() >= p_slb->P_OuterWordSelBlk->MinSymbCount && p_slb->P_OuterWordSelBlk->SearchText(p_slb->Text, &id, result_text) > 0) {
						p_slb->P_OuterWordSelBlk->SetData(id, result_text);
						::SendMessageW(hwndDlg, WM_COMMAND, MAKEWPARAM(IDOK, BN_CLICKED), p_slb->InputCtlId);
					}
					else
						p_slb->P_WordSel->Refresh(p_slb->Text);
				}
			}
			return 0;
		case WM_KEYDOWN:
			{
				UiSearchTextBlock * p_slb = static_cast<UiSearchTextBlock *>(TView::GetWindowUserData(hwndDlg));
				if(p_slb->IsWsVisible()) {
					const long key = static_cast<long>(wParam);
					if(key == VK_DOWN) {
						if(p_slb->P_WordSel->Activate() > 0)
							return 0;
					}
					else if(key == VK_ESCAPE)
						::SendMessageW(hwndDlg, WM_COMMAND, MAKEWPARAM(IDCANCEL, BN_CLICKED), p_slb->InputCtlId);
					else if(key == VK_RETURN)
						::SendMessageW(hwndDlg, WM_COMMAND, MAKEWPARAM(IDOK, BN_CLICKED), p_slb->InputCtlId);
				}
			}
			break;
		default:
			return FALSE;
	}
	return 1;
}

void SmartListBox::search(char * pFirstLetter, int srchMode)
{
	char   pattern[512];
	SString srch_pattern;
	StrPool.getnz(SrchPatternPos, srch_pattern);
	if((srchMode & ~srchFlags) == srchFirst) {
		if(pFirstLetter)
			srch_pattern = pFirstLetter;
		SString preserve_srch_pattern = srch_pattern;
		int    r2 = UiSearchTextBlock::ExecDialog(getHandle(), Id, srch_pattern, BIN(pFirstLetter), P_WordSelBlk, HasState(stLBIsLinkedUISrchTextBlock));
		if(preserve_srch_pattern.Cmp(srch_pattern, 0) != 0)
			StrPool.add(srch_pattern, &SrchPatternPos);
		if(r2 == cmOK) {
			srch_pattern.CopyTo(pattern, sizeof(pattern));
			Search_(pattern, SrchFunc, (pattern[0] == '*') ? srchNext : srchFirst);
		}
	}
	else if((srchMode & ~srchFlags) == srchNext) {
		if(srch_pattern.NotEmpty()) {
			srch_pattern.CopyTo(pattern, sizeof(pattern));
			Search_(pattern, SrchFunc, srchNext);
		}
	}
	/*
	if(r >= 0)
		Draw_();
	*/
}

void FASTCALL SmartListBox::setDef(ListBoxDef * aDef)
{
	if(State & stTreeList) {
		Helper_ClearTreeWnd();
	}
	ZDELETE(P_Def);
	if(aDef) {
		P_Def = aDef;
		setRange(P_Def->GetRecsCount());
		if(P_Def->Options & lbtHSizeAlreadyDef)
			ItemHeight = P_Def->ViewHight;
		else
			P_Def->setViewHight(ItemHeight);
		P_Def->top();
	}
}

int SmartListBox::TransmitData(int dir, void * pData)
{
	int    s = 0;
	if(dir > 0) {
		if(P_Def) {
			SETFLAG(State, stDataFounded, (pData ? P_Def->TransmitData(dir, pData) : 0));
			if(!IsMultiColumn()) {
				if(!pData)
					P_Def->top();
				else if(!(State & stTreeList)) {
					long scroll_delta, scroll_pos;
					P_Def->getScrollData(&scroll_delta, &scroll_pos);
					SetScrollBarPos(scroll_pos, 1);
				}
			}
			s = 1;
		}
		else
			State &= ~stDataFounded;
	}
	else if(P_Def)
		s = P_Def->TransmitData(dir, pData);
	return s;
}

IMPL_HANDLE_EVENT(SmartListBox)
{
	if(event.isCmd(cmDraw)) {
		Implement_Draw();
		clearEvent(event); // @v11.2.4
	}
	else {
		TView::handleEvent(event); // @v11.2.4
		// @v12.2.2 {
		switch(event.what) {
			case TEvent::evCommand:
				switch(TVCMD) {
					case cmSetBounds:
						{
							const TRect * p_rc = static_cast<const TRect *>(TVINFOPTR);
							HWND h = getHandle();
							if(p_rc && h) {
								::SetWindowPos(h, 0, p_rc->a.x, p_rc->a.y, p_rc->width(), p_rc->height(), SWP_NOZORDER|SWP_NOCOPYBITS);
								// @v12.2.5 {
								if(IsMultiColumn()) {  
									CalculateColumnsWidth(auotocalccolszNominal);
								}
								else if(!IsTreeList()) {
									MoveScrollBar(1);
								}
								// } @v12.2.5 
								clearEvent(event);
							}
						}
						break;
				}
				break;
		}
		// } @v12.2.2 
	}
}

void SmartListBox::CalculateColumnsWidth(int autoCalcVariant/*auotocalccolszXXX*/)
{
	const  HWND h_lb = getHandle();
	const  uint cc = Columns.getCount();
	if(P_Def && h_lb && cc) {
		long   first_item = 0;
		SString temp_buf;
		SString cell_buf;
		StringSet ss(SLBColumnDelim);
		const long  last_item = P_Def->GetRecsCount();
		if(autoCalcVariant/*&& first_item < last_item*/) {
			LongArray column_text_size_list;
			if(autoCalcVariant == auotocalccolszNominal) {
				for(uint cidx = 0; cidx < cc; cidx++) {
					const ColumnDescr * p_hdr_item = static_cast<const ColumnDescr *>(Columns.at(cidx));
					column_text_size_list.add(NZOR(p_hdr_item->Width, 4));
				}
			}
			else {
				for(uint cidx = 0; cidx < cc; cidx++) {
					long   max_sw = 50; // minimal=4
					const ColumnDescr * p_hdr_item = static_cast<const ColumnDescr *>(Columns.at(cidx));
					StrPool.get(p_hdr_item->TitlePos, cell_buf);
					cell_buf.Transf(CTRANSF_INNER_TO_OUTER);
					int sw = ListView_GetStringWidth(h_lb, SUcSwitch(cell_buf.cptr()));
					SETMAX(max_sw, sw);
					for(long i = 0, item = first_item; i < last_item; i++, item++) {
						P_Def->getText(item, temp_buf);
						{
							ss.setBuf(temp_buf);
							cell_buf.Z();
							for(uint k = 0, pos = 0; k <= cidx; k++)
								ss.get(&pos, cell_buf);
							cell_buf.Transf(CTRANSF_INNER_TO_OUTER);
						}
						int sw = ListView_GetStringWidth(h_lb, SUcSwitch(cell_buf.cptr()));
						SETMAX(max_sw, sw);
					}
					column_text_size_list.add(max_sw);
				}
			}
			assert(column_text_size_list.getCount() == cc);
			{
				RECT view_rect;
				::GetClientRect(h_lb, &view_rect);
				const int vscroll_width = GetSystemMetrics(SM_CXVSCROLL);
				const float szy = 20.0f; // any positive value 
				SUiLayout layout;
				SUiLayoutParam alb(DIREC_HORZ, SUiLayoutParam::alignStart, 0);
				alb.SetFixedSizeX(static_cast<float>(view_rect.right - view_rect.left - vscroll_width));
				alb.SetFixedSizeY(szy);
				layout.SetLayoutBlock(alb);
				{
					for(uint cidx = 0; cidx < cc; cidx++) {
						SUiLayoutParam alb_c;
						alb_c.SetVariableSizeX(SUiLayoutParam::szUndef, 0.0f);
						alb_c.SetVariableSizeY(SUiLayoutParam::szByContainer, 1.0f);
						float _w = static_cast<float>(column_text_size_list.get(cidx));
						if(autoCalcVariant == auotocalccolszLogContent)
							_w = logf(_w);
						alb_c.GrowFactor = _w;
						SUiLayout * p_lo = layout.InsertItem();
						p_lo->SetLayoutBlock(alb_c);
					}
					layout.Evaluate(0);
					assert(layout.GetChildrenCount() == cc);
				}
				{
					for(uint cidx = 0; cidx < cc; cidx++) {
						const SUiLayout * p_lo = layout.GetChildC(cidx);
						const FRect lo_frame = p_lo->GetFrame();
						ListView_SetColumnWidth(h_lb, cidx, static_cast<int>(lo_frame.Width()));
					}
				}
			}
		}
	}
}

void SmartListBox::Implement_Draw()
{
	if(P_Def) {
		int    auto_calc_column_sizes = auotocalccolszNominal;
		if(State & stTreeList) {
			SetupTreeWnd2(0);
			SelectTreeItem();
		}
		else {
			long   i;
			long   item;
			SString buf;
			SString cell_buf;
			buf.Space().Z();
			cell_buf.Space().Z();
			const  HWND h_lb = getHandle();
			const  uint cc = Columns.getCount();
			if(cc)
				ListView_DeleteAllItems(h_lb);
			else
				::SendMessageW(h_lb, LB_RESETCONTENT, 0, 0);
			{
				if(HIML) {
					ImageList_Destroy(static_cast<HIMAGELIST>(HIML));
					HIML = 0;
				}
				HIML = P_Def->CreateImageList(TProgram::GetInst());
				if(HIML)
		 			ListView_SetImageList(h_lb, HIML, LVSIL_SMALL);
			}
			if(ItemHeight) {
				long   first_item = 0;
				long   last_item = 0;
				StringSet ss(SLBColumnDelim);
				if(cc) {
					last_item  = P_Def->GetRecsCount();
					CalculateColumnsWidth(auto_calc_column_sizes);
				}
				/*else if(P_Def->_isSolid()) { // @v11.4.5
					//first_item = P_Def->_topItem();
					//last_item  = P_Def->ViewHight;
					last_item  = P_Def->GetRecsCount();
				}*/
				else {
					first_item = P_Def->_topItem();
					last_item  = P_Def->ViewHight;
				}
				for(i = 0, item = first_item; i < last_item; i++, item++) {
					P_Def->getText(item, buf);
					buf.Transf(CTRANSF_INNER_TO_OUTER);
					if(cc) {
						long id = 0;
						long img_idx = 0;
						LVITEM    lvi;
						ss.setBuf(buf);
						lvi.mask  = LVIF_TEXT;
						lvi.iItem = i;
						if(P_Def->Options & lbtAutoID)
							id = item;
						else
							getID(item, &id);
						if(P_Def->GetImageIdxByID(id, &img_idx) > 0) {
							lvi.iImage = img_idx;
							lvi.mask |= LVIF_IMAGE;
						}
						for(uint k = 0, pos = 0; k < cc; k++) {
							ss.get(&pos, cell_buf);
							lvi.pszText  = const_cast<TCHAR *>(SUcSwitch(cell_buf.Strip().cptr())); // @badcast // @unicodeproblem
							lvi.iSubItem = k;
							if(k)
								ListView_SetItem(h_lb, &lvi);
							else
								ListView_InsertItem(h_lb, &lvi);
						}
					}
					/*else if(P_Def->_isSolid()) { // @v11.4.5
						::SendMessageW(h_lb, LB_ADDSTRING, 0, (State & stOwnerDraw) ? static_cast<LPARAM>(item) : reinterpret_cast<LPARAM>(SUcSwitch(buf.cptr())));
					}*/
					else
						::SendMessageW(h_lb, LB_ADDSTRING, 0, (State & stOwnerDraw) ? static_cast<LPARAM>(item) : reinterpret_cast<LPARAM>(SUcSwitch(buf.cptr())));
				}
			}
			::ShowWindow(h_lb, SW_NORMAL);
			::UpdateWindow(h_lb);
			if(cc) { // multi-column
				LVITEM lvi;
				lvi.mask  = LVIF_STATE;
				lvi.state = LVIS_FOCUSED | LVIS_SELECTED;
				lvi.stateMask = LVIS_FOCUSED | LVIS_SELECTED;
				::SendMessageW(h_lb, LVM_SETITEMSTATE, P_Def->_curItem(), reinterpret_cast<LPARAM>(&lvi));
				ListView_EnsureVisible(h_lb, P_Def->_curItem(), 0); // AHTOXA
			}
			else
				::SendMessageW(h_lb, LB_SETCURSEL, P_Def ? (P_Def->_curItem() - P_Def->_topItem()) : 0, 0);
		}
	}
}

void SmartListBox::selectItem(long)
{
	MessageCommandToOwner(cmLBItemSelected);
}

void SmartListBox::setRange(long aRange)
{
	Range = aRange;
	if(!IsMultiColumn())
		SendDlgItemMessage(Parent, MAKE_BUTTON_ID(Id,1), SBM_SETRANGE, 0, Range-1);
}

void SmartListBox::setState(uint aState, bool enable)
{
	// @v11.2.5 const long preserve_state = State; // @v11.2.4
	TView::setState(aState, enable);
	// @v11.2.5 if((aState & (sfSelected|sfActive)) != (preserve_state & (sfSelected|sfActive)) && !IsMultiColumn())
	if(aState & (sfSelected|sfActive) && !IsMultiColumn()) // @v11.2.5
		if(!(State & stTreeList))
			Draw_();
}

int SmartListBox::addItem(long id, const char * s, long * pPos)
{
	const int r = P_Def ? P_Def->addItem(id, s, pPos) : -1;
	if(r > 0)
		setRange(P_Def->GetRecsCount());
	return r;
}

int SmartListBox::removeItem(long pos)
{
	const int r = P_Def ? P_Def->removeItem(pos) : -1;
	if(r > 0)
		setRange(P_Def->GetRecsCount());
	return r;
}

void SmartListBox::freeAll()
{
	if(P_Def) {
		P_Def->freeAll();
		setRange(P_Def->GetRecsCount());
	}
}
//
//
//
ListWindowSmartListBox::ListWindowSmartListBox(const TRect & r, ListBoxDef * d, int) : SmartListBox(r, d, false), combo(0) {}
//
//
//
WordSelectorSmartListBox::WordSelectorSmartListBox(const TRect & bounds, ListBoxDef * aDef) : ListWindowSmartListBox(bounds, aDef, 0)
{
}

int WordSelectorSmartListBox::handleWindowsMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg) {
		case WM_INITDIALOG: onInitDialog(0); return 1;
		case WM_CHAR: return 0;
		case WM_MOUSEWHEEL:
		case WM_VSCROLL:
		case WM_VKEYTOITEM:
			{
				short code = -1;
				if(uMsg == WM_MOUSEWHEEL) {
					short delta = static_cast<short>(HIWORD(wParam));
					code = (delta > 0) ? SB_LINEUP : SB_LINEDOWN;
				}
				else if(uMsg == WM_VKEYTOITEM) {
					switch(LOWORD(wParam)) {
						case VK_UP:    code = SB_LINEUP; break;
						case VK_PRIOR: code = SB_PAGEUP; break;
						case VK_HOME:  code = SB_TOP; break;
					}
				}
				else
					code  = LOWORD(wParam);
				if(P_Def->_curItem() == 0 && oneof3(code, SB_LINEUP, SB_PAGEUP, SB_TOP)) {
					HWND parent = GetParent(getHandle());
					WordSelector * p_selector = static_cast<WordSelector *>(TView::GetWindowUserData(parent));
					TView::messageCommand(p_selector, cmUp);
				}
			}
			break;
	}
	return SmartListBox::handleWindowsMessage(uMsg, wParam, lParam);
}
