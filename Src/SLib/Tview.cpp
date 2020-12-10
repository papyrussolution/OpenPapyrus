// TVIEW.CPP  Turbo Vision 1.0
// Copyright (c) 1991 by Borland International
//
#include <slib-internal.h>
#pragma hdrstop
#include <htmlhelp.h> // @Muxa
//
//
void FASTCALL ZDeleteWinGdiObject(void * pHandle)
{
	if(pHandle) {
		HGDIOBJ * p_obj = static_cast<HGDIOBJ *>(pHandle);
		if(*p_obj) {
			::DeleteObject(*p_obj);
			*p_obj = 0;
		}
	}
}
//
//
//
/*static*/void * FASTCALL TView::messageCommand(TView * pReceiver, uint command)
{
	void * p_ret = 0;
	if(pReceiver) {
		TEvent event;
		pReceiver->handleEvent(event.setCmd(command, 0));
		if(event.what == TEvent::evNothing)
			p_ret = event.message.infoPtr;
	}
	return p_ret;
}

/*static*/void * FASTCALL TView::messageCommand(TView * pReceiver, uint command, void * pInfoPtr)
{
	void * p_ret = 0;
	if(pReceiver) {
		TEvent event;
		event.what = TEvent::evCommand;
		event.message.command = command;
		event.message.infoPtr = pInfoPtr;
		pReceiver->handleEvent(event);
		if(event.what == TEvent::evNothing)
			p_ret = event.message.infoPtr;
	}
	return p_ret;
}

/*static*/void * FASTCALL TView::messageBroadcast(TView * pReceiver, uint command)
{
	void * p_ret = 0;
	if(pReceiver) {
		TEvent event;
		event.what = TEvent::evBroadcast;
		event.message.command = command;
		pReceiver->handleEvent(event);
		if(event.what == TEvent::evNothing)
			p_ret = event.message.infoPtr;
	}
	return p_ret;
}

/*static*/void * FASTCALL TView::messageBroadcast(TView * pReceiver, uint command, void * pInfoPtr)
{
	void * p_ret = 0;
	if(pReceiver) {
		TEvent event;
		event.what = TEvent::evBroadcast;
		event.message.command = command;
		event.message.infoPtr = pInfoPtr;
		pReceiver->handleEvent(event);
		if(event.what == TEvent::evNothing)
			p_ret = event.message.infoPtr;
	}
	return p_ret;
}

/*static*/void * FASTCALL TView::messageKeyDown(TView * pReceiver, uint keyCode)
{
	void * p_ret = 0;
	if(pReceiver) {
		TEvent event;
		event.what = TEvent::evKeyDown;
		event.keyDown.keyCode = keyCode;
		pReceiver->handleEvent(event);
		if(event.what == TEvent::evNothing)
			p_ret = event.message.infoPtr;
	}
	return p_ret;
}
//
//
//
TCommandSet::TCommandSet()
	{ resetbitstring(cmds, sizeof(cmds)); }
int TCommandSet::has(int cmd) const
	{ return getbit32(cmds, sizeof(cmds), cmd); }
void TCommandSet::enableAll()
	{ memset(cmds, 0xff, sizeof(cmds)); }

void TCommandSet::enableCmd(int cmd, int is_enable)
{
	if(is_enable)
		setbit32(cmds, sizeof(cmds), cmd);
	else
		resetbit32(cmds, sizeof(cmds), cmd);
}

void TCommandSet::enableCmd(const TCommandSet & tc, int is_enable)
{
	int    i = 0;
	if(is_enable)
		for(; i < SIZEOFARRAY(cmds); i++)
			cmds[i] |= tc.cmds[i];
	else
		for(; i < SIZEOFARRAY(cmds); i++)
			cmds[i] &= ~(tc.cmds[i]);
}

TCommandSet & TCommandSet::operator &= (const TCommandSet & tc)
{
	for(int i = 0; i < SIZEOFARRAY(cmds); i++)
		cmds[i] &= tc.cmds[i];
	return *this;
}

TCommandSet & TCommandSet::operator |= (const TCommandSet & tc)
{
	for(int i = 0; i < SIZEOFARRAY(cmds); i++)
		cmds[i] |= tc.cmds[i];
	return *this;
}

TCommandSet operator & (const TCommandSet & tc1, const TCommandSet & tc2)
{
	TCommandSet temp(tc1);
	temp &= tc2;
	return temp;
}

TCommandSet operator | (const TCommandSet& tc1, const TCommandSet& tc2)
{
	return (TCommandSet(tc1) |= tc2);
}

int TCommandSet::IsEmpty() const
{
	for(int i = 0; i < SIZEOFARRAY(cmds); i++)
		if(cmds[i])
			return 0;
	return 1;
}

int operator == (const TCommandSet& tc1, const TCommandSet& tc2)
{
	return (memcmp(tc1.cmds, tc2.cmds, sizeof(tc1.cmds)) == 0);
}

void TCommandSet::operator += (int cmd) { enableCmd(cmd, 1);  }
void TCommandSet::operator -= (int cmd) { enableCmd(cmd, 0); }
void TCommandSet::operator += (const TCommandSet & tc) { enableCmd(tc, 1); }
void TCommandSet::operator -= (const TCommandSet & tc) { enableCmd(tc, 0); }
int  TCommandSet::loc(int cmd) { return (cmd >> 3); }
int  TCommandSet::mask(int cmd) { return (1 << (cmd & 0x07)); }
int  operator != (const TCommandSet& tc1, const TCommandSet& tc2) { return !operator == (tc1, tc2); }
//
//
//
static TCommandSet initCommands()
{
	TCommandSet temp;
	temp.enableAll();
	temp.enableCmd(cmZoom, 0);
	temp.enableCmd(cmClose, 0);
	temp.enableCmd(cmResize, 0);
	temp.enableCmd(cmNext, 0);
	temp.enableCmd(cmPrev, 0);
	return temp;
}
//
//
//
TBitmapHash::TBitmapHash()
{
}

TBitmapHash::~TBitmapHash()
{
	for(uint i = 0; i < List.getCount(); i++) {
		Entry & r_entry = List.at(i);
		if(r_entry.H)
			ZDeleteWinGdiObject(&r_entry.H);
	}
}

HBITMAP FASTCALL TBitmapHash::Get(uint bmpId)
{
	HBITMAP h = 0;
	ENTER_CRITICAL_SECTION
	uint   p = 0;
	if(List.lsearch(&bmpId, &p, CMPF_LONG)) {
		h = static_cast<HBITMAP>(List.at(p).H);
	}
	else {
		h = static_cast<HBITMAP>(::LoadImage(APPL->GetInst(), MAKEINTRESOURCE(bmpId), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION));
		// h = ::LoadBitmap(APPL->GetInst(), MAKEINTRESOURCE(bmpId));
		if(h) {
			Entry new_entry;
			new_entry.ID = bmpId;
			new_entry.H = h;
			List.insert(&new_entry);
		}
	}
	LEAVE_CRITICAL_SECTION
	return h;
}

HBITMAP FASTCALL TBitmapHash::GetSystem(uint bmpId)
{
	HBITMAP h = 0;
	ENTER_CRITICAL_SECTION
	uint   p = 0;
	if(List.lsearch(&bmpId, &p, CMPF_LONG)) {
		h = static_cast<HBITMAP>(List.at(p).H);
	}
	else {
		h = static_cast<HBITMAP>(::LoadImage(0, MAKEINTRESOURCE(bmpId), IMAGE_BITMAP, 0, 0, LR_DEFAULTSIZE|LR_SHARED));
		if(h) {
			Entry new_entry;
			new_entry.ID = bmpId;
			new_entry.H = h;
			List.insert(&new_entry);
		}
	}
	LEAVE_CRITICAL_SECTION
	return h;
}
//
//
//
TView::EvBarrier::EvBarrier(TView * pV) : P_V(pV)
{
	assert(pV);
	Busy = P_V->EventBarrier(0);
}

