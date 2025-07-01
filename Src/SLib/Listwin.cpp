// LISTWIN.CPP
// Copyright (c) V.Antonov, A.Osolotkin, A.Starodub, A.Sobolev 1999-2002, 2003, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2015, 2016, 2017, 2018, 2019, 2020, 2021, 2023, 2024, 2025
// @codepage UTF-8
//
#include <slib-internal.h>
#pragma hdrstop

// @v12.2.4 (более на актуально) static const TRect _DefLwRect(0, 0, /*80*/0, 25); // @v11.9.5 Из-за того, что ширина списка теперь определяется настройками в uid-papyrus.json здесь она должна быть равна 0

ListWindow::ListWindow(ListBoxDef * pDef) : TDialog(0, 0, coNothing), PrepareSearchLetter(0), TbId(0)
{
	P_Lb = new ListWindowSmartListBox(TRect(0, 0, 11, 11), pDef);
	setDef(pDef);
	TButton * b = new TButton(TRect(0, 0, 20, 20), "OK", cmOK, TButton::spcfDefault);
	Insert_(&b->SetId(IDOK)); /*CTLID_LISTBOXOKBUTTON*/
}

ListWindow::ListWindow() : TDialog(0, 0, coNothing), P_Def(0), PrepareSearchLetter(0), P_Lb(0), TbId(0)
{
}

void   ListWindow::setCompFunc(CompFunc f) { CALLPTRMEMB(P_Lb, setCompFunc(f)); }
ListWindowSmartListBox * ListWindow::listBox() const { return P_Lb; }
void   ListWindow::prepareForSearching(int firstLetter) { PrepareSearchLetter = firstLetter; }
int    FASTCALL ListWindow::getResult(long * pVal) { return P_Def ? P_Def->getCurID(pVal) : 0; }
int    ListWindow::getString(SString & rBuf) { return P_Def ? P_Def->getCurString(rBuf) : (rBuf.Z(), 0); }
int    ListWindow::getListData(void * pData) { return P_Def ? P_Def->getCurData(pData) : 0; }
void   ListWindow::SetToolbar(uint tbId) { TbId = tbId; }

void ListWindow::executeNM(HWND parent)
{
	HWND   hwnd_parent = parent ? parent : APPL->H_MainWnd;
	MessageCommandToOwner(cmLBLoadDef);
	Id = IsTreeList() ? DLGW_TREELBX : DLGW_LBX;
	HW = APPL->CreateDlg(Id, hwnd_parent, TDialog::DialogProc, reinterpret_cast<LPARAM>(this));
	TView::SetWindowProp(H(), GWL_STYLE, WS_CHILD);
	TView::SetWindowProp(H(), GWL_EXSTYLE, 0L);
	SetParent(H(), hwnd_parent);
	if(oneof2(Id, DLGW_LBX, DLGW_TREELBX)) {
		SFontDescr list_font_descr = APPL->GetUiSettings().ListFont;
		if(list_font_descr.Face.NotEmpty())
			if(Id == DLGW_LBX)
				SetCtrlFont(CTL_LBX_LIST, list_font_descr);
			else if(Id == DLGW_TREELBX)
				SetCtrlFont(CTL_TREELBX_TREELIST, list_font_descr);
	}
	CALLPTRMEMB(P_Lb, SetLBLnkToUISrchState());
	APPL->SetWindowViewByKind(H(), TProgram::wndtypListDialog);
	Move_(0, 0);
	if(DlgFlags & fLarge) {
		P_Def->SetOption(lbtSelNotify, 1);
		::SendDlgItemMessage(H(), CTL_LBX_LIST, LB_SETITEMHEIGHT, 0, static_cast<LPARAM>(40));
	}
}

