// TVIEW.CPP  Turbo Vision 1.0
// Copyright (c) 1991 by Borland International
//
#include <slib.h>
#include <tv.h>
#pragma hdrstop
#include <htmlhelp.h> // @Muxa
//
//
void FASTCALL ZDeleteWinGdiObject(void * pHandle)
{
	if(pHandle) {
		HGDIOBJ * p_obj = (HGDIOBJ *)pHandle;
		if(*p_obj) {
			::DeleteObject(*p_obj);
			*p_obj = 0;
		}
	}
}
//
//
// static
/* @v9.5.5 void * TView::message(TView * pReceiver, uint what, uint command)
{
	void * p_ret = 0;
	if(pReceiver) {
		TEvent event;
		event.what = what;
		event.message.command = command;
		pReceiver->handleEvent(event);
		if(event.what == evNothing)
			p_ret = event.message.infoPtr;
	}
	return p_ret;
}*/

// static
/* @v9.5.5 void * TView::message(TView * pReceiver, uint what, uint command, void * pInfoPtr)
{
	void * p_ret = 0;
	if(pReceiver) {
		TEvent event;
		event.what = what;
		event.message.command = command;
		event.message.infoPtr = pInfoPtr;
		pReceiver->handleEvent(event);
		if(event.what == evNothing)
			p_ret = event.message.infoPtr;
	}
	return p_ret;
}*/

// static
/* @v9.5.5 void * TView::message(TView * pReceiver, uint what, uint command, long infoVal)
{
	void * p_ret = 0;
	if(pReceiver) {
		TEvent event;
		event.what = what;
		event.message.command = command;
		event.message.infoLong = infoVal;
		pReceiver->handleEvent(event);
		if(event.what == evNothing)
			p_ret = event.message.infoPtr;
	}
	return p_ret;
}*/

//static 
void * FASTCALL TView::messageCommand(TView * pReceiver, uint command)
{
	void * p_ret = 0;
	if(pReceiver) {
		TEvent event;
		event.what = evCommand;
		event.message.command = command;
		pReceiver->handleEvent(event);
		if(event.what == evNothing)
			p_ret = event.message.infoPtr;
	}
	return p_ret;
}

//static 
void * TView::messageCommand(TView * pReceiver, uint command, void * pInfoPtr)
{
	void * p_ret = 0;
	if(pReceiver) {
		TEvent event;
		event.what = evCommand;
		event.message.command = command;
		event.message.infoPtr = pInfoPtr;
		pReceiver->handleEvent(event);
		if(event.what == evNothing)
			p_ret = event.message.infoPtr;
	}
	return p_ret;
}

//static 
void * FASTCALL TView::messageBroadcast(TView * pReceiver, uint command)
{
	void * p_ret = 0;
	if(pReceiver) {
		TEvent event;
		event.what = evBroadcast;
		event.message.command = command;
		pReceiver->handleEvent(event);
		if(event.what == evNothing)
			p_ret = event.message.infoPtr;
	}
	return p_ret;
}

//static 
void * TView::messageBroadcast(TView * pReceiver, uint command, void * pInfoPtr)
{
	void * p_ret = 0;
	if(pReceiver) {
		TEvent event;
		event.what = evBroadcast;
		event.message.command = command;
		event.message.infoPtr = pInfoPtr;
		pReceiver->handleEvent(event);
		if(event.what == evNothing)
			p_ret = event.message.infoPtr;
	}
	return p_ret;
}
//
//
//
TCommandSet::TCommandSet()
{
	resetbitstring(cmds, sizeof(cmds));
}

TCommandSet::TCommandSet(const TCommandSet & tc)
{
	memcpy(cmds, tc.cmds, sizeof(cmds));
}

int TCommandSet::has(int cmd) const
{
	return getbit32(cmds, sizeof(cmds), cmd);
}

void TCommandSet::enableAll()
{
	memset(cmds, 0xff, sizeof(cmds));
}

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
	TCommandSet temp(tc1);
	return (temp |= tc2);
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

void TCommandSet::operator += (int cmd)
	{ enableCmd(cmd, 1);  }
void TCommandSet::operator -= (int cmd)
	{ enableCmd(cmd, 0); }
