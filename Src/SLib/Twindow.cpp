// TWINDOW.CPP  Turbo Vision 1.0
// Copyright (c) 1991 by Borland International
// WIN32
// Modified and adopted by A.Sobolev 1996-2001, 2002, 2003, 2005, 2006, 2007, 2008, 2010, 2011, 2013, 2015, 2016, 2017, 2018
//
#include <slib.h>
#include <tv.h>
#pragma hdrstop
//
//
//
#define MPST_ERROR         0x0001
#define MPST_PREVSEPARATOR 0x0002

TMenuPopup::TMenuPopup() : State(0), Count(0), H((uint32)::CreatePopupMenu())
{
}

TMenuPopup::~TMenuPopup()
{
	if(H) {
		::DestroyMenu((HMENU)H);
		H = 0;
	}
}

uint TMenuPopup::GetCount() const
{
	return Count;
}

int TMenuPopup::Add(const char * pText, int cmd)
{
	int    ok = 0;
	assert(pText);
	if(H) {
		SString temp_buf;
		if(pText[0] == '@') {
			if(!SLS.LoadString(pText+1, temp_buf))
				temp_buf = pText+1;
			else
				temp_buf.Transf(CTRANSF_INNER_TO_OUTER);
			pText = temp_buf;
		}
		UINT flags = MF_STRING|MF_ENABLED;
		SETFLAG(flags, MF_SEPARATOR, BIN(cmd == TV_MENUSEPARATOR));
		ok = BIN(::AppendMenu((HMENU)H, flags, (cmd == TV_MENUSEPARATOR) ? 0 : cmd, pText)); // @unicodeproblem
		if(ok)
			Count++;
		State &= ~MPST_PREVSEPARATOR;
	}
	return ok;
}

int TMenuPopup::AddSubstr(const char * pText, int idx, int cmd)
{
	SString temp_buf;
	return temp_buf.GetSubFrom(pText, ';', idx) ? Add(temp_buf, cmd) : 0;
}

int TMenuPopup::AddSeparator()
{
	int    ok = 0;
	if(H) {
		if(State & MPST_PREVSEPARATOR)
			ok = -1;
		else {
			ok = BIN(::AppendMenu((HMENU)H, MF_SEPARATOR, 0, 0));
			State |= MPST_PREVSEPARATOR;
		}
	}
	return ok;
}

int TMenuPopup::Execute(HWND hWnd, long flags)
{
	int    cmd = 0;
	if(H) {
		POINT p;
		::GetCursorPos(&p);
		uint f = TPM_LEFTALIGN|TPM_LEFTBUTTON;
		if(flags & efRet)
			f |= TPM_RETURNCMD;
		cmd = ::TrackPopupMenu((HMENU)H, f, p.x, p.y, 0, hWnd, 0);
	}
	return cmd;
}
//
//
//
TWindow::LocalMenuPool::LocalMenuPool(TWindow * pWin) : List(sizeof(TWindow::LocalMenuPool::Item)), P_Win(pWin)
{
	StrPool.add("$"); // zero index - is empty string
}

int TWindow::LocalMenuPool::AddItem(uint ctrlId, uint buttonId, long keyCode, const char * pText)
{
	int    ok = 1;
	Item   item;
	MEMSZERO(item);
	item.CtrlId = ctrlId;
	item.ButtonId = buttonId;
	item.KeyCode = keyCode;
	if(pText)
		StrPool.add(pText, &item.StrPos);
	List.insert(&item);
	return ok;
}

int TWindow::LocalMenuPool::GetCtrlIdByButtonId(uint buttonId, uint * pCtrlId) const
{
	int    ok = 0;
	if(buttonId) {
		for(uint i = 0; !ok && i < List.getCount(); i++) {
			const Item * p_item = (const Item *)List.at(i);
			if(p_item->ButtonId == buttonId && p_item->CtrlId) {
				ASSIGN_PTR(pCtrlId, p_item->CtrlId);
				ok = 1;
			}
		}
	}
	return ok;
}

int TWindow::LocalMenuPool::GetButtonIdByCtrlId(uint ctrlId, uint * pButtonId) const
{
	int    ok = 0;
	if(ctrlId) {
		for(uint i = 0; !ok && i < List.getCount(); i++) {
			const Item * p_item = (const Item *)List.at(i);
			if(p_item->CtrlId == ctrlId && p_item->ButtonId) {
				ASSIGN_PTR(pButtonId, p_item->ButtonId);
				ok = 1;
			}
		}
	}
	return ok;
}

int TWindow::LocalMenuPool::ShowMenu(uint buttonId)
{
	int    ok = 1;
	if(P_Win) {
		uint   sel = 0;
		SString text;
		TMenuPopup menu;
		for(uint i = 0; ok > 0 && i < List.getCount(); i++) {
			const Item * p_item = (const Item *)List.at(i);
			if(p_item->ButtonId == buttonId && p_item->StrPos) {
				if(p_item->CtrlId) {
					TView * p_view = P_Win->getCtrlView(p_item->CtrlId);
					if(p_view && p_view->IsInState(sfDisabled))
						ok = -1;
				}
				if(ok > 0) {
					StrPool.get(p_item->StrPos, text);
					menu.Add(text, p_item->KeyCode);
					SETIFZ(sel, p_item->CtrlId);
				}
			}
		}
		if(ok > 0) {
			menu.Execute(P_Win->H());
			P_Win->selectCtrl(sel);
		}
	}
	return ok;
}
//
//
//
ToolbarList::ToolbarList() : SVector(sizeof(ToolbarItem)), Bitmap(0) {} // @v9.8.4 SArray-->SVector
void  ToolbarList::setBitmap(uint b) { Bitmap = b; }
uint  ToolbarList::getBitmap() const { return Bitmap; }
uint  ToolbarList::getItemsCount() const { return getCount(); }
const ToolbarItem & FASTCALL ToolbarList::getItem(uint idx) const { return *(ToolbarItem *)at(idx); }
void  ToolbarList::clearAll() { freeAll(); }
int   ToolbarList::addItem(const ToolbarItem * pItem) { return pItem ? insert(pItem) : -1; }

ToolbarList & FASTCALL ToolbarList::operator = (const ToolbarList & s)
{
	Bitmap = s.Bitmap;
	copy(s);
	return *this;
}

uint ToolbarList::getVisibleItemsCount() const
{
	uint visible_count = 0;
	for(uint i = 0; i < getItemsCount(); i++) {
		const ToolbarItem & r_item = getItem(i);
		if(!(r_item.Flags & ToolbarItem::fHidden))
			visible_count++;
	}
	return visible_count;
}

int ToolbarList::enumItems(uint * pIdx, ToolbarItem * pItem)
{
	ToolbarItem * p_item;
	if(SVector::enumItems(pIdx, (void**)&p_item) > 0) {
		ASSIGN_PTR(pItem, *p_item);
		return 1;
	}
	else
		return 0;
}

int ToolbarList::searchKeyCode(ushort keyCode, uint * pIdx) const
{
	for(uint i = 0; i < getCount(); i++)
		if(getItem(i).KeyCode == keyCode) {
			ASSIGN_PTR(pIdx, i);
			return 1;
		}
	ASSIGN_PTR(pIdx, 0);
	return 0;
}

int ToolbarList::moveItem(uint pos, int up)
{
	int    ok = 0;
	if(pos < getCount()) {
		if(up) {
			if(pos > 0) {
				ToolbarItem item = *(ToolbarItem *)at(pos);
				atFree(pos);
				atInsert(pos-1, &item);
				ok = 1;
			}
		}
		else {
			if(pos < getCount()-1) {
				ToolbarItem item = *(ToolbarItem *)at(pos);
				atFree(pos);
				atInsert(pos+1, &item);
				ok = 1;
			}
		}
	}
	return ok;
}
//
//
//
// static
int TWindow::IsMDIClientWindow(HWND h)
{
	SString cls_name;
	TView::SGetWindowClassName(h, cls_name);
	return (cls_name == "MDICLIENT");
}

