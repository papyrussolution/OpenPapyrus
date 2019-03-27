// SMRTLBX.CPP
// Copyright (c) Sobolev A. 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2015, 2016, 2017, 2018, 2019
// @codepage UTF-8
// Release for WIN32
//
#include <slib.h>
#include <tv.h>
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
		size_t p_len = sstrlen(p);
		size_t c_len = sstrlen(c);
		int    r = strnicmp866(p, c, MIN(p_len, c_len));
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
	SPathStruc::NormalizePath(p1, SPathStruc::npfKeepCase, r_s1);
	r_su1.CopyFromUtf8Strict(r_s1, r_s1.Len());
	r_su1.ToLower();
	SPathStruc::NormalizePath(p2, SPathStruc::npfKeepCase, r_s2);
	r_su2.CopyFromUtf8Strict(r_s2, r_s2.Len());
	r_su2.ToLower();
	return r_su1.Cmp(r_su2);
}
//
//
//
INT_PTR CALLBACK ListBoxDialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) // DLGPROC
{
	SmartListBox * p_view = static_cast<SmartListBox *>(TView::GetWindowUserData(hWnd));
	if(p_view) {
		switch(uMsg) {
			case WM_DESTROY: // Не вызывается. Непонятно, правда, почему.
				CALLPTRMEMB(p_view, OnDestroy(hWnd));
				return 0;
			case WM_KEYDOWN:
			case WM_SYSKEYDOWN:
				::SendMessage(GetParent(hWnd), WM_VKEYTOITEM, MAKEWPARAM((WORD)wParam, 0), reinterpret_cast<LPARAM>(hWnd));
				return 0;
			case WM_SETFOCUS:
			case WM_KILLFOCUS:
				::SendMessage(GetParent(hWnd), uMsg, wParam, reinterpret_cast<LPARAM>(hWnd));
				break;
			case WM_CHAR:
				::SendMessage(GetParent(hWnd), uMsg, wParam, reinterpret_cast<LPARAM>(hWnd));
				return 0;
			case WM_MOUSEWHEEL:
				::SendMessage(GetParent(hWnd), uMsg, wParam, reinterpret_cast<LPARAM>(hWnd));
				return 0;
				/*
				{
					short delta = (short)HIWORD(wParam);
					int   scroll_code = (delta > 0) ? SB_LINEUP : SB_LINEDOWN;
					for(int i = 0; i < 3; i++)
						p_view->Scroll(scroll_code, 0);
				}
				return 0;
				*/
			case WM_RBUTTONDOWN:
				::SendMessage(GetParent(hWnd), uMsg, MAKEWPARAM(LOWORD(wParam), 1), lParam);
				return 0;
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

INT_PTR CALLBACK TreeListBoxDialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) // DLGPROC
{
	SmartListBox * p_view = static_cast<SmartListBox *>(TView::GetWindowUserData(hWnd));
	const WNDPROC prev_wnd_proc = p_view ? p_view->PrevWindowProc : 0;
	switch(uMsg) {
		case WM_DESTROY: CALLPTRMEMB(p_view, OnDestroy(hWnd)); return 0; // Не вызывается. Непонятно правда почему.
		case WM_KEYDOWN:
		case WM_SYSKEYDOWN: ::SendMessage(GetParent(hWnd), WM_VKEYTOITEM, MAKEWPARAM((WORD)wParam, 0), reinterpret_cast<LPARAM>(hWnd)); break; // Process by default
		case WM_SETFOCUS:
		case WM_KILLFOCUS: ::SendMessage(GetParent(hWnd), uMsg, wParam, reinterpret_cast<LPARAM>(hWnd)); break;
		case WM_CHAR: ::SendMessage(GetParent(hWnd), uMsg, wParam, reinterpret_cast<LPARAM>(hWnd)); return 0;
		case WM_NOTIFY: ::SendMessage(GetParent(hWnd), uMsg, wParam, lParam); return 0;
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
	//return p_view ? CallWindowProc(p_view->PrevWindowProc, hWnd, uMsg, wParam, lParam) : 1;
	return prev_wnd_proc ? CallWindowProc(prev_wnd_proc, hWnd, uMsg, wParam, lParam) : 1;
}

INT_PTR CALLBACK ListViewDialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) // DLGPROC
{
	SmartListBox * p_view = static_cast<SmartListBox *>(TView::GetWindowUserData(hWnd));
	switch(uMsg) {
		case WM_DESTROY: // Не вызывается. Непонятно правда почему.
			CALLPTRMEMB(p_view, OnDestroy(hWnd));
			return 0;
		case WM_KEYDOWN:
		case WM_SYSKEYDOWN:
			::SendMessage(GetParent(hWnd), WM_VKEYTOITEM, MAKEWPARAM((WORD)wParam, 0), reinterpret_cast<LPARAM>(hWnd));
			break; // Process by default
		case WM_SETFOCUS:
		case WM_KILLFOCUS:
			::SendMessage(GetParent(hWnd), uMsg, wParam, reinterpret_cast<LPARAM>(hWnd));
			break;
		case WM_CHAR:
			::SendMessage(GetParent(hWnd), uMsg, wParam, reinterpret_cast<LPARAM>(hWnd));
			return 0;
		case WM_LBUTTONDOWN:
			::PostMessage(GetParent(hWnd), WM_COMMAND, MAKEWPARAM((WORD)GetDlgCtrlID(hWnd), (WORD)LBN_SELCHANGE), reinterpret_cast<LPARAM>(hWnd));
			break;
		case WM_LBUTTONDBLCLK:
			::SendMessage(GetParent(hWnd), WM_COMMAND, MAKEWPARAM((WORD)GetDlgCtrlID(hWnd), (WORD)LBN_DBLCLK), reinterpret_cast<LPARAM>(hWnd));
			return 0;
		case WM_RBUTTONDOWN:
			CALLPTRMEMB(p_view, handleWindowsMessage(uMsg, wParam, lParam));
			// SendMessage(GetParent(hWnd), uMsg, MAKEWPARAM(LOWORD(wParam), 1), lParam);
			break;
		/*
		case WM_INPUTLANGCHANGE: // @v6.4.4 AHTOXA
			PostMessage(GetParent(hWnd), uMsg, wParam, lParam);
			break;
		*/
	}
	return p_view ? CallWindowProc(p_view->PrevWindowProc, hWnd, uMsg, wParam, lParam) : 1;
}
//
//
//
SmartListBox::SmartListBox(const TRect & bounds, ListBoxDef * aDef, int isTreeList) : TView(bounds), Columns(sizeof(ColumnDescr)),
	State(0), SrchPatternPos(0), ColumnsSpcPos(0), def(0), Range(0), Height(0), Top(0), HIML(0), SrchFunc(PTR_CMPFUNC(_PcharNoCase))
{
	SubSign = TV_SUBSIGN_LISTBOX;
	StrPool.add("$"); // zero index - is empty string
	SetTreeListState(isTreeList);
	ViewOptions |= ofSelectable | ofFirstClick;
	setDef(aDef);
}

SmartListBox::~SmartListBox()
{
	RestoreOnDestruction();
	delete def;
	if(HIML)
		ImageList_Destroy(HIML);
}

int SmartListBox::SearchColumnByIdent(long ident, uint * pPos) const
{
	int    ok = 0;
	uint   pos = 0;
	for(uint i = 0; !ok && i < Columns.getCount(); i++) {
		const ColumnDescr * p_item = (const ColumnDescr *)Columns.at(i);
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
	ColumnDescr item;
	MEMSZERO(item);
	SString temp_buf;
	SString title = pTitle;
	if(!title.NotEmptyS()) {
		title.Space();
	}
	uint cf = CHKXORFLAGS(format, STRF_ANSI, STRF_OEM);
	if(title.C(0) == '@') {
		if(cf == STRF_ANSI) {
			title.Transf(CTRANSF_OUTER_TO_INNER);
			cf = STRF_OEM;
		}
		if(SLS.LoadString(title.ShiftLeft(), temp_buf) > 0) {
			title = temp_buf.Strip();
			cf = STRF_OEM;
		}
	}
	if(cf == STRF_OEM || cf == 0)
		title.Transf(CTRANSF_INNER_TO_OUTER);
	SETIFZ(width, title.Len());
	item.Width = MIN(255, MAX(width, 2));
	if(format & ALIGN_LEFT)
		item.Format |= ALIGN_LEFT;
	else if(format & ALIGN_RIGHT)
		item.Format |= ALIGN_RIGHT;
	else if(format & ALIGN_CENTER)
		item.Format |= ALIGN_CENTER;
	else
		item.Format |= ALIGN_LEFT;
	item.Ident = ident; // @v9.2.9
	if(title.Len())
		StrPool.add(title, &item.TitlePos);
	if(pos < 0 || pos >= (int)Columns.getCount())
		pos = Columns.getCount();
	ok = Columns.atInsert((uint)pos, &item);
	if(ok /* @v9.2.9 && !(State & stInited)*/)
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

int SmartListBox::GetOrgColumnsDescr(SString & rBuf) const
{
	return StrPool.getnz(ColumnsSpcPos, rBuf.Z());
}

int SmartListBox::SetupColumns(const char * pColsBuf)
{
	RemoveColumns();
	SString columns_buf = pColsBuf;
	if(columns_buf.NotEmptyS()) {
		SString cstr, citem, left, right, title_buf;
		if(columns_buf.Strip().C(0) == '@' && SLS.LoadString(columns_buf.ShiftLeft(), cstr) > 0)
			columns_buf = cstr;
		columns_buf.Transf(CTRANSF_INNER_TO_OUTER);
		StringSet columns_ss(';', columns_buf);
		StringSet ss(",");
		for(uint columns_pos = 0; columns_ss.get(&columns_pos, cstr);) {
			uint   pos = 0;
			ss.setBuf(cstr);
			uint   width = 0;
			uint   format = 0;
			title_buf.Z();
			if(ss.get(&pos, citem)) {
				if(citem.Divide('w', left, right) > 0)
					width = static_cast<uint16>(right.ToLong());
				else
					width = static_cast<uint16>(citem.ToLong());
				if(ss.get(&pos, citem)) {
					int a = toupper(citem.Strip().C(0));
					if(a == 'L')      format |= ALIGN_LEFT;
					else if(a == 'R') format |= ALIGN_RIGHT;
					else if(a == 'C') format |= ALIGN_CENTER;
					else              format |= ALIGN_LEFT;
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

int    FASTCALL SmartListBox::getCurID(long * pId) { return def ? def->getCurID(pId) : 0; }
int    FASTCALL SmartListBox::getCurData(void * pData) { return def ? def->getCurData(pData) : 0; }
int    FASTCALL SmartListBox::getCurString(SString & rBuf) { return def ? def->getCurString(rBuf) : (rBuf.Z(), 0); }
int    SmartListBox::isTreeList() const { return BIN(def && def->_isTreeList()); }
void   SmartListBox::setHorzRange(int) {}

int SmartListBox::getText(long itemN  /* 0.. */, SString & rBuf)
{
	int    ok = 0;
	rBuf.Z();
	if(def) {
		def->getText(itemN, rBuf);
		ok = 1;
	}
	return ok;
}

int SmartListBox::getID(long itemN, long * pID)
{
	int    ok = 0;
	if(def) {
		const char * p = static_cast<const char *>(def->getRow_(itemN));
		if(p) {
			uint   h = (def->Options & lbtAutoID) ? 0 : ((def->Options & lbtWordID) ? 2 : 4);
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
		lv.pszText = const_cast<TCHAR *>(SUcSwitch(title_buf.cptr())); // @badcast // @unicodeproblem
		if(slbc.Format & ALIGN_LEFT)
			lv.fmt = LVCFMT_LEFT;
		else if(slbc.Format & ALIGN_RIGHT)
			lv.fmt = LVCFMT_RIGHT;
		else if(slbc.Format & ALIGN_CENTER)
			lv.fmt = LVCFMT_CENTER;
		else
			lv.fmt = LVCFMT_LEFT;
		ListView_InsertColumn(getHandle(), pos, &lv); // @unicodeproblem
	}
}

void SmartListBox::MoveScrollBar(int autoHeight)
{
	long   scroll_delta = 0, scroll_pos = 0;
	CreateScrollBar(1);
	if(def) {
		if(autoHeight) {
			def->SetOption(lbtHSizeAlreadyDef, 0);
			def->setViewHight(GetMaxListHeight());
		}
		def->getScrollData(&scroll_delta, &scroll_pos);
		if(def->_curItem() == 0)
			scroll_pos = 0;
		setRange(def->getRecsCount());
	}
	SetScrollBarPos(scroll_pos, 1);
	Draw_();
}

void SmartListBox::CreateScrollBar(int create)
{
	HWND h_wnd = GetDlgItem(Parent, MAKE_BUTTON_ID(Id, 1));
	if(h_wnd)
		DestroyWindow(h_wnd);
	if(create) {
		int    tabbed = TView::GetWindowStyle(Parent) & WS_CHILD;
		const  HWND h_lb = getHandle();
		RECT   rc_list;
		::GetWindowRect(h_lb, &rc_list);
		int   sc_width = GetSystemMetrics(SM_CXVSCROLL);
		int   sc_height = rc_list.bottom - rc_list.top;
		POINT sc_lu;
		sc_lu.x = rc_list.right;
		sc_lu.y = rc_list.top;
		::MapWindowPoints(NULL, Parent, &sc_lu, 1);
		h_wnd = ::CreateWindow(_T("SCROLLBAR"), _T(""), WS_CHILD|SBS_LEFTALIGN|SBS_VERT, sc_lu.x, sc_lu.y, sc_width, sc_height, Parent, 
			reinterpret_cast<HMENU>(MAKE_BUTTON_ID(Id, 1)), TProgram::GetInst(), 0); // @unicodeproblem
		::ShowWindow(h_wnd, SW_SHOWNORMAL);
	}
}

void SmartListBox::SetScrollBarPos(long pos, LPARAM lParam)
{
	HWND h_wnd = GetDlgItem(Parent, MAKE_BUTTON_ID(Id, 1));
	if(h_wnd)
		::SendMessage(h_wnd, SBM_SETPOS, pos, lParam);
}

int SmartListBox::GetMaxListHeight()
{
	int    item_height = 0, max_height = 0;
	int    list_height = 0;
	const  HWND h_lb = getHandle();
	RECT   list_rect;
	::GetClientRect(Parent, &list_rect);
	list_height = list_rect.bottom - list_rect.top;
	item_height = ::SendMessage(h_lb, LB_GETITEMHEIGHT, 0, 0);
	return (def && def->Options & lbtHSizeAlreadyDef) ? def->ViewHight : (item_height ? (list_height-5) / item_height : 0);
}

void SmartListBox::onInitDialog(int useScrollBar)
{
	const  HWND h_lb = getHandle();
	DLGPROC dlg_proc = 0;
	if(State & stTreeList) {
		//StdTreeListBoxDef * p_def = (StdTreeListBoxDef*)def;
		dlg_proc = TreeListBoxDialogProc;
	}
	else {
		RECT   rc_cli_parent;
		RECT   rc_list;
		RECT   rc_parent;
		const int is_multi_col = BIN(Columns.getCount());
		const int is_tabbed = BIN(TView::GetWindowStyle(Parent) & WS_CHILD);
		::GetClientRect(Parent, &rc_cli_parent);
		if(!is_multi_col && !GetDlgItem(Parent, MAKE_BUTTON_ID(Id, 1))) {
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
		if(def)
			setRange(def->getRecsCount());
		//
		::GetWindowRect(h_lb, &rc_list);
		int    item_height;
		int    list_height = (rc_list.bottom - rc_list.top);
		if(is_multi_col) {
			RECT   rc;
			HWND   hwh = ListView_GetHeader(h_lb);
			::GetWindowRect(hwh, &rc);
			item_height = rc.bottom - rc.top - 4;
			for(uint i = 0; i < Columns.getCount(); i++)
				Helper_InsertColumn(i);
			Height = ((list_height-3) / item_height) - 1;
			ListView_SetExtendedListViewStyle(h_lb, LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES);
		}
		else {
			Height = GetMaxListHeight();
			/*
			item_height = SendMessage(h_lb, LB_GETITEMHEIGHT, 0, 0);
			Height = (def && def->Options & lbtHSizeAlreadyDef) ? def->ViewHight : (item_height ? (list_height-5) / item_height : 0);
			*/
			if(def) {
				long   scroll_delta, scroll_pos;
				Top = def->_topItem();
				def->getScrollData(&scroll_delta, &scroll_pos);
				if(def->_curItem() == 0)
					scroll_pos = 0;
				SetScrollBarPos(scroll_pos, 1);
			}
		}
		CALLPTRMEMB(def, setViewHight(Height));
		dlg_proc = is_multi_col ? ListViewDialogProc : ListBoxDialogProc;
	}
	State |= stInited;
	Draw_();
	PrevWindowProc = static_cast<WNDPROC>(TView::SetWindowProp(h_lb, GWLP_WNDPROC, dlg_proc));
	TView::SetWindowUserData(h_lb, this);
}

#if 0 // {

int SmartListBox::SetupTreeWnd(HTREEITEM hParent, long grpParentID)
{
	int    ok = -1;
	HWND   h_lb = getHandle();
	PROFILE_START
	if(def && h_lb) {
		long   save_pos = 0;
		StdTreeListBoxDef * p_def = (StdTreeListBoxDef *)def;
		if(hParent == 0) {
			PROFILE_START
			save_pos = p_def->_curItem();
			if(p_def->Assoc.getCount()) {
				LAssoc * p_assoc_item = 0;
				p_def->top();
				SelectTreeItem();
				LongArray child_list;
				for(uint i = 1; p_def->Assoc.enumItems(&i, (void **)&p_assoc_item) > 0;) {
					child_list.clear();
					p_def->GetListByParent(p_assoc_item->Key, child_list);
					for(uint k = 0; k < child_list.getCount(); k++) {
						const  long item_id = child_list.get(k);
						uint   j = i;
						if(p_def->Assoc.lsearch(&item_id, &j, CMPF_LONG))
							if(TreeView_GetParent(h_lb, (HTREEITEM)p_def->Assoc.at(j).Val) == (HTREEITEM)p_assoc_item->Val)
								p_def->Assoc.atFree(j);
					}
					TreeView_DeleteItem(h_lb, (HTREEITEM)p_assoc_item->Val);
				}
				TreeView_DeleteItem(h_lb, (HTREEITEM)p_def->Assoc.at(0).Val);
				p_def->Assoc.freeAll();
			}
			PROFILE_END
			//
			// добавляем список картинок
			//
			PROFILE_START
			{
				if(HIML)
					ImageList_Destroy(HIML);
				HIML = p_def->CreateImageList(TProgram::GetInst());
				if(HIML)
			   		::SendMessage(static_cast<HWND>(h_lb), (UINT)TVM_SETIMAGELIST, (WPARAM)TVSIL_NORMAL, (LPARAM)HIML);
			}
			PROFILE_END
		}
		{
			LongArray items_list;
			PROFILE_START
			if(p_def->GetListByParent(grpParentID, items_list) > 0) {
				SString err_msg;
				for(uint i = 0; i < items_list.getCount(); i++) {
					const long item_id = items_list.get(i);
					TVINSERTSTRUCT is;
					is.hParent      = NZOR(hParent, TVI_ROOT);
					is.hInsertAfter = TVI_LAST;
					is.item.mask    = TVIF_TEXT | TVIF_PARAM | TVIF_CHILDREN;
					if(p_def->GetImageIdxByID(item_id, 0) > 0) {
						is.item.iImage = I_IMAGECALLBACK;
						is.item.iSelectedImage = I_IMAGECALLBACK;
						is.item.mask |= (TVIF_IMAGE|TVIF_SELECTEDIMAGE);
					}
					const  int has_child = p_def->HasChild(item_id);
					is.item.cChildren = has_child ? 1 : 0;
					is.item.pszText = LPSTR_TEXTCALLBACK;
					is.item.lParam  = item_id;
					HTREEITEM h_tree = TreeView_InsertItem(h_lb, &is);
					if(h_tree) {
						p_def->Assoc.Add(item_id, (long)h_tree, 0);
						if(has_child && !(grpParentID == 0 && item_id == 0))
							SetupTreeWnd(h_tree, item_id); // @recursion
					}
					else {
						char temp_buf[256];
						FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(),
							MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), temp_buf, sizeof(temp_buf), 0);
						(err_msg = temp_buf).Chomp();
					}
				}
				ok = 1;
			}
			PROFILE_END
		}
		if(hParent == 0) {
			PROFILE(def->go(save_pos));
		}
	}
	PROFILE_END
	return ok;
}

#endif // } 0

void SmartListBox::Helper_ClearTreeWnd()
{
	HWND   h_lb = getHandle();
	if(h_lb)
		TreeView_DeleteAllItems(h_lb);
}

int SmartListBox::SetupTreeWnd2(uint32 parentP)
{
	int    ok = -1;
	HWND   h_lb = getHandle();
	if(def && h_lb) {
		HTREEITEM h_parent = 0;
		long   save_pos = 0;
		StdTreeListBoxDef * p_def = static_cast<StdTreeListBoxDef *>(def);
		if(parentP == 0) {
			save_pos = p_def->_curItem();
			Helper_ClearTreeWnd();
			//
			// добавляем список картинок
			//
			{
				if(HIML)
					ImageList_Destroy(HIML);
				HIML = p_def->CreateImageList(TProgram::GetInst());
				if(HIML)
					::SendMessage(static_cast<HWND>(h_lb), (UINT)TVM_SETIMAGELIST, static_cast<WPARAM>(TVSIL_NORMAL), reinterpret_cast<LPARAM>(HIML));
			}
		}
		else {
			const StdTreeListBoxDef::TreeItem * p_item = static_cast<const StdTreeListBoxDef::TreeItem *>(p_def->T.GetData(parentP));
			if(p_item)
				h_parent = p_item->H;
		}
		{
			SString err_msg;
			for(STree::Iter t_iter(parentP); p_def->T.Enum(t_iter);) {
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
						if(first_child_p && !(parentP == 0 && p_item->Id == 0))
							SetupTreeWnd2(t_iter.GetCurrentPos()); // @recursion
					}
					else {
						//
						// @? Здесь программа получает сообщение от системы, которое нигде ни используется!
						//
						//TCHAR temp_buf[256];
						//::FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), temp_buf, SIZEOFARRAY(temp_buf), 0);
						//(err_msg = SUcSwitch(temp_buf)).Chomp();
						// @v10.3.11 {
						SSystem::SFormatMessage(err_msg); 
						err_msg.Chomp();
						// } @v10.3.11
					}
					ok = 1;
				}
			}
		}
		if(parentP == 0) {
			def->go(save_pos);
		}
	}
	return ok;
}

int SmartListBox::GetStringByID(long id, SString & rBuf)
{
	rBuf.Z();
	return def ? static_cast<StdTreeListBoxDef *>(def)->GetStringByID(id, rBuf) : 0;
}

int SmartListBox::GetImageIdxByID(long id, long * pIdx) { return def ? static_cast<StdListBoxDef *>(def)->GetImageIdxByID(id, pIdx) : 0; }

int FASTCALL SmartListBox::onVKeyToItem(WPARAM wParam)
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
}

int SmartListBox::handleWindowsMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg) {
		case WM_INITDIALOG:
			onInitDialog(1);
			break;
		case WM_DESTROY: // данная команда практически никогда не вызывается. См. WM_DESTROY в DialogProc.
			// Из-за этого, если список картинок валиден, то на каждом вызове диалога с таким списком теряется 4 объекта GDI.
			/*{ // @v6.0.14 AHTOXA
				HIMAGELIST himl = (HIMAGELIST)SendMessage(GetDlgItem(Parent, Id), (UINT)TVM_SETIMAGELIST, (WPARAM)TVSIL_NORMAL, (LPARAM)0);
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
					{
						const int index = Columns.getCount() ?
							SendDlgItemMessage(Parent, Id, LVM_GETNEXTITEM, -1, MAKELPARAM(LVNI_SELECTED, 0)) :
							(SendDlgItemMessage(Parent, Id, LB_GETCURSEL, 0, 0) + def->_topItem());
						if(index >= 0) {
							long   prev_top_item = Top;
							def->go(index);
							if(!Columns.getCount() && Top != prev_top_item)
								Draw_();
							if(def->Options & lbtFocNotify)
								MessageCommandToOwner(cmLBItemFocused);
							Scroll(-1, 0);
							if(def->Options & lbtSelNotify)
								MessageCommandToOwner(cmLBItemSelected);
						}
					}
					break;
			}
			break;
		}
		case WM_RBUTTONDOWN:
			{
				const int is_multi_col = BIN(Columns.getCount());
				if(!is_multi_col) {
					const long hw_clarea_lw_index = SendDlgItemMessage(Parent, Id, LB_ITEMFROMPOINT, 0, lParam);
					if(HIWORD(hw_clarea_lw_index) == 0 && LOWORD(hw_clarea_lw_index) >= 0) {
						long   prev_top_item = Top;
						def->go(LOWORD(hw_clarea_lw_index) + def->_topItem());
						if(!Columns.getCount() && Top != prev_top_item)
							Draw_();
						if(def->Options & lbtFocNotify)
							MessageCommandToOwner(cmLBItemFocused);
						Scroll(-1, 0);
					}
				}
				MessageCommandToOwner(cmRightClick);
			}
			break;
		case WM_VKEYTOITEM:
			{
				int    r = onVKeyToItem(wParam);
				if(r >= 0)
					return r;
			}
			break;
		case WM_CHAR:
			if(wParam == kbCtrlF)
				search(0, srchFirst);
			else if(wParam == kbCtrlG)
				search(0, srchNext);
			else if(!(State & stOmitSearchByFirstChar)) {
				char b[2];
				b[0] = static_cast<char>(wParam);
				b[1] = 0;
				SCharToOem(b);
				uchar ub = b[0];
				if(isalnum(ub) || IsLetter866(ub) || ub == '*')
					search(b, srchFirst);
				else
					return 0;
			}
			// @v10.0.12 {
			else {
				char b[2];
				b[0] = static_cast<char>(wParam);
				b[1] = 0;
				SCharToOem(b);
				uchar ub = b[0];
				if(isalnum(ub) || IsLetter866(ub))
					return 0;
				else
					return 1;
			}
			// } @v10.0.12
			/* @v10.0.12
			else
				return 1; // @v9.8.12 0-->1
			*/
			break;
		case WM_MOUSEWHEEL:
			{
				short delta = static_cast<short>(HIWORD(wParam));
				int   scroll_code = (delta > 0) ? SB_LINEUP : SB_LINEDOWN;
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
								// @v10.3.10 r_temp_buf.Transf(CTRANSF_INNER_TO_OUTER).CopyTo(lptvdi->item.pszText, 0); // @unicodeproblem
								r_temp_buf.Transf(CTRANSF_INNER_TO_OUTER); // @v10.3.10 
								strnzcpy(lptvdi->item.pszText, SUcSwitch(r_temp_buf), lptvdi->item.cchTextMax); // @v10.3.10 
							}
							if(lptvdi->item.mask & (TVIF_IMAGE|TVIF_SELECTEDIMAGE)) {
								long idx = 0;
								lptvdi->item.iImage         = -1;
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
							static_cast<StdTreeListBoxDef *>(def)->GoByID(pnmtv->itemNew.lParam);
							if(def->Options & lbtFocNotify)
								MessageCommandToOwner(cmLBItemFocused);
							if(State & stLButtonDown) {
								if(def->Options & lbtSelNotify)
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
							static_cast<StdTreeListBoxDef *>(def)->GoByID(t_item.lParam);
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
					HWND   h_ctl = p_nm->hwndFrom;
					long   result = CDRF_DODEFAULT;
					if(!p_cd)
						result = -1;
					else {
						switch(p_cd->nmcd.dwDrawStage) {
	    					case CDDS_PREPAINT:
								if(def && def->HasItemColorSpec())
									result = CDRF_NOTIFYITEMDRAW;
								break;
							case CDDS_ITEMPREPAINT:
								{
									long   _top = 0; //def->_topItem();
									long   item_pos = _top + (long)p_cd->nmcd.dwItemSpec;
									long   item_id = 0;
									if((p_cd->nmcd.uItemState&CDIS_FOCUS) != CDIS_FOCUS) {
										if(getID(item_pos, &item_id)) {
											SColor bc, fc;
											if(def->GetItemColor(item_id, &fc, &bc) > 0) {
												p_cd->clrText = (COLORREF)fc;
												p_cd->clrTextBk = (COLORREF)bc;
											}
										}
									}
									else {
										// @v10.2.12 @fix p_cd->clrText = p_cd->clrText;
										// @v10.2.12 @fix p_cd->clrTextBk = p_cd->clrTextBk;
									}
								}
								break;
							}
					}
					//SetWindowLong(Parent, DWL_MSGRESULT, result);
					TView::SetWindowProp(Parent, DWLP_MSGRESULT, result);
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
	if(def) {
		long   scroll_delta, scroll_pos, prev_top_item = Top;
		int    to_draw = 0, need_sel = 1;
		int    fsize  = def->GetFrameSize() - 1;
		int    fstate = def->GetFrameState();
		switch(Code) {
			case SB_BOTTOM:
				if(fstate == -1)
					fstate = 2;
				if(fstate == 2 && def->_curItem() >= fsize)
					need_sel = 0;
				else {
					def->bottom();
					to_draw = 1;
					Top = def->_curItem() - def->ViewHight + 1;
				}
				break;
			case SB_LINEDOWN:
				if(fstate == -1)
					fstate = 2;
				if(fstate == 2 && def->_curItem() >= fsize)
					need_sel = 0;
				else {
					def->step(1);
					if((def->_curItem() - Top) >= def->ViewHight)
						Top++;
					to_draw = (def->_curItem() < def->ViewHight) ? 0 : 1;
				}
				break;
			case SB_LINEUP:
				if(fstate == -1)
					fstate = 1;
				if(fstate == 1 && def->_curItem() == 0)
					need_sel = 0;
				else {
					def->step(-1);
					to_draw = Top ? 0 : 1;
					if(Top > def->_curItem())
						Top--;
					// @vmiller {
					else if((def->_curItem() - def->ViewHight) >= Top)
						Top = def->_curItem();
					// } @vmiller
				}
				break;
			case SB_PAGEDOWN:
				if(fstate == -1)
					fstate = 2;
				if(fstate == 2 && def->_curItem() >= fsize)
					need_sel = 0;
				else {
					def->step(def->ViewHight);
					to_draw = 1;
					Top = def->_curItem() - def->ViewHight + 1;
				}
				break;
			case SB_PAGEUP:
				if(fstate == -1)
					fstate = 1;
				if(fstate == 1 && def->_curItem() == 0)
					need_sel = 0;
				else {
					def->step(-def->ViewHight);
					to_draw = 1;
					Top = def->_curItem();
				}
				break;
			case SB_TOP:
				if(fstate == -1)
					fstate = 1;
				if(fstate == 1 && def->_curItem() == 0)
					need_sel = 0;
				else {
					def->top();
					to_draw = 1;
					Top = 0;
				}
				break;
			case SB_THUMBPOSITION:
			case SB_THUMBTRACK:
				if(def->HasCapability(ListBoxDef::cCountable)) {
					if(value < 0)
						def->top();
					else if(value >= def->getRecsCount())
						def->bottom();
					else
						def->go(value);
					to_draw = 1;
				}
				break;
			default:
				break;
		}
		def->getScrollData(&scroll_delta, &scroll_pos);
		if(def->_curItem() == 0)
			scroll_pos = 0;
		if(!Columns.getCount()) {
			SetScrollBarPos(scroll_pos, 1);
			if(Top != prev_top_item || to_draw)
				Draw_();
			else if(need_sel) {
				const HWND h_lb = getHandle();
				if(Columns.getCount()) {
					LVITEM lvi;
					lvi.mask  = LVIF_STATE;
					lvi.state = LVIS_FOCUSED | LVIS_SELECTED;
					lvi.stateMask = LVIS_FOCUSED | LVIS_SELECTED;
					::SendMessage(h_lb, LVM_SETITEMSTATE, def ? def->_curItem() : 0, (LPARAM)&lvi);
				}
				else {
					long   cur_sel_idx = def ? (def->_curItem()-def->_topItem()) : 0;
					::SendMessage(h_lb, LB_SETCURSEL, cur_sel_idx, 0);
				}
			}
		}
		if(def->Options & lbtFocNotify)
			MessageCommandToOwner(cmLBItemFocused);
	}
}

int SmartListBox::SetTreeListState(int yes)
{
	SETFLAG(State, stTreeList, yes);
	return BIN(yes);
}

void   SmartListBox::SetOwnerDrawState() { State |= stOwnerDraw; }
void   SmartListBox::SetLBLnkToUISrchState() { State |= stLBIsLinkedUISrchTextBlock; }
void   SmartListBox::SetOmitSearchByFirstChar() { State |= stOmitSearchByFirstChar; }
int    SmartListBox::HasState(long s) const { return BIN(State & s); }

void SmartListBox::SelectTreeItem()
{
	if(def) {
		long   scroll_delta = 0, scroll_pos = 0;
		def->getScrollData(&scroll_delta, &scroll_pos);
		if(scroll_pos != 0)
			TreeView_SelectItem(getHandle(), (HTREEITEM)scroll_pos);
	}
}

void FASTCALL SmartListBox::focusItem(long item)
{
	if(def && def->go(item)) {
		if(def->_isTreeList())
			SelectTreeItem();
		else {
			const  HWND h_lb = getHandle();
			const long cur__item = def->_curItem();
			if(Columns.getCount()) {
				LVITEM lvi;
				lvi.mask  = LVIF_STATE;
				lvi.state = LVIS_FOCUSED | LVIS_SELECTED;
				lvi.stateMask = LVIS_FOCUSED | LVIS_SELECTED;
				::SendMessage(h_lb, LVM_SETITEMSTATE, cur__item, (LPARAM)&lvi);
				ListView_EnsureVisible(h_lb, cur__item, 0); // AHTOXA
			}
			else
				::SendMessage(h_lb, LB_SETCURSEL, cur__item-def->_topItem(), 0);
		}
		if(def->Options & lbtFocNotify)
			MessageCommandToOwner(cmLBItemFocused);
	}
}

int SmartListBox::search(const void * pPattern, CompFunc fcmp, int srchMode)
{
	if(def && def->search(pPattern, fcmp, srchMode)) {
		if(!(State & stTreeList)) {
			long   scroll_delta, scroll_pos;
			def->getScrollData(&scroll_delta, &scroll_pos);
			SetScrollBarPos(scroll_pos, 1);
			Draw_();
		}
		else
			SelectTreeItem();
		return 1;
	}
	return 0;
}
//
//
//
#define UISEARCHTEXTBLOCK_MAXLEN 64

UiSearchTextBlock::UiSearchTextBlock(HWND h, uint ctlId, char * pText, int firstLetter, WordSel_ExtraBlock * pBlk, int linkToList) :
	H_Wnd(h), Id(ctlId), Text(pText), P_WordSel(0), P_WordSelBlk(pBlk), PrevInputCtlProc(0), LinkToList(linkToList), FirstLetter(firstLetter), IsBnClicked(0)
{
}

UiSearchTextBlock::~UiSearchTextBlock()
{
	if(P_WordSel && P_WordSel->H())
		::EndDialog(P_WordSel->H(), cmCancel);
	ZDELETE(P_WordSel);
}

//static
int UiSearchTextBlock::ExecDialog(HWND hWnd, uint ctlId, SString & rText, int isFirstLetter, WordSel_ExtraBlock * pBlk, int linkToList)
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

// static
LRESULT CALLBACK UiSearchTextBlock::InputCtlProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	UiSearchTextBlock * p_slb = static_cast<UiSearchTextBlock *>(TView::GetWindowUserData(hWnd));
	switch(uMsg) {
		case WM_KEYDOWN:
			::SendMessage(GetParent(hWnd), uMsg, wParam, lParam);
			break;
	}
	return (p_slb) ? CallWindowProc(p_slb->PrevInputCtlProc, hWnd, uMsg, wParam, lParam) : DefWindowProc(hWnd, uMsg, wParam, lParam);
}

// static
BOOL CALLBACK UiSearchTextBlock::DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	SString temp_buf;
	switch(uMsg) {
		case WM_INITDIALOG:
			{
				TView::SetWindowUserData(hwndDlg, reinterpret_cast<void *>(lParam));
				UiSearchTextBlock * p_slb = reinterpret_cast<UiSearchTextBlock *>(lParam);
				int    is_browser = 0;
				RECT   parent, chld;
				GetWindowRect(hwndDlg, &chld);
				{
					HWND   h_parent = 0;
					SString cls_name;
					TView::SGetWindowClassName(p_slb->H_Wnd, cls_name);
					if(cls_name.Cmp(SUcSwitch(BrowserWindow::WndClsName), 0) == 0) { // @unicodeproblem
						h_parent = p_slb->H_Wnd;
						is_browser = 1;
					}
					else if(cls_name.Cmp(SUcSwitch(TInputLine::WndClsName), 0) == 0) // @unicodeproblem
						h_parent = p_slb->H_Wnd;
					else
						h_parent = (p_slb->LinkToList) ? p_slb->H_Wnd : GetWindow(hwndDlg, GW_OWNER);
					GetWindowRect(h_parent, &parent);
				}
				int    wd = parent.right - parent.left;
				int    ht = chld.bottom - chld.top;
				int    x  = parent.left;
				int    y  = is_browser ? parent.top : (parent.top - ht);
				MoveWindow(hwndDlg, x, y, wd, ht, 0);
				SendDlgItemMessage(hwndDlg, CTL_LBX_LIST, EM_SETLIMITTEXT, UISEARCHTEXTBLOCK_MAXLEN, 0);
				// @v9.1.5 SString buf = p_slb->P_Text;
				// @v9.1.5 SendDlgItemMessage(hwndDlg, CTL_LBX_LIST, WM_SETTEXT, 0, (LPARAM)(const char *)buf.Transf(CTRANSF_INNER_TO_OUTER));
				TView::SSetWindowText(GetDlgItem(hwndDlg, CTL_LBX_LIST), (temp_buf = p_slb->Text).Transf(CTRANSF_INNER_TO_OUTER));
				p_slb->IsBnClicked = 0;
				if(p_slb->P_WordSelBlk) {
					p_slb->P_WordSelBlk->H_InputDlg = hwndDlg;
					p_slb->P_WordSel = new WordSelector(p_slb->P_WordSelBlk);
					HWND h_ctl = GetDlgItem(hwndDlg, CTL_LBX_LIST);
					TView::SetWindowUserData(h_ctl, p_slb);
					p_slb->PrevInputCtlProc = static_cast<WNDPROC>(TView::SetWindowProp(h_ctl, GWLP_WNDPROC, UiSearchTextBlock::InputCtlProc));
				}
			}
			return 1;
		case WM_SHOWWINDOW:
			if(wParam == 1) {
				UiSearchTextBlock * p_slb = static_cast<UiSearchTextBlock *>(TView::GetWindowUserData(hwndDlg));
				if(p_slb && p_slb->P_WordSel) {
					// @v9.1.5 SendDlgItemMessage(hwndDlg, CTL_LBX_LIST, WM_GETTEXT, UISEARCHTEXTBLOCK_MAXLEN, (long)p_slb->P_Text);
					// @v9.1.5 SCharToOem(p_slb->P_Text);
					TView::SGetWindowText(GetDlgItem(hwndDlg, CTL_LBX_LIST), p_slb->Text);
					p_slb->Text.Transf(CTRANSF_OUTER_TO_INNER);
					p_slb->P_WordSel->Refresh(p_slb->Text);
				}
			}
			break;
		case WM_DESTROY:
			{
				UiSearchTextBlock * p_slb = static_cast<UiSearchTextBlock *>(TView::GetWindowUserData(hwndDlg));
				//SetWindowLong(GetDlgItem(hwndDlg, CTL_LBX_LIST), GWLP_WNDPROC, (long)p_slb->PrevInputCtlProc);
				TView::SetWindowProp(GetDlgItem(hwndDlg, CTL_LBX_LIST), GWLP_WNDPROC, p_slb->PrevInputCtlProc);
			}
			break;
		case WM_COMMAND:
			if(HIWORD(wParam) == BN_CLICKED) {
				UiSearchTextBlock * p_slb = static_cast<UiSearchTextBlock *>(TView::GetWindowUserData(hwndDlg));
				if(p_slb) {
					// @v9.1.5 SendDlgItemMessage(hwndDlg, CTL_LBX_LIST, WM_GETTEXT, UISEARCHTEXTBLOCK_MAXLEN, (long)p_slb->P_Text);
					// @v9.1.5 SCharToOem(p_slb->P_Text);
					TView::SGetWindowText(GetDlgItem(hwndDlg, CTL_LBX_LIST), p_slb->Text); // @v9.1.5
					p_slb->Text.Transf(CTRANSF_OUTER_TO_INNER); // @v9.1.5
					p_slb->IsBnClicked = 1;
					if(p_slb->P_WordSel)
						TView::messageCommand(p_slb->P_WordSel, cmCancel);
				}
				EndDialog(hwndDlg, (LOWORD(wParam) == IDOK) ? cmOK : cmCancel);
			}
			else if(HIWORD(wParam) == EN_SETFOCUS) {
				UiSearchTextBlock * p_slb = static_cast<UiSearchTextBlock *>(TView::GetWindowUserData(hwndDlg));
				int wordsel_visible = (p_slb->P_WordSel) ? p_slb->P_WordSel->CheckVisible() : 0;
				if(!wordsel_visible) {
					SendDlgItemMessage(hwndDlg, CTL_LBX_LIST, WM_KEYDOWN, VK_END, 1);
					SendDlgItemMessage(hwndDlg, CTL_LBX_LIST, WM_KEYUP,   VK_END, 1);
					SendDlgItemMessage(hwndDlg, CTL_LBX_LIST, EM_SETSEL,  0xffff, 0xffff);
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
				int wordsel_visible = (p_slb->P_WordSel) ? p_slb->P_WordSel->CheckVisible() : 0;
				int send_cancel = BIN(!p_slb->IsBnClicked && !wordsel_visible);
				if(send_cancel || p_slb->LinkToList) {
					HWND   h_parent = (p_slb->LinkToList) ? p_slb->H_Wnd : GetWindow(hwndDlg, GW_OWNER);
					if(send_cancel)
						EndDialog(hwndDlg, cmCancel);
					if(h_parent)
						SetFocus(h_parent);
						// SendMessage(h_parent, WM_SETFOCUS, 0, 0);
				}
			}
			if(HIWORD(wParam) == EN_CHANGE) {
				UiSearchTextBlock * p_slb = static_cast<UiSearchTextBlock *>(TView::GetWindowUserData(hwndDlg));
				if(p_slb->P_WordSel) {
					long   id = 0;
					SString result_text;
					// @v9.1.5 SendDlgItemMessage(hwndDlg, CTL_LBX_LIST, WM_GETTEXT, UISEARCHTEXTBLOCK_MAXLEN, (long)p_slb->P_Text);
					// @v9.1.5 SCharToOem(p_slb->P_Text);
					TView::SGetWindowText(GetDlgItem(hwndDlg, CTL_LBX_LIST), p_slb->Text); // @v9.1.5
					p_slb->Text.Transf(CTRANSF_OUTER_TO_INNER); // @v9.1.5
					if(p_slb->P_WordSelBlk && p_slb->Text.Len() >= p_slb->P_WordSelBlk->MinSymbCount && p_slb->P_WordSelBlk->SearchText(p_slb->Text, &id, result_text) > 0) {
						p_slb->P_WordSelBlk->SetData(id, result_text);
						::SendMessage(hwndDlg, WM_COMMAND, MAKEWPARAM(IDOK, BN_CLICKED), CTL_LBX_LIST);
					}
					else {
						p_slb->P_WordSel->Refresh(p_slb->Text);
					}
				}
			}
			return 0;
		case WM_KEYDOWN:
			{
				UiSearchTextBlock * p_slb = static_cast<UiSearchTextBlock *>(TView::GetWindowUserData(hwndDlg));
				if(p_slb->P_WordSel && p_slb->P_WordSel->CheckVisible()) {
					long key = wParam;
					if(key == VK_DOWN) {
						if(p_slb->P_WordSel->Activate() > 0)
							return 0;
					}
					else if(key == VK_ESCAPE)
						::SendMessage(hwndDlg, WM_COMMAND, MAKEWPARAM(IDCANCEL, BN_CLICKED), CTL_LBX_LIST);
					else if(key == VK_RETURN)
						::SendMessage(hwndDlg, WM_COMMAND, MAKEWPARAM(IDOK, BN_CLICKED), CTL_LBX_LIST);
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
	int    r = -1;
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
			r = search(pattern, SrchFunc, (pattern[0] == '*') ? srchNext : srchFirst);
		}
	}
	else if((srchMode & ~srchFlags) == srchNext) {
		if(srch_pattern.NotEmpty()) {
			srch_pattern.CopyTo(pattern, sizeof(pattern));
			r = search(pattern, SrchFunc, srchNext);
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
	ZDELETE(def);
	if(aDef) {
		def = aDef;
		setRange(def->getRecsCount());
		if(def->Options & lbtHSizeAlreadyDef)
			Height = def->ViewHight;
		else
			def->setViewHight(Height);
		def->top();
	}
}

int SmartListBox::TransmitData(int dir, void * pData)
{
	int    s = 0;
	if(dir > 0) {
		if(def) {
			SETFLAG(State, stDataFounded, (pData ? def->TransmitData(dir, pData) : 0));
			if(!Columns.getCount())
				if(!pData)
					def->top();
				else if(!(State & stTreeList)) {
					long scroll_delta, scroll_pos;
					def->getScrollData(&scroll_delta, &scroll_pos);
					SetScrollBarPos(scroll_pos, 1);
				}
			s = 1;
		}
		else
			State &= ~stDataFounded;
	}
	else if(def)
		s = def->TransmitData(dir, pData);
	return s;
}

IMPL_HANDLE_EVENT(SmartListBox)
{
	if(event.isCmd(cmDraw)) {
		Implement_Draw();
	}
}

void SmartListBox::Implement_Draw()
{
	if(!(State & stTreeList)) {
		long   i;
		long   item;
		SString buf, cell_buf;
		buf.Space().Z();
		cell_buf.Space().Z();
		const  HWND h_lb = getHandle();
		if(Columns.getCount())
			ListView_DeleteAllItems(h_lb);
		else
			::SendMessage(h_lb, LB_RESETCONTENT, 0, 0);
		{
			if(HIML) {
				ImageList_Destroy(HIML);
				HIML = 0;
			}
			if(def)
				HIML = def->CreateImageList(TProgram::GetInst());
			if(HIML)
		 		ListView_SetImageList(h_lb, HIML, LVSIL_SMALL);
		}
		if(Height) {
			long   first_item = 0, last_item = 0;
			const  uint cc = Columns.getCount();
			StringSet ss(SLBColumnDelim);
			if(def)
				if(cc)
					last_item  = def->getRecsCount();
				else {
					first_item = def->_topItem();
					last_item  = def->ViewHight;
				}
			for(i = 0, item = first_item; i < last_item; i++, item++) {
				if(def) {
					def->getText(item, buf);
					buf.Transf(CTRANSF_INNER_TO_OUTER);
				}
				else
					buf.Z();
				if(cc) {
					LVITEM    lvi;
					ss.setBuf(buf);
					lvi.mask  = LVIF_TEXT;
					lvi.iItem = i;
					if(def) {
						long id = 0, img_idx = 0;
						if(def->Options & lbtAutoID)
							id = item;
						else
							getID(item, &id);
						if(def->GetImageIdxByID(id, &img_idx) > 0) {
							lvi.iImage = img_idx;
							lvi.mask |= LVIF_IMAGE;
						}
					}
					for(uint k = 0, pos = 0; k < cc; k++) {
						ss.get(&pos, cell_buf);
						lvi.pszText  = const_cast<TCHAR *>(SUcSwitch(cell_buf.Strip().cptr())); // @badcast // @unicodeproblem
						lvi.iSubItem = k;
						if(k) {
							// lvi.mask &= ~LVIF_IMAGE;
							// lvi.iImage = -1;
							ListView_SetItem(h_lb, &lvi); // @unicodeproblem
						}
						else
							ListView_InsertItem(h_lb, &lvi); // @unicodeproblem
					}
				}
				else
					::SendMessage(h_lb, LB_ADDSTRING, 0, (State & stOwnerDraw) ? static_cast<LPARAM>(item) : reinterpret_cast<LPARAM>(SUcSwitch(buf.cptr())));
			}
		}
		ShowWindow(h_lb, SW_NORMAL);
		UpdateWindow(h_lb);
		if(Columns.getCount()) {
			LVITEM lvi;
			lvi.mask  = LVIF_STATE;
			lvi.state = LVIS_FOCUSED | LVIS_SELECTED;
			lvi.stateMask = LVIS_FOCUSED | LVIS_SELECTED;
			::SendMessage(h_lb, LVM_SETITEMSTATE, def ? def->_curItem() : 0, reinterpret_cast<LPARAM>(&lvi));
			ListView_EnsureVisible(h_lb, def ? def->_curItem() : 0, 0); // AHTOXA
		}
		else
			::SendMessage(h_lb, LB_SETCURSEL, def ? (def->_curItem()-def->_topItem()) : 0, 0);
	}
	else if(def) {
		//SetupTreeWnd(0, 0);
		SetupTreeWnd2(0);
		SelectTreeItem();
	}
}

void SmartListBox::selectItem(long)
{
	MessageCommandToOwner(cmLBItemSelected);
}

void SmartListBox::setRange(long aRange)
{
	Range = aRange;
	if(!Columns.getCount())
		SendDlgItemMessage(Parent, MAKE_BUTTON_ID(Id,1), SBM_SETRANGE, 0, Range-1);
}

void SmartListBox::setState(uint aState, bool enable)
{
	TView::setState(aState, enable);
	if((aState & (sfSelected | sfActive)) && !Columns.getCount())
		if(!(State & stTreeList))
			Draw_();
}

int SmartListBox::addItem(long id, const char * s, long * pPos)
{
	int    r = -1;
	if(def && (r = def->addItem(id, s, pPos)) > 0)
		setRange(def->getRecsCount());
	return r;
}

int SmartListBox::removeItem(long pos)
{
	int    r = -1;
	if(def && (r = def->removeItem(pos)) > 0)
		setRange(def->getRecsCount());
	return r;
}

void SmartListBox::freeAll()
{
	if(def) {
		def->freeAll();
		setRange(def->getRecsCount());
	}
}
//
//
//
ListWindowSmartListBox::ListWindowSmartListBox(const TRect & r, ListBoxDef * d, int) : SmartListBox(r, d), combo(0) {}
//
//
//
WordSelectorSmartListBox::WordSelectorSmartListBox(const TRect & bounds, ListBoxDef * aDef) : ListWindowSmartListBox(bounds, aDef, 0)
{
}

int WordSelectorSmartListBox::handleWindowsMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg) {
		case WM_INITDIALOG:
			onInitDialog(0);
			return 1;
		case WM_CHAR:
			return 0;
		case WM_MOUSEWHEEL:
		case WM_VSCROLL:
		case WM_VKEYTOITEM:
			{
				short code = -1;
				if(uMsg == WM_MOUSEWHEEL) {
					short delta = (short)HIWORD(wParam);
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
					code  = LOWORD(wParam); // @v10.3.2 (short code)-->code
				if(def->_curItem() == 0 && oneof3(code, SB_LINEUP, SB_PAGEUP, SB_TOP)) {
					HWND parent = GetParent(getHandle());
					WordSelector * p_selector = (WordSelector *)TView::GetWindowUserData(parent);
					TView::messageCommand(p_selector, cmUp);
				}
			}
			break;
	}
	return SmartListBox::handleWindowsMessage(uMsg, wParam, lParam);
}