TView::EvBarrier::~EvBarrier()
{
	if(!Busy && P_V->IsConsistent())
		P_V->EventBarrier(1);
}

int TView::EvBarrier::operator !() const
{
	return (Busy != 0);
}

#define SIGN_TVIEW 0x09990999UL

void TView::SendToParent(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if(Sf & sfMsgToParent)
		::SendMessage(GetParent(hWnd), uMsg, wParam, lParam);
	else
		handleWindowsMessage(uMsg, wParam, lParam);
}

TView::TView(const TRect & bounds) : Sign(SIGN_TVIEW), SubSign(0), Id(0), Reserve(0), Sf(sfVisible | sfMsgToParent),
	ViewOptions(0), EndModalCmd(0), HelpCtx(0), P_Next(0), P_Owner(0), Parent(0), PrevWindowProc(0), P_CmdSet(0), P_WordSelBlk(0)
{
	setBounds(bounds);
}

TView::TView() : Sign(SIGN_TVIEW), SubSign(0), Id(0), Reserve(0), Sf(sfVisible | sfMsgToParent),
	ViewOptions(0), EndModalCmd(0), HelpCtx(0), P_Next(0), P_Owner(0), Parent(0), PrevWindowProc(0), P_CmdSet(0), P_WordSelBlk(0)
{
}

TView::~TView()
{
	ZDELETE(P_WordSelBlk);
	CALLPTRMEMB(P_Owner, remove(this));
	Sign = 0;
	Id = 0;
	Parent = 0;
	ZDELETE(P_CmdSet);
}

int FASTCALL TView::EventBarrier(int rmv)
{
	if(Sf & sfEventBarrier) {
		if(rmv)
			Sf &= ~sfEventBarrier;
		return 1;
	}
	else {
		if(!rmv)
			Sf |= sfEventBarrier;
		return 0;
	}
}

void TView::Draw_()
{
	TView::messageCommand(this, cmDraw);
}

/*static*/void * TView::SetWindowProp(HWND hWnd, int propIndex, void * ptr)
	{ return reinterpret_cast<void *>(::SetWindowLongPtr(hWnd, propIndex, reinterpret_cast<LONG_PTR>(ptr))); }
/*static*/void * FASTCALL TView::SetWindowUserData(HWND hWnd, void * ptr)
	{ return reinterpret_cast<void *>(::SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(ptr))); }
/*static*/long TView::SetWindowProp(HWND hWnd, int propIndex, long value) { return ::SetWindowLongPtr(hWnd, propIndex, value); }
/*static*/void * FASTCALL TView::GetWindowProp(HWND hWnd, int propIndex) { return reinterpret_cast<void *>(::GetWindowLongPtr(hWnd, propIndex)); }
/*static*/void * FASTCALL TView::GetWindowUserData(HWND hWnd) { return reinterpret_cast<void *>(::GetWindowLongPtr(hWnd, GWLP_USERDATA)); }
/*static*/long FASTCALL TView::GetWindowStyle(HWND hWnd) { return ::GetWindowLong(hWnd, GWL_STYLE); }
/*static*/long FASTCALL TView::GetWindowExStyle(HWND hWnd) { return ::GetWindowLong(hWnd, GWL_EXSTYLE); }
//
//
//
static BOOL CALLBACK SetupWindowCtrlTextProc(HWND hwnd, LPARAM lParam)
{
	int    ok = -1;
	SString text_buf;
	TView::SGetWindowText(hwnd, text_buf);
	if(text_buf.Len() > 1) {
		SString temp_buf = text_buf;
		int   do_replace = 0;
		if(temp_buf.C(0) == '@') {
			SLS.LoadString_(text_buf+1, temp_buf);
			temp_buf.Transf(CTRANSF_INNER_TO_OUTER);
			do_replace = 1;
		}
		else if(SLS.ExpandString(temp_buf, CTRANSF_UTF8_TO_OUTER) > 0)
			do_replace = 1;
		// @v10.5.4 {
		else {
			if(!temp_buf.IsAscii() && temp_buf.IsLegalUtf8()) {
				temp_buf.Transf(CTRANSF_UTF8_TO_OUTER);
				do_replace = 1;
			}
		}
		// } @v10.5.4 
		if(do_replace) {
			ok = TView::SSetWindowText(hwnd, temp_buf);
		}
	}
	return ok;
}

/*static*/void FASTCALL TView::PreprocessWindowCtrlText(HWND hWnd)
{
	SetupWindowCtrlTextProc(hWnd, 0); // @v10.7.10
	::EnumChildWindows(hWnd, SetupWindowCtrlTextProc, 0);
}

/*static*/int FASTCALL TView::SGetWindowClassName(HWND hWnd, SString & rBuf)
{
	int    ok = 1;
	rBuf.Z();
#ifdef UNICODE
	wchar_t buf[256];
	if(::GetClassName(hWnd, buf, SIZEOFARRAY(buf))) {
		rBuf.CopyUtf8FromUnicode(buf, sstrlen(buf), 0);
		rBuf.Transf(CTRANSF_UTF8_TO_OUTER);
	}
	else {
		ok = SLS.SetOsError();
	}
#else
	char   buf[256];
	if(::GetClassName(hWnd, buf, SIZEOFARRAY(buf))) {
		rBuf = buf;
	}
	else {
		ok = SLS.SetOsError();
	}
#endif // UNICODE
	return ok;
}

/*static*/int FASTCALL TView::SGetWindowText(HWND hWnd, SString & rBuf)
{
	rBuf.Z();
    long  text_len = ::SendMessage(hWnd, WM_GETTEXTLENGTH, 0, 0);
    if(text_len > 0) {
    	void * p_text_ptr = 0;
    	int    is_allocated = 0;
    	uint8  static_buf[4096]; // @v10.9.7 [1024]-->[4096]
#ifdef UNICODE
		if(text_len >= sizeof(static_buf)/sizeof(wchar_t)) {
			p_text_ptr = SAlloc::M((text_len+16) * sizeof(wchar_t)); // @v10.9.7 @fix (text_len * sizeof(wchar_t) + 1)-->((text_len+16) * sizeof(wchar_t))
			is_allocated = 1;
		}
		else
			p_text_ptr = static_buf;
		long actual_len = ::SendMessage(hWnd, WM_GETTEXT, text_len+1, reinterpret_cast<LPARAM>(p_text_ptr));
		rBuf.CopyUtf8FromUnicode(static_cast<wchar_t *>(p_text_ptr), actual_len, 0);
		rBuf.Transf(CTRANSF_UTF8_TO_OUTER);
#else
		if(text_len >= sizeof(static_buf)/sizeof(char)) {
			p_text_ptr = SAlloc::M(text_len * sizeof(char) + 1);
			is_allocated = 1;
		}
		else
			p_text_ptr = static_buf;
		::SendMessage(hWnd, WM_GETTEXT, text_len+1, reinterpret_cast<LPARAM>(p_text_ptr));
		rBuf = (const char *)p_text_ptr;
#endif // UNICODE
		if(is_allocated)
			SAlloc::F(p_text_ptr);
	}
    return (int)rBuf.Len();
}

/*static*/int FASTCALL TView::SSetWindowText(HWND hWnd, const char * pText)
{
	int    ok = 1;
#ifdef UNICODE
	SStringU temp_buf_u;
	temp_buf_u.CopyFromMb(cpANSI, pText, sstrlen(pText));
	ok = BIN(::SendMessage(hWnd, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(temp_buf_u.ucptr())));
#else
	ok = BIN(::SendMessage(hWnd, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(pText)));
#endif // UNICODE
	return ok;
}

