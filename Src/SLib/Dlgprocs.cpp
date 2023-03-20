// DLGPROCS.CPP
// Copyright (c) V.Antonov, A.Osolotkin 1999-2002, 2003, 2004, 2005, 2007, 2008, 2010, 2011, 2013, 2015, 2016, 2018, 2019, 2020, 2021, 2023
// @codepage UTF-8
//
#include <slib-internal.h>
#pragma hdrstop

struct __KeyAssoc {
	uint32 Vk;
	uint16 K;
	uint16 SK;
	uint16 CK;
	uint16 AK;
};

static const __KeyAssoc _KeyAssocTab[] = {
	{ VK_RETURN,   kbEnter,     0,          kbCtrlEnter, 0  },
	{ VK_ADD,      kbGrayPlus,  0,          0,           0  },
	{ VK_SUBTRACT, kbGrayMinus, 0,          0x4a00,      kbAltMinus },
	{ VK_INSERT ,  kbIns,       kbShiftIns, kbCtrlIns,   kbAltIns   },
	{ VK_DELETE,   kbDel,       kbShiftDel, kbCtrlDel,   0          },
	{ VK_F1,       kbF1,        kbShiftF1,  kbCtrlF1,    kbAltF1 },
	{ VK_F2,       kbF2,        kbShiftF2,  kbCtrlF2,    kbAltF2 },
	{ VK_F3,       kbF3,        kbShiftF3,  kbCtrlF3,    kbAltF3 },
	{ VK_F4,       kbF4,        kbShiftF4,  kbCtrlF4,    kbAltF4 },
	{ VK_F5,       kbF5,        kbShiftF5,  kbCtrlF5,    kbAltF5 },
	{ VK_F6,       kbF6,        kbShiftF6,  kbCtrlF6,    kbAltF6 },
	{ VK_F7,       kbF7,        kbShiftF7,  kbCtrlF7,    kbAltF7 },
	{ VK_F8,       kbF8,        kbShiftF8,  kbCtrlF8,    kbAltF8 },
	{ VK_F9,       kbF9,        kbShiftF9,  kbCtrlF9,    kbAltF9  },
	{ VK_F10,      kbF10,       kbShiftF10, kbCtrlF10,   kbAltF10 },
	{ VK_F11,      kbF11,       kbShiftF11, kbCtrlF11,   kbAltF11 },
	{ VK_F12,      kbF12,       kbShiftF12, kbCtrlF12,   kbAltF12 },
	{ VK_UP,       kbUp,        0,          0,           0 },
	{ VK_DOWN,     kbDown,      0,          0,           0 },
	{ VK_LEFT,     kbLeft,      0,          0,           0 }, 
	{ VK_RIGHT,    kbRight,     0,          0,           0 },
	{ VK_TAB,      kbTab,       kbShiftTab, kbCtrlTab,   0 }, // @v10.2.2
};

static uint16 FASTCALL __MapVk(uint32 vk, uint stateP)
{
	for(uint i = 0; i < SIZEOFARRAY(_KeyAssocTab); i++) {
		if(_KeyAssocTab[i].Vk == vk) {
			switch(stateP) {
				case 0: return _KeyAssocTab[i].K;
				case 1: return _KeyAssocTab[i].SK;
				case 2: return _KeyAssocTab[i].CK;
				case 3: return _KeyAssocTab[i].AK;
				default: return 0;
			}
		}
	}
	return 0;
}

int TView::HandleKeyboardEvent(WPARAM wParam, int isPpyCodeType)
{
	TEvent event;
	event.what = TEvent::evKeyDown;
	if(isPpyCodeType)
		event.keyDown.keyCode = static_cast<uchar>(wParam);
	else {
		if(GetKeyState(VK_SHIFT) & 0x8000)
			event.keyDown.keyCode = __MapVk(wParam, 1);
		else if(GetKeyState(VK_CONTROL) & 0x8000) {
			if(wParam == VK_F11)
				wParam = VK_RETURN;
			event.keyDown.keyCode = __MapVk(wParam, 2);
		}
		else if(GetKeyState(VK_MENU) & 0x8000)
			event.keyDown.keyCode = __MapVk(wParam, 3);
		else if(wParam == VK_ESCAPE) // @v11.2.4
			event.keyDown.keyCode = kbEsc;
		else
			event.keyDown.keyCode = __MapVk(wParam, 0);
	}
	if(event.keyDown.keyCode) {
		handleEvent(event);
		return 1;
	}
	else
		return 0;
#undef GETKEYCODE
}

