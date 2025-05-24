// WBROWSE.CPP
// Copyright (c) Sobolev A. 1994-2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021, 2022, 2023, 2024, 2025
// @codepage UTF-8
// WIN32
//
#include <slib-internal.h>
#pragma hdrstop
#include <scintilla.h>
#include <scilexer.h>
//
//
//
SylkWriter::SylkWriter(const char * pFileName) : Stream(0)
{
	Open(pFileName);
}

SylkWriter::~SylkWriter()
{
	Close();
}

int SylkWriter::Open(const char * pFileName)
{
	Close();
	if(pFileName) {
		Stream = fopen(pFileName, "w");
		return BIN(Stream);
	}
	else
		return 1;
}

void SylkWriter::Close()
{
	SFile::ZClose(&Stream);
	Buf.Z();
}

void FASTCALL SylkWriter::PutLine(const char * pStr)
{
	if(Stream) {
		fputs(pStr, Stream);
		fputc('\n', Stream);
	}
	else
		Buf.Cat(pStr).CR();
}

void SylkWriter::PutRec(const char * pTypeStr, const char * pStr)
{
	if(Stream) {
		fputs(pTypeStr, Stream);
		fputc(';', Stream);
	}
	Buf.Cat(pTypeStr).Semicol();
	PutLine(pStr);
}

void SylkWriter::PutRec(int typeChr, const char * pStr)
{
	char temp_buf[8];
	temp_buf[0] = typeChr;
	temp_buf[1] = 0;
	PutRec(temp_buf, pStr);
}