/*static*/void * TView::CreateFont(const SFontDescr & rFd)
{
	LOGFONT log_font;
 	MEMSZERO(log_font);
	rFd.MakeLogFont(&log_font);
	HFONT new_font = ::CreateFontIndirect(&log_font);
	return new_font;
}

/*static*/HFONT TView::setFont(HWND hWnd, const char * pFontName, int height)
{
	HFONT   new_font = 0;
	LOGFONT log_font;
	MEMSZERO(log_font);
	log_font.lfCharSet = DEFAULT_CHARSET;
	if(pFontName)
		STRNSCPY(log_font.lfFaceName, SUcSwitch(pFontName)); // @unicodeproblem
	log_font.lfHeight = height;
	new_font = ::CreateFontIndirect(&log_font);
	if(new_font)
		::SendMessage(hWnd, WM_SETFONT, reinterpret_cast<WPARAM>(new_font), TRUE);
	return new_font;
}

/*static*/void TView::CallOnAcceptInputForWordSelExtraBlocks(TGroup * pG)
{
	if(pG) {
		TView * p_temp = pG->GetLastView();
		if(p_temp) {
			const TView * p_term = p_temp;
			do {
				p_temp = p_temp->P_Next;
				if(p_temp && p_temp->IsConsistent()) {
					if(p_temp->P_WordSelBlk) {
						p_temp->P_WordSelBlk->OnAcceptInput(0, 0);
					}
				}
				else
					break;
			} while(p_temp != p_term);
		}
	}
}

int TView::IsConsistent() const
{
	int    ok = 1;
	__try {
		ok = BIN(Sign == SIGN_TVIEW);
	}
	__except(1) {
		ok = 0;
	}
	return ok;
}

int TView::OnDestroy(HWND hWnd)
{
	int    ok = -1;
	if(this && PrevWindowProc) {
		TView::SetWindowProp(hWnd, GWLP_WNDPROC, PrevWindowProc);
		ok = 1;
	}
	return ok;
}

int TView::RestoreOnDestruction()
{
	int    ok = -1;
	if(PrevWindowProc) {
		HWND   h_wnd = getHandle();
		if(IsWindow(h_wnd)) {
			TView::SetWindowProp(h_wnd, GWLP_WNDPROC, PrevWindowProc);
			ok = 1;
		}
	}
	return ok;
}

int TView::SetupText(SString * pText)
{
	int    ok = -1;
	if(P_Owner && P_Owner->IsSubSign(TV_SUBSIGN_DIALOG) && static_cast<TDialog *>(P_Owner)->CheckFlag(TDialog::fExport))
		ok = -1;
	else {
		SString temp_buf;
		SString subst;
		TView::SGetWindowText(GetDlgItem(Parent, Id), temp_buf);
		if(SLS.SubstString(temp_buf, 1, subst) > 0) {
			TView::SSetWindowText(GetDlgItem(Parent, Id), subst);
			ASSIGN_PTR(pText, subst);
			ok = 1;
		}
	}
	return ok;
}

TView * TView::prev() const
{
	TView * res = (TView *)this; // @badcast
	while(res->P_Next != this)
		res = res->P_Next;
	return res;
}

int    FASTCALL TView::IsSubSign(uint sign) const { return BIN(SubSign == sign); }
void   TView::Show(int doShow) { ::ShowWindow(getHandle(), doShow ? SW_SHOW : SW_HIDE); }
int    TView::handleWindowsMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) { return  0; }
TView * TView::nextView() const { return (this == P_Owner->GetLastView()) ? 0 : P_Next; }
TView * TView::prevView() const { return (this == P_Owner->GetFirstView()) ? 0 : prev(); }
int    TView::commandEnabled(ushort command) const { return BIN((command >= 64*32) || !P_CmdSet || P_CmdSet->has(command)); }
int    TView::TransmitData(int dir, void * pData) { return 0; } // Ничего не передается и не получается. Размер данных - 0.

void FASTCALL TView::enableCommands(const TCommandSet & cmds, int isEnable)
{
	if(!P_CmdSet) {
		P_CmdSet = new TCommandSet;
		P_CmdSet->enableAll();
	}
	if(P_CmdSet) {
		if(!(Sf & sfCmdSetChanged))
			if(isEnable) {
				SETFLAG(Sf, sfCmdSetChanged, (*P_CmdSet & cmds) != cmds);
			}
			else {
				SETFLAG(Sf, sfCmdSetChanged, !(*P_CmdSet & cmds).IsEmpty());
			}
		P_CmdSet->enableCmd(cmds, isEnable);
		if(Sf & sfCmdSetChanged) {
			TView::messageBroadcast(this, cmCommandSetChanged);
			Sf &= ~sfCmdSetChanged;
		}
	}
}

void FASTCALL TView::enableCommand(ushort cmd, int isEnable)
{
	if(!P_CmdSet) {
		P_CmdSet = new TCommandSet;
		P_CmdSet->enableAll();
	}
	if(P_CmdSet) {
		if(!(Sf & sfCmdSetChanged))
			if(P_CmdSet->has(cmd)) {
				SETFLAG(Sf, sfCmdSetChanged, !isEnable);
			}
			else {
				SETFLAG(Sf, sfCmdSetChanged, isEnable);
			}
		P_CmdSet->enableCmd(cmd, isEnable);
		if(Sf & sfCmdSetChanged) {
			TView::messageBroadcast(this, cmCommandSetChanged);
			Sf &= ~sfCmdSetChanged;
		}
	}
}

void TView::getCommands(TCommandSet & commands) const
{
	if(!RVALUEPTR(commands, P_CmdSet))
		commands.enableAll();
}

void TView::setCommands(const TCommandSet & cmds)
{
	if(!P_CmdSet) {
		P_CmdSet = new TCommandSet;
		P_CmdSet->enableAll();
	}
	if(P_CmdSet) {
		SETFLAG(Sf, sfCmdSetChanged, BIN((Sf & sfCmdSetChanged) || (*P_CmdSet != cmds)));
		*P_CmdSet = cmds;
	}
}

void TView::ResetOwnerCurrent()
{
	if(P_Owner && P_Owner->IsCurrentView(this))
		P_Owner->SetCurrentView(0, TGroup::normalSelect);
}

TView & TView::SetId(uint id)
{
	Id = id;
	return *this;
}

void   TView::select() { CALLPTRMEMB(P_Owner, SetCurrentView(this, normalSelect)); }
uint   TView::GetId() const { return (this && IsConsistent()) ? static_cast<uint>(Id) : 0; }
int    FASTCALL TView::TestId(uint id) const { return (this && IsConsistent() && id && id == static_cast<uint>(Id)); }
int    FASTCALL TView::IsInState(uint s) const { return ((Sf & s) == s); }
void * FASTCALL TView::MessageCommandToOwner(uint command) { return P_Owner ? TView::messageCommand(P_Owner, command, this) : 0; }
HWND   TView::getHandle() const { return GetDlgItem(Parent, Id); }
int    FASTCALL TView::valid(ushort) { return 1; }

uint TView::getHelpCtx()
{
	uint   ctx = 0;
	TView::messageCommand(this, cmGetHelpContext, &ctx);
	return ctx;
}

void FASTCALL TView::clearEvent(TEvent & event)
{
	event.what = TEvent::evNothing;
	event.message.infoPtr = this;
}