SLAPI TWindow::TWindow(const TRect & bounds, const char * pTitle, short aNumber) : TGroup(bounds), P_Lmp(0), HW(0), PrevInStack(0)
{
	options  |= ofSelectable;
	Title = pTitle;
}

SLAPI TWindow::~TWindow()
{
	delete P_Lmp;
	if(HW) {
		//
		// Обнуляем ссылку на this в системной структуре окна во
		// избежании попытки повторного удаления оконной процедурой.
		//
		//SetWindowLong(hWnd, GWLP_USERDATA, 0);
		TView::SetWindowUserData(HW, (void *)0);
	}
}

void TWindow::endModal(ushort command)
{
	EndModalCmd = NZOR(command, cmCancel);
	if(Sf & sfModal) {
		::PostMessage(H(), WM_NULL, 0, 0L);
	}
	else {
		::DestroyWindow(H());
		// После вызова DestroyWindow экземпляр this разрушается: никаких действий с ним далее проводить нельзя.
	}
}

int TWindow::AddLocalMenuItem(uint ctrlId, uint buttonId, long keyCode, const char * pText)
{
	return SETIFZ(P_Lmp, new LocalMenuPool(this)) ? P_Lmp->AddItem(ctrlId, buttonId, keyCode, pText) : (SLibError = SLERR_NOMEM, 0);
}

void TWindow::close()
{
	if(valid(cmClose))
		delete this;
}

static bool searchItem(TView * v, void *ptr)
{
	return v->TestId(*(ushort *)ptr) ? true : false;
}

TView * FASTCALL TWindow::getCtrlView(ushort ctlID)
{
	// @v8.6.11 return Id ? firstThat(searchItem, &ctlID) : 0;
	// @v8.6.11 Функция вызывается очень часто потому ради скорости исполнения развернута {
	if(ctlID) {
		TView * p_temp = P_Last;
		if(p_temp) {
			const TView * p_term = P_Last;
			do {
				p_temp = p_temp->P_Next;
				if(p_temp && p_temp->IsConsistent()) {
					if(p_temp->GetId_Unsafe() == ctlID)
						return p_temp;
				}
				else
					return 0;
			} while(p_temp != p_term);
		}
	}
	return 0;
	// } @v8.6.11
}

HWND FASTCALL TWindow::getCtrlHandle(ushort ctlID)
	{ return GetDlgItem(HW, ctlID); }
void * SLAPI TWindow::messageToCtrl(ushort ctlID, ushort cmd, void *ptr)
	{ return TView::messageCommand(getCtrlView(ctlID), cmd, ptr); }

int SLAPI TWindow::setSmartListBoxOption(uint ctlID, uint option)
{
	int    ok = 1;
	SmartListBox * p_list = (SmartListBox *)getCtrlView(ctlID);
	if(p_list && p_list->IsSubSign(TV_SUBSIGN_LISTBOX) && p_list->def)
		p_list->def->SetOption(option, 1);
	else
		ok = 0;
	return ok;
}

void FASTCALL TWindow::setCtrlReadOnly(ushort ctlID, int enable)
{
	TView * v = getCtrlView(ctlID);
	if(v) {
		v->setState(sfReadOnly, enable ? true : false);
		if(enable && (P_Current->IsInState(sfDisabled|sfReadOnly) || P_Current == v))
			selectNext();
	}
}

void FASTCALL TWindow::drawCtrl(ushort ctlID)
{
	TView * v = getCtrlView(ctlID);
	if(v) {
		HWND h = v->getHandle();
		if(h) {
			InvalidateRect(h, 0, 1);
			UpdateWindow(h);
		}
	}
}

void FASTCALL TWindow::disableCtrl(ushort ctlID, int enable)
{
	TView * v = getCtrlView(ctlID);
	if(v) {
		v->setState(sfDisabled, enable ? true : false);
		if(enable && P_Current && (P_Current->IsInState(sfDisabled) || P_Current == v)) // ((P_Current->state & sfDisabled))
			selectNext();
		if(P_Lmp) {
			uint   button_id = 0;
			if(P_Lmp->GetButtonIdByCtrlId(ctlID, &button_id) && button_id != ctlID)
				disableCtrl(button_id, enable); // @recursion
		}
	}
}

void cdecl TWindow::disableCtrls(int enable, ...)
{
	va_list p;
	ushort ctl;
	va_start(p, enable);
	while((ctl = va_arg(p, ushort)) != 0)
		disableCtrl(ctl, enable);
	va_end(p);
}

void FASTCALL TWindow::selectCtrl(ushort ctlID)
{
	if(ctlID) {
		TView * p_v = getCtrlView(ctlID);
		if(p_v)
			SetCurrentView(p_v, normalSelect);
	}
}

TButton * FASTCALL TWindow::SearchButton(uint cmd)
{
	TView * p_view = (TView *)TView::messageBroadcast(this, cmSearchButton, (void *)cmd);
	return (p_view && p_view->IsSubSign(TV_SUBSIGN_BUTTON)) ? (TButton *)p_view : 0;
}

int SLAPI TWindow::selectButton(ushort cmd)
{
	int    ok = 1;
	TButton * p_view = SearchButton(cmd);
	if(p_view)
		SetCurrentView(p_view, forceSelect);
	else
		ok = 0;
	return ok;
}

void SLAPI TWindow::showButton(uint cmd, int s)
{
	TButton * p_view = SearchButton(cmd);
	CALLPTRMEMB(p_view, Show(s));
}

int SLAPI TWindow::setButtonText(uint cmd, const char * pText)
{
	int    ok = 1;
	TButton * p_ctl = SearchButton(cmd);
	const uint ctl_id = p_ctl ? p_ctl->GetId() : 0;
	if(ctl_id) {
		p_ctl->Title = pText;
		// @v9.1.5 SendDlgItemMessage(HW, ctl_id, WM_SETTEXT, 0, (LPARAM)(const char *)p_ctl->Title);
		TView::SSetWindowText(GetDlgItem(HW, ctl_id), p_ctl->Title); // @v9.1.5
	}
	else
		ok = 0;
	return ok;
}

int SLAPI TWindow::setButtonBitmap(uint cmd, uint bmpID)
{
	TButton * p_ctl = SearchButton(cmd);
	return p_ctl ? p_ctl->SetBitmap(bmpID) : 0;
}

int SLAPI TWindow::destroyCtrl(uint ctlID)
{
	int    ok = -1;
	if(ctlID) {
		TView * p_v = getCtrlView(ctlID);
		if(p_v) {
			DestroyWindow(p_v->getHandle());
			removeView(p_v);
			delete p_v;
			ok = 1;
		}
	}
	return ok;
}

int FASTCALL TWindow::setCtrlData(ushort ctlID, void * data)
{
	TView * v = getCtrlView(ctlID);
	return v ? (v->TransmitData(+1, data), 1) : 0;
}

int FASTCALL TWindow::getCtrlData(ushort ctlID, void * data)
{
	TView * v = getCtrlView(ctlID);
	return v ? (v->TransmitData(-1, data), 1) : 0;
}

long FASTCALL TWindow::getCtrlLong(uint ctlID)
{
	long   val = 0;
	TView * p_v = getCtrlView(ctlID);
	CALLPTRMEMB(p_v, TransmitData(-1, &val));
	return val;
}

int FASTCALL TWindow::setCtrlUInt16(uint ctlID, int s)
{
	uint16 val = (uint16)s;
	return setCtrlData(ctlID, &val);
}

