// LISTWIN.CPP
// Copyright (c) V.Antonov, A.Osolotkin, A.Starodub, A.Sobolev 1999-2002, 2003, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2015, 2016, 2017, 2018
// @codepage UTF-8
//
#include <slib.h>
#include <tv.h>
#pragma hdrstop

static TRect _DefLwRect(0, 0, 80, 25); // @v8.8.2 60-->80

ListWindow::ListWindow(ListBoxDef * pDef, const char * pTitle, int aNum) : TDialog(_DefLwRect, pTitle), PrepareSearchLetter(0), TbId(0)
{
	//@v9.8.0 IsLargeListBox      = 0;
	P_Lb = new ListWindowSmartListBox(TRect(0, 0, 11, 11), pDef);
	setDef(pDef);
	TButton * b = new TButton(TRect(0, 0, 20, 20), "OK", cmOK, bfDefault);
	Insert_(&b->SetId(IDOK)); /*CTLID_LISTBOXOKBUTTON*/
}

ListWindow::ListWindow() : TDialog(_DefLwRect, 0), P_Def(0), PrepareSearchLetter(0), P_Lb(0), TbId(0)
{
}

void ListWindow::setCompFunc(CompFunc f)
{
	CALLPTRMEMB(P_Lb, setCompFunc(f));
}

ListWindowSmartListBox * ListWindow::listBox() const
{
	return P_Lb;
}

void ListWindow::prepareForSearching(int firstLetter)
{
	PrepareSearchLetter = firstLetter;
}

void ListWindow::executeNM(HWND parent)
{
	HWND   hwnd_parent = parent ? parent : APPL->H_MainWnd;
	MessageCommandToOwner(cmLBLoadDef);
	Id = (isTreeList()) ? DLGW_TREELBX : DLGW_LBX;
	HW = APPL->CreateDlg(Id, hwnd_parent, (DLGPROC)TDialog::DialogProc, (LPARAM)this);
	TView::SetWindowProp(H(), GWL_STYLE, WS_CHILD);
	TView::SetWindowProp(H(), GWL_EXSTYLE, 0L);
	SetParent(H(), hwnd_parent);
	if(oneof2(Id, DLGW_LBX, DLGW_TREELBX)) {
		UserInterfaceSettings ui_cfg;
		if(ui_cfg.Restore() > 0) {
			if(Id == DLGW_LBX)
				SetCtrlFont(CTL_LBX_LIST, ui_cfg.ListFont);
			else if(Id == DLGW_TREELBX)
				SetCtrlFont(CTL_TREELBX_TREELIST, ui_cfg.ListFont);
		}
	}
	CALLPTRMEMB(P_Lb, SetLBLnkToUISrchState());
	APPL->SetWindowViewByKind(H(), TProgram::wndtypListDialog);
	MoveWindow(0, 0);
	if(DlgFlags & fLarge) {
		P_Def->SetOption(lbtSelNotify, 1);
		::SendDlgItemMessage(H(), CTL_LBX_LIST, LB_SETITEMHEIGHT, 0, (LPARAM)40);
	}
}

int FASTCALL ListWindow::setDef(ListBoxDef * pDef)
{
	int    found = 0;
	TView * p_next = 0;
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
		do {
			if(p->TestId(P_Lb->GetId())) {
				p_next = p->P_Next;
				remove(p);
				found = 1;
			}
		} while(!found && p && (p = p->P_Next));
		P_Lb->SetId(isTreeList() ? CTL_TREELBX_TREELIST : CTL_LBX_LIST);
		P_Lb->SetTreeListState(isTreeList());
		if(found)
			insertBefore(P_Lb, p_next);
		else
			Insert_(P_Lb);
	}
	return 1;
}

int ListWindow::isTreeList() const
{
	return BIN(P_Def && P_Def->_isTreeList());
}

