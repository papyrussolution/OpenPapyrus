// WBROWSE.CPP
// Copyright (c) Sobolev A. 1994-2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2013, 2014, 2015, 2016
// @codepage windows-1251
// WIN32
//
#include <slib.h>
#include <tv.h>
#pragma hdrstop
//
//
//
SLAPI SylkWriter::SylkWriter(const char * pFileName)
{
	Stream = 0;
	Buf = 0;
	Open(pFileName);
}

SLAPI SylkWriter::~SylkWriter()
{
	Close();
}

int SLAPI SylkWriter::Open(const char * pFileName)
{
	Close();
	if(pFileName) {
		Stream = fopen(pFileName, "w");
		return BIN(Stream);
	}
	else
		return 1;
}

int SLAPI SylkWriter::Close()
{
	SFile::ZClose(&Stream);
	Buf = 0;
	return 1;
}

int SLAPI SylkWriter::PutLine(const char * pStr)
{
	if(Stream) {
		fputs(pStr, Stream);
		fputc('\n', Stream);
		return 1;
	}
	Buf.Cat(pStr).CR();
	return 1;
}

int SLAPI SylkWriter::PutRec(const char * pTypeStr, const char * pStr)
{
	if(Stream) {
		fputs(pTypeStr, Stream);
		fputc(';', Stream);
	}
	Buf.Cat(pTypeStr).Semicol();
	return PutLine(pStr);
}

int SLAPI SylkWriter::PutRec(int typeChr, const char * pStr)
{
	char temp_buf[8];
	temp_buf[0] = typeChr;
	temp_buf[1] = 0;
	return PutRec(temp_buf, pStr);
}

int SLAPI SylkWriter::PutVal(const char * pStr, int cvtOemToChr)
{
	char temp_buf[128];
	if(pStr) {
		size_t d = 0;
		const char * s = pStr;
		while(*s && d < sizeof(temp_buf)-2) {
			if(*s == ';')
				temp_buf[d++] = ';';
			temp_buf[d++] = *s++;
		}
		temp_buf[d] = 0;
		if(cvtOemToChr)
			SOemToChar(temp_buf);
	}
	else
		temp_buf[0] = 0;
	if(Stream)
		fprintf(Stream, "C;K\"%s\"\n", temp_buf);
	Buf.Cat("C;K\"").Cat(temp_buf).CatChar('\"').CR();
	return 1;
}

int SLAPI SylkWriter::PutVal(double val)
{
	if(Stream)
		fprintf(Stream, "C;K%lf\n", val);
	Buf.Cat("C;K").Cat(val, SFMT_MONEY).CR();
	return 1;
}

int SLAPI SylkWriter::PutColumnWidth(int start, int end, int width)
{
	char temp_buf[32];
	sprintf(temp_buf, "W%d %d %d", start, end, width);
	return PutRec('F', temp_buf);
}

int SLAPI SylkWriter::GetBuf(SString * pBuf) const
{
	return pBuf ? ((*pBuf = Buf), 1) : 0;
}

int SLAPI SylkWriter::PutFormat(const char * pBuf, int fontId, int col, int row)
{
	char   temp_buf[128];
	size_t p = 0;
	strnzcpy(temp_buf+p, pBuf, sizeof(temp_buf)-p);
	p += strlen(temp_buf+p);
	if(fontId) {
		temp_buf[p++] = ';';
		temp_buf[p++] = 'S';
		temp_buf[p++] = 'M';
		p += strlen(itoa(fontId, temp_buf+p, 10));
	}
	if(col > 0 && row > 0) {
		temp_buf[p++] = ';';
		temp_buf[p++] = 'X';
		p += strlen(itoa(col, temp_buf+p, 10));
		temp_buf[p++] = ';';
		temp_buf[p++] = 'Y';
		p += strlen(itoa(row, temp_buf+p, 10));
	}
	else if(col > 0) {
		temp_buf[p++] = ';';
		temp_buf[p++] = 'C';
		p += strlen(itoa(col, temp_buf+p, 10));
	}
	else if(row > 0) {
		temp_buf[p++] = ';';
		temp_buf[p++] = 'R';
		p += strlen(itoa(row, temp_buf+p, 10));
	}
	temp_buf[p] = 0;
	return PutRec('F', temp_buf);
}

int SLAPI SylkWriter::PutFont(int symb, const char * pFontName, int size, uint fontStyle)
{
	char   temp_buf[128];
	uint   p = 0;
	temp_buf[p++] = symb;
	strnzcpy(temp_buf+p, pFontName, sizeof(temp_buf)-p);
	p += strlen(temp_buf+p);
	if(size) {
		temp_buf[p++] = ';';
		temp_buf[p++] = 'M';
		itoa(size*20, temp_buf+p, 10);
		p += strlen(temp_buf+p);
	}
	if(fontStyle) {
		temp_buf[p++] = ';';
		temp_buf[p++] = 'S';
		if(fontStyle & slkfsBold)
			temp_buf[p++] = 'B';
		if(fontStyle & slkfsItalic)
			temp_buf[p++] = 'I';
		if(fontStyle & slkfsUnderline)
			temp_buf[p++] = 'U';
	}
	temp_buf[p] = 0;
	return PutRec('P', temp_buf);
}
//
//
//
BroColumn::BroColumn()
{
	THISZERO();
}
//
//
//
TBaseBrowserWindow::TBaseBrowserWindow(LPCTSTR pWndClsName) : TWindow(TRect(1, 1, 50, 20), 0, 0)
{
	ResourceID = 0;
	ClsName    = pWndClsName;
	ToolbarID  = 0;
	BbState    = 0;
	//WoScrollbars = false;
}

//virtual
TBaseBrowserWindow::IdentBlock & TBaseBrowserWindow::GetIdentBlock(TBaseBrowserWindow::IdentBlock & rBlk)
{
	rBlk.IdBias = 0;
	rBlk.ClsName = 0;
	rBlk.InstanceIdent = 0;
	return rBlk;
}

int TBaseBrowserWindow::Insert()
{
	int    ret = 0;
	SString buf = getTitle();
	if(buf.Empty())
		buf = ClsName;
	buf.Transf(CTRANSF_INNER_TO_OUTER);
	if(IsIconic(APPL->H_MainWnd))
		ShowWindow(APPL->H_MainWnd, SW_MAXIMIZE);
	if(GetActiveWindow() != APPL->H_MainWnd) {
		setState(sfModal, true);
		{
			// @v9.0.4 ret = (int)execute();
			// @v9.0.4 {
			{
				TEvent event;
				event.what = evCommand;
				event.message.command = cmExecute;
				event.message.infoPtr = 0;
				this->handleEvent(event);
				ret = (event.what == evNothing) ? event.message.infoLong : 0;
			}
			// } @v9.0.4
		}
		delete this;
	}
	else {
		DWORD  style = WS_CHILD | WS_CLIPSIBLINGS | WS_TABSTOP;
		HWND   hw_frame = APPL->GetFrameWindow();
		if(!(BbState & bbsWoScrollbars))
			style |= (WS_HSCROLL | WS_VSCROLL);
		if(hw_frame) {
			RECT   rc_frame;
			::GetClientRect(hw_frame, &rc_frame);
			HW = ::CreateWindowEx(0, ClsName, buf, style, 0, 0,
				rc_frame.right-16, rc_frame.bottom, hw_frame, 0, TProgram::GetInst(), this); // @unicodeproblem
			::ShowWindow(HW, SW_SHOW);
			if(ResourceID == 0) {
				HWND   hw = GetTopWindow(hw_frame);
				while(hw && (hw == APPL->H_CloseWnd || hw == HW))
					hw = GetNextWindow(hw, GW_HWNDNEXT);
				if(hw) {
					TBaseBrowserWindow * p_brw = (TBaseBrowserWindow *)TView::GetWindowUserData(hw);
					if(p_brw) {
						if(ClsName == STimeChunkBrowser::WndClsName)
							ResourceID = p_brw->GetResID() + TBaseBrowserWindow::IdBiasTimeChunkBrowser;
						else if(ClsName == /*STextBrowser::WndClsName*/"STextBrowser")
							ResourceID = p_brw->GetResID() + TBaseBrowserWindow::IdBiasTextBrowser;
						else
							ResourceID = p_brw->GetResID() + TBaseBrowserWindow::IdBiasBrowser;
					}
				}
			}
			::UpdateWindow(H());
		}
		ret = H() ? -1 : 0;
	}
	return ret;
}

uint SLAPI TBaseBrowserWindow::GetResID() const
	{ return ResourceID; }

void SLAPI TBaseBrowserWindow::SetResID(uint res)
{
	ResourceID = res;
	if(res && res < TBaseBrowserWindow::IdBiasBrowser) {
		if(ClsName == STimeChunkBrowser::WndClsName)
			ResourceID += TBaseBrowserWindow::IdBiasTimeChunkBrowser;
		else if(ClsName == /*STextBrowser::WndClsName*/"STextBrowser")
			ResourceID += TBaseBrowserWindow::IdBiasTextBrowser;
		/*
		if(ClsName == STimeChunkBrowser::WndClsName)
			ResourceID += TBaseBrowserWindow::IdBiasTimeChunkBrowser;
		else
			ResourceID += TBaseBrowserWindow::IdBiasBrowser;
		*/
	}
}

void SLAPI TBaseBrowserWindow::SetToolbarID(uint toolbarID)
{
	ToolbarID = toolbarID;
}

IMPL_HANDLE_EVENT(TBaseBrowserWindow)
{
	if(event.isCmd(cmExecute)) {
		ushort last_command = 0;
		SString buf = getTitle();
		buf.Transf(CTRANSF_INNER_TO_OUTER);
		RECT   parent, r;
		long   tree_width = 0;
		if(IsIconic(APPL->H_MainWnd))
			ShowWindow(APPL->H_MainWnd, SW_MAXIMIZE);
		APPL->GetStatusBarRect(&r);
		long   status_height = r.bottom - r.top;
		GetWindowRect(APPL->H_MainWnd, &parent);
		if(APPL->IsTreeVisible()) {
			APPL->GetTreeRect(r);
			tree_width = r.right - r.left;
		}
		{
			const int par_wd = parent.right-parent.left;
			const int par_ht = parent.bottom-parent.top;
			const int border_x = GetSystemMetrics(SM_CXBORDER);
			const int border_y = GetSystemMetrics(SM_CYBORDER);
			const int menu_y = GetSystemMetrics(SM_CYMENU);
			const int cap_y = GetSystemMetrics(SM_CYCAPTION);
			r.left   = par_wd * origin.x / 80 + tree_width;
			r.right  = par_wd * size.x / 80 - border_x * 2 - tree_width;
			r.top    = par_ht * origin.y / 23;
			r.bottom = par_ht * size.y / 23 - border_y * 2 - cap_y - menu_y - status_height;
			r.left += border_x + parent.left;
			r.top  += border_y + cap_y + menu_y + parent.top;
		}
		DWORD  style = WS_POPUP|WS_CAPTION|WS_HSCROLL|WS_VSCROLL|WS_SYSMENU|WS_THICKFRAME|WS_VISIBLE|WS_CLIPSIBLINGS;
		if(!H() && APPL->H_MainWnd) {
			if(IsMDIClientWindow(APPL->H_MainWnd)) {
				MDICREATESTRUCT child;
				child.szClass = BrowserWindow::WndClsName;
				child.szTitle = buf;
				child.hOwner = TProgram::GetInst();
				child.x  = CW_USEDEFAULT;	// rect->Left;
				child.y  = CW_USEDEFAULT;	// rect->top;
				child.cx = CW_USEDEFAULT;	// rect->Right;
				child.cy = CW_USEDEFAULT;	// rect->bottom;
				child.style  = style;
				child.lParam = (LPARAM)(BrowserWindow*)this;
				HW = (HWND)LOWORD(SendMessage(APPL->H_MainWnd, WM_MDICREATE, 0, (LPARAM)&child)); // @unicodeproblem
			}
			else {
				HW = CreateWindow(ClsName, buf, style, r.left,
					r.top, r.right, r.bottom, (APPL->H_TopOfStack), NULL, TProgram::GetInst(), this); // @unicodeproblem
			}
		}
		/*
		if(InitPos > 0)
			go(InitPos);
		*/
		::ShowWindow(H(), SW_NORMAL);
		::UpdateWindow(H());
		if(APPL->PushModalWindow(this, H())) {
			EnableWindow(PrevInStack, 0);
			APPL->MsgLoop(this, EndModalCmd);
			last_command = EndModalCmd;
			EndModalCmd = 0;
			APPL->PopModalWindow(this, 0);
		}
		clearEvent(event);
		event.message.infoLong = last_command;
	}
	else
		TWindow::handleEvent(event);
}
//
// BrowseWindow
//
const BrowserColorsSchema BrwColorsSchemas[NUMBRWCOLORSCHEMA] = {
	{1, RGB(0xD6, 0xD3, 0xCE), RGB(0x90, 0x90, 0x90), RGB(0xFF, 0xFF, 0xFF), RGB(0x00, 0x00, 0x00),
		RGB(0x80, 0x80, 0x80), RGB(0xFF, 0xFF, 0xFF), RGB(0xC0, 0xC0, 0xC0), RGB(0x00, 0x00, 0x00),
		RGB(0xC0, 0xC0, 0xC0), RGB(0x90, 0x90, 0x90)
	},
	{2, RGB(0xD6, 0xD3, 0xCE), RGB(0x90, 0x90, 0x90), RGB(0xFF, 0xFB, 0xF0), RGB(0x00, 0x00, 0x00),
		RGB(0x29, 0x69, 0x9C), RGB(0xFF, 0xFF, 0xFF), RGB(0xC0, 0xC0, 0xC0), RGB(0x00, 0x00, 0x00),
		RGB(0xB5, 0xCB, 0xC6), RGB(0xFF, 0xB6, 0xE7)
	},
	{3, RGB(0xDE, 0xD7, 0xB5), RGB(0xC6, 0xBA, 0x9C), RGB(0xD6, 0xE3, 0xCE), RGB(0x00, 0x00, 0x00),
		RGB(0xC6, 0xA7, 0x86), RGB(0xFF, 0xFF, 0xFF), RGB(0xC6, 0xBA, 0x9C), RGB(0x00, 0x00, 0x00),
		RGB(0xC6, 0xBA, 0x9C), RGB(0xC6, 0xBA, 0x9C)
	}
};