uint16 FASTCALL TWindow::getCtrlUInt16(uint ctlID)
{
	uint16 val = 0;
	TView * p_v = getCtrlView(ctlID);
	CALLPTRMEMB(p_v, TransmitData(-1, &val));
	return val;
}

double FASTCALL TWindow::getCtrlReal(uint ctlID)
{
	double val = 0.0;
	TView * p_v = getCtrlView(ctlID);
	CALLPTRMEMB(p_v, TransmitData(-1, &val));
	return val;
}

LDATE FASTCALL TWindow::getCtrlDate(uint ctlID)
{
	LDATE dt = ZERODATE;
	return getCtrlData(ctlID, &dt) ? dt : ZERODATE;
}

LTIME FASTCALL TWindow::getCtrlTime(uint ctlID)
{
	LTIME tm = ZEROTIME;
	return getCtrlData(ctlID, &tm) ? tm : ZEROTIME;
}

int FASTCALL TWindow::setCtrlString(uint ctlID, const SString & s)
{
	int    ok = 0;
	char   temp_buf[1024];
	size_t temp_len = sizeof(temp_buf);
	char * p_temp = temp_buf;
	int    is_temp_allocated = 0;
	TView * p_v = (TView *)getCtrlView(ctlID);
	if(p_v) {
		const uint ctrl_subsign = p_v->GetSubSign();
		if(ctrl_subsign == TV_SUBSIGN_INPUTLINE) {
			const size_t max_len = ((TInputLine *)p_v)->getMaxLen();
			if(max_len > temp_len && s.Len() >= temp_len) {
				temp_len = max_len+32;
				p_temp = (char *)SAlloc::M(temp_len);
				if(p_temp) {
					is_temp_allocated = 1;
					temp_len = max_len+32;
				}
				else {
					p_temp = temp_buf;
					temp_len = sizeof(temp_buf);
				}
			}
			s.CopyTo(p_temp, temp_len);
			ok = setCtrlData(ctlID, p_temp);
		}
		else if(ctrl_subsign == TV_SUBSIGN_IMAGEVIEW) {
			s.CopyTo(temp_buf, sizeof(temp_buf));
			ok = setCtrlData(ctlID, temp_buf);
		}
		// @v9.4.5 {
		else if(ctrl_subsign == TV_SUBSIGN_STATIC) {
			((TStaticText *)p_v)->setText(s);
		}
		// } @v9.4.5 
	}
	if(is_temp_allocated)
		ZFREE(p_temp);
	return ok;
}

int FASTCALL TWindow::getCtrlString(uint ctlID, SString & s)
{
	int    ok = 0;
	char   temp_buf[1024];
	char * p_temp = temp_buf;
	int    is_temp_allocated = 0;
	TInputLine * p_il = (TInputLine *)getCtrlView(ctlID);
	if(p_il && p_il->IsSubSign(TV_SUBSIGN_INPUTLINE)) {
		const size_t max_len = p_il->getMaxLen();
		if(max_len > sizeof(temp_buf)) {
			p_temp = (char *)SAlloc::M(max_len+32);
			if(p_temp)
				is_temp_allocated = 1;
		}
		if(getCtrlData(ctlID, p_temp)) {
			s = p_temp;
			ok = 1;
		}
	}
	if(is_temp_allocated)
		ZFREE(p_temp);
	return ok;
}

int FASTCALL TWindow::setCtrlLong(uint ctlID, long val) { return setCtrlData(ctlID, &val); }
int FASTCALL TWindow::setCtrlReal(uint ctlID, double val) { return setCtrlData(ctlID, &val); }
int FASTCALL TWindow::setCtrlDate(uint ctlID, LDATE val) { return setCtrlData(ctlID, &val); }
int FASTCALL TWindow::setCtrlTime(uint ctlID, LTIME val) { return setCtrlData(ctlID, &val); }
int SLAPI TWindow::setCtrlDatetime(uint dtCtlID, uint tmCtlID, LDATETIME dtm) { return BIN(setCtrlData(dtCtlID, &dtm.d) && setCtrlData(tmCtlID, &dtm.t)); }
int SLAPI TWindow::setCtrlDatetime(uint dtCtlID, uint tmCtlID, LDATE dt, LTIME tm) { return BIN(setCtrlData(dtCtlID, &dt) && setCtrlData(tmCtlID, &tm)); }
int SLAPI TWindow::getCtrlDatetime(uint dtCtlID, uint tmCtlID, LDATETIME & rDtm) { return BIN(getCtrlData(dtCtlID, &rDtm.d) && getCtrlData(tmCtlID, &rDtm.t)); }

void SLAPI TWindow::setCtrlOption(ushort ctlID, ushort flags, int s)
{
	TView * v = getCtrlView(ctlID);
	if(v)
		SETFLAG(v->options, flags, s);
}

int SLAPI TWindow::getStaticText(ushort ctlID, SString & rBuf)
{
	int    ok = 1;
	TStaticText * p_st = (TStaticText*)getCtrlView(ctlID);
	if(p_st)
		p_st->getText(rBuf);
	else {
		rBuf.Z();
		ok = 0;
	}
	return ok;
}

int SLAPI TWindow::setStaticText(ushort ctlID, const char * pText)
{
	int    ok = 1;
	TStaticText * p_st = (TStaticText*)getCtrlView(ctlID);
	if(p_st)
		p_st->setText(pText);
	else
		ok = 0;
	return ok;
}

void SLAPI TWindow::showCtrl(ushort ctlID, int s)
{
	TView * v = getCtrlView(ctlID);
	if(v)
		v->setState(sfVisible, s ? true : false);
	else {
		HWND   w_ctl = GetDlgItem(HW, ctlID);
		if(w_ctl)
			ShowWindow(w_ctl, s ? SW_SHOW : SW_HIDE);
	}
}

void FASTCALL TWindow::setTitle(const char * pBuf)
	{ return Helper_SetTitle(pBuf, 0); }
void FASTCALL TWindow::setOrgTitle(const char * pBuf)
	{ return Helper_SetTitle(pBuf, 1); }
const SString & TWindow::getTitle() const
	{ return Title; }

void FASTCALL TWindow::Helper_SetTitle(const char * pBuf, int setOrgTitle)
{
	if(!isempty(pBuf)) { // @v8.8.4
		SString temp_title = pBuf;
		HWND   title_wnd = GetDlgItem(HW, SPEC_TITLEWND_ID);
		Title = temp_title;
		if(setOrgTitle)
			OrgTitle = Title;
		SETIFZ(title_wnd, HW);
		temp_title.Transf(CTRANSF_INNER_TO_OUTER);
		TView::SSetWindowText(HW, temp_title);
		APPL->UpdateItemInMenu(temp_title, this);
	}
}

void FASTCALL TWindow::setSubTitle(const char * pBuf)
{
	char   temp_buf[32];
	if(pBuf == 0) {
		temp_buf[0] = 0;
		pBuf = temp_buf;
	}
	if(Title.NotEmpty() || OrgTitle.NotEmpty()) {
		SString temp_title;
		setTitle(temp_title.Printf(OrgTitle.NotEmpty() ? OrgTitle : Title, pBuf));
	}
	else
		setTitle(pBuf);
}

void TWindow::showLocalMenu()
{
	if(Toolbar.getItemsCount()) {
		TMenuPopup menu;
		ToolbarItem item;
		for(uint i = 0; Toolbar.enumItems(&i, &item);) {
			if(item.KeyCode != TV_MENUSEPARATOR)
				menu.Add(item.ToolTipText, item.KeyCode);
			else if(i < Toolbar.getItemsCount()) // Последний разделитель не заносим
				menu.AddSeparator();
		}
		menu.Execute(HW);
	}
}