void FASTCALL ListWindow::setDef(ListBoxDef * pDef)
{
	P_Def = pDef;
	if(P_Def) {
		UserInterfaceSettings uiset;
		uiset.Restore();
		if(uiset.ListElemCount < 2) {
			uiset.ListElemCount = 20;
			uiset.Save();
		}
		P_Def->setViewHight(uiset.ListElemCount);
		P_Def->SetOption(lbtHSizeAlreadyDef, 1);
	}
	if(P_Lb) {
		TView * p = GetFirstView();
		TView * p_next = 0;
		bool   found = false;
		do {
			if(p->TestId(P_Lb->GetId())) {
				p_next = p->P_Next;
				remove(p);
				found = true;
			}
		} while(!found && p && (p = p->P_Next) != 0);
		P_Lb->SetId(IsTreeList() ? CTL_TREELBX_TREELIST : CTL_LBX_LIST);
		P_Lb->SetTreeListState(IsTreeList());
		if(found)
			insertBefore(P_Lb, p_next);
		else
			Insert_(P_Lb);
	}
}

IMPL_HANDLE_EVENT(ListWindow)
{
	if(event.isCmd(cmExecute)) {
		const int lw_dlg_id = (DlgFlags & fLarge) ? DLGW_LBX_L : DLGW_LBX;
		ComboBox * p_combo = P_Lb ? P_Lb->combo : 0;
		HWND   hwnd_parent = (p_combo && p_combo->GetLink()) ? p_combo->GetLink()->Parent : APPL->H_MainWnd;
		MessageCommandToOwner(cmLBLoadDef);
		Id = IsTreeList() ? DLGW_TREELBX : ((P_Def && P_Def->Options & lbtOwnerDraw) ? DLGW_OWNDRAWLBX : lw_dlg_id);
		HW = APPL->CreateDlg(Id, hwnd_parent, TDialog::DialogProc, reinterpret_cast<LPARAM>(this));
		if(oneof2(Id, DLGW_LBX, DLGW_TREELBX)) {
			UserInterfaceSettings ui_cfg;
			if(ui_cfg.Restore() > 0) {
				if(Id == DLGW_LBX)
					SetCtrlFont(CTL_LBX_LIST, ui_cfg.ListFont);
				else if(Id == DLGW_TREELBX)
					SetCtrlFont(CTL_TREELBX_TREELIST, ui_cfg.ListFont);
			}
		}
		APPL->SetWindowViewByKind(H(), TProgram::wndtypListDialog);
		if(p_combo) {
			if(!IsTreeList())
				p_combo->setupListWindow(1);
			else
				p_combo->setupTreeListWindow(1);
		}
		else
			Move_(0, 0);
		if(DlgFlags & fLarge) {
			P_Def->SetOption(lbtSelNotify, 1);
			::SendDlgItemMessage(H(), CTL_LBX_LIST, LB_SETITEMHEIGHT, 0, static_cast<LPARAM>(40));
		}
		if(APPL->PushModalWindow(this, H())) {
			ShowWindow(H(), SW_SHOW);
			resourceID = -1;
			EndModalCmd = 0;
			MSG    msg;
			const  uint ctl_id = IsTreeList() ? CTL_TREELBX_TREELIST : CTL_LBX_LIST;
			HWND   h_ctl_wnd = ::GetDlgItem(H(), ctl_id);
			if(PrepareSearchLetter) {
				::SendMessageW(h_ctl_wnd, WM_CHAR, PrepareSearchLetter, 0);
				PrepareSearchLetter = 0;
			}
			LDATETIME lost_focus_start_tm = ZERODATETIME;
			do {
				::GetMessage(&msg, 0, 0, 0);
				HWND h_focus_wnd = GetFocus();
				if(H() != h_focus_wnd && h_ctl_wnd != h_focus_wnd) {
					if(!lost_focus_start_tm)
						lost_focus_start_tm = getcurdatetime_();
					else if(diffdatetimesec(getcurdatetime_(), lost_focus_start_tm) > 30) {
						::PostMessage(H(), WM_COMMAND, IDCANCEL, 0);
						break;
					}
				}
				if(oneof5(msg.message, WM_LBUTTONDOWN, WM_RBUTTONDOWN, WM_NCLBUTTONDOWN, WM_NCRBUTTONDOWN, WM_MBUTTONDOWN)) {
					if(H() != msg.hwnd && H() != GetParent(msg.hwnd)) {
						::PostMessage(H(), WM_COMMAND, IDCANCEL, 0);
						break;
					}
				}
				if(!::IsDialogMessage(H(), &msg)) {
					::TranslateMessage(&msg);
					::DispatchMessage(&msg);
				}
				if(EndModalCmd && APPL->TestWindowForEndModal(this) && !IsCommandValid(EndModalCmd))
					EndModalCmd = 0;
			} while(!EndModalCmd);
			APPL->PopModalWindow(this, 0);
		}
		::DestroyWindow(H());
		HW = 0;
		if(p_combo && p_combo->GetLink())
			SetFocus(GetDlgItem(p_combo->Parent, p_combo->GetLink()->GetId()));
		clearEvent(event);
		event.message.infoLong = EndModalCmd;
	}
	else {
		if(TVCOMMAND) {
			if(TVCMD == cmOK && IsTreeList()) {
				const HWND h_tlist = GetDlgItem(H(), CTL_TREELBX_TREELIST);
				TreeView_Expand(h_tlist, TreeView_GetSelection(h_tlist), TVE_TOGGLE);
			}
			// Специальная предохранительная мера, препятствующая разрушению окна последующим вызовом TDialog::handleEvent 
			else if(TVCMD == cmCancel) {
				if(!IsInState(sfModal))
					clearEvent(event);
			}
			else if(oneof2(TVCMD, cmLBDblClk, cmLBItemSelected))
				event.setCmd(cmOK, 0);
			else if(TVCMD == cmRightClick) {
				TMenuPopup menu;
				SString item_text;
				if(TbId == 0) {
					SLS.LoadString_("add", item_text);
					menu.Add(item_text.Transf(CTRANSF_INNER_TO_OUTER).Tab().Cat("Insert"), cmaInsert);
					SLS.LoadString_("edit", item_text);
					menu.Add(item_text.Transf(CTRANSF_INNER_TO_OUTER).Tab().Cat("F11"), cmaEdit);
				}
				else {
					ToolbarList tb_items;
					if(LoadToolbar(P_SlRez, TV_EXPTOOLBAR, TbId, &tb_items)) {
						ToolbarItem tb_item;
						for(uint i = 0; tb_items.enumItems(&i, &tb_item) > 0;)
							menu.Add(tb_item.ToolTipText, NZOR(tb_item.Cmd, tb_item.KeyCode));
					}
				}
				uint   cmd = 0;
				if(menu.Execute(H(), TMenuPopup::efRet, &cmd, 0) && cmd)
					TView::messageCommand(this, cmd);
				else
					return;
			}
			else if(TVCMD == cmDrawItem) {
				const HWND parent = GetParent(H());
				TDialog * p_dlg = static_cast<TDialog *>(TView::GetWindowUserData(parent));
				TView::messageCommand(p_dlg, cmDrawItem, TVINFOPTR);
			}
		}
		else if(TVKEYDOWN) {
			switch(TVKEY) {
				case kbEsc: TView::messageCommand(this, cmCancel); break;
				case kbGrayPlus: TView::messageCommand(this, cmaLevelDown); break;
				case kbGrayMinus: TView::messageCommand(this, cmaLevelUp); break;
				default: return;
			}
			clearEvent(event);
		}
		/*
		else if(TVBROADCAST) {
			if(TVCMD == cmReleasedFocus) {

			}
		}
		*/
		TDialog::handleEvent(event);
	}
}