BrowserPens::BrowserPens()
{
	THISZERO();
}

void BrowserPens::Destroy()
{
	ZDeleteWinGdiObject(&GridHorzPen);
	ZDeleteWinGdiObject(&GridVertPen);
	ZDeleteWinGdiObject(&DrawFocusPen);
	ZDeleteWinGdiObject(&ClearFocusPen);
	ZDeleteWinGdiObject(&TitlePen);
	ZDeleteWinGdiObject(&FocusOuterPen);
}

BrowserBrushes::BrowserBrushes()
{
	THISZERO();
}

void BrowserBrushes::Destroy()
{
	ZDeleteWinGdiObject(&DrawBrush);
	ZDeleteWinGdiObject(&ClearBrush);
	ZDeleteWinGdiObject(&TitleBrush);
	ZDeleteWinGdiObject(&CursorBrush);
}

// static
LPCTSTR BrowserWindow::WndClsName = _T("SBROWSER");
//
//
//
int ImpLoadToolbar(TVRez & rez, ToolbarList * pList)
{
	pList->setBitmap(rez.getUINT());
	SString temp_buf;
	while(rez.getUINT() != TV_END) {
		fseek(rez.getStream(), -((long)sizeof(uint16)), SEEK_CUR);
		ToolbarItem item;
		MEMSZERO(item);
		item.Cmd = rez.getUINT();
		if(item.Cmd != TV_MENUSEPARATOR) {
			item.KeyCode = rez.getUINT();
			item.Flags = rez.getUINT();
			item.BitmapIndex = rez.getUINT();
			// @v9.0.11 rez.getString(item.ToolTipText);
			// @v9.0.11 {
			rez.getString(temp_buf, 0);
            SLS.ExpandString(temp_buf, CTRANSF_UTF8_TO_OUTER);
            STRNSCPY(item.ToolTipText, temp_buf);
            // } @v9.0.11
		}
		else
			item.KeyCode = (ushort)item.Cmd;
		pList->addItem(&item);
	}
	return 1;
}
#if 0 // {
static int ImpLoadToolbar(TVRez & rez, ToolbarList * pList)
{
	pList->setBitmap(rez.getUINT());
	while(rez.getUINT() != TV_END) {
		fseek(rez.getStream(), -((long)sizeof(uint16)), SEEK_CUR);
		ToolbarItem item;
		MEMSZERO(item);
		item.Cmd = rez.getUINT();
		if(item.Cmd != TV_MENUSEPARATOR) {
			item.KeyCode = rez.getUINT();
			item.Flags = rez.getUINT();
			item.BitmapIndex = rez.getUINT();
			rez.getString(item.ToolTipText);
		}
		else
			item.KeyCode = (ushort)item.Cmd;
		pList->addItem(&item);
	}
	return 1;
}
#endif // } 0

int SLAPI LoadToolbar(TVRez * rez, uint tbType, uint tbID, ToolbarList * pList)
{
	return rez->findResource(tbID, tbType, 0, 0) ? ImpLoadToolbar(*rez, pList) : 0;
}

static const char * WrSubKey_BrwUserSetting = "Software\\Papyrus\\Table2"; // @global @v6.3.11 Table --> Table2

int BrowserWindow::SaveUserSettings(int ifChangedOnly)
{
	int    ok = -1;
	if(!ifChangedOnly || IsUserSettingsChanged) {
		char   param[32];
		WinRegKey reg_key(HKEY_CURRENT_USER, WrSubKey_BrwUserSetting, 0);
		ltoa(RezID, param, 10);
		// version, num_columns, column1_size, ...
		char   ver[32];
		ver[0] = '0';
		ver[1] = 0;
		SString temp_buf;
		temp_buf.Cat(ver).Semicol().Cat(P_Def->getCount());
		for(uint i = 0; i < P_Def->getCount(); i++) {
			const BroColumn & r_col = P_Def->at(i);
			long _p = (r_col.OrgOffs == 0xffff) ? r_col.Offs : r_col.OrgOffs;
			temp_buf.Semicol().Cat(_p).Comma().Cat(SFMTLEN(r_col.format));
		}
		ok = reg_key.PutString(param, temp_buf);
	}
	return -1;
}

int BrowserWindow::RestoreUserSettings()
{
	int    ok = -1;
	char   spec[1024], param[32];
	WinRegKey reg_key(HKEY_CURRENT_USER, WrSubKey_BrwUserSetting, 1);
	ltoa(RezID, param, 10);
	if(reg_key.GetString(param, spec, sizeof(spec))) {
		// version, num_columns, column1_size, ...
		char   ver[32];
		SString temp_buf, org_offs_buf, len_buf;
		uint   pos = 0;
		spec[sizeof(spec)-1] = 0;
		StringSet ss(';', spec);
		ss.get(&pos, ver, sizeof(ver));
		ss.get(&pos, temp_buf);
		const uint num_cols = (uint)temp_buf.ToLong();
		while(ss.get(&pos, temp_buf)) {
			if(temp_buf.Divide(',', org_offs_buf, len_buf) > 0) {
				const long org_offs = org_offs_buf.ToLong();
				const long len = len_buf.ToLong();
				for(uint i = 0; i < P_Def->getCount(); i++) {
					const BroColumn & r_col = P_Def->at(i);
					long _p = (r_col.OrgOffs == 0xffff) ? r_col.Offs : r_col.OrgOffs;
					if(_p == org_offs) {
						if(len > 0 && len < 0x0fffL) {
							SetColumnWidth(i, len);
							P_Def->at(i).Options |= BCO_SIZESET;
						}
						break;
					}
				}
			}
		}
		ok = 1;
	}
	return ok;
}
//
// Если dataKind == 1, то data - (SArray *), 2 - (DBQuery *)
//
int BrowserWindow::LoadResource(uint rezID, void * pData, int dataKind, uint uOptions/*=0*/)
{
	int   ok = 1;
	THROW(P_SlRez);
	{
		char   buf[512];
		SString temp_buf;
		BroGroup grp;
		int    is_group = 0;
		int    columns_count = 0;
		TVRez & rez = *P_SlRez;
		THROW(rez.findResource(rezID, TV_BROWSER));
		Id = rezID;
		{
			const  TRect _bounds = rez.getRect();
			uint   hight  = rez.getUINT();
			uint   freeze = rez.getUINT();
			// @v9.1.1 setTitle(rez.getString(buf, 2), 1);
			// @v9.1.1 {
			rez.getString(temp_buf = 0, 2);
			SLS.ExpandString(temp_buf, CTRANSF_UTF8_TO_INNER);
			setOrgTitle(temp_buf);
			// } @v9.1.1
			uint   options = rez.getUINT();
			options = NZOR(uOptions, options);
			uint   help_ctx = rez.getUINT();
			BrowserDef * p_def;
			if(dataKind == 1)
				p_def = new AryBrowserDef((SArray *)pData, 0, hight, options);
			else
				p_def = new DBQBrowserDef(*(DBQuery *)pData, hight, options);
			THROW(p_def);
			for(int __done = 0; !__done;) {
				const uint tag = rez.getUINT();
				switch(tag) {
					case TV_BROCOLUMN:
						{
							char   fld_name[64];
							// @v9.1.1 rez.getString(buf, 2);
							// @v9.1.1 {
							rez.getString(temp_buf, 2);
							SLS.ExpandString(temp_buf, CTRANSF_UTF8_TO_INNER);
							// } @v9.1.1
							uint   fld = rez.getUINT();
							if(fld == 0xffff)
								fld = UNDEF;
							uint   opt = rez.getUINT();
							TYPEID type = rez.getType(0);
							long   fmt  = rez.getFormat(0);
							rez.getString(fld_name, 2);
							const uint t = atoi(fld_name);
							if(t == -1 || fld == t || *fld_name == '0')
								p_def->insertColumn(UNDEF, temp_buf, fld, type, fmt, opt);
							else
								p_def->insertColumn(UNDEF, temp_buf, fld_name, type, fmt, opt);
						}
						columns_count++;
						break;
					case TV_BROGROUP:
						grp.text  = newStr(rez.getString(buf, 2));
						grp.hight = rez.getUINT();
						grp.first = columns_count;
						is_group = 1;
						break;
					case TV_TOOLBAR:
						{
							ToolbarList tb_list;
							ImpLoadToolbar(rez, &tb_list);
							setupToolbar(&tb_list);
						}
						break;
					case TV_IMPTOOLBAR:
						{
							uint   toolbar_id = rez.getUINT();
							long   sav_pos = rez.getStreamPos();
							ToolbarList tb_list;
							if(::LoadToolbar(&rez, TV_EXPTOOLBAR, toolbar_id, &tb_list))
								setupToolbar(&tb_list);
							fseek(rez.getStream(), sav_pos, SEEK_SET);
						}
						break;
					case TV_CROSSTAB:
						{
							BroCrosstab ct;
							MEMSZERO(ct);
							// @v9.1.1 ct.P_Text  = newStr(rez.getString(buf, 2));
							// @v9.1.1 {
							rez.getString(temp_buf, 2);
							SLS.ExpandString(temp_buf, CTRANSF_UTF8_TO_INNER);
							ct.P_Text = newStr(temp_buf);
							// } @v9.1.1
							ct.Options = rez.getUINT();
							ct.Type    = rez.getType(0);
							ct.Format  = rez.getFormat(0);
							p_def->AddCrosstab(&ct);
						}
						break;
					case TV_END:
						if(is_group) {
							grp.count = columns_count - grp.first;
							p_def->addGroup(&grp);
							is_group = 0;
						}
						else
							__done = 1;
						break;
					default:
						__done = 1;
						break;
				}
			}
			changeBounds(_bounds);
			P_Def = p_def;
			if(freeze)
				SetFreeze(freeze);
		}
	}
	CATCHZOK
	return 1;
}

int BrowserWindow::LoadToolbar(uint toolbarId)
{
	int    ok = 1;
	ToolbarList tb_list;
	THROW(P_SlRez);
	THROW(::LoadToolbar(P_SlRez, TV_EXPTOOLBAR, toolbarId, &tb_list));
	setupToolbar(&tb_list);
	CATCHZOK
	return ok;
}

DECL_CMPFUNC(_PcharNoCase);

void BrowserWindow::go(long p)
{
	SETMAX(p, 0);
	WMHScroll(SB_VERT, SB_THUMBPOSITION, p-P_Def->_curItem());
	refresh();
}

void BrowserWindow::top()
{
	WMHScroll(SB_VERT, SB_TOP, 0);
	refresh();
}

void BrowserWindow::bottom()
{
	WMHScroll(SB_VERT, SB_BOTTOM, 0);
	refresh();
}

int BrowserWindow::search2(void * pSrchData, CompFunc cmpFunc, int srchMode, size_t offs)
{
	int    ok = 0;
	long   hdr_width = CalcHdrWidth(1);
	if(P_Def && P_Def->search2(pSrchData, cmpFunc, srchMode, offs) > 0) {
		long scrollDelta, scrollPos;
		P_Def->getScrollData(&scrollDelta, &scrollPos);
		VScrollPos = (P_Def->_curItem() - P_Def->_topItem());
		ItemRect(HScrollPos, VScrollPos, &RectCursors.CellCursor, TRUE);
		LineRect(VScrollPos, &RectCursors.LineCursor, TRUE);
		AdjustCursorsForHdr();
		TRect inv_rec;
		invalidateRect(inv_rec.setwidthrel(0, CliSz.x).setheightrel(CapOffs + hdr_width, ViewHeight * YCell).setmarginy(-3), 1);
		ok = 1;
	}
	else
		::PostMessage(H(), WM_KEYDOWN, VK_END, 0L);
	return ok;
}

void BrowserWindow::setRange(ushort)
{
}

void BrowserWindow::initWin()
{
	HW = 0;
	MainCursor = LoadCursor(NULL, IDC_ARROW);
	ResizeCursor = LoadCursor(NULL, IDC_SIZEWE);
	Font = 0;
	Pens.Destroy();
	Pens.DefPen = 0;
	P_Header = 0;
	Brushes.Destroy();
	Brushes.DefBrush = 0;
	DefFont = 0;
	ToolBarWidth = 0;
}

void BrowserWindow::init(BrowserDef * pDef)
{
	SrchFunc = PTR_CMPFUNC(_PcharNoCase);
	F_CellStyle = 0;
	CellStyleFuncExtraPtr = 0;
	InitPos = 0;
	P_Toolbar = 0;
	HScrollPos = 0;
	VScrollPos = 0;
	ResizedCol = 0;
	Freeze = 0;
	Left   = 0;
	Right  = 0;
	P_Def  = 0;
	ChrSz.Set(7, 14);
	YCell  = 16; // default value (эта величина используется как делитель)
	IsUserSettingsChanged = 0;
	LastResizeColumnPos = -1;
	EndModalCmd = 0;
	view = this;
	if(pDef) {
		P_Def = pDef;
		P_Def->VerifyCapHeight();
		P_Def->setViewHight((CliSz.y - CapOffs) / YCell - 1);
		P_Def->top();
		CalcRight();
		setRange((uint16)P_Def->getRecsCount());
	}
	if(H())
		invalidateAll(1);
}

int BrowserWindow::SetCellStyleFunc(CellStyleFunc func, void * extraPtr)
{
	F_CellStyle = func;
	CellStyleFuncExtraPtr = extraPtr;
	return 1;
}

void * BrowserWindow::getItemByPos(long pos)
	{ return (P_Def) ? P_Def->getRow(/*P_Def->_topItem() + */pos) : 0; }