int SLAPI TWindow::translateKeyCode(ushort keyCode, uint * pCmd) const
{
	int    ok = 0;
	uint   idx = 0;
	if(Toolbar.searchKeyCode(keyCode, &idx)) {
		const ToolbarItem & r_item = Toolbar.getItem(idx);
		if(r_item.Cmd) {
			ASSIGN_PTR(pCmd, (uint)r_item.Cmd);
			ok = 1;
		}
	}
	return ok;
}

void TWindow::setupToolbar(const ToolbarList * pToolbar)
{
	if(!RVALUEPTR(Toolbar, pToolbar))
		Toolbar.clearAll();
}

HWND TWindow::showToolbar()
{
	HWND   h_tool_bar = CreateWindowEx(0, TOOLBARCLASSNAME, 0, WS_CHILD|TBSTYLE_TOOLTIPS|CCS_TOP|WS_CLIPSIBLINGS,
	   0, 0, 0, 0, H(), (HMENU)1, TProgram::GetInst(), 0);
	/*
	SendMessage(h_tool_bar, TB_BUTTONSTRUCTSIZE, (WPARAM)sizeof(TBBUTTON), 0);

	TBADDBITMAP tbab;
	tbab.hInst  = HINST_COMMCTRL;
	tbab.nID    = IDB_STD_SMALL_COLOR;
	std_images  = SendMessage(h_tool_bar, TB_ADDBITMAP, 15, (WPARAM)&tbab);
	tbab.hInst  = TProgram::GetInst();
	tbab.nID    = Toolbar.getBitmap();
	user_images = SendMessage(h_tool_bar, TB_ADDBITMAP, 100, (WPARAM)&tbab);
	*/

	HIMAGELIST himl = ImageList_Create(GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), ILC_COLOR32, 16, 64);
 	SendMessage(h_tool_bar, CCM_SETVERSION, (WPARAM) 5, 0);

	ToolbarItem item;
	for(uint i = 0; Toolbar.enumItems(&i, &item);) {
		if(!(item.Flags & ToolbarItem::fHidden)) {
			TBBUTTON btns;
			/*
			btns.fsState = TBSTATE_ENABLED;
			btns.dwData  = 0;
			btns.iString = 0;
			*/
			if(item.ToolTipText[0] != 0) {
				long image_idx = -1;
				if((image_idx = ImageList_Add(himl, APPL->FetchBitmap(item.BitmapIndex), (HBITMAP)0)) >= 0)
					btns.iBitmap = MAKELONG(1, image_idx);
				/*
				btns.iBitmap = item.BitmapIndex + ((item.BitmapIndex < TB_ICON_USER) ? std_images : (user_images-TB_ICON_USER));
				btns.fsStyle   = TBSTYLE_BUTTON;
				btns.idCommand = item.KeyCode;
				*/
			}
			else {
				btns.iBitmap   = 0;
				btns.fsStyle   = TBSTYLE_SEP;
				btns.idCommand = 0;
			}
			SendMessage(h_tool_bar, TB_ADDBUTTONS, 1, (LPARAM)(&btns));
		}
	}
	SendMessage(h_tool_bar, TB_SETIMAGELIST, 0, (LPARAM)himl);
	ShowWindow(h_tool_bar, SW_SHOW);
	return h_tool_bar;
}

IMPL_HANDLE_EVENT(TWindow)
{
	TGroup::handleEvent(event);
	if(event.isCmd(cmClose)) {
		if(!TVINFOPTR || TVINFOPTR == this || TVINFOVIEW->P_Owner == this) { // @v9.8.12 (|| TVINFOVIEW->owner == this)
			if(IsInState(sfModal))
				TView::messageCommand(this, cmCancel, this);
			else
				close();
		}
		else
			return;
	}
	else if(event.isCmd(cmLocalMenu)) {
		if(P_Lmp && event.getCtlID())
			P_Lmp->ShowMenu(event.getCtlID());
		else
			return;
	}
	else if(event.isKeyDown(kbShiftF10))
		showLocalMenu();
	else
		return;
	clearEvent(event);
}

void TWindow::setState(uint aState, bool enable)
{
	TGroup::setState(aState, enable);
	if(aState & sfSelected)
		setState(sfActive, enable); // @recursion
}
//
//
//
TRect TWindow::getClientRect() const
{
	TRect ret;
	RECT r;
	if(::GetClientRect(HW, &r))
		ret = r;
	return ret;
}

TRect TWindow::getRect() const
{
	TRect ret;
	RECT r;
	if(::GetWindowRect(HW, &r))
		ret = r;
	return ret;
}

int TWindow::invalidateRect(const TRect & rRect, int erase)
{
	RECT r = rRect;
	return ::InvalidateRect(HW, &r, erase ? TRUE : FALSE);
}

int TWindow::invalidateRegion(const SRegion & rRgn, int erase)
{
	const SHandle * p_hdl = (SHandle *)&rRgn; // @trick
	const HRGN h_rgn = (HRGN)(void *)*p_hdl;
	return ::InvalidateRgn(HW, h_rgn, erase);
}

void FASTCALL TWindow::invalidateAll(int erase)
{
	::InvalidateRect(HW, 0, erase ? TRUE : FALSE);
}

int TWindow::RegisterMouseTracking(int leaveNotify, int hoverTimeout)
{
	TRACKMOUSEEVENT tme;
	MEMSZERO(tme);
	tme.cbSize = sizeof(tme);
	if(!leaveNotify && hoverTimeout < 0)
		tme.dwFlags |= TME_CANCEL;
	else {
		if(leaveNotify)
			tme.dwFlags |= TME_LEAVE;
		if(hoverTimeout >= 0) {
			tme.dwFlags |= TME_HOVER;
			tme.dwHoverTime = (hoverTimeout == 0) ? HOVER_DEFAULT : hoverTimeout;
		}
		else
			tme.dwHoverTime = HOVER_DEFAULT;
	}
	tme.hwndTrack = HW;
	return BIN(::_TrackMouseEvent(&tme)); // win sdk commctrl
}
//
//
//
void SRectLayout::Dim::Set(int v, int f)
{
	Val = (int16)v;
	Flags = (int16)f;
}

SRectLayout::Item::Item()
{
	THISZERO();
	EmptyWidth = -1;
	EmptyHeight = -1;
	InnerOrder = SRectLayout::inoOverlap;
}

SRectLayout::Item & SRectLayout::Item::SetLeft(int size, int pct)
{
	Left.Set(0, SRectLayout::dfAbs|SRectLayout::dfGravity);
	if(pct)
		Right.Set(size * 100, SRectLayout::dfRel); // Сотые доли от размера контейнера
	else
		Right.Set(size, SRectLayout::dfAbs);
	Top.Set(0, SRectLayout::dfAbs);
	Bottom.Set(0, SRectLayout::dfAbs|SRectLayout::dfOpp);
	return *this;
}

SRectLayout::Item & SRectLayout::Item::SetRight(int size, int pct)
{
	if(pct)
		Left.Set(size * 100, SRectLayout::dfRel|SRectLayout::dfOpp);
	else
		Left.Set(size, SRectLayout::dfAbs|SRectLayout::dfOpp);
	Right.Set(0, SRectLayout::dfAbs|SRectLayout::dfGravity|SRectLayout::dfOpp);
	Top.Set(0, SRectLayout::dfAbs);
	Bottom.Set(0, SRectLayout::dfAbs|SRectLayout::dfOpp);
	return *this;
}

SRectLayout::Item & SRectLayout::Item::SetTop(int size, int pct)
{
	Left.Set(0, SRectLayout::dfAbs);
	Right.Set(0, SRectLayout::dfAbs|SRectLayout::dfOpp);
	Top.Set(0, SRectLayout::dfAbs|SRectLayout::dfGravity);
	if(pct)
		Bottom.Set(size * 100, SRectLayout::dfRel);
	else
		Bottom.Set(size, SRectLayout::dfAbs);
	return *this;
}