int ListWindow::getSingle(long * pVal)
{
	if(P_Def) {
		const long c = P_Def->GetRecsCount();
		if(c == 1) {
			P_Def->go(0);
			return P_Def->getCurID(pVal);
		}
	}
	return 0;
}

void ListWindow::Move_(HWND linkHwnd, long right)
{
	
	uint   list_ctl = IsTreeList() ? CTL_TREELBX_TREELIST : CTL_LBX_LIST;
	HWND   h_list = GetDlgItem(H(), list_ctl);
	RECT   link_rect, list_rect;
	::GetWindowRect(H(), &list_rect);
	int    _width = 0;
	if(linkHwnd) {
		::GetWindowRect(linkHwnd, &link_rect);
		link_rect.right = (right) ? right : link_rect.right;
		_width = (link_rect.right - link_rect.left); // @v11.9.4
	}
	else {
		link_rect = list_rect;
		link_rect.bottom = link_rect.top;
		_width = (ViewSize.x > 0) ? ViewSize.x : (link_rect.right - link_rect.left);
	}
	long   item_height = IsTreeList() ? TreeView_GetItemHeight(h_list) : ::SendMessageW(h_list, LB_GETITEMHEIGHT, 0, 0);
	int    h = 	P_Def ? ((P_Def->ViewHight + 1) * item_height) : (list_rect.bottom - list_rect.top);
	int    tt = link_rect.top;
	int    x  = link_rect.left;
	// @v11.9.4 int    w  = (ViewSize.x > 0) ? ViewSize.x : (link_rect.right - link_rect.left); // @v11.9.4 ViewSize.x
	HWND   h_scroll = IsTreeList() ? 0 : GetDlgItem(Parent, MAKE_BUTTON_ID(Id, 1));
	// @v12.0.12 {
	// Это - исследование путей решения проблемы неверного размещения вертикального скрол-бара 
	/*
	bool   debug_mark = false; // @debug
	HWND   v_scroll = 0; 
	{
		SCROLLBARINFO sbi;
		INITWINAPISTRUCT(sbi);
		if(::GetScrollBarInfo(Parent, OBJID_VSCROLL, &sbi)) {
			debug_mark = true;
		}
		else if(::GetScrollBarInfo(H(), OBJID_VSCROLL, &sbi)) {
			debug_mark = true;
		}
	}*/
	// } @v12.0.12
	if(GetSystemMetrics(SM_CYFULLSCREEN) > link_rect.bottom+h)
		::MoveWindow(H(), x, link_rect.bottom, _width, h, 1);
	else
		::MoveWindow(H(), x, tt-h, _width, h, 1);
	::GetClientRect(H(), &list_rect);
	if(!IsTreeList())
		list_rect.right -= (h_scroll ? GetSystemMetrics(SM_CXVSCROLL) : 0);
	::MoveWindow(h_list, 0, 0, list_rect.right, list_rect.bottom, 1);
	if(!IsTreeList() && h_scroll)
		::MoveWindow(h_scroll, list_rect.right, 0, GetSystemMetrics(SM_CXVSCROLL), list_rect.bottom, 1);
}