void TCommandSet::operator += (const TCommandSet & tc)
	{ enableCmd(tc, 1); }
void TCommandSet::operator -= (const TCommandSet & tc)
	{ enableCmd(tc, 0); }
int  TCommandSet::loc(int cmd)
	{ return (cmd >> 3); }
int  TCommandSet::mask(int cmd)
	{ return (1 << (cmd & 0x07)); }
int  operator != (const TCommandSet& tc1, const TCommandSet& tc2)
	{ return !operator == (tc1, tc2); }
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
		h = (HBITMAP)List.at(p).H;
	}
	else {
		h = (HBITMAP)LoadImage(APPL->GetInst(), MAKEINTRESOURCE(bmpId), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
		// h = ::LoadBitmap(APPL->GetInst(), MAKEINTRESOURCE(bmpId));
		if(h) {
			Entry new_entry;
			new_entry.ID = bmpId;
			new_entry.H = (uint32)h;
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
		h = (HBITMAP)List.at(p).H;
	}
	else {
		h = (HBITMAP)LoadImage(0, MAKEINTRESOURCE(bmpId), IMAGE_BITMAP, 0, 0, LR_DEFAULTSIZE|LR_SHARED);
		if(h) {
			Entry new_entry;
			new_entry.ID = bmpId;
			new_entry.H = (uint32)h;
			List.insert(&new_entry);
		}
	}
	LEAVE_CRITICAL_SECTION
	return h;
}
//
//
//
TView::EvBarrier::EvBarrier(TView * pV)
{
	assert(pV);
	P_V = pV;
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

TView::TView(const TRect & bounds)
{
	Sign = SIGN_TVIEW;
	SubSign = 0;
	Id = 0;
	Reserve = 0;
	Sf = (sfVisible | sfMsgToParent);
	options = 0;
	EndModalCmd = 0;
	HelpCtx = 0;
	next = 0;
	owner = 0;
	Parent = 0;
	PrevWindowProc = 0;
	setBounds(bounds);
	P_CmdSet = 0;
	P_WordSelBlk = 0;
}

TView::TView()
{
	Sign = SIGN_TVIEW;
	SubSign = 0;
	Id = 0;
	Reserve = 0;
	Sf = (sfVisible | sfMsgToParent);
	options = 0;
	EndModalCmd = 0;
	HelpCtx = 0;
	next = 0;
	owner = 0;
	Parent = 0;
	PrevWindowProc = 0;
	P_CmdSet = 0;
	P_WordSelBlk = 0;
}

TView::~TView()
{
	ZDELETE(P_WordSelBlk);
	CALLPTRMEMB(owner, remove(this));
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

int TView::GetEndModalCmd() const
{
	return EndModalCmd;
}

void TView::Draw_()
{
	TView::messageCommand(this, cmDraw);
}

/* @v9.1.3 int TView::Paint_(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	return -1;
}*/

//static
void * TView::SetWindowProp(HWND hWnd, int propIndex, void * ptr)
{
	return reinterpret_cast<void *>(::SetWindowLongPtr(hWnd, propIndex, reinterpret_cast<LONG_PTR>(ptr)));
}

//static
void * TView::SetWindowUserData(HWND hWnd, void * ptr)
{
	return reinterpret_cast<void *>(::SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(ptr)));
}

//static
long TView::SetWindowProp(HWND hWnd, int propIndex, long value)
{
	return ::SetWindowLongPtr(hWnd, propIndex, value);
}

//static
void * FASTCALL TView::GetWindowProp(HWND hWnd, int propIndex)
{
	return reinterpret_cast<void *>(::GetWindowLongPtr(hWnd, propIndex));
}

//static 
void * FASTCALL TView::GetWindowUserData(HWND hWnd)
{
	return reinterpret_cast<void *>(::GetWindowLongPtr(hWnd, GWLP_USERDATA));
}

//static
long FASTCALL TView::GetWindowStyle(HWND hWnd)
{
	return ::GetWindowLong(hWnd, GWL_STYLE);
}

//static
long FASTCALL TView::GetWindowExStyle(HWND hWnd)
{
	return ::GetWindowLong(hWnd, GWL_EXSTYLE);
}
//
//
//
static BOOL CALLBACK SetupWindowCtrlTextProc(HWND hwnd, LPARAM lParam)
{
	int    ok = -1;
	//char   text[512];
	//::GetWindowText(hwnd, text, sizeof(text));
	//size_t tlen = sstrlen(text);
	SString text_buf;
	TView::SGetWindowText(hwnd, text_buf); // @v9.1.5
	if(text_buf.Len() > 1) {
		SString temp_buf = text_buf;
		int   do_replace = 0;
		if(temp_buf.C(0) == '@') {
			SLS.LoadString(text_buf+1, temp_buf);
			temp_buf.Transf(CTRANSF_INNER_TO_OUTER);
			do_replace = 1;
		}
		else if(SLS.ExpandString(temp_buf, CTRANSF_UTF8_TO_OUTER) > 0)
			do_replace = 1;
		if(do_replace) {
			//temp_buf.CopyTo(text, sizeof(text));
			//ok = ::SetWindowText(hwnd, text) ? 1 : 0;
			ok = TView::SSetWindowText(hwnd, temp_buf);
		}
	}
	return ok;
}

//static
void FASTCALL TView::PreprocessWindowCtrlText(HWND hWnd)
{
	::EnumChildWindows(hWnd, SetupWindowCtrlTextProc, 0);
}

//static
int FASTCALL TView::SGetWindowClassName(HWND hWnd, SString & rBuf)
{
	int    ok = 1;
	rBuf.Z();
#ifdef UNICODE
	wchar_t buf[256];
	int    buf_len = sizeof(buf) / sizeof(buf[0]);
	if(::GetClassName(hWnd, buf, buf_len)) {
		rBuf.CopyUtf8FromUnicode(buf, strlen(buf), 0);
		rBuf.Transf(CTRANSF_UTF8_TO_OUTER);
	}
	else {
		ok = SLS.SetOsError();
	}
#else
	char   buf[256];
	int    buf_len = sizeof(buf) / sizeof(buf[0]);
	if(::GetClassName(hWnd, buf, buf_len)) {
		rBuf = buf;
	}
	else {
		ok = SLS.SetOsError();
	}
#endif // UNICODE
	return ok;
}

//static
int FASTCALL TView::SGetWindowText(HWND hWnd, SString & rBuf)
{
	rBuf.Z();
    long  text_len = ::SendMessage(hWnd, WM_GETTEXTLENGTH, 0, 0);
    if(text_len > 0) {
    	void * p_text_ptr = 0;
    	int    is_allocated = 0;
    	uint8  static_buf[1024];
#ifdef UNICODE
		if(text_len >= sizeof(static_buf)/sizeof(wchar_t)) {
			p_text_ptr = SAlloc::M(text_len * sizeof(wchar_t) + 1);
			is_allocated = 1;
		}
		else
			p_text_ptr = static_buf;
		long actual_len = ::SendMessage(hWnd, WM_GETTEXT, text_len+1, (LPARAM)p_text_ptr);
		rBuf.CopyUtf8FromUnicode((wchar_t *)p_text_ptr, actual_len, 0);
		rBuf.Transf(CTRANSF_UTF8_TO_OUTER);
#else
		if(text_len >= sizeof(static_buf)/sizeof(char)) {
			p_text_ptr = SAlloc::M(text_len * sizeof(char) + 1);
			is_allocated = 1;
		}
		else
			p_text_ptr = static_buf;
		::SendMessage(hWnd, WM_GETTEXT, text_len+1, (LPARAM)p_text_ptr);
		rBuf = (const char *)p_text_ptr;
#endif // UNICODE
		if(is_allocated)
			SAlloc::F(p_text_ptr);
	}
    return (int)rBuf.Len();
}

//static
int FASTCALL TView::SSetWindowText(HWND hWnd, const char * pText)
{
	int    ok = 1;
#ifdef UNICODE
	SStringU temp_buf_u;
	temp_buf_u.CopyFromMb(cpANSI, pText, strlen(pText));
	ok = BIN(::SendMessage(hWnd, WM_SETTEXT, 0, (LPARAM)(wchar_t *)temp_buf_u));
#else
	ok = BIN(::SendMessage(hWnd, WM_SETTEXT, 0, (LPARAM)pText));
#endif // UNICODE
	return ok;
}

//static
void * TView::CreateFont(const SFontDescr & rFd)
{
	LOGFONT log_font;
 	MEMSZERO(log_font);
	rFd.MakeLogFont(&log_font);
	HFONT new_font = ::CreateFontIndirect(&log_font);
	return new_font;
}
//
// static
HFONT TView::setFont(HWND hWnd, const char * pFontName, int height)
{
	HFONT   new_font = 0;
	LOGFONT log_font;
	MEMSZERO(log_font);
	log_font.lfCharSet = DEFAULT_CHARSET;
	if(pFontName)
		STRNSCPY(log_font.lfFaceName, pFontName);
	log_font.lfHeight = height;
	new_font = ::CreateFontIndirect(&log_font);
	if(new_font)
		::SendMessage(hWnd, WM_SETFONT, (WPARAM)new_font, TRUE);
	return new_font;
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

int FASTCALL TView::IsSubSign(uint sign) const
{
	return BIN(SubSign == sign);
}

uint TView::GetSubSign() const
{
	return SubSign;
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
	if(owner && owner->IsSubSign(TV_SUBSIGN_DIALOG) && ((TDialog *)owner)->CheckFlag(TDialog::fExport))
		ok = -1;
	else {
		//char   temp_buf[1024];
		SString temp_buf;
		SString subst;
		//temp_buf[0] = 0;
		//int    nc = SendDlgItemMessage(Parent, Id, WM_GETTEXT, sizeof(temp_buf), (long)temp_buf);
		TView::SGetWindowText(GetDlgItem(Parent, Id), temp_buf); // @v9.1.5
		if(SLS.SubstString(temp_buf, 1, subst) > 0) {
			//SendDlgItemMessage(Parent, Id, WM_SETTEXT, 0, (LPARAM)(const char *)subst);
			TView::SSetWindowText(GetDlgItem(Parent, Id), subst); // @v9.1.5
			ASSIGN_PTR(pText, subst);
			ok = 1;
		}
	}
	return ok;
}

void TView::Show(int doShow)
{
	ShowWindow(getHandle(), doShow ? SW_SHOW : SW_HIDE);
}

TView * TView::prev()
{
	TView * res = this;
	while(res->next != this)
		res = res->next;
	return res;
}

int TView::handleWindowsMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	return  0;
}

TView * TView::nextView()
{
	return (this == owner->P_Last) ? 0 : next;
}

TView * TView::prevView()
{
	return (this == owner->first()) ? 0 : prev();
}

int TView::commandEnabled(ushort command) const
{
	return BIN((command >= 64*32) || !P_CmdSet || P_CmdSet->has(command));
}

int TView::TransmitData(int dir, void * pData)
{
	return 0; // Ничего не передается и не получается. Размер данных - 0.
}

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

/* @v9.6.2 replaced by Draw_()
void TView::drawView()
{
	Draw_();
}
*/

/* @v9.0.4
void TView::endModal(ushort command)
{
	TView * p_top = TopView();
	CALLPTRMEMB(p_top, endModal(command));
}
*/

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

void TView::select()
{
	CALLPTRMEMB(owner, setCurrent(this, normalSelect));
}

void TView::ResetOwnerCurrent()
{
	if(owner && owner->P_Current == this)
		owner->setCurrent(0, TGroup::normalSelect);
}

/* @v9.0.1
TRect TView::getExtent() const
{
	return TRect(0, 0, size.x, size.y);
}
*/

TView & TView::SetId(uint id)
{
	Id = id;
	return *this;
}

uint TView::GetId() const
{
	return (this && IsConsistent()) ? (uint)Id : 0;
}

int FASTCALL TView::TestId(uint id) const
{
	return (this && IsConsistent() && id && id == (uint)Id);
}

int FASTCALL TView::IsInState(uint s) const
{
	return BIN((Sf & s) == s);
}

uint TView::getHelpCtx()
{
	uint   ctx = 0;
	TView::messageCommand(this, cmGetHelpContext, &ctx);
	return ctx;
}

HWND TView::getHandle() const
{
	return GetDlgItem(Parent, Id);
}

void FASTCALL TView::clearEvent(TEvent & event)
{
	event.what = evNothing;
	event.message.infoPtr = this;
}

IMPL_HANDLE_EVENT(TView)
{
	if(event.isCmd(cmExecute)) {
		clearEvent(event);
		event.message.infoLong = cmCancel;
	}
	else if(event.isCmd(cmGetHelpContext)) {
		uint * p = (uint *)event.message.infoPtr;
		ASSIGN_PTR(p, HelpCtx);
		clearEvent(event);
	}
	else if(TVKEYDOWN) {
		if(TVKEY == kbF1) {
			HWND hw_parent = APPL->H_MainWnd; // GetDesktopWindow()
			SLS.CallHelp((uint32)hw_parent, HH_HELP_CONTEXT, HelpCtx);
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
		if(owner) {
			switch(aState) {
				case sfVisible:
					{
						TView * p_label = (TView *)TView::messageBroadcast(owner, cmSearchLabel, this);
						CALLPTRMEMB(p_label, setState(aState, enable));
						Show(enable);
					}
					break;
				case sfSelected:
				case sfFocused:
					TView::messageBroadcast(owner, enable ? cmReceivedFocus : cmReleasedFocus, this);
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
			p = p->owner;
	}
	return p;
}

void TView::changeBounds(const TRect & rBounds)
{
	TRect new_bounds = rBounds;
	origin = new_bounds.a;
	size = new_bounds.b - new_bounds.a;
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

int FASTCALL TView::valid(ushort)
{
	return 1;
}

void TView::setBounds(const TRect & bounds)
{
	origin = bounds.a;
	size = bounds.b - bounds.a;
}

void TView::SetWordSelBlock(WordSel_ExtraBlock * pBlk)
{
	ZDELETE(P_WordSelBlk);
	P_WordSelBlk = pBlk;
}
//
//
//
/*
SLAPI KeyDownCommand::KeyDownCommand()
{
	State = 0;
	KeyCode = 0;
}
*/

int SLAPI KeyDownCommand::GetKeyName(SString & rBuf, int onlySpecKeys) const
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

int FASTCALL KeyDownCommand::SetWinMsgCode(uint32 wParam)
{
	Code = (uint16)wParam;
	State = 0;
	if(GetKeyState(VK_CONTROL) & 0x8000)
		State |= stateCtrl;
	if(GetKeyState(VK_MENU) & 0x8000)
		State |= stateAlt;
	if(GetKeyState(VK_SHIFT) & 0x8000)
		State |= stateShift;
	return 1;
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

int FASTCALL KeyDownCommand::SetChar(uint chr)
{
	int    ok = 1;
	short r = VkKeyScan(chr);
	if(r == -1) {
		ok = 0;
	}
	else {
        State = 0;
		uint8 _state = (r & 0xff00) >> 16;
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

uint SLAPI KeyDownCommand::GetChar() const
{
	uint   c = 0;
	switch(Code) {
		case VK_OEM_PLUS:   c = '+'; break;
		case VK_OEM_COMMA:  c = ','; break;
		case VK_OEM_MINUS:  c = '-'; break;
		case VK_OEM_PERIOD: c = '.'; break;
		case VK_MULTIPLY:   c = '*'; break;
		case VK_ADD:        c = '+'; break;
		case VK_SUBTRACT:   c = '-'; break;
		case VK_DECIMAL:    c = '.'; break;
		case VK_DIVIDE:     c = '/'; break;
		case VK_RETURN:     c = '\x0D'; break;
		default:
			if(Code >= '0' && Code <= '9')
				c = Code;
			else if(Code >= 'A' && Code <= 'Z')
				c = Code;
			else if(Code >= VK_NUMPAD0 && Code <= VK_NUMPAD9)
				c = (Code - VK_NUMPAD0) + '0';
	}
	return c;
}

int SLAPI KeyDownCommand::SetKeyName(const char * pStr, uint * pLen)
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
				ss.setBuf(r_ks.S, strlen(r_ks.S)+1);
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
	message.command = (ushort)cmd;
	message.infoPtr = pInfoView;
	return *this;
}

uint TEvent::getCtlID() const
{
	return message.infoView->GetId();
}

int FASTCALL TEvent::isCtlEvent(uint ctlID) const
{
	return message.infoView->TestId(ctlID);
}

int FASTCALL TEvent::isCmd(uint cmd) const
{
	return (what == evCommand && message.command == cmd);
}

int FASTCALL TEvent::isKeyDown(uint keyCode) const
{
	return (what == evKeyDown && keyDown.keyCode == keyCode);
}

int FASTCALL TEvent::isCbSelected(uint ctlID) const
{
	return (what == evCommand && message.command == cmCBSelected && message.infoView->TestId(ctlID));
}

int FASTCALL TEvent::isClusterClk(uint ctlID) const
{
	return (what == evCommand && message.command == cmClusterClk && message.infoView->TestId(ctlID));
}

int FASTCALL TEvent::wasFocusChanged(uint ctlID) const
{
	return BIN(what == evBroadcast && message.command == cmChangedFocus && message.infoView->TestId(ctlID));
}

int SLAPI TEvent::wasFocusChanged2(uint ctl01, uint ctl02) const
{
	return BIN(what == evBroadcast && message.command == cmChangedFocus &&
		(message.infoView->TestId(ctl01) || message.infoView->TestId(ctl02)));
}

int SLAPI TEvent::wasFocusChanged3(uint ctl01, uint ctl02, uint ctl03) const
{
	return BIN(what == evBroadcast && message.command == cmChangedFocus &&
		(message.infoView->TestId(ctl01) || message.infoView->TestId(ctl02) || message.infoView->TestId(ctl03)));
}
//
//
//
SLAPI TGroup::TGroup(const TRect & bounds) : TView(bounds)
{
	P_Last = 0;
	P_Current = 0;
	// @v9.0.1 Phase_ = phFocused;
	MsgLockFlags = 0;
	options |= ofSelectable;
}

SLAPI TGroup::~TGroup()
{
	TView * p = P_Last;
	if(p) do {
		if(p->IsConsistent()) { // @v8.0.6
			TView * p_prev = p->prev();
			delete p;
			p = p_prev;
		}
		else
			break;
	} while(P_Last);
	P_Current = 0;
}

/* @v9.4.8 TView * FASTCALL TGroup::at(short index) const
{
	TView * p_temp = P_Last;
	while(index-- > 0)
		p_temp = p_temp->next;
	return p_temp;
}*/

/* @v9.0.1
TView * TGroup::firstThat(Boolean (*func)(TView *, void *), void *args)
{
	TView * p_term = P_Last;
	TView * p_temp = P_Last;
	if(p_temp) do {
		if(func(p_temp = p_temp->next, args) == True)
			return p_temp;
	} while(p_temp != p_term);
	return 0;
}
*/

void TGroup::forEach(void (*func)(TView*, void *), void *args)
{
	TView * p_term = P_Last;
	TView * p_temp = P_Last;
	if(p_temp) do {
		p_temp = p_temp->next;
		func(p_temp, args);
	} while(p_temp != p_term);
}

void TGroup::removeView(TView *p)
{
	if(P_Last) {
		TView * t = P_Last;
		do {
			if(t->next == p) {
				t->next = p->next;
				if(p != P_Last)
					return;
				P_Last = (p->next == p) ? 0 : t;
				break;
			}
			t = t->next;
		} while(t != P_Last);
	}
}

static void addSubviewDataSize(TView * p, void * pSize)
{
	*((int *)pSize) += ((TGroup *)p)->TransmitData(0, 0);
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

void TGroup::remove(TView * p)
{
	removeView(p);
	p->owner = 0;
	p->next = 0;
}

/* @v9.6.2
void TGroup::draw()
{
	redraw();
}
*/

/* @v9.0.4
void TGroup::endModal(ushort command)
{
	if(Sf & sfModal) {
		EndModalCmd = NZOR(command, cmCancel);
	}
	else {
		TView::endModal(command);
	}
}
*/

ushort TGroup::execView(TWindow * p)
{
	ushort retval = cmCancel;
	if(p) {
		const uint32 save_options = p->options;
		TGroup  * save_owner = p->owner;
		TWindow * save_top_view = APPL->P_TopView;
		TView   * save_current = P_Current;
		TCommandSet save_commands;
		getCommands(save_commands);
		APPL->P_TopView = p;
		p->options &= ~ofSelectable;
		p->setState(sfModal, true);
		setCurrent(p, enterSelect);
		if(save_owner == 0)
			Insert_(p);
		{
			// @v9.0.4 retval = p->execute();
			// @v9.0.4 {
			{
				TEvent event;
				event.what = evCommand;
				event.message.command = cmExecute;
				event.message.infoPtr = 0;
				p->handleEvent(event);
				retval = (event.what == evNothing) ? (ushort)event.message.infoLong : 0;
			}
			// } @v9.0.4
		}
		if(save_owner == 0)
			remove(p);
		setCurrent(save_current, leaveSelect);
		p->setState(sfModal, false);
		p->options = save_options;
		APPL->P_TopView = save_top_view;
		setCommands(save_commands);
	}
	return retval;
}

TView * TGroup::first() const
{
	return P_Last ? P_Last->next : 0;
}

struct handleStruct {
	TEvent * event;
	TGroup * grp;
	TView::phaseType phase;
};

static void doHandleEvent(TView * p, void *s)
{
	handleStruct * ptr = (handleStruct *)s;
	if(p && !(p->IsInState(sfDisabled) && (ptr->event->what & (positionalEvents|focusedEvents)))) {
		switch(ptr->phase) {
			case TView::phPreProcess:
				if(p->options & ofPreProcess)
					p->handleEvent(*ptr->event);
				break;
			case TView::phPostProcess:
				if(p->options & ofPostProcess)
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
		if(event.what != evNothing) {
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
	insertBefore(p, first());
}

void TGroup::insertBefore(TView * p, TView * Target)
{
	if(p && !p->owner && (!Target || Target->owner == this))
		insertView(p, Target);
}

void TGroup::insertView(TView * p, TView * Target)
{
	p->owner = this;
	if(Target) {
		Target = Target->prev();
		p->next = Target->next;
		Target->next = p;
	}
	else {
		if(P_Last == 0)
			p->next = p;
		else {
			p->next = P_Last->next;
			P_Last->next = p;
		}
		P_Last = p;
	}
}

void TGroup::redraw()
{
	for(TView * p = first(); p != 0; p = p->nextView())
		p->Draw_();
}

void TGroup::selectNext(/*Boolean forwards*/ /*false*/)
{
	if(P_Current) {
		TView * p = P_Current;
		do {
			p = p->prev();
		} while(!(p == P_Current || (p->IsInState(sfVisible) && !p->IsInState(sfDisabled) && p->options & ofSelectable)));
		p->select();
	}
}

void TGroup::setCurrent(TView * p, selectMode mode)
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
		if(p_temp)
			do {
				p_temp = p_temp->next;
				p_temp->setState(aState, enable);
			} while(p_temp != p_term);
	}
	if(aState & sfFocused && P_Current)
		P_Current->setState(sfFocused, enable);
}

/* @v9.4.8 static bool isInvalid(TView * p, void * cmd)
{
	return p->valid((ushort)cmd) ? false : true; // @valid
}*/

int FASTCALL TGroup::valid(ushort command)
{
	// @v9.0.1 ushort cmd = command;
	// @v9.0.1 return BIN(firstThat(isInvalid, (void *)cmd) == 0);
	// @v9.0.1 {
	//TView * TGroup::firstThat(Boolean (*func)(TView *, void *), void *args)
	{
		int    ok = 1;
		TView * p_term = P_Last;
		TView * p_temp = P_Last;
		if(p_temp) do {
			p_temp = p_temp->next;
			if(!p_temp->valid(command)) {
				ok = 0;
			}
		} while(ok && p_temp != p_term);
		return ok;
	}
	// } @v9.0.1
}

void TGroup::lock()
{
}

void TGroup::unlock()
{
}

uint TGroup::GetCurrId() const
{
    return P_Current ? P_Current->GetId() : 0;
}

int FASTCALL TGroup::IsCurrCtl(const TView * pV) const
{
	return BIN(pV && P_Current == pV);
}

int FASTCALL TGroup::isCurrCtlID(uint ctlID) const
{
	return (P_Current && P_Current->TestId(ctlID));
}