IMPL_HANDLE_EVENT(TView)
{
	if(event.isCmd(cmExecute)) {
		clearEvent(event);
		event.message.infoLong = cmCancel;
	}
	else if(event.isCmd(cmGetHelpContext)) {
		uint * p = static_cast<uint *>(event.message.infoPtr);
		ASSIGN_PTR(p, HelpCtx);
		clearEvent(event);
	}
	else if(TVKEYDOWN) {
		if(TVKEY == kbF1) {
			HWND hw_parent = APPL->H_MainWnd; // GetDesktopWindow()
			SLS.CallHelp(hw_parent, HH_HELP_CONTEXT, HelpCtx);
			clearEvent(event);
		}
	}
}

void TView::setState(uint aState, bool enable)
{
	if(Sf != sfEventBarrier) { // Нельзя произвольно менять флаг sfEventBarrier
		SETFLAG(Sf, aState, enable);
		if(aState & sfDisabled)
			EnableWindow(getHandle(), !(Sf & sfDisabled));
		if(P_Owner) {
			switch(aState) {
				case sfVisible:
					{
						TView * p_label = static_cast<TView *>(TView::messageBroadcast(P_Owner, cmSearchLabel, this));
						CALLPTRMEMB(p_label, setState(aState, enable));
						Show(enable);
					}
					break;
				case sfSelected:
				case sfFocused:
					TView::messageBroadcast(P_Owner, enable ? cmReceivedFocus : cmReleasedFocus, this);
					break;
			}
		}
	}
}

TView * TView::TopView()
{
	TView * p;
	if(APPL->P_TopView)
		p = APPL->P_TopView;
	else {
		for(p = this; p && !(p->Sf & sfModal);)
			p = p->P_Owner;
	}
	return p;
}

void TView::changeBounds(const TRect & rBounds)
{
	TRect new_bounds = rBounds;
	ViewOrigin = new_bounds.a;
	ViewSize = new_bounds.b - new_bounds.a;
	{
		static int _lock = 0; // Блокировка от рекурсии
		ENTER_CRITICAL_SECTION
		if(!_lock) {
			_lock = 1;
			TView::messageCommand(this, cmSetBounds, &new_bounds);
			_lock = 0;
		}
		LEAVE_CRITICAL_SECTION
	}
}

void TView::setBounds(const TRect & bounds)
{
	ViewOrigin = bounds.a;
	ViewSize = bounds.b - bounds.a;
}

void TView::SetWordSelBlock(WordSel_ExtraBlock * pBlk)
{
	ZDELETE(P_WordSelBlk);
	P_WordSelBlk = pBlk;
}
//
//
//
/*KeyDownCommand::KeyDownCommand()
{
	State = 0;
	KeyCode = 0;
}*/

void KeyDownCommand::Clear()
{
	State = 0;
	Code = 0;
}

int KeyDownCommand::GetKeyName(SString & rBuf, int onlySpecKeys) const
{
	int    ok = -1;
	rBuf.Z();
	if(Code) {
		int    is_func_key = BIN(Code >= VK_F1 && Code <= VK_F12);
		if(!onlySpecKeys || is_func_key || State & stateAlt || State & stateCtrl) {
			if(State & stateAlt)
				(rBuf = "Alt").CatChar('-');
			else if(State & stateCtrl)
				(rBuf = "Ctrl").CatChar('-');
			else if(State & stateShift)
				(rBuf = "Shift").CatChar('-');
			if(is_func_key)
				rBuf.CatChar('F').Cat(Code - VK_F1 + 1);
			else
				rBuf.Cat((char)Code);
			ok = 1;
		}
	}
	return ok;
}

void FASTCALL KeyDownCommand::SetWinMsgCode(uint32 wParam)
{
	Code = static_cast<uint16>(wParam);
	State = 0;
	if(GetKeyState(VK_CONTROL) & 0x8000)
		State |= stateCtrl;
	if(GetKeyState(VK_MENU) & 0x8000)
		State |= stateAlt;
	if(GetKeyState(VK_SHIFT) & 0x8000)
		State |= stateShift;
}

struct _TvKeyCodeVK {
	uint16 TvKeyCode;
	uint16 KState;
	uint16 KCode;
};