void * BrowserWindow::getCurItem()
	{ return P_Def ? P_Def->getRow(P_Def->_curItem()) : 0; }
const UserInterfaceSettings * BrowserWindow::GetUIConfig() const
	{ return &UICfg; }
long BrowserWindow::CalcHdrWidth(int plusToolbar) const
	{ return (P_Header ? (P_Header->size.y * ChrSz.y) : 0) + (plusToolbar ? ToolBarWidth : 0); }

void BrowserWindow::AdjustCursorsForHdr()
{
	long   hdr_width = CalcHdrWidth(1);
	RectCursors.CellCursor.top    += hdr_width;
	RectCursors.CellCursor.bottom += hdr_width;
	RectCursors.LineCursor.top    += hdr_width;
	RectCursors.LineCursor.bottom += hdr_width;
}

void BrowserWindow::Insert_(TView *p)
{
	TGroup::Insert_(p);
	P_Header = p;
	// @v9.0.1 P_Def->setViewHight((CliSz.y - CapOffs) / YCell - p->getExtent().b.y - 1);
	P_Def->setViewHight((CliSz.y - CapOffs) / YCell - p->size.y - 1); // @v9.0.1
}

SLAPI BrowserWindow::BrowserWindow(uint _rezID, DBQuery * pQuery, uint broDefOptions /*=0*/) :
	TBaseBrowserWindow(BrowserWindow::WndClsName)
{
	P_RowsHeightAry = 0;
	init();
	initWin();
	RezID = ResourceID = _rezID;
	P_Header = 0;
	LoadResource(_rezID, pQuery, 2, broDefOptions);
	options |= ofSelectable;
	HelpCtx = _rezID; // @Muxa
}

SLAPI BrowserWindow::BrowserWindow(uint _rezID, SArray * pAry, uint broDefOptions /*=0*/) :
	TBaseBrowserWindow(BrowserWindow::WndClsName)
{
	P_RowsHeightAry = 0;
	init();
	initWin();
	RezID = ResourceID = _rezID;
	P_Header = 0;
	LoadResource(_rezID, pAry, 1, broDefOptions);
	options |= ofSelectable;
	HelpCtx = _rezID; // @Muxa
}

//virtual
TBaseBrowserWindow::IdentBlock & BrowserWindow::GetIdentBlock(TBaseBrowserWindow::IdentBlock & rBlk)
{
	rBlk.IdBias = IdBiasBrowser;
	rBlk.ClsName = BrowserWindow::WndClsName;
	(rBlk.InstanceIdent = 0).Cat(GetResID());
	return rBlk;
}

int BrowserWindow::ChangeResource(uint resID, DBQuery * pQuery, int force)
{
	int    ok = 1;
	if(force || resID != RezID) {
		SaveUserSettings(1);
		ZDELETE(P_Def);
		P_Toolbar->DestroyHWND();
		ZDELETE(P_Toolbar);
		setupToolbar(0);
		SearchPattern = 0;
		init();
		RezID = ResourceID = resID;
		P_Header = 0;
		LoadResource(resID, pQuery, 2);
		options |= ofSelectable;
		WMHCreate(0);
		ok = 2;
	}
	else
		((DBQBrowserDef *)getDef())->setQuery(*pQuery);
	APPL->redraw();
	return ok;
}

int BrowserWindow::ChangeResource(uint resID, SArray * pAry, int force)
{
	int    ok = 1;
	if(force || resID != RezID) {
		SaveUserSettings(1);
		ZDELETE(P_Def);
		P_Toolbar->DestroyHWND();
		ZDELETE(P_Toolbar);
		setupToolbar(0);
		SearchPattern = 0;
		init();
		RezID = ResourceID = resID;
		P_Header = 0;
		LoadResource(resID, pAry, 1);
		options |= ofSelectable;
		WMHCreate(0);
		ok = 2;
	}
	else
		((AryBrowserDef *)getDef())->setArray(pAry, 0, 1);
	return ok;
}

SLAPI BrowserWindow::~BrowserWindow()
{
	CALLPTRMEMB(P_Toolbar, SaveUserSettings(ToolbarID));
	{
		//
		// Перенесено из BrowserWindow::BrowserWndProc WM_DESTROY
		//
		APPL->DelItemFromMenu(this);
		SaveUserSettings(1);
		//
		// Восстановление ширин колонок броузера
		//
		for(uint i = 0; i < P_Def->getCount(); i++)
			SetColumnWidth(i, (P_Def->at(i).width - 1) / 2);
		ResetOwnerCurrent();
		if(!IsInState(sfModal))
			APPL->P_DeskTop->remove(this);
	}
	ZDELETE(P_Def);
	ZDELETE(P_Toolbar);
	ZDELETE(P_RowsHeightAry);
	if(::IsWindow(H())) {
		HDC    hdc = GetDC(H());
		if(DefFont)
			::SelectObject(hdc, DefFont);
		if(Pens.DefPen)
			::SelectObject(hdc, Pens.DefPen);
		if(Brushes.DefBrush)
			::SelectObject(hdc, Brushes.DefBrush);
		Pens.Destroy();
		Brushes.Destroy();
		ZDeleteWinGdiObject(&Font);
		BrowserWindow * p_this_view_from_wnd = (BrowserWindow*)TView::GetWindowUserData(H());
		if(p_this_view_from_wnd) {
			Sf |= sfOnDestroy;
			::DestroyWindow(H());
			HW = 0;
		}
	}
}

// Prototype
// @v9.1.2 int SLAPI PPCalculator();

int BrowserWindow::GetCurColumn() const
	{ return (int)HScrollPos; }
int BrowserWindow::SetCurColumn(int col)
	{ HScrollPos = (uint)col; return 1; }
void BrowserWindow::setInitPos(long p)
	{ InitPos = p; }

int BrowserWindow::CopyToClipboard()
{
	int    ok = -1;
	uint   cn_count = SelectedColumns.getCount();
	if(P_Def && cn_count) {
		uint   i, j;
		char   buf[512], * p_buf = 0;
    	long   row = 0;
		HGLOBAL h_glb = 0;
		LongArray col_types;
		const char * p_fontface_tnr = "Times New Roman";
		LAssocArray width_ary;
		SString val_buf, out_buf;
		SString dec;
		SylkWriter sw(0);

		sw.PutRec("ID", "PPapyrus");
		::GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SDECIMAL, buf, sizeof(buf)); // @unicodeproblem
		dec.Cat(buf);
		for(j = 0; j < cn_count; j++) {
			long cn = SelectedColumns.at(j);
			if(cn >= 0 && cn < (long)P_Def->getCount())
				col_types.add((long)GETSTYPE(P_Def->at(cn).T));
		}
		OpenClipboard(APPL->H_MainWnd);
		EmptyClipboard();
		sw.PutFont('F', p_fontface_tnr, 10, slkfsBold);
		sw.PutFont('F', p_fontface_tnr, 8,  0);
		sw.PutRec('F', "G");
		// Выводим название групп столбцов
		for(i = 0; i < P_Def->GetGroupCount(); i++) {
			uint pos = 0;
			const BroGroup * p_grp = P_Def->GetGroup(i);
			STRNSCPY(buf, p_grp->text);
			if(SelectedColumns.lsearch(p_grp->first, &pos) > 0) {
				sw.PutFormat("FC0L", 1, pos + 1, 1);
				sw.PutFont('F', p_fontface_tnr, 10, slkfsBold);
				sw.PutVal(buf, 1);
			}
		}
		if(P_Def->GetGroupCount())
			row++;
		// Выводим название столбцов
		for(i = 0; i < cn_count; i++) {
			uint col_num = SelectedColumns.at(i);
			if(col_num < P_Def->getCount()) {
				const BroColumn & r_c = P_Def->at(col_num);
				const long type = GETSTYPE(r_c.T);
				STRNSCPY(buf, r_c.text);
				sw.PutFormat("FC0L", 1, i + 1, row + 1);
				sw.PutVal(buf, 1);
				width_ary.Add(col_num, (long)strlen(buf), 0);
			}
		}
		row += 1; // @v9.1.3 +=2 --> +=1
		P_Def->top();
		do {
			for(j = 0; j < cn_count; j++) {
				long cn  = SelectedColumns.at(j);
				if(cn >= 0 && cn < (long)P_Def->getCount()) {
					long  len = 0;
					uint  stype = col_types.at(j);
					P_Def->getFullText(P_Def->_curItem(), cn, val_buf);
					val_buf.Strip();
					if(stype == S_FLOAT) {
						val_buf.ReplaceChar('.', dec.C(0));
						sw.PutFormat("FG0R", 2, j + 1, row + 1);
						sw.PutVal(val_buf.ToReal());
					}
					else if(stype == S_INT) {
						sw.PutFormat("FG0R", 2, j + 1, row + 1);
						sw.PutVal(val_buf.ToLong());
					}
					else {
						sw.PutFormat("FG0L", 2, j + 1, row + 1);
						sw.PutVal((const char*)val_buf, 1);
					}
					if(width_ary.Search(cn, &len, 0, 1) > 0 && len < (long)val_buf.Len())
						width_ary.Update(cn, val_buf.Len(), 1);
				}
			}
			row++;
		} while(P_Def->step(1) > 0);
		for(i = 0; i < width_ary.getCount(); i++) {
			LAssoc item = width_ary.at(i);
			sw.PutColumnWidth(i + 1, i + 1, item.Val + 2);
		}
		WMHScroll(SB_VERT, SB_BOTTOM, 0);
		sw.PutLine("E");
		sw.GetBuf(&out_buf);
		h_glb = ::GlobalAlloc(GMEM_MOVEABLE, (out_buf.Len() + 1));
		p_buf = (char*)GlobalLock(h_glb);
		out_buf.CopyTo(p_buf, out_buf.Len());
		p_buf[out_buf.Len()] = '\0';
		GlobalUnlock(h_glb);
		SetClipboardData(CF_SYLK, h_glb);
		CloseClipboard();
		ok = 1;
	}
	return ok;
}

IMPL_HANDLE_EVENT(BrowserWindow)
{
	ushort cmd = 0;
	TBaseBrowserWindow::handleEvent(event);
	if(TVCOMMAND) {
		switch(TVCMD) {
			case cmaCalculate:
				{
					// @v9.1.2 PPCalculator();
					SlExtraProcBlock epb;
					SLS.GetExtraProcBlock(&epb);
					if(epb.F_CallCalc)
						epb.F_CallCalc((uint32)H(), 0);
				}
				break;
			case cmGetFocusedNumber:
				{
					double * p_number = (double*)event.message.infoPtr;
					if(p_number) {
						char b[256];
						P_Def->getText(P_Def->_curItem(), GetCurColumn(), b);
						strtodoub(b, p_number);
					}
				}
				break;
			case cmGetFocusedText:
				{
					char * p_text = (char *)event.message.infoPtr;
					if(p_text)
						P_Def->getText(P_Def->_curItem(), GetCurColumn(), p_text);
				}
				break;
			case cmaDesktop:
				if(APPL->H_Desktop && !IsInState(sfModal)) {
					TWindow * p_desk = (TWindow*)TView::GetWindowUserData(APPL->H_Desktop);
					SendMessage(APPL->H_MainWnd, WM_COMMAND, (WPARAM)(long)p_desk, 0);
				}
				break;
			case cmSetFont:
				{
					const SetFontEvent * p_sfe = (const SetFontEvent *)event.message.infoPtr;
					if(p_sfe) {
						TEXTMETRIC tm;
						HDC    dc = GetDC(H());
						Font = p_sfe->FontHandle ? (HFONT)p_sfe->FontHandle : GetStockObject(DEFAULT_GUI_FONT);
						::DeleteObject(SelectObject(dc, Font));
						GetTextMetrics(dc, &tm);
						ChrSz.Set(tm.tmAveCharWidth, tm.tmHeight + tm.tmExternalLeading);
						YCell = ChrSz.y + 2;
						CalcRight();
						ItemRect(0, 0, &RectCursors.CellCursor, TRUE);
						LineRect(0, &RectCursors.LineCursor, TRUE);
						SetupScroll();
						if(p_sfe->DoRedraw) {
							invalidateAll(1);
							::UpdateWindow(H());
						}
					}
				}
				break;
			default:
				return;
		}
	}
	else if(TVKEYDOWN) {
		switch(event.keyDown.keyCode) {
			case kbEnter:  cmd = cmaEdit;   break;
			case kbDel:    cmd = cmaDelete; break;
			case kbIns:    cmd = cmaInsert; break;
			case kbAltIns: cmd = cmaAltInsert;   break;
			case kbCtrlIns:	CopyToClipboard(); break;
			case kbCtrlF:  search(0, srchFirst); break;
			case kbCtrlG:  search(0, srchNext);  break;
			default:
				uchar b[2];
				b[0] = TVCHR;
				b[1] = 0;
				SCharToOem((char *)b);
				if(isalnum(b[0]) || IsLetter866(b[0]) || b[0] == '*') {
					search((char *)b, srchFirst);
					break;
				}
				else
					return;
		}
	}
	else
		return;
	clearEvent(event);
	if(cmd)
		TView::messageCommand(this, cmd);
}

void BrowserWindow::CalcRight()
{
	uint   cnt = P_Def->getCount();
	uint   i = 0;
	int    x = 3;
	BroColumn * c;
	(c = &P_Def->at(0))->x = x;
	while(i < Freeze && (x += (ChrSz.x*c->width + 3)) < CliSz.x)
		(c = &P_Def->at(++i))->x = x;
	(c = &P_Def->at(i = Left))->x = x;
	while(++i < cnt && (x += (ChrSz.x*c->width + 3)) < CliSz.x)
		(c = &P_Def->at(i))->x = x;
	Right = i - 1;
}

BrowserDef * BrowserWindow::getDef()
	{ return P_Def; }

int BrowserWindow::SetDefUserProc(SBrowserDataProc proc, void * extraPtr)
{
	if(P_Def) {
		P_Def->SetUserProc(proc, extraPtr);
		return 1;
	}
	else
		return 0;
}