void ListWindow::Move_(const RECT & rRect)
{
	uint   list_ctl = IsTreeList() ? CTL_TREELBX_TREELIST : CTL_LBX_LIST;
	HWND   h_list = GetDlgItem(H(), list_ctl);
	HWND   h_scroll = IsTreeList() ? 0 : GetDlgItem(Parent, MAKE_BUTTON_ID(Id, 1));
	RECT   list_rect;
	::MoveWindow(H(), rRect.left, rRect.top, rRect.right, rRect.bottom, 1);
	::GetClientRect(H(), &list_rect);
	if(!IsTreeList())
		list_rect.right -= GetSystemMetrics(SM_CXVSCROLL);
	::MoveWindow(h_list, list_rect.left, list_rect.top, list_rect.right, list_rect.bottom, 1);
	if(P_Lb && !IsTreeList())
		P_Lb->MoveScrollBar(1);
}
//
// WordSelector
//
WordSel_ExtraBlock::WordSel_ExtraBlock()
{
	Init(0, 0, 0, 0, 0, 0);
}

WordSel_ExtraBlock::WordSel_ExtraBlock(uint inputCtl, HWND hInputDlg, TDialog * pOutDlg, uint outCtlId, uint minSymbCount, long flags)
{
	Init(inputCtl, hInputDlg, pOutDlg, outCtlId, minSymbCount, flags);
}