static _TvKeyCodeVK TvKeyCodeVKList[] = {
	{ kbCtrlA,     KeyDownCommand::stateCtrl, 'A' },
	{ kbCtrlB,     KeyDownCommand::stateCtrl, 'B' },
	{ kbCtrlC,     KeyDownCommand::stateCtrl, 'C' },
	{ kbCtrlD,     KeyDownCommand::stateCtrl, 'D' },
	{ kbCtrlE,     KeyDownCommand::stateCtrl, 'E' },
	{ kbCtrlF,     KeyDownCommand::stateCtrl, 'F' },
	{ kbCtrlG,     KeyDownCommand::stateCtrl, 'G' },
	{ kbCtrlH,     KeyDownCommand::stateCtrl, 'H' },
	{ kbCtrlI,     KeyDownCommand::stateCtrl, 'I' },
	{ kbCtrlJ,     KeyDownCommand::stateCtrl, 'J' },
	{ kbCtrlK,     KeyDownCommand::stateCtrl, 'K' },
	{ kbCtrlL,     KeyDownCommand::stateCtrl, 'L' },
	{ kbCtrlM,     KeyDownCommand::stateCtrl, 'M' },
	{ kbCtrlN,     KeyDownCommand::stateCtrl, 'N' },
	{ kbCtrlO,     KeyDownCommand::stateCtrl, 'O' },
	{ kbCtrlP,     KeyDownCommand::stateCtrl, 'P' },
	{ kbCtrlQ,     KeyDownCommand::stateCtrl, 'Q' },
	{ kbCtrlR,     KeyDownCommand::stateCtrl, 'R' },
	{ kbCtrlS,     KeyDownCommand::stateCtrl, 'S' },
	{ kbCtrlT,     KeyDownCommand::stateCtrl, 'T' },
	{ kbCtrlU,     KeyDownCommand::stateCtrl, 'U' },
	{ kbCtrlV,     KeyDownCommand::stateCtrl, 'V' },
	{ kbCtrlW,     KeyDownCommand::stateCtrl, 'W' },
	{ kbCtrlX,     KeyDownCommand::stateCtrl, 'X' },
	{ kbCtrlY,     KeyDownCommand::stateCtrl, 'Y' },
	{ kbCtrlZ,     KeyDownCommand::stateCtrl, 'Z' },
	{ kbSpace,     0,                          VK_SPACE  },
	{ kbEsc,       0,                          VK_ESCAPE },
	{ kbAltSpace,  KeyDownCommand::stateAlt,   VK_SPACE  },
	{ kbCtrlIns,   KeyDownCommand::stateCtrl,  VK_INSERT },
	{ kbShiftIns,  KeyDownCommand::stateShift, VK_INSERT },
	{ kbCtrlDel,   KeyDownCommand::stateCtrl,  VK_DELETE },
	{ kbShiftDel,  KeyDownCommand::stateShift, VK_DELETE },
	{ kbBack,      0,                          VK_BACK },
	{ kbCtrlBack,  KeyDownCommand::stateCtrl,  VK_BACK },
	{ kbShiftTab,  KeyDownCommand::stateShift, VK_TAB },
	{ kbTab,       0,                          VK_TAB },
	{ kbCtrlTab,   KeyDownCommand::stateCtrl,  VK_TAB },
	{ kbCtrlEnter, KeyDownCommand::stateCtrl,  VK_RETURN },
	{ kbEnter,     0,                          VK_RETURN },
	{ kbAltA,      KeyDownCommand::stateAlt, 'A' },
	{ kbAltB,      KeyDownCommand::stateAlt, 'B' },
	{ kbAltC,      KeyDownCommand::stateAlt, 'C' },
	{ kbAltD,      KeyDownCommand::stateAlt, 'D' },
	{ kbAltE,      KeyDownCommand::stateAlt, 'E' },
	{ kbAltF,      KeyDownCommand::stateAlt, 'F' },
	{ kbAltG,      KeyDownCommand::stateAlt, 'G' },
	{ kbAltH,      KeyDownCommand::stateAlt, 'H' },
	{ kbAltI,      KeyDownCommand::stateAlt, 'I' },
	{ kbAltJ,      KeyDownCommand::stateAlt, 'J' },
	{ kbAltK,      KeyDownCommand::stateAlt, 'K' },
	{ kbAltL,      KeyDownCommand::stateAlt, 'L' },
	{ kbAltM,      KeyDownCommand::stateAlt, 'M' },
	{ kbAltN,      KeyDownCommand::stateAlt, 'N' },
	{ kbAltO,      KeyDownCommand::stateAlt, 'O' },
	{ kbAltP,      KeyDownCommand::stateAlt, 'P' },
	{ kbAltQ,      KeyDownCommand::stateAlt, 'Q' },
	{ kbAltR,      KeyDownCommand::stateAlt, 'R' },
	{ kbAltS,      KeyDownCommand::stateAlt, 'S' },
	{ kbAltT,      KeyDownCommand::stateAlt, 'T' },
	{ kbAltU,      KeyDownCommand::stateAlt, 'U' },
	{ kbAltV,      KeyDownCommand::stateAlt, 'V' },
	{ kbAltW,      KeyDownCommand::stateAlt, 'W' },
	{ kbAltX,      KeyDownCommand::stateAlt, 'X' },
	{ kbAltY,      KeyDownCommand::stateAlt, 'Y' },
	{ kbAltZ,      KeyDownCommand::stateAlt, 'Z' },
	{ kbF1,        0, VK_F1 },
	{ kbF2,        0, VK_F2 },
	{ kbF3,        0, VK_F3 },
	{ kbF4,        0, VK_F4 },
	{ kbF5,        0, VK_F5 },
	{ kbF6,        0, VK_F6 },
	{ kbF7,        0, VK_F7 },
	{ kbF8,        0, VK_F8 },
	{ kbF9,        0, VK_F9 },
	{ kbF10,       0, VK_F10 },
	{ kbF11,       0, VK_F11 },
	{ kbF12,       0, VK_F12 },
	{ kbShiftF1,   KeyDownCommand::stateShift, VK_F1 },
	{ kbShiftF2,   KeyDownCommand::stateShift, VK_F2 },
	{ kbShiftF3,   KeyDownCommand::stateShift, VK_F3 },
	{ kbShiftF4,   KeyDownCommand::stateShift, VK_F4 },
	{ kbShiftF5,   KeyDownCommand::stateShift, VK_F5 },
	{ kbShiftF6,   KeyDownCommand::stateShift, VK_F6 },
	{ kbShiftF7,   KeyDownCommand::stateShift, VK_F7 },
	{ kbShiftF8,   KeyDownCommand::stateShift, VK_F8 },
	{ kbShiftF9,   KeyDownCommand::stateShift, VK_F9 },
	{ kbShiftF10,  KeyDownCommand::stateShift, VK_F10 },
	{ kbShiftF11,  KeyDownCommand::stateShift, VK_F11 }, // @v9.8.7
	{ kbShiftF12,  KeyDownCommand::stateShift, VK_F12 }, // @v9.8.7
	{ kbCtrlF1,    KeyDownCommand::stateCtrl,  VK_F1 },
	{ kbCtrlF2,    KeyDownCommand::stateCtrl,  VK_F2 },
	{ kbCtrlF3,    KeyDownCommand::stateCtrl,  VK_F3 },
	{ kbCtrlF4,    KeyDownCommand::stateCtrl,  VK_F4 },
	{ kbCtrlF5,    KeyDownCommand::stateCtrl,  VK_F5 },
	{ kbCtrlF6,    KeyDownCommand::stateCtrl,  VK_F6 },
	{ kbCtrlF7,    KeyDownCommand::stateCtrl,  VK_F7 },
	{ kbCtrlF8,    KeyDownCommand::stateCtrl,  VK_F8 },
	{ kbCtrlF9,    KeyDownCommand::stateCtrl,  VK_F9 },
	{ kbCtrlF10,   KeyDownCommand::stateCtrl,  VK_F10 },
	{ kbCtrlF11,   KeyDownCommand::stateCtrl,  VK_F11 }, // @v9.8.7
	{ kbCtrlF12,   KeyDownCommand::stateCtrl,  VK_F12 }, // @v9.8.7
	{ kbAltF1,     KeyDownCommand::stateAlt,   VK_F1 },
	{ kbAltF2,     KeyDownCommand::stateAlt,   VK_F2 },
	{ kbAltF3,     KeyDownCommand::stateAlt,   VK_F3 },
	{ kbAltF4,     KeyDownCommand::stateAlt,   VK_F4 },
	{ kbAltF5,     KeyDownCommand::stateAlt,   VK_F5 },
	{ kbAltF6,     KeyDownCommand::stateAlt,   VK_F6 },
	{ kbAltF7,     KeyDownCommand::stateAlt,   VK_F7 },
	{ kbAltF8,     KeyDownCommand::stateAlt,   VK_F8 },
	{ kbAltF9,     KeyDownCommand::stateAlt,   VK_F9 },
	{ kbAltF10,    KeyDownCommand::stateAlt,   VK_F10 },
	{ kbAltF11,    KeyDownCommand::stateAlt,   VK_F11 }, // @v9.8.7
	{ kbAltF12,    KeyDownCommand::stateAlt,   VK_F12 }, // @v9.8.7
	{ kbHome,      0, VK_HOME  },
	{ kbUp,        0, VK_UP    },
	{ kbPgUp,      0, VK_PRIOR },
	{ kbLeft,      0, VK_LEFT  },
	{ kbRight,     0, VK_RIGHT },
	{ kbEnd,       0, VK_END   },
	{ kbDown,      0, VK_DOWN  },
	{ kbPgDn,      0, VK_NEXT  },
	{ kbIns,       0, VK_INSERT },
	{ kbDel,       0, VK_DELETE },
	{ kbCtrlLeft,  KeyDownCommand::stateCtrl, VK_LEFT  },
	{ kbCtrlRight, KeyDownCommand::stateCtrl, VK_RIGHT },
	{ kbCtrlEnd,   KeyDownCommand::stateCtrl, VK_END   },
	{ kbCtrlPgUp,  KeyDownCommand::stateCtrl, VK_PRIOR },
	{ kbCtrlPgDn,  KeyDownCommand::stateCtrl, VK_NEXT  },
	{ kbCtrlHome,  KeyDownCommand::stateCtrl, VK_HOME  },
	{ kbAlt1,      KeyDownCommand::stateAlt,   '1' },
	{ kbAlt2,      KeyDownCommand::stateAlt,   '2' },
	{ kbAlt3,      KeyDownCommand::stateAlt,   '3' },
	{ kbAlt4,      KeyDownCommand::stateAlt,   '4' },
	{ kbAlt5,      KeyDownCommand::stateAlt,   '5' },
	{ kbAlt6,      KeyDownCommand::stateAlt,   '6' },
	{ kbAlt7,      KeyDownCommand::stateAlt,   '7' },
	{ kbAlt8,      KeyDownCommand::stateAlt,   '8' },
	{ kbAlt9,      KeyDownCommand::stateAlt,   '9' },
	{ kbAlt0,      KeyDownCommand::stateAlt,   '0' },
	{ kbAltMinus,  KeyDownCommand::stateAlt,   VK_SUBTRACT },
	{ kbAltEqual,  KeyDownCommand::stateAlt,   VK_SUBTRACT },
	{ kbAltIns,    KeyDownCommand::stateAlt,   VK_INSERT },
	{ kbCtrlPrtSc, KeyDownCommand::stateCtrl,  VK_SNAPSHOT }
};