SRectLayout::Item & SRectLayout::Item::SetBottom(int size, int pct)
{
	Left.Set(0, SRectLayout::dfAbs);
	Right.Set(0, SRectLayout::dfAbs|SRectLayout::dfOpp);
	if(pct)
		Top.Set(size * 100, SRectLayout::dfRel|SRectLayout::dfOpp);
	else
		Top.Set(size, SRectLayout::dfAbs|SRectLayout::dfOpp);
	Bottom.Set(0, SRectLayout::dfAbs|SRectLayout::dfGravity|SRectLayout::dfOpp);
	return *this;
}

SRectLayout::Item & SRectLayout::Item::SetCenter()
{
	Left.Set(0, SRectLayout::dfAbs);
	Right.Set(0, SRectLayout::dfAbs|SRectLayout::dfOpp);
	Top.Set(0, SRectLayout::dfAbs);
	Bottom.Set(0, SRectLayout::dfAbs|SRectLayout::dfOpp);
	return *this;
}

SRectLayout::SRectLayout()
{
}

SRectLayout::~SRectLayout()
{
}

int SRectLayout::Add(long itemId, const SRectLayout::Item & rItem)
{
	int    ok = 1;
	if(!List.lsearch(&itemId, 0, CMPF_LONG)) {
		RItem new_item;
		MEMSZERO(new_item);
		new_item.Id = itemId;
		new_item.Left = rItem.Left;
		new_item.Top = rItem.Top;
		new_item.Right = rItem.Right;
		new_item.Bottom = rItem.Bottom;
		new_item.EmptyWidth = rItem.EmptyWidth;
		new_item.EmptyHeight = rItem.EmptyHeight;
		new_item.InnerOrder = rItem.InnerOrder;
		List.insert(&new_item);
	}
	else
		ok = 0;
	return ok;
}

int SRectLayout::SetContainerBounds(const TRect & rRect)
{
	int    rearrange = 0;
	if(ContainerBounds.width() != rRect.width() || ContainerBounds.height() != rRect.height())
		rearrange = 1;
	ContainerBounds = rRect;
	if(rearrange)
		Arrange();
	return 1;
}

int SRectLayout::IsEmpty(long itemId) const
{
	uint   pos = 0;
	return WinList.lsearch(&itemId, &pos, PTR_CMPFUNC(long)) ? 0 : 1;
}

int SRectLayout::InsertWindow(long itemId, TView * pView, int minWidth, int minHeight)
{
	int    ok = 1;
	if(List.lsearch(&itemId, 0, CMPF_LONG)) {
		WItem  witem;
		MEMSZERO(witem);
		witem.ItemId = itemId;
		witem.P_View = pView;
		witem.MinWidth = (int16)minWidth;
		witem.MinHeight = (int16)minHeight;
		WinList.insert(&witem);
	}
	else
		ok = 0;
	return ok;
}

int SRectLayout::CalcCoord(Dim dim, int containerLow, int containerUpp, int gravitySide) const
{
	int    p = 0;
	if(dim.Flags & dfGravity)
		p = gravitySide ? containerUpp : containerLow;
	else if(dim.Flags & dfRel) {
		int    z = ((containerUpp - containerLow) * dim.Val) / 10000;
		if(dim.Flags & dfOpp)
			p = containerLow + ((containerUpp - containerLow) - z);
		else
			p = containerLow + z;
	}
	else {
		if(dim.Flags & dfOpp)
			p = containerUpp - dim.Val;
		else
			p = containerLow + dim.Val;
	}
	return p;
}

int SRectLayout::Locate(TPoint p, uint * pItemPos) const
{
	int    ret = 0;
	for(uint i = 0; !ret && i < List.getCount(); i++)
		ret = List.at(i).Bounds.contains(p);
	return ret;
}

int SRectLayout::Arrange()
{
	int    ok = 1;
	TRect  bounds = ContainerBounds;
	for(uint i = 0; i < List.getCount(); i++) {
		RItem & r_item = List.at(i);
		r_item.Bounds.a.x = CalcCoord(r_item.Left,   ContainerBounds.a.x,  ContainerBounds.b.x, 0);
		r_item.Bounds.a.y = CalcCoord(r_item.Top,    ContainerBounds.a.y,  ContainerBounds.b.y, 0);
		r_item.Bounds.b.x = CalcCoord(r_item.Right,  ContainerBounds.a.x,  ContainerBounds.b.x, 1);
		r_item.Bounds.b.y = CalcCoord(r_item.Bottom, ContainerBounds.a.y,  ContainerBounds.b.y, 1);
		if(IsEmpty(r_item.Id)) {
			if(r_item.EmptyWidth >= 0) {
				if(r_item.Left.Flags & dfGravity)
					r_item.Bounds.b.x = r_item.Bounds.a.x + r_item.EmptyWidth;
				else if(r_item.Right.Flags & dfGravity)
					r_item.Bounds.a.x = r_item.Bounds.b.x - r_item.EmptyWidth;
			}
			if(r_item.EmptyHeight >= 0) {
				if(r_item.Top.Flags & dfGravity)
					r_item.Bounds.b.y = r_item.Bounds.a.y + r_item.EmptyHeight;
				else if(r_item.Bottom.Flags & dfGravity)
					r_item.Bounds.a.y = r_item.Bounds.b.y - r_item.EmptyHeight;
			}
		}
		for(uint j = 0; j < i; j++) {
			TRect isect;
			if(r_item.Bounds.Intersect(List.at(j).Bounds, &isect) > 0) {
				int    wleft = isect.a.x - r_item.Bounds.a.x;
				int    wright = r_item.Bounds.b.x - isect.b.x;
				int    hupp = isect.a.y - r_item.Bounds.a.y;
				int    hlow = r_item.Bounds.b.y - isect.b.y;
				int    s1 = wleft * r_item.Bounds.height();
				int    s2 = wright * r_item.Bounds.height();
				int    s3 = r_item.Bounds.width() * hlow;
				int    s4 = r_item.Bounds.width() * hupp;
				int    m = MAX(MAX(s1, s2), MAX(s3, s4));
				if(m == s1)
					r_item.Bounds.b.x = isect.a.x;
				else if(m == s2)
					r_item.Bounds.a.x = isect.b.x;
				else if(m == s3)
					r_item.Bounds.a.y = isect.b.y;
				else if(m == s4)
					r_item.Bounds.b.y = isect.a.y;
			}
		}
		for(uint j = 0; j < WinList.getCount(); j++) {
			WItem & r_witem = WinList.at(j);
			if(r_witem.ItemId == r_item.Id) {
				r_witem.Bounds = r_item.Bounds;
				if(r_witem.P_View) {
					r_witem.P_View->changeBounds(r_witem.Bounds);
				}
			}
		}
	}
	return ok;
}
//
//
//
TScrollBlock::TScrollBlock()
{
	Rx = 0;
	Ry = 0;
	ScX = ScY = 0;
}

int TScrollBlock::Set(int x, int y)
{
	int    ok = -1;
	const int prev_sc_x = ScX;
	const int prev_sc_y = ScY;
	ScX = MIN(Rx.upp, MAX(Rx.low, x));
	ScY = MIN(Ry.upp, MAX(Ry.low, y));
	return (ok && (prev_sc_x != ScX || prev_sc_y != ScY)) ? 1 : ok;
}

int TScrollBlock::MoveToEdge(int side)
{
	int    ok = -1;
	const int prev_sc_x = ScX;
	const int prev_sc_y = ScY;
	switch(side) {
		case SIDE_LEFT:   ScX = Rx.low; break;
		case SIDE_RIGHT:  ScX = Rx.upp; break;
		case SIDE_TOP:    ScY = Ry.low; break;
		case SIDE_BOTTOM: ScY = Ry.upp; break;
		default: ok = 0; break;
	}
	return (ok && (prev_sc_x != ScX || prev_sc_y != ScY)) ? 1 : ok;
}