void BrowserWindow::SetupScroll()
{
	ViewHeight = (CliSz.y - CapOffs - CalcHdrWidth(1)) / YCell - 1;
	P_Def->setViewHight(ViewHeight);
	VScrollMax = MAX(0, (int)P_Def->getRecsCount() - 1);
	VScrollPos = MIN(VScrollPos, VScrollMax);
	HScrollMax = P_Def->getCount() - 1;
	HScrollPos = MIN(HScrollPos, HScrollMax);
	SetScrollRange(H(), SB_VERT, 0, VScrollMax, FALSE);
	SetScrollRange(H(), SB_HORZ, 0, HScrollMax, FALSE);
	SetScrollPos(H(), SB_VERT, VScrollPos, TRUE);
	SetScrollPos(H(), SB_HORZ, HScrollPos, TRUE);
	HWND   parent = GetParent(H());
	if(parent) {
		SendMessage(parent, BRO_ROWCHANGED, (WPARAM)H(), MAKELPARAM(VScrollPos, 0));
		SendMessage(parent, BRO_COLCHANGED, (WPARAM)H(), MAKELPARAM(HScrollPos, 0));
	}
	SendMessage(H(), WM_VSCROLL, SB_THUMBPOSITION, 0);
}

void BrowserWindow::SetColorsSchema(uint32 schemaNum)
{
	HDC    dc = GetDC(H());
	Pens.Destroy();
	Brushes.Destroy();
	schemaNum = (schemaNum >= 0 && schemaNum < NUMBRWCOLORSCHEMA) ? schemaNum : 0;
	SetTextColor(dc, BrwColorsSchemas[schemaNum].Text);
	SetBkColor(dc, BrwColorsSchemas[schemaNum].Background);
	Pens.GridHorzPen    = CreatePen(PS_SOLID, 1, BrwColorsSchemas[schemaNum].GridHorizontal);
	Pens.GridVertPen    = CreatePen(PS_SOLID, 1, BrwColorsSchemas[schemaNum].GridVertical);
	Pens.TitlePen       = CreatePen(PS_SOLID, 1, BrwColorsSchemas[schemaNum].TitleDelim);
	Pens.DrawFocusPen   = CreatePen(/*PS_INSIDEFRAME*/PS_NULL, 2, GetTextColor(dc));
	Pens.ClearFocusPen  = CreatePen(/*PS_INSIDEFRAME*/PS_NULL, 2, GetBkColor(dc));
	Pens.FocusOuterPen  = CreatePen(PS_SOLID, 1, GetGrayColorRef(0.35f)/*RGB(90, 90, 90)*/);
	Brushes.DrawBrush   = CreateSolidBrush(BrwColorsSchemas[schemaNum].LineCursor);
	Brushes.ClearBrush  = CreateSolidBrush(BrwColorsSchemas[schemaNum].Background);
	Brushes.TitleBrush  = CreateSolidBrush(BrwColorsSchemas[schemaNum].Title);
	Brushes.CursorBrush = CreateSolidBrush(BrwColorsSchemas[schemaNum].Cursor);
}

void BrowserWindow::WMHCreate(LPCREATESTRUCT)
{
	TEXTMETRIC tm;
	RECT client;
	HDC  dc = GetDC(H());
	// @v7.7.7 {
	if(DefFont)
		SelectObject(dc, DefFont);
	if(Pens.DefPen)
		SelectObject(dc, Pens.DefPen);
	if(Brushes.DefBrush)
		SelectObject(dc, Brushes.DefBrush);
	ZDeleteWinGdiObject(&Font);
	// } @v7.7.7
	Pens.DefPen = (HPEN)GetCurrentObject(dc, OBJ_PEN);
	DefFont     = (HFONT)GetCurrentObject(dc, OBJ_FONT);
	Brushes.DefBrush = (HBRUSH)GetCurrentObject(dc, OBJ_BRUSH);
	UserInterfaceSettings ui_cfg;
	if(ui_cfg.Restore() > 0)
		UICfg = ui_cfg;
	if(labs(UICfg.TableFont.Size) > 0 && UICfg.TableFont.Face.NotEmpty()) {
		Font = TView::CreateFont(UICfg.TableFont);
	}
	else
		Font = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
	SelectObject(dc, Font);
	GetTextMetrics(dc, &tm);
	ChrSz.Set(tm.tmAveCharWidth, tm.tmHeight + tm.tmExternalLeading);
	YCell = ChrSz.y + 2;
	SetColorsSchema(UICfg.GetBrwColorSchema());
	TView::SetWindowUserData(H(), (BrowserWindow *)this);
	SetCursor(MainCursor);
	SetFocus(H());
	SendMessage(H(), WM_NCACTIVATE, TRUE, 0L);
	GetClientRect(H(), &client);
	CliSz.Set(client.right, client.bottom);
	P_Def->VerifyCapHeight();
	CapOffs = YCell * P_Def->GetCapHeight() + 4;
	RestoreUserSettings();
	for(uint i = 0; i < P_Def->getCount(); i++) {
		SetupColumnWidth(i);
	}
	if(Toolbar.getItemsCount()) {
		P_Toolbar = new TToolbar(H(), TBS_NOMOVE);
		P_Toolbar->Init(ToolbarID, &Toolbar);
		if(P_Toolbar->Valid()) {
			RECT tbr;
			::GetWindowRect(P_Toolbar->H(), &tbr);
			ToolBarWidth = tbr.bottom - tbr.top;
		}
	}
	CalcRight();
	ItemRect(0, 0, &RectCursors.CellCursor, TRUE);
	LineRect(0, &RectCursors.LineCursor, TRUE);
	AdjustCursorsForHdr();
	SetupScroll();
}

void BrowserWindow::SetupColumnWidth(uint colNo)
{
	if(colNo < P_Def->getCount()) {
		BroColumn & r_c = P_Def->at(colNo);
		if(!(r_c.Options & BCO_SIZESET)) {
			int    w, cw = r_c.width;
			const TYPEID ct = r_c.T;
			if(GETSTYPE(ct) == S_ZSTRING)
				w = cw * 2 + 1;
			else if(GETSTYPE(ct) == S_DATE)
				w = 10;
			else
				w = (int)(cw * 6 / 5 + 1);
			r_c.width = max(6, w);
			CalcRight();
			SETSFMTLEN(r_c.format, r_c.width);
			r_c.Options |= BCO_SIZESET;
		}
	}
}

void BrowserWindow::SetColumnWidth(int colNo, int newWidth)
{
	if(colNo >= 0 && colNo < (int)P_Def->getCount()) {
		BroColumn & c = P_Def->at(colNo);
		c.width = max(6, newWidth);
		CalcRight();
		SETSFMTLEN(c.format, c.width);
	}
}

int BrowserWindow::SetColumnTitle(int colNo, const char * pText)
{
	return P_Def ? P_Def->setColumnTitle(colNo, pText) : 0;
}

int BrowserWindow::insertColumn(int atPos, const char * pTxt, uint fldNo, TYPEID typ, long fmt, uint opt)
{
	int    ok = P_Def ? P_Def->insertColumn(atPos, pTxt, fldNo, typ, fmt, opt) : 0;
	if(ok) {
		CalcRight();
		SetupScroll();
	}
	return ok;
}

int BrowserWindow::insertColumn(int atPos, const char * pTxt, const char * pFldName, TYPEID typ, long fmt, uint opt)
{
	int    ok = P_Def ? P_Def->insertColumn(atPos, pTxt, pFldName, typ, fmt, opt) : 0;
	if(ok) {
		CalcRight();
		SetupScroll();
	}
	return ok;
}

int BrowserWindow::removeColumn(int atPos)
{
	int    ok = P_Def ? P_Def->removeColumn(atPos) : 0;
	if(ok) {
		if(atPos <= (int)Left) { // @v9.2.3
			Left--;
			CalcRight();
			SetupScroll();
		}
	}
	return ok;
}

void BrowserWindow::SetFreeze(uint numFreezeCols)
{
	//
	// По крайней мере один столбец
	// должен оставаться незамороженным
	//
	Freeze = P_Def->getCount() ? MIN(numFreezeCols, P_Def->getCount() - 1) : 0;
	for(uint i = 0; i < Freeze; i++)
		if(P_Def->groupOf(i)) { // Замороженные столбцы не должны быть в группе
			Freeze = i;
			break;
		}
	Left = Freeze;
	CalcRight();
}

int FASTCALL BrowserWindow::CellRight(const BroColumn & rC) const
{
	return (ChrSz.x*rC.width + rC.x);
}

int FASTCALL BrowserWindow::GetRowHeightMult(long row) const
{
	return (P_RowsHeightAry && row < (long)P_RowsHeightAry->getCount()) ?
		((RowHeightInfo*)P_RowsHeightAry->at(row))->HeightMult : 1;
}

int FASTCALL BrowserWindow::GetRowTop(long row) const
{
	return (P_RowsHeightAry && row < (long)P_RowsHeightAry->getCount()) ?
		((RowHeightInfo*)P_RowsHeightAry->at(row))->Top : (YCell * row);
}

LPRECT BrowserWindow::ItemRect(int hPos, int vPos, LPRECT rect, BOOL isFocus)
{
	const BroColumn & c = P_Def->at(hPos);
#if 1 // @v8.2.0 { GetRowHeightMult и GetRowTop элиминированы ради быстродействия //
	if(P_RowsHeightAry && vPos < (long)P_RowsHeightAry->getCount()) {
		rect->top    = CapOffs + ((RowHeightInfo*)P_RowsHeightAry->at(vPos))->Top;
		rect->bottom = rect->top + ChrSz.y + YCell * (((RowHeightInfo*)P_RowsHeightAry->at(vPos))->HeightMult-1);
	}
	else {
		rect->top    = CapOffs + (YCell * vPos);
		rect->bottom = rect->top + ChrSz.y;
	}
#else // }{ @v8.2.0
	rect->top    = CapOffs + GetRowTop(vPos);
	rect->bottom = rect->top + ChrSz.y + YCell * (GetRowHeightMult(vPos)-1);
#endif // } @v8.2.0
	rect->left   = c.x + 1;
	rect->right  = CellRight(c) - 1;
	if(isFocus) {
		SInflateRect(*rect, 3, 2);
		rect->bottom--;
	}
	return rect;
}

LPRECT BrowserWindow::LineRect(int vPos, LPRECT rect, BOOL isFocus)
{
	uint   left_pos = Freeze ? 0 : Left;
	uint   count = P_Def->getCount();
	rect->top    = CapOffs + GetRowTop(vPos);
	rect->bottom = rect->top + ChrSz.y + YCell * (GetRowHeightMult(vPos)-1);
	rect->left = P_Def->at(left_pos).x + 1;
	rect->right = 0;
	if(Right >= 0) {
		const uint _right = (Right < count) ? Right : (count-1);
		rect->right = CellRight(P_Def->at(_right)) - 1;
	}
	if(isFocus) {
		SInflateRect(*rect, 3, 2);
		rect->bottom--;
	}
	return rect;
}

void BrowserWindow::DrawFocus(HDC hDC, LPRECT lpRect, BOOL DrawOrClear, BOOL isCellCursor)
{
	HPEN   pen;
	HBRUSH brush;
	if(DrawOrClear) {
		pen   = Pens.DrawFocusPen;
		brush = (isCellCursor ? Brushes.CursorBrush : Brushes.DrawBrush);
	}
	else {
		pen = Pens.ClearFocusPen;
		brush = Brushes.ClearBrush;
	}
	SelectObject(hDC, pen);
	SelectObject(hDC, brush);
	Rectangle(hDC, lpRect->left, lpRect->top, lpRect->right, lpRect->bottom);
	if(DrawOrClear) {
		HPEN   old_pen = (HPEN)SelectObject(hDC, Pens.FocusOuterPen);
		MoveToEx(hDC, lpRect->left + 1, lpRect->top + 1, 0);
		LineTo(hDC, lpRect->right - 3,  lpRect->top + 1);
		LineTo(hDC, lpRect->right - 3,  lpRect->bottom - 2);
		LineTo(hDC, lpRect->left + 1,   lpRect->bottom - 2);
		LineTo(hDC, lpRect->left + 1,   lpRect->top);
		if(old_pen)
			SelectObject(hDC, old_pen);
	}
}

void BrowserWindow::DrawCapBk(HDC hDC, LPRECT lpRect, BOOL isPressed)
{
	RECT   r = *lpRect;
	if(!isPressed) {
		HPEN   oldPen = (HPEN)SelectObject(hDC, Pens.TitlePen);
		POINT  points[6];
		points[0].x = r.right;
		points[0].y = r.top;
		points[1].x = r.right;
		points[1].y = r.bottom;
		points[2].x = r.left;
		points[2].y = r.bottom;
		points[3].x = r.left + 1;
		points[3].y = r.bottom - 1;
		points[4].x = r.right - 1;
		points[4].y = r.bottom - 1;
		points[5].x = r.right - 1;
		points[5].y = r.top + 1;
		Polygon(hDC, points, sizeof(points) / sizeof(POINT));
		SelectObject(hDC, oldPen);
		SInflateRect(r, -1, -1);
	}
	FillRect(hDC, &r, Brushes.TitleBrush);
}

void BrowserWindow::ClearFocusRect(LPRECT pRect)
{
	::InvalidateRect(H(), pRect, FALSE);
	DrawFocus(::GetDC(H()), pRect, FALSE);
}

static int FASTCALL GetCapAlign(int option)
{
	int    fmt = DT_EXTERNALLEADING;
	fmt |= (option & BCO_CAPCENTER)  ? DT_CENTER  : ((option & BCO_CAPRIGHT)  ? DT_RIGHT  : DT_LEFT);
	fmt |= (option & BCO_CAPVCENTER) ? DT_VCENTER : ((option & BCO_CAPBOTTOM) ? DT_BOTTOM : DT_TOP);
	return fmt;
}