int FASTCALL KeyDownCommand::SetTvKeyCode(uint16 tvKeyCode)
{
	int    ok = 0;
	Clear();
	for(uint i = 0; !ok && i < SIZEOFARRAY(TvKeyCodeVKList); i++) {
		if(TvKeyCodeVKList[i].TvKeyCode == tvKeyCode) {
			State = TvKeyCodeVKList[i].KState;
			Code = TvKeyCodeVKList[i].KCode;
			ok = 1;
		}
	}
	return ok;
}

uint16 KeyDownCommand::GetTvKeyCode() const
{
	for(uint i = 0; i < SIZEOFARRAY(TvKeyCodeVKList); i++) {
		const _TvKeyCodeVK & r_entry = TvKeyCodeVKList[i];
		if(r_entry.KState == State && r_entry.KCode == Code)
			return r_entry.TvKeyCode;
	}
	return 0;
}

struct _KeySymb {
	int16  K;
	const  char * S;
};

static _KeySymb KeySymbList[] = {
	{ VK_LBUTTON,        "lmouse,leftmouse"},
	{ VK_RBUTTON,        "rmouse,rightmouse"},
	{ VK_MBUTTON,        "mmouse,middlemouse"},
	{ VK_BACK,           "back"},
	{ VK_TAB,            "tab"},
	{ VK_RETURN,         "enter,return,cr"},
	{ VK_SHIFT,          "shift"},
	{ VK_CONTROL,        "ctrl"},
	{ VK_MENU,           "alt,menu"}, // ALT
	{ VK_PAUSE,          "pause"},
	{ VK_CAPITAL,        "caps,capital"},
	{ VK_KANA,           "kana"},
	{ VK_HANGUL,         "hangul,hangeul"},
	{ VK_JUNJA,          "junja"},
	{ VK_FINAL,          "final"},
	{ VK_HANJA,          "hanja"},
	{ VK_KANJI,          "kanji"},
	{ VK_ESCAPE,         "escape,esc"},
	{ VK_CONVERT,        "convert"},
	{ VK_NONCONVERT,     "nonconvert"},
	{ VK_ACCEPT,         "accept"},
	{ VK_MODECHANGE,     "modechange"},
	{ VK_SPACE,          "space,spc"},
	{ VK_PRIOR,          "prior"},
	{ VK_NEXT,           "next"},
	{ VK_END,            "end"},
	{ VK_HOME,           "home,hm"},
	{ VK_LEFT,           "left,lf"},
	{ VK_UP,             "up"},
	{ VK_RIGHT,          "right,rt"},
	{ VK_DOWN,           "down,dn"},
	{ VK_SELECT,         "select"},
	{ VK_PRINT,          "print,prn"},
	{ VK_EXECUTE,        "execute,exec"},
	{ VK_SNAPSHOT,       "snapshot"},
	{ VK_INSERT,         "insert,ins"},
	{ VK_DELETE,         "delete,del"},
	{ VK_HELP,           "help"},
	{ VK_LWIN,           "lwin"},
	{ VK_RWIN,           "rwin"},
	{ VK_APPS,           "apps"},
	{ VK_SLEEP,          "sleep"},
	{ VK_NUMPAD0,        "numpad0"},
	{ VK_NUMPAD1,        "numpad1"},
	{ VK_NUMPAD2,        "numpad2"},
	{ VK_NUMPAD3,        "numpad3"},
	{ VK_NUMPAD4,        "numpad4"},
	{ VK_NUMPAD5,        "numpad5"},
	{ VK_NUMPAD6,        "numpad6"},
	{ VK_NUMPAD7,        "numpad7"},
	{ VK_NUMPAD8,        "numpad8"},
	{ VK_NUMPAD9,        "numpad9"},
	{ VK_MULTIPLY,       "multiply,mult,mul"},
	{ VK_ADD,            "add"},
	{ VK_SEPARATOR,      "separator"},
	{ VK_SUBTRACT,       "subtract,sub"},
	{ VK_DECIMAL,        "decimal,dec"},
	{ VK_DIVIDE,         "divide,div"},
	{ VK_F1,             "f1"},
	{ VK_F2,             "f2"},
	{ VK_F3,             "f3"},
	{ VK_F4,             "f4"},
	{ VK_F5,             "f5"},
	{ VK_F6,             "f6"},
	{ VK_F7,             "f7"},
	{ VK_F8,             "f8"},
	{ VK_F9,             "f9"},
	{ VK_F10,            "f10"},
	{ VK_F11,            "f11"},
	{ VK_F12,            "f12"},
	{ VK_F13,            "f13"},
	{ VK_F14,            "f14"},
	{ VK_F15,            "f15"},
	{ VK_F16,            "f16"},
	{ VK_F17,            "f17"},
	{ VK_F18,            "f18"},
	{ VK_F19,            "f19"},
	{ VK_F20,            "f20"},
	{ VK_F21,            "f21"},
	{ VK_F22,            "f22"},
	{ VK_F23,            "f23"},
	{ VK_F24,            "f24"},
	{ VK_NUMLOCK,        "numlock,num"},
	{ VK_SCROLL,         "scroll"},
	{ VK_OEM_NEC_EQUAL,  "equal"},   // '=' key on numpad
	{ VK_LSHIFT,         "leftshift,lshift"},
	{ VK_RSHIFT,         "rightshift,rshift"},
	{ VK_LCONTROL,       "lctrl"},
	{ VK_RCONTROL,       "rctrl"},
	{ VK_LMENU,          "lmenu"},
	{ VK_RMENU,          "rmenu"},
	{ VK_BROWSER_BACK,        "brwback"},
	{ VK_BROWSER_FORWARD,     "brwforward"},
	{ VK_BROWSER_REFRESH,     "brwrefresh"},
	{ VK_BROWSER_STOP,        "brwstop"},
	{ VK_BROWSER_SEARCH,      "brwsearch"},
	{ VK_BROWSER_FAVORITES,   "brwfav"},
	{ VK_BROWSER_HOME,        "brwhome"},
	{ VK_VOLUME_MUTE,         "volumemute"},
	{ VK_VOLUME_DOWN,         "volumedown"},
	{ VK_VOLUME_UP,           "volumeup"},
	{ VK_MEDIA_NEXT_TRACK,    "medianext"},
	{ VK_MEDIA_PREV_TRACK,    "mediaprev"},
	{ VK_MEDIA_STOP,          "mediastop"},
	{ VK_MEDIA_PLAY_PAUSE,    "mediaplay"},
	{ VK_LAUNCH_MAIL,         "launchmail"},
	{ VK_LAUNCH_MEDIA_SELECT, "launchselect"},
	{ VK_LAUNCH_APP1,         "launchapp1"},
	{ VK_LAUNCH_APP2,         "launchapp2"}
};

int FASTCALL KeyDownCommand::SetCharU(wchar_t chr)
{
	int    ok = 1;
	short  r = VkKeyScanW(chr); 
	if(r == -1) {
		ok = 0;
	}
	else {
        State = 0;
		uint8 _state = (r & 0xff00) >> 8;
		uint8 _vk = (r & 0x00ff);
		if(_state & 0x01)
			State |= stateShift;
		if(_state & 0x02)
			State |= stateCtrl;
		if(_state & 0x04)
			State |= stateAlt;
		Code = _vk;
	}
	return ok;
}

