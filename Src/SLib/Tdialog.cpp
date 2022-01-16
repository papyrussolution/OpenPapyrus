// TDIALOG.CPP  TurboVision 1.0
// Copyright (c) 1991 by Borland International
// Modified by A.Sobolev 1994, 1996-2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2010, 2011, 2013, 2015, 2016, 2017, 2018, 2019, 2020, 2021, 2022
// @codepage UTF-8
// Release for WIN32
//
#include <slib-internal.h>
#pragma hdrstop
#include <ppdefs.h> // @ Muxa При вызове findResource используется TAB_HELP, определенный в ppdefs.h

#define TV_DEBUG_STACK 0
//
CtrlGroup::CtrlGroup() : Id(0) {}
CtrlGroup::~CtrlGroup() {}
void CtrlGroup::handleEvent(TDialog*, TEvent&) {}
int CtrlGroup::setData(TDialog*, void *) { return -1; }
int CtrlGroup::getData(TDialog*, void *) { return -1; }
//
//
//
ODC OwnerDrawCtrls[32]; // @global
//
//
/*static*/void TDialog::centerDlg(HWND hWnd)
{
	const int sx = GetSystemMetrics(SM_CXSCREEN);
	const int sy = GetSystemMetrics(SM_CYSCREEN);
	RECT   r;
	GetWindowRect(hWnd, &r);
	r.right  -= r.left;
	r.bottom -= r.top;
	r.left = (sx-r.right)  / 2;
	r.top  = (sy-r.bottom) / 2;
	MoveWindow(hWnd, r.left, r.top, r.right, r.bottom, 1);
}

int TDialog::setupPosition()
{
	int    ok = -1;
	RECT   r;
	if(DlgFlags & fCascade) {
		RECT  pr;
		if(GetWindowRect(PrevInStack, &pr) && GetWindowRect(H(), &r))
			MoveWindow(H(), pr.left + 10, pr.top + 10, r.right - r.left, r.bottom - r.top, TRUE);
		else
			centerDlg(H());
		ok = 2;
	}
	else if(DlgFlags & fUserSettings) {
		if(GetWindowRect(H(), &r)) {
			int    x = Settings.Left;
			int    y = Settings.Top;
			int    sizex = r.right - r.left;
			int    sizey = r.bottom - r.top;
			const  int sx = GetSystemMetrics(SM_CXFULLSCREEN);
			const  int sy = GetSystemMetrics(SM_CYFULLSCREEN);
			SETMIN(x, sx-sizex);
			SETMAX(x, 0);
			SETMIN(y, sy - sizey);
			SETMAX(y, 0);
			MoveWindow(H(), x, y, sizex, sizey, TRUE);
			ok = 1;
		}
		else
			ok = 0;
	}
	else {
		SPoint2S next_dialog_lu_pos = SLS.GetTLA().GetNextDialogLuPos();
		if(next_dialog_lu_pos.x >= 0 && next_dialog_lu_pos.y >= 0) {
			RECT  pr;
			if(::GetWindowRect(PrevInStack, &pr) && ::GetWindowRect(H(), &r))
				::MoveWindow(H(), next_dialog_lu_pos.x, next_dialog_lu_pos.y, r.right - r.left, r.bottom - r.top, TRUE);
			ok = 2;
		}
		else if(DlgFlags & fCentered) {
			centerDlg(H());
			ok = 2;
		}
	}
	return 1; // @?
}

SString & TProgram::MakeModalStackDebugText(SString & rBuf) const
{
	rBuf.Z();
	SString temp_buf;
	for(uint i = 0; i < ModalStack.getPointer(); i++) {
		HWND _hs = *static_cast<HWND *>(ModalStack.at(i));
		if(i)
			rBuf.Space().Cat(">>").Space();
		if(::IsWindow(_hs))
			TView::SGetWindowText(_hs, temp_buf);
		else
			temp_buf = "non window";
		rBuf.CatHex((long)_hs).Space().CatBrackStr(temp_buf);
	}
	return rBuf;
}

int TProgram::TestWindowForEndModal(TWindow * pV)
{
	int    ok = 0;
	assert(pV);
	void * p_prev = ModalStack.SStack::peek();
	if(p_prev) {
		HWND   h_prev = *static_cast<HWND *>(p_prev);
		if(h_prev == pV->H()) {
			ok = 1;
		}
		else {
			SString msg_buf, temp_buf;
			MakeModalStackDebugText(temp_buf);
			(msg_buf = "Exit nontop window from modal mode (h_prev != pV->hWnd)").CatDiv(':', 2).Cat(temp_buf);
			SLS.LogMessage(0, msg_buf);
		}
	}
	else {
		SString msg_buf, temp_buf;
		MakeModalStackDebugText(temp_buf);
		(msg_buf = "Exit nontop window from modal mode (h_prev == 0)").CatDiv(':', 2).Cat(temp_buf);
		SLS.LogMessage(0, msg_buf);
	}
	return ok;
}

int TProgram::PushModalWindow(TWindow * pV, HWND h)
{
	int    ok = 0;
	assert(pV);
	if(::IsWindow(h)) {
#if TV_DEBUG_STACK
		void * p_prev = ModalStack.SStack::peek();
		if(p_prev) {
			HWND   h_prev = *static_cast<HWND *>(p_prev);
			// @debug {
			if(h_prev != H_TopOfStack) {
				SString msg_buf, temp_buf;
				MakeModalStackDebugText(temp_buf);
				(msg_buf = "Modal window stack order violation (h_prev != H_TopOfStack)").CatDiv(':', 2).Cat(temp_buf);
				SLS.LogMessage(0, msg_buf);
			}
			assert(h_prev == H_TopOfStack);
			// } @debug
		}
#endif
		pV->PrevInStack = H_TopOfStack;
		H_TopOfStack = h;
		ModalStack.push(h);
		ok = 1;
	}
	return ok;
}

int TProgram::PopModalWindow(TWindow * pV, HWND * pH)
{
	int    ok = 0;
	HWND   h = 0;
	assert(pV);
	if(ModalStack.pop(h)) {
#if TV_DEBUG_STACK
		// @debug {
		if(pV->hWnd != h) {
			SString msg_buf, temp_buf;
			MakeModalStackDebugText(temp_buf);
			(msg_buf = "Modal window stack order violation (pV->hWnd != h)").CatDiv(':', 2).Cat(temp_buf);
			SLS.LogMessage(0, msg_buf);
		}
		assert(pV->hWnd == h);
		if(H_TopOfStack != h) {
			SString msg_buf, temp_buf;
			MakeModalStackDebugText(temp_buf);
			(msg_buf = "Modal window stack order violation (H_TopOfStack != h)").CatDiv(':', 2).Cat(temp_buf);
			SLS.LogMessage(0, msg_buf);
		}
		assert(H_TopOfStack == h);
		// } @debug
#endif
		void * p_prev = ModalStack.SStack::peek();
		H_TopOfStack = pV->PrevInStack;
		EnableWindow(pV->PrevInStack, 1);
		if(p_prev) {
			HWND   h_prev = *static_cast<HWND *>(p_prev);
#if TV_DEBUG_STACK
			// @debug {
			if(h_prev != pV->PrevInStack) {
				SString msg_buf, temp_buf;
				MakeModalStackDebugText(temp_buf);
				(msg_buf = "Modal window stack order violation (h_prev != pV->PrevInStack)").CatDiv(':', 2).Cat(temp_buf);
				SLS.LogMessage(0, msg_buf);
			}
			assert(h_prev == pV->PrevInStack);
			// } @debug
#endif
			SetForegroundWindow(h_prev);
			SetActiveWindow(h_prev);
		}
		ShowWindow(h, SW_HIDE);
	}
	ASSIGN_PTR(pH, h);
	return ok;
}
//
// Load Dialog
//
static void loadLocalMenu(TVRez & rez, TDialog * dlg)
{
	/*
	char buf[128];
	int  button_count = 0;
	LocalMenuItem * m = new LocalMenuItem;
	while(rez.getUINT() != TV_END) {
		m = (LocalMenuItem*)SAlloc::R(m, (1+button_count)*sizeof(LocalMenuItem));
		fseek(rez.getStream(), -((long)sizeof(uint16)), SEEK_CUR);
		m[button_count].KeyCode = rez.getUINT();
		rez.getString(buf);
		OemToChar(buf, m[button_count++].Name);
	}
	dlg->setupLocalMenu(m, button_count);
	*/
}