int BrowserWindow::refresh()
{
	if(P_Def) {
		P_Def->refresh();
		ItemRect(HScrollPos, P_Def->_curItem() - P_Def->_topItem(), &RectCursors.CellCursor, TRUE);
		LineRect(P_Def->_curItem() - P_Def->_topItem(), &RectCursors.LineCursor, TRUE);
		AdjustCursorsForHdr();
	}
	invalidateAll(1);
	return 1;
}

int BrowserWindow::DrawTextUnderCursor(HDC hdc, char * pBuf, RECT * pTextRect, int fmt, int isLineCursor)
{
	int    ok = 0;
	if(pTextRect && pBuf) {
		uint32 schema_num = UICfg.GetBrwColorSchema();
		COLORREF old_color = SetTextColor(hdc, isLineCursor ?
			BrwColorsSchemas[schema_num].LineCursorOverText : BrwColorsSchemas[schema_num].CursorOverText);
		HFONT  old_font = 0, curs_over_txt_font = 0;
		char   buf[64];
		LOGFONT log_font;
		TEXTMETRIC metrics;
		MEMSZERO(log_font);
		MEMSZERO(metrics);
		memzero(&buf, sizeof(buf));
		GetTextMetrics(hdc, &metrics);
		GetTextFace(hdc, sizeof(buf), buf); // @unicodeproblem
		log_font.lfCharSet = RUSSIAN_CHARSET;
		STRNSCPY(log_font.lfFaceName, buf);
		log_font.lfWeight = FW_MEDIUM;
		log_font.lfHeight = metrics.tmHeight;
		curs_over_txt_font = ::CreateFontIndirect(&log_font);
		if(curs_over_txt_font)
			old_font = (HFONT)SelectObject(hdc, curs_over_txt_font);
		DrawMultiLinesText(hdc, pBuf, pTextRect, fmt);
		SetTextColor(hdc, old_color);
		if(old_font)
			SelectObject(hdc, old_font);
		ZDeleteWinGdiObject(&curs_over_txt_font);
		ok = 1;
	}
	return ok;
}

int BrowserWindow::DrawMultiLinesText(HDC hdc, char * pBuf, RECT * pTextRect, int fmt)
{
	if(pTextRect && pBuf) {
		char   buf[256];
		RECT   rect = *pTextRect;
		StringSet ss('\n', pBuf);
		for(uint i = 0; ss.get(&i, buf, sizeof(buf)); rect.top += YCell, rect.bottom += YCell)
			::DrawText(hdc, buf, strlen(buf), &rect, fmt); // @unicodeproblem
	}
	return 1;
}

int BrowserWindow::GetCellColor(long row, long col, COLORREF * pColor)
{
	int    ok = -1;
	void * p_row = P_Def->getRow(row);
	if(p_row) {
		CellStyle style;
		if(F_CellStyle && F_CellStyle(p_row, col, BrowserWindow::paintNormal, &style, CellStyleFuncExtraPtr) > 0) {
			ASSIGN_PTR(pColor, style.Color);
			ok = 1;
		}
	}
	return ok;
}

int BrowserWindow::PaintCell(HDC hdc, RECT r, long row, long col, int paintAction)
{
	int    ok = -1;
	const  void * p_row = P_Def->getRow(P_Def->_topItem() + row);
	if(p_row) {
		ok = 1;
		CellStyle style;
		MEMSZERO(style);
		if(F_CellStyle && F_CellStyle(p_row, col, paintAction, &style, CellStyleFuncExtraPtr) > 0) {
			HPEN   pen = 0;
			HPEN   oldpen = 0;
			HBRUSH br = 0;
			HBRUSH oldbr = 0;
			if(style.Flags & (CellStyle::fCorner|CellStyle::fLeftBottomCorner|CellStyle::fRightFigCircle)) {
				if(oneof2(paintAction, BrowserWindow::paintFocused, BrowserWindow::paintClear))
					if(paintAction == BrowserWindow::paintClear)
						ClearFocusRect(&r);
					else
						DrawFocus(hdc, &r, TRUE, BIN(col != -1));
				POINT points[4];
				if(style.Flags & CellStyle::fCorner) {
					COLORREF color = style.Color;
					pen = CreatePen(PS_SOLID, 1, color);
					oldpen = (HPEN)SelectObject(hdc, pen);
					br = CreateSolidBrush(color);
					oldbr = (HBRUSH)SelectObject(hdc, br);

					const int    t = r.top+2;
					points[0].x = r.left;
					points[0].y = t;
					points[1].x = r.left + (r.bottom - r.top) / 2;
					points[1].y = t;
					points[2].x = r.left;
					points[2].y = r.top + (r.bottom - r.top) / 2;
					points[3].x = r.left;
					points[3].y = t;
					Polygon(hdc, points, 4);

					SelectObject(hdc, oldpen);
					SelectObject(hdc, oldbr);
					ZDeleteWinGdiObject(&pen);
					ZDeleteWinGdiObject(&br);
				}
				if(style.Flags & CellStyle::fLeftBottomCorner) {
					COLORREF color = style.Color2;
					pen = CreatePen(PS_SOLID, 1, color);
					oldpen = (HPEN)SelectObject(hdc, pen);
					br = CreateSolidBrush(color);
					oldbr = (HBRUSH)SelectObject(hdc, br);

					const int    b = r.bottom-2;
					points[0].x = r.left;
					points[0].y = b;
					points[1].x = r.left + (r.bottom - r.top) / 2;
					points[1].y = b;
					points[2].x = r.left;
					points[2].y = r.top + (r.bottom - r.top) / 2;
					points[3].x = r.left;
					points[3].y = b;
					Polygon(hdc, points, 4);

					SelectObject(hdc, oldpen);
					SelectObject(hdc, oldbr);
					ZDeleteWinGdiObject(&pen);
					ZDeleteWinGdiObject(&br);
				}
				if(style.Flags & CellStyle::fRightFigCircle) {
					COLORREF color = style.RightFigColor;
					pen = CreatePen(PS_SOLID, 1, color);
					oldpen = (HPEN)SelectObject(hdc, pen);
					br = CreateSolidBrush(color);
					oldbr = (HBRUSH)SelectObject(hdc, br);

					const int _diam = 6;
					int   _right = r.right - 6;
					int   _left = _right - _diam;
					int   _top = r.top + 4;
					int   _bottom = _top + _diam + 2;
					Ellipse(hdc, _left, _top, _right, _bottom);

					SelectObject(hdc, oldpen);
					SelectObject(hdc, oldbr);
					ZDeleteWinGdiObject(&pen);
					ZDeleteWinGdiObject(&br);
				}
			}
			else {
				pen = CreatePen(PS_SOLID, 1, style.Color);
				oldpen = (HPEN)SelectObject(hdc, pen);
				br = CreateSolidBrush(style.Color);
				oldbr = (HBRUSH)SelectObject(hdc, br);
				Rectangle(hdc, (long)r.left, (long)r.top, (long)r.right-1, (long)r.bottom);

				SelectObject(hdc, oldpen);
				SelectObject(hdc, oldbr);
				ZDeleteWinGdiObject(&pen);
				ZDeleteWinGdiObject(&br);
			}
		}
		else if(oneof2(paintAction, BrowserWindow::paintFocused, BrowserWindow::paintClear))
			if(paintAction == BrowserWindow::paintClear)
				ClearFocusRect(&r);
			else
				DrawFocus(hdc, &r, TRUE, BIN(col != -1));
	}
	return ok;
}

void BrowserWindow::Paint()
{
	if(P_Def) {
		const  long   hdr_width = CalcHdrWidth(1);
		PAINTSTRUCT ps;
		RECT   r;
		UINT   i, cn, row;
		uint   count = P_Def->getCount();
		uint   gidx;
		int    lt, rt, fmt;
		char   buf[512];
		SString temp_buf;
		BeginPaint(H(), (LPPAINTSTRUCT)&ps);
		if(ps.fErase) {
			RECT rect;
			GetClientRect(H(), &rect);
			FillRect(GetDC(H()), &rect, Brushes.ClearBrush);
			ps.fErase = 0;
		}
		if(ps.rcPaint.bottom == 0 || ps.rcPaint.right == 0)
			GetClientRect(H(), &ps.rcPaint);
		r.top    = ToolBarWidth;
		r.left   = 0;
		r.right  = CliSz.x;
		r.bottom = hdr_width - 1;
		if(P_Header && SIntersectRect(ps.rcPaint, r)) {
			SString temp_buf;
			((TStaticText *)P_Header)->getText(temp_buf);
			temp_buf.Transf(CTRANSF_INNER_TO_OUTER).CopyTo(buf, sizeof(buf));
			::DrawText(ps.hdc, buf, strlen(buf), &r, DT_LEFT); // @unicodeproblem
		}
		r.top     = r.left = 0;
		r.bottom  = ChrSz.y * CapOffs - 3;
		r.right   = CliSz.x;
		r.top    += hdr_width;
		r.bottom += hdr_width;
		if(SIntersectRect(ps.rcPaint, r)) {
			COLORREF oldColor;
			uint32 schema_num = UICfg.GetBrwColorSchema();
			oldColor = SetBkColor(ps.hdc, BrwColorsSchemas[schema_num].Title);
			cn = Freeze ? 0 : Left;
			i = 0;
			while(cn <= (UINT)Right && cn < count) {
				const BroColumn & c = P_Def->at(cn);
				r.left    = c.x - 1;
				r.right   = CellRight(c);
				r.bottom  = YCell * P_Def->GetCapHeight();
				r.top     = P_Def->isColInGroup(cn, &gidx) ? (YCell * P_Def->GetGroup(gidx)->hight + 1) : 0;
				r.top    += hdr_width;
				r.bottom += hdr_width;
				DrawCapBk(ps.hdc, &r, FALSE);
				r.left += 3;
				r.top++;
				r.right -= 3;
				r.bottom--;
				(temp_buf = c.text).Transf(CTRANSF_INNER_TO_OUTER);
				::DrawText(ps.hdc, temp_buf, temp_buf.Len(), &r, GetCapAlign(c.Options)); // @unicodeproblem
				cn = (++i == (UINT)Freeze) ? Left : (cn + 1);
			}
			if(r.right < CliSz.x) {
				r.top     = hdr_width-ToolBarWidth;
				r.bottom  = YCell * P_Def->GetCapHeight() + 1;
				r.left    = r.right + 4;
				r.right   = CliSz.x;
				r.bottom += hdr_width;
				DrawCapBk(ps.hdc, &r, TRUE);
			}
			for(i = 0; i < P_Def->GetGroupCount(); i++) {
				const BroGroup * p_grp = P_Def->GetGroup(i);
				lt = max(p_grp->first, Left);
				rt = min(p_grp->NextColumn()-1, Right);
				if(lt <= rt) {
					r.left    = P_Def->at(lt).x - 1;
					r.right   = CellRight(P_Def->at(rt));
					r.top     = hdr_width;
					r.bottom  = YCell * p_grp->hight + hdr_width;
					fmt = DT_CENTER | DT_EXTERNALLEADING;
					DrawCapBk(ps.hdc, &r, FALSE);
					r.left++;
					r.top++;
					r.right--;
					r.bottom--;
					(temp_buf = p_grp->text).Transf(CTRANSF_INNER_TO_OUTER);
					::DrawText(ps.hdc, temp_buf, temp_buf.Len(), &r, fmt); // @unicodeproblem
				}
			}
			SetBkColor(ps.hdc, oldColor);
		}
		uint   view_height = (P_RowsHeightAry && P_RowsHeightAry->getCount()) ? P_RowsHeightAry->getCount() : ViewHeight;
		for(row = 0; row < view_height; row++) {
			ItemRect(Left, row, &r, FALSE);
			r.left    = 0;
			r.right   = CliSz.x;
			r.top    += hdr_width;
			r.bottom += hdr_width;
			if(SIntersectRect(ps.rcPaint, r)) {
				int    is_focused = BIN(row == (UINT)(P_Def->_curItem() - P_Def->_topItem()));
				int    paint_action = is_focused ? BrowserWindow::paintFocused : BrowserWindow::paintNormal;
				RECT   paint_rect;
				if(is_focused)
					paint_rect = RectCursors.LineCursor;
				else {
					LineRect(row, &paint_rect, TRUE);
					paint_rect.top    += hdr_width;
					paint_rect.bottom += hdr_width - 1;
					paint_rect.right -= 2;
				}
				PaintCell(ps.hdc, paint_rect, row, -1, paint_action);
				i = 0;
				cn = Freeze ? 0 : Left;
				for(; cn <= (UINT)Right && cn < count; cn = (++i == (UINT)Freeze) ? Left : (cn + 1)) {
					int paint_action = (is_focused && cn == HScrollPos) ? BrowserWindow::paintFocused : BrowserWindow::paintNormal;
					ItemRect(cn, row, &r, TRUE);
					r.top    += hdr_width;
					r.bottom += hdr_width;
					if(SIntersectRect(ps.rcPaint, r))
						PaintCell(ps.hdc, r, row, cn, paint_action);
					ItemRect(cn, row, &r, FALSE);
					r.top    += hdr_width;
					r.bottom += hdr_width;
					if(SIntersectRect(ps.rcPaint, r)) {
						uint   height_mult = GetRowHeightMult(row);
						strip(P_Def->getMultiLinesText(P_Def->_topItem() + row, cn, buf, height_mult));
						if(row > 0 && (P_Def->at(cn).Options & BCO_DONTSHOWDUPL)) {
							char   prev_buf[512];
							strip(P_Def->getText(P_Def->_topItem() + row - 1, cn, prev_buf));
							if(!strcmp(buf, prev_buf))
								buf[0] = 0;
						}
						int    opt = P_Def->at(cn).format;
						int    align = SFMTALIGN(opt);
						if(align == ALIGN_LEFT)
							fmt = DT_LEFT;
						else if(align == ALIGN_RIGHT)
							fmt = DT_RIGHT;
						else if(align == ALIGN_CENTER)
							fmt = DT_CENTER;
						else
							fmt = DT_LEFT;
						SOemToChar(buf);
						fmt |= (DT_NOPREFIX | DT_SINGLELINE);
						if(is_focused && cn == HScrollPos)
							SetBkMode(ps.hdc, TRANSPARENT);
						int    already_draw = 0;
						if(SIntersectRect(RectCursors.CellCursor, r))
							already_draw = DrawTextUnderCursor(ps.hdc, buf, &r, fmt, 0);
						else if(SIntersectRect(RectCursors.LineCursor, r))
							already_draw = DrawTextUnderCursor(ps.hdc, buf, &r, fmt, 1);
						if(!already_draw) {
							ItemRect(cn, row, &paint_rect, TRUE);
							paint_rect.top    += hdr_width;
							paint_rect.bottom += hdr_width - 1;
							paint_rect.right -= 2;
							PaintCell(ps.hdc, paint_rect, row, cn, BrowserWindow::paintNormal);
							DrawMultiLinesText(ps.hdc, buf, &r, fmt);
						}
					}
				}
			}
		}
		//
		// Drawing grid
		//
		{
			ItemRect(Left, 0, &r, FALSE);
			uint   r_h_count = P_RowsHeightAry ? P_RowsHeightAry->getCount() : 0;
			uint   sel_col_count = SelectedColumns.getCount();
			long   last_fill_row = 0;
			r.top += hdr_width - 2 + YCell * GetRowHeightMult(0);
			r.left = 0;
			const uint _right = (Right < P_Def->getCount()) ? Right : (P_Def->getCount()-1);
			const BroColumn & c = P_Def->at(_right);
			int    bottom = MIN(ViewHeight, (UINT)P_Def->getRecsCount());
			uint   view_height = r_h_count ? r_h_count : ViewHeight;
			r.right = MIN(CliSz.x, CellRight(c));
			HPEN   old_pen = (HPEN)SelectObject(ps.hdc, Pens.GridHorzPen);
			view_height = (!r_h_count && sel_col_count) ? (view_height - 1) : view_height;
			for(row = 0; row < view_height; row++) {
				MoveToEx(ps.hdc, r.left, r.top, 0);
				LineTo(ps.hdc, r.right, r.top);
				if(r_h_count && (row+1) < view_height)
					r.top += YCell * GetRowHeightMult(row+1);
				else
					r.top += YCell;
			}
			int    topold = (!r_h_count && sel_col_count) ? r.top : r.top - YCell;
			long   prev_left = r.left;
			ItemRect(Left, 0, &r, FALSE);
			HPEN   black_pen = CreatePen(PS_SOLID, 1, GetColorRef(SClrBlack));
			HPEN   dot_line_pen = CreatePen(PS_SOLID, 3, GetColorRef(SClrBlack));
			r.bottom = topold;
			r.top   += hdr_width - 2;
			i = 0;
			SelectObject(ps.hdc, Pens.GridVertPen);
			for(cn = Freeze ? 0 : Left; cn <= (UINT)Right && cn < count;) {
				int  dot_line = 0;
				long dot_line_delta = 6;
				r.left = CellRight(P_Def->at(cn));
				if(sel_col_count) {
					int selected      = BIN(SelectedColumns.bsearch((long)cn, 0) > 0);
					int next_selected = BIN(SelectedColumns.bsearch((long)cn + 1, 0) > 0);
					dot_line = (!next_selected && selected || next_selected && !selected);
					if(selected) {
						SelectObject(ps.hdc, dot_line_pen);
						// нарисуем последнюю линию броузера пунктиром
						for(long left = prev_left; left < r.left; left += dot_line_delta + 2) {
							MoveToEx(ps.hdc, left, r.bottom, 0);
							LineTo(ps.hdc, ((left + dot_line_delta < r.left) ? left + dot_line_delta : r.left), r.bottom);
						}
						SelectObject(ps.hdc, Pens.GridVertPen);
					}
					else {
						SelectObject(ps.hdc, Pens.GridHorzPen);
						// нарисуем последнюю линию броузера
						MoveToEx(ps.hdc, prev_left, r.bottom, 0);
						LineTo(ps.hdc, r.left, r.bottom);
						SelectObject(ps.hdc, Pens.GridVertPen);
					}
				}
				if(dot_line) {
					SelectObject(ps.hdc, dot_line_pen);
					for(long top = r.top; top < r.bottom; top += dot_line_delta + 2) {
						MoveToEx(ps.hdc, r.left, top, 0);
						LineTo(ps.hdc, r.left, ((top + dot_line_delta < r.bottom) ? top + dot_line_delta : r.bottom));
					}
					SelectObject(ps.hdc, Pens.GridVertPen);
				}
				else {
					MoveToEx(ps.hdc, r.left, r.top, 0);
					LineTo(ps.hdc, r.left, r.bottom);
				}
				cn = (++i == (UINT)Freeze) ? Left : (cn + 1);
				prev_left = r.left;
			}
			SelectObject(ps.hdc, old_pen);
			ZDeleteWinGdiObject(&black_pen);
			ZDeleteWinGdiObject(&dot_line_pen);
		}
		::EndPaint(H(), (LPPAINTSTRUCT)&ps);
	}
}