WordSel_ExtraBlock::~WordSel_ExtraBlock()
{
}

void WordSel_ExtraBlock::Init(uint inputCtl, HWND hInputDlg, TDialog * pOutDlg, uint outCtlId, uint minSymbCount, long flags)
{
	Flags        = flags;
	InputCtl     = inputCtl;
	H_InputDlg   = hInputDlg;
	P_OutDlg     = pOutDlg;
	OutCtlId     = outCtlId;
	MinSymbCount = NZOR(minSymbCount, 2);
	SelId        = 0;
}

int    WordSel_ExtraBlock::Search(long id, SString & rBuf) { return -1; }
int    WordSel_ExtraBlock::SearchText(const char * pText, long * pID, SString & rBuf) { return -1; }
void   WordSel_ExtraBlock::SetTextMode(bool v) { CtrlTextMode = v; }
/*virtual*/StrAssocArray * WordSel_ExtraBlock::GetRecentList() { return 0; }

void WordSel_ExtraBlock::SetData(long id, const char * pText)
 {
 	SelId = id;
 	if(P_OutDlg && OutCtlId) {
 		uint   ctl_id = OutCtlId;
 		TDialog * p_dlg = P_OutDlg;
		TView * p_v = p_dlg->getCtrlView(ctl_id);
 		if(p_v) {
 			if(p_v->IsSubSign(TV_SUBSIGN_INPUTLINE)) {
				const bool preserve_text_mode = CtrlTextMode;
				SetTextMode(true);
				p_dlg->setCtrlData(ctl_id, (void *)pText); // @badcast
				SetTextMode(preserve_text_mode);
 			}
 			else if(p_v->IsSubSign(TV_SUBSIGN_LISTBOX)) {
 				SmartListBox * p_lbx = static_cast<SmartListBox *>(p_v);
 				p_lbx->Search_(&id, 0, lbSrchByID);
 				p_lbx->selectItem(id);
 			}
			TView::messageCommand(p_dlg, cmWSSelected, p_v);
		}
	}
}

void WordSel_ExtraBlock::SetupData(long id)
{
	SString buf;
	Search(id, buf);
	SetData(id, buf);
}

int WordSel_ExtraBlock::GetData(long * pId, SString & rBuf)
{
	if(SelId)
		Search(SelId, rBuf);
	ASSIGN_PTR(pId, SelId);
	return 1;
}

/*virtual*/StrAssocArray * WordSel_ExtraBlock::GetList(const char * pText)
{
	StrAssocArray * p_list = 0;
	SString text;
	uint text_len = sstrlen(pText);
	if(text_len > 0) {
		uint min_symb_count = MinSymbCount;
		text_len = (pText[0] == '*') ? (text_len-1) : text_len;
		if(text_len >= min_symb_count) {
			if(P_OutDlg) {
				TView * p_view = P_OutDlg->getCtrlView(OutCtlId);
				if(p_view && (p_view->IsSubSign(TV_SUBSIGN_LISTBOX) || p_view->IsSubSign(TV_SUBSIGN_COMBOBOX))) {
					//SmartListBox * p_lbx = (P_OutDlg) ? (SmartListBox *)P_OutDlg->getCtrlView(OutCtlId) : 0;
					SmartListBox * p_lbx = static_cast<SmartListBox *>(p_view);
					p_list = p_lbx->P_Def ? p_lbx->P_Def->GetListByPattern(pText) : 0;
				}
			}
		}
	}
	return p_list;
}

/*virtual*/void WordSel_ExtraBlock::OnAcceptInput(const char * pText, long id)
{
}