IMPL_HANDLE_EVENT(ListWindow)
{
	if(event.isCmd(cmExecute)) {
		int    lw_dlg_id = (DlgFlags & fLarge) ? DLGW_LBX_L : DLGW_LBX;
		ComboBox * p_combo = P_Lb ? P_Lb->combo : 0;
		HWND   hwnd_parent = p_combo ? p_combo->link()->Parent : APPL->H_MainWnd;
		MessageCommandToOwner(cmLBLoadDef);
		Id = isTreeList() ? DLGW_TREELBX : ((P_Def && P_Def->Options & lbtOwnerDraw) ? DLGW_OWNDRAWLBX : lw_dlg_id);
		HW = APPL->CreateDlg(Id, hwnd_parent, (DLGPROC)TDialog::DialogProc, (LPARAM)this);
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
			if(!isTreeList())
				p_combo->setupListWindow(1);
			else
				p_combo->setupTreeListWindow(1);
		}
		else
			MoveWindow(0, 0);
		if(DlgFlags & fLarge) {
			P_Def->SetOption(lbtSelNotify, 1);
			::SendDlgItemMessage(H(), CTL_LBX_LIST, LB_SETITEMHEIGHT, 0, (LPARAM)40);
		}
		if(APPL->PushModalWindow(this, H())) {
			ShowWindow(H(), SW_SHOW);
			resourceID = -1;
			EndModalCmd = 0;
			MSG    msg;
			const  uint ctl_id = isTreeList() ? CTL_TREELBX_TREELIST : CTL_LBX_LIST;
			HWND   h_ctl_wnd = ::GetDlgItem(H(), ctl_id);
			if(PrepareSearchLetter) {
				::SendMessage(h_ctl_wnd, WM_CHAR, PrepareSearchLetter, 0);
				PrepareSearchLetter = 0;
			}
			LDATETIME lost_focus_start_tm;
			lost_focus_start_tm.SetZero();
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
				if(EndModalCmd && APPL->TestWindowForEndModal(this) && !valid(EndModalCmd))
					EndModalCmd = 0;
			} while(!EndModalCmd);
			APPL->PopModalWindow(this, 0);
		}
		::DestroyWindow(H());
		HW = 0;
		if(p_combo)
			SetFocus(GetDlgItem(p_combo->Parent, p_combo->link()->GetId()));
		clearEvent(event);
		event.message.infoLong = EndModalCmd;
	}
	else {
		if(TVCOMMAND) {
			if(TVCMD == cmOK && isTreeList()) {
				HWND   h_tlist = GetDlgItem(H(), CTL_TREELBX_TREELIST);
				TreeView_Expand(h_tlist, TreeView_GetSelection(h_tlist), TVE_TOGGLE);
			}
			// @v9.8.12 { 
			//  Специальная предохранительная мера, препятствующая разрушению окна последующим вызовом 
			//  TDialog::handleEvent
			else if(TVCMD == cmCancel) {
				if(!IsInState(sfModal))
					clearEvent(event);
			}
			// } @v9.8.12 
			else if(oneof2(TVCMD, cmLBDblClk, cmLBItemSelected))
				event.setCmd(cmOK, 0);
			else if(TVCMD == cmRightClick) {
				TMenuPopup menu;
				SString item_text;
				if(TbId == 0) {
					SLS.LoadString("add", item_text);
					menu.Add(item_text.Transf(CTRANSF_INNER_TO_OUTER).Tab().Cat("Insert"), cmaInsert);
					SLS.LoadString("edit", item_text);
					menu.Add(item_text.Transf(CTRANSF_INNER_TO_OUTER).Tab().Cat("F11"), cmaEdit);
				}
				else {
					ToolbarList tb_items;
					if(LoadToolbar(P_SlRez, TV_EXPTOOLBAR, TbId, &tb_items)) {
						ToolbarItem tb_item;
						for(uint i = 0; tb_items.enumItems(&i, &tb_item) > 0;)
							menu.Add(tb_item.ToolTipText, (tb_item.Cmd) ? tb_item.Cmd : tb_item.KeyCode);
					}
				}
				int    cmd = menu.Execute(H(), TMenuPopup::efRet);
				if(cmd > 0) {
					// @v8.1.3 event.message.command = cmd;
					TView::messageCommand(this, cmd); // @v8.1.3
				}
				else
					return;
			}
			else if(TVCMD == cmDrawItem) {
				HWND parent = GetParent(H());
				TDialog * p_dlg = (TDialog*)TView::GetWindowUserData(parent);
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

int FASTCALL ListWindow::getResult(long * pVal)
{
	return P_Def ? P_Def->getCurID(pVal) : 0;
}

int ListWindow::getString(SString & rBuf)
{
	return P_Def ? P_Def->getCurString(rBuf) : (rBuf.Z(), 0);
}

int ListWindow::getListData(void * pData)
{
	return P_Def ? P_Def->getCurData(pData) : 0;
}

int ListWindow::getSingle(long * pVal)
{
	if(P_Def) {
		const long c = P_Def->getRecsCount();
		if(c == 1) {
			P_Def->go(0);
			return P_Def->getCurID(pVal);
		}
	}
	return 0;
}

int ListWindow::MoveWindow(HWND linkHwnd, long right)
{
	uint   list_ctl = isTreeList() ? CTL_TREELBX_TREELIST : CTL_LBX_LIST;
	HWND   h_list = GetDlgItem(H(), list_ctl);
	RECT   link_rect, list_rect;
	if(linkHwnd) {
		GetWindowRect(linkHwnd, &link_rect);
		link_rect.right = (right) ? right : link_rect.right;
	}
	else {
		GetWindowRect(H(), &link_rect);
		link_rect.bottom = link_rect.top;
	}
	GetWindowRect(H(), &list_rect);
	long   item_height = isTreeList() ? TreeView_GetItemHeight(h_list) : ::SendMessage(h_list, LB_GETITEMHEIGHT, 0, 0);
	int    h = 	P_Def ? ((P_Def->ViewHight + 1) * item_height) : (list_rect.bottom - list_rect.top);
	int    tt = link_rect.top;
	int    x  = link_rect.left;
	int    w  = (link_rect.right - link_rect.left);
	HWND   h_scroll = (!isTreeList()) ? GetDlgItem(Parent, MAKE_BUTTON_ID(Id, 1)) : 0;

	if(GetSystemMetrics(SM_CYFULLSCREEN) > link_rect.bottom+h)
		::MoveWindow(H(), x, link_rect.bottom, w, h, 1);
	else
		::MoveWindow(H(), x, tt-h, w, h, 1);
	::GetClientRect(H(), &list_rect);
	if(!isTreeList())
		list_rect.right -= (h_scroll) ? GetSystemMetrics(SM_CXVSCROLL) : 0;
	::MoveWindow(h_list, 0, 0, list_rect.right, list_rect.bottom, 1);
	if(!isTreeList() && h_scroll)
		::MoveWindow(h_scroll, list_rect.right, 0, GetSystemMetrics(SM_CXVSCROLL), list_rect.bottom, 1);
	return 1;
}

int ListWindow::MoveWindow(RECT & rRect)
{
	uint   list_ctl = (isTreeList()) ? CTL_TREELBX_TREELIST : CTL_LBX_LIST;
	HWND   h_list = GetDlgItem(H(), list_ctl);
	HWND   h_scroll = (!isTreeList()) ? GetDlgItem(Parent, MAKE_BUTTON_ID(Id, 1)) : 0;
	RECT   list_rect;

	::MoveWindow(H(), rRect.left, rRect.top, rRect.right, rRect.bottom, 1);
	::GetClientRect(H(), &list_rect);
	if(!isTreeList())
		list_rect.right -= GetSystemMetrics(SM_CXVSCROLL);
	::MoveWindow(h_list, list_rect.left, list_rect.top, list_rect.right, list_rect.bottom, 1);
	if(P_Lb && !isTreeList())
		P_Lb->MoveScrollBar(1);
	return 1;
}

void ListWindow::SetToolbar(uint tbId)
{
	TbId = tbId;
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
	// @v8.3.11 CtrlTextMode = false;
	SelId        = 0;
}

int WordSel_ExtraBlock::Search(long id, SString & rBuf)
{
	return -1;
}

int WordSel_ExtraBlock::SearchText(const char * pText, long * pID, SString & rBuf)
{
	return -1;
}

void WordSel_ExtraBlock::SetTextMode(bool v)
{
	CtrlTextMode = v;
}

void WordSel_ExtraBlock::SetData(long id, const char * pText)
 {
 	SelId = id;
 	if(P_OutDlg && OutCtlId) {
 		uint   ctl_id = OutCtlId;
 		TDialog * p_dlg = P_OutDlg;
		TView * p_v = p_dlg ? p_dlg->getCtrlView(ctl_id) : 0;
 		if(p_v) {
 			if(p_v->IsSubSign(TV_SUBSIGN_INPUTLINE)) {
				bool   preserve_text_mode = CtrlTextMode;
				SetTextMode(true);
				p_dlg->setCtrlData(ctl_id, (void *)pText); // @badcast
				SetTextMode(preserve_text_mode);
 			}
 			else if(p_v->IsSubSign(TV_SUBSIGN_LISTBOX)) {
 				SmartListBox * p_lbx = (SmartListBox*)p_v;
 				p_lbx->search(&id, 0, lbSrchByID);
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

// virtual
StrAssocArray * WordSel_ExtraBlock::GetList(const char * pText)
{
	StrAssocArray * p_list = 0;
	SString text;
	uint text_len = sstrlen(pText);
	if(text_len > 0) {
		uint min_symb_count = MinSymbCount;
		text_len = (pText[0] == '*') ? text_len - 1 : text_len;
		if(text_len >= min_symb_count) {
			if(P_OutDlg) {
				TView * p_view = P_OutDlg->getCtrlView(OutCtlId);
				if(p_view && (p_view->IsSubSign(TV_SUBSIGN_LISTBOX) || p_view->IsSubSign(TV_SUBSIGN_COMBOBOX))) {
					//SmartListBox * p_lbx = (P_OutDlg) ? (SmartListBox *)P_OutDlg->getCtrlView(OutCtlId) : 0;
					SmartListBox * p_lbx = (SmartListBox *)p_view;
					p_list = p_lbx->def ? p_lbx->def->GetListByPattern(pText) : 0;
				}
			}
		}
	}
	return p_list;
}


WordSelector::WordSelector(WordSel_ExtraBlock * pBlk) : IsActive(0), IsVisible(0), P_Blk(pBlk)
{
	P_Def = new StrAssocListBoxDef(new StrAssocArray(), lbtDisposeData|lbtDblClkNotify|lbtSelNotify|lbtOwnerDraw);
	P_Lb = (WordSelectorSmartListBox*)new WordSelectorSmartListBox(TRect(0, 0, 11, 11), P_Def);
	P_Lb->SetOwnerDrawState();
	setDef(P_Def);
	TButton * b = new TButton(TRect(0, 0, 20, 20), "OK", cmOK, bfDefault);
	Insert_(&b->SetId(IDOK)); /*CTLID_LISTBOXOKBUTTON*/

	Ptb.SetColor(clrFocus,  RGB(0x20, 0xAC, 0x90));
	Ptb.SetColor(clrOdd,    RGB(0xDC, 0xED, 0xD5));
	Ptb.SetColor(clrBkgnd,  RGB(0xFF, 0xFF, 0x00));
	// @v9.7.12 Ptb.SetBrush(brSel,    SPaintObj::psSolid, Ptb.GetColor(clrFocus), 0);
	// @v9.7.12 Ptb.SetBrush(brOdd,    SPaintObj::psSolid, Ptb.GetColor(clrOdd), 0);
	// @v9.7.12 Ptb.SetBrush(brBkgnd,  SPaintObj::psSolid, Ptb.GetColor(clrBkgnd), 0);
}

int WordSelector::CheckActive() const
{
	return IsActive;
}

int WordSelector::CheckVisible() const
{
	return IsVisible;
}

int WordSelector::Activate()
{
	int    ok = -1;
	if(IsVisible) {
		IsActive = 1;
		::SetFocus(H());
		ok = 1;
	}
	return ok;
}

void WordSelector::ActivateInput()
{
	IsActive = 0;
	SetFocus(GetDlgItem(P_Blk->H_InputDlg, P_Blk->InputCtl));
}

int WordSelector::Refresh(const char * pText)
{
	if(sstrlen(pText) >= P_Blk->MinSymbCount) { // @v8.3.11
		int    r = 0;
		StrAssocArray * p_data = 0;
		SString text;
		if((P_Blk->GetFlags() & WordSel_ExtraBlock::fAlwaysSearchBySubStr) && pText[0] != '*')
			(text = "*").Cat(pText);
		else
			text = pText;
		if((p_data = P_Blk->GetList(text)) && p_data->getCount()) {
			if(P_Def) {
				((StrAssocListBoxDef*)P_Def)->setArray(p_data);
				setDef(P_Def);
				MoveWindow(P_Blk->H_InputDlg, 0);
			}
			if(CheckVisible() == 0) {
				IsVisible = 1;
				IsActive  = 1;
				if(APPL->P_DeskTop->execView(this) == cmOK) {
					long   id = 0L;
					SString buf;
					getResult(&id);
					if(id && P_Blk->Search(id, buf) > 0) {
						;
					}
					else {
						getString(buf);
					}
					// @v8.3.11 buf.Transf(CTRANSF_INNER_TO_OUTER);
					IsActive  = 0;
					IsVisible = 0;
					P_Blk->SetData(id, buf);
					if(P_Blk->P_OutDlg && P_Blk->P_OutDlg->IsConsistent() && P_Blk->OutCtlId) { // @v9.4.11 P_Blk->P_OutDlg->IsConsistent()
						TView * p_v = P_Blk->P_OutDlg->getCtrlView(P_Blk->OutCtlId);
 						if(p_v) {
 							if(p_v->IsSubSign(TV_SUBSIGN_INPUTLINE)) {
 								SetFocus(p_v->getHandle());
 							}
 							else if(p_v->IsSubSign(TV_SUBSIGN_LISTBOX)) {
 								SmartListBox * p_lbx = (SmartListBox*)p_v;
 								SetFocus(p_lbx->getHandle());
 							}
						}
					}
				}
				else
					IsVisible = 0;
			}
			else
				Draw_();
			r = 1;
		}
		if(r <= 0 && CheckVisible() == 1) {
			::SetFocus(H());
			TView::messageCommand(this, cmCancel);
			IsVisible = 0;
		}
	}
	return 1;
}

IMPL_HANDLE_EVENT(WordSelector)
{
	if(event.isCmd(cmExecute)) {
		ushort last_cmd = 0;
		int    lw_dlg_id = DLGW_LBXFLAT;
		HWND   hwnd_parent = P_Blk->H_InputDlg;
		MessageCommandToOwner(cmLBLoadDef);
		Id = lw_dlg_id;
		HW = APPL->CreateDlg(Id, hwnd_parent, (DLGPROC)TDialog::DialogProc, (LPARAM)this);
		APPL->SetWindowViewByKind(H(), TProgram::wndtypListDialog);
		MoveWindow(P_Blk->H_InputDlg, 0);
		if(APPL->PushModalWindow(this, H())) {
			::ShowWindow(H(), SW_SHOW);
			resourceID = -1;
			EndModalCmd = 0;
			MSG    msg;
			const  uint ctl_id = isTreeList() ? CTL_TREELBX_TREELIST : CTL_LBX_LIST;
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
				if(EndModalCmd && APPL->TestWindowForEndModal(this) && !valid(EndModalCmd))
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
	else {
		if(TVCOMMAND) {
			if(TVCMD == cmDrawItem) {
				//DrawListItem((TDrawItemData*)TVINFOPTR);
				DrawListItem2((TDrawItemData*)TVINFOPTR);
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

#if 0 // @v9.7.12 (replaced with DrawListItem2) {
void WordSelector::DrawListItem(TDrawItemData * pDrawItem)
{
	if(pDrawItem && pDrawItem->P_View) {
		long   list_ctrl_id = pDrawItem->P_View->GetId();
		HDC    h_dc = pDrawItem->H_DC;
		HFONT  h_fnt_def  = 0;
		HBRUSH h_br_def   = 0;
		HPEN   h_pen_def  = 0;
		COLORREF clr_prev = 0;
		SmartListBox * p_lbx = (SmartListBox *)pDrawItem->P_View;
		RECT   rc = pDrawItem->ItemRect;
		SString temp_buf;
		{
			if(pDrawItem->ItemAction & TDrawItemData::iaBackground) {
				::FillRect(h_dc, &rc, (HBRUSH)Ptb.Get(brBkgnd));
				pDrawItem->ItemAction = 0; // Мы перерисовали фон
			}
			else if(pDrawItem->ItemID != 0xffffffff) {
				// h_fnt_def = (HFONT)SelectObject(h_dc, (HFONT)Ptb.Get(font));
				p_lbx->getText((long)pDrawItem->ItemData, temp_buf);
				temp_buf.Transf(CTRANSF_INNER_TO_OUTER);
				if(pDrawItem->ItemState & (ODS_FOCUS|ODS_SELECTED) && CheckActive()) {
					h_br_def = (HBRUSH)SelectObject(h_dc, Ptb.Get(brSel));
					clr_prev = SetBkColor(h_dc, Ptb.GetColor(clrFocus));
					::FillRect(h_dc, &rc, (HBRUSH)Ptb.Get(brSel));
				}
				else {
					h_br_def = (HBRUSH)SelectObject(h_dc, Ptb.Get(brBkgnd));
					clr_prev = SetBkColor(h_dc, Ptb.GetColor(clrBkgnd));
					::FillRect(h_dc, &rc, (HBRUSH)Ptb.Get(brBkgnd));
				}
				::DrawText(h_dc, temp_buf.cptr(), temp_buf.Len(), &rc, DT_LEFT|DT_VCENTER|DT_SINGLELINE); // @unicodeproblem
			}
		}
		if(h_fnt_def)
			SelectObject(h_dc, h_fnt_def);
		if(h_br_def)
			SelectObject(h_dc, h_br_def);
		if(h_pen_def)
			SelectObject(h_dc, h_pen_def);
		if(clr_prev)
			SetBkColor(h_dc, clr_prev);
	}
	else
		pDrawItem->ItemAction = 0; // Список не активен - строку не рисуем
}
#endif // } 0

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
					SmartListBox * p_lbx = (SmartListBox *)pDrawItem->P_View;
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

int FASTCALL WordSelector::setDef(ListBoxDef * pDef)
{
	int    found = 0;
	TView * p_next = 0;
	P_Def = pDef;
	if(P_Def) {
		int    create_scroll = 0;
		uint   elem_count = P_Def->getRecsCount();
		if(elem_count > 20) {
			create_scroll = 1;
			elem_count = 20;
		}
		P_Lb->CreateScrollBar(create_scroll);
		P_Def->setViewHight(elem_count);
		P_Def->SetOption(lbtHSizeAlreadyDef, 1);
	}
	if(P_Lb) {
		TView * p = GetFirstView();
		do {
			if(p->TestId(P_Lb->GetId())) {
				p_next = p->P_Next;
				remove(p);
				found = 1;
			}
		} while(!found && p && (p = p->P_Next));
		P_Lb->SetId(isTreeList() ? CTL_TREELBX_TREELIST : CTL_LBX_LIST);
		P_Lb->SetTreeListState(isTreeList());
		if(found)
			insertBefore(P_Lb, p_next);
		else
			Insert_(P_Lb);
	}
	return 1;
}
//
// Utils
//
ListWindow * SLAPI CreateListWindow(SArray * pAry, uint options, TYPEID type) { return new ListWindow(new StdListBoxDef(pAry, options, type), 0, 0); }
ListWindow * SLAPI CreateListWindow(StrAssocArray * pAry, uint options) { return new ListWindow(new StrAssocListBoxDef(pAry, options), 0, 0); }
ListWindow * SLAPI CreateListWindow(DBQuery & rQuery, uint options) { return new ListWindow(new DBQListBoxDef(rQuery, options, 32), 0, 0); }
ListWindow * SLAPI CreateListWindow(uint sz, uint options) { return new ListWindow(new StringListBoxDef(sz, options), 0, 0); }
// @v9.8.12 (unused) WordSelector * SLAPI CreateWordSelector(WordSel_ExtraBlock * pBlk) { return new WordSelector(pBlk); }
