// TWINDOW.CPP  Turbo Vision 1.0
// Copyright (c) 1991 by Borland International
// @codepage UTF-8
// Modified and adopted by A.Sobolev 1996-2001, 2002, 2003, 2005, 2006, 2007, 2008, 2010, 2011, 2013, 2015, 2016, 2017, 2018, 2019, 2020, 2021, 2022, 2023, 2024, 2025
//
#include <slib-internal.h>
#pragma hdrstop

#define MPST_ERROR         0x0001
#define MPST_PREVSEPARATOR 0x0002

TMenuPopup::TMenuPopup() : State(0), H(::CreatePopupMenu())
{
}

TMenuPopup::~TMenuPopup()
{
	if(H) {
		::DestroyMenu(static_cast<HMENU>(H));
		H = 0;
	}
}

uint TMenuPopup::GetCount() const { return L.getCount(); }
int  TMenuPopup::Add(const char * pText, int cmd) { return Add(pText, cmd, 0); }

int TMenuPopup::Add(const char * pText, int cmd, int keyCode)
{
	int    ok = 0;
	assert(pText);
	if(H) {
		SString temp_buf;
		if(pText[0] == '@') {
			if(!SLS.LoadString_(pText+1, temp_buf))
				temp_buf = pText+1;
			else
				temp_buf.Transf(CTRANSF_INNER_TO_OUTER);
			pText = temp_buf;
		}
		Item new_item;
		new_item.Cmd = cmd;
		new_item.KeyCode = keyCode;
		uint new_item_id = L.getCount()+1;
		UINT flags = MF_STRING|MF_ENABLED;
		SETFLAG(flags, MF_SEPARATOR, BIN(cmd == TV_MENUSEPARATOR));
		ok = BIN(::AppendMenu(static_cast<HMENU>(H), flags, (cmd == TV_MENUSEPARATOR) ? 0 : /*cmd*/new_item_id, SUcSwitch(pText)));
		if(ok) {
			L.insert(&new_item);
		}
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
			ok = BIN(::AppendMenu(static_cast<HMENU>(H), MF_SEPARATOR, 0, 0));
			State |= MPST_PREVSEPARATOR;
		}
	}
	return ok;
}

int TMenuPopup::Execute(HWND hWnd, long flags, uint * pCmd, uint * pKeyCode)
{
	int    ok = 0;
	if(H) {
		POINT p;
		::GetCursorPos(&p);
		uint f = TPM_LEFTALIGN|TPM_LEFTBUTTON;
		if(flags & efRet)
			f |= TPM_RETURNCMD;
		uint item_id = ::TrackPopupMenu(static_cast<HMENU>(H), f, p.x, p.y, 0, hWnd, 0);
		if(item_id > 0 && item_id <= L.getCount()) {
			ASSIGN_PTR(pCmd, L.at(item_id-1).Cmd);
			ASSIGN_PTR(pKeyCode, L.at(item_id-1).KeyCode);
			ok = 1;
		}
	}
	return ok;
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
			const Item * p_item = static_cast<const Item *>(List.at(i));
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
			const Item * p_item = static_cast<const Item *>(List.at(i));
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
			const Item * p_item = static_cast<const Item *>(List.at(i));
			if(p_item->ButtonId == buttonId && p_item->StrPos) {
				if(p_item->CtrlId) {
					TView * p_view = P_Win->getCtrlView(p_item->CtrlId);
					if(p_view && p_view->IsInState(sfDisabled))
						ok = -1;
				}
				if(ok > 0) {
					StrPool.get(p_item->StrPos, text);
					menu.Add(text, 0, p_item->KeyCode);
					SETIFZ(sel, p_item->CtrlId);
				}
			}
		}
		if(ok > 0 && sel) {
			uint   cmd = 0;
			uint   key = 0;
			if(menu.Execute(P_Win->H(), menu.efRet, &cmd, &key)) {
				if(cmd) {
					P_Win->selectCtrl(sel);
					TView::messageCommand(P_Win, cmd);
				}
				else if(key) {
					P_Win->selectCtrl(sel);
					TView::messageKeyDown(P_Win, key);
				}
			}
		}
	}
	return ok;
}
//
//
//
ToolbarItem::ToolbarItem() : Cmd(0), KeyCode(0), Flags(0), BitmapIndex(0)
{
	ToolTipText[0] = 0;
}

ToolbarList::ToolbarList() : SVector(sizeof(ToolbarItem)), Bitmap(0) {}
void  ToolbarList::setBitmap(uint b) { Bitmap = b; }
uint  ToolbarList::getBitmap() const { return Bitmap; }
uint  ToolbarList::getItemsCount() const { return getCount(); }
const ToolbarItem & FASTCALL ToolbarList::getItem(uint idx) const { return *static_cast<const ToolbarItem *>(at(idx)); }
void  ToolbarList::clearAll() { freeAll(); }
int   ToolbarList::addItem(const ToolbarItem * pItem) { return pItem ? insert(pItem) : -1; }