int TScrollBlock::Move(int side, int delta)
{
	int    ok = -1;
	const int prev_sc_x = ScX;
	const int prev_sc_y = ScY;
	switch(side) {
		case SIDE_LEFT:
			ScX = MAX(Rx.low, (ScX-delta));
			break;
		case SIDE_RIGHT:
			ScX = MIN(Rx.upp, (ScX+delta));
			break;
		case SIDE_TOP:
			ScY = MAX(Ry.low, (ScY-delta));
			break;
		case SIDE_BOTTOM:
			ScY = MIN(Ry.upp, (ScY+delta));
			break;
		default:
			ok = 0;
			break;
	}
	return (ok && (prev_sc_x != ScX || prev_sc_y != ScY)) ? 1 : ok;
}

int TScrollBlock::SetupWindow(HWND hWnd) const
{
	int    ok = 1;
	SCROLLINFO si;
	MEMSZERO(si);
	si.cbSize = sizeof(si);
	si.fMask = SIF_POS | SIF_RANGE;
	si.nMin = Ry.low;
	si.nMax = Ry.upp;
	si.nPos = MIN(si.nMax, ScY);
	::SetScrollInfo(hWnd, SB_VERT, &si, TRUE);
	si.nMin = Rx.low;
	si.nMax = Rx.upp;
	si.nPos = MIN(si.nMax, ScX);
	::SetScrollInfo(hWnd, SB_HORZ, &si, TRUE);
	return ok;
}
//
//
//
//static
const char * TWindowBase::P_ClsName = "SLibWindowBase";

//static
int TWindowBase::RegWindowClass(int iconId)
{
	WNDCLASSEX wc;
	const HINSTANCE h_inst = TProgram::GetInst();
	if(!::GetClassInfoEx(h_inst, TWindowBase::P_ClsName, &wc)) { // @unicodeproblem
		MEMSZERO(wc);
		wc.cbSize        = sizeof(wc);
		wc.lpszClassName = TWindowBase::P_ClsName; // @unicodeproblem
		wc.hInstance     = h_inst;
		wc.lpfnWndProc   = TWindowBase::WndProc;
		wc.style         = /*CS_HREDRAW | CS_VREDRAW |*/ /*CS_OWNDC |*/ CS_SAVEBITS | CS_DBLCLKS;
		wc.hIcon         = iconId ? LoadIcon(h_inst, MAKEINTRESOURCE(/*ICON_MAIN_P2*/ /*102*/iconId)) : 0;
		wc.cbClsExtra    = BRWCLASS_CEXTRA;
		wc.cbWndExtra    = BRWCLASS_WEXTRA;
		return ::RegisterClassEx(&wc); // @unicodeproblem
	}
	else
		return -1;
}

TWindowBase::TWindowBase(int capability) : TWindow(TRect(), 0, 0)
{
	WbState = 0;
	WbCapability = capability;
	H_DrawBuf = 0;
}

TWindowBase::~TWindowBase()
{
	ZDeleteWinGdiObject(&H_DrawBuf);
	// @v8.0.3 {
	if(::IsWindow(HW)) {
		TWindowBase * p_this_view_from_wnd = (TWindowBase *)TView::GetWindowUserData(HW);
		if(p_this_view_from_wnd) {
			Sf |= sfOnDestroy;
			::DestroyWindow(HW);
			HW = 0;
		}
	}
	// } @v8.0.3
}

int TWindowBase::Create(long parent, long createOptions)
{
	const HINSTANCE h_inst = TProgram::GetInst();
	TWindowBase::RegWindowClass(102);

	SString title_buf = getTitle();
	title_buf.SetIfEmpty(P_ClsName).Transf(CTRANSF_INNER_TO_OUTER);
	HWND  hw_parent = (HWND)parent;
	DWORD style = WS_HSCROLL | WS_VSCROLL /*| WS_CLIPSIBLINGS | WS_CLIPCHILDREN*/;
	int   x = size.x ? origin.x : CW_USEDEFAULT;
	int   y = size.y ? origin.y : CW_USEDEFAULT;
	int   cx = NZOR(size.x, CW_USEDEFAULT);
	int   cy = NZOR(size.y, CW_USEDEFAULT);
	if(APPL->H_MainWnd && createOptions & coMaxSize) {
		RECT par_rect, r;
		GetWindowRect(APPL->H_MainWnd, &par_rect);
		APPL->GetStatusBarRect(&r);
		int    status_height = r.bottom - r.top;
		int    tree_width = 0;
		if(APPL->IsTreeVisible()) {
			APPL->GetTreeRect(r);
			tree_width = r.right - r.left;
		}
		const int par_wd = par_rect.right-par_rect.left;
		const int par_ht = par_rect.bottom-par_rect.top;
		const int border_x = GetSystemMetrics(SM_CXBORDER);
		const int border_y = GetSystemMetrics(SM_CYBORDER);
		const int menu_y = GetSystemMetrics(SM_CYMENU);
		const int cap_y = GetSystemMetrics(SM_CYCAPTION);
		x   = tree_width + border_x + par_rect.left;
		y   = border_y + cap_y + menu_y + par_rect.top;
		cx  = par_wd - border_x * 2 - tree_width - x;
		cy  = par_ht - border_y * 2 - cap_y - menu_y - status_height - y;
		/*
		r.left   = par_wd * origin.x / 80 + tree_width;
		r.right  = par_wd * size.x / 80 - border_x * 2 - tree_width;
		r.top    = par_ht * origin.y / 23;
		r.bottom = par_ht * size.y / 23 - border_y * 2 - cap_y - menu_y - status_height;
		r.left += border_x + parent.left;
		r.top  += border_y + cap_y + menu_y + parent.top;
		*/
	}
	else {

	}
	if(createOptions & coChild) {
		style = WS_CHILD|WS_TABSTOP;
		HW = CreateWindowEx(WS_EX_CLIENTEDGE, P_ClsName, title_buf, style, 0, 0, cx, cy, hw_parent, 0, h_inst, this); // @unicodeproblem
	}
	else { // coPopup
		style = WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_VISIBLE;
		if(createOptions & coMDI) {
			MDICREATESTRUCT child;
			child.szClass = P_ClsName; // @unicodeproblem
			child.szTitle = title_buf; // @unicodeproblem
			child.hOwner = h_inst;
			child.x  = CW_USEDEFAULT;
			child.y  = CW_USEDEFAULT;
			child.cx = CW_USEDEFAULT;
			child.cy = CW_USEDEFAULT;
			child.style  = style;
			child.lParam = (LPARAM)this;
			HW = (HWND)LOWORD(SendMessage(hw_parent, WM_MDICREATE, 0, (LPARAM)&child)); // @unicodeproblem
		}
		else {
			HW = CreateWindowEx(0, P_ClsName, title_buf, style, x, y, cx, cy, hw_parent, 0, h_inst, this); // @unicodeproblem
		}
	}
	return BIN(HW);
}

int TWindowBase::AddChild(TWindowBase * pWin, long createOptions, long zone)
{
	int    ok = 1;
	if(pWin) {
		pWin->Create((long)HW, createOptions);
		if(zone)
			Layout.InsertWindow(zone, pWin, 0, 0);
		TWindow::Insert_(pWin);
		Layout.Arrange();
		::ShowWindow(pWin->H(), SW_SHOWNORMAL);
		::UpdateWindow(H());
	}
	return ok;
}