int FASTCALL KeyDownCommand::SetChar(uint chr)
{
	int    ok = 1;
	short r = VkKeyScanA(static_cast<char>(chr)); // @v10.4.6 VkKeyScan-->VkKeyScanA
	//short r2 = VkKeyScanW(static_cast<char>(chr)); // @test
	if(r == -1) {
		ok = 0;
	}
	else {
        State = 0;
		uint8 _state = (r & 0xff00) >> 8;
		uint8 _vk = (r & 0x00ff);
		if(_state & 0x01)
			State |= stateShift;
		if(_state & 0x02)
			State |= stateCtrl;
		if(_state & 0x04)
			State |= stateAlt;
		Code = _vk;
	}
	return ok;
}

uint KeyDownCommand::GetChar() const
{
	uint   c = 0;
	switch(Code) {
		// @v10.8.10 case VK_OEM_PLUS:   c = '+'; break;
		case VK_OEM_PLUS:   c = (State & stateShift) ? '+' : '='; break; // @v10.8.10
		case VK_OEM_COMMA:  c = (State & stateShift) ? '<' : ','; break;
		// @v10.9.0 case VK_OEM_MINUS:  c = '-'; break;
		case VK_OEM_MINUS:  c = (State & stateShift) ? '_' : '-'; break; // @v10.9.0 
		case VK_OEM_PERIOD: c = (State & stateShift) ? '>' : '.'; break;
		case VK_MULTIPLY:   c = '*'; break;
		case VK_ADD:        c = '+'; break;
		// @v10.8.10 case VK_SUBTRACT:   c = '-'; break;
		case VK_SUBTRACT:   c = (State & stateShift) ? '_' : '-'; break; // @v10.8.10
		case VK_DECIMAL:    c = '.'; break;
		case VK_DIVIDE:     c = '/'; break;
		case VK_RETURN:     c = '\x0D'; break;
		case VK_OEM_1:      c = (State & stateShift) ? ':' : ';'; break;
		case VK_OEM_2:      c = (State & stateShift) ? '?' : '/'; break; // @v10.8.0
		case VK_OEM_4:      c = (State & stateShift) ? '{' : '['; break; // @v10.8.0
		case VK_OEM_6:      c = (State & stateShift) ? '}' : ']'; break; // @v10.8.0
		case VK_OEM_7:      c = (State & stateShift) ? '\"' : '\''; break; // @v10.8.0
		default:
			if(Code >= '0' && Code <= '9') {
				if(State & stateShift) {
					switch(Code) {
						case '0': c = ')'; break;
						case '1': c = '!'; break;
						case '2': c = '@'; break;
						case '3': c = '#'; break;
						case '4': c = '$'; break;
						case '5': c = '%'; break;
						case '6': c = '^'; break;
						case '7': c = '&'; break;
						case '8': c = '*'; break;
						case '9': c = '('; break;
					}
				}
				else
					c = Code;
			}
			else if(Code >= 'A' && Code <= 'Z')
				c = (State & stateShift) ? Code : (Code + ('a'-'A'));
			else if(Code >= VK_NUMPAD0 && Code <= VK_NUMPAD9)
				c = (Code - VK_NUMPAD0) + '0';
	}
	return c;
}

int KeyDownCommand::SetKeyName(const char * pStr, uint * pLen)
{
	int    ok = 1;
	size_t len = 0;
	if(isempty(pStr))
		ok = -1;
	else {
		uint16 state = 0;
		uint16 key_code = 0;
		SString temp_buf, key_buf;
		SStrScan scan(pStr);
		const size_t org_offs = scan.Offs;
		scan.Skip();
		if(scan.Get("alt", temp_buf)) {
			state |= stateAlt;
		}
		else if(scan.Get("ctrl", temp_buf) || scan.Get("ctl", temp_buf)) {
			state |= stateCtrl;
		}
		else if(scan.Get("shift", temp_buf) || scan.Get("shft", temp_buf)) {
			state |= stateShift;
		}
		scan.Skip();
		if(state) {
			if(oneof3(scan[0], '-', '+', '_')) {
				scan.Incr();
				scan.Skip();
			}
		}
		{
			StringSet ss(",");
			for(uint i = 0; !key_code && i < SIZEOFARRAY(KeySymbList); i++) {
				const _KeySymb & r_ks = KeySymbList[i];
				ss.setBuf(r_ks.S, sstrlen(r_ks.S)+1);
				for(uint p = 0; !key_code && ss.get(&p, key_buf);) {
					if(scan.Get(key_buf, temp_buf)) {
						key_code = r_ks.K;
					}
				}
			}
			if(!key_code) {
				if(scan[0] >= '0' && scan[0] <= '9') {
					key_code = scan[0];
					scan.Incr();
					scan.Skip();
				}
				else if(scan[0] >= 'A' && scan[0] <= 'Z') {
					key_code = scan[0];
					scan.Incr();
					scan.Skip();
				}
			}
		}
		len = scan.Offs - org_offs;
		State = state;
		Code  = key_code;
		if(!Code && !State)
			ok = -1;
	}
	ASSIGN_PTR(pLen, len);
	return ok;
}

TEvent::TEvent()
{
	THISZERO();
}

TEvent & TEvent::setCmd(uint cmd, TView * pInfoView)
{
	what = evCommand;
	message.command = static_cast<ushort>(cmd);
	message.infoPtr = pInfoView;
	return *this;
}

TEvent & TEvent::setWinCmd(uint uMsg, WPARAM wParam, LPARAM lParam)
{
	what = evWinCmd;
	message.command = uMsg;
	message.WP = wParam;
	message.LP = lParam;
	return *this;
}

uint TEvent::getCtlID() const { return message.infoView->GetId(); }
int FASTCALL TEvent::isCtlEvent(uint ctlID) const { return message.infoView->TestId(ctlID); }
int FASTCALL TEvent::isCmd(uint cmd) const { return (what == evCommand && message.command == cmd); }
int FASTCALL TEvent::isKeyDown(uint keyCode) const { return (what == evKeyDown && keyDown.keyCode == keyCode); }
int FASTCALL TEvent::isCbSelected(uint ctlID) const { return (what == evCommand && message.command == cmCBSelected && message.infoView->TestId(ctlID)); }
int FASTCALL TEvent::isClusterClk(uint ctlID) const { return (what == evCommand && message.command == cmClusterClk && message.infoView->TestId(ctlID)); }
int FASTCALL TEvent::wasFocusChanged(uint ctlID) const { return BIN(what == evBroadcast && message.command == cmChangedFocus && message.infoView->TestId(ctlID)); }

int TEvent::wasFocusChanged2(uint ctl01, uint ctl02) const
{
	return BIN(what == evBroadcast && message.command == cmChangedFocus &&
		(message.infoView->TestId(ctl01) || message.infoView->TestId(ctl02)));
}

int TEvent::wasFocusChanged3(uint ctl01, uint ctl02, uint ctl03) const
{
	return BIN(what == evBroadcast && message.command == cmChangedFocus &&
		(message.infoView->TestId(ctl01) || message.infoView->TestId(ctl02) || message.infoView->TestId(ctl03)));
}
//
//
//
TGroup::TGroup(const TRect & bounds) : TView(bounds), P_Last(0), P_Current(0), MsgLockFlags(0)
{
	ViewOptions |= ofSelectable;
}

TGroup::~TGroup()
{
	TView * p = P_Last;
	if(p) do {
		if(p->IsConsistent()) {
			TView * p_prev = p->prev();
			delete p;
			p = p_prev;
		}
		else
			break;
	} while(P_Last);
	P_Current = 0;
}