int BrowserWindow::SelColByPoint(LPPOINT point, int action)
{
	int    ok = -1;
	if(!action) {
		if(SelectedColumns.getCount()) {
			SelectedColumns.freeAll();
			ok = 1;
		}
	}
	else if(P_Def) {
		const  long hdr_width = CalcHdrWidth(1);
		if(point->y <= hdr_width + P_Def->GetCapHeight() * YCell) {
			if(action == -1) {
				if(SelectedColumns.getCount())
					SelectedColumns.freeAll();
				else
					for(uint i = 0; i < P_Def->getCount(); i++)
						SelectedColumns.add((long)i);
			}
			else {
				long col = 0;
				uint pos = 0;
				TPoint tp;
				tp = *point;
				ItemByPoint(tp, &col, 0);
				if(SelectedColumns.bsearch(col, &pos) > 0)
					SelectedColumns.atFree(pos);
				else if(col >= 0 && col < (long)P_Def->getCount())
					SelectedColumns.add(col);
			}
			SelectedColumns.sort();
			ok = 1;
		}
	}
	return ok;
}

void BrowserWindow::ItemByPoint(TPoint point, long * pHorzPos, long * pVertPos)
{
	const  long hdr_width = CalcHdrWidth(1);
	int    found = 0;
	uint   i = 0;
	if(Freeze) {
		for(i = 0; i <= Right && i < Freeze; i++) {
			const BroColumn & c = P_Def->at(i);
			if(point.x >= (int)c.x - 2 && point.x <= (CellRight(c) + 2)) {
				found = 1;
				break;
			}
		}
	}
	if(!found) {
		for(i = Left; i <= Right; i++) {
			const BroColumn & c = P_Def->at(i);
			if(point.x >= (int)c.x - 2 && point.x <= (CellRight(c) + 2))
				break;
		}
	}
	ASSIGN_PTR(pHorzPos, (long)i);
	long   vpos = 0;
	uint   r_h_count = (P_RowsHeightAry && P_RowsHeightAry->getCount()) ? P_RowsHeightAry->getCount() : 0;
	if(r_h_count) {
		uint   y = point.y - hdr_width;
		for(uint row = 0; row < r_h_count; row++) {
			RECT rect;
			ItemRect(HScrollPos, row, &rect, FALSE);
			if(y <= (uint)rect.bottom || row == r_h_count - 1) {
				vpos = (long)(P_Def->_topItem() + row);
				break;
			}
		}
	}
	else if(point.y > hdr_width + P_Def->GetCapHeight() * YCell)
		vpos = (point.y - (hdr_width + CapOffs)) / YCell + P_Def->_topItem();
	else
		vpos = P_Def->_topItem();
	ASSIGN_PTR(pVertPos, vpos);
}

void BrowserWindow::ItemByMousePos(long * pHorzPos, long * pVertPos)
{
	TPoint tp;
	POINT p;
	RECT parent_rect;
	::GetWindowRect(H(), &parent_rect);
	GetCursorPos(&p);
	p.x -= parent_rect.left;
	p.y -= parent_rect.top;
	tp = p;
	ItemByPoint(tp, pHorzPos, pVertPos);
}

int BrowserWindow::IsResizePos(TPoint p)
{
	long   hdr_width = CalcHdrWidth();
	if(p.y > hdr_width && p.y < hdr_width + ToolBarWidth + P_Def->GetCapHeight() * YCell)
		for(int i = 0, cn = P_Def->getCount(); i < cn; i++) {
			int    b = CellRight(P_Def->at(i)) - 1;
			if(p.x > (b-5) && p.x < (b+5))
				return i + 1;
		}
	return 0;
}

void BrowserWindow::Resize(TPoint p, int mode)
{
	RECT   r;
	const  long hdr_width = CalcHdrWidth(1);
	int    newSz;
	const  BroColumn & r_c = P_Def->at(ResizedCol - 1);
	r.top = 0;
	r.bottom = CliSz.y;
	r.top    += hdr_width;
	r.bottom += hdr_width;
	if(LastResizeColumnPos != -1 || mode == 1) {
		if(mode == 1) {
			int    pos = CellRight(r_c) - 1;
			p.x = LastResizeColumnPos = pos;
			POINT tmp = p;
			::ClientToScreen(H(), &tmp);
			SetCursorPos(tmp.x, tmp.y);
		}
		r.left = r.right = LastResizeColumnPos;
		DrawFocusRect(GetDC(H()), &r);
		if(mode == 0) {
			LastResizeColumnPos = -1;
			if(p.x > 8192)
				p.x = 0;
			if((newSz = max(p.x - r_c.x, 6)) != (int)(ChrSz.x * r_c.width)) {
				newSz /= ChrSz.x;
				SetColumnWidth(ResizedCol - 1, newSz);
				IsUserSettingsChanged = 1;
				{
					const BroColumn & c = P_Def->at(HScrollPos);
					RectCursors.LineCursor.left  = P_Def->at(Freeze ? 0 : Left).x + 1;
					RectCursors.LineCursor.right = CellRight(P_Def->at(Right)) - 1;
					RectCursors.CellCursor.left  = c.x + 1;
					RectCursors.CellCursor.right = CellRight(c) - 1;
					SInflateRect(RectCursors.CellCursor, 3, 0);
					SInflateRect(RectCursors.LineCursor, 3, 0);
				}
				uint   gidx;
				if(P_Def->isColInGroup(ResizedCol - 1, &gidx)) {
					const BroGroup * p_grp = P_Def->GetGroup(gidx);
					gidx = max(p_grp->first, Left);
					r.left = P_Def->at(gidx).x + 1;
				}
				else
					r.left = P_Def->at(ResizedCol - 1).x+1;
				r.top    = ToolBarWidth-1;
				r.bottom = CliSz.y;
				r.right  = CliSz.x;
				InvalidateRect(H(), &r, TRUE);
			}
		}
		else
			LastResizeColumnPos = p.x;
	}
	if(mode == 2) {
		r.left = r.right = p.x;
		DrawFocusRect(GetDC(H()), &r);
	}
	if(mode == 0)
		WMHScroll(SB_VERT, SB_THUMBPOSITION, 0);
}

void BrowserWindow::FocusItem(int hPos, int vPos)
{
	long   v = vPos - P_Def->_curItem();
	long   h = hPos;
	uint   view_height = (P_RowsHeightAry && P_RowsHeightAry->getCount()) ? P_RowsHeightAry->getCount() : ViewHeight;
	if(vPos < (long)(view_height + P_Def->_topItem()) && h <= (long)Right && h < (long)P_Def->getCount()) {
		SendMessage(H(), WM_VSCROLL, MAKELONG(SB_THUMBPOSITION, v), v);
		SendMessage(H(), WM_HSCROLL, MAKELONG(SB_THUMBPOSITION, h), h);
	}
}

int BrowserWindow::IsLastPage(uint viewHeight)
{
	return P_Def && P_Def->IsEOQ() && (!(char *)P_Def->getRow(P_Def->_topItem() + viewHeight)) ? 1 : 0;
}