int (* getUserControl)(TVRez*, TDialog*) = 0;

/*static*/int TDialog::LoadDialog(TVRez * rez, uint dialogID, TDialog * dlg, long flags)
{
	assert(dlg != 0);
	if(rez == 0)
		return 0;
	uint   tag, cmd;
	long   format;
	TYPEID type;
	uint   id, calc_inputline_id, calc_button_id, help_ctx, options, temp_id;
	TRect  r;
	TView * p_view = 0, * p_ctl = 0;
	SString temp_buf, symb, cmd_symb;
	char   buf[256];
	long   sz = 0L;
	long   ofs = 0L;
	StrAssocArray symb_list;
	if(rez->findResource(dialogID, TV_DIALOG, &ofs, &sz)) {
		r = rez->getRect();
		rez->getString(buf);
		rez->getString(symb, 0);
		if(flags & ldfDL600_Cvt && symb.NotEmptyS()) {
			// @v11.2.0 SETIFZ(dlg->P_SymbList, new StrAssocArray);
			// @v11.2.0 CALLPTRMEMB(dlg->P_SymbList, Add(-1000, symb));
			dlg->SetCtlSymb(-1000, symb); // @v11.2.0 
		}
		dlg->changeBounds(r);
		strip(buf);
		if(!(flags & ldfDL600_Cvt) && buf[0] == '@' && SLS.LoadString_(buf+1, temp_buf) > 0)
			dlg->setTitle(temp_buf);
		else
			dlg->setTitle(buf);
		dlg->Id = dialogID;
		while(rez->getStreamPos() < (ofs + sz)) {
			tag = rez->getUINT();
			switch(tag) {
				case TV_INPUTLINE:
					r = rez->getRect();
					id = rez->getUINT();
					rez->getString(symb, 0);
					type = rez->getType(0);
					format = rez->getFormat(0);
					calc_inputline_id = rez->getUINT();
					calc_button_id = rez->getUINT();
					help_ctx = rez->getUINT();
					if(calc_inputline_id)
						p_ctl = new TCalcInputLine(calc_inputline_id, calc_button_id, r, type, format);
					else
						p_ctl = new TInputLine(r, type, format);
					dlg->InsertCtl(p_ctl, id, (flags & ldfDL600_Cvt) ? symb.cptr() : 0);
					break;
				case TV_BUTTON:
					{
						r  = rez->getRect();
						id = rez->getUINT();
						rez->getString(symb, 0);
						rez->getString(buf);
						cmd = rez->getUINT();
						rez->getString(cmd_symb, 0);
						options = rez->getUINT();
						help_ctx = rez->getUINT();
						uint bmp_id = rez->getUINT();
						dlg->InsertCtl(new TButton(r, buf, cmd, options, bmp_id), id, (flags & ldfDL600_Cvt) ? symb.cptr() : 0);
						if(flags & ldfDL600_Cvt && cmd_symb.NotEmptyS())
							dlg->SetCtlSymb(cmd+100000, cmd_symb);
					}
					break;
				case TV_CHECKBOXES:
				case TV_RADIOBUTTONS:
					{
						r  = rez->getRect();
						id = rez->getUINT();
						rez->getString(symb, 0);
						help_ctx = rez->getUINT();
						TCluster * p_cluster = new TCluster(r, (tag == TV_CHECKBOXES) ? CHECKBOXES : RADIOBUTTONS, 0);
						while(rez->getUINT() != TV_END) {
							assert(p_cluster->getNumItems() < 36);
							fseek(rez->getStream(), -((long)sizeof(uint16)), SEEK_CUR);
							p_cluster->addItem(-1, rez->getString(buf));
						}
						dlg->InsertCtl(p_cluster, id, (flags & ldfDL600_Cvt) ? symb.cptr() : 0);
					}
					break;
				case TV_LISTBOX:
					{
						SString columns_buf;
						r  = rez->getRect();
						id = rez->getUINT();
						rez->getString(symb, 0);
						rez->getUINT();    // options
						rez->getType(0);   // type
						rez->getFormat(0); // format
						help_ctx = rez->getUINT();
						rez->getString(columns_buf, 0);
						{
							const int is_tree_list = columns_buf.IsEqiAscii("TREELISTVIEW");
							SmartListBox * p_lb = new SmartListBox(r, 0, is_tree_list);
							if(p_lb) {
								if(!is_tree_list)
									p_lb->SetupColumns(columns_buf);
								dlg->InsertCtl(p_lb, id, (flags & ldfDL600_Cvt) ? symb.cptr() : 0);
							}
						}
					}
					break;
				case TV_INFOPANE:
					r = rez->getRect();
					id = rez->getUINT();
					rez->getString(symb, 0);
					//dlg->InsertCtl(new TInfoPane(r), id, 0);
					break;
				case TV_STATIC:
					{
						char columns_buf[256];
						memzero(columns_buf, sizeof(columns_buf));
						r = rez->getRect();
						id = rez->getUINT();
						rez->getString(symb, 0);
						rez->getString(buf);
                		rez->getString(columns_buf);
						rez->getString(temp_buf.Z(), 0); // image_symbol
						if(sstreqi_ascii(columns_buf, "IMAGEVIEW"))
							p_ctl = new TImageView(r, temp_buf);
						else
							p_ctl = new TStaticText(r, buf);
					}
					dlg->InsertCtl(p_ctl, id, (flags & ldfDL600_Cvt) ? symb.cptr() : 0);
					break;
				case TV_LABEL:
					r = rez->getRect();
					id = rez->getUINT();
					rez->getString(symb, 0);
					rez->getString(buf);
					p_view = dlg->getCtrlView(rez->getUINT());
					if(p_view)
						dlg->InsertCtl(new TLabel(r, buf, p_view), id, (flags & ldfDL600_Cvt) ? symb.cptr() : 0);
					break;
				case TV_COMBO:
					r  = rez->getRect();
					id = rez->getUINT();
					rez->getString(symb, 0);
					options  = rez->getUINT();
					temp_id  = rez->getUINT();
					help_ctx = rez->getUINT();
					rez->getUINT(); // bmp_id == 0
					p_view = dlg->getCtrlView(temp_id);
					if(temp_id) {
						ComboBox * combo = new ComboBox(r,  options, static_cast<TInputLine *>(p_view));
						dlg->InsertCtl(combo, id, (flags & ldfDL600_Cvt) ? symb.cptr() : 0);
					}
					break;
				case TV_LOCALMENU:
					loadLocalMenu(*rez, dlg);
					break;
				default:
					if(getUserControl == 0 || getUserControl(rez, dlg) == 0)
						return 0;
			}
		}
	}
	else
		return 0;
	dlg->selectNext();
	dlg->Id = dialogID;
	//
	// @ Muxa {
	// Получение идентификатора топика контекстной справки для данного диалога
	//
	uint   num_flds = 0;
	long   res_size = 0;
	if(rez->findResource(TAB_HELP, PP_RCDATA, NULL, &res_size) > 0) {
		num_flds = res_size / (sizeof(short) * 2);
		for(uint i = 0; i < num_flds; i++) {
			if(dlg->Id == rez->getUINT()) {
				dlg->HelpCtx = rez->getUINT();
				break;
			}
			rez->getUINT();
		}
	}
	else
		dlg->HelpCtx = 0;
	// } @ Muxa
	return 1;
}

/*static*/int TDialog::GetSymbolBody(const char * pSymb, SString & rBodyBuf)
{
	int    ok = 1;
	SString symb(pSymb);
	rBodyBuf.Z();
	if(symb.HasPrefixIAscii("DLGW_"))
		symb.Sub(5, symb.Len()-5, rBodyBuf);
	else if(symb.HasPrefixIAscii("DLG_"))
		symb.Sub(4, symb.Len()-4, rBodyBuf);
	else {
		rBodyBuf = symb;
		ok = 2;
	}
	return ok;
}