void TGroup::forEach(void (*func)(TView*, void *), void *args)
{
	TView * p_term = P_Last;
	TView * p_temp = P_Last;
	if(p_temp) do {
		p_temp = p_temp->P_Next;
		func(p_temp, args);
	} while(p_temp != p_term);
}

void TGroup::removeView(TView * p)
{
	if(P_Last) {
		TView * t = P_Last;
		do {
			if(t->P_Next == p) {
				t->P_Next = p->P_Next;
				if(p != P_Last)
					return;
				P_Last = (p->P_Next == p) ? 0 : t;
				break;
			}
			t = t->P_Next;
		} while(t != P_Last);
	}
}

static void addSubviewDataSize(TView * p, void * pSize)
{
	*static_cast<int *>(pSize) += static_cast<TGroup *>(p)->TransmitData(0, 0);
}

int TGroup::TransmitData(int dir, void * pData)
{
	int    s = 0;
	if(dir == 0) {
		forEach(addSubviewDataSize, &s);
	}
	else {
		TView * v = P_Last;
		if(v) do {
			v->TransmitData(dir, pData ? (PTR8(pData) + s) : 0);
			s += v->TransmitData(0, 0);
		} while((v = v->prev()) != P_Last);
	}
	return s;
}

void FASTCALL TGroup::remove(TView * p)
{
	if(p) {
		removeView(p);
		p->P_Owner = 0;
		p->P_Next = 0;
	}
}

ushort FASTCALL TGroup::execView(TWindow * p)
{
	ushort retval = cmCancel;
	if(p) {
		const uint32 save_options = p->ViewOptions;
		TGroup  * save_owner = p->P_Owner;
		TWindow * save_top_view = APPL->P_TopView;
		TView   * save_current = P_Current;
		TCommandSet save_commands;
		getCommands(save_commands);
		APPL->P_TopView = p;
		p->ViewOptions &= ~ofSelectable;
		p->setState(sfModal, true);
		SetCurrentView(p, enterSelect);
		if(save_owner == 0)
			Insert_(p);
		{
			TEvent event;
			p->handleEvent(event.setCmd(cmExecute, 0));
			retval = (event.what == TEvent::evNothing) ? static_cast<ushort>(event.message.infoLong) : 0;
		}
		if(save_owner == 0)
			remove(p);
		SetCurrentView(save_current, leaveSelect);
		p->setState(sfModal, false);
		p->ViewOptions = save_options;
		APPL->P_TopView = save_top_view;
		setCommands(save_commands);
	}
	return retval;
}

TView * TGroup::GetFirstView() const
{
	return P_Last ? P_Last->P_Next : 0;
}

struct handleStruct {
	TEvent * event;
	TGroup * grp;
	TView::phaseType phase;
};

static void doHandleEvent(TView * p, void * s)
{
	handleStruct * ptr = static_cast<handleStruct *>(s);
	if(ptr->event->what && p && p->IsConsistent() && !(p->IsInState(sfDisabled) && (ptr->event->what & (positionalEvents|focusedEvents)))) {
		switch(ptr->phase) {
			case TView::phPreProcess:
				if(p->ViewOptions & ofPreProcess)
					p->handleEvent(*ptr->event);
				break;
			case TView::phPostProcess:
				if(p->ViewOptions & ofPostProcess)
					p->handleEvent(*ptr->event);
				break;
			default:
				p->handleEvent(*ptr->event);
				break;
		}
	}
}

IMPL_HANDLE_EVENT(TGroup)
{
	if(event.isCmd(cmExecute)) {
		clearEvent(event);
		event.message.infoLong = 0;
	}
	else if(event.isCmd(cmDraw)) {
		redraw();
	}
	else {
		TView::handleEvent(event);
		handleStruct hs;
		if(event.what != TEvent::evNothing) {
			hs.event = &event;
			hs.grp = this;
			if(event.what & focusedEvents) {
				hs.phase = phPreProcess;
				forEach(doHandleEvent, &hs);
				hs.phase = phFocused;
				doHandleEvent(P_Current, &hs);
				hs.phase = phPostProcess;
				forEach(doHandleEvent, &hs);
			}
			else {
				hs.phase = phFocused;
				forEach(doHandleEvent, &hs);
			}
		}
	}
}

void TGroup::Insert_(TView * p)
{
	insertBefore(p, GetFirstView());
}

void TGroup::insertBefore(TView * p, TView * pTarget)
{
	if(p && !p->P_Owner && (!pTarget || pTarget->P_Owner == this))
		insertView(p, pTarget);
}

void TGroup::insertView(TView * p, TView * Target)
{
	p->P_Owner = this;
	if(Target) {
		Target = Target->prev();
		p->P_Next = Target->P_Next;
		Target->P_Next = p;
	}
	else {
		if(P_Last == 0)
			p->P_Next = p;
		else {
			p->P_Next = P_Last->P_Next;
			P_Last->P_Next = p;
		}
		P_Last = p;
	}
}

void TGroup::redraw()
{
	for(TView * p = GetFirstView(); p != 0; p = p->nextView())
		p->Draw_();
}

void TGroup::selectNext(/*Boolean forwards*/ /*false*/)
{
	if(P_Current) {
		TView * p = P_Current;
		do {
			p = p->prev();
		} while(!(p == P_Current || (p->IsInState(sfVisible) && !p->IsInState(sfDisabled) && p->ViewOptions & ofSelectable)));
		p->select();
	}
}

void TGroup::SetCurrentView(TView * p, selectMode mode)
{
	if(P_Current != p || mode == forceSelect) {
		TView * p_save_current = P_Current;
		if(mode != enterSelect) {
			CALLPTRMEMB(P_Current, setState(sfSelected, false));
		}
		if(mode != leaveSelect) {
			CALLPTRMEMB(p, setState(sfSelected, true));
		}
		if(IsInState(sfFocused) && p)
			p->setState(sfFocused, true);
		if(!p || p->IsConsistent()) {
			P_Current = p;
			if(p && p->Parent) {
				HWND   ctrl = p->getHandle();
				if(ctrl || (ctrl = GetDlgItem(p->Parent, MAKE_BUTTON_ID(p->GetId(), 1))))
					SetFocus(ctrl);
				P_Current->Draw_();
			}
		}
		if(!(MsgLockFlags & TGroup::fLockMsgChangedFocus)) {
			MsgLockFlags |= TGroup::fLockMsgChangedFocus;
			TView::messageBroadcast(this, cmChangedFocus, p_save_current);
			MsgLockFlags &= ~TGroup::fLockMsgChangedFocus;
		}
	}
}

void TGroup::setState(uint aState, bool enable)
{
	TView::setState(aState, enable);
	if(aState & sfActive) {
		TView * p_term = P_Last;
		TView * p_temp = P_Last;
		if(p_temp) do {
			p_temp = p_temp->P_Next;
			p_temp->setState(aState, enable);
		} while(p_temp != p_term);
	}
	if(aState & sfFocused && P_Current)
		P_Current->setState(sfFocused, enable);
}

int FASTCALL TGroup::valid(ushort command)
{
	{
		int    ok = 1;
		TView * p_term = P_Last;
		TView * p_temp = P_Last;
		if(p_temp) do {
			p_temp = p_temp->P_Next;
			if(!p_temp->valid(command)) {
				ok = 0;
			}
		} while(ok && p_temp != p_term);
		return ok;
	}
}

uint TGroup::GetCurrId() const { return P_Current ? P_Current->GetId() : 0; }
int FASTCALL TGroup::IsCurrentView(const TView * pV) const { return BIN(pV && P_Current == pV); }
int FASTCALL TGroup::isCurrCtlID(uint ctlID) const { return (P_Current && P_Current->TestId(ctlID)); }