ToolbarList & FASTCALL ToolbarList::operator = (const ToolbarList & s)
{
	Bitmap = s.Bitmap;
	SVector::copy(s);
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
	if(SVector::enumItems(pIdx, (void **)&p_item) > 0) {
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
				ToolbarItem item = *static_cast<const ToolbarItem *>(at(pos));
				atFree(pos);
				atInsert(pos-1, &item);
				ok = 1;
			}
		}
		else {
			if(pos < getCount()-1) {
				ToolbarItem item = *static_cast<const ToolbarItem *>(at(pos));
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
/*static*/int TWindow::IsMDIClientWindow(HWND h)
{
	SString cls_name;
	TView::SGetWindowClassName(h, cls_name);
	return (cls_name == "MDICLIENT");
}

/*static*/BOOL CALLBACK TWindow::SetupCtrlTextProc(HWND hwnd, LPARAM lParam)
{
	SString temp_buf;
	TView::SGetWindowText(hwnd, temp_buf);
	if(temp_buf.NotEmpty()) {
		SString subst;
		if(SLS.SubstString(temp_buf, 1, subst) > 0) {
			TView::SSetWindowText(hwnd, subst);
		}
		else if(SLS.ExpandString(temp_buf, CTRANSF_UTF8_TO_OUTER) > 0) {
			TView::SSetWindowText(hwnd, temp_buf);
		}
		else if(!temp_buf.IsAscii() && temp_buf.IsLegalUtf8()) {
			temp_buf.Transf(CTRANSF_UTF8_TO_OUTER);
			TView::SSetWindowText(hwnd, temp_buf);
		}
	}
	return TRUE;
}

/*static*/int TWindow::PassMsgToCtrl(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	int    ok = 0;
	TWindow * p_this = static_cast<TWindow *>(TView::GetWindowUserData(hwndDlg));
	if(p_this) {
		const short  cntlid = GetDlgCtrlID(reinterpret_cast<HWND>(lParam));
		TView * v = p_this->P_Last;
		if(v) do {
			if(v->TestId(CLUSTER_ID(LOWORD(cntlid)))) {
				ok = v->handleWindowsMessage(uMsg, wParam, lParam) ? 1 : -1;
				break;
			}
			v = v->prev();
		} while(v != p_this->P_Last);
	}
	return ok;
}

int TWindow::RedirectDrawItemMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	int    result = FALSE;
	if(H() && lParam) {
		DRAWITEMSTRUCT * p_dis = reinterpret_cast<DRAWITEMSTRUCT *>(lParam);
		TDrawItemData di;
		di.CtlType = p_dis->CtlType;
		di.CtlID   = p_dis->CtlID;
		di.ItemID  = p_dis->itemID;
		di.ItemAction = p_dis->itemAction;
		di.ItemState  = p_dis->itemState;
		di.H_Item   = p_dis->hwndItem;
		di.H_DC     = p_dis->hDC;
		di.ItemRect = p_dis->rcItem;
		di.P_View   = getCtrlView(LOWORD(di.CtlID));
		di.ItemData = p_dis->itemData;
		TView::messageCommand(this, cmDrawItem, &di);
		if(APPL->DrawControl(H(), uMsg, wParam, lParam) > 0)
			di.ItemAction = p_dis->itemAction;
		if(!di.ItemAction)
			result = TRUE;
	}
	return result;
}

TWindow::TWindow(const TRect & rRect) : 
	TGroup(rRect), WbCapability(0), P_Lmp(0), HW(0), PrevInStack(0), P_SymbList(0), P_FontsAry(0), P_Lfc(0)
{
	ViewOptions |= ofSelectable;
}

TWindow::TWindow(long wbCapability) : 
	// Здесь мы применяем искусственно-непустой прямоугольник из-за того, что некоторые функции валидации могут воспринять 
	// пустой прямоугольник как "сигнал бедствия" и отказаться работать дальше.
	TGroup(TRect(0, 0, 0, 25)), WbCapability(wbCapability), P_Lmp(0), HW(0), PrevInStack(0), P_SymbList(0), P_FontsAry(0), P_Lfc(0)
{
}

TWindow::~TWindow()
{
	delete P_Lmp;
	ZDELETE(P_SymbList);
	if(HW) {
		//
		// Обнуляем ссылку на this в системной структуре окна во
		// избежании попытки повторного удаления оконной процедурой.
		//
		TView::SetWindowUserData(HW, static_cast<void *>(0));
	}
	if(P_FontsAry) {
		for(uint c = 0; c < P_FontsAry->getCount(); c++)
			::DeleteObject(*static_cast<HFONT *>(P_FontsAry->at(c)));
		ZDELETE(P_FontsAry);
	}
	// @v12.2.4 (moved from TWindowBase::~TWindowBase()) {
	if(P_Lfc && !(Sf & (sfOnDestroy|sfOnParentDestruction))) {
		if(!P_Lfc->FatherKillMe())
			delete P_Lfc;
		P_Lfc = 0;
	}
	// } @v12.2.4
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

static bool searchItem(TView * v, void * ptr) { return v->TestId(*static_cast<const ushort *>(ptr)); }

const TView * FASTCALL TWindow::getCtrlViewC(ushort ctlID) const
{
	// Функция вызывается очень часто потому ради скорости исполнения развернута {
	if(ctlID) {
		const TView * p_temp = P_Last;
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
}

TView * FASTCALL TWindow::getCtrlView(ushort ctlID) { return const_cast<TView *>(getCtrlViewC(ctlID)); }

TView * FASTCALL TWindow::getCtrlByHandle(HWND h)
{
	if(h) {
		TView * p_temp = P_Last;
		if(p_temp) {
			const TView * p_term = P_Last;
			do {
				p_temp = p_temp->P_Next;
				if(p_temp && p_temp->IsConsistent()) {
					if(p_temp->GetId() && GetDlgItem(HW, p_temp->GetId()) == h)
						return p_temp;
				}
				else
					return 0;
			} while(p_temp != p_term);
		}
	}
	return 0;
}

HWND FASTCALL TWindow::getCtrlHandle(ushort ctlID) { return GetDlgItem(HW, ctlID); }
void * TWindow::messageToCtrl(ushort ctlID, ushort cmd, void *ptr) { return TView::messageCommand(getCtrlView(ctlID), cmd, ptr); }

int TWindow::setSmartListBoxOption(uint ctlID, uint option)
{
	int    ok = 1;
	SmartListBox * p_list = static_cast<SmartListBox *>(getCtrlView(ctlID));
	if(TView::IsSubSign(p_list, TV_SUBSIGN_LISTBOX)) {
		CALLPTRMEMB(p_list->P_Def, SetOption(option, 1));
	}
	else
		ok = 0;
	return ok;
}

void STDCALL TWindow::setCtrlReadOnly(ushort ctlID, int enable)
{
	TView * v = getCtrlView(ctlID);
	if(v) {
		v->setState(sfReadOnly, LOGIC(enable));
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
			::InvalidateRect(h, 0, 1);
			::UpdateWindow(h);
		}
	}
}

void STDCALL TWindow::disableCtrl(ushort ctlID, int enable)
{
	TView * v = getCtrlView(ctlID);
	if(v) {
		v->setState(sfDisabled, LOGIC(enable));
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
	TView * p_view = static_cast<TView *>(TView::messageBroadcast(this, cmSearchButton, reinterpret_cast<void *>(cmd)));
	return TView::IsSubSign(p_view, TV_SUBSIGN_BUTTON) ? static_cast<TButton *>(p_view) : 0;
}

int TWindow::selectButton(ushort cmd)
{
	int    ok = 1;
	TButton * p_view = SearchButton(cmd);
	if(p_view)
		SetCurrentView(p_view, forceSelect);
	else
		ok = 0;
	return ok;
}

void TWindow::showButton(uint cmd, int s)
{
	TButton * p_view = SearchButton(cmd);
	CALLPTRMEMB(p_view, Show(s));
}

int TWindow::setButtonText(uint cmd, const char * pText)
{
	int    ok = 1;
	TButton * p_ctl = SearchButton(cmd);
	const uint ctl_id = p_ctl ? p_ctl->GetId() : 0;
	if(ctl_id) {
		p_ctl->Title = pText;
		TView::SSetWindowText(GetDlgItem(HW, ctl_id), p_ctl->Title);
	}
	else
		ok = 0;
	return ok;
}

int TWindow::setButtonBitmap(uint cmd, uint bmpID)
{
	TButton * p_ctl = SearchButton(cmd);
	return p_ctl ? p_ctl->SetBitmap(bmpID) : 0;
}

int TWindow::destroyCtrl(uint ctlID)
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

TLabel * TWindow::getCtlLabel(uint ctlID)
{
	TView  * v = getCtrlView(ctlID);
	return v ? static_cast<TLabel *>(TView::messageBroadcast(this, cmSearchLabel, v)) : 0;
}

int TWindow::getLabelText(uint ctlID, SString & rText)
{
	rText.Z(); // @v11.3.1
	TLabel * p_label = getCtlLabel(ctlID);
	return p_label ? (p_label->getText(rText), 1) : 0;
}

int TWindow::setLabelText(uint ctlID, const char * pText)
{
	TLabel * p_label = getCtlLabel(ctlID);
	return p_label ? p_label->setText(pText) : 0;
}

int STDCALL TWindow::setCtrlData(ushort ctlID, void * data)
{
	TView * v = getCtrlView(ctlID);
	return v ? (v->TransmitData(+1, data), 1) : 0;
}

int STDCALL TWindow::getCtrlData(ushort ctlID, void * data)
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

int STDCALL TWindow::setCtrlUInt16(uint ctlID, int s)
{
	uint16 val = static_cast<uint16>(s);
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

int STDCALL TWindow::setCtrlString(uint ctlID, const SString & s)
{
	int    ok = 0;
	char   temp_buf[1024];
	size_t temp_len = sizeof(temp_buf);
	char * p_temp = temp_buf;
	int    is_temp_allocated = 0;
	TView * p_v = static_cast<TView *>(getCtrlView(ctlID));
	if(p_v) {
		const uint ctrl_subsign = p_v->GetSubSign();
		if(ctrl_subsign == TV_SUBSIGN_INPUTLINE) {
			const size_t max_len = static_cast<TInputLine *>(p_v)->getMaxLen();
			if(max_len > temp_len && s.Len() >= temp_len) {
				temp_len = max_len+32;
				p_temp = static_cast<char *>(SAlloc::M(temp_len));
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
		else if(ctrl_subsign == TV_SUBSIGN_STATIC) {
			static_cast<TStaticText *>(p_v)->setText(s);
		}
	}
	if(is_temp_allocated)
		ZFREE(p_temp);
	return ok;
}

int STDCALL TWindow::getCtrlString(uint ctlID, SString & s)
{
	int    ok = 0;
	char   temp_buf[1024];
	char * p_temp = temp_buf;
	int    is_temp_allocated = 0;
	TInputLine * p_il = static_cast<TInputLine *>(getCtrlView(ctlID));
	if(TView::IsSubSign(p_il, TV_SUBSIGN_INPUTLINE)) {
		const size_t max_len = p_il->getMaxLen();
		if(max_len > sizeof(temp_buf)) {
			p_temp = static_cast<char *>(SAlloc::M(max_len+32));
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

int STDCALL TWindow::setCtrlLong(uint ctlID, long val) { return setCtrlData(ctlID, &val); }
int STDCALL TWindow::setCtrlReal(uint ctlID, double val) { return setCtrlData(ctlID, &val); }
int STDCALL TWindow::setCtrlDate(uint ctlID, LDATE val) { return setCtrlData(ctlID, &val); }
int STDCALL TWindow::setCtrlTime(uint ctlID, LTIME val) { return setCtrlData(ctlID, &val); }
int TWindow::setCtrlDatetime(uint dtCtlID, uint tmCtlID, LDATETIME dtm) { return BIN(setCtrlData(dtCtlID, &dtm.d) && setCtrlData(tmCtlID, &dtm.t)); }
int TWindow::setCtrlDatetime(uint dtCtlID, uint tmCtlID, LDATE dt, LTIME tm) { return BIN(setCtrlData(dtCtlID, &dt) && setCtrlData(tmCtlID, &tm)); }
int TWindow::getCtrlDatetime(uint dtCtlID, uint tmCtlID, LDATETIME & rDtm) { return BIN(getCtrlData(dtCtlID, &rDtm.d) && getCtrlData(tmCtlID, &rDtm.t)); }

void TWindow::setCtrlOption(ushort ctlID, ushort flags, int s)
{
	TView * v = getCtrlView(ctlID);
	if(v)
		SETFLAG(v->ViewOptions, flags, s);
}

int TWindow::SetFont(const SFontDescr & rFd)
{
	int    ok = 0;
	if(rFd.Face.NotEmpty() && rFd.Size) {
		HFONT new_font = static_cast<HFONT>(TView::CreateFont_(rFd));
		if(new_font) {
			ok = SETIFZ(P_FontsAry, new SVector(sizeof(HFONT))) ? P_FontsAry->insert(&new_font) : 0;
			::SendMessage(H(), WM_SETFONT, reinterpret_cast<WPARAM>(new_font), TRUE);
		}
	}
	return ok;
}

int TWindow::SetCtrlFont(uint ctlID, const SFontDescr & rFd)
{
	int    ok = 0;
	HWND   h_ctl = GetDlgItem(H(), ctlID);
	if(h_ctl && rFd.Face.NotEmpty() && rFd.Size) {
		HFONT new_font = static_cast<HFONT>(TView::CreateFont_(rFd));
		if(new_font) {
			ok = SETIFZ(P_FontsAry, new SVector(sizeof(HFONT))) ? P_FontsAry->insert(&new_font) : 0;
			::SendMessage(h_ctl, WM_SETFONT, reinterpret_cast<WPARAM>(new_font), TRUE);
			ok = 1;
		}
	}
	return ok;
}

int TWindow::SetCtrlFont(uint ctrlID, const char * pFontName, int height)
{
	int    ok = -1;
	HFONT  new_font = TView::setFont(GetDlgItem(H(), ctrlID), pFontName, height);
	if(new_font) {
		SETIFZ(P_FontsAry, new SVector(sizeof(HFONT)));
		ok = P_FontsAry ? P_FontsAry->insert(&new_font) : 0;
	}
	return ok;
}

int __cdecl TWindow::SetCtrlsFont(const char * pFontName, int height, ...)
{
	int   ok = -1;
	long  ctrl_id;
	va_list  vl;
	va_start(vl, height);
	ctrl_id = va_arg(vl, long);
	if(ctrl_id) {
		HFONT  new_font = TView::setFont(GetDlgItem(H(), ctrl_id), pFontName, height);
		if(new_font) {
			SETIFZ(P_FontsAry, new SVector(sizeof(HFONT)));
			ok = P_FontsAry ? P_FontsAry->insert(&new_font) : 0;
			if(ok > 0)
				while((ctrl_id = va_arg(vl, long)) != 0)
					::SendMessage(::GetDlgItem(H(), ctrl_id), WM_SETFONT, reinterpret_cast<WPARAM>(new_font), TRUE);
		}
	}
	va_end(vl);
	return ok;
}

int TWindow::getStaticText(ushort ctlID, SString & rBuf)
{
	rBuf.Z();
	int    ok = 0;
	TView * p_v = getCtrlView(ctlID);
	if(p_v && p_v->GetSubSign() == TV_SUBSIGN_STATIC) {
		static_cast<TStaticText *>(p_v)->getText(rBuf);
		ok = 1;
	}
	return ok;
}

int TWindow::setStaticText(ushort ctlID, const char * pText)
{
	int    ok = 0;
	TView * p_v = getCtrlView(ctlID);
	if(p_v) {
		if(p_v->GetSubSign() == TV_SUBSIGN_STATIC) {
			static_cast<TStaticText *>(p_v)->setText(pText);
			ok = 1;
		}
		else if(p_v->GetSubSign() == TV_SUBSIGN_INPUTLINE) {
			static_cast<TInputLine *>(p_v)->setText(pText);
		}
	}
	return ok;
}

void TWindow::showCtrl(ushort ctlID, int s)
{
	TView * v = getCtrlView(ctlID);
	if(v)
		v->setState(sfVisible, LOGIC(s));
	else {
		HWND   w_ctl = ::GetDlgItem(HW, ctlID);
		if(w_ctl)
			::ShowWindow(w_ctl, s ? SW_SHOW : SW_HIDE);
	}
}

void FASTCALL TWindow::setTitle(const char * pBuf) { return Helper_SetTitle(pBuf, 0); }
void FASTCALL TWindow::setOrgTitle(const char * pBuf) { return Helper_SetTitle(pBuf, 1); }
const SString & TWindow::getTitle() const { return Title; }

void STDCALL TWindow::Helper_SetTitle(const char * pBuf, int setOrgTitle)
{
	if(!isempty(pBuf)) {
		SString temp_title;
		{
			if(pBuf[0] != '@' || SLS.LoadString_(pBuf+1, temp_title) <= 0) // @v11.9.11
				temp_title = pBuf;
		}
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
	if(ToolbarL.getItemsCount()) {
		TMenuPopup menu;
		ToolbarItem item;
		for(uint i = 0; ToolbarL.enumItems(&i, &item);) {
			if(item.KeyCode != TV_MENUSEPARATOR)
				menu.Add(item.ToolTipText, 0, item.KeyCode);
			else if(i < ToolbarL.getItemsCount()) // Последний разделитель не заносим
				menu.AddSeparator();
		}
		uint   cmd = 0;
		uint   key = 0;
		if(menu.Execute(HW, TMenuPopup::efRet, &cmd, &key) > 0) {
			if(cmd)
				TView::messageCommand(this, cmd);
			else if(key)
				TView::messageKeyDown(this, key);
		}
	}
}

int TWindow::translateKeyCode(ushort keyCode, uint * pCmd) const
{
	int    ok = 0;
	uint   idx = 0;
	if(ToolbarL.searchKeyCode(keyCode, &idx)) {
		const ToolbarItem & r_item = ToolbarL.getItem(idx);
		if(r_item.Cmd) {
			ASSIGN_PTR(pCmd, static_cast<uint>(r_item.Cmd));
			ok = 1;
		}
	}
	return ok;
}

void TWindow::setupToolbar(const ToolbarList * pToolbar)
{
	if(!RVALUEPTR(ToolbarL, pToolbar))
		ToolbarL.clearAll();
}

int TWindow::LoadToolbarResource(uint toolbarResourceId)
{
	int    ok = 1;
	if(toolbarResourceId) {
		ToolbarList tb_list;
		THROW(P_SlRez);
		THROW(::LoadToolbar(P_SlRez, TV_EXPTOOLBAR, toolbarResourceId, &tb_list));
		setupToolbar(&tb_list);
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

HWND TWindow::showToolbar()
{
	HWND   h_tool_bar = CreateWindowEx(0, TOOLBARCLASSNAME, 0, WS_CHILD|TBSTYLE_TOOLTIPS|CCS_TOP|WS_CLIPSIBLINGS, 
		0, 0, 0, 0, H(), reinterpret_cast<HMENU>(1), TProgram::GetInst(), 0);
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
 	::SendMessage(h_tool_bar, CCM_SETVERSION, 5, 0);
	ToolbarItem item;
	for(uint i = 0; ToolbarL.enumItems(&i, &item);) {
		if(!(item.Flags & ToolbarItem::fHidden)) {
			TBBUTTON btns;
			/*
			btns.fsState = TBSTATE_ENABLED;
			btns.dwData  = 0;
			btns.iString = 0;
			*/
			if(!isempty(item.ToolTipText)) {
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
			::SendMessage(h_tool_bar, TB_ADDBUTTONS, 1, reinterpret_cast<LPARAM>(&btns));
		}
	}
	::SendMessage(h_tool_bar, TB_SETIMAGELIST, 0, reinterpret_cast<LPARAM>(himl));
	ShowWindow(h_tool_bar, SW_SHOW);
	return h_tool_bar;
}

IMPL_HANDLE_EVENT(TWindow)
{
	TGroup::handleEvent(event);
	if(event.isCmd(cmClose)) {
		if(!TVINFOPTR || TVINFOPTR == this || TVINFOVIEW->P_Owner == this) {
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

void TWindow::invalidateRect(const TRect & rRect, bool erase)
{
	RECT r = rRect;
	::InvalidateRect(HW, &r, erase);
}

void TWindow::invalidateRect(const FRect & rRect, bool erase)
{
	RECT r;
	r.left = R0i(rRect.a.x);
	r.top = R0i(rRect.a.y);
	r.right = R0i(rRect.b.x);
	r.bottom = R0i(rRect.b.y);
	::InvalidateRect(HW, &r, erase);
}

void TWindow::invalidateRegion(const SRegion & rRgn, bool erase)
{
	if(HW) {
		const SPtrHandle * p_hdl = reinterpret_cast<const SPtrHandle *>(&rRgn); // @trick
		const HRGN h_rgn = static_cast<HRGN>(static_cast<void *>(*p_hdl));
		::InvalidateRgn(HW, h_rgn, erase);
	}
}

void FASTCALL TWindow::invalidateAll(bool erase)
{
	if(HW) // @v11.2.7
		::InvalidateRect(HW, 0, erase);
}

int TWindow::RegisterMouseTracking(int leaveNotify, int hoverTimeout)
{
	TRACKMOUSEEVENT tme;
	INITWINAPISTRUCT(tme);
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

int TWindow::SetCtlSymb(uint id, const char * pSymb)
{
	int    ok = -1;
	if(!isempty(pSymb) && id > 0) {
		SETIFZ(P_SymbList, new StrAssocArray);
		if(P_SymbList) {
			P_SymbList->Add(static_cast<long>(id), pSymb);
			ok = 1;
		}
		else
			ok = 0;
	}
	return ok;
}

TView * FASTCALL TWindow::CtrlIdToView(long id) const
{
	TView * v = P_Last;
	if(v) do {
		if(v->TestId(id))
			return v;
		else
			v = v->prev();
	} while(v != P_Last);
	return 0;
}

int TWindow::GetCtlSymb(uint id, SString & rBuf) const
{
	rBuf.Z();
	return BIN(P_SymbList && P_SymbList->GetText(static_cast<long>(id), rBuf) > 0);
}

void TWindow::InvalidateLayoutRefList(const SUiLayout::RefCollection & rRedrawLoList, int erase)
{
	for(uint i = 0; i < rRedrawLoList.GetCount(); i++) {
		const SUiLayout * p_lo = rRedrawLoList.Get(i);
		if(p_lo->IsConsistent())
			invalidateRect(p_lo->GetFrameAdjustedToParent(), LOGIC(erase));
	}
}

int TWindow::InsertCtl(TView * pCtl, uint id, const char * pSymb)
{
	int    ok = 0;
	if(pCtl) {
		if(pCtl->IsSubSign(TV_SUBSIGN_COMBOBOX)) {
			ComboBox * p_cb = static_cast<ComboBox *>(pCtl);
			if(p_cb->link())
				Insert_(p_cb->link());
		}
		Insert_(&pCtl->SetId(id));
		SetCtlSymb(id, pSymb);
		ok = 1;
	}
	return ok;
}

int TWindow::InsertCtlWithCorrespondingNativeItem(TView * pCtl, uint id, const char * pSymb, void * extraPtr)
{
	int    ok = 0;
	if(InsertCtl(pCtl, id, pSymb)) {
		TView::CreateCorrespondingNativeItem(pCtl);
		ok = 1;
	}
	return ok;
}

bool TWindow::SetLayout(SUiLayout * pLo)
{
	if(P_Lfc) {
		delete P_Lfc;
	}
	P_Lfc = pLo;
	return true;
}

void TWindow::SetupLayoutItem(void * pLayout)
{
	if(pLayout) {
		P_Lfc = static_cast<SUiLayout *>(pLayout);
		P_Lfc->SetCallbacks(0, TWindowBase::SetupLayoutItemFrame, this);
	}
}

SUiLayout * TWindow::GetLayout() { return P_Lfc; }

void TWindow::EvaluateLayout(const TRect & rR)
{
	if(P_Lfc) {
		SUiLayout::Param evp;
		evp.ForceSize.x = static_cast<float>(rR.width());
		evp.ForceSize.y = static_cast<float>(rR.height());
		P_Lfc->Evaluate(&evp);
	}
}

/*static*/void __stdcall TWindow::SetupLayoutItemFrame(SUiLayout * pItem, const SUiLayout::Result & rR)
{
	TView * p_view = static_cast<TView *>(SUiLayout::GetManagedPtr(pItem));
	if(p_view) {
		p_view->changeBounds(TRect(rR));
	}
}
//
//
//
static LPCTSTR P_SLibWindowBaseClsName = _T("SLibWindowBase");

/*static*/int TWindowBase::RegWindowClass(int iconId)
{
	WNDCLASSEX wc;
	const HINSTANCE h_inst = TProgram::GetInst();
	if(!::GetClassInfoEx(h_inst, P_SLibWindowBaseClsName, &wc)) {
		INITWINAPISTRUCT(wc);
		wc.lpszClassName = P_SLibWindowBaseClsName;
		wc.hInstance     = h_inst;
		wc.lpfnWndProc   = TWindowBase::WndProc;
		wc.style = /*CS_HREDRAW | CS_VREDRAW |*/ /*CS_OWNDC |*/ CS_SAVEBITS | CS_DBLCLKS;
		wc.hIcon = iconId ? ::LoadIcon(h_inst, MAKEINTRESOURCE(/*ICON_MAIN_P2*/ /*102*/iconId)) : 0;
		wc.cbClsExtra    = BRWCLASS_CEXTRA;
		wc.cbWndExtra    = BRWCLASS_WEXTRA;
		return ::RegisterClassEx(&wc);
	}
	else
		return -1;
}

/*static*/void TWindowBase::Helper_Finalize(HWND hWnd, TBaseBrowserWindow * pView)
{
	if(pView) {
		SETIFZ(pView->EndModalCmd, cmCancel);
		APPL->DelItemFromMenu(pView);
		pView->ResetOwnerCurrent();
		if(!pView->IsInState(sfModal)) {
			APPL->P_DeskTop->remove(pView);
			TView::SetWindowUserData(hWnd, 0);
			delete pView;
		}
	}
}

TWindowBase::TWindowBase(LPCTSTR pWndClsName, long wbCapability) : ClsName(SUcSwitch(pWndClsName)), 
	TWindow(wbCapability), WbState(0), H_DrawBuf(0)
{
}

TWindowBase::~TWindowBase()
{
	ZDeleteWinGdiObject(&H_DrawBuf);
	TWindowBase * p_this_view_from_wnd = 0;
	if(::IsWindow(HW)) {
		p_this_view_from_wnd = static_cast<TWindowBase *>(TView::GetWindowUserData(HW));
		if(p_this_view_from_wnd) {
			Sf |= sfOnDestroy;
			::DestroyWindow(HW);
			HW = 0;
			Sf &= ~sfOnDestroy;
		}
	}
	/* @v12.2.4 (moved to TWindow::~TWindow())
	if(P_Lfc && !(Sf & (sfOnDestroy|sfOnParentDestruction))) {
		if(!P_Lfc->FatherKillMe())
			delete P_Lfc;
		P_Lfc = 0;
	}
	*/
}

int TWindowBase::Create(void * hParentWnd, long createOptions)
{
	const HINSTANCE h_inst = TProgram::GetInst();
	TWindowBase::RegWindowClass(102);
	SString title_buf = getTitle();
	title_buf.SetIfEmpty(ClsName).Transf(CTRANSF_INNER_TO_OUTER);
	HWND  hw_parent = static_cast<HWND>(hParentWnd);
	DWORD style = WS_HSCROLL | WS_VSCROLL /*| WS_CLIPSIBLINGS | WS_CLIPCHILDREN*/;
	int   x = ViewSize.x ? ViewOrigin.x : CW_USEDEFAULT;
	int   y = ViewSize.y ? ViewOrigin.y : CW_USEDEFAULT;
	int   cx = NZOR(ViewSize.x, CW_USEDEFAULT);
	int   cy = NZOR(ViewSize.y, CW_USEDEFAULT);
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
	}
	else {

	}
	Parent = hw_parent; // @v11.2.4
	if(createOptions & coChild) {
		style = WS_CHILD|WS_TABSTOP;
		HW = CreateWindowEx(WS_EX_CLIENTEDGE, P_SLibWindowBaseClsName, SUcSwitch(title_buf), style, 0, 0, cx, cy, hw_parent, 0, h_inst, this);
	}
	else { // coPopup
		style = WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_VISIBLE;
		if(createOptions & coMDI) {
			MDICREATESTRUCT child;
			child.szClass = P_SLibWindowBaseClsName;
			child.szTitle = SUcSwitch(title_buf);
			child.hOwner = h_inst;
			child.x  = CW_USEDEFAULT;
			child.y  = CW_USEDEFAULT;
			child.cx = CW_USEDEFAULT;
			child.cy = CW_USEDEFAULT;
			child.style  = style;
			child.lParam = reinterpret_cast<LPARAM>(this);
			HW = reinterpret_cast<HWND>(LOWORD(SendMessage(hw_parent, WM_MDICREATE, 0, reinterpret_cast<LPARAM>(&child))));
		}
		else {
			HW = CreateWindowEx(0, P_SLibWindowBaseClsName, SUcSwitch(title_buf), style, x, y, cx, cy, hw_parent, 0, h_inst, this);
		}
	}
	return BIN(HW);
}

int TWindowBase::AddChild(TWindowBase * pWin, long createOptions, long zone)
{
	int    ok = 1;
	if(pWin) {
		pWin->Create(HW, createOptions);
		TWindow::Insert_(pWin);
		::ShowWindow(pWin->H(), SW_SHOWNORMAL);
		::UpdateWindow(H());
	}
	return ok;
}

int TWindowBase::AddChildWithLayout(TWindowBase * pChildWindow, long createOptions, void * pLayout) 
{
	int    ok = 1;
	if(pChildWindow) {
		pChildWindow->Create(HW, createOptions);
		TWindow::Insert_(pChildWindow);
		if(pLayout) {
			assert(P_Lfc);
			if(P_Lfc) {
				pChildWindow->SetupLayoutItem(pLayout);
				P_Lfc->GetRoot()->Evaluate(0);
			}
		}
		::ShowWindow(pChildWindow->H(), SW_SHOWNORMAL);
		::UpdateWindow(H());
	}
	return ok;
}

IMPL_HANDLE_EVENT(TWindowBase)
{
	if(event.isCmd(cmExecute)) {
		ushort last_command = 0;
		SString buf(getTitle());
		buf.Transf(CTRANSF_INNER_TO_OUTER);
		if(APPL->H_MainWnd) {
			if(::IsIconic(APPL->H_MainWnd))
				::ShowWindow(APPL->H_MainWnd, SW_MAXIMIZE);
			if(IsMDIClientWindow(APPL->H_MainWnd))
				Create(APPL->H_MainWnd, coMDI);
			else
				Create(APPL->H_TopOfStack, coPopup/* @v11.2.0 | coMaxSize*/);
		}
		// @v11.2.7 SetWindowPos(HW, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE); // @v11.2.4
		// @v11.2.4 ::ShowWindow(HW, SW_SHOW); // @v11.2.4 SW_NORMAL-->SW_SHOW
		// @v11.2.4 ::UpdateWindow(HW);
		if(APPL->PushModalWindow(this, HW)) {
			// @v11.2.4 ::EnableWindow(PrevInStack, 0);
			APPL->MsgLoop(this, EndModalCmd);
			last_command = EndModalCmd;
			EndModalCmd = 0;
			APPL->PopModalWindow(this, 0);
		}
		// @v12.2.4 {
		//::DestroyWindow(H());
		//HW = 0;
		// } @v12.2.4 
		clearEvent(event);
		event.message.infoLong = last_command;
	}
	else {
		TWindow::handleEvent(event);
		if(TVINFOPTR) {
			if(event.isCmd(cmInit)) {
				const TRect cr = getClientRect();
				if(P_Lfc && !P_Lfc->GetParent()) {
					P_Lfc->GetLayoutBlock().SetFixedSize(cr);
					P_Lfc->Evaluate(0);
				}
			}
			else if(event.isCmd(cmSetBounds)) {
				const TRect * p_rc = static_cast<const TRect *>(TVINFOPTR);
				::SetWindowPos(H(), 0, p_rc->a.x, p_rc->a.y, p_rc->width(), p_rc->height(), SWP_NOZORDER|SWP_NOREDRAW|SWP_NOCOPYBITS);
				clearEvent(event);
			}
			else if(event.isCmd(cmSize)) {
				//SizeEvent * p_se = static_cast<SizeEvent *>(TVINFOPTR);
				const TRect cr = getClientRect();
				if(P_Lfc && !P_Lfc->GetParent()) {
					P_Lfc->GetLayoutBlock().SetFixedSize(cr);
					P_Lfc->Evaluate(0);
				}
				invalidateAll(true);
				::UpdateWindow(H());
				// Don't call clearEvent(event) there!
			}
		}
	}
}

void TWindowBase::SetDefaultCursor()
{
	::SetCursor(::LoadCursor(0, IDC_ARROW));
}

void TWindowBase::RegisterMouseTracking(int force)
{
	if(force || !(WbState & wbsMouseTrackRegistered)) {
		TWindow::RegisterMouseTracking(1, 0);
		WbState |= wbsMouseTrackRegistered;
	}
}

void TWindowBase::MakeMouseEvent(uint msg, WPARAM wParam, LPARAM lParam, MouseEvent & rMe)
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
			rMe.WeelDelta = static_cast<signed short>(HIWORD(wParam)); // @v11.2.4 static_cast<signed short>()
			break;
		default:
			return;
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
}

PaintEvent::PaintEvent() : PaintType(0), H_DeviceContext(0), Flags(0)
{
	Rect.Z();
}

/*static*/int TView::Helper_SendCmSizeAsReplyOnWmSize(TView * pV, WPARAM wParam, LPARAM lParam)
{
	int   result = -1;
	if(pV) {
		SizeEvent se;
		switch(wParam) {
			case SIZE_MAXHIDE:   se.ResizeType = SizeEvent::tMaxHide;   break;
			case SIZE_MAXIMIZED: se.ResizeType = SizeEvent::tMaximized; break;
			case SIZE_MAXSHOW:   se.ResizeType = SizeEvent::tMaxShow;   break;
			case SIZE_MINIMIZED: se.ResizeType = SizeEvent::tMinimized; break;
			case SIZE_RESTORED:  se.ResizeType = SizeEvent::tRestored;  break;
			default: se.ResizeType = 0; break;
		}
		se.PrevSize = pV->ViewSize;
		pV->ViewSize = se.NewSize.setwparam(lParam);
		if(TView::messageCommand(pV, cmSize, &se))
			result = 0;
	}
	return result;
}

/*static*/LRESULT CALLBACK TWindowBase::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	TWindowBase * p_view = static_cast<TWindowBase *>(TView::GetWindowUserData(hWnd));
	LPCREATESTRUCT p_init_data;
	switch(message) {
		case WM_HELP:
			{
				HelpEvent he;
				MEMSZERO(he);
				const HELPINFO * p_hi = reinterpret_cast<const HELPINFO *>(lParam);
				if(p_hi) {
					if(p_hi->iContextType == HELPINFO_MENUITEM)
						he.ContextType = HelpEvent::ctxtMenu;
					else if(p_hi->iContextType == HELPINFO_WINDOW)
						he.ContextType = HelpEvent::ctxtWindow;
					he.CtlId = p_hi->iCtrlId;
					he.H_Item = p_hi->hItemHandle;
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
				p_init_data = reinterpret_cast<LPCREATESTRUCT>(lParam);
				if(IsMDIClientWindow(p_init_data->hwndParent)) {
					MDICREATESTRUCT * p_mdi_init_data = static_cast<LPMDICREATESTRUCT>(p_init_data->lpCreateParams);
					p_view = reinterpret_cast<TWindowBase *>(p_mdi_init_data->lParam);
					p_view->WbState |= wbsMDI;
					cr_blk.Coord.setwidthrel(p_mdi_init_data->x, p_mdi_init_data->cx);
					cr_blk.Coord.setheightrel(p_mdi_init_data->y, p_mdi_init_data->cy);
					cr_blk.Param = reinterpret_cast<void *>(p_mdi_init_data->lParam);
					cr_blk.H_Process = p_mdi_init_data->hOwner;
					cr_blk.Style = p_mdi_init_data->style;
					cr_blk.ExStyle = 0;
					cr_blk.H_Parent = 0;
					cr_blk.H_Menu = 0;
					cr_blk.P_WndCls = SUcSwitch(p_mdi_init_data->szClass);
					cr_blk.P_Title = SUcSwitch(p_mdi_init_data->szTitle);
				}
				else {
					p_view = static_cast<TWindowBase *>(p_init_data->lpCreateParams);
					p_view->WbState &= ~wbsMDI;
					cr_blk.Coord.setwidthrel(p_init_data->x, p_init_data->cx);
					cr_blk.Coord.setheightrel(p_init_data->y, p_init_data->cy);
					cr_blk.Param = p_init_data->lpCreateParams;
					cr_blk.H_Process = p_init_data->hInstance;
					cr_blk.Style = p_init_data->style;
					cr_blk.ExStyle = p_init_data->dwExStyle;
					cr_blk.H_Parent = p_init_data->hwndParent;
					cr_blk.H_Menu = p_init_data->hMenu;
					cr_blk.P_WndCls = SUcSwitch(p_init_data->lpszClass);
					cr_blk.P_Title = SUcSwitch(p_init_data->lpszName);
				}
				if(p_view) {
					p_view->HW = hWnd;
					p_view->ViewOrigin.Set(p_init_data->x, p_init_data->y);
					p_view->ViewSize.Set(p_init_data->cx, p_init_data->cy);
					//p_view->RegisterMouseTracking(1);
					TView::SetWindowUserData(hWnd, p_view);
					TView::messageCommand(p_view, cmInit, &cr_blk);
					// @v11.2.3 {
					{
						SetupCtrlTextProc(p_view->H(), 0);
						TView * p_child_view = p_view->P_Last;
						if(p_child_view) {
							do {
								HWND   ctrl = GetDlgItem(hWnd, p_child_view->GetId());
								SETIFZ(ctrl, GetDlgItem(hWnd, MAKE_BUTTON_ID(p_child_view->GetId(), 1)));
								if(IsWindow(ctrl)) {
									p_child_view->Parent = hWnd;
									// Теоретически, в качестве первого аргумента должно быть message (WM_CREATE),
									// но учитывая то, что блок перенесен из TDialog для унификации взаимодействия с
									// управляющими элементами и из иных окон, пока для совместимости оставим WM_INITDIALOG
									p_child_view->handleWindowsMessage(WM_INITDIALOG, wParam, lParam);
									EnableWindow(ctrl, !p_child_view->IsInState(sfDisabled));
								}
							} while((p_child_view = p_child_view->prev()) != p_view->P_Last);
						}
						EnumChildWindows(hWnd, SetupCtrlTextProc, 0);
					}
					// } @v11.2.3 
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
				SetFontEvent sfe(reinterpret_cast<void *>(wParam), LOWORD(lParam));
				if(TView::messageCommand(p_view, cmSetFont, &sfe))
					return 0;
			}
			break;
		case WM_SIZE:
			if(!TView::Helper_SendCmSizeAsReplyOnWmSize(p_view, wParam, lParam))
				return 0;
			break;
		case WM_MOVE:
			{
				p_view->ViewOrigin.setwparam(lParam);
				if(TView::messageCommand(p_view, cmMove))
					return 0;
			}
			break;
		case WM_PAINT:
			if(p_view) {
				void * p_ret = 0;
				PAINTSTRUCT ps;
				PaintEvent pe;
				pe.PaintType = PaintEvent::tPaint;
				BeginPaint(hWnd, &ps);
				SETFLAG(pe.Flags, PaintEvent::fErase, ps.fErase);
				pe.Rect = ps.rcPaint;
				const  bool use_draw_buf = LOGIC(static_cast<const TWindowBase *>(p_view)->WbCapability & wbcDrawBuffer);
				if(!use_draw_buf || !ps.fErase) {
					RECT   cr;
					HDC    h_dc_mem = 0;
					HBITMAP h_bmp = 0;
					HBITMAP h_old_bmp = 0;
					if(use_draw_buf) {
						GetClientRect(hWnd, &cr);
						h_dc_mem = CreateCompatibleDC(ps.hdc);
						h_bmp = CreateCompatibleBitmap(ps.hdc, cr.right - cr.left, cr.bottom - cr.top);
						h_old_bmp = static_cast<HBITMAP>(::SelectObject(h_dc_mem, h_bmp));
						pe.H_DeviceContext = h_dc_mem;
					}
					else
						pe.H_DeviceContext = ps.hdc;
					p_ret = TView::messageCommand(p_view, cmPaint, &pe);
					if(use_draw_buf) {
						BitBlt(ps.hdc, 0, 0, cr.right - cr.left, cr.bottom - cr.top, h_dc_mem, 0, 0, SRCCOPY);
						SelectObject(h_dc_mem, h_old_bmp);
						ZDeleteWinGdiObject(&h_bmp);
						DeleteDC(h_dc_mem);
					}
				}
				EndPaint(hWnd, &ps);
				//if(p_ret)
					return 0;
			}
			break;
		case WM_NCPAINT:
			{
				PaintEvent pe;
				pe.PaintType = PaintEvent::tNcPaint;
				HDC hdc = GetDCEx(hWnd, (HRGN)wParam, DCX_WINDOW|DCX_INTERSECTRGN);
				pe.H_DeviceContext = hdc;
				void * p_ret = TView::messageCommand(p_view, cmPaint, &pe);
				ReleaseDC(hWnd, hdc);
				if(p_ret)
					return 0;
			}
			break;
		case WM_ERASEBKGND:
			if(p_view) {
				PaintEvent pe;
				pe.PaintType = PaintEvent::tEraseBackground;
				pe.H_DeviceContext = reinterpret_cast<void *>(wParam);
				pe.Rect = p_view->getClientRect();
				void * p_ret = TView::messageCommand(p_view, cmPaint, &pe);
				//
				// Если получатель очистил фон, он должен акцептировть сообщение.
				// Windows ожидает получить в этом случае !0.
				//
				return BIN(p_ret);
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
				se.H_Wnd = reinterpret_cast<void *>(lParam);
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
		case WM_MOUSEWHEEL:
			if(hWnd != GetFocus())
				SetFocus(hWnd);
			if(hWnd != GetCapture())
				::SetCapture(hWnd);
		case WM_LBUTTONUP:
		case WM_LBUTTONDBLCLK:
		case WM_RBUTTONDOWN:
		case WM_RBUTTONUP:
		case WM_RBUTTONDBLCLK:
		case WM_MBUTTONDOWN:
		case WM_MBUTTONUP:
		case WM_MBUTTONDBLCLK:
		case WM_MOUSEMOVE:
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
			if(p_view && (wParam != VK_RETURN || LOBYTE(HIWORD(lParam)) != 0x1c))
				TView::messageKeyDown(p_view, wParam);
			return 0;
		case WM_DRAWITEM: // @v11.2.0
			return p_view ? p_view->RedirectDrawItemMessage(message, wParam, lParam) : FALSE;
		case WM_CLOSE: // @v11.2.4
			if(p_view && p_view->IsInState(sfModal)) {
				TView::messageCommand(p_view, cmCancel, p_view);
				return 0;
			}
			break;
		case WM_COMMAND: // @v11.2.0
			// Этот участок кода почти в точности скопирован из класса TDialog. Вполне возможно, что возникнут проблемы!
			{
				uint16 hiw = HIWORD(wParam);
				uint16 low = LOWORD(wParam);
				if(GetKeyState(VK_CONTROL) & 0x8000 && low != cmaCalculate && hiw != EN_UPDATE && hiw != EN_CHANGE) {
					//return 0;
					;
				}
				else if(p_view) {
					if(hiw == 0 && low == IDCANCEL) {
						TView::messageCommand(p_view, cmCancel, p_view);
						//return 0;
					}
					else {
						if(!lParam) {
							if(hiw == 0) { // from menu
								TView::messageKeyDown(p_view, low);
								return 0;
							}
							else if(hiw == 1) { // from accelerator
								TEvent event;
								event.what = TEvent::evCommand;
								event.message.command = low;
								p_view->handleEvent(event);
							}
						}
						TView * v = p_view->CtrlIdToView(CLUSTER_ID(low));
						CALLPTRMEMB(v, handleWindowsMessage(message, wParam, lParam));
					}
				}
			}
			break;
		case WM_VKEYTOITEM: // @v11.2.4 (сделано по аналогии с аналогичным сообщением в TDialog::DialogProc)
			if(PassMsgToCtrl(hWnd, message, wParam, lParam) == -1) {
				CALLPTRMEMB(p_view, HandleKeyboardEvent(LOWORD(wParam)));
			}
			::SendMessage(hWnd, WM_USER_KEYDOWN, wParam, lParam);
			return 0;
		default:
			break;
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}