void TDialog::Helper_Constructor(uint resID, DialogPreProcFunc dlgPreFunc, void * extraPtr, ConstructorOption co)
{
	SubSign = TV_SUBSIGN_DIALOG;
	P_PrevData = 0;
	GrpCount = 0;
	PP_Groups = 0;
	DlgFlags = fCentered;
	SETFLAG(DlgFlags, fLarge, SLS.CheckUiFlag(sluifUseLargeDialogs));
	resourceID = resID;
	P_Frame  = 0;
	HW = 0;
	ToolTipsWnd = 0;
	MEMSZERO(ResizedRect);
	MEMSZERO(ToResizeRect);
	DefInputLine  = 0;
	if(resID) {
		if(co == coExport)
			DlgFlags |= fExport;
		TDialog::LoadDialog(P_SlRez, resID, this, (co == coExport) ? ldfDL600_Cvt : 0);
		if(dlgPreFunc)
			dlgPreFunc(this, extraPtr);
		//
		// @v4.2.5
		// Операция по сохранению текущего окна необходима из-за того, что при создании
		// диалога указатель APPL->P_DeskTop->P_Current обнуляется. Это, как правило не страшно,
		// но делает неработоспособными некоторые функции (например, не срабатывает функция //
		// TView::messageCommand(APPL->P_DeskTop, cmGetFocusedNumber, &c); в калькуляторе).
		//
		TView * preserve_current = APPL->P_DeskTop->GetCurrentView();
		HW = APPL->CreateDlg(resourceID, APPL->H_TopOfStack, TDialog::DialogProc, reinterpret_cast<LPARAM>(this));
		::ShowWindow(H(), SW_HIDE);
		APPL->P_DeskTop->SetCurrentView(preserve_current, leaveSelect);
	}
}

TDialog::TDialog(const TRect & bounds, const char *aTitle) : TWindow(bounds, aTitle, wnNoNumber)
	{ Helper_Constructor(0, 0, 0, coNothing); }
TDialog::TDialog(uint resID, DialogPreProcFunc dlgPreFunc, void * extraPtr) : TWindow(TRect(), 0, wnNoNumber)
	{ Helper_Constructor(resID, dlgPreFunc, extraPtr, coNothing); }
TDialog::TDialog(uint resID) : TWindow(TRect(), 0, wnNoNumber) { Helper_Constructor(resID, 0, 0, coNothing); }
TDialog::TDialog(uint resID, ConstructorOption co) : TWindow(TRect(), 0, wnNoNumber) { Helper_Constructor(resID, 0, 0, co); }
void TDialog::ToCascade() { DlgFlags |= fCascade; }
bool FASTCALL TDialog::CheckFlag(long f) const { return LOGIC(DlgFlags & f); }

TDialog::~TDialog()
{
	::DestroyWindow(ToolTipsWnd);
	::DestroyWindow(H());
	HW = 0;
	delete P_PrevData;
	if(PP_Groups) {
		for(uint i = 0; i < GrpCount; i++)
			if(PP_Groups[i] && PP_Groups[i]->Id > 0)
				delete PP_Groups[i];
		SAlloc::F(PP_Groups);
	}
}

int FASTCALL TDialog::addGroup(ushort grpID, CtrlGroup * pGroup)
{
	int    ok = 0;
	int    old_grp_idx = -1;
	if(PP_Groups) {
		for(uint i = 0; old_grp_idx < 0 && i < GrpCount; i++)
			if(PP_Groups[i] && PP_Groups[i]->Id == grpID) {
				ZDELETE(PP_Groups[i]);
				old_grp_idx = i;
			}
	}
	if(old_grp_idx >= 0) {
		if(pGroup)
			pGroup->Id = grpID;
		PP_Groups[old_grp_idx] = pGroup;
		ok = 1;
	}
	else if(pGroup) {
		PP_Groups = static_cast<CtrlGroup **>(SAlloc::R(PP_Groups, sizeof(CtrlGroup*) * (GrpCount+1)));
		pGroup->Id = grpID;
		ok = PP_Groups ? (PP_Groups[GrpCount++] = pGroup, 1) : (GrpCount = 0, 0);
	}
	if(ok && pGroup) {
		TEvent event;
		pGroup->handleEvent(this, event.setCmd(cmGroupInserted, this));
	}
	return ok;
}

int FASTCALL TDialog::setGroupData(ushort grpID, void * aData)
{
	CtrlGroup * p_grp = getGroup(grpID);
	return p_grp ? p_grp->setData(this, aData) : 0;
}

int FASTCALL TDialog::getGroupData(ushort grpID, void * aData)
{
	CtrlGroup * p_grp = getGroup(grpID);
	return p_grp ? p_grp->getData(this, aData) : 0;
}

CtrlGroup * FASTCALL TDialog::getGroup(ushort grpID)
{
	const uint _c = PP_Groups ? GrpCount : 0;
	for(uint i = 0; i < _c; i++) {
		CtrlGroup * p_grp = PP_Groups[i];
		if(p_grp && p_grp->Id == grpID)
			return p_grp;
	}
	return 0;
}

IMPL_HANDLE_EVENT(TDialog)
{
	if(event.isCmd(cmExecute)) {
		ushort retval = 0;
		if(H()) {
			bool is_list_win = false;
			TView::SetWindowProp(H(), GWLP_USERDATA, this);
			if(APPL->PushModalWindow(this, H())) {
				setupPosition();
				InitRect = getRect();
				TView::messageCommand(this, cmSetupResizeParams);
				APPL->SetWindowViewByKind(H(), TProgram::wndtypDialog);
				::ShowWindow(H(), SW_SHOW);
				::SetForegroundWindow(H());
				::SetActiveWindow(H());
				::EnableWindow(PrevInStack, 0);
				if(P_Next && P_Next->IsConsistent() && P_Next->IsSubSign(TV_SUBSIGN_DIALOG) && static_cast<TDialog *>(P_Next)->resourceID == -1) {
					is_list_win = true;
					::EnableWindow(GetParent(PrevInStack), 0);
				}
				if(HW) { // @v10.8.1
					TEvent event;
					this->handleEvent(event.setCmd(cmModalPostCreate, this)); // @recursion
				}
				MSG msg;
				do {
					GetMessage(&msg, 0, 0, 0);
					if(!TranslateAccelerator(msg.hwnd, APPL->H_Accel, &msg)) {
						if(!IsDialogMessage(H(), &msg)) {
							::TranslateMessage(&msg);
							DispatchMessage(&msg);
						}
					}
					if(EndModalCmd) {
						if(APPL->TestWindowForEndModal(this) && valid(EndModalCmd))
							break;
						else
							EndModalCmd = 0;
					}
				} while(1);
				retval = EndModalCmd;
				EndModalCmd = 0;
				APPL->PopModalWindow(this, 0);
				if(is_list_win) {
					::EnableWindow(GetParent(PrevInStack), 1);
					::SetFocus(::GetDlgItem(PrevInStack, CTL_LBX_LIST));
				}
			}
		}
		else {
			SString errmsg_en("Error creating dialog box.");
			::MessageBox(APPL->H_MainWnd, SUcSwitch(errmsg_en), _T("PROJECT PAPYRUS"), MB_OK);
		}
		clearEvent(event);
		event.message.infoLong = retval;
	}
	else {
		TWindow::handleEvent(event);
		switch(event.what) {
			case TEvent::evCommand:
				switch(event.message.command) {
					case cmOK:
						// @v10.7.7 {
						{
							TView * p_temp = P_Last;
							if(p_temp) {
								const TView * p_term = P_Last;
								do {
									p_temp = p_temp->P_Next;
									if(p_temp && p_temp->IsConsistent())
										TView::messageCommand(p_temp, cmNotifyCommit, this);
									else
										break;
								} while(p_temp != p_term);
							}
						}
						// @fallthrough
						// } @v10.7.7 
					case cmCancel:
					case cmYes:
					case cmNo:
					case cmaAll:
						if(IsInState(sfModal)) {
							EndModalCmd = event.message.command;
							clearEvent(event);
						}
						else if(event.message.command == cmCancel) {
							close();
							return; // Окно разрушено - делать в этой процедуре больше нечего!
						}
						break;
					case cmaCalculate:
						{
							SlExtraProcBlock epb;
							SLS.GetExtraProcBlock(&epb);
							if(epb.F_CallCalc)
								epb.F_CallCalc(H(), 0);
						}
						clearEvent(event);
						break;
					/* @v6.6.2 case cmGetHelpContext:
						{
							uint * p = (uint *)event.message.infoPtr;
							ASSIGN_PTR(p, (Id+2000)); //_DLG_OFFSET   ppdefs.h
							clearEvent(event);
						}
						break;*/
					case cmResize:
						if(event.message.infoPtr) {
							if(!(DlgFlags & fMouseResizing))
								GetWindowRect(H(), &ResizedRect);
							ToResizeRect = *static_cast<RECT *>(event.message.infoPtr);
							DlgFlags |= fMouseResizing;
							clearEvent(event);
						}
						else if(DlgFlags & fMouseResizing) {
							// Если изменяется левая и/или верхняя координата, надо скорректировать ResizedRect
							ResizedRect.right  += ToResizeRect.left - ResizedRect.left;
							ResizedRect.left    = ToResizeRect.left;
							ResizedRect.bottom += ToResizeRect.top - ResizedRect.top;
							ResizedRect.top     = ToResizeRect.top;
							ResizeDlgToRect(&ToResizeRect);
							MEMSZERO(ResizedRect);
							MEMSZERO(ToResizeRect);
							DlgFlags &= ~fMouseResizing;
							clearEvent(event);
						}
						// @v9.6.6 {
						/*
						else {
							GetWindowRect(H(), &ToResizeRect);
							ResizeDlgToRect(&ToResizeRect);
							MEMSZERO(ResizedRect);
							MEMSZERO(ToResizeRect);
							DlgFlags &= ~fMouseResizing;
							clearEvent(event);
						}
						*/
						// } @v9.6.6 
						break;
				}
				break;
		}
		for(uint i = 0; event.what && i < GrpCount; i++) {
			CALLPTRMEMB(PP_Groups[i], handleEvent(this, event));
		}
	}
}