int BrowserWindow::WMHScrollMult(int sbEvent, int thumbPos, long * pOldTop)
{
	int    res = 0;
	long   old_top;
	int    recalc_heights = 1;
	uint   view_height = (P_RowsHeightAry && P_RowsHeightAry->getCount()) ? P_RowsHeightAry->getCount() : ViewHeight;
	uint   bottom_item = 0, top = 0;
	long   step = 0, next_page_height = 0;
	long   scrll_delta = 0;
	old_top = -1;
	switch(sbEvent) {
		case SB_TOP:
			res = P_Def->top();
			break;
		case SB_BOTTOM:
			res = P_Def->bottom();
			bottom_item = P_Def->_curItem();
			break;
		case SB_LINEUP:
			old_top = P_Def->_topItem();
			if(!old_top && !P_Def->_curItem() && !P_Def->IsBOQ())
				old_top--;
			res = P_Def->step(-1);
			recalc_heights = 0;
			break;
		case SB_LINEDOWN:
			old_top = P_Def->_topItem();
			if((long)(P_Def->_curItem()+1) >= (long)(view_height + old_top) && !P_Def->IsEOQ())
				old_top = -1;
			if(P_Def->_curItem() - P_Def->_topItem() != view_height - 1)
				recalc_heights = 0;
			res = P_Def->step(1);
			if(recalc_heights) {
				if(IsLastPage(view_height)/*P_Def->IsEOQ()*/)
					bottom_item = P_Def->_curItem();
				P_Def->step(-(int)(view_height - 1));
			}
			else
				recalc_heights = 0;
			break;
		case SB_PAGEUP:
			{
				if(P_Def->IsBOQ())
					old_top = P_Def->_topItem();
				uint prev_top = P_Def->_topItem();
				res = P_Def->step(-(int)view_height);
				res = P_Def->step(view_height - 1);
				bottom_item = P_Def->_curItem();
				recalc_heights = 1;
			}
			break;
		case SB_PAGEDOWN:
			if(IsLastPage(view_height)/*P_Def->IsEOQ()*/) {
				old_top = P_Def->_topItem();
				bottom_item = P_Def->_topItem() + view_height - 1;
			}
			res = P_Def->step(view_height);
			P_Def->step(-(int)(view_height - 1));
			recalc_heights = 1;
			break;
		case SB_THUMBPOSITION:
			old_top = P_Def->_topItem();
			res = P_Def->step(thumbPos);
			if(thumbPos == 0) {
				recalc_heights = 1;
				bottom_item = (P_RowsHeightAry && P_RowsHeightAry->getCount() && IsLastPage(view_height)) ?
					(P_Def->_topItem() + view_height - 1) : 0;
			}
			else {
				bottom_item = P_Def->_curItem();
				recalc_heights = 0;
			}
			break;
		default:
			recalc_heights = 0;
	}
	if(!recalc_heights) {
		P_Def->getScrollData(&scrll_delta, (LPLONG) & VScrollPos);
		recalc_heights = scrll_delta ? 1 : 0;
	}
	//
	// calc row hights array
	//
	if(recalc_heights) {
		long   thumb_step = 0;
		CalcRowsHeight(P_Def->_topItem(), bottom_item);
		view_height = P_RowsHeightAry->getCount();
		if(sbEvent == SB_THUMBPOSITION && thumbPos == 0) {
			if((long)view_height < (P_Def->_curItem() - P_Def->_topItem()))
				P_Def->step(-(long)(P_Def->_curItem() - P_Def->_topItem() - view_height + 1));
			else if(P_Def->_curItem() < (long)view_height) {
				thumb_step = -(long)view_height + P_Def->_curItem() - P_Def->_topItem() + 1;
				P_Def->step(-(long)(P_Def->_curItem() - P_Def->_topItem() - view_height + 1));
			}
		}
		P_Def->setViewHight(view_height);
		if(oneof3(sbEvent, SB_PAGEDOWN, SB_LINEDOWN, SB_BOTTOM))
			P_Def->step(view_height - 1);
		else if(sbEvent == SB_PAGEUP)
			P_Def->step(-(int)(view_height - 1));
		else
			P_Def->step(thumb_step);
		old_top = -1;
	}
	ASSIGN_PTR(pOldTop, old_top);
	return res;
}

int BrowserWindow::WMHScroll(int sbType, int sbEvent, int thumbPos)
{
	RECT   r;
	const  long hdr_width = CalcHdrWidth(1);
	int    res = 0;
	long   scrll_delta;
	int    oldHScrollPos;
	int    oldLeft;
	long   old_top;
	uint   cur;
	if(sbType == SB_VERT) {
		int multi_lines = 0;
		old_top = -1;
		for(uint cn = 0; !multi_lines && cn < P_Def->getCount(); cn++)
			multi_lines = BIN(P_Def->at(cn).Options & BCO_RESIZEABLE);
		if(multi_lines)
			res = WMHScrollMult(sbEvent, thumbPos, &old_top);
		else {
			switch(sbEvent) {
				case SB_TOP:    res = P_Def->top();    break;
				case SB_BOTTOM: res = P_Def->bottom(); break;
				case SB_LINEUP:
					old_top = P_Def->_topItem();
					if(!old_top && !P_Def->_curItem() && !P_Def->IsBOQ())
						old_top--;
					res = P_Def->step(-1);
					break;
				case SB_LINEDOWN:
					old_top = P_Def->_topItem();
					if((long)(P_Def->_curItem()+1) >= (long)(ViewHeight + old_top) && !P_Def->IsEOQ())
						old_top = -1;
					res = P_Def->step(1);
					break;
				case SB_PAGEUP:
					if(P_Def->IsBOQ())
						old_top = P_Def->_topItem();
					res = P_Def->step(-(int)ViewHeight);
					break;
				case SB_PAGEDOWN:
					if(P_Def->IsEOQ())
						old_top = P_Def->_topItem();
					res = P_Def->step(ViewHeight);
					break;
				case SB_THUMBPOSITION:
					old_top = P_Def->_topItem();
					res = P_Def->step(thumbPos);
					break;
				default:;
			}
		}
		P_Def->getScrollData(&scrll_delta, (LPLONG) & VScrollPos);
		if(res) {
			// AHTOXA {
			RECT prev_line_rect = RectCursors.LineCursor;
			RECT prev_cell_rect = RectCursors.CellCursor;
			// } AHTOXA
			r.top = CapOffs;
			r.bottom = r.top + ViewHeight * YCell;
			r.top    += hdr_width;
			r.bottom += hdr_width;
			r.left = 0;
			r.right = CliSz.x;
			SendMessage(GetParent(H()), BRO_ROWCHANGED, (WPARAM)H(), MAKELPARAM(VScrollPos, 0));
			ItemRect(HScrollPos, P_Def->_curItem() - P_Def->_topItem(), &RectCursors.CellCursor, TRUE);
			LineRect(P_Def->_curItem() - P_Def->_topItem(), &RectCursors.LineCursor, TRUE);
			AdjustCursorsForHdr();
			if(P_Def->_topItem() != old_top)
				InvalidateRect(H(), &r, TRUE);
			// AHTOXA {
			if(prev_line_rect.top != RectCursors.LineCursor.top) {
				ClearFocusRect(&prev_line_rect);
				InvalidateRect(H(), &RectCursors.LineCursor, FALSE);
			}
			if(prev_cell_rect.top != RectCursors.CellCursor.top || prev_cell_rect.left != RectCursors.CellCursor.left) {
				ClearFocusRect(&prev_cell_rect);
				InvalidateRect(H(), &RectCursors.CellCursor, FALSE);
			}
			::UpdateWindow(H());
			// } AHTOXA
		}
		SetScrollPos(H(), SB_VERT, VScrollPos, TRUE);
	}
	if(sbType == SB_HORZ) {
		oldHScrollPos = HScrollPos;
		switch(sbEvent) {
			case SB_TOP:    HScrollPos = 0; break;
			case SB_BOTTOM: HScrollPos = HScrollMax; break;
			case SB_PAGEUP:
			case SB_LINEUP:
				if(HScrollPos > 0)
					HScrollPos--;
				break;
			case SB_PAGEDOWN:
			case SB_LINEDOWN:
				if(HScrollPos < HScrollMax)
					HScrollPos++;
				break;
			case SB_THUMBPOSITION:
			case SB_THUMBTRACK:
				HScrollPos = thumbPos;
				break;
			default:
				break;
		}
		if(HScrollPos != (UINT)oldHScrollPos) {
			oldLeft = Left;
			cur = HScrollPos;
			if(cur >= Freeze) {
				while(cur > Right)
					Left++, CalcRight();
				if(cur == Right) {
					while(Left < (int)HScrollMax && (CellRight(P_Def->at(cur)) + 5) > CliSz.x)
						Left++, CalcRight();
				}
				if(cur < Left)
					Left = cur, CalcRight();
			}
			SendMessage(GetParent(H()), BRO_ROWCHANGED, (WPARAM)H(), MAKELPARAM(HScrollPos, 0));
			ClearFocusRect(&RectCursors.CellCursor);
			ClearFocusRect(&RectCursors.LineCursor);
			ItemRect(HScrollPos, P_Def->_curItem() - P_Def->_topItem(), &RectCursors.CellCursor, TRUE);
			LineRect(P_Def->_curItem() - P_Def->_topItem(), &RectCursors.LineCursor, TRUE);
			AdjustCursorsForHdr();
			if(Left != oldLeft) {
				// ItemRect(Left, P_Def->_curItem() - P_Def->_topItem(), &r, TRUE);
				r.left   = P_Def->at(Left).x+1;
				r.top    = ToolBarWidth;
				r.bottom = CliSz.y;
				r.right  = CliSz.x;
				::InvalidateRect(H(), &r, TRUE);
			}
			else
				::InvalidateRect(H(), &RectCursors.CellCursor, FALSE);
			SetScrollPos(H(), SB_HORZ, HScrollPos, TRUE);
		}
	}
	return 1;
}

int BrowserWindow::CalcRowsHeight(long topItem, long bottomItem)
{
	int    r = 1;
	if(P_Def) {
		uint   count = P_Def->getCount();
		uint   c = 0;
		if(P_RowsHeightAry)
			P_RowsHeightAry->freeAll();
		else
			P_RowsHeightAry = new SArray(sizeof(RowHeightInfo));
		if(P_RowsHeightAry) {
			uint   row;
			int    stop = 0;
			uint   view_height = 0;
			long   recs_count = P_Def->getRecsCount();
			RowHeightInfo heights_info;
			heights_info.Top = 0;
			for(row = 0; !stop && row < ViewHeight && view_height < ViewHeight; row++) {
				long   bro_row = bottomItem ? (bottomItem - row) : (topItem + row);
				if(bro_row >= 0) {
					heights_info.HeightMult = 1;
					for(uint cn = 0; cn < count; cn++) {
						uint new_row_height = 0;
						P_Def->getMultiLinesText(bro_row, cn, 0, 0, &new_row_height);
						heights_info.HeightMult = (heights_info.HeightMult > new_row_height && new_row_height) ?
							heights_info.HeightMult : new_row_height;
					}
					if(heights_info.HeightMult == 0)
						stop = 1;
					else {
						view_height += heights_info.HeightMult;
						heights_info.HeightMult = (view_height > ViewHeight) ?
							heights_info.HeightMult - (view_height - ViewHeight) : heights_info.HeightMult;
						if(bottomItem)
							P_RowsHeightAry->atInsert(0, &heights_info);
						else
							P_RowsHeightAry->insert(&heights_info);
						heights_info.Top += YCell * heights_info.HeightMult;
					}
				}
				else
					stop = 1;
			}
			if(bottomItem) {
				uint top = 0;
				RowHeightInfo * p_item = 0;
				for(row = 0; P_RowsHeightAry->enumItems(&row, (void**)&p_item) > 0;) {
					p_item->Top = top;
					top += YCell * p_item->HeightMult;
				}
			}
		}
		else
			r = 0;
	}
	return r;
}

static int IsBrowserWindow(HWND hWnd)
{
	SString cls_name;
	TView::SGetWindowClassName(hWnd, cls_name);
	return BIN(cls_name == BrowserWindow::WndClsName || cls_name == STimeChunkBrowser::WndClsName ||
		cls_name == /*STextBrowser::WndClsName*/"STextBrowser" || cls_name == CLASSNAME_DESKTOPWINDOW);
}

HWND FASTCALL GetNextBrowser(HWND hw, int reverse)
{
	if(reverse) {
		for(HWND hb = GetWindow(hw, GW_HWNDLAST); hb; hb = GetWindow(hb, GW_HWNDPREV))
			if(IsBrowserWindow(hb))
				return hb;
	}
	else {
		for(HWND hb = hw; (hb = GetWindow(hb, GW_HWNDNEXT)) != 0;)
			if(hb != hw && IsBrowserWindow(hb))
				return hb;
	}
	return hw;
}

/*
HWND GetNextBrowser(HWND hw)
{
	for(HWND hb = hw; (hb = GetWindow(hb, GW_HWNDNEXT)) != 0;)
		if(hb != hw && IsBrowserWindow(hb))
			return hb;
	return hw;
}

HWND GetPrevBrowser(HWND hw)
{
	for(HWND hb = GetWindow(hw, GW_HWNDLAST); hb; hb = GetWindow(hb, GW_HWNDPREV))
		if(IsBrowserWindow(hb))
			return hb;
	return hw;
}
*/

// static
LRESULT CALLBACK BrowserWindow::BrowserWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static const struct scrollkeys {
		WORD wVirtkey;
		int  iMessage;
		WORD wRequest;
	} key2scroll[] = {
		{ VK_HOME, WM_VSCROLL, SB_TOP },
		{ VK_END, WM_VSCROLL, SB_BOTTOM },
		{ VK_PRIOR, WM_VSCROLL, SB_PAGEUP },
		{ VK_NEXT, WM_VSCROLL, SB_PAGEDOWN },
		{ VK_UP, WM_VSCROLL, SB_LINEUP },
		{ VK_DOWN, WM_VSCROLL, SB_LINEDOWN },
		{ VK_LEFT, WM_HSCROLL, SB_LINEUP },
		{ VK_RIGHT, WM_HSCROLL, SB_LINEDOWN }
	};