void TDialog::RemoveUnusedControls()
{
	const HWND h_wnd = H();
	TView * v = P_Last;
	if(v) do {
		while(v) {
			const  uint ctl_id = v->GetId();
			if(GetDlgItem(h_wnd, ctl_id) || GetDlgItem(h_wnd, MAKE_BUTTON_ID(ctl_id, 1)))
				break;
			else {
				TView * p_to_remove_view = v;
				v = v->prev();
				TGroup::remove(p_to_remove_view);
				delete p_to_remove_view;
				if(v == P_Last)
					return;
			}
		}
		v = v->prev();
	} while(v != P_Last);
}

/*static*/INT_PTR CALLBACK TDialog::DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	TDialog * p_dlg;
	TView * v;
	TEvent event;
	switch(uMsg) {
		case WM_INITDIALOG:
			if(lParam) {
				TView::SetWindowUserData(hwndDlg, reinterpret_cast<void *>(lParam));
				// @v11.2.4 {
				/*{
					long   exstyle = TView::SGetWindowExStyle(hwndDlg);
					TView::SetWindowProp(hwndDlg, GWL_EXSTYLE, (exstyle | WS_EX_COMPOSITED));
				}*/
				// } @v11.2.4 
				p_dlg = reinterpret_cast<TDialog *>(lParam);
				p_dlg->HW = hwndDlg;
				const bool export_mode = p_dlg->CheckFlag(TDialog::fExport);
				TView::messageCommand(p_dlg, cmInit);
				if(!export_mode) // @v11.0.4
					SetupCtrlTextProc(p_dlg->H(), 0);
				p_dlg->RemoveUnusedControls();
				if((v = p_dlg->P_Last) != 0) {
					do {
						HWND   ctrl = GetDlgItem(hwndDlg, v->GetId());
						SETIFZ(ctrl, GetDlgItem(hwndDlg, MAKE_BUTTON_ID(v->GetId(), 1)));
						if(IsWindow(ctrl)) {
							v->Parent = hwndDlg;
							v->handleWindowsMessage(uMsg, wParam, lParam);
							EnableWindow(ctrl, !v->IsInState(sfDisabled));
						}
					} while((v = v->prev()) != p_dlg->P_Last);
				}
				if(!export_mode)
					EnumChildWindows(hwndDlg, SetupCtrlTextProc, 0);
			}
			return 1;
		case WM_DESTROY:
			p_dlg = static_cast<TDialog *>(TView::GetWindowUserData(hwndDlg));
			if(p_dlg) {
				if((v = p_dlg->P_Last) != 0)  // @todo Практически всегда, за редкими исключениями, 0. Из-за того, что p_dlg->P_Last обнуляется раньше, чем уничтожается данное окно. Требуется исправить.
					do {
						if(reinterpret_cast<long>(v) == 0xddddddddL)
							break;
						v->handleWindowsMessage(uMsg, wParam, lParam);
					} while((v = v->prev()) != p_dlg->P_Last);
			}
			TView::SetWindowUserData(hwndDlg, static_cast<void *>(0));
			break;
		case WM_KILLFOCUS:
			p_dlg = static_cast<TDialog *>(TView::GetWindowUserData(hwndDlg));
			if(p_dlg) {
				long prev_id = CLUSTER_ID(GetDlgCtrlID(reinterpret_cast<HWND>(lParam)));
				v = p_dlg->P_Last;
				if(v) do {
					if(v->TestId(prev_id)) {
						TView::messageBroadcast(p_dlg, cmReleasedFocus, v);
		 				if(!(p_dlg->MsgLockFlags & TGroup::fLockMsgChangedFocus)) {
							p_dlg->MsgLockFlags |= TGroup::fLockMsgChangedFocus;
							TView::messageBroadcast(p_dlg, cmChangedFocus, v);
							p_dlg->MsgLockFlags &= ~TGroup::fLockMsgChangedFocus;
						}
						break;
					}
				} while((v = v->prev()) != p_dlg->P_Last);
			}
			break;
		case WM_SETFOCUS:
			p_dlg = static_cast<TDialog *>(TView::GetWindowUserData(hwndDlg));
			if(p_dlg) {
				if(lParam) {
					long prev_id = CLUSTER_ID(GetDlgCtrlID(reinterpret_cast<HWND>(lParam)));
					if((v = p_dlg->P_Last) != 0)
						do {
							if(v->TestId(prev_id)) {
								TView::messageBroadcast(p_dlg, cmReceivedFocus, v);
								v->setState(sfSelected, true);
								p_dlg->P_Current = v;
								break;
							}
						} while((v = v->prev()) != p_dlg->P_Last);
				}
				else if((v = p_dlg->P_Current) != 0) {
					HWND h_ctl = GetDlgItem(hwndDlg, v->GetId());
					if(h_ctl)
						SetFocus(h_ctl);
				}
			}
			break;
		case WM_COMMAND:
			{
				uint16 hiw = HIWORD(wParam);
				uint16 low = LOWORD(wParam);
				p_dlg = static_cast<TDialog *>(TView::GetWindowUserData(hwndDlg));
				if(GetKeyState(VK_CONTROL) & 0x8000 && low != cmaCalculate && hiw != EN_UPDATE && hiw != EN_CHANGE)
					return 0;
				else if(p_dlg) {
					if(hiw == 0 && low == IDCANCEL) {
						TView::messageCommand(p_dlg, cmCancel, p_dlg);
						return 0;
					}
					else {
						if(!lParam) {
							if(hiw == 0) // from menu
								TView::messageKeyDown(p_dlg, low);
							else if(hiw == 1) { // from accelerator
								event.what = TEvent::evCommand;
								event.message.command = low;
								p_dlg->handleEvent(event);
							}
						}
						v = p_dlg->CtrlIdToView(CLUSTER_ID(low));
						CALLPTRMEMB(v, handleWindowsMessage(uMsg, wParam, lParam));
					}
				}
				else
					return 0;
			}
			break;
		case WM_LBUTTONDBLCLK:
			p_dlg = static_cast<TDialog *>(TView::GetWindowUserData(hwndDlg));
			if(p_dlg) {
				event.what      = TEvent::evMouseDown;
				event.mouse.buttons     = static_cast<uchar>(wParam);
				event.mouse.WhereX      = LOWORD(lParam);
				event.mouse.WhereY      = HIWORD(lParam);
				event.mouse.doubleClick = 1;
				p_dlg->handleEvent(event);
			}
			break;
		case WM_LBUTTONDOWN:
			p_dlg = static_cast<TDialog *>(TView::GetWindowUserData(hwndDlg));
			if(p_dlg) {
				event.what = TEvent::evMouseDown;
				event.mouse.buttons = static_cast<uchar>(wParam);
				event.mouse.WhereX = LOWORD(lParam);
				event.mouse.WhereY = HIWORD(lParam);
				p_dlg->handleEvent(event);
			}
			break;
		case WM_RBUTTONDOWN:
			p_dlg = static_cast<TDialog *>(TView::GetWindowUserData(hwndDlg));
			if(p_dlg && HIWORD(wParam) == 1) {
				CALLPTRMEMB(p_dlg->P_Current, handleWindowsMessage(uMsg, wParam, lParam));
			}
			break;
		case WM_LBUTTONUP:
			p_dlg = static_cast<TDialog *>(TView::GetWindowUserData(hwndDlg));
			if(p_dlg) {
				event.what = TEvent::evMouseUp;
				event.mouse.buttons = static_cast<uchar>(wParam);
				event.mouse.WhereX = LOWORD(lParam);
				event.mouse.WhereY = HIWORD(lParam);
				p_dlg->handleEvent(event);
			}
			break;
		case WM_RBUTTONUP:
			p_dlg = static_cast<TDialog *>(TView::GetWindowUserData(hwndDlg));
			if(p_dlg && HIWORD(wParam) != 1)
				TView::messageKeyDown(p_dlg, kbShiftF10);
			break;
		case WM_MOUSEMOVE:
			p_dlg = static_cast<TDialog *>(TView::GetWindowUserData(hwndDlg));
			if(p_dlg) {
				event.what = TEvent::evMouseMove;
				event.mouse.buttons = static_cast<uchar>(wParam);
				event.mouse.WhereX = LOWORD(lParam);
				event.mouse.WhereY = HIWORD(lParam);
				p_dlg->handleEvent(event);
			}
			break; // @v10.3.2 @fix (отсутствовал break)
		case WM_VKEYTOITEM:
			if(PassMsgToCtrl(hwndDlg, uMsg, wParam, lParam) == -1) {
				p_dlg = static_cast<TDialog *>(TView::GetWindowUserData(hwndDlg));
				CALLPTRMEMB(p_dlg, HandleKeyboardEvent(LOWORD(wParam)));
			}
			::SendMessageW(hwndDlg, WM_USER_KEYDOWN, wParam, lParam);
			return -2;
		case WM_KEYUP:
			PassMsgToCtrl(hwndDlg, uMsg, wParam, lParam);
			return 0;
		case WM_USER_KEYDOWN:
			// @v11.2.8 if((wParam >= VK_F1 && wParam <= VK_F12) || (wParam >= 48 && wParam <= 57) || (wParam >= 65 && wParam <= 90) || (wParam >= 97 && wParam <= 122)) {
			if(checkirangef(wParam, VK_F1, VK_F12) || checkirangef(wParam, 48, 57) || checkirangef(wParam, 65, 90) || checkirangef(wParam, 97, 122)) { // @v11.2.8 
				TDialog * p_dlg = static_cast<TDialog *>(TView::GetWindowUserData(hwndDlg));
				KeyDownCommand key_cmd;
				key_cmd.State = 0;
				if(GetKeyState(VK_MENU) & 0x8000)
					key_cmd.State |= KeyDownCommand::stateAlt;
				if(GetKeyState(VK_CONTROL) & 0x8000)
					key_cmd.State |= KeyDownCommand::stateCtrl;
				if(GetKeyState(VK_SHIFT) & 0x8000)
					key_cmd.State |= KeyDownCommand::stateShift;
				key_cmd.Code = static_cast<uint16>(wParam);
				event.what = TEvent::evCommand;
				event.message.command = cmWinKeyDown;
				event.message.infoPtr = &key_cmd;
				p_dlg->handleEvent(event);
			}
			return 0;
		case WM_CHAR:
			p_dlg = static_cast<TDialog *>(TView::GetWindowUserData(hwndDlg));
			if(p_dlg) {
				short  ctrl_id = GetDlgCtrlID(reinterpret_cast<HWND>(lParam));
				short  def_inln_id = (p_dlg->DefInputLine && GetDlgItem(hwndDlg, p_dlg->DefInputLine)) ? p_dlg->DefInputLine : 0;
				if(def_inln_id && def_inln_id != ctrl_id) {
					::SetFocus(GetDlgItem(hwndDlg, def_inln_id));
					// @v11.20 SString temp_buf;
					// @v11.20 temp_buf.CatChar(static_cast<char>(LOWORD(wParam)));
					// @v11.20 TView::SSetWindowText(GetDlgItem(hwndDlg, def_inln_id), temp_buf);
					TView::SSetWindowText(GetDlgItem(hwndDlg, def_inln_id), SLS.AcquireRvlStr().CatChar(static_cast<char>(LOWORD(wParam)))); // @v11.20
					::SendDlgItemMessage(hwndDlg, def_inln_id, WM_KEYDOWN, VK_END, 0);
					return 0;
				}
			}
			if(PassMsgToCtrl(hwndDlg, uMsg, wParam, lParam) == -1) {
				if(wParam != VK_RETURN) {
					p_dlg = static_cast<TDialog *>(TView::GetWindowUserData(hwndDlg));
					CALLPTRMEMB(p_dlg, HandleKeyboardEvent(LOWORD(wParam), 1));
					return -2;
				}
			}
			return 0;
		case WM_MOUSEWHEEL:
		case WM_VSCROLL:
			PassMsgToCtrl(hwndDlg, uMsg, wParam, lParam);
			break;
		case WM_NOTIFY:
			p_dlg = static_cast<TDialog *>(TView::GetWindowUserData(hwndDlg));
			if(p_dlg && (v = p_dlg->P_Last) != 0) {
				do {
					if(v->TestId(wParam)) {
						v->handleWindowsMessage(uMsg, wParam, lParam);
						break;
					}
				} while((v = v->prev()) != p_dlg->P_Last);
			}
			break;
		case WM_ACTIVATE:
			return TRUE;
		case WM_SIZE:
		case WM_SIZING:
			p_dlg = static_cast<TDialog *>(TView::GetWindowUserData(hwndDlg));
			if(p_dlg) {
				event.what = TEvent::evCommand;
				event.message.command = cmResize;
				event.message.infoPtr = (uMsg == WM_SIZING) ? reinterpret_cast<void *>(lParam) : 0;
				p_dlg->handleEvent(event);
				APPL->DrawControl(hwndDlg, uMsg, wParam, lParam);
			}
			break;
		case WM_GETMINMAXINFO:
			p_dlg = static_cast<TDialog *>(TView::GetWindowUserData(hwndDlg));
			CALLPTRMEMB(p_dlg, SetDlgTrackingSize(reinterpret_cast<MINMAXINFO *>(lParam)));
			return 0;
		case WM_MEASUREITEM:
			{
				MEASUREITEMSTRUCT * p_mis = reinterpret_cast<MEASUREITEMSTRUCT *>(lParam);
				for(int i = 0; i < 32; i++)
					if(OwnerDrawCtrls[i].CtrlID == wParam) {
						p_mis->itemHeight = OwnerDrawCtrls[i].ExtraParam;
						break;
					}
				APPL->DrawControl(hwndDlg, uMsg, wParam, lParam);
			}
			return TRUE;
		case WM_ERASEBKGND:
			/*
			p_dlg = static_cast<TDialog *>(TView::GetWindowUserData(hwndDlg));
			if(p_dlg && APPL->EraseBackground(p_dlg, hwndDlg, reinterpret_cast<HDC>(wParam), 0) > 0)
				return 1;
			*/
			return 0;
		case WM_DRAWITEM:
			p_dlg = static_cast<TDialog *>(TView::GetWindowUserData(hwndDlg));
			return p_dlg ? p_dlg->RedirectDrawItemMessage(uMsg, wParam, lParam) : FALSE;
			/*
			if(p_dlg) {
				DRAWITEMSTRUCT * p_dis = reinterpret_cast<DRAWITEMSTRUCT *>(lParam);
				TDrawItemData di;
				di.CtlType = p_dis->CtlType;
				di.CtlID   = p_dis->CtlID;
				di.ItemID  = p_dis->itemID;
				di.ItemAction = p_dis->itemAction;
				di.ItemState  = p_dis->itemState;
				di.H_Item     = p_dis->hwndItem;
				di.H_DC       = p_dis->hDC;
				di.ItemRect   = p_dis->rcItem;
				di.P_View   = p_dlg->getCtrlView(LOWORD(di.CtlID));
				di.ItemData = p_dis->itemData;
				TView::messageCommand(p_dlg, cmDrawItem, &di);
				if(APPL->DrawControl(hwndDlg, uMsg, wParam, lParam) > 0)
					di.ItemAction = p_dis->itemAction;
				if(di.ItemAction == 0)
					return FALSE;
			}
			break;
			*/
		/*
		case WM_INPUTLANGCHANGE: // @v6.4.4 AHTOXA
			PostMessage(GetParent(hwndDlg), uMsg, wParam, lParam);
			break;
		*/
		case WM_CTLCOLORSTATIC:
		case WM_CTLCOLOREDIT:
		case WM_CTLCOLORSCROLLBAR:
			p_dlg = static_cast<TDialog *>(TView::GetWindowUserData(hwndDlg));
			if(p_dlg) {
				TDrawCtrlData dc;
				if(uMsg == WM_CTLCOLORSTATIC)
					dc.Src = TDrawCtrlData::cStatic;
				else if(uMsg == WM_CTLCOLOREDIT)
					dc.Src = TDrawCtrlData::cEdit;
				else if(uMsg == WM_CTLCOLORSCROLLBAR)
					dc.Src = TDrawCtrlData::cScrollBar;
				else
					dc.Src = 0;
				dc.H_Ctl = reinterpret_cast<HWND>(lParam);
				dc.H_DC  = reinterpret_cast<HDC>(wParam);
				dc.H_Br  = 0;
				if(TView::messageCommand(p_dlg, cmCtlColor, &dc))
					return (BOOL)dc.H_Br;
				/* @construction
				else {
					if(dc.Src == TDrawCtrlData::cEdit) {
						TView * p_view = p_dlg->getCtrlByHandle(dc.H_Ctl);
						if(p_view && p_view->GetSubSign() == TV_SUBSIGN_INPUTLINE) {
							TInputLine * p_il = (TInputLine *)p_view;
							if(GETSTYPE(p_il->getType()) == S_DATE) {
								SString & r_temp_buf = SLS.AcquireRvlStr();
								p_il->getText(r_temp_buf);
								if(r_temp_buf.NotEmpty()) {
									int    d, m, y;
									long   sdret = 0;
									_strtodate(r_temp_buf, p_il->getFormat(), &d, &m, &y, &sdret);
									if(sdret & strtodatefInvalid) {
										SPaintToolBox & r_tb = APPL->GetUiToolBox();
										return (BOOL)r_tb.Get(TProgram::tbiInvalInpBrush);
									}
								}
							}
						}
					}
				}
				@construction */
			}
			// no break: ret FALSE by default
		case WM_PAINT:
			p_dlg = static_cast<TDialog *>(TView::GetWindowUserData(hwndDlg));
			if(TView::messageCommand(p_dlg, cmPaint))
				return FALSE;
		default:
			return FALSE;
	}
	return TRUE;
}