WordSelector::WordSelector(WordSel_ExtraBlock * pBlk) : WsState(0), P_Blk(pBlk)
{
	P_Def = new StrAssocListBoxDef(new StrAssocArray(), lbtDisposeData|lbtDblClkNotify|lbtSelNotify|lbtOwnerDraw);
	P_Lb = new WordSelectorSmartListBox(TRect(0, 0, 11, 11), P_Def);
	P_Lb->SetOwnerDrawState();
	setDef(P_Def);
	TButton * b = new TButton(TRect(0, 0, 20, 20), "OK", cmOK, TButton::spcfDefault);
	Insert_(&b->SetId(IDOK)); /*CTLID_LISTBOXOKBUTTON*/
	Ptb.SetColor(clrFocus,  RGB(0x20, 0xAC, 0x90));
	Ptb.SetColor(clrOdd,    RGB(0xDC, 0xED, 0xD5));
	Ptb.SetColor(clrBkgnd,  GetColorRef((P_Blk && P_Blk->Flags & WordSel_ExtraBlock::fFreeText) ? SClrAntiquewhite : SClrYellow));
}

bool WordSelector::CheckActive() const { return LOGIC(WsState & wssActive); }
bool WordSelector::CheckVisible() const { return LOGIC(WsState & wssVisible); }
int  WordSelector::ViewRecent() { return Helper_PullDown(0, 1); }
int  WordSelector::Refresh(const char * pText) { return Helper_PullDown(pText, 0); }

int WordSelector::Activate()
{
	int    ok = -1;
	if(WsState & wssVisible) {
		WsState |= wssActive;
		::SetFocus(H());
		ok = 1;
	}
	return ok;
}

void WordSelector::ActivateInput()
{
	WsState &= ~wssActive;
	SetFocus(GetDlgItem(P_Blk->H_InputDlg, P_Blk->InputCtl));
}

int WordSelector::Helper_PullDown(const char * pText, int recent)
{
	if(recent || sstrlen(pText) >= P_Blk->MinSymbCount) {
		int    r = 0;
		SString temp_buf;
		StrAssocArray * p_data = 0;
		if(recent)
			p_data = P_Blk->GetRecentList();
		else {
			if((P_Blk->GetFlags() & WordSel_ExtraBlock::fAlwaysSearchBySubStr) && pText[0] != '*')
				(temp_buf = "*").Cat(pText);
			else
				temp_buf = pText;
			p_data = P_Blk->GetList(temp_buf);
		}
		bool   skip = (!p_data || p_data->getCount() == 0);
		if(!skip && !recent && p_data->getCount() == 1 && (P_Blk->GetFlags() & WordSel_ExtraBlock::fFreeText)) {
			//
			// Специальный случай: в режиме fFreeText единственный доступный в списке элемент,
			// равный образцу не предусмотрен для вывода в списке (в этом просто нет смыслы - текст уже в поле ввода)
			//
			StrAssocArray::Item sitem = p_data->Get(0);
			(temp_buf = pText).Strip();
			if(temp_buf.CmpNC(sitem.Txt) == 0)
				skip = true;
		}
		if(!skip) {
			if(P_Def) {
				static_cast<StrAssocListBoxDef *>(P_Def)->setArray(p_data);
				setDef(P_Def);
				HWND hw_resize_base = 0;
				if(P_Blk->H_InputDlg) {
					if(P_Blk->InputCtl)
						hw_resize_base = GetDlgItem(P_Blk->H_InputDlg, P_Blk->InputCtl);
					SETIFZ(hw_resize_base, P_Blk->H_InputDlg);
				}
				Move_(hw_resize_base, 0);
			}
			if(!CheckVisible()) {
				WsState |= (wssVisible|wssActive);
				if(APPL->P_DeskTop->execView(this) == cmOK) {
					long   id = 0L;
					getResult(&id);
					if(id && P_Blk->Search(id, temp_buf) > 0) {
						;
					}
					else
						getString(temp_buf);
					WsState &= ~(wssVisible|wssActive);
					P_Blk->SetData(id, temp_buf);
					if(P_Blk->P_OutDlg && P_Blk->P_OutDlg->IsConsistent() && P_Blk->OutCtlId) { // @v9.4.11 P_Blk->P_OutDlg->IsConsistent()
						TView * p_v = P_Blk->P_OutDlg->getCtrlView(P_Blk->OutCtlId);
 						if(p_v) {
 							if(p_v->IsSubSign(TV_SUBSIGN_INPUTLINE)) {
 								SetFocus(p_v->getHandle());
 							}
 							else if(p_v->IsSubSign(TV_SUBSIGN_LISTBOX)) {
 								SmartListBox * p_lbx = static_cast<SmartListBox *>(p_v);
 								SetFocus(p_lbx->getHandle());
 							}
						}
					}
				}
				else
					WsState &= ~wssVisible;
			}
			else
				Draw_();
			r = 1;
		}
		if(r <= 0 && CheckVisible() == 1) {
			ZDELETE(p_data);
			::SetFocus(H());
			TView::messageCommand(this, cmCancel);
			WsState &= ~wssVisible;
		}
	}
	return 1;
}