IMPL_HANDLE_EVENT(TWindowBase)
{
	if(event.isCmd(cmExecute)) {
		ushort last_command = 0;
		SString buf = getTitle();
		buf.Transf(CTRANSF_INNER_TO_OUTER);
		if(::IsIconic(APPL->H_MainWnd))
			::ShowWindow(APPL->H_MainWnd, SW_MAXIMIZE);
		if(APPL->H_MainWnd) {
			if(IsMDIClientWindow(APPL->H_MainWnd)) {
				Create((long)APPL->H_MainWnd, coMDI);
			}
			else {
				Create((long)APPL->H_TopOfStack, coPopup | coMaxSize);
			}
		}
		::ShowWindow(HW, SW_NORMAL);
		::UpdateWindow(HW);
		if(APPL->PushModalWindow(this, HW)) {
			::EnableWindow(PrevInStack, 0);
			APPL->MsgLoop(this, EndModalCmd);
			last_command = EndModalCmd;
			EndModalCmd = 0;
			APPL->PopModalWindow(this, 0);
		}
		clearEvent(event);
		event.message.infoLong = last_command;
	}
	else {
		TWindow::handleEvent(event);
		if(TVINFOPTR) {
			if(event.isCmd(cmInit)) {
				CreateBlock * p_blk = (CreateBlock *)TVINFOPTR;
				Layout.SetContainerBounds(getClientRect());
			}
			else if(event.isCmd(cmSetBounds)) {
				const TRect * p_rc = (const TRect *)TVINFOPTR;
				::SetWindowPos(H(), 0, p_rc->a.x, p_rc->a.y, p_rc->width(), p_rc->height(), SWP_NOZORDER|SWP_NOREDRAW);
				clearEvent(event);
			}
			else if(event.isCmd(cmSize)) {
				SizeEvent * p_se = (SizeEvent *)TVINFOPTR;
				Layout.SetContainerBounds(getClientRect());
				invalidateAll(1);
				::UpdateWindow(H());
			}
		}
	}
}

int TWindowBase::SetDefaultCursor()
{
	::SetCursor(::LoadCursor(0, IDC_ARROW));
	return 1;
}

void TWindowBase::RegisterMouseTracking(int force)
{
	if(force || !(WbState & wbsMouseTrackRegistered)) {
		TWindow::RegisterMouseTracking(1, 0);
		WbState |= wbsMouseTrackRegistered;
	}
}

int TWindowBase::MakeMouseEvent(uint msg, WPARAM wParam, LPARAM lParam, MouseEvent & rMe)
{
	MEMSZERO(rMe);
	rMe.Coord.setwparam(lParam);
	switch(msg) {
		case WM_LBUTTONDOWN:
			rMe.Type = MouseEvent::tLDown;
			::SetCapture(HW);
			break;
		case WM_LBUTTONUP:
			rMe.Type = MouseEvent::tLUp;
			::ReleaseCapture();
			break;
		case WM_LBUTTONDBLCLK: rMe.Type = MouseEvent::tLDblClk; break;
		case WM_RBUTTONDOWN:   rMe.Type = MouseEvent::tRDown; break;
		case WM_RBUTTONUP:     rMe.Type = MouseEvent::tRUp; break;
		case WM_RBUTTONDBLCLK: rMe.Type = MouseEvent::tRDblClk; break;
		case WM_MBUTTONDOWN:   rMe.Type = MouseEvent::tMDown; break;
		case WM_MBUTTONUP:     rMe.Type = MouseEvent::tMUp; break;
		case WM_MBUTTONDBLCLK: rMe.Type = MouseEvent::tMDblClk; break;
		case WM_MOUSEHOVER:
			WbState &= ~wbsMouseTrackRegistered;
			rMe.Type = MouseEvent::tHover;
			break;
		case WM_MOUSELEAVE:
			WbState &= ~wbsMouseTrackRegistered;
			rMe.Type = MouseEvent::tLeave;
			break;
		case WM_MOUSEMOVE:
			if(getClientRect().contains(rMe.Coord))
				RegisterMouseTracking(0);
			rMe.Type = MouseEvent::tMove;
			break;
		case WM_MOUSEWHEEL:
			rMe.Type = MouseEvent::tWeel;
			rMe.WeelDelta = HIWORD(wParam);
			break;
		default:
			return 0;
	}
	if(wParam & MK_CONTROL)
		rMe.Flags |= MouseEvent::fControl;
	if(wParam & MK_LBUTTON)
		rMe.Flags |= MouseEvent::fLeft;
	if(wParam & MK_MBUTTON)
		rMe.Flags |= MouseEvent::fMiddle;
	if(wParam & MK_RBUTTON)
		rMe.Flags |= MouseEvent::fRight;
	if(wParam & MK_SHIFT)
		rMe.Flags |= MouseEvent::fShift;
	if(wParam & MK_XBUTTON1)
		rMe.Flags |= MouseEvent::fX1;
	if(wParam & MK_XBUTTON2)
		rMe.Flags |= MouseEvent::fX2;
	return 1;
}