int FASTCALL TDialog::valid(ushort command)
{
	if(command == cmCancel)
		return 1;
	else if(command == cmValid) {
		if((ViewSize.x | ViewSize.y) == 0)
			return 0;
	}
	return TGroup::valid(command);
}

int TDialog::TransmitData(int dir, void * pData)
{
	int    s = TWindow::TransmitData(0, 0);
	if(dir > 0) {
		TWindow::TransmitData(dir, pData);
		delete P_PrevData;
		if((P_PrevData = new char[s]) !=0)
			memcpy(P_PrevData, pData, s);
	}
	else if(dir < 0) {
		TWindow::TransmitData(dir, pData);
		if(P_PrevData && memcmp(P_PrevData, pData, s))
			DlgFlags |= fModified;
	}
	return s;
}

long TDialog::getVirtButtonID(uint ctlID)
{
	TView * v = getCtrlView(ctlID);
	return v ? reinterpret_cast<long>(TView::messageBroadcast(this, cmSearchVirtButton, v)) : 0;
}

int FASTCALL TDialog::AddClusterAssoc(uint ctlID, long pos, long val)
{
	TCluster * p_clu = static_cast<TCluster *>(getCtrlView(ctlID));
	return p_clu ? p_clu->addAssoc(pos, val) : 0;
}

int FASTCALL TDialog::AddClusterAssocDef(uint ctlID, long pos, long val)
{
	int    ok = 1;
	TCluster * p_clu = static_cast<TCluster *>(getCtrlView(ctlID));
	if(p_clu) {
		p_clu->addAssoc(pos, val);
		if(pos >= 0 && p_clu->getKind() == RADIOBUTTONS)
			p_clu->addAssoc(-1, val);
	}
	else
		ok = 0;
	return ok;
}

int TDialog::GetClusterItemByAssoc(uint ctlID, long val, int * pPos)
{
	TCluster * p_clu = static_cast<TCluster *>(getCtrlView(ctlID));
	return p_clu ? p_clu->getItemByAssoc(val, pPos) : 0;
}

int FASTCALL TDialog::SetClusterData(uint ctlID, long val)
{
	TCluster * p_clu = static_cast<TCluster *>(getCtrlView(ctlID));
	return p_clu ? p_clu->setDataAssoc(val) : 0;
}

int FASTCALL TDialog::GetClusterData(uint ctlID, long * pVal)
{
	TCluster * p_clu = static_cast<TCluster *>(getCtrlView(ctlID));
	return p_clu ? p_clu->getDataAssoc(pVal) : 0;
}

long FASTCALL TDialog::GetClusterData(uint ctlID)
{
	long   temp_long = 0;
	return GetClusterData(ctlID, &temp_long) ? temp_long : 0;
}

int FASTCALL TDialog::GetClusterData(uint ctlID, int16 * pVal)
{
	long   temp_long = *pVal;
	int    r = GetClusterData(ctlID, &temp_long);
	if(r)
		*pVal = static_cast<int16>(temp_long);
	return r;
}

void TDialog::DisableClusterItem(uint ctlID, int itemNo, int toDisable)
{
	TCluster * p_clu = static_cast<TCluster *>(getCtrlView(ctlID));
	CALLPTRMEMB(p_clu, disableItem(itemNo, toDisable));
}

int TDialog::SetClusterItemText(uint ctlID, int itemNo /* 0.. */, const char * pText)
{
	TCluster * p_clu = static_cast<TCluster *>(getCtrlView(ctlID));
	return p_clu ? p_clu->setText(itemNo, pText) : 0;
}

int TDialog::SetDefaultButton(uint ctlID, int setDefault)
{
	TButton * p_ctl = static_cast<TButton *>(getCtrlView(ctlID));
	return p_ctl ? p_ctl->makeDefault(LOGIC(setDefault), 1) : 0;
}

int TDialog::SetCtrlBitmap(uint ctlID, uint bmID)
{
	int    ok = 0;
	HBITMAP h_bm = APPL->FetchBitmap(bmID);
	if(h_bm) {
		::SendDlgItemMessage(H(), ctlID, STM_SETIMAGE, IMAGE_BITMAP, reinterpret_cast<LPARAM>(h_bm));
		ok = 1;
	}
	return ok;
}

int TDialog::SetupInputLine(uint ctlID, TYPEID typ, long format)
{
	int    ok = 0;
	TInputLine * p_v = static_cast<TInputLine *>(getCtrlView(ctlID));
	if(p_v && p_v->IsSubSign(TV_SUBSIGN_INPUTLINE)) {
		p_v->setFormat(format);
		p_v->setType(typ);
		ok = 1;
	}
	return ok;
}