IMPL_HANDLE_EVENT(WordSelector)
{
	if(event.isCmd(cmExecute)) {
		if(P_Blk) {
			ushort last_cmd = 0;
			int    lw_dlg_id = DLGW_LBXFLAT;
			HWND   hwnd_parent = P_Blk->H_InputDlg;
			MessageCommandToOwner(cmLBLoadDef);
			Id = lw_dlg_id;
			HW = APPL->CreateDlg(Id, hwnd_parent, TDialog::DialogProc, reinterpret_cast<LPARAM>(this));
			APPL->SetWindowViewByKind(H(), TProgram::wndtypListDialog);
			{
				HWND hw_resize_base = 0;
				if(P_Blk->H_InputDlg) {
					if(P_Blk->InputCtl)
						hw_resize_base = GetDlgItem(P_Blk->H_InputDlg, P_Blk->InputCtl);
					SETIFZ(hw_resize_base, P_Blk->H_InputDlg);
				}
				Move_(hw_resize_base, 0);
			}
			if(APPL->PushModalWindow(this, H())) {
				::ShowWindow(H(), SW_SHOW);
				resourceID = -1;
				EndModalCmd = 0;
				MSG    msg;
				const  uint ctl_id = IsTreeList() ? CTL_TREELBX_TREELIST : CTL_LBX_LIST;
				HWND   h_ctl_wnd = ::GetDlgItem(H(), ctl_id);
				ActivateInput();
				do {
					GetMessage(&msg, 0, 0, 0);
					if(oneof5(msg.message, WM_LBUTTONDOWN, WM_RBUTTONDOWN, WM_NCLBUTTONDOWN, WM_NCRBUTTONDOWN, WM_MBUTTONDOWN)) {
						if(H() != msg.hwnd && H() != GetParent(msg.hwnd)) {
							::PostMessage(H(), WM_COMMAND, IDCANCEL, 0);
							break;
						}
					}
					if(!IsDialogMessage(H(), &msg)) {
						::TranslateMessage(&msg);
						::DispatchMessage(&msg);
					}
					if(EndModalCmd && APPL->TestWindowForEndModal(this) && !IsCommandValid(EndModalCmd))
						EndModalCmd = 0;
				} while(!EndModalCmd);
				last_cmd = EndModalCmd;
				EndModalCmd = 0;
				APPL->PopModalWindow(this, 0);
				{
					HWND h_input = P_Blk ? P_Blk->H_InputDlg : 0;
					if(h_input) {
						if(EndModalCmd == cmOK) {
							::PostMessage(h_input, WM_COMMAND, IDCANCEL, 0);
						}
						else {
							SetForegroundWindow(h_input);
							SetActiveWindow(h_input);
						}
					}
				}
			}
			::DestroyWindow(H());
			HW = 0;
			clearEvent(event);
			event.message.infoLong = last_cmd;
		}
	}
	else {
		if(TVCOMMAND) {
			if(TVCMD == cmDrawItem) {
				DrawListItem2(static_cast<TDrawItemData *>(TVINFOPTR));
				clearEvent(event);
			}
			else if(TVCMD == cmRightClick)
				clearEvent(event);
			else if(TVCMD == cmUp)
				ActivateInput();
			/*
			else if(TVCMD == cmLBItemFocused && CheckActive())
				;
			*/
		}
		ListWindow::handleEvent(event);
	}
}