int SylkWriter::PutVal(const char * pStr, int cvtOemToChr)
{
	char temp_buf[512]; // @v11.7.1 [128]-->[512]
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

int SylkWriter::PutVal(double val)
{
	if(Stream)
		fprintf(Stream, "C;K%lf\n", val);
	Buf.Cat("C;K").Cat(val, SFMT_MONEY).CR();
	return 1;
}

void SylkWriter::PutColumnWidth(int start, int end, int width)
{
	char temp_buf[32];
	sprintf(temp_buf, "W%d %d %d", start, end, width);
	PutRec('F', temp_buf);
}

int SylkWriter::GetBuf(SString * pBuf) const
{
	return pBuf ? ((*pBuf = Buf), 1) : 0;
}

void SylkWriter::PutFormat2(const char * pBuf, int fontId, int col, int row) // @v11.7.10 (to replace PutFormat)
{
	SString temp_buf;
	temp_buf.Cat(pBuf);
	if(fontId) {
		temp_buf.Semicol().Cat("SM").Cat(fontId);
	}
	if(col > 0 && row > 0) {
		temp_buf.Semicol().CatChar('X').Cat(col).Semicol().CatChar('Y').Cat(row);
	}
	else if(col > 0) {
		temp_buf.Semicol().CatChar('C').Cat(col);
	}
	else if(row > 0) {
		temp_buf.Semicol().CatChar('R').Cat(row);
	}
	PutRec('F', temp_buf);
}

void SylkWriter::PutFormat(const char * pBuf, int fontId, int col, int row)
{
	char   temp_buf[128];
	size_t p = 0;
	strnzcpy(temp_buf+p, pBuf, sizeof(temp_buf)-p);
	p += sstrlen(temp_buf+p);
	if(fontId) {
		temp_buf[p++] = ';';
		temp_buf[p++] = 'S';
		temp_buf[p++] = 'M';
		p += sstrlen(itoa(fontId, temp_buf+p, 10));
	}
	if(col > 0 && row > 0) {
		temp_buf[p++] = ';';
		temp_buf[p++] = 'X';
		p += sstrlen(itoa(col, temp_buf+p, 10));
		temp_buf[p++] = ';';
		temp_buf[p++] = 'Y';
		p += sstrlen(itoa(row, temp_buf+p, 10));
	}
	else if(col > 0) {
		temp_buf[p++] = ';';
		temp_buf[p++] = 'C';
		p += sstrlen(itoa(col, temp_buf+p, 10));
	}
	else if(row > 0) {
		temp_buf[p++] = ';';
		temp_buf[p++] = 'R';
		p += sstrlen(itoa(row, temp_buf+p, 10));
	}
	temp_buf[p] = 0;
	PutRec('F', temp_buf);
}

void SylkWriter::PutFont(int symb, const char * pFontName, int size, uint fontStyle)
{
	char   temp_buf[128];
	size_t p = 0;
	temp_buf[p++] = symb;
	strnzcpy(temp_buf+p, pFontName, sizeof(temp_buf)-p);
	p += sstrlen(temp_buf+p);
	if(size) {
		temp_buf[p++] = ';';
		temp_buf[p++] = 'M';
		itoa(size*20, temp_buf+p, 10);
		p += sstrlen(temp_buf+p);
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
	PutRec('P', temp_buf);
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
/*static*/TBaseBrowserWindow * TBaseBrowserWindow::Helper_InitCreation(LPARAM lParam, void ** ppInitData)
{
	TBaseBrowserWindow * p_view = 0;
	LPCREATESTRUCT p_init_data = reinterpret_cast<LPCREATESTRUCT>(lParam);
	if(p_init_data) {
		if(IsMDIClientWindow(p_init_data->hwndParent)) {
			p_view = reinterpret_cast<TBaseBrowserWindow *>(static_cast<LPMDICREATESTRUCT>(p_init_data->lpCreateParams)->lParam);
			p_view->BbState |= TBaseBrowserWindow::bbsIsMDI;
		}
		else {
			p_view = static_cast<TBaseBrowserWindow *>(p_init_data->lpCreateParams);
			p_view->BbState &= ~TBaseBrowserWindow::bbsIsMDI;
		}
	}
	ASSIGN_PTR(ppInitData, p_init_data);
	return p_view;
}

TBaseBrowserWindow::TBaseBrowserWindow(LPCTSTR pWndClsName) :  TWindowBase(pWndClsName, 0), 
	ResourceID(0), ClsName(SUcSwitch(pWndClsName)), ToolbarID(0), BbState(0), ToolBarWidth(0), P_Toolbar(0)
{
	//WoScrollbars = false;
}

TBaseBrowserWindow::~TBaseBrowserWindow()
{
	if(P_Toolbar) {
		P_Toolbar->DestroyHWND();
		ZDELETE(P_Toolbar);
	}
}

/*virtual*/TBaseBrowserWindow::IdentBlock & TBaseBrowserWindow::GetIdentBlock(TBaseBrowserWindow::IdentBlock & rBlk)
{
	rBlk.IdBias = 0;
	rBlk.ClsName.Z();
	rBlk.InstanceIdent.Z();
	return rBlk;
}

int TBaseBrowserWindow::Insert()
{
	int    ret = 0;
	SString buf = getTitle();
	buf.SetIfEmpty(ClsName).Transf(CTRANSF_INNER_TO_OUTER);
	if(IsIconic(APPL->H_MainWnd))
		ShowWindow(APPL->H_MainWnd, SW_MAXIMIZE);
	const HWND hw_active = GetActiveWindow();
	if(hw_active != APPL->H_MainWnd) {
		setState(sfModal, true);
		{
			TEvent event;
			this->handleEvent(event.setCmd(cmExecute, 0));
			ret = (event.what == TEvent::evNothing) ? event.message.infoLong : 0;
		}
		delete this;
	}
	else {
		DWORD  style = WS_CHILD|WS_CLIPSIBLINGS|WS_TABSTOP;
		HWND   hw_frame = APPL->GetFrameWindow();
		if(!(BbState & bbsWoScrollbars))
			style |= (WS_HSCROLL | WS_VSCROLL);
		if(hw_frame) {
			RECT   rc_frame;
			::GetClientRect(hw_frame, &rc_frame);
			// @v11.2.6 WS_EX_COMPOSITED
			HW = ::CreateWindowEx(/*WS_EX_COMPOSITED*/0, SUcSwitch(ClsName), SUcSwitch(buf), style, 0, 0, 
				rc_frame.right-16, rc_frame.bottom, hw_frame, 0, TProgram::GetInst(), this);
			APPL->SizeMainWnd(HW);
			::ShowWindow(HW, SW_SHOW);
			if(ResourceID == 0) {
				HWND   hw = GetTopWindow(hw_frame);
				while(hw && (hw == APPL->H_CloseWnd || hw == HW))
					hw = GetNextWindow(hw, GW_HWNDNEXT);
				if(hw) {
					TBaseBrowserWindow * p_brw = static_cast<TBaseBrowserWindow *>(TView::GetWindowUserData(hw));
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

uint TBaseBrowserWindow::GetResID() const { return ResourceID; }

void TBaseBrowserWindow::SetResID(uint res)
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

IMPL_HANDLE_EVENT(TBaseBrowserWindow)
{
	if(event.isCmd(cmExecute)) {
		ushort last_command = 0;
		SString buf = getTitle();
		buf.Transf(CTRANSF_INNER_TO_OUTER);
		RECT   parent, r;
		long   tree_width = 0;
		HWND   h_main_wnd = APPL->H_MainWnd;
		if(IsIconic(h_main_wnd))
			ShowWindow(h_main_wnd, SW_MAXIMIZE);
		APPL->GetStatusBarRect(&r);
		long   status_height = r.bottom - r.top;
		GetWindowRect(h_main_wnd, &parent);
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
			r.left   = par_wd * ViewOrigin.x / 80 + tree_width;
			r.right  = par_wd * ViewSize.x / 80 - border_x * 2 - tree_width;
			r.top    = par_ht * ViewOrigin.y / 23;
			r.bottom = par_ht * ViewSize.y / 23 - border_y * 2 - cap_y - menu_y - status_height;
			r.left += border_x + parent.left;
			r.top  += border_y + cap_y + menu_y + parent.top;
		}
		const DWORD  style = WS_POPUP|WS_CAPTION|WS_HSCROLL|WS_VSCROLL|WS_SYSMENU|WS_THICKFRAME|WS_VISIBLE|WS_CLIPSIBLINGS;
		if(!H() && h_main_wnd) {
			const TCHAR * p_title = SUcSwitch(buf);
			if(IsMDIClientWindow(h_main_wnd)) {
				MDICREATESTRUCT child;
				child.szClass = BrowserWindow::WndClsName;
				child.szTitle = p_title;
				child.hOwner = TProgram::GetInst();
				child.x  = CW_USEDEFAULT;
				child.y  = CW_USEDEFAULT;
				child.cx = CW_USEDEFAULT;
				child.cy = CW_USEDEFAULT;
				child.style  = style;
				child.lParam = reinterpret_cast<LPARAM>(static_cast<BrowserWindow *>(this));
				HW = reinterpret_cast<HWND>(LOWORD(::SendMessage(h_main_wnd, WM_MDICREATE, 0, reinterpret_cast<LPARAM>(&child))));
			}
			else {
				// @v11.2.6 WS_EX_COMPOSITED
				HW = ::CreateWindowEx(/*WS_EX_COMPOSITED*/0, SUcSwitch(ClsName), p_title, style, r.left, r.top, r.right, r.bottom, (APPL->H_TopOfStack), NULL, TProgram::GetInst(), this);
			}
			if(HW) {
				TEvent event;
				this->handleEvent(event.setCmd(cmModalPostCreate, this)); // @recursion
			}
		}
		if(BbState & bbsCancel) {
			clearEvent(event);
			event.message.infoLong = cmCancel;
		}
		else {
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
	}
	else {
		if(!event.isCmd(cmSetBounds)) { // Для TBaseBrowserWindow команда cmSetBounds пока не должна обрабатываться поскольку координатная сетка унаследована от MS-DOS
			TWindowBase::handleEvent(event); 
		}
	}
}
//
// BrowseWindow
//
const BrowserColorsSchema BrwColorsSchemas[NUMBRWCOLORSCHEMA] = {
	/*
	COLORREF Title;
	COLORREF TitleDelim;
	COLORREF Background;
	COLORREF Text;
	COLORREF Cursor;
	COLORREF CursorOverText;
	COLORREF LineCursor;
	COLORREF LineCursorOverText;
	COLORREF GridHorizontal;
	COLORREF GridVertical;
	*/
	{1, 
		RGB(0xD6, 0xD3, 0xCE), 
		RGB(0x90, 0x90, 0x90), 
		RGB(0xFF, 0xFF, 0xFF), 
		RGB(0x00, 0x00, 0x00),
		RGB(0x80, 0x80, 0x80), 
		RGB(0xFF, 0xFF, 0xFF), 
		RGB(0xC0, 0xC0, 0xC0), 
		RGB(0x00, 0x00, 0x00),
		RGB(0xC0, 0xC0, 0xC0), 
		RGB(0x90, 0x90, 0x90)
	},
	{2, 
		RGB(0xD6, 0xD3, 0xCE), 
		RGB(0x90, 0x90, 0x90), 
		RGB(0xFF, 0xFB, 0xF0), 
		RGB(0x00, 0x00, 0x00),
		RGB(0x29, 0x69, 0x9C), 
		RGB(0xFF, 0xFF, 0xFF), 
		RGB(0xC0, 0xC0, 0xC0), 
		RGB(0x00, 0x00, 0x00),
		RGB(0xB5, 0xCB, 0xC6), 
		RGB(0xFF, 0xB6, 0xE7)
	},
	{3, 
		RGB(0xDE, 0xD7, 0xB5), 
		RGB(0xC6, 0xBA, 0x9C), 
		RGB(0xD6, 0xE3, 0xCE), 
		RGB(0x00, 0x00, 0x00),
		RGB(0xC6, 0xA7, 0x86), 
		RGB(0xFF, 0xFF, 0xFF), 
		RGB(0xC6, 0xBA, 0x9C), 
		RGB(0x00, 0x00, 0x00),
		RGB(0xC6, 0xBA, 0x9C), 
		RGB(0xC6, 0xBA, 0x9C)
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

/*static*/LPCTSTR BrowserWindow::WndClsName = _T("SBROWSER");
//
//
//
int ImpLoadToolbar(TVRez & rez, ToolbarList * pList)
{
	pList->setBitmap(rez.getUINT());
	SString temp_buf;
	while(rez.getUINT() != TV_END) {
		//fseek(rez.getStream(), -static_cast<long>(sizeof(uint16)), SEEK_CUR);
		rez.Seek(-static_cast<long>(sizeof(uint16)), SEEK_CUR);
		ToolbarItem item;
		item.Cmd = rez.getUINT();
		if(item.Cmd != TV_MENUSEPARATOR) {
			item.KeyCode = rez.getUINT();
			item.Flags = rez.getUINT();
			item.BitmapIndex = rez.getUINT();
			rez.getString(temp_buf, 0);
            SLS.ExpandString(temp_buf, CTRANSF_UTF8_TO_OUTER);
            STRNSCPY(item.ToolTipText, temp_buf);
		}
		else
			item.KeyCode = static_cast<ushort>(item.Cmd);
		pList->addItem(&item);
	}
	return 1;
}

BrowserWindow::CellStyle::CellStyle() : Color(0), Color2(0), RightFigColor(0), Flags(0)
{
}

int FASTCALL BrowserWindow::CellStyle::SetFullCellColor(COLORREF c) // returns strictly 1
{
	Color = c;
	return 1; // @necessarily
}

int FASTCALL BrowserWindow::CellStyle::SetRightFigCircleColor(COLORREF c) // returns strictly 1
{
	RightFigColor = c;
	Flags |= fRightFigCircle;
	return 1; // @necessarily
}

int FASTCALL BrowserWindow::CellStyle::SetRightFigTriangleColor(COLORREF c) // returns strictly 1
{
	RightFigColor = c;
	Flags |= fRightFigTriangle;
	return 1; // @necessarily
}

int FASTCALL BrowserWindow::CellStyle::SetLeftBottomCornerColor(COLORREF c) // returns strictly 1
{
	Color2 = c;
	Flags |= fLeftBottomCorner;
	return 1; // @necessarily
}

int FASTCALL BrowserWindow::CellStyle::SetLeftTopCornerColor(COLORREF c) // returns strictly 1
{
	Color = c;
	Flags |= fCorner;
	return 1; // @necessarily
}

int LoadToolbar(TVRez * rez, uint tbType, uint tbID, ToolbarList * pList)
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
	SString spec_buf;
	char   param[32];
	WinRegKey reg_key(HKEY_CURRENT_USER, WrSubKey_BrwUserSetting, 1);
	ltoa(RezID, param, 10);
	if(reg_key.GetString(param, spec_buf)) {
		// version, num_columns, column1_size, ...
		char   ver[32];
		SString temp_buf;
		SString org_offs_buf;
		SString len_buf;
		uint   pos = 0;
		//spec[sizeof(spec)-1] = 0;
		StringSet ss(';', spec_buf);
		ss.get(&pos, ver, sizeof(ver));
		ss.get(&pos, temp_buf);
		const uint num_cols = (uint)temp_buf.ToLong();
		BrowserDef * p_def_ = P_Def;
		while(ss.get(&pos, temp_buf)) {
			if(temp_buf.Divide(',', org_offs_buf, len_buf) > 0) {
				const long org_offs = org_offs_buf.ToLong();
				const long len = len_buf.ToLong();
				for(uint i = 0; i < p_def_->getCount(); i++) {
					const BroColumn & r_col = p_def_->at(i);
					long _p = (r_col.OrgOffs == 0xffff) ? r_col.Offs : r_col.OrgOffs;
					if(_p == org_offs) {
						if(len > 0 && len < 0x0fffL) {
							SetColumnWidth(i, len);
							p_def_->at(i).Options |= BCO_SIZESET;
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
			rez.getString(temp_buf.Z(), 2);
			SLS.ExpandString(temp_buf, CTRANSF_UTF8_TO_INNER);
			setOrgTitle(temp_buf);
			uint   options = rez.getUINT();
			options = NZOR(uOptions, options);
			uint   help_ctx = rez.getUINT();
			BrowserDef * p_def;
			if(dataKind == 1)
				p_def = new AryBrowserDef(static_cast<SArray *>(pData), 0, hight, options);
			else
				p_def = new DBQBrowserDef(*static_cast<DBQuery *>(pData), hight, options);
			THROW(p_def);
			for(int __done = 0; !__done;) {
				const uint tag = rez.getUINT();
				switch(tag) {
					case TV_BROCOLUMN:
						{
							char   fld_name[64];
							rez.getString(temp_buf, 2);
							SLS.ExpandString(temp_buf, CTRANSF_UTF8_TO_INNER);
							uint   fld = rez.getUINT();
							if(fld == 0xffff)
								fld = UNDEF;
							uint   opt = rez.getUINT();
							TYPEID type = rez.getType(0);
							long   fmt  = rez.getFormat(0);
							rez.getString(fld_name, 2);
							const uint t = satoi(fld_name);
							if(t == -1 || fld == t || *fld_name == '0')
								p_def->insertColumn(UNDEF, temp_buf, fld, type, fmt, opt);
							else
								p_def->insertColumn(UNDEF, temp_buf, fld_name, type, fmt, opt);
						}
						columns_count++;
						break;
					case TV_BROGROUP:
						{
							rez.getString(temp_buf, 2);
							SLS.ExpandString(temp_buf, CTRANSF_UTF8_TO_INNER);
							grp.P_Text  = newStr(temp_buf);
							grp.Height = rez.getUINT();
							grp.First = columns_count;
							is_group = 1;
						}
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
							rez.Seek(sav_pos, SEEK_SET);
						}
						break;
					case TV_CROSSTAB:
						{
							BroCrosstab ct;
							MEMSZERO(ct);
							rez.getString(temp_buf, 2);
							SLS.ExpandString(temp_buf, CTRANSF_UTF8_TO_INNER);
							ct.P_Text = newStr(temp_buf);
							ct.Options = rez.getUINT();
							ct.Type    = rez.getType(0);
							ct.Format  = rez.getFormat(0);
							p_def->AddCrosstab(&ct);
						}
						break;
					case TV_END:
						if(is_group) {
							grp.Count = columns_count - grp.First;
							p_def->AddColumnGroup(&grp);
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
			/*// @v11.0.0*/ changeBounds(_bounds);
			P_Def = p_def;
			if(freeze)
				SetFreeze(freeze);
		}
	}
	CATCHZOK
	return 1;
}

DECL_CMPFUNC(_PcharNoCase);

void BrowserWindow::go(long p)
{
	SETMAX(p, 0);
	WMHScroll(SB_VERT, SB_THUMBPOSITION, p-P_Def->_curItem());
	Refresh();
}

void BrowserWindow::top()
{
	WMHScroll(SB_VERT, SB_TOP, 0);
	Refresh();
}

void BrowserWindow::bottom()
{
	WMHScroll(SB_VERT, SB_BOTTOM, 0);
	Refresh();
}

int BrowserWindow::search2(const void * pSrchData, CompFunc cmpFunc, int srchMode, size_t offs)
{
	int    ok = 0;
	long   hdr_width = CalcHdrWidth(1);
	if(P_Def && P_Def->search2(pSrchData, cmpFunc, srchMode, offs) > 0) {
		long scrollDelta, scrollPos;
		P_Def->getScrollData(&scrollDelta, &scrollPos);
		VScrollPos = P_Def->_curFrameItem();
		ItemRect(HScrollPos, VScrollPos, &RectCursors.CellCursor, TRUE);
		LineRect(VScrollPos, &RectCursors.LineCursor, TRUE);
		AdjustCursorsForHdr();
		TRect inv_rec;
		invalidateRect(inv_rec.setwidthrel(0, CliSz.x).setheightrel(CapOffs + hdr_width, ViewHeight * YCell).setmarginy(-3), true);
		ok = 1;
	}
	else
		::PostMessage(H(), WM_KEYDOWN, VK_END, 0L);
	return ok;
}

void BrowserWindow::setRange(ushort)
{
}

void BrowserWindow::__Init(/*BrowserDef * pDef*/)
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
	if(H())
		invalidateAll(true);
}

void BrowserWindow::SetCellStyleFunc(CellStyleFunc func, void * extraPtr)
{
	F_CellStyle = func;
	CellStyleFuncExtraPtr = extraPtr;
}

const  void * BrowserWindow::getItemByPos(long pos) { return P_Def ? P_Def->getRow(pos) : 0; }
const  void * BrowserWindow::getCurItem() { return P_Def ? P_Def->getRow(P_Def->_curItem()) : 0; }
const  UserInterfaceSettings * BrowserWindow::GetUIConfig() const { return &UICfg; }
long   BrowserWindow::CalcHdrWidth(int plusToolbar) const { return (P_Header ? (P_Header->ViewSize.y * ChrSz.y) : 0) + (plusToolbar ? ToolBarWidth : 0); }
int    BrowserWindow::GetCurColumn() const { return (int)HScrollPos; }
void   BrowserWindow::SetCurColumn(int col) { HScrollPos = (uint)col; }
void   BrowserWindow::setInitPos(long p) { InitPos = p; }
BrowserDef * BrowserWindow::getDef() { return P_Def; }
const  BrowserDef * BrowserWindow::getDefC() const { return P_Def; }
void   BrowserWindow::SetDefUserProc(SBrowserDataProc proc, void * extraPtr) { CALLPTRMEMB(P_Def, SetUserProc(proc, extraPtr)); }
int    BrowserWindow::SetColumnTitle(int colNo, const char * pText) { return P_Def ? P_Def->setColumnTitle(colNo, pText) : 0; }
int    BrowserWindow::IsLastPage(uint viewHeight) { return P_Def && P_Def->IsEOQ() && (!P_Def->getRow(P_Def->_topItem() + viewHeight)) ? 1 : 0; }
int    FASTCALL BrowserWindow::CellRight(const BroColumn & rC) const { return (ChrSz.x*rC.width + rC.x); }
int    FASTCALL BrowserWindow::GetRowHeightMult(long row) const { return (P_RowsHeightAry && row < P_RowsHeightAry->getCountI()) ? static_cast<const RowHeightInfo *>(P_RowsHeightAry->at(row))->HeightMult : 1; }
int    FASTCALL BrowserWindow::GetRowTop(long row) const { return (P_RowsHeightAry && row < P_RowsHeightAry->getCountI()) ? static_cast<const RowHeightInfo *>(P_RowsHeightAry->at(row))->Top : (YCell * row); }

void BrowserWindow::AdjustCursorsForHdr()
{
	const long hdr_width = CalcHdrWidth(1);
	RectCursors.CellCursor.top    += hdr_width;
	RectCursors.CellCursor.bottom += hdr_width;
	RectCursors.LineCursor.top    += hdr_width;
	RectCursors.LineCursor.bottom += hdr_width;
}

void BrowserWindow::Insert_(TView *p)
{
	TViewGroup::Insert_(p);
	P_Header = p;
	P_Def->setViewHight((CliSz.y - CapOffs) / YCell - p->ViewSize.y - 1);
}

BrowserWindow::BrowserWindow(uint _rezID, DBQuery * pQuery, uint broDefOptions /*=0*/) :
	TBaseBrowserWindow(BrowserWindow::WndClsName), P_RowsHeightAry(0), P_Header(0), Font(0), DefFont(0), 
	RezID(_rezID), MainCursor(LoadCursor(NULL, IDC_ARROW)), ResizeCursor(LoadCursor(NULL, IDC_SIZEWE))
{
	__Init();
	ResourceID = _rezID;
	LoadResource(_rezID, pQuery, 2, broDefOptions);
	ViewOptions |= ofSelectable;
	HelpCtx = _rezID; // @Muxa
}

BrowserWindow::BrowserWindow(uint _rezID, SArray * pAry, uint broDefOptions /*=0*/) :
	TBaseBrowserWindow(BrowserWindow::WndClsName), P_RowsHeightAry(0), P_Header(0), Font(0), DefFont(0), 
	RezID(_rezID), MainCursor(LoadCursor(NULL, IDC_ARROW)), ResizeCursor(LoadCursor(NULL, IDC_SIZEWE))
{
	__Init();
	ResourceID = _rezID;
	LoadResource(_rezID, pAry, 1, broDefOptions);
	ViewOptions |= ofSelectable;
	HelpCtx = _rezID; // @Muxa
}

/*virtual*/TBaseBrowserWindow::IdentBlock & BrowserWindow::GetIdentBlock(TBaseBrowserWindow::IdentBlock & rBlk)
{
	rBlk.IdBias = IdBiasBrowser;
	rBlk.ClsName = SUcSwitch(BrowserWindow::WndClsName);
	rBlk.InstanceIdent.Z().Cat(GetResID());
	return rBlk;
}

int BrowserWindow::ChangeResource(uint resID, DBQuery * pQuery, int force)
{
	int    ok = 1;
	if(force || resID != RezID) {
		SaveUserSettings(1);
		ZDELETE(P_Def);
		if(P_Toolbar) {
			P_Toolbar->DestroyHWND();
			ZDELETE(P_Toolbar);
		}
		setupToolbar(0);
		SearchPattern.Z();
		__Init();
		RezID = ResourceID = resID;
		P_Header = 0;
		LoadResource(resID, pQuery, 2, 0);
		ViewOptions |= ofSelectable;
		WMHCreate();
		ok = 2;
	}
	else
		static_cast<DBQBrowserDef *>(getDef())->setQuery(*pQuery);
	APPL->redraw();
	return ok;
}

int BrowserWindow::ChangeResource(uint resID, SArray * pAry, int force)
{
	int    ok = 1;
	if(force || resID != RezID) {
		SaveUserSettings(1);
		ZDELETE(P_Def);
		if(P_Toolbar) {
			P_Toolbar->DestroyHWND();
			ZDELETE(P_Toolbar);
		}
		setupToolbar(0);
		SearchPattern.Z();
		__Init();
		RezID = ResourceID = resID;
		P_Header = 0;
		LoadResource(resID, pAry, 1, 0);
		ViewOptions |= ofSelectable;
		WMHCreate();
		ok = 2;
	}
	else
		static_cast<AryBrowserDef *>(getDef())->setArray(pAry, 0, 1);
	return ok;
}

BrowserWindow::~BrowserWindow()
{
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
		BrowserWindow * p_this_view_from_wnd = static_cast<BrowserWindow *>(TView::GetWindowUserData(H()));
		if(p_this_view_from_wnd) {
			Sf |= sfOnDestroy;
			::DestroyWindow(H());
			HW = 0;
		}
	}
}

int BrowserWindow::CopyToClipboard()
{
	int    ok = -1;
	const  uint  cn_count = SelectedColumns.getCount();
	BrowserDef * p_def_ = P_Def;
	if(p_def_) {
		SString val_buf;
		if(cn_count) {
			uint   i, j;
    		long   row = 0;
			LongArray col_types;
			const char * p_fontface_tnr = "Times New Roman";
			LAssocArray width_ary;
			SString out_buf;
			SString dec;
			SylkWriter sw(0);
			sw.PutRec("ID", "PPapyrus");
			{
				char   buf[64];
				::GetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_SDECIMAL, buf, sizeof(buf));
				dec.Cat(buf);
			}
			for(j = 0; j < cn_count; j++) {
				long cn = SelectedColumns.at(j);
				if(cn >= 0 && cn < p_def_->getCountI())
					col_types.add((long)GETSTYPE(p_def_->at(cn).T));
			}
			sw.PutFont('F', p_fontface_tnr, 10, slkfsBold);
			sw.PutFont('F', p_fontface_tnr, 8,  0);
			sw.PutRec('F', "G");
			//
			// Выводим название групп столбцов
			//
			for(i = 0; i < p_def_->GetGroupCount(); i++) {
				uint   pos = 0;
				const  BroGroup * p_grp = p_def_->GetGroup(i);
				val_buf = p_grp->P_Text;
				if(SelectedColumns.lsearch(p_grp->First, &pos)) {
					sw.PutFormat("FC0L", 1, pos + 1, 1);
					sw.PutFont('F', p_fontface_tnr, 10, slkfsBold);
					sw.PutVal(val_buf, 1);
				}
			}
			if(p_def_->GetGroupCount())
				row++;
			//
			// Выводим название столбцов
			//
			for(i = 0; i < cn_count; i++) {
				uint col_num = SelectedColumns.at(i);
				if(col_num < p_def_->getCount()) {
					const BroColumn & r_c = p_def_->at(col_num);
					const long type = GETSTYPE(r_c.T);
					val_buf = r_c.text;
					sw.PutFormat("FC0L", 1, i + 1, row + 1);
					sw.PutVal(val_buf.cptr(), 1);
					width_ary.Add(col_num, (long)val_buf.Len(), 0);
				}
			}
			row += 1;
			p_def_->top();
			do {
				for(j = 0; j < cn_count; j++) {
					long cn = SelectedColumns.at(j);
					if(cn >= 0 && cn < p_def_->getCountI()) {
						long  len = 0;
						uint  stype = col_types.at(j);
						p_def_->getFullText(p_def_->_curItem(), cn, val_buf);
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
							sw.PutVal(val_buf.cptr(), 1);
						}
						if(width_ary.BSearch(cn, &len, 0) && len < (long)val_buf.Len())
							width_ary.Update(cn, (long)val_buf.Len(), 1);
					}
				}
				row++;
			} while(p_def_->step(1) > 0);
			for(i = 0; i < width_ary.getCount(); i++) {
				LAssoc item = width_ary.at(i);
				sw.PutColumnWidth(i + 1, i + 1, item.Val + 2);
			}
			WMHScroll(SB_VERT, SB_BOTTOM, 0);
			sw.PutLine("E");
			sw.GetBuf(&out_buf);
			SClipboard::Copy_SYLK(out_buf);
			ok = 1;
		}
		// @v11.1.12 {
		else {
			p_def_->getFullText(p_def_->_curItem(), GetCurColumn(), val_buf);
			SStringU val_buf_u;
			val_buf_u.CopyFromMb_INNER(val_buf, val_buf.Len());
			SClipboard::Copy_TextUnicode(val_buf_u, val_buf_u.Len());
		}
		// } @v11.1.12 
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
					SlExtraProcBlock epb;
					SLS.GetExtraProcBlock(&epb);
					if(epb.F_CallCalc)
						epb.F_CallCalc(H(), 0);
				}
				break;
			case cmGetFocusedNumber:
				{
					double * p_number = static_cast<double *>(event.message.infoPtr);
					if(p_number) {
						char b[1024];
						P_Def->getText(P_Def->_curItem(), GetCurColumn(), b);
						strtodoub(b, p_number);
					}
				}
				break;
			case cmGetFocusedText:
				{
					char * p_text = static_cast<char *>(event.message.infoPtr);
					if(p_text)
						P_Def->getText(P_Def->_curItem(), GetCurColumn(), p_text);
				}
				break;
			case cmaDesktop:
				if(APPL->H_Desktop && !IsInState(sfModal)) {
					TWindow * p_desk = static_cast<TWindow *>(TView::GetWindowUserData(APPL->H_Desktop));
					::SendMessage(APPL->H_MainWnd, WM_COMMAND, reinterpret_cast<WPARAM>(p_desk), 0);
				}
				break;
			case cmSetFont:
				{
					const SetFontEvent * p_sfe = static_cast<const SetFontEvent *>(event.message.infoPtr);
					if(p_sfe) {
						TEXTMETRIC tm;
						HDC    dc = GetDC(H());
						Font = p_sfe->FontHandle ? p_sfe->FontHandle : GetStockObject(DEFAULT_GUI_FONT);
						::DeleteObject(SelectObject(dc, Font));
						::GetTextMetrics(dc, &tm);
						ChrSz.Set(tm.tmAveCharWidth, tm.tmHeight + tm.tmExternalLeading);
						YCell = ChrSz.y + 2;
						CalcRight();
						ItemRect(0, 0, &RectCursors.CellCursor, TRUE);
						LineRect(0, &RectCursors.LineCursor, TRUE);
						SetupScroll();
						if(p_sfe->DoRedraw) {
							invalidateAll(true);
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
				uchar b[4];
				b[0] = TVCHR;
				b[1] = 0;
				SCharToOem(reinterpret_cast<char *>(b));
				if(isalnum(b[0]) || IsLetter866(b[0]) || b[0] == '*') {
					search(reinterpret_cast<const char *>(b), srchFirst);
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
	BrowserDef * p_def_ = P_Def;
	if(p_def_) {
		uint   cnt = p_def_->getCount();
		uint   i = 0;
		int    x = 3;
		BroColumn * c;
		(c = &p_def_->at(0))->x = x;
		while(i < Freeze && (x += (ChrSz.x*c->width + 3)) < CliSz.x)
			(c = &p_def_->at(++i))->x = x;
		(c = &p_def_->at(i = Left))->x = x;
		while(++i < cnt && (x += (ChrSz.x*c->width + 3)) < CliSz.x)
			(c = &p_def_->at(i))->x = x;
		Right = i - 1;
	}
}

void BrowserWindow::SetupScroll()
{
	{
		const long hdr_width = CalcHdrWidth(1);
		if((CapOffs + hdr_width) < CliSz.y && YCell > 0)
			ViewHeight = ((CliSz.y - CapOffs - hdr_width) / YCell) - 1;
		else
			ViewHeight = 0;
	}
	CALLPTRMEMB(P_Def, setViewHight(ViewHeight));
	VScrollMax = P_Def ? smax(0L, P_Def->GetRecsCount()-1) : 0;
	VScrollPos = MIN(VScrollPos, VScrollMax);
	HScrollMax = SVectorBase::GetCount(P_Def) ? (P_Def->getCount()-1) : 0;
	HScrollPos = MIN(HScrollPos, HScrollMax);
	SetScrollRange(H(), SB_VERT, 0, VScrollMax, FALSE);
	SetScrollRange(H(), SB_HORZ, 0, HScrollMax, FALSE);
	SetScrollPos(H(), SB_VERT, VScrollPos, TRUE);
	SetScrollPos(H(), SB_HORZ, HScrollPos, TRUE);
	HWND   parent = GetParent(H());
	if(parent) {
		::SendMessage(parent, BRO_ROWCHANGED, reinterpret_cast<WPARAM>(H()), MAKELPARAM(VScrollPos, 0));
		::SendMessage(parent, BRO_COLCHANGED, reinterpret_cast<WPARAM>(H()), MAKELPARAM(HScrollPos, 0));
	}
	::SendMessage(H(), WM_VSCROLL, SB_THUMBPOSITION, 0);
}

void BrowserWindow::SetColorsSchema(uint32 schemaNum)
{
	HDC    dc = GetDC(H());
	Pens.Destroy();
	Brushes.Destroy();
	schemaNum = (schemaNum < NUMBRWCOLORSCHEMA) ? schemaNum : 0;
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

void BrowserWindow::EvaluateSomeMetricsOnInit()
{
	HDC  dc = GetDC(H());
	if(DefFont)
		::SelectObject(dc, DefFont);
	if(Pens.DefPen)
		::SelectObject(dc, Pens.DefPen);
	if(Brushes.DefBrush)
		::SelectObject(dc, Brushes.DefBrush);
	ZDeleteWinGdiObject(&Font);
	Pens.DefPen = static_cast<HPEN>(GetCurrentObject(dc, OBJ_PEN));
	DefFont     = static_cast<HFONT>(GetCurrentObject(dc, OBJ_FONT));
	Brushes.DefBrush = static_cast<HBRUSH>(GetCurrentObject(dc, OBJ_BRUSH));
	UserInterfaceSettings ui_cfg;
	if(ui_cfg.Restore() > 0)
		UICfg = ui_cfg;
	if(labs(UICfg.TableFont.Size) > 0 && UICfg.TableFont.Face.NotEmpty())
		Font = TView::CreateFont_(UICfg.TableFont);
	else
		Font = static_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
	::SelectObject(dc, Font);
	{
		TEXTMETRICW txtm;
		GetTextMetricsW(dc, &txtm);
		ChrSz.Set(txtm.tmAveCharWidth, txtm.tmHeight + txtm.tmExternalLeading);
	}
	YCell = ChrSz.y + 2;
	P_Def->VerifyCapHeight();
	CapOffs = YCell * P_Def->GetCapHeight() + 4;
	SetColorsSchema(UICfg.GetBrwColorSchema());
}

void BrowserWindow::WMHCreate()
{
	/* @v12.2.2
	HDC  dc = GetDC(H());
	if(DefFont)
		::SelectObject(dc, DefFont);
	if(Pens.DefPen)
		::SelectObject(dc, Pens.DefPen);
	if(Brushes.DefBrush)
		::SelectObject(dc, Brushes.DefBrush);
	ZDeleteWinGdiObject(&Font);
	Pens.DefPen = static_cast<HPEN>(GetCurrentObject(dc, OBJ_PEN));
	DefFont     = static_cast<HFONT>(GetCurrentObject(dc, OBJ_FONT));
	Brushes.DefBrush = static_cast<HBRUSH>(GetCurrentObject(dc, OBJ_BRUSH));
	UserInterfaceSettings ui_cfg;
	if(ui_cfg.Restore() > 0)
		UICfg = ui_cfg;
	if(labs(UICfg.TableFont.Size) > 0 && UICfg.TableFont.Face.NotEmpty())
		Font = TView::CreateFont_(UICfg.TableFont);
	else
		Font = static_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
	::SelectObject(dc, Font);
	{
		TEXTMETRICW txtm;
		GetTextMetricsW(dc, &txtm);
		ChrSz.Set(txtm.tmAveCharWidth, txtm.tmHeight + txtm.tmExternalLeading);
	}
	YCell = ChrSz.y + 2;
	P_Def->VerifyCapHeight();
	CapOffs = YCell * P_Def->GetCapHeight() + 4;
	SetColorsSchema(UICfg.GetBrwColorSchema());
	*/
	EvaluateSomeMetricsOnInit(); // @v12.2.2
	TView::SetWindowUserData(H(), static_cast<BrowserWindow *>(this));
	SetCursor(MainCursor);
	SetFocus(H());
	SendMessageW(H(), WM_NCACTIVATE, TRUE, 0L);
	{
		RECT client_rect;
		GetClientRect(H(), &client_rect);
		CliSz.Set(client_rect.right, client_rect.bottom);
	}
	RestoreUserSettings();
	SetupColumnsWith();
	if(ToolbarL.getItemsCount()) {
		P_Toolbar = new TToolbar(H(), TBS_NOMOVE);
		P_Toolbar->Init(ToolbarID, &ToolbarL);
		if(P_Toolbar->IsValid()) {
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

void BrowserWindow::SetupColumnsWith()
{
	if(P_Def) {
		for(uint i = 0; i < P_Def->getCount(); i++) {
			BroColumn & r_c = P_Def->at(i);
			if(!(r_c.Options & BCO_SIZESET)) {
				const int cw = r_c.width;
				const TYPEID ct = r_c.T;
				int    w;
				if(GETSTYPE(ct) == S_ZSTRING)
					w = cw * 2 + 1;
				else if(GETSTYPE(ct) == S_DATE)
					w = 10;
				else
					w = static_cast<int>(cw * 6 / 5 + 1);
				r_c.width = MAX(6, w);
				CalcRight();
				SETSFMTLEN(r_c.format, r_c.width);
				r_c.Options |= BCO_SIZESET;
			}
		}
	}
}

void BrowserWindow::SetColumnWidth(int colNo, int newWidth)
{
	if(colNo >= 0 && colNo < static_cast<int>(P_Def->getCount())) {
		BroColumn & c = P_Def->at(colNo);
		c.width = MAX(6, newWidth);
		CalcRight();
		SETSFMTLEN(c.format, c.width);
	}
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
		if(atPos <= static_cast<int>(Left)) {
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

LPRECT BrowserWindow::ItemRect(int hPos, int vPos, LPRECT rect, BOOL isFocus) const
{
	const BroColumn & c = P_Def->at(hPos);
	if(P_RowsHeightAry && vPos < static_cast<long>(P_RowsHeightAry->getCount())) {
		rect->top    = CapOffs + static_cast<const RowHeightInfo *>(P_RowsHeightAry->at(vPos))->Top;
		rect->bottom = rect->top + ChrSz.y + YCell * (static_cast<const RowHeightInfo *>(P_RowsHeightAry->at(vPos))->HeightMult-1);
	}
	else {
		rect->top    = CapOffs + (YCell * vPos);
		rect->bottom = rect->top + ChrSz.y;
	}
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
	BrowserDef * p_def_ = P_Def;
	uint   left_pos = Freeze ? 0 : Left;
	uint   count = p_def_->getCount();
	rect->top    = CapOffs + GetRowTop(vPos);
	rect->bottom = rect->top + ChrSz.y + YCell * (GetRowHeightMult(vPos)-1);
	rect->left = p_def_->at(left_pos).x + 1;
	rect->right = 0;
	const uint _right = (Right < count) ? Right : (count-1);
	rect->right = CellRight(p_def_->at(_right)) - 1;
	if(isFocus) {
		SInflateRect(*rect, 3, 2);
		rect->bottom--;
	}
	return rect;
}

void BrowserWindow::DrawFocus(HDC hDC, const RECT * lpRect, BOOL DrawOrClear, BOOL isCellCursor)
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
		HPEN   old_pen = static_cast<HPEN>(SelectObject(hDC, Pens.FocusOuterPen));
		MoveToEx(hDC, lpRect->left + 1, lpRect->top + 1, 0);
		LineTo(hDC, lpRect->right - 3,  lpRect->top + 1);
		LineTo(hDC, lpRect->right - 3,  lpRect->bottom - 2);
		LineTo(hDC, lpRect->left + 1,   lpRect->bottom - 2);
		LineTo(hDC, lpRect->left + 1,   lpRect->top);
		if(old_pen)
			SelectObject(hDC, old_pen);
	}
}

void BrowserWindow::DrawCapBk(HDC hDC, const RECT * lpRect, BOOL isPressed)
{
	RECT   r = *lpRect;
	if(!isPressed) {
		const  HPEN  old_pen = static_cast<HPEN>(SelectObject(hDC, Pens.TitlePen));
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
		Polygon(hDC, points, SIZEOFARRAY(points));
		SelectObject(hDC, old_pen);
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

void BrowserWindow::Refresh()
{
	BrowserDef * p_def_ = P_Def;
	if(p_def_) {
		p_def_->refresh();
		ItemRect(HScrollPos, p_def_->_curFrameItem(), &RectCursors.CellCursor, TRUE);
		LineRect(p_def_->_curFrameItem(), &RectCursors.LineCursor, TRUE);
		AdjustCursorsForHdr();
	}
	invalidateAll(true);
}

int BrowserWindow::DrawTextUnderCursor(HDC hdc, char * pBuf, RECT * pTextRect, uint _fmt, int isLineCursor)
{
	int    ok = 0;
	if(pTextRect && pBuf) {
		uint32 schema_num = UICfg.GetBrwColorSchema();
		COLORREF old_color = SetTextColor(hdc, isLineCursor ? BrwColorsSchemas[schema_num].LineCursorOverText : BrwColorsSchemas[schema_num].CursorOverText);
		HFONT  old_font = 0, curs_over_txt_font = 0;
		TCHAR  buf[64];
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
			old_font = static_cast<HFONT>(SelectObject(hdc, curs_over_txt_font));
		DrawMultiLinesText(hdc, pBuf, pTextRect, _fmt);
		SetTextColor(hdc, old_color);
		if(old_font)
			SelectObject(hdc, old_font);
		ZDeleteWinGdiObject(&curs_over_txt_font);
		ok = 1;
	}
	return ok;
}

void BrowserWindow::DrawMultiLinesText(HDC hdc, char * pBuf, RECT * pTextRect, uint _fmt)
{
	if(pTextRect && pBuf) {
		SString temp_buf;
		RECT   rect = *pTextRect;
		StringSet ss('\n', pBuf);
		for(uint i = 0; ss.get(&i, temp_buf); rect.top += YCell, rect.bottom += YCell)
			::DrawText(hdc, SUcSwitch(temp_buf), static_cast<int>(temp_buf.Len()), &rect, _fmt);
	}
}

int BrowserWindow::GetCellColor(long row, long col, COLORREF * pColor)
{
	int    ok = -1;
	const  void * p_row = P_Def->getRow(row);
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
		if(F_CellStyle && F_CellStyle(p_row, col, paintAction, &style, CellStyleFuncExtraPtr) > 0) {
			HPEN   pen = 0;
			HPEN   oldpen = 0;
			HBRUSH br = 0;
			HBRUSH oldbr = 0;
			if(style.Color && !(style.Flags & CellStyle::fCorner)) {
				pen = CreatePen(PS_SOLID, 1, style.Color);
				oldpen = static_cast<HPEN>(SelectObject(hdc, pen));
				br = CreateSolidBrush(style.Color);
				oldbr = static_cast<HBRUSH>(SelectObject(hdc, br));
				Rectangle(hdc, r.left, r.top, r.right-1, r.bottom);

				SelectObject(hdc, oldpen);
				SelectObject(hdc, oldbr);
				ZDeleteWinGdiObject(&pen);
				ZDeleteWinGdiObject(&br);
			}
			if(style.Flags & (CellStyle::fCorner|CellStyle::fLeftBottomCorner|CellStyle::fRightFigCircle|CellStyle::fRightFigTriangle)) {
				if(oneof2(paintAction, BrowserWindow::paintFocused, BrowserWindow::paintClear))
					if(paintAction == BrowserWindow::paintClear)
						ClearFocusRect(&r);
					else
						DrawFocus(hdc, &r, TRUE, BIN(col != -1));
				POINT points[4];
				if(style.Flags & CellStyle::fCorner) {
					COLORREF color = style.Color;
					pen = CreatePen(PS_SOLID, 1, color);
					oldpen = static_cast<HPEN>(SelectObject(hdc, pen));
					br = CreateSolidBrush(color);
					oldbr = static_cast<HBRUSH>(SelectObject(hdc, br));

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
					oldpen = static_cast<HPEN>(SelectObject(hdc, pen));
					br = CreateSolidBrush(color);
					oldbr = static_cast<HBRUSH>(SelectObject(hdc, br));

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
					oldpen = static_cast<HPEN>(SelectObject(hdc, pen));
					br = CreateSolidBrush(color);
					oldbr = static_cast<HBRUSH>(SelectObject(hdc, br));
					const int _diam = 6;
					const int _right = r.right - 6;
					const int _left = _right - _diam;
					const int _top = r.top + 4;
					const int _bottom = _top + _diam + 2;
					Ellipse(hdc, _left, _top, _right, _bottom);
					SelectObject(hdc, oldpen);
					SelectObject(hdc, oldbr);
					ZDeleteWinGdiObject(&pen);
					ZDeleteWinGdiObject(&br);
				}
				else if(style.Flags & CellStyle::fRightFigTriangle) { // @v11.1.12
					COLORREF color = style.RightFigColor;
					pen = CreatePen(/*PS_SOLID*/PS_NULL, 1, color);
					oldpen = static_cast<HPEN>(SelectObject(hdc, pen));
					br = CreateSolidBrush(color);
					oldbr = static_cast<HBRUSH>(SelectObject(hdc, br));
					const int _diam = 6;
					int   _right = r.right - 6;
					int   _left = _right - _diam;
					int   _top = r.top + 4;
					int   _bottom = _top + _diam + 2;
					//Ellipse(hdc, _left, _top, _right, _bottom);
					//
					points[0].x = _left;
					points[0].y = _top;
					points[1].x = _right;
					points[1].y = _top;
					points[2].x = (_left + _right) / 2;
					points[2].y = _bottom;
					Polygon(hdc, points, 3);
					//
					SelectObject(hdc, oldpen);
					SelectObject(hdc, oldbr);
					ZDeleteWinGdiObject(&pen);
					ZDeleteWinGdiObject(&br);					
				}
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
	BrowserDef * p_def_ = P_Def;
	if(p_def_) {
		const  long hdr_width = CalcHdrWidth(1);
		const  uint count = p_def_->getCount();
		PAINTSTRUCT ps;
		RECT   r;
		union {;
			TCHAR  tbuf[512];
			char   cbuf[512];
		};
		SString temp_buf;
		::BeginPaint(H(), &ps);
		{
			RECT cli_rect;
			GetClientRect(H(), &cli_rect);
			if(ps.fErase) {
				FillRect(ps.hdc, &cli_rect, Brushes.ClearBrush);
				ps.fErase = 0;
			}
			if(ps.rcPaint.bottom == 0 || ps.rcPaint.right == 0) {
				// @v11.2.6 GetClientRect(H(), &ps.rcPaint);
				ps.rcPaint = cli_rect; // @v11.2.6 
			}
		}
		r.top    = ToolBarWidth;
		r.left   = 0;
		r.right  = CliSz.x;
		r.bottom = hdr_width - 1;
		if(P_Header && SIntersectRect(ps.rcPaint, r)) {
			static_cast<const TStaticText *>(P_Header)->getText(temp_buf);
			temp_buf.Transf(CTRANSF_INNER_TO_OUTER);
			STRNSCPY(tbuf, SUcSwitch(temp_buf));
			::DrawText(ps.hdc, tbuf, sstrleni(tbuf), &r, DT_LEFT);
		}
		r.top     = r.left = 0;
		r.bottom  = ChrSz.y * CapOffs - 3;
		r.right   = CliSz.x;
		r.top    += hdr_width;
		r.bottom += hdr_width;
		if(SIntersectRect(ps.rcPaint, r)) {
			uint32 schema_num = UICfg.GetBrwColorSchema();
			const COLORREF oldColor = SetBkColor(ps.hdc, BrwColorsSchemas[schema_num].Title);
			uint   i = 0;
			for(uint cn = Freeze ? 0 : Left; cn <= Right && cn < count;) {
				const  BroColumn & c = p_def_->at(cn);
				uint   gidx;
				r.left    = c.x - 1;
				r.right   = CellRight(c);
				r.bottom  = YCell * p_def_->GetCapHeight();
				r.top     = p_def_->IsColInGroup(cn, &gidx) ? (YCell * p_def_->GetGroup(gidx)->Height + 1) : 0;
				r.top    += hdr_width;
				r.bottom += hdr_width;
				DrawCapBk(ps.hdc, &r, FALSE);
				int   sort_status = 0;
				if(c.Options & BCO_SORTABLE) {
					uint soidx = 0;
					if(SettledOrder.lsearch(cn+1, &soidx))
						sort_status = 1;
					else if(SettledOrder.lsearch(-static_cast<int>(cn+1), &soidx))
						sort_status = 2;
					else
						sort_status = -1;
					if(sort_status) {
						COLORREF color = GetColorRef(SClrMediumorchid);
						HPEN sort_pen = CreatePen(PS_SOLID, 1, color);
						HPEN oldpen = static_cast<HPEN>(SelectObject(ps.hdc, sort_pen));
						HBRUSH sort_br = 0; //CreateSolidBrush(color);
						HBRUSH oldbr = 0; //static_cast<HBRUSH>(SelectObject(ps.hdc, sort_br));
						POINT  points[6];
						const int middle_y = (r.top + r.bottom) / 2;
						const int middle_x = (2 * r.left + 11) / 2;
						if(sort_status == 1) {
							sort_br = CreateSolidBrush(color);
							oldbr = static_cast<HBRUSH>(SelectObject(ps.hdc, sort_br));
							points[0].x = r.left;
							points[0].y = middle_y;
							points[1].x = middle_x;
							points[1].y = r.bottom-3;
							points[2].x = r.left+10;
							points[2].y = middle_y;
							points[3].x = r.left;
							points[3].y = middle_y;
							Polygon(ps.hdc, points, 4);
						}
						else if(sort_status == 2) {
							sort_br = CreateSolidBrush(color);
							oldbr = static_cast<HBRUSH>(SelectObject(ps.hdc, sort_br));
							points[0].x = r.left;
							points[0].y = middle_y;
							points[1].x = middle_x;
							points[1].y = r.top+3;
							points[2].x = r.left+10;
							points[2].y = middle_y;
							points[3].x = r.left;
							points[3].y = middle_y;
							Polygon(ps.hdc, points, 4);
						}
						else {
							sort_br = CreateSolidBrush(GetColorRef(SClrWheat));
							oldbr = static_cast<HBRUSH>(SelectObject(ps.hdc, sort_br));
							points[0].x = r.left;
							points[0].y = middle_y;
							points[1].x = middle_x;
							points[1].y = r.top+3;
							points[2].x = r.left+10;
							points[2].y = middle_y;
							points[3].x = middle_x;
							points[3].y = r.bottom-3;
							points[4].x = r.left;
							points[4].y = middle_y;
							Polygon(ps.hdc, points, 5);
						}
						SelectObject(ps.hdc, oldpen);
						ZDeleteWinGdiObject(&sort_pen);
						if(oldbr) {
							SelectObject(ps.hdc, oldbr);
							ZDeleteWinGdiObject(&sort_br);
						}
					}
				}
				r.left += sort_status ? 12 : 3;
				r.top++;
				r.right -= 3;
				r.bottom--;
				(temp_buf = c.text).Transf(CTRANSF_INNER_TO_OUTER);
				::DrawText(ps.hdc, SUcSwitch(temp_buf), (int)temp_buf.Len(), &r, GetCapAlign(c.Options));
				cn = (++i == Freeze) ? Left : (cn + 1);
			}
			if(r.right < CliSz.x) {
				r.top     = hdr_width-ToolBarWidth;
				r.bottom  = YCell * p_def_->GetCapHeight() + 1;
				r.left    = r.right + 4;
				r.right   = CliSz.x;
				r.bottom += hdr_width;
				DrawCapBk(ps.hdc, &r, TRUE);
			}
			for(i = 0; i < p_def_->GetGroupCount(); i++) {
				const BroGroup * p_grp = p_def_->GetGroup(i);
				const uint lt = MAX(p_grp->First, Left);
				const uint rt = MIN(p_grp->NextColumn()-1, Right);
				if(lt <= rt) {
					r.left    = p_def_->at(lt).x - 1;
					r.right   = CellRight(p_def_->at(rt));
					r.top     = hdr_width;
					r.bottom  = YCell * p_grp->Height + hdr_width;
					const uint tfmt = DT_CENTER|DT_EXTERNALLEADING;
					DrawCapBk(ps.hdc, &r, FALSE);
					r.left++;
					r.top++;
					r.right--;
					r.bottom--;
					(temp_buf = p_grp->P_Text).Transf(CTRANSF_INNER_TO_OUTER);
					::DrawText(ps.hdc, SUcSwitch(temp_buf), static_cast<int>(temp_buf.Len()), &r, tfmt);
				}
			}
			SetBkColor(ps.hdc, oldColor);
		}
		{
			const uint view_height = (P_RowsHeightAry && P_RowsHeightAry->getCount()) ? P_RowsHeightAry->getCount() : ViewHeight;
			for(uint row = 0; row < view_height; row++) {
				ItemRect(Left, row, &r, FALSE);
				r.left    = 0;
				r.right   = CliSz.x;
				r.top    += hdr_width;
				r.bottom += hdr_width;
				if(SIntersectRect(ps.rcPaint, r)) {
					int    is_focused = BIN(row == static_cast<UINT>(p_def_->_curFrameItem()));
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
					uint i = 0;
					uint cn = Freeze ? 0 : Left;
					for(; cn <= Right && cn < count; cn = (++i == Freeze) ? Left : (cn + 1)) {
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
							const uint height_mult = GetRowHeightMult(row);
							strip(p_def_->getMultiLinesText(p_def_->_topItem() + row, cn, cbuf, height_mult));
							if(row > 0 && (p_def_->at(cn).Options & BCO_DONTSHOWDUPL)) {
								char   prev_buf[512];
								strip(p_def_->getText(p_def_->_topItem() + row - 1, cn, prev_buf));
								if(sstreq(cbuf, prev_buf))
									PTR32(cbuf)[0] = 0;
							}
							const  int opt = p_def_->at(cn).format;
							const  int align = SFMTALIGN(opt);
							uint   tfmt;
							if(align == ALIGN_LEFT)
								tfmt = DT_LEFT;
							else if(align == ALIGN_RIGHT)
								tfmt = DT_RIGHT;
							else if(align == ALIGN_CENTER)
								tfmt = DT_CENTER;
							else
								tfmt = DT_LEFT;
							SOemToChar(cbuf);
							tfmt |= (DT_NOPREFIX | DT_SINGLELINE);
							if(is_focused && cn == HScrollPos)
								SetBkMode(ps.hdc, TRANSPARENT);
							int    already_draw = 0;
							if(SIntersectRect(RectCursors.CellCursor, r))
								already_draw = DrawTextUnderCursor(ps.hdc, cbuf, &r, tfmt, 0);
							else if(SIntersectRect(RectCursors.LineCursor, r))
								already_draw = DrawTextUnderCursor(ps.hdc, cbuf, &r, tfmt, 1);
							if(!already_draw) {
								ItemRect(cn, row, &paint_rect, TRUE);
								paint_rect.top    += hdr_width;
								paint_rect.bottom += hdr_width - 1;
								paint_rect.right -= 2;
								PaintCell(ps.hdc, paint_rect, row, cn, BrowserWindow::paintNormal);
								DrawMultiLinesText(ps.hdc, cbuf, &r, tfmt);
							}
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
			//uint   r_h_count = P_RowsHeightAry ? P_RowsHeightAry->getCount() : 0;
			const  uint r_h_count = SVectorBase::GetCount(P_RowsHeightAry);
			const  uint sel_col_count = SelectedColumns.getCount();
			long   last_fill_row = 0;
			r.top += hdr_width - 2 + YCell * GetRowHeightMult(0);
			r.left = 0;
			const uint _right = (Right < p_def_->getCount()) ? Right : (p_def_->getCount()-1);
			const BroColumn & c = p_def_->at(_right);
			const int  bottom = smin(ViewHeight, static_cast<uint>(p_def_->GetRecsCount()));
			uint   view_height = r_h_count ? r_h_count : ViewHeight;
			r.right = smin(static_cast<int>(CliSz.x), CellRight(c));
			HPEN   old_pen = static_cast<HPEN>(SelectObject(ps.hdc, Pens.GridHorzPen));
			view_height = (!r_h_count && sel_col_count) ? (view_height - 1) : view_height;
			for(uint row = 0; row < view_height; row++) {
				MoveToEx(ps.hdc, r.left, r.top, 0);
				LineTo(ps.hdc, r.right, r.top);
				r.top += ((r_h_count && (row+1) < view_height) ? (YCell * GetRowHeightMult(row+1)) : YCell);
			}
			const  int  topold = (!r_h_count && sel_col_count) ? r.top : r.top - YCell;
			long   prev_left = r.left;
			ItemRect(Left, 0, &r, FALSE);
			HPEN   black_pen = ::CreatePen(PS_SOLID, 1, GetColorRef(SClrBlack));
			HPEN   dot_line_pen = ::CreatePen(PS_SOLID, 3, GetColorRef(SClrBlack));
			r.bottom = topold;
			r.top   += hdr_width - 2;
			uint   i = 0;
			SelectObject(ps.hdc, Pens.GridVertPen);
			for(uint cn = Freeze ? 0 : Left; cn <= Right && cn < count;) {
				int  dot_line = 0;
				long dot_line_delta = 6;
				r.left = CellRight(p_def_->at(cn));
				if(sel_col_count) {
					const bool selected      = SelectedColumns.bsearch(static_cast<long>(cn), 0);
					const bool next_selected = SelectedColumns.bsearch(static_cast<long>(cn) + 1, 0);
					dot_line = ((!next_selected && selected) || (next_selected && !selected));
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
				cn = (++i == Freeze) ? Left : (cn + 1);
				prev_left = r.left;
			}
			SelectObject(ps.hdc, old_pen);
			ZDeleteWinGdiObject(&black_pen);
			ZDeleteWinGdiObject(&dot_line_pen);
		}
		::EndPaint(H(), &ps);
	}
}

int BrowserWindow::SelColByPoint(const POINT * point, int action)
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
						SelectedColumns.add(static_cast<long>(i));
			}
			else {
				long col = 0;
				uint pos = 0;
				SPoint2S tp;
				tp = *point;
				if(ItemByPoint(tp, &col, 0)) {
					if(SelectedColumns.bsearch(col, &pos))
						SelectedColumns.atFree(pos);
					else if(col >= 0 && col < static_cast<long>(P_Def->getCount()))
						SelectedColumns.add(col);
				}
			}
			SelectedColumns.sort();
			ok = 1;
		}
	}
	return ok;
}

int BrowserWindow::GetColumnByX(int x) const
{
	if(Freeze) {
		for(int i = 0; i <= static_cast<int>(Right) && i < static_cast<int>(Freeze); i++) {
			const BroColumn & c = P_Def->at(i);
			if(x >= static_cast<int>(c.x) - 2 && x <= (CellRight(c) + 2))
				return i;
		}
	}
	{
		for(int i = static_cast<int>(Left); i <= static_cast<int>(Right); i++) {
			const BroColumn & c = P_Def->at(i);
			if(x >= static_cast<int>(c.x) - 2 && x <= (CellRight(c) + 2))
				return i;
		}
	}
	return -1;
}

int BrowserWindow::HeaderByPoint(SPoint2S point, int hdrzone, long * pVertPos) const
{
	int    ok = 0;
	long   vpos = 0;
	const  long hdr_width = CalcHdrWidth(1);
	int    cx = GetColumnByX(point.x);
	if(cx >= 0) {
		uint   gidx = 0;
		int    bottom  = YCell * P_Def->GetCapHeight();
		int    top     = P_Def->IsColInGroup(cx, &gidx) ? (YCell * P_Def->GetGroup(gidx)->Height + 1) : 0;
		top    += hdr_width;
		bottom += hdr_width;
		if(hdrzone == hdrzoneSortPoint) {
			const BroColumn & r_col = P_Def->at(cx);
			int _left = r_col.x;
			const int middle_y = (top + bottom) / 2;
			const int middle_x = (2 * _left + 11) / 2;
			if(point.x >= (middle_x-5) && point.x <= (middle_x+5) && point.y >= (middle_y-5) && point.y <= (middle_y+5)) {
				vpos = cx;
				ok = 1;
			}
		}
		else if(point.y < bottom && point.y > top) {
			vpos = cx;
			ok = 1;
		}
	}
	ASSIGN_PTR(pVertPos, vpos);
	return ok;
}

int BrowserWindow::ItemByPoint(SPoint2S point, long * pHorzPos, long * pVertPos) const
{
	int    ok = 1;
	const  long hdr_width = CalcHdrWidth(1);
	int    i = GetColumnByX(point.x);
	long   vpos = 0;
	if(i >= 0) {
		ASSIGN_PTR(pHorzPos, static_cast<long>(i));
		const  uint   r_h_count = SVectorBase::GetCount(P_RowsHeightAry);
		if(r_h_count) {
			uint   y = point.y - hdr_width;
			for(uint row = 0; row < r_h_count; row++) {
				RECT rect;
				ItemRect(HScrollPos, row, &rect, FALSE);
				if(static_cast<LONG>(y) <= rect.bottom || row == r_h_count - 1) {
					vpos = static_cast<long>(P_Def->_topItem() + row);
					break;
				}
			}
		}
		else if(point.y > hdr_width + P_Def->GetCapHeight() * YCell)
			vpos = (point.y - (hdr_width + CapOffs)) / YCell + P_Def->_topItem();
		else
			vpos = P_Def->_topItem();
		ok = 1;
	}
	ASSIGN_PTR(pVertPos, vpos);
	return ok;
}

int BrowserWindow::ItemByMousePos(long * pHorzPos, long * pVertPos)
{
	SPoint2S tp;
	POINT p;
	RECT parent_rect;
	::GetWindowRect(H(), &parent_rect);
	GetCursorPos(&p);
	p.x -= parent_rect.left;
	p.y -= parent_rect.top;
	tp = p;
	return ItemByPoint(tp, pHorzPos, pVertPos);
}

int BrowserWindow::IsResizePos(SPoint2S p)
{
	const long hdr_width = CalcHdrWidth(0);
	if(p.y > hdr_width && p.y < hdr_width + ToolBarWidth + P_Def->GetCapHeight() * YCell) {
		for(int i = 0, cn = P_Def->getCount(); i < cn; i++) {
			const int b = CellRight(P_Def->at(i)) - 1;
			if(p.x > (b-5) && p.x < (b+5))
				return i + 1;
		}
	}
	return 0;
}

void BrowserWindow::Resize(SPoint2S p, int mode)
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
			if((newSz = MAX(p.x - r_c.x, 6)) != static_cast<int>(ChrSz.x * r_c.width)) {
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
				if(P_Def->IsColInGroup(ResizedCol - 1, &gidx)) {
					const BroGroup * p_grp = P_Def->GetGroup(gidx);
					gidx = MAX(p_grp->First, Left);
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
	const long v = (vPos - P_Def->_curItem());
	const long h = hPos;
	const uint view_height = (P_RowsHeightAry && P_RowsHeightAry->getCount()) ? P_RowsHeightAry->getCount() : ViewHeight;
	if(vPos < (long)(view_height + P_Def->_topItem()) && h <= (long)Right && h < P_Def->getCountI()) {
		::SendMessage(H(), WM_VSCROLL, MAKELONG(SB_THUMBPOSITION, v), v);
		::SendMessage(H(), WM_HSCROLL, MAKELONG(SB_THUMBPOSITION, h), h);
	}
}

int BrowserWindow::WMHScrollMult(int sbEvent, int thumbPos, long * pOldTop)
{
	BrowserDef * p_def_ = P_Def;
	int    res = 0;
	if(p_def_) {
		int    recalc_heights = 1;
		uint   view_height = (P_RowsHeightAry && P_RowsHeightAry->getCount()) ? P_RowsHeightAry->getCount() : ViewHeight;
		uint   bottom_item = 0;
		uint   top = 0;
		long   step = 0;
		long   next_page_height = 0;
		long   scrll_delta = 0;
		long   old_top = -1;
		switch(sbEvent) {
			case SB_TOP:
				res = p_def_->top();
				break;
			case SB_BOTTOM:
				res = p_def_->bottom();
				bottom_item = p_def_->_curItem();
				break;
			case SB_LINEUP:
				old_top = p_def_->_topItem();
				if(!old_top && !p_def_->_curItem() && !p_def_->IsBOQ())
					old_top--;
				res = p_def_->step(-1);
				recalc_heights = 0;
				break;
			case SB_LINEDOWN:
				old_top = p_def_->_topItem();
				if((long)(p_def_->_curItem()+1) >= (long)(view_height + old_top) && !p_def_->IsEOQ())
					old_top = -1;
				if(p_def_->_curFrameItem() != (view_height - 1))
					recalc_heights = 0;
				res = p_def_->step(1);
				if(recalc_heights) {
					if(IsLastPage(view_height)/*p_def_->IsEOQ()*/)
						bottom_item = p_def_->_curItem();
					p_def_->step(-(int)(view_height - 1));
				}
				else
					recalc_heights = 0;
				break;
			case SB_PAGEUP:
				{
					if(p_def_->IsBOQ())
						old_top = p_def_->_topItem();
					uint prev_top = p_def_->_topItem();
					res = p_def_->step(-(int)view_height);
					res = p_def_->step(view_height - 1);
					bottom_item = p_def_->_curItem();
					recalc_heights = 1;
				}
				break;
			case SB_PAGEDOWN:
				if(IsLastPage(view_height)/*p_def_->IsEOQ()*/) {
					old_top = p_def_->_topItem();
					bottom_item = p_def_->_topItem() + view_height - 1;
				}
				res = p_def_->step(view_height);
				p_def_->step(-(int)(view_height - 1));
				recalc_heights = 1;
				break;
			case SB_THUMBPOSITION:
				old_top = p_def_->_topItem();
				res = p_def_->step(thumbPos);
				if(thumbPos == 0) {
					recalc_heights = 1;
					bottom_item = (P_RowsHeightAry && P_RowsHeightAry->getCount() && IsLastPage(view_height)) ?
						(p_def_->_topItem() + view_height - 1) : 0;
				}
				else {
					bottom_item = p_def_->_curItem();
					recalc_heights = 0;
				}
				break;
			default:
				recalc_heights = 0;
		}
		if(!recalc_heights) {
			p_def_->getScrollData(&scrll_delta, reinterpret_cast<long *>(&VScrollPos));
			recalc_heights = scrll_delta ? 1 : 0;
		}
		//
		// calc row hights array
		//
		if(recalc_heights) {
			long   thumb_step = 0;
			CalcRowsHeight(p_def_->_topItem(), bottom_item);
			view_height = P_RowsHeightAry->getCount();
			if(sbEvent == SB_THUMBPOSITION && thumbPos == 0) {
				if((long)view_height < p_def_->_curFrameItem())
					p_def_->step(-(long)(p_def_->_curFrameItem() - view_height + 1));
				else if(p_def_->_curItem() < (long)view_height) {
					thumb_step = -(long)view_height + p_def_->_curFrameItem() + 1;
					p_def_->step(-(long)(p_def_->_curFrameItem() - view_height + 1));
				}
			}
			p_def_->setViewHight(view_height);
			if(oneof3(sbEvent, SB_PAGEDOWN, SB_LINEDOWN, SB_BOTTOM))
				p_def_->step(view_height - 1);
			else if(sbEvent == SB_PAGEUP)
				p_def_->step(-(int)(view_height - 1));
			else
				p_def_->step(thumb_step);
			old_top = -1;
		}
		ASSIGN_PTR(pOldTop, old_top);
	}
	return res;
}

void BrowserWindow::WMHScroll(int sbType, int sbEvent, int thumbPos)
{
	BrowserDef * p_def_ = P_Def;
	if(p_def_) {
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
			for(uint cn = 0; !multi_lines && cn < p_def_->getCount(); cn++)
				multi_lines = BIN(p_def_->at(cn).Options & BCO_RESIZEABLE);
			if(multi_lines)
				res = WMHScrollMult(sbEvent, thumbPos, &old_top);
			else {
				switch(sbEvent) {
					case SB_TOP:    res = p_def_->top();    break;
					case SB_BOTTOM: res = p_def_->bottom(); break;
					case SB_LINEUP:
						old_top = p_def_->_topItem();
						if(!old_top && !p_def_->_curItem() && !p_def_->IsBOQ())
							old_top--;
						res = p_def_->step(-1);
						break;
					case SB_LINEDOWN:
						old_top = p_def_->_topItem();
						if((long)(p_def_->_curItem()+1) >= (long)(ViewHeight + old_top) && !p_def_->IsEOQ())
							old_top = -1;
						res = p_def_->step(1);
						break;
					case SB_PAGEUP:
						if(p_def_->IsBOQ())
							old_top = p_def_->_topItem();
						res = p_def_->step(-(int)ViewHeight);
						break;
					case SB_PAGEDOWN:
						if(p_def_->IsEOQ())
							old_top = p_def_->_topItem();
						res = p_def_->step(ViewHeight);
						break;
					case SB_THUMBPOSITION:
						old_top = p_def_->_topItem();
						res = p_def_->step(thumbPos);
						break;
					default:;
				}
			}
			p_def_->getScrollData(&scrll_delta, reinterpret_cast<long *>(&VScrollPos));
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
				::SendMessage(GetParent(H()), BRO_ROWCHANGED, reinterpret_cast<WPARAM>(H()), MAKELPARAM(VScrollPos, 0));
				ItemRect(HScrollPos, p_def_->_curFrameItem(), &RectCursors.CellCursor, TRUE);
				LineRect(p_def_->_curFrameItem(), &RectCursors.LineCursor, TRUE);
				AdjustCursorsForHdr();
				if(p_def_->_topItem() != old_top)
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
					while(cur > Right) {
						Left++;
						CalcRight();
					}
					if(cur == Right) {
						while(Left < (int)HScrollMax && (CellRight(p_def_->at(cur)) + 5) > CliSz.x) {
							Left++;
							CalcRight();
						}
					}
					if(cur < Left) {
						Left = cur;
						CalcRight();
					}
				}
				SendMessage(GetParent(H()), BRO_ROWCHANGED, reinterpret_cast<WPARAM>(H()), MAKELPARAM(HScrollPos, 0));
				ClearFocusRect(&RectCursors.CellCursor);
				ClearFocusRect(&RectCursors.LineCursor);
				ItemRect(HScrollPos, p_def_->_curFrameItem(), &RectCursors.CellCursor, TRUE);
				LineRect(p_def_->_curFrameItem(), &RectCursors.LineCursor, TRUE);
				AdjustCursorsForHdr();
				if(Left != oldLeft) {
					// ItemRect(Left, p_def_->_curFrameItem(), &r, TRUE);
					r.left   = p_def_->at(Left).x+1;
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
	}
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
			const  long recs_count = P_Def->GetRecsCount();
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
				for(row = 0; P_RowsHeightAry->enumItems(&row, (void **)&p_item) > 0;) {
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
	return BIN(cls_name == SUcSwitch(BrowserWindow::WndClsName) || cls_name == STimeChunkBrowser::WndClsName ||
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

/*static*/LRESULT CALLBACK BrowserWindow::BrowserWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	int    i;
	long   hPos;
	long   vPos;
	POINT  pnt;
	SPoint2S tp;
	TEvent e;
	BrowserWindow * p_view = static_cast<BrowserWindow *>(TView::GetWindowUserData(hWnd));
	long   hdr_width = p_view ? p_view->CalcHdrWidth(1) : 0;
	CREATESTRUCT * p_init_data = 0;
	switch(msg) {
		case WM_CREATE:
			// reinterpret_cast<LPCREATESTRUCT>(lParam)
			p_view = static_cast<BrowserWindow *>(Helper_InitCreation(lParam, (void **)&p_init_data));
			if(p_view) {
				p_view->HW = hWnd;
				p_view->WMHCreate();
				InvalidateRect(hWnd, 0, TRUE);
				::PostMessage(hWnd, WM_PAINT, 0, 0);
				{
					SString temp_buf;
					TView::SGetWindowText(hWnd, temp_buf);
					APPL->AddItemToMenu(temp_buf, p_view);
				}
				if(p_view->InitPos > 0)
					p_view->go(p_view->InitPos);
				return 0;
			}
			else
				return -1;
		case WM_DESTROY:
			if(p_view && p_view->IsConsistent()) {
				if((p_view->Sf & sfModal) && !(p_view->Sf & sfOnDestroy)) {
					p_view->EndModalCmd = cmCancel;
					::PostMessage(hWnd, WM_NULL, 0, 0L);
				}
				else {
					TView::SetWindowUserData(hWnd, 0);
					if(!(p_view->Sf & sfOnDestroy))
						delete p_view;
				}
				APPL->NotifyFrame(1);
			}
			return 0;
		case WM_SETFONT:
			{
				SetFontEvent sfe(reinterpret_cast<void *>(wParam), LOWORD(lParam));
				TView::messageCommand(p_view, cmSetFont, &sfe);
			}
			return 0;
		case BRO_GETCURCOL:
			if(p_view) {
				*reinterpret_cast<UINT *>(reinterpret_cast<void **>(lParam)) = p_view->HScrollPos;
			}
			return 0;
		case BRO_DATACHG:
			if(p_view) {
				p_view->P_Def->refresh();
				InvalidateRect(hWnd, NULL, TRUE);
				UpdateWindow(hWnd);
			}
			return 0;
		case WM_SIZE:
			if(lParam && p_view) {
				const int cell = p_view->YCell;
				const int cap  = p_view->CapOffs;
				p_view->CliSz.Set(LOWORD(lParam), cap + ((HIWORD(lParam) - cap) / cell + 1) * cell);
				lParam = p_view->CliSz.towparam();
				p_view->SetupScroll();
				p_view->CalcRight();
				HWND hw = p_view->P_Toolbar ? p_view->P_Toolbar->H() : 0;
				if(::IsWindowVisible(hw)) {
					::MoveWindow(hw, 0, 0, LOWORD(lParam), p_view->ToolBarWidth, 0);
					// @v12.2.5 TView::messageCommand(p_view, cmResize); // must be cmSize
					// @v12.2.5 {
					if(!TView::Helper_SendCmSizeAsReplyOnWmSize(p_view, wParam, lParam))
						return 0;
					// } @v12.2.5 
				}
			}
			break;
		case WM_PAINT:
			if(p_view) {
				p_view->Paint();
				if(p_view->P_Header) {
					HWND hw_child = GetWindow(hWnd, GW_CHILD);
					if(hw_child)
						::UpdateWindow(hw_child);
				}
			}
			return 0L;
		case WM_HSCROLL:
		case WM_VSCROLL:
			if(hWnd == GetFocus() && p_view) {
				p_view->WMHScroll((msg == WM_HSCROLL) ? SB_HORZ : SB_VERT, LOWORD(wParam), (int16)HIWORD(wParam));
				return 0L;
			}
			else
				SetFocus(hWnd);
			break;
		case WM_MOUSEWHEEL:
			if(p_view) {
				short delta = static_cast<short>(HIWORD(wParam));
				int   scroll_code = (delta > 0) ? SB_LINEUP : SB_LINEDOWN;
				for(int i = 0; i < 5; i++)
					p_view->WMHScroll(SB_VERT, scroll_code, 0);
			}
			break;
		case WM_MOUSEHOVER:
			tp.setwparam(lParam);
			///* @construction
			{
				long vpos = 0;
				if(p_view->HeaderByPoint(tp, hdrzoneAny, &vpos) && vpos >= 0 && vpos < p_view->P_Def->getCountI()) {
					const BroColumn & c = p_view->P_Def->at(vpos);
					SString temp_buf(c.text);
					SMessageWindow * p_win = new SMessageWindow;
					if(p_win) {
						temp_buf.ReplaceChar('\003', ' ').Strip();
						COLORREF color = GetColorRef(SClrLightyellow);
						long   flags = SMessageWindow::fShowOnCursor|SMessageWindow::fSizeByText|SMessageWindow::fOpaque|SMessageWindow::fPreserveFocus;
						p_win->Open(temp_buf, 0, 0, 0, 3000, color, flags, 0);
					}
				}
			}
			//*/
			TView::messageBroadcast(p_view, cmMouseHover, &tp);
			break;
		case WM_SETFOCUS:
			if(!(TView::SGetWindowStyle(hWnd) & WS_CAPTION)) {
				SetWindowPos(hWnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE);
				APPL->NotifyFrame(0);
			}
			if(p_view) {
				APPL->SelectTabItem(p_view);
				TView::messageBroadcast(p_view, cmReceivedFocus);
				p_view->select();
			}
			break;
		case WM_KILLFOCUS:
			if(!(TView::SGetWindowStyle(hWnd) & WS_CAPTION))
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
				::PostMessage(APPL->H_MainWnd, WM_SYSKEYDOWN, wParam, lParam);
				return 0;
			}
		case WM_KEYDOWN:
			if(p_view) {
				static const struct scrollkeys {
					WORD wVirtkey;
					int  iMessage;
					WORD wRequest;
				} key2scroll[] = {
					{ VK_HOME,  WM_VSCROLL, SB_TOP },
					{ VK_END,   WM_VSCROLL, SB_BOTTOM },
					{ VK_PRIOR, WM_VSCROLL, SB_PAGEUP },
					{ VK_NEXT,  WM_VSCROLL, SB_PAGEDOWN },
					{ VK_UP,    WM_VSCROLL, SB_LINEUP },
					{ VK_DOWN,  WM_VSCROLL, SB_LINEDOWN },
					{ VK_LEFT,  WM_HSCROLL, SB_LINEUP },
					{ VK_RIGHT, WM_HSCROLL, SB_LINEDOWN }
				};
				for(i = 0; i < SIZEOFARRAY(key2scroll); i++) {
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
					if(GetKeyState(VK_CONTROL) & 0x8000 && !p_view->IsInState(sfModal)) {
						SetFocus(GetNextBrowser(hWnd, (GetKeyState(VK_SHIFT) & 0x8000) ? 0 : 1));
						return 0;
					}
				}
				p_view->HandleKeyboardEvent(LOWORD(wParam));
			}
			return 0;
		case WM_CHAR:
			if(p_view) {
				const char kb_scan_code = LOBYTE(HIWORD(lParam));
				if(wParam != VK_RETURN || kb_scan_code != 0x1c) {
					const char ac = SSystem::TranslateWmCharToAnsi(wParam);
					TView::messageKeyDown(p_view, ac);
				}
			}
			return 0;
		case WM_LBUTTONDBLCLK:
			::SendMessage(GetParent(hWnd), BRO_LDBLCLKNOTIFY, reinterpret_cast<WPARAM>(hWnd), lParam);
			TView::messageCommand(p_view, cmaEdit);
			return 0;
		case WM_RBUTTONDBLCLK:
			::SendMessage(GetParent(hWnd), BRO_RDBLCLKNOTIFY, reinterpret_cast<WPARAM>(hWnd), lParam);
			return 0;
		case WM_NCLBUTTONDOWN:
			if(hWnd != GetFocus())
				SetFocus(hWnd);
			break;
		case WM_RBUTTONUP:
			if(p_view) {
				tp.setwparam(lParam);
				if(p_view->ItemByPoint(tp, &hPos, &vPos)) {
					p_view->FocusItem(hPos, vPos);
					TView::messageKeyDown(p_view, kbShiftF10);
				}
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
					int done = 0;
					if(p_view->HeaderByPoint(tp, hdrzoneSortPoint, &vPos) && vPos >= 0 && vPos < p_view->P_Def->getCountI()) {
						const BroColumn & c = p_view->P_Def->at(vPos);
						if(c.Options & BCO_SORTABLE) {
							long   vp1 = vPos+1;
							uint   sopos = 0;
							if(p_view->SettledOrder.lsearch(vp1, &(sopos = 0)) || p_view->SettledOrder.lsearch(-vp1, &(sopos = 0))) {
								long org_v = p_view->SettledOrder.get(sopos);
								p_view->SettledOrder.clear();
								p_view->SettledOrder.add(-org_v);
							}
							else {
								p_view->SettledOrder.clear();
								p_view->SettledOrder.add(vp1);
							}
							TView::messageCommand(p_view, cmSort, 0);
							InvalidateRect(hWnd, 0, TRUE);
							UpdateWindow(hWnd);
							done = 1;
						}
					} 
					if(!done && p_view->ItemByPoint(tp, &hPos, &vPos)) {
						p_view->FocusItem(hPos, vPos);
						if(p_view->SelColByPoint(&pnt, (wParam & MK_CONTROL) ? 1 : ((wParam & MK_SHIFT) ? -1 : 0)) > 0) {
							InvalidateRect(hWnd, 0, TRUE);
							UpdateWindow(hWnd);
						}
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
				tp.setwparam(lParam);
				if(wParam == 0 && hWnd == GetFocus()) {
					if(p_view->PrevMouseCoord != tp) {
						p_view->TWindow::RegisterMouseTracking(0, 1000);
						p_view->PrevMouseCoord = tp;
					}
					const int rc = p_view->IsResizePos(tp);
					if(rc) {
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
						if(p_view->ItemByPoint(tp, &hPos, &vPos))
							p_view->FocusItem(hPos, vPos);
					}
				}
			}
			return 0;
		case WM_COMMAND:
			PostMessage(hWnd, WM_USER+101, wParam, lParam);
			return 0;
		case WM_USER+101:
			if(p_view && p_view->IsConsistent()) { // @v12.2.5 (&& p_view->IsConsistent())
				if(HIWORD(wParam) == 0) {
					TView::messageKeyDown(p_view, LOWORD(wParam));
				}
				else if(HIWORD(wParam) == 1) {
					e.what = TEvent::evCommand;
					e.message.command = LOWORD(wParam);
					p_view->handleEvent(e);
				}
			}
			return 0;
		case WM_USER_NOTIFYOTHERWNDEVNT: // @v11.1.12
			if(p_view) {
				RECT rc_client;
				GetClientRect(hWnd, &rc_client); // @debug				
				e.what = TEvent::evCommand;
				e.message.command = cmNotifyForeignFocus;
				e.message.infoPtr = (void *)lParam;
				p_view->handleEvent(e);
			}
			return 0;
		case WM_GETMINMAXINFO:
			if(!IsIconic(hWnd)) {
				LPMINMAXINFO p_min_max = reinterpret_cast<LPMINMAXINFO>(lParam);
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

/*static*/int BrowserWindow::RegWindowClass(HINSTANCE hInst)
{
	WNDCLASSEX wc;
	INITWINAPISTRUCT(wc);
	wc.lpszClassName = BrowserWindow::WndClsName;
	wc.hInstance     = hInst;
	wc.lpfnWndProc   = BrowserWindow::BrowserWndProc;
	wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC | CS_DBLCLKS;
	wc.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(/*ICON_MAIN_P2*/ 102));
	wc.cbClsExtra    = BRWCLASS_CEXTRA;
	wc.cbWndExtra    = BRWCLASS_WEXTRA;
	return ::RegisterClassEx(&wc);
}

int BrowserWindow::search(void * pPattern, CompFunc fcmp, int srchMode)
{
	int    ok = 0;
	if(P_Def && P_Def->search(pPattern, fcmp, srchMode, HScrollPos)) {
		long   scrll_delta, scrll_pos;
		P_Def->getScrollData(&scrll_delta, &scrll_pos);
		VScrollPos = P_Def->_curFrameItem();
		ItemRect(HScrollPos, VScrollPos, &RectCursors.CellCursor, TRUE);
		LineRect(VScrollPos, &RectCursors.LineCursor, TRUE);
		AdjustCursorsForHdr();
		ok = 1;
	}
	else if(fcmp == SrchFunc) {
		SendMessage(H(), WM_KEYDOWN, VK_END, 0L);
		// @v11.2.6 UserInterfaceSettings ui_cfg;
		// @v11.2.6 ui_cfg.Restore();
		// @v11.2.6 if(!(ui_cfg.Flags & UserInterfaceSettings::fDisableNotFoundWindow)) {
		if(!(APPL->GetUiSettings().Flags & UserInterfaceSettings::fDisableNotFoundWindow)) { // @v11.2.6
			SMessageWindow * p_win = new SMessageWindow;
			if(p_win) {
				SString msg_buf, fmt_buf;
				SLS.LoadString_("strnfound", fmt_buf);
				msg_buf.Printf(fmt_buf, static_cast<const char *>(pPattern));
				p_win->Open(msg_buf, 0, H(), 0, 5000, GetColorRef(SClrRed), SMessageWindow::fShowOnCenter|SMessageWindow::fChildWindow, 0);
			}
		}
	}
	return ok;
}

void BrowserWindow::search(const char * pFirstLetter, int srchMode)
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