void TDialog::SetupSpin(uint ctlID, uint buddyCtlID, int low, int upp, int cur)
{
	::SendDlgItemMessage(H(), ctlID, UDM_SETBUDDY, reinterpret_cast<WPARAM>(GetDlgItem(H(), buddyCtlID)), 0);
	::SendDlgItemMessage(H(), ctlID, UDM_SETRANGE, 0, MAKELONG(upp, low));
	::SendDlgItemMessage(H(), ctlID, UDM_SETPOS, 0, MAKELONG(cur, 0));
}
//
//
//
int TDialog::SetCtrlToolTip(uint ctrlID, const char * pToolTipText)
{
    int   ok = -1;
    HWND  ctrl_wnd = GetDlgItem(H(), ctrlID);
	if(ctrl_wnd) {
		if(!ToolTipsWnd) {
			ToolTipsWnd = ::CreateWindowEx(WS_EX_TOPMOST, TOOLTIPS_CLASS, NULL, WS_POPUP|TTS_NOPREFIX|TTS_ALWAYSTIP,
				CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, ctrl_wnd, NULL, TProgram::GetInst(), 0);
			SetWindowPos(ToolTipsWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
		}
		RECT  ctrl_rect;
		GetWindowRect(ctrl_wnd, &ctrl_rect);
		TOOLINFO ti;
		INITWINAPISTRUCT(ti);
		ti.uFlags = TTF_SUBCLASS | TTF_IDISHWND;
		ti.hwnd   = ctrl_wnd;
		ti.hinst  = TProgram::GetInst();
		ti.uId    = reinterpret_cast<UINT_PTR>(ctrl_wnd);
		ti.lpszText = const_cast<TCHAR *>(SUcSwitch(pToolTipText));
		ti.rect     = ctrl_rect;
		SendMessage(ToolTipsWnd, TTM_ADDTOOL, 0, reinterpret_cast<LPARAM>(&ti));
		ok = 1;
	}
	return ok;
}

static const char * WrSubKey_DlgUserSetting = "Software\\Papyrus\\Dialog";

int TDialog::SaveUserSettings()
{
	char   param[32];
	RECT   rect;
	WinRegKey reg_key(HKEY_CURRENT_USER, WrSubKey_DlgUserSetting, 0);
	ltoa(resourceID, param, 10);
	// version, left, top
	char   ver[32], temp_buf[32];
	uint   pos = 0;
	int    num_cols = 0;
	StringSet ss(',', 0);
	ver[0] = '0';
	ver[1] = 0;
	ss.add(ver, &pos);
	GetWindowRect(H(), &rect);
	Settings.Left = rect.left;
	Settings.Top  = rect.top;
	DlgFlags |= fUserSettings;
	itoa(rect.left, temp_buf, 10);
	ss.add(temp_buf, &pos);
	itoa(rect.top, temp_buf, 10);
	ss.add(temp_buf, &pos);
	return reg_key.PutString(param, ss.getBuf());
}

int TDialog::RestoreUserSettings()
{
	int    ok = -1;
	SString spec_buf;
	char   param[32];
	WinRegKey reg_key(HKEY_CURRENT_USER, WrSubKey_DlgUserSetting, 1);
	ltoa(resourceID, param, 10);
	if(reg_key.GetString(param, spec_buf)) {
		char   ver[32], temp_buf[32];
		uint   pos = 0;
		uint   num_cols = 0;
		StringSet ss(',', spec_buf);
		ss.get(&pos, ver, sizeof(ver));
		Settings.Ver = satoi(ver);
		ss.get(&pos, temp_buf, sizeof(temp_buf));
		Settings.Left = satoi(temp_buf);
		ss.get(&pos, temp_buf, sizeof(temp_buf));
		Settings.Top = satoi(temp_buf);
		DlgFlags |= fUserSettings;
		ok = 1;
	}
	return ok;
}
//
// Изменение размеров окна диалога
//
int TDialog::SetCtrlResizeParam(long ctrlID, long lCtrl, long tCtrl, long rCtrl, long bCtrl, long ctrlResizeFlags)
{
	int    ok = 1;
	int    undef_ctrls = 0;
	if(lCtrl < 0) {
		undef_ctrls++;
		if(rCtrl < 0 || ctrlResizeFlags & crfLinkLeft)
			ok = -1;
	}
	if(ok > 0 && tCtrl < 0) {
		undef_ctrls++;
		if(bCtrl < 0 || ctrlResizeFlags & crfLinkTop)
			ok = -1;
	}
	if(ok > 0 && rCtrl < 0) {
		undef_ctrls++;
		if(ctrlResizeFlags & crfLinkRight)
			ok = -1;
	}
	if(ok > 0 && bCtrl < 0) {
		undef_ctrls++;
		if(ctrlResizeFlags & crfLinkBottom)
			ok = -1;
	}
	if(ok > 0 && (ctrlResizeFlags & crfResizeable) && undef_ctrls > 1)
		ok = -1;
	if(ok > 0) {
		ResizeParamEntry  rpe;
		rpe.CtrlID  = ctrlID;
		rpe.CtrlWnd = 0;
		rpe.Left    = lCtrl;
		rpe.Top     = tCtrl;
		rpe.Right   = rCtrl;
		rpe.Bottom  = bCtrl;
		rpe.Flags   = ctrlResizeFlags;
		ok = ResizeParamAry.insert(&rpe);
	}
	return ok;
}

int __cdecl TDialog::LinkCtrlsToDlgBorders(long ctrlResizeFlags, ...)
{
	int   ok = 1;
	int   left    = (ctrlResizeFlags & crfLinkLeft)  ? 0 : -1;
	int   top     = (ctrlResizeFlags & crfLinkTop)   ? 0 : -1;
	int   right   = (ctrlResizeFlags & crfLinkRight) ? 0 : -1;
	int   bottom  = (ctrlResizeFlags & crfLinkBottom)? 0 : -1;
	long  ctrl_id;
	va_list  vl;
	va_start(vl, ctrlResizeFlags);
	while(ok > 0 && ((ctrl_id = va_arg(vl, long)) != 0))
		ok = SetCtrlResizeParam(ctrl_id, left, top, right, bottom, ctrlResizeFlags);
	va_end(vl);
	return ok;
}

void TDialog::RecalcCtrlCoords(long firstCoord, long secondCoord, long * pFirstCtrlCoord, long * pSecondCtrlCoord, long ctrlSize, int recalcParam)
{
	if(pFirstCtrlCoord && pSecondCtrlCoord) {
		if(recalcParam == 1) {
			*pFirstCtrlCoord  = firstCoord;
			*pSecondCtrlCoord = firstCoord + ctrlSize;
		}
		else if(recalcParam == 2) {
			*pFirstCtrlCoord  = secondCoord - ctrlSize;
			*pSecondCtrlCoord = secondCoord;
		}
		else if(recalcParam == 3) {
			// @v10.9.0 double mult = ((double)(secondCoord - firstCoord - ctrlSize)) / (*pFirstCtrlCoord + *pSecondCtrlCoord);
			const double mult = fdivi(secondCoord - firstCoord - ctrlSize, *pFirstCtrlCoord + *pSecondCtrlCoord); // @v10.9.0 
			*pFirstCtrlCoord  = firstCoord + R0i(*pFirstCtrlCoord * mult);
			*pSecondCtrlCoord = *pFirstCtrlCoord + ctrlSize;
		}
		else if(recalcParam == 4) {
			*pFirstCtrlCoord  = firstCoord;
			*pSecondCtrlCoord = secondCoord;
		}
	}
}

int TDialog::Helper_ToRecalcCtrlSet(const RECT * pNewDlgRect, const ResizeParamEntry & rCtrlParam, TSVector <ResizeParamEntry> * pCoordAry, LongArray * pCalcedCtrlAry, int isXDim)
{
	int    ok = 1;
	int    is_found;
	uint   p;
	double mult;
	HWND   ctrl_wnd;
	RECT   ctrl_rect, linked_rect;
	ResizeParamEntry * p_coord = 0;
	ResizeParamEntry new_coord;
	ResizeParamEntry linked_coord;
	LongArray ctrl_set_ary;
	ResizeParamEntry rpe = rCtrlParam;
	long    old_f_bound, old_s_bound, new_f_bound, new_s_bound;
	long    srch_flg_f = isXDim ? crfLinkLeft  : crfLinkTop;
	long    srch_flg_s = isXDim ? crfLinkRight : crfLinkBottom;
	long  * p_param_f      = isXDim ? &rpe.Left           : &rpe.Top;
	long  * p_param_s      = isXDim ? &rpe.Right          : &rpe.Bottom;
	long  * p_old_dlg_f    = isXDim ? &ResizedRect.left   : &ResizedRect.top;
	long  * p_old_dlg_s    = isXDim ? &ResizedRect.right  : &ResizedRect.bottom;
	const   long  * p_new_dlg_f    = isXDim ? &pNewDlgRect->left  : &pNewDlgRect->top;
	const   long  * p_new_dlg_s    = isXDim ? &pNewDlgRect->right : &pNewDlgRect->bottom;
	long  * p_old_rect_f   = isXDim ? &ctrl_rect.left     : &ctrl_rect.top;
	long  * p_old_rect_s   = isXDim ? &ctrl_rect.right    : &ctrl_rect.bottom;
	long  * p_old_linked_f = isXDim ? &linked_rect.left   : &linked_rect.top;
	long  * p_old_linked_s = isXDim ? &linked_rect.right  : &linked_rect.bottom;
	long  * p_new_linked_f = isXDim ? &linked_coord.Left  : &linked_coord.Top;
	long  * p_new_linked_s = isXDim ? &linked_coord.Right : &linked_coord.Bottom;
	long    size_to_resize = 0;
	ok  =  ctrl_set_ary.add(rpe.CtrlID);
	for(is_found = 0; ok > 0 && !is_found;) {
		if(rpe.Flags & crfResizeable) {
			ctrl_wnd = GetDlgItem(H(), rpe.CtrlID);
			if(SETIFZ(ctrl_wnd, GetDlgItem(H(), MAKE_BUTTON_ID(rpe.CtrlID, 1)))) {
				::GetWindowRect(ctrl_wnd, &ctrl_rect);
				size_to_resize += *p_old_rect_s - *p_old_rect_f;
			}
		}
		if(*p_param_f) {
			if(!(rpe.Flags & srch_flg_f)) {
				if(ResizeParamAry.lsearch(p_param_f, &(p = 0), CMPF_LONG)) {
					rpe = ResizeParamAry.at(p);
					ok = ctrl_set_ary.atInsert(0, &rpe.CtrlID);
				}
				else
					ok = -1;
			}
			else if(pCalcedCtrlAry->lsearch(*p_param_f) && pCoordAry->lsearch(p_param_f, &(p = 0), CMPF_LONG)) {
				linked_coord = pCoordAry->at(p);
				if(linked_coord.CtrlWnd) {
					::GetWindowRect(linked_coord.CtrlWnd, &linked_rect);
					old_f_bound = *p_old_linked_f;
					new_f_bound = *p_new_linked_f;
					is_found = 1;
				}
				else
					ok = -1;
			}
			else
				ok = -1;
		}
		else {
			old_f_bound = *p_old_dlg_f;
			new_f_bound = *p_new_dlg_f;
			is_found = 1;
		}
	}
	if(is_found) {
		const long first_ctrl_id = *p_param_f;
		rpe = rCtrlParam;
		rpe.Flags &= ~crfResizeable;
		for(is_found = 0; ok > 0 && !is_found;) {
			if(rpe.Flags & crfResizeable) {
				ctrl_wnd = GetDlgItem(H(), rpe.CtrlID);
				if(SETIFZ(ctrl_wnd, GetDlgItem(H(), MAKE_BUTTON_ID(rpe.CtrlID, 1)))) {
					::GetWindowRect(ctrl_wnd, &ctrl_rect);
					size_to_resize += *p_old_rect_s - *p_old_rect_f;
				}
			}
			if(*p_param_s) {
				if(!(rpe.Flags & srch_flg_s)) {
					if(ResizeParamAry.lsearch(p_param_s, &(p = 0), CMPF_LONG)) {
						rpe = ResizeParamAry.at(p);
						ok = ctrl_set_ary.add(rpe.CtrlID);
					}
					else
						ok = -1;
				}
				else if(pCalcedCtrlAry->lsearch(*p_param_s) && pCoordAry->lsearch(p_param_s, &(p = 0), CMPF_LONG)) {
					linked_coord = pCoordAry->at(p);
					if(linked_coord.CtrlWnd) {
						::GetWindowRect(linked_coord.CtrlWnd, &linked_rect);
						old_s_bound = *p_old_linked_s;
						new_s_bound = *p_new_linked_s;
						is_found = 1;
					}
					else
						ok = -1;
				}
				else
					ok = -1;
			}
			else {
				old_s_bound = *p_old_dlg_s;
				new_s_bound = *p_new_dlg_s;
				is_found = 1;
			}
		}
		if(is_found) {
			// @v10.9.0 mult = (double)(new_s_bound - new_f_bound - (old_s_bound - old_f_bound - size_to_resize)) / size_to_resize;
			mult = fdivi(new_s_bound - new_f_bound - (old_s_bound - old_f_bound - size_to_resize), size_to_resize); // @v10.9.0
			if(first_ctrl_id) {
				pCoordAry->lsearch(&first_ctrl_id, &(p = 0), CMPF_LONG);
				new_coord = pCoordAry->at(p);
				GetWindowRect(new_coord.CtrlWnd, &ctrl_rect);
				linked_coord.Left   = new_coord.Right;
				linked_coord.Right  = new_coord.Left;
				linked_coord.Top    = new_coord.Bottom;
				linked_coord.Bottom = new_coord.Top;
				linked_rect.left    = ctrl_rect.right;
				linked_rect.right   = ctrl_rect.left;
				linked_rect.top     = ctrl_rect.bottom;
				linked_rect.bottom  = ctrl_rect.top;
			}
			else {
				linked_coord.Left   = pNewDlgRect->right;
				linked_coord.Right  = pNewDlgRect->left;
				linked_coord.Top    = pNewDlgRect->bottom;
				linked_coord.Bottom = pNewDlgRect->top;
				linked_rect.left    = ResizedRect.right;
				linked_rect.right   = ResizedRect.left;
				linked_rect.top     = ResizedRect.bottom;
				linked_rect.bottom  = ResizedRect.top;
			}
			for(uint i = 0; ok > 0 && i < ctrl_set_ary.getCount(); i++) {
				ResizeParamAry.lsearch(&ctrl_set_ary.at(i), &(p = 0), CMPF_LONG);
				rpe = ResizeParamAry.at(p);
				is_found = pCoordAry->lsearch(&rpe.CtrlID, &(p = 0), CMPF_LONG);
				if(is_found)
					p_coord = &pCoordAry->at(p);
				else {
					MEMSZERO(new_coord);
					new_coord.CtrlWnd = GetDlgItem(H(), rpe.CtrlID);
					if(SETIFZ(new_coord.CtrlWnd, GetDlgItem(H(), MAKE_BUTTON_ID(rpe.CtrlID, 1)))) {
						new_coord.CtrlID = rpe.CtrlID;
						new_coord.Flags  = rpe.Flags;
						p_coord = &new_coord;
					}
					else
						ok = -1;
				}
				if(ok > 0) {
					long * p_new_coord_f = isXDim ? &p_coord->Left  : &p_coord->Top;
					long * p_new_coord_s = isXDim ? &p_coord->Right : &p_coord->Bottom;
					GetWindowRect(p_coord->CtrlWnd, &ctrl_rect);
					size_to_resize = *p_old_rect_s - *p_old_rect_f;
					*p_new_coord_f = *p_new_linked_s + (*p_old_rect_f - *p_old_linked_s);
					*p_new_coord_s = *p_new_coord_f  + ((p_coord->Flags & crfResizeable) ? R0i(size_to_resize * mult) : size_to_resize);
					if(!is_found)
						ok = pCoordAry->insert(&new_coord);
					linked_coord = *p_coord;
					linked_rect  = ctrl_rect;
					if(ok)
						ok = pCalcedCtrlAry->insert(&p_coord->CtrlID);
				}
			}
		}
	}
	return ok;
}

int TDialog::Helper_ToResizeDlg(const RECT * pNewDlgRect)
{
	int   ok = 1;
	uint  i, p;
	LongArray x_calced, y_calced;
	TSVector <ResizeParamEntry> new_coord_ary;
	ResizeParamEntry new_coord;
	for(int pass = 0; ok > 0 && pass < 3; pass++) {
		for(i = 0; ok > 0 && i < ResizeParamAry.getCount(); i++) {
			const ResizeParamEntry rpe = ResizeParamAry.at(i);
			int   recalc;
			int   recalc_param;
			int   is_x_calced = x_calced.lsearch(rpe.CtrlID);
			int   is_y_calced = y_calced.lsearch(rpe.CtrlID);
			long  first_of_diap;
			long  second_of_diap;
			RECT  ctrl_rect;
			RECT  linked_ctrl_rect;
			ResizeParamEntry * p_coord = 0;
			ResizeParamEntry new_linked_coord;
			if(!is_x_calced) {
				if((rpe.Left > 0 && !(rpe.Flags & crfLinkLeft)) || (rpe.Right > 0 && !(rpe.Flags & crfLinkRight))) {
					recalc = Helper_ToRecalcCtrlSet(pNewDlgRect, rpe, &new_coord_ary, &x_calced, 1);
					if(recalc > 0)
						is_x_calced = 1;
					else if(recalc == 0)
						ok = 0;
				}
				else {
					if(!is_y_calced) {
						MEMSZERO(new_coord);
						new_coord.CtrlWnd = GetDlgItem(H(), rpe.CtrlID);
						SETIFZ(new_coord.CtrlWnd, GetDlgItem(H(), MAKE_BUTTON_ID(rpe.CtrlID, 1)));
						if(new_coord.CtrlWnd) {
							new_coord.CtrlID = rpe.CtrlID;
							new_coord.Flags  = rpe.Flags;
							p_coord = &new_coord;
						}
						else
							continue;
					}
					else if(new_coord_ary.lsearch(&rpe.CtrlID, &(p = 0), CMPF_LONG))
						p_coord = &new_coord_ary.at(p);
					else
						ok = -1;
					if(ok > 0) {
						recalc = 1;
						GetWindowRect(p_coord->CtrlWnd, &ctrl_rect);
						if(rpe.Left <= 0) {
							p_coord->Left = ctrl_rect.left - ResizedRect.left;
							first_of_diap = pNewDlgRect->left + p_coord->Left;
						}
						else if(x_calced.lsearch(rpe.Left) && new_coord_ary.lsearch(&rpe.Left, &(p = 0), CMPF_LONG)) {
							new_linked_coord = new_coord_ary.at(p);
							GetWindowRect(new_linked_coord.CtrlWnd, &linked_ctrl_rect);
							p_coord->Left = ctrl_rect.left - linked_ctrl_rect.left;
							first_of_diap = new_linked_coord.Left + p_coord->Left;
						}
						else
							recalc = 0;
						if(recalc) {
							if(rpe.Right <= 0) {
								p_coord->Right = ResizedRect.right  - ctrl_rect.right;
								second_of_diap = pNewDlgRect->right - p_coord->Right;
							}
							else if(x_calced.lsearch(rpe.Right) && new_coord_ary.lsearch(&rpe.Right, &(p = 0), CMPF_LONG)) {
								new_linked_coord = new_coord_ary.at(p);
								GetWindowRect(new_linked_coord.CtrlWnd, &linked_ctrl_rect);
								p_coord->Right = linked_ctrl_rect.right - ctrl_rect.right;
								second_of_diap = new_linked_coord.Right - p_coord->Right;
							}
							else
								recalc = 0;
							if(recalc) {
								recalc_param = BIN(rpe.Left >= 0);
								if(rpe.Right >= 0)
									recalc_param += 2;
								if(recalc_param == 3 && rpe.Flags & crfResizeable)
									recalc_param = 4;
								RecalcCtrlCoords(first_of_diap, second_of_diap, &p_coord->Left, &p_coord->Right,
									ctrl_rect.right - ctrl_rect.left, recalc_param);
								ok = x_calced.insert(&rpe.CtrlID);
								is_x_calced = 1;
								if(ok && !is_y_calced)
									ok = new_coord_ary.insert(&new_coord);
							}
						}
					}
				}
			}
			if(ok > 0 && !is_y_calced) {
				if((rpe.Top > 0 && !(rpe.Flags & crfLinkTop)) || (rpe.Bottom > 0 && !(rpe.Flags & crfLinkBottom))) {
					recalc = Helper_ToRecalcCtrlSet(pNewDlgRect, rpe, &new_coord_ary, &y_calced, 0);
					if(recalc > 0)
						is_y_calced = 1;
					else if(recalc == 0)
						ok = 0;
				}
				else {
					if(!is_x_calced) {
						MEMSZERO(new_coord);
						new_coord.CtrlWnd = GetDlgItem(H(), rpe.CtrlID);
						SETIFZ(new_coord.CtrlWnd, GetDlgItem(H(), MAKE_BUTTON_ID(rpe.CtrlID, 1)));
						if(new_coord.CtrlWnd) {
							new_coord.CtrlID = rpe.CtrlID;
							new_coord.Flags  = rpe.Flags;
							p_coord = &new_coord;
						}
						else
							continue;
					}
					else if(new_coord_ary.lsearch(&rpe.CtrlID, &(p = 0), CMPF_LONG))
						p_coord = &new_coord_ary.at(p);
					else
						ok = -1;
					if(ok > 0) {
						recalc = 1;
						GetWindowRect(p_coord->CtrlWnd, &ctrl_rect);
						if(rpe.Top <= 0) {
							p_coord->Top  = ctrl_rect.top - ResizedRect.top;
							first_of_diap = pNewDlgRect->top + p_coord->Top;
						}
						else if(y_calced.lsearch(rpe.Top) && new_coord_ary.lsearch(&rpe.Top, &(p = 0), CMPF_LONG)) {
							new_linked_coord = new_coord_ary.at(p);
							GetWindowRect(new_linked_coord.CtrlWnd, &linked_ctrl_rect);
							p_coord->Top  = ctrl_rect.top - linked_ctrl_rect.top;
							first_of_diap = new_linked_coord.Top + p_coord->Top;
						}
						else
							recalc = 0;
						if(recalc) {
							if(rpe.Bottom <= 0) {
								p_coord->Bottom = ResizedRect.bottom  - ctrl_rect.bottom;
								second_of_diap  = pNewDlgRect->bottom - p_coord->Bottom;
							}
							else if(y_calced.lsearch(rpe.Bottom) && new_coord_ary.lsearch(&rpe.Bottom, &(p = 0), CMPF_LONG)) {
								new_linked_coord = new_coord_ary.at(p);
								GetWindowRect(new_linked_coord.CtrlWnd, &linked_ctrl_rect);
								p_coord->Bottom = linked_ctrl_rect.bottom - ctrl_rect.bottom;
								second_of_diap  = new_linked_coord.Bottom - p_coord->Bottom;
							}
							else
								recalc = 0;
							if(recalc) {
								recalc_param = BIN(rpe.Top >= 0);
								if(rpe.Bottom >= 0)
									recalc_param += 2;
								if(recalc_param == 3 && p_coord->Flags & crfResizeable)
									recalc_param = 4;
								RecalcCtrlCoords(first_of_diap, second_of_diap, &p_coord->Top, &p_coord->Bottom,
									ctrl_rect.bottom - ctrl_rect.top, recalc_param);
								ok = y_calced.insert(&rpe.CtrlID);
								if(ok && !is_x_calced)
									ok = new_coord_ary.insert(&new_coord);
							}
						}
					}
				}
			}
		}
	}
	for(i = 0; ok > 0 && i < ResizeParamAry.getCount(); i++) {
		const ResizeParamEntry rpe = ResizeParamAry.at(i);
		HWND   ctrl_wnd = GetDlgItem(H(), rpe.CtrlID);
		if(ctrl_wnd && new_coord_ary.lsearch(&rpe.CtrlID, &(p = 0), CMPF_LONG)) {
			new_coord = new_coord_ary.at(p);
			ResizeParamEntry added_ctrl;
			RECT  ctrl_rect, added_rect;
			GetWindowRect(ctrl_wnd, &ctrl_rect);
			TLabel * p_label = getCtlLabel(new_coord.CtrlID);
			if(p_label && p_label->GetId() && (ctrl_wnd = GetDlgItem(H(), p_label->GetId())) != 0) {
				GetWindowRect(ctrl_wnd, &added_rect);
				added_ctrl.CtrlID  = p_label->GetId();
				added_ctrl.CtrlWnd = ctrl_wnd;
				added_ctrl.Left    = new_coord.Left + (added_rect.left   - ctrl_rect.left);
				added_ctrl.Right   = new_coord.Left + (added_rect.right  - ctrl_rect.left);
				added_ctrl.Top     = new_coord.Top  + (added_rect.top    - ctrl_rect.top);
				added_ctrl.Bottom  = new_coord.Top  + (added_rect.bottom - ctrl_rect.top);
				added_ctrl.Flags   = 0;
				ok = new_coord_ary.insert(&added_ctrl);
			}
			long  vb_id = getVirtButtonID(new_coord.CtrlID);
			if(vb_id && (ctrl_wnd = GetDlgItem(H(), vb_id)) != 0) {
				GetWindowRect(ctrl_wnd, &added_rect);
				added_ctrl.CtrlID  = vb_id;
				added_ctrl.CtrlWnd = ctrl_wnd;
				added_ctrl.Left    = new_coord.Right + (added_rect.left   - ctrl_rect.right);
				added_ctrl.Right   = new_coord.Right + (added_rect.right  - ctrl_rect.right);
				added_ctrl.Top     = new_coord.Top   + (added_rect.top    - ctrl_rect.top);
				added_ctrl.Bottom  = new_coord.Top   + (added_rect.bottom - ctrl_rect.top);
				added_ctrl.Flags   = 0;
				ok = new_coord_ary.insert(&added_ctrl);
			}
			if(new_coord.Flags & crfWClusters)
				for(p = 1; ok > 0 && (ctrl_wnd = GetDlgItem(H(), MAKE_BUTTON_ID(new_coord.CtrlID, p))) != 0; p++) {
					GetWindowRect(ctrl_wnd, &added_rect);
					added_ctrl.CtrlID  = MAKE_BUTTON_ID(rpe.CtrlID, p);
					added_ctrl.CtrlWnd = ctrl_wnd;
					added_ctrl.Left    = new_coord.Left + (added_rect.left   - ctrl_rect.left);
					added_ctrl.Right   = new_coord.Left + (added_rect.right  - ctrl_rect.left);
					added_ctrl.Top     = new_coord.Top  + (added_rect.top    - ctrl_rect.top);
					added_ctrl.Bottom  = new_coord.Top  + (added_rect.bottom - ctrl_rect.top);
					added_ctrl.Flags   = 0;
					ok = new_coord_ary.insert(&added_ctrl);
				}
		}
	}
	if(ok > 0) {
		int   sx = GetSystemMetrics((DlgFlags & fResizeable) ? SM_CXSIZEFRAME : SM_CXFIXEDFRAME);
		int   sy = GetSystemMetrics((DlgFlags & fResizeable) ? SM_CYSIZEFRAME : SM_CYFIXEDFRAME);
		int   cy = GetSystemMetrics(SM_CYCAPTION);
		::ShowWindow(H(), SW_HIDE);
		::MoveWindow(H(), pNewDlgRect->left, pNewDlgRect->top, pNewDlgRect->right - pNewDlgRect->left, pNewDlgRect->bottom - pNewDlgRect->top, 1);
		for(i = 0; i < new_coord_ary.getCount(); i++) {
			new_coord = new_coord_ary.at(i);
			::MoveWindow(new_coord.CtrlWnd,
				new_coord.Left - (pNewDlgRect->left + sx), new_coord.Top - (pNewDlgRect->top + sy + cy),
				new_coord.Right - new_coord.Left, new_coord.Bottom - new_coord.Top, 1);
		}
		::ShowWindow(H(), SW_SHOW);
		if(ToolTipsWnd) {
			::DestroyWindow(ToolTipsWnd);
			ToolTipsWnd = 0;
			TView::messageCommand(this, cmSetupTooltip);
		}
	}
	return ok;
}

int TDialog::ResizeDlgToRect(const RECT * pRect)
{
	return Helper_ToResizeDlg(pRect);
}

int TDialog::ResizeDlgToFullScreen()
{
	RECT  new_dlg_rect;
	new_dlg_rect.left   = -::GetSystemMetrics((DlgFlags & fResizeable) ? SM_CXSIZEFRAME : SM_CXFIXEDFRAME);
	new_dlg_rect.top    = -::GetSystemMetrics((DlgFlags & fResizeable) ? SM_CYSIZEFRAME : SM_CYFIXEDFRAME);
	new_dlg_rect.right  =  ::GetSystemMetrics(SM_CXFULLSCREEN) - new_dlg_rect.left;
	new_dlg_rect.bottom =  ::GetSystemMetrics(SM_CYCAPTION) + GetSystemMetrics(SM_CYFULLSCREEN) - new_dlg_rect.top;
	::GetWindowRect(H(), &ResizedRect);
	return Helper_ToResizeDlg(&new_dlg_rect);
}

void TDialog::SetDlgTrackingSize(MINMAXINFO * pMinMaxInfo)
{
	if(DlgFlags & fResizeable) {
		pMinMaxInfo->ptMinTrackSize.x = InitRect.width();
		pMinMaxInfo->ptMinTrackSize.y = InitRect.height();
	}
}

void TDialog::SetCtrlState(uint ctlID, uint state, bool enable)
{
	TView * p_v = getCtrlView(ctlID);
	CALLPTRMEMB(p_v, setState(state, enable));
}
//
// Ассоциирует с элементом диалога специальный список выбора, который фильтруется по мере ввода текста.
// Если proc = 0, то используется GetListFromSmartLbx
// Если wordSelExtra = 0 и элемент ctlID является списком или комбобоксом, то wordSelExtra = (long)SmartListBox*
//
int TDialog::SetupWordSelector(uint ctlID, WordSel_ExtraBlock * pExtra, long id, int minSymbCount, long flags)
{
	int    ok = -1;
	TView * p_v = getCtrlView(ctlID);
	if(p_v) {
		if(p_v->IsSubSign(TV_SUBSIGN_LISTBOX) || p_v->IsSubSign(TV_SUBSIGN_COMBOBOX)) {
			if(SETIFZ(pExtra, new WordSel_ExtraBlock())) {
				TDialog * p_dlg = this;
				SmartListBox * p_list = 0;
				if(p_v->IsSubSign(TV_SUBSIGN_LISTBOX))
					p_list = static_cast<SmartListBox *>(p_v);
				else {
					ListWindow * p_lw = static_cast<ComboBox *>(p_v)->getListWindow();
					if(p_lw) {
						p_list = p_lw->listBox();
						p_dlg = p_lw;
					}
				}
				if(p_list) {
					// hInputDlg, в данном случае это значение будет подставлено при вызове UiSearchBlock, который является диалогом
					pExtra->Init(CTL_LBX_LIST, 0, p_dlg, p_list->GetId(), minSymbCount, flags);
					p_list->SetWordSelBlock(pExtra);
					ok = 1;
				}
			}
		}
		else if(p_v->IsSubSign(TV_SUBSIGN_INPUTLINE)) {
			// @v10.7.7 {
			if(flags & WordSel_ExtraBlock::fFreeText) {
				//
				// В случае WordSel_ExtraBlock::fFreeText нулевой pExtra сбросит текущую установку селектора
				//
				TInputLine * p_il = static_cast<TInputLine *>(p_v);
				CALLPTRMEMB(pExtra, Init(p_il->GetId(), H(), this, p_il->GetId(), minSymbCount, flags));
				p_il->setupFreeTextWordSelector(pExtra);
				ok = 1;
			}
			// } @v10.7.7 
			else if(pExtra) {
				pExtra->Init(CTL_LBX_LIST, 0, this, p_v->GetId(), minSymbCount, flags);
				p_v->SetWordSelBlock(pExtra);
				pExtra->SetupData(id);
				ok = 1;
			}
		}
	}
	if(ok <= 0)
		delete pExtra;
	return ok;
}

int TDialog::ResetWordSelector(uint ctlID)
{
	int    ok = -1;
	TView * p_v = getCtrlView(ctlID);
	if(p_v) {
		if(p_v->IsSubSign(TV_SUBSIGN_LISTBOX) || p_v->IsSubSign(TV_SUBSIGN_COMBOBOX)) {
			SmartListBox * p_list = 0;
			if(p_v->IsSubSign(TV_SUBSIGN_LISTBOX))
				p_list = static_cast<SmartListBox *>(p_v);
			else {
				ListWindow * p_lw = static_cast<ComboBox *>(p_v)->getListWindow();
				if(p_lw)
					p_list = p_lw->listBox();
			}
			if(p_list) {
				p_list->SetWordSelBlock(0);
				ok = 1;
			}
		}
		else if(p_v->IsSubSign(TV_SUBSIGN_INPUTLINE)) {
			p_v->SetWordSelBlock(0);
			ok = 1;
		}
	}
	return ok;
}

int TDialog::Insert()
{
	int    ret = 0;
	SString buf = getTitle();
	buf.SetIfEmpty("Dialog").Transf(CTRANSF_INNER_TO_OUTER);
	if(::IsIconic(APPL->H_MainWnd))
		::ShowWindow(APPL->H_MainWnd, SW_MAXIMIZE);
	if(HW) {
		HWND   hw_frame = APPL->GetFrameWindow();
		if(hw_frame) {
			APPL->SetWindowViewByKind(HW, TProgram::wndtypDialog);
			::ShowWindow(HW, SW_SHOW);
			::UpdateWindow(HW);
		}
		ret = -1;
	}
	return ret;
}