void WordSelector::DrawListItem2(TDrawItemData * pDrawItem)
{
	if(pDrawItem) {
		if(pDrawItem->P_View) {
			TCanvas2 canv(Ptb, pDrawItem->H_DC);
			TRect _rc(pDrawItem->ItemRect);
			if(pDrawItem->ItemAction & TDrawItemData::iaBackground) {
				canv.Rect(_rc, 0, clrBkgnd);
				pDrawItem->ItemAction = 0; // Мы перерисовали фон
			}
			else if(pDrawItem->ItemID != 0xffffffff) {
				int   _clr_id = (pDrawItem->ItemState & (ODS_FOCUS|ODS_SELECTED) && CheckActive()) ? clrFocus : clrBkgnd;
				canv.SetBkColor(Ptb.GetColor(_clr_id));
				canv.Rect(_rc, 0, _clr_id);
				{
					SString & r_temp_buf = SLS.AcquireRvlStr();
					SmartListBox * p_lbx = static_cast<SmartListBox *>(pDrawItem->P_View);
					p_lbx->getText((long)pDrawItem->ItemData, r_temp_buf);
					r_temp_buf.Transf(CTRANSF_INNER_TO_OUTER);
					canv._DrawText(_rc, r_temp_buf.cptr(), DT_LEFT|DT_VCENTER|DT_SINGLELINE);
				}
			}
		}
		else
			pDrawItem->ItemAction = 0; // Список не активен - строку не рисуем
	}
}

void FASTCALL WordSelector::setDef(ListBoxDef * pDef)
{
	int    found = 0;
	TView * p_next = 0;
	P_Def = pDef;
	if(P_Def && P_Lb) {
		int    create_scroll = 0;
		uint   elem_count = P_Def->GetRecsCount();
		if(elem_count > 20) {
			create_scroll = 1;
			elem_count = 20;
		}
		P_Lb->CreateScrollBar(create_scroll);
		P_Def->setViewHight(elem_count);
		P_Def->SetOption(lbtHSizeAlreadyDef, 1);
		//
		TView * p = GetFirstView();
		do {
			if(p->TestId(P_Lb->GetId())) {
				p_next = p->P_Next;
				remove(p);
				found = 1;
			}
		} while(!found && p && (p = p->P_Next));
		P_Lb->SetId(IsTreeList() ? CTL_TREELBX_TREELIST : CTL_LBX_LIST);
		P_Lb->SetTreeListState(IsTreeList());
		if(found)
			insertBefore(P_Lb, p_next);
		else
			Insert_(P_Lb);
	}
}
//
// Utils
//
ListWindow * CreateListWindow(SArray * pAry, uint options, TYPEID type) { return new ListWindow(new StdListBoxDef(pAry, options, type)); }
ListWindow * CreateListWindow(StrAssocArray * pAry, uint options) { return new ListWindow(new StrAssocListBoxDef(pAry, options)); }
ListWindow * CreateListWindow(DBQuery & rQuery, uint options) { return new ListWindow(new DBQListBoxDef(rQuery, options, 32)); }
// @v11.2.5 ListWindow * CreateListWindow(uint sz, uint options) { return new ListWindow(new StringListBoxDef(sz, options)); }
ListWindow * CreateListWindow_Simple(uint options) // @v11.2.5
{ 
	StrAssocListBoxDef * p_def = new StrAssocListBoxDef(new StrAssocArray, options | lbtDisposeData);
	return new ListWindow(p_def); 
}