#define NUMKEYS (sizeof key2scroll / sizeof key2scroll[0])
	int    i;
	long   hPos, vPos;
	POINT  pnt;
	TPoint tp;
	TEvent e;
	BrowserWindow * p_view = (BrowserWindow *)TView::GetWindowUserData(hWnd);
	long   hdr_width = p_view ? p_view->CalcHdrWidth(1) : 0;
	LPCREATESTRUCT initData;
	switch(msg) {
		case WM_CREATE:
			initData = (LPCREATESTRUCT)lParam;
			if(IsMDIClientWindow(initData->hwndParent)) {
				p_view = (BrowserWindow *)((LPMDICREATESTRUCT)(initData->lpCreateParams))->lParam;
				p_view->BbState |= TBaseBrowserWindow::bbsIsMDI;
			}
			else {
				p_view = (BrowserWindow*)initData->lpCreateParams;
				p_view->BbState &= ~TBaseBrowserWindow::bbsIsMDI;
			}
			if(p_view) {
				p_view->HW = hWnd;
				p_view->WMHCreate((LPCREATESTRUCT)lParam);
				InvalidateRect(hWnd, 0, TRUE);
				PostMessage(hWnd, WM_PAINT, 0, 0);
				{
					//char   cap[256];
					//::GetWindowText(hWnd, cap, sizeof(cap));
					SString temp_buf;
					TView::SGetWindowText(hWnd, temp_buf); // @v9.1.5
					APPL->AddItemToMenu(temp_buf, p_view);
				}
				if(p_view->InitPos > 0)
					p_view->go(p_view->InitPos);
				return 0;
			}
			else
				return -1;
		case WM_DESTROY:
			if(p_view) {
				if(p_view->IsConsistent()) {
					if((p_view->Sf & sfModal) && !(p_view->Sf & sfOnDestroy)) {
						p_view->EndModalCmd = cmCancel;
						PostMessage(hWnd, WM_NULL, 0, 0L);
					}
					else {
						TView::SetWindowUserData(hWnd, (void *)0);
						if(!(p_view->Sf & sfOnDestroy)) {
							delete p_view;
						}
					}
					APPL->NotifyFrame(1);
				}
				else {
					return 0; // @v8.4.11 @debug
				}
			}
			return 0;
		case WM_SETFONT:
			{
				SetFontEvent sfe;
				sfe.FontHandle = wParam;
				sfe.DoRedraw = LOWORD(lParam);
				TView::messageCommand(p_view, cmSetFont, &sfe);
			}
			return 0;
		case BRO_GETCURREC:
			// *(void FAR * FAR *) lParam = p_view->P_Def->getCurrItem();
			return 0;
		case BRO_GETCURCOL:
			if(p_view) {
				*(UINT *)(void FAR * FAR *)lParam = p_view->HScrollPos;
			}
			return 0;
		case BRO_DATACHG:
			if(p_view) {
				p_view->P_Def->refresh();
				InvalidateRect(hWnd, NULL, TRUE);
				UpdateWindow(hWnd);
			}
			return 0;
		case BRO_SETDATA:
			if(p_view) {
				p_view->P_Def->setData((LPVOID) lParam);
				RECT r;
				r.top    = p_view->CapOffs;
				r.bottom = r.top + p_view->ViewHeight * p_view->YCell;
				r.top   += hdr_width;
				r.left   = 0;
				r.right  = p_view->CliSz.x;
				InvalidateRect(hWnd, &r, TRUE);
				UpdateWindow(hWnd);
				SendMessage(GetParent(hWnd), BRO_ROWCHANGED, (WPARAM) hWnd, MAKELPARAM(p_view->HScrollPos, 0));
			}
			return 0;
		case WM_SIZE:
			if(lParam && p_view) {
				int    cell = p_view->YCell;
				int    cap  = p_view->CapOffs;
				p_view->CliSz.Set(LOWORD(lParam), cap + ((HIWORD(lParam) - cap) / cell + 1) * cell);
				lParam = p_view->CliSz.towparam();
				p_view->SetupScroll();
				p_view->CalcRight();
				HWND hw = p_view->P_Toolbar ? p_view->P_Toolbar->H() : 0;
				if(IsWindowVisible(hw)) {
					MoveWindow(hw, 0, 0, LOWORD(lParam), p_view->ToolBarWidth, 0);
					TView::messageCommand(p_view, cmResize);
				}
			}
			break;
		case WM_PAINT:
			if(p_view) {
				p_view->Paint();
				if(p_view->P_Header)
					UpdateWindow(GetWindow(hWnd, GW_CHILD));
			}
			return 0L;
		case WM_HSCROLL:
		case WM_VSCROLL:
			if(hWnd == GetFocus() && p_view) {
				if(p_view->WMHScroll((msg == WM_HSCROLL) ? SB_HORZ : SB_VERT, LOWORD(wParam), (int16)HIWORD(wParam)))
					return 0L;
			}
			else
				SetFocus(hWnd);
			break;
		case WM_MOUSEWHEEL:
			if(p_view) {
				short delta = (short)HIWORD(wParam);
				int   scroll_code = (delta > 0) ? SB_LINEUP : SB_LINEDOWN;
				for(int i = 0; i < 5; i++)
					p_view->WMHScroll(SB_VERT, scroll_code, 0);
			}
			break;
		case WM_MOUSEHOVER:
			tp.setwparam(lParam);
			TView::messageBroadcast(p_view, cmMouseHover, (void *)&tp);
			break;
		case WM_SETFOCUS:
			if(!(TView::GetWindowStyle(hWnd) & WS_CAPTION)) {
				SetWindowPos(hWnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
				APPL->NotifyFrame(0);
			}
			if(p_view) {
				APPL->SelectTabItem(p_view);
				TView::messageBroadcast(p_view, cmReceivedFocus);
				p_view->select();
			}
			break;
		case WM_KILLFOCUS:
			if(!(TView::GetWindowStyle(hWnd) & WS_CAPTION))
				APPL->NotifyFrame(0);
			if(p_view) {
				TView::messageBroadcast(p_view, cmReleasedFocus);
				p_view->ResetOwnerCurrent();
			}
			break;
		case WM_SYSCOMMAND:
			if(wParam == SC_CLOSE) {
				if(p_view) {
					p_view->endModal(cmCancel);
					return 0; // Message is processed
				}
			}
			break;
		case WM_SYSKEYDOWN:
			if(wParam == VK_MENU) {
				PostMessage(APPL->H_MainWnd, WM_SYSKEYDOWN, wParam, lParam);
				return 0;
			}
		case WM_KEYDOWN:
			if(p_view) {
				for(i = 0; i < NUMKEYS; i++) {
					if(wParam == key2scroll[i].wVirtkey) {
						SendMessage(hWnd, key2scroll[i].iMessage, key2scroll[i].wRequest, 0L);
						return 0;
					}
				}
				if(wParam == VK_ESCAPE) {
					if(!(p_view->GetUIConfig()->Flags & UserInterfaceSettings::fDontExitBrowserByEsc)) {
						p_view->endModal(cmCancel);
						return 0;
					}
				}
				else if(wParam == VK_TAB) {
					if(GetKeyState(VK_CONTROL) & 0x8000 && p_view != 0 && !p_view->IsInState(sfModal)) {
						SetFocus(GetNextBrowser(hWnd, (GetKeyState(VK_SHIFT) & 0x8000) ? 0 : 1));
						return 0;
					}
				}
				p_view->HandleKeyboardEvent(LOWORD(wParam));
			}
			return 0;
		case WM_CHAR:
			if(p_view) {
				char kb_scan_code = LOBYTE(HIWORD(lParam));
				if(wParam != VK_RETURN || kb_scan_code != 0x1c) {
					e.what = evKeyDown;
					e.keyDown.keyCode = wParam;
					p_view->handleEvent(e);
				}
			}
			return 0;
		case WM_LBUTTONDBLCLK:
			SendMessage(GetParent(hWnd), BRO_LDBLCLKNOTIFY, (WPARAM)hWnd, lParam);
			TView::messageCommand(p_view, cmaEdit);
			return 0;
		case WM_RBUTTONDBLCLK:
			SendMessage(GetParent(hWnd), BRO_RDBLCLKNOTIFY, (WPARAM) hWnd, lParam);
			return 0;
		case WM_NCLBUTTONDOWN:
			if(hWnd != GetFocus())
				SetFocus(hWnd);
			break;
		case WM_RBUTTONUP:
			if(p_view) {
				tp.setwparam(lParam);
				p_view->ItemByPoint(tp, &hPos, &vPos);
				p_view->FocusItem(hPos, vPos);
				e.what = evKeyDown;
				e.keyDown.keyCode = kbShiftF10;
				p_view->handleEvent(e);
			}
			return 0;
		case WM_LBUTTONDOWN:
			if(p_view) {
				if(hWnd != GetFocus())
					SetFocus(hWnd);
				pnt = tp.setwparam(lParam);
				SetCapture(hWnd);
				if(p_view->ResizedCol)
					p_view->Resize(tp, 1);
				else {
					if(p_view->BbState & TBaseBrowserWindow::bbsIsMDI) {
						DefMDIChildProc(hWnd, msg, wParam, lParam);
						UpdateWindow(hWnd);
					}
					p_view->ItemByPoint(tp, &hPos, &vPos);
					p_view->FocusItem(hPos, vPos);
					if(p_view->SelColByPoint(&pnt, (wParam & MK_CONTROL) ? 1 : ((wParam & MK_SHIFT) ? -1 : 0)) > 0) {
						InvalidateRect(hWnd, 0, TRUE);
						UpdateWindow(hWnd);
					}
				}
			}
			return 0;
		case WM_LBUTTONUP:
			if(p_view) {
				ReleaseCapture();
				if(hWnd == GetFocus() && p_view->ResizedCol) {
					p_view->Resize(tp.setwparam(lParam), 0);
					return 0;
				}
			}
			break;
		case WM_MOUSEMOVE:
			if(p_view) {
				int rc;
				tp.setwparam(lParam);
				if(wParam == 0 && hWnd == GetFocus()) {
					if(p_view->PrevMouseCoord != tp) {
						p_view->RegisterMouseTracking(0, 1000);
						p_view->PrevMouseCoord = tp;
					}
					if((rc = p_view->IsResizePos(tp)) != 0) {
						SetCursor(p_view->ResizeCursor);
						p_view->ResizedCol = rc;
					}
					else if(p_view->ResizedCol) {
						SetCursor(p_view->MainCursor);
						p_view->ResizedCol = 0;
					}
				}
				else if(wParam & MK_LBUTTON && hWnd == GetCapture()) {
					if(p_view->ResizedCol)
						p_view->Resize(tp, 2);
					else {
						if(p_view->BbState & TBaseBrowserWindow::bbsIsMDI) {
							DefMDIChildProc(hWnd, msg, wParam, lParam);
							UpdateWindow(hWnd);
						}
						p_view->ItemByPoint(tp, &hPos, &vPos);
						p_view->FocusItem(hPos, vPos);
					}
				}
			}
			return 0;
		case WM_COMMAND:
			PostMessage(hWnd, WM_USER+101, wParam, lParam);
			return 0;
		case WM_USER+101:
			if(p_view) {
				if(HIWORD(wParam) == 0) {
					e.what = evKeyDown;
					e.keyDown.keyCode = LOWORD(wParam);
					p_view->handleEvent(e);
				}
				else if(HIWORD(wParam) == 1) {
					e.what = evCommand;
					e.message.command = LOWORD(wParam);
					p_view->handleEvent(e);
				}
			}
			return 0;
		case WM_GETMINMAXINFO:
			if(!IsIconic(hWnd)) {
				LPMINMAXINFO p_min_max = (LPMINMAXINFO)lParam;
				RECT rc_client;
				GetClientRect(hWnd, &rc_client);
				p_min_max->ptMinTrackSize.x = 200;
				p_min_max->ptMinTrackSize.y = 150;
				return 0;
			}
			else
				break;
		default:
			break;
	}
	return DefWindowProc(hWnd, msg, wParam, lParam);
}

// static
int BrowserWindow::RegisterClass(HINSTANCE hInst)
{
	WNDCLASSEX wc;
	MEMSZERO(wc);
	wc.cbSize        = sizeof(wc);
	wc.lpszClassName = BrowserWindow::WndClsName;
	wc.hInstance     = hInst;
	wc.lpfnWndProc   = BrowserWindow::BrowserWndProc;
	wc.style         = CS_HREDRAW | CS_VREDRAW | CS_OWNDC | CS_DBLCLKS;
	wc.hIcon         = LoadIcon(hInst, MAKEINTRESOURCE(/*ICON_MAIN_P2*/ 102));
	wc.cbClsExtra    = BRWCLASS_CEXTRA;
	wc.cbWndExtra    = BRWCLASS_WEXTRA;
	return ::RegisterClassEx(&wc); // @unicodeproblem
}

int BrowserWindow::search(void * pPattern, CompFunc fcmp, int srchMode)
{
	if(P_Def && P_Def->search(pPattern, fcmp, srchMode, HScrollPos)) {
		long   scrll_delta, scrll_pos;
		P_Def->getScrollData(&scrll_delta, &scrll_pos);
		VScrollPos = (P_Def->_curItem() - P_Def->_topItem());
		ItemRect(HScrollPos, VScrollPos, &RectCursors.CellCursor, TRUE);
		LineRect(VScrollPos, &RectCursors.LineCursor, TRUE);
		AdjustCursorsForHdr();
		return 1;
	}
	else if(fcmp == SrchFunc) {
		SendMessage(H(), WM_KEYDOWN, VK_END, 0L);
		UserInterfaceSettings ui_cfg;
		ui_cfg.Restore();
		if(!(ui_cfg.Flags & UserInterfaceSettings::fDisableNotFoundWindow)) {
			SMessageWindow * p_win = new SMessageWindow;
			if(p_win) {
				SString msg_buf, fmt_buf;
				SLS.LoadString("strnfound", fmt_buf);
				msg_buf.Printf(fmt_buf, (const char *)pPattern);
				p_win->Open(msg_buf, 0, H(), 0, 5000, GetColorRef(SClrRed), SMessageWindow::fShowOnCenter|SMessageWindow::fChildWindow, 0);
			}
		}
	}
	return 0;
}

void BrowserWindow::search(char * pFirstLetter, int srchMode)
{
	int    r = -1;
	char   srch_buf[512];
	if((srchMode & ~srchFlags) == srchFirst) {
		if(pFirstLetter)
			SearchPattern = pFirstLetter;
		if(UiSearchTextBlock::ExecDialog(H(), Id, SearchPattern, BIN(pFirstLetter), 0, 0) == cmOK) {
			SearchPattern.CopyTo(srch_buf, sizeof(srch_buf));
			if(!TView::messageCommand(this, cmBrwsSrchPreprocess, srch_buf))
				r = search(srch_buf, SrchFunc, (SearchPattern.C(0) == '*') ? srchNext : srchFirst);
		}
	}
	else if((srchMode & ~srchFlags) == srchNext && SearchPattern.NotEmpty()) {
		SearchPattern.CopyTo(srch_buf, sizeof(srch_buf));
		r = search(srch_buf, SrchFunc, srchNext);
	}
	if(r >= 0) {
		RECT   rc;
		rc.top    = CapOffs;
		rc.bottom = rc.top + ViewHeight * YCell;
		long   hdr_width = CalcHdrWidth(1);
		rc.top    += hdr_width - 3;
		rc.bottom += hdr_width + 3;
		rc.left    = 0;
		rc.right   = CliSz.x;
		InvalidateRect(H(), &rc, 1);
	}
}