//static
LRESULT CALLBACK TWindowBase::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	TWindowBase * p_view = (TWindowBase *)TView::GetWindowUserData(hWnd);
	LPCREATESTRUCT p_init_data;
	switch(message) {
		case WM_HELP:
			{
				HelpEvent he;
				MEMSZERO(he);
				const HELPINFO * p_hi = (const HELPINFO *)lParam;
				if(p_hi) {
					if(p_hi->iContextType == HELPINFO_MENUITEM)
						he.ContextType = HelpEvent::ctxtMenu;
					else if(p_hi->iContextType == HELPINFO_WINDOW)
						he.ContextType = HelpEvent::ctxtWindow;
					he.CtlId = p_hi->iCtrlId;
					he.H_Item = (long)p_hi->hItemHandle;
					he.ContextId = p_hi->dwContextId;
					he.Mouse = p_hi->MousePos;
				}
				TView::messageCommand(p_view, cmHelp, &he);
			}
			break;
		case WM_CREATE:
			{
				CreateBlock cr_blk;
				MEMSZERO(cr_blk);
				p_init_data = (LPCREATESTRUCT)lParam;
				if(IsMDIClientWindow(p_init_data->hwndParent)) {
					MDICREATESTRUCT * p_mdi_init_data = (LPMDICREATESTRUCT)(p_init_data->lpCreateParams);
					p_view = (TWindowBase *)(p_mdi_init_data)->lParam;
					p_view->WbState |= wbsMDI;
					cr_blk.Coord.setwidthrel(p_mdi_init_data->x, p_mdi_init_data->cx);
					cr_blk.Coord.setheightrel(p_mdi_init_data->y, p_mdi_init_data->cy);
					cr_blk.Param = p_mdi_init_data->lParam;
					cr_blk.H_Process = (long)p_mdi_init_data->hOwner;
					cr_blk.Style = p_mdi_init_data->style;
					cr_blk.ExStyle = 0;
					cr_blk.H_Parent = 0;
					cr_blk.H_Menu = 0;
					cr_blk.P_WndCls = p_mdi_init_data->szClass; // @unicodeproblem
					cr_blk.P_Title = p_mdi_init_data->szTitle; // @unicodeproblem
				}
				else {
					p_view = (TWindowBase *)p_init_data->lpCreateParams;
					p_view->WbState &= ~wbsMDI;
					cr_blk.Coord.setwidthrel(p_init_data->x, p_init_data->cx);
					cr_blk.Coord.setheightrel(p_init_data->y, p_init_data->cy);
					cr_blk.Param = (long)p_init_data->lpCreateParams;
					cr_blk.H_Process = (long)p_init_data->hInstance;
					cr_blk.Style = p_init_data->style;
					cr_blk.ExStyle = p_init_data->dwExStyle;
					cr_blk.H_Parent = (long)p_init_data->hwndParent;
					cr_blk.H_Menu = (long)p_init_data->hMenu;
					cr_blk.P_WndCls = p_init_data->lpszClass; // @unicodeproblem
					cr_blk.P_Title = p_init_data->lpszName; // @unicodeproblem
				}
				if(p_view) {
					p_view->HW = hWnd;
					p_view->origin.Set(p_init_data->x, p_init_data->y);
					p_view->size.Set(p_init_data->cx, p_init_data->cy);
					//p_view->RegisterMouseTracking(1);
					TView::SetWindowUserData(hWnd, p_view);
					TView::messageCommand(p_view, cmInit, &cr_blk);
					return 0;
				}
				else
					return -1;
			}
		case WM_DESTROY:
			if(p_view) {
				SETIFZ(p_view->EndModalCmd, cmCancel);
				p_view->ResetOwnerCurrent();
				if(!p_view->IsInState(sfModal)) {
					APPL->P_DeskTop->remove(p_view);
					if(!(p_view->Sf & sfOnDestroy)) {
						delete p_view;
						TView::SetWindowUserData(hWnd, (void *)0);
					}
				}
			}
			return 0;
		case WM_SETFONT:
			{
				SetFontEvent sfe;
				sfe.FontHandle = wParam;
				sfe.DoRedraw = LOWORD(lParam);
				if(TView::messageCommand(p_view, cmSetFont, &sfe))
					return 0;
			}
			break;
		case WM_SIZE:
			{
				SizeEvent se;
				switch(wParam) {
					case SIZE_MAXHIDE:   se.ResizeType = SizeEvent::tMaxHide;   break;
					case SIZE_MAXIMIZED: se.ResizeType = SizeEvent::tMaximized; break;
					case SIZE_MAXSHOW:   se.ResizeType = SizeEvent::tMaxShow;   break;
					case SIZE_MINIMIZED: se.ResizeType = SizeEvent::tMinimized; break;
					case SIZE_RESTORED:  se.ResizeType = SizeEvent::tRestored;  break;
					default: se.ResizeType = 0; break;
				}
				se.PrevSize = p_view->size;
				p_view->size = se.NewSize.setwparam(lParam);
				if(TView::messageCommand(p_view, cmSize, &se))
					return 0;
			}
			break;
		case WM_MOVE:
			{
				p_view->origin.setwparam(lParam);
				if(TView::messageCommand(p_view, cmMove))
					return 0;
			}
			break;
		case WM_PAINT:
			if(p_view) {
				PAINTSTRUCT ps;
				PaintEvent pe;
				MEMSZERO(pe);
				pe.PaintType = PaintEvent::tPaint;
				BeginPaint(hWnd, &ps);

				SETFLAG(pe.Flags, PaintEvent::fErase, ps.fErase);
				pe.Rect = ps.rcPaint;

				int    use_draw_buf = 0;
				RECT   cr;
				HDC    h_dc_mem = 0;
				HBITMAP h_bmp = 0;
				HBITMAP h_old_bmp = 0;
				if(((TWindowBase *)p_view)->WbCapability & wbcDrawBuffer) {
					GetClientRect(hWnd, &cr);
					h_dc_mem = CreateCompatibleDC(ps.hdc);
					h_bmp = CreateCompatibleBitmap(ps.hdc, cr.right - cr.left, cr.bottom - cr.top);
					h_old_bmp = (HBITMAP)SelectObject(h_dc_mem, h_bmp);
					use_draw_buf = 1;
					pe.H_DeviceContext = (long)h_dc_mem;
				}
				else
					pe.H_DeviceContext = (long)ps.hdc;
				void * p_ret = TView::messageCommand(p_view, cmPaint, &pe);
				if(use_draw_buf) {
					BitBlt(ps.hdc, 0, 0, cr.right - cr.left, cr.bottom - cr.top, h_dc_mem, 0, 0, SRCCOPY);
					SelectObject(h_dc_mem, h_old_bmp);
					ZDeleteWinGdiObject(&h_bmp);
					DeleteDC(h_dc_mem);
				}
				EndPaint(hWnd, &ps);
				if(p_ret)
					return 0;
			}
			break;
		case WM_NCPAINT:
			{
				PaintEvent pe;
				MEMSZERO(pe);
				pe.PaintType = PaintEvent::tNcPaint;
				HDC hdc = GetDCEx(hWnd, (HRGN)wParam, DCX_WINDOW|DCX_INTERSECTRGN);
				pe.H_DeviceContext = (long)hdc;
				void * p_ret = TView::messageCommand(p_view, cmPaint, &pe);
				ReleaseDC(hWnd, hdc);
				if(p_ret)
					return 0;
			}
			break;
		case WM_ERASEBKGND:
			if(p_view) {
				PaintEvent pe;
				MEMSZERO(pe);
				pe.PaintType = PaintEvent::tEraseBackground;
				pe.H_DeviceContext = (long)wParam;
				pe.Rect = p_view->getClientRect();
				void * p_ret = TView::messageCommand(p_view, cmPaint, &pe);
				//
				// Если получатель очистил фон, он должен акцептировть сообщение.
				// Windows ожидает получить в этом случае !0.
				//
				return p_ret ? 1 : 0;
			}
			break;
		case WM_HSCROLL:
		case WM_VSCROLL:
			{
				ScrollEvent se;
				MEMSZERO(se);
				if(message == WM_HSCROLL)
					se.Dir = DIREC_HORZ;
				else if(message == WM_VSCROLL)
					se.Dir = DIREC_VERT;
				se.H_Wnd = lParam;
				switch(LOWORD(wParam)) {
					case SB_TOP:       se.Type = ScrollEvent::tTop; break;
					case SB_BOTTOM:    se.Type = ScrollEvent::tBottom; break;
					case SB_ENDSCROLL: se.Type = ScrollEvent::tEnd; break;
					case SB_LINEDOWN:  se.Type = ScrollEvent::tLineDown; break;
					case SB_LINEUP:    se.Type = ScrollEvent::tLineUp; break;
					case SB_PAGEDOWN:  se.Type = ScrollEvent::tPageDown; break;
					case SB_PAGEUP:    se.Type = ScrollEvent::tPageUp; break;
					case SB_THUMBPOSITION:
						se.Type = ScrollEvent::tThumbPos;
						se.TrackPos = HiWord(wParam);
						break;
					case SB_THUMBTRACK:
						se.Type = ScrollEvent::tThumbTrack;
						se.TrackPos = HiWord(wParam);
						break;
				}
				TView::messageCommand(p_view, cmScroll, &se);
			}
			break;
		case WM_LBUTTONDOWN:
			if(hWnd != GetFocus())
				SetFocus(hWnd);
		case WM_LBUTTONUP:
		case WM_LBUTTONDBLCLK:
		case WM_RBUTTONDOWN:
		case WM_RBUTTONUP:
		case WM_RBUTTONDBLCLK:
		case WM_MBUTTONDOWN:
		case WM_MBUTTONUP:
		case WM_MBUTTONDBLCLK:
		case WM_MOUSEMOVE:
		case WM_MOUSEWHEEL:
		case WM_MOUSELEAVE:
		case WM_MOUSEHOVER:
			if(p_view) {
				MouseEvent me;
				p_view->MakeMouseEvent(message, wParam, lParam, me);
				if(TView::messageCommand(p_view, cmMouse, &me))
					return 0;
			}
			break;
		case WM_KEYDOWN:
			p_view->HandleKeyboardEvent(LOWORD(wParam));
			return 0;
		case WM_CHAR:
			if(p_view && (wParam != VK_RETURN || LOBYTE(HIWORD(lParam)) != 0x1c)) {
				TEvent event;
				event.what = TEvent::evKeyDown;
				event.keyDown.keyCode = wParam;
				p_view->handleEvent(event);
			}
			return 0;
		default:
			break;
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}

