// TEXTBRW.CPP
// Copyright (c) A.Starodub 2013, 2014, 2015, 2016
// STextBrowser
//
#include <pp.h>
#pragma hdrstop
#include <scintilla.h>
#include <scilexer.h>
#include "..\sartr\sartr.h"

static int FASTCALL VkToScTranslate(int keyIn)
{
	switch(keyIn) {
		case VK_DOWN:		return SCK_DOWN;
		case VK_UP:			return SCK_UP;
		case VK_LEFT:		return SCK_LEFT;
		case VK_RIGHT:		return SCK_RIGHT;
		case VK_HOME:		return SCK_HOME;
		case VK_END:		return SCK_END;
		case VK_PRIOR:		return SCK_PRIOR;
		case VK_NEXT:		return SCK_NEXT;
		case VK_DELETE:		return SCK_DELETE;
		case VK_INSERT:		return SCK_INSERT;
		case VK_ESCAPE:		return SCK_ESCAPE;
		case VK_BACK:		return SCK_BACK;
		case VK_TAB:		return SCK_TAB;
		case VK_RETURN:		return SCK_RETURN;
		case VK_ADD:		return SCK_ADD;
		case VK_SUBTRACT:	return SCK_SUBTRACT;
		case VK_DIVIDE:		return SCK_DIVIDE;
		case VK_OEM_2:		return '/';
		case VK_OEM_3:		return '`';
		case VK_OEM_4:		return '[';
		case VK_OEM_5:		return '\\';
		case VK_OEM_6:		return ']';
		default:			return keyIn;
	}
};

static int FASTCALL ScToVkTranslate(int keyIn)
{
	switch(keyIn) {
		case SCK_DOWN: return VK_DOWN;
		case SCK_UP: return VK_UP;
		case SCK_LEFT: return VK_LEFT;
		case SCK_RIGHT: return VK_RIGHT;
		case SCK_HOME: return VK_HOME;
		case SCK_END: return VK_END;
		case SCK_PRIOR: return VK_PRIOR;
		case SCK_NEXT: return VK_NEXT;
		case SCK_DELETE: return VK_DELETE;
		case SCK_INSERT: return VK_INSERT;
		case SCK_ESCAPE: return VK_ESCAPE;
		case SCK_BACK: return VK_BACK;
		case SCK_TAB: return VK_TAB;
		case SCK_RETURN: return VK_RETURN;
		case SCK_ADD: return VK_ADD;
		case SCK_SUBTRACT: return VK_SUBTRACT;
		case SCK_DIVIDE: return VK_DIVIDE;
		case '/': return VK_OEM_2;
		case '`': return VK_OEM_3;
		case '[': return VK_OEM_4;
		case '\\': return VK_OEM_5;
		case ']': return VK_OEM_6;
		default: return keyIn;
	}
}

STextBrowser::Document::Document()
{
	Cp = cpANSI;
	Eolf = eolUndef;
	State = 0;
	SciDoc = 0;
}

STextBrowser::Document & STextBrowser::Document::Reset()
{
	OrgCp = cpUndef;
	Cp = cpUndef;
	Eolf = eolUndef;
	State = 0;
	SciDoc = 0;
	FileName = 0;
	return *this;
}

long STextBrowser::Document::SetState(long st, int set)
{
	SETFLAG(State, st, set);
	return State;
}

STextBrowser::STextBrowser() : TBaseBrowserWindow(WndClsName)
{
	P_SrDb = 0; // @v9.2.0
	SpcMode = spcmNo; // @v9.2.0
	Init(0);
}

STextBrowser::STextBrowser(const char * pFileName, int toolbarId) : TBaseBrowserWindow(WndClsName)
{
	P_SrDb = 0; // @v9.2.0
	SpcMode = spcmNo; // @v9.2.0
	Init(pFileName, toolbarId);
}

STextBrowser::~STextBrowser()
{
	if(::IsWindow(HwndSci)) {
		FileClose();
		if(OrgScintillaWndProc) {
			TView::SetWindowProp(HwndSci, GWL_WNDPROC, OrgScintillaWndProc);
			TView::SetWindowProp(HwndSci, GWL_USERDATA, (void *)0);
		}
		DestroyWindow(HwndSci);
	}
	P_Toolbar->DestroyHWND();
	ZDELETE(P_Toolbar);
	ZDELETE(P_SrDb); // @v9.2.0
}

int STextBrowser::SetSpecialMode(int spcm)
{
	int    ok = 1;
	if(spcm == spcmSartrTest) {
		if(!P_SrDb) {
            SString db_path;
			getExecPath(db_path);
			db_path.SetLastSlash().Cat("SARTRDB");
			THROW_S(P_SrDb = new SrDatabase(), SLERR_NOMEM);
			THROW(P_SrDb->Open(db_path));
		}
		SpcMode = spcm;
	}
	else {
		ZDELETE(P_SrDb);
		SpcMode = spcmNo;
	}
	CATCH
		SpcMode = spcmNo;
		ok = 0;
	ENDCATCH
	return ok;
}

//virtual
TBaseBrowserWindow::IdentBlock & STextBrowser::GetIdentBlock(TBaseBrowserWindow::IdentBlock & rBlk)
{
	rBlk.IdBias = IdBiasTextBrowser;
	rBlk.ClsName = STextBrowser::WndClsName; // @unicodeproblem
	(rBlk.InstanceIdent = Doc.FileName).Strip().ToLower();
	return rBlk;
}

int STextBrowser::Init(const char * pFileName, int toolbarId)
{
	OrgScintillaWndProc = 0;
	SysState = 0;
	Doc.Reset();
	Doc.FileName = pFileName;
	P_SciPtr     = 0;
	P_SciFn      = 0;
	BbState |= bbsWoScrollbars;
	P_Toolbar    = 0;
	ToolBarWidth = 0;
	if(toolbarId < 0)
		ToolbarId = TOOLBAR_TEXTBROWSER;
	else if(toolbarId > 0)
		ToolbarId = toolbarId;
	{
		KeyDownCommand k;
		k.SetTvKeyCode(kbF3);
		SetKeybAccelerator(k, PPVCMD_SEARCHNEXT);
	}
	return 1;
}

// static
LPCTSTR STextBrowser::WndClsName = _T("STextBrowser"); // @global

// static
int STextBrowser::RegisterClass(HINSTANCE hInst)
{
	WNDCLASSEX wc;
	wc.cbSize        = sizeof(wc);
	wc.style         = CS_HREDRAW | CS_VREDRAW | CS_OWNDC | CS_DBLCLKS;
	wc.lpfnWndProc   = STextBrowser::WndProc;
	wc.cbClsExtra    = BRWCLASS_CEXTRA;
	wc.cbWndExtra    = BRWCLASS_WEXTRA;
	wc.hInstance     = hInst;
	wc.hIcon         = LoadIcon(hInst, MAKEINTRESOURCE(/*ICON_TIMEGRID*/172));
	wc.hCursor       = NULL; // LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = ::CreateSolidBrush(RGB(0xEE, 0xEE, 0xEE));
	wc.lpszMenuName  = 0;
	wc.lpszClassName = STextBrowser::WndClsName;
	wc.hIconSm       = 0;
#if !defined(_PPDLL) && !defined(_PPSERVER)
	Scintilla_RegisterClasses(hInst);
#endif
	return RegisterClassEx(&wc); // @unicodeproblem
}

// static
LRESULT CALLBACK STextBrowser::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	CREATESTRUCT * p_init_data;
	STextBrowser * p_view = 0;
	switch(message) {
		case WM_CREATE:
			p_init_data = (CREATESTRUCT *)lParam;
			if(TWindow::IsMDIClientWindow(p_init_data->hwndParent)) {
				p_view = (STextBrowser *)((LPMDICREATESTRUCT)(p_init_data->lpCreateParams))->lParam;
				p_view->BbState |= bbsIsMDI;
			}
			else {
				p_view = (STextBrowser *)p_init_data->lpCreateParams;
				p_view->BbState &= ~bbsIsMDI;
			}
			if(p_view) {
				p_view->HW = hWnd;
				TView::SetWindowProp(hWnd, GWL_USERDATA, p_view);
				::SetFocus(hWnd);
				::SendMessage(hWnd, WM_NCACTIVATE, TRUE, 0L);
				p_view->WMHCreate();
				PostMessage(hWnd, WM_PAINT, 0, 0);
				{
					SString temp_buf;
					TView::SGetWindowText(hWnd, temp_buf); // @v9.1.5
					APPL->AddItemToMenu(temp_buf, p_view);
				}
				::SetFocus(p_view->HwndSci);
				return 0;
			}
			else
				return -1;
		case WM_COMMAND:
			{
				p_view = (STextBrowser *)TView::GetWindowUserData(hWnd);
				if(p_view) {
					if(HIWORD(wParam) == 0) {
						if(p_view->KeyAccel.getCount()) {
							long   cmd = 0;
							KeyDownCommand k;
							k.SetTvKeyCode(LOWORD(wParam));
							if(p_view->KeyAccel.Search(*(long *)&k, &cmd, 0, 1)) {
								p_view->ProcessCommand(cmd, 0, p_view);
							}
						}
					}
					/*
					if(LOWORD(wParam))
						p_view->ProcessCommand(LOWORD(wParam), 0, p_view);
					*/
				}
			}
			break;
		case WM_DESTROY:
			p_view = (STextBrowser *)TView::GetWindowUserData(hWnd);
			if(p_view) {
				p_view->SaveChanges();
				SETIFZ(p_view->EndModalCmd, cmCancel);
				APPL->DelItemFromMenu(p_view);
				p_view->ResetOwnerCurrent();
				if(!p_view->IsInState(sfModal)) {
					APPL->P_DeskTop->remove(p_view);
					delete p_view;
					TView::SetWindowProp(hWnd, GWL_USERDATA, (void *)0);
				}
			}
			return 0;
		case WM_SETFOCUS:
			if(!(TView::GetWindowStyle(hWnd) & WS_CAPTION)) {
				SetWindowPos(hWnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE);
				APPL->NotifyFrame(0);
			}
			p_view = (STextBrowser *)TView::GetWindowUserData(hWnd);
			if(p_view) {
				::SetFocus(p_view->HwndSci);
				APPL->SelectTabItem(p_view);
				TView::message(p_view, evBroadcast, cmReceivedFocus);
				p_view->select();
			}
			break;
		case WM_KILLFOCUS:
			if(!(TView::GetWindowStyle(hWnd) & WS_CAPTION))
				APPL->NotifyFrame(0);
			p_view = (STextBrowser *)TView::GetWindowUserData(hWnd);
			if(p_view) {
				TView::message(p_view, evBroadcast, cmReleasedFocus);
				p_view->ResetOwnerCurrent();
			}
			break;
		case WM_KEYDOWN:
			if(wParam == VK_ESCAPE) {
				p_view = (STextBrowser *)TView::GetWindowUserData(hWnd);
				if(p_view) {
					p_view->endModal(cmCancel);
					return 0;
				}
			}
			else if(wParam == VK_TAB) {
				p_view = (STextBrowser *)TView::GetWindowUserData(hWnd);
				if(p_view && GetKeyState(VK_CONTROL) & 0x8000 && !p_view->IsInState(sfModal)) {
					SetFocus(GetNextBrowser(hWnd, (GetKeyState(VK_SHIFT) & 0x8000) ? 0 : 1));
					return 0;
				}
			}
			return 0;
		case WM_SIZE:
			p_view = (STextBrowser *)TView::GetWindowUserData(hWnd);
			if(lParam && p_view) {
				HWND hw = p_view->P_Toolbar ? p_view->P_Toolbar->H() : 0;
				if(IsWindowVisible(hw)) {
					MoveWindow(hw, 0, 0, LOWORD(lParam), p_view->ToolBarWidth, 0);
					TView::message(p_view, evCommand, cmResize);
				}
				p_view->Resize();
			}
			break;
		case WM_NOTIFY:
			{
				//LPNMHDR lpnmhdr = (LPNMHDR)lParam;
				const SCNotification * p_scn = (const SCNotification *)lParam;
				p_view = (STextBrowser*)TView::GetWindowUserData(hWnd);
				if(p_view && p_scn->nmhdr.hwndFrom == p_view->GetSciWnd()) {
					int    test_value = 0; // @debug
					switch(p_scn->nmhdr.code) {
						case SCN_CHARADDED:
						case SCN_MODIFIED:
							p_view->Doc.SetState(stDirty, 1);
							break;
						case SCN_DWELLSTART:
							{
								test_value = 1;
								if(p_view->SpcMode == spcmSartrTest) {
									const char * p_wb = " \t.,;:()[]{}/\\!@#$%^&*+=<>\n\r\"\'?";
									const Sci_Position _start_pos = p_scn->position;
									IntArray left, right;
									Sci_Position _pos = _start_pos;
									int    c;
                                    while((c = p_view->CallFunc(SCI_GETCHARAT, _pos++, 0)) != 0) {
										if(!strchr(p_wb, (uchar)c)) {
											right.add(c);
										}
										else
											break;
                                    }
                                    if(_start_pos > 0) {
										_pos = _start_pos;
										while((c = p_view->CallFunc(SCI_GETCHARAT, --_pos, 0)) != 0) {
											if(!strchr(p_wb, (uchar)c)) {
												left.add(c);
											}
											else
												break;
										}
										left.reverse(0, left.getCount());
                                    }
                                    left.add(&right);
                                    //SCI_CALLTIPSHOW(int posStart, const char *definition)
                                    if(left.getCount()) {
                                    	SString src_text, text_to_show;
                                    	TSArray <SrWordInfo> info_list;
                                        for(uint ti = 0; ti < left.getCount(); ti++)
											src_text.CatChar((char)left.at(ti));
										if(p_view->P_SrDb->GetWordInfo(src_text, 0, info_list) > 0) {
											SString temp_buf;
											for(uint j = 0; j < info_list.getCount(); j++) {
												p_view->P_SrDb->WordInfoToStr(info_list.at(j), temp_buf);
												if(j)
													text_to_show.CR();
												text_to_show.Cat(temp_buf);
											}
										}
										if(text_to_show.Len())
											p_view->CallFunc(SCI_CALLTIPSHOW, _start_pos, (int)(const char *)text_to_show);
                                    }
								}
							}
							break;
						case SCN_DWELLEND:
							{
								p_view->CallFunc(SCI_CALLTIPCANCEL, 0, 0);
							}
							break;
					}
				}
			}
			break;
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}

int STextBrowser::CallFunc(int msg, int param1, int param2)
{
	return (P_SciFn && P_SciPtr) ? P_SciFn(P_SciPtr, msg, param1, param2) : 0;
}

int STextBrowser::Resize()
{
	if(HwndSci != 0) {
		RECT rc;
		GetWindowRect(H(), &rc);
		if(IsWindowVisible(APPL->H_ShortcutsWnd)) {
			RECT sh_rect;
			GetWindowRect(APPL->H_ShortcutsWnd, &sh_rect);
			rc.bottom -= sh_rect.bottom - sh_rect.top;
		}
		MoveWindow(HwndSci, 0, ToolBarWidth, rc.right - rc.left, rc.bottom - rc.top, 1);
	}
	return 1;
}

SKeyAccelerator::SKeyAccelerator() : LAssocArray()
{
}

int SKeyAccelerator::Set(KeyDownCommand & rK, int cmd)
{
	int    ok = 0;
	long   key = rK;
	long   val = 0;
	uint   pos = 0;
	if(Search(key, &val, &pos)) {
		if(cmd > 0) {
			if(cmd != val) {
				at(pos).Val = cmd;
				ok = 2;
			}
			else
				ok = -1;
		}
		else {
			atFree(pos);
			ok = 4;
		}
	}
	else {
		Add(key, cmd, 0);
		ok = 1;
	}
	return ok;
}

int STextBrowser::SetKeybAccelerator(KeyDownCommand & rK, int cmd)
{
	int    ok = OuterKeyAccel.Set(rK, cmd);
	KeyAccel.Set(rK, cmd);
	return ok;
}

int ImpLoadToolbar(TVRez & rez, ToolbarList * pList); // @prototype(wbrowse.cpp)

#if 0 // {
static int ImpLoadToolbar(TVRez & rez, ToolbarList * pList)
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
            PPExpandString(temp_buf);
            STRNSCPY(item.ToolTipText, temp_buf);
            // } @v9.0.11
		}
		else
			item.KeyCode = (ushort)item.Cmd;
		pList->addItem(&item);
	}
	return 1;
}
#endif // } 0

int SLAPI STextBrowser::LoadToolbar(uint tbId)
{
	int    r = 0;
	TVRez & rez = *P_SlRez;
	ToolbarList tb_list;
	r = rez.findResource(tbId, TV_EXPTOOLBAR, 0, 0) ? ImpLoadToolbar(rez, &tb_list) : 0;
	if(r > 0)
		setupToolbar(&tb_list);
	return r;
}

//static
LRESULT CALLBACK STextBrowser::ScintillaWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	STextBrowser * p_this = (STextBrowser *)::GetWindowLongPtr(hwnd, GWL_USERDATA);
	if(p_this) {
		switch(msg) {
			case WM_DESTROY:
				TView::SetWindowProp(p_this->HwndSci, GWL_WNDPROC, p_this->OrgScintillaWndProc);
				TView::SetWindowProp(p_this->HwndSci, GWL_USERDATA, (void *)0);
				return ::CallWindowProc(p_this->OrgScintillaWndProc, hwnd, msg, wParam, lParam);
			case WM_CHAR:
				if(p_this->SysState & p_this->sstLastKeyDownConsumed)
					return ::DefWindowProc(hwnd, msg, wParam, lParam);
				else
					return ::CallWindowProc(p_this->OrgScintillaWndProc, hwnd, msg, wParam, lParam);
			case WM_SYSKEYDOWN:
			case WM_KEYDOWN:
				{
					p_this->SysState &= ~p_this->sstLastKeyDownConsumed;
					int    processed = 0;
					KeyDownCommand k;
					k.SetWinMsgCode(wParam);
					if(k.Code == VK_TAB && k.State & k.stateCtrl) {
						SendMessage(p_this->H(), WM_KEYDOWN, wParam, lParam);
						p_this->SysState |= p_this->sstLastKeyDownConsumed;
						processed = 1;
					}
					else if(p_this->KeyAccel.getCount()) {
						long   cmd = 0;
						if(p_this->KeyAccel.Search(*(long *)&k, &cmd, 0, 1)) {
							p_this->SysState |= p_this->sstLastKeyDownConsumed;
							p_this->ProcessCommand(cmd, 0, p_this);
							processed = 1;
						}
					}
					return processed ? ::DefWindowProc(hwnd, msg, wParam, lParam) : ::CallWindowProc(p_this->OrgScintillaWndProc, hwnd, msg, wParam, lParam);
				}
				break;
			default:
				return ::CallWindowProc(p_this->OrgScintillaWndProc, hwnd, msg, wParam, lParam);
		}
	}
	else
		return ::DefWindowProc(hwnd, msg, wParam, lParam);
};

int STextBrowser::WMHCreate()
{
	RECT rc;
	GetWindowRect(H(), &rc);
	P_Toolbar = new TToolbar(H(), TBS_NOMOVE);
	if(P_Toolbar && LoadToolbar(ToolbarId) > 0) {
		P_Toolbar->Init(ToolbarID, &Toolbar);
		if(P_Toolbar->Valid()) {
			RECT tbr;
			::GetWindowRect(P_Toolbar->H(), &tbr);
			ToolBarWidth = tbr.bottom - tbr.top;
		}
	}
	HwndSci = ::CreateWindowEx(WS_EX_CLIENTEDGE, _T("Scintilla"), _T(""), WS_CHILD|WS_VISIBLE|WS_TABSTOP|WS_CLIPCHILDREN,
		0, ToolBarWidth, rc.right - rc.left, rc.bottom - rc.top, H(), 0/*(HMENU)GuiID*/, APPL->GetInst(), NULL);
	P_SciFn  = (int (__cdecl *)(void *, int, int, int))SendMessage(HwndSci, SCI_GETDIRECTFUNCTION, 0, 0);
	P_SciPtr = (void *)SendMessage(HwndSci, SCI_GETDIRECTPOINTER, 0, 0);

	TView::SetWindowProp(HwndSci, GWL_USERDATA, this);
	OrgScintillaWndProc = (WNDPROC)TView::SetWindowProp(HwndSci, GWL_WNDPROC, ScintillaWindowProc);
	// @v8.6.2 (SCI_SETKEYSUNICODE deprecated in sci 3.5.5) CallFunc(SCI_SETKEYSUNICODE, 1, 0);
	CallFunc(SCI_SETCARETLINEVISIBLE, 1, 0);
	CallFunc(SCI_SETCARETLINEBACK, RGB(232,232,255), 0);
	CallFunc(SCI_SETSELBACK, 1, RGB(117,217,117));
	CallFunc(SCI_SETFONTQUALITY, SC_EFF_QUALITY_ANTIALIASED, 0);
	//
	CallFunc(SCI_SETMOUSEDWELLTIME, 500, 0); // @v9.2.0
	//
	{
		KeyAccel.clear();
		{
			for(uint i = 0; i < OuterKeyAccel.getCount(); i++) {
				const LAssoc & r_accel_item = OuterKeyAccel.at(i);
				KeyDownCommand & r_k = *(KeyDownCommand *)&r_accel_item.Key;
				KeyAccel.Set(r_k, r_accel_item.Val);
			}
		}
		if(P_Toolbar) {
			const uint tbc = P_Toolbar->getItemsCount();
			for(uint i = 0; i < tbc; i++) {
				const ToolbarItem & r_tbi = P_Toolbar->getItem(i);
				if(!(r_tbi.Flags & r_tbi.fHidden) && r_tbi.KeyCode && r_tbi.KeyCode != TV_MENUSEPARATOR && r_tbi.Cmd) {
					KeyDownCommand k;
					if(k.SetTvKeyCode(r_tbi.KeyCode)) {
						KeyAccel.Set(k, r_tbi.Cmd);
					}
				}
			}
		}
		KeyAccel.Sort();
	}
	FileLoad(Doc.FileName, cpANSI, 0);
	return BIN(P_SciFn && P_SciPtr);
}

SCodepage STextBrowser::SelectEncoding(SCodepage initCp) const
{
	SCodepage result_cp = initCp;
	ListWindow * p_lw = CreateListWindow(16, lbtDisposeData | lbtDblClkNotify);
	if(p_lw) {
		SCodepage cp;
		SString cp_name;
		for(uint i = 0; i < SCodepageIdent::GetRegisteredCodepageCount(); i++) {
			if(SCodepageIdent::GetRegisteredCodepage(i, cp, cp_name = 0)) {
				p_lw->listBox()->addItem(cp, cp_name);
			}
		}
		p_lw->listBox()->TransmitData(+1, (long *)&result_cp);
		if(ExecView(p_lw) == cmOK) {
			p_lw->listBox()->TransmitData(-1, (long *)&result_cp);
		}
	}
	else {
		result_cp = cpUndef;
		PPError();
	}
	delete p_lw;
	return result_cp;
}

int STextBrowser::SetEncoding(SCodepage cp)
{
	int    ok = -1;
	if(cp == cpUndef) {
		cp = SelectEncoding(cp);
	}
	if(cp != cpUndef) {
		if(SaveChanges() > 0) {
			if(FileLoad(Doc.FileName, cp, 0))
				ok = 1;
		}
	}
	return ok;
}

int STextBrowser::InsertWorkbookLink()
{
	int    ok = -1;
	PPObjWorkbook wb_obj;
	PPObjWorkbook::SelectLinkBlock link;
	link.Type = PPWBTYP_MEDIA;
	if(wb_obj.SelectLink(&link) > 0 && link.ID) {
		WorkbookTbl::Rec rec, addendum_rec;
		if(wb_obj.Fetch(link.ID, &rec) > 0) {
			SString text;
			if(link.Type == link.ltImage) {
				(text = "#IMAGE").CatChar('(').CatChar('\'').Cat(rec.Symb).CatChar('\'').CatChar(')');
				text.Transf(CTRANSF_INNER_TO_UTF8);
				CallFunc(SCI_INSERTTEXT, -1, (int)(const char *)text);
			}
			else if(link.Type == link.ltRef) {
				(text = "#REF").CatChar('(').CatChar('\'').Cat(rec.Symb).CatChar('\'').CatChar(')');
				text.Transf(CTRANSF_INNER_TO_UTF8);
				CallFunc(SCI_INSERTTEXT, -1, (int)(const char *)text);
			}
			else if(link.Type == link.ltLink) {
				(text = "#LINK").CatChar('(').CatChar('\'').Cat(rec.Symb).CatChar('\'').CatChar(')');
				text.Transf(CTRANSF_INNER_TO_UTF8);
				CallFunc(SCI_INSERTTEXT, -1, (int)(const char *)text);
			}
			else if(link.Type == link.ltAnnot) {
				if(link.AddendumID && wb_obj.Fetch(link.AddendumID, &addendum_rec) > 0) {
					text = "#ANNOTIMG";
					(text = "#ANNOTIMG").CatChar('(').CatChar('\'').Cat(rec.Symb).CatChar('\'').CatDiv(',', 2).
						CatChar('\'').Cat(addendum_rec.Symb).CatChar('\'').CatChar(')');
				}
				else {
					(text = "#ANNOT").CatChar('(').CatChar('\'').Cat(rec.Symb).CatChar('\'').CatChar(')');
				}
				text.Transf(CTRANSF_INNER_TO_UTF8);
				CallFunc(SCI_INSERTTEXT, -1, (int)(const char *)text);
			}
		}
	}
	return ok;
}
//
//
//
class SearchReplaceDialog : public TDialog {
public:
	SearchReplaceDialog() : TDialog(DLG_SCISEARCH)
	{
	}
	int    setDTS(SSearchReplaceParam * pData)
	{
		Data = *pData;
		int    ok = 1;
		setCtrlString(CTL_SCISEARCH_PATTERN, Data.Pattern);
		AddClusterAssoc(CTL_SCISEARCH_RF, 0, SSearchReplaceParam::fReplace);
		SetClusterData(CTL_SCISEARCH_RF, Data.Flags);
		if(Data.Flags & SSearchReplaceParam::fReplace) {
			disableCtrl(CTL_SCISEARCH_REPLACE, 0);
			setCtrlString(CTL_SCISEARCH_REPLACE, Data.Replacer);
		}
		else {
			disableCtrl(CTL_SCISEARCH_REPLACE, 1);
		}
		AddClusterAssoc(CTL_SCISEARCH_FLAGS, 0, SSearchReplaceParam::fNoCase);
		AddClusterAssoc(CTL_SCISEARCH_FLAGS, 1, SSearchReplaceParam::fWholeWords);
		AddClusterAssoc(CTL_SCISEARCH_FLAGS, 2, SSearchReplaceParam::fReverse);
		SetClusterData(CTL_SCISEARCH_FLAGS, Data.Flags);
		return ok;
	}
	int    getDTS(SSearchReplaceParam * pData)
	{
		int    ok = 1;
		getCtrlString(CTL_SCISEARCH_PATTERN, Data.Pattern);
		getCtrlString(CTL_SCISEARCH_REPLACE, Data.Replacer);
		GetClusterData(CTL_SCISEARCH_RF, &Data.Flags);
		GetClusterData(CTL_SCISEARCH_FLAGS, &Data.Flags);
		ASSIGN_PTR(pData, Data);
		return ok;
	}
private:
	DECL_HANDLE_EVENT
	{
		TDialog::handleEvent(event);
		if(event.isClusterClk(CTL_SCISEARCH_RF)) {
			GetClusterData(CTL_SCISEARCH_RF, &Data.Flags);
			disableCtrl(CTL_SCISEARCH_REPLACE, !BIN(Data.Flags & SSearchReplaceParam::fReplace));
		}
		else
			return;
		clearEvent(event);
	}
	SSearchReplaceParam Data;
};

int SLAPI EditSearchReplaceParam(SSearchReplaceParam * pData)
{
	DIALOG_PROC_BODY(SearchReplaceDialog, pData);
}
//
//
//
struct TidyProcessBlock {
	TidyProcessBlock();

	long   Flags;
	SBaseBuffer InputBuffer;
	SString Output;
	StrAssocArray TidyOptions;
};

int TidyProcessText(TidyProcessBlock & rBlk);

#include <..\osf\tidy\include\tidy.h>

TidyProcessBlock::TidyProcessBlock()
{
	Flags = 0;
	InputBuffer.Init();
}

int TidyProcessText(TidyProcessBlock & rBlk)
{
	int    ok = 1, r;
	TidyDoc tdoc = tidyCreate();
	TidyBuffer input;
	TidyBuffer output;
	TidyBuffer errbuf;

	tidyBufInit(&input);
	tidyBufInit(&output);
	tidyBufInit(&errbuf);
	for(uint i = 0; i < rBlk.TidyOptions.getCount(); i++) {
		StrAssocArray::Item item = rBlk.TidyOptions.at_WithoutParent(i);
		tidyOptSetValue(tdoc, (TidyOptionId)item.Id, item.Txt);
	}
	r = tidySetErrorBuffer(tdoc, &errbuf);
	tidyBufAppend(&input, rBlk.InputBuffer.P_Buf, rBlk.InputBuffer.Size);

	r = tidyParseBuffer(tdoc, &input);
	r = tidyCleanAndRepair(tdoc);
	r = tidyRunDiagnostics(tdoc);
	r = tidyOptSetBool(tdoc, TidyForceOutput, true);
	r = tidySaveBuffer(tdoc, &output);
	rBlk.Output = 0;
	while(!tidyBufEndOfInput(&output)) {
		rBlk.Output.CatChar(tidyBufGetByte(&output));
	}
	tidyBufFree(&output);
	tidyBufFree(&errbuf);
	tidyRelease(tdoc);
	return ok;
}

int32 STextBrowser::GetCurrentPos()
{
	return CallFunc(SCI_GETCURRENTPOS, 0, 0);
}

int32 FASTCALL STextBrowser::SetCurrentPos(int32 pos)
{
	int32 prev = CallFunc(SCI_GETCURRENTPOS, 0, 0);
	CallFunc(SCI_SETCURRENTPOS, pos, 0);
	return prev;
}

int FASTCALL STextBrowser::GetSelection(IntRange & rR)
{
	rR.low = CallFunc(SCI_GETSELECTIONSTART, 0, 0);
	rR.upp = CallFunc(SCI_GETSELECTIONEND, 0, 0);
	return 1;
}

int FASTCALL STextBrowser::SetSelection(const IntRange * pR)
{
	int    ok = -1;
	if(!pR || pR->IsZero()) {
		CallFunc(SCI_SETEMPTYSELECTION, 0, 0);
		ok = -1;
	}
	else {
		CallFunc(SCI_SETSELECTIONSTART, pR->low, 0);
		CallFunc(SCI_SETSELECTIONEND, pR->upp, 0);
		ok = 1;
	}
	return ok;
}

int STextBrowser::SearchAndReplace(long flags)
{
	int    ok = -1;
	SSearchReplaceParam param = LastSrParam;
	if(!(flags & srfUseDialog) || EditSearchReplaceParam(&param) > 0) {
		LastSrParam = param;
		SString pattern = param.Pattern;
		pattern.Transf(CTRANSF_INNER_TO_UTF8);
		if(pattern.NotEmpty()) {
			int    sci_srch_flags = 0;
			int    _func = 0;
			IntRange sel;
			if(!(param.Flags & param.fNoCase))
				sci_srch_flags |= SCFIND_MATCHCASE;
			if(param.Flags & param.fWholeWords)
				sci_srch_flags |= SCFIND_WHOLEWORD;
			GetSelection(sel);
			const IntRange preserve_sel = sel;
			if(param.Flags & param.fReverse) {
				_func = SCI_SEARCHPREV;
			}
			else {
				_func = SCI_SEARCHNEXT;
				sel.low++;
				SetSelection(&sel);
			}
			CallFunc(SCI_SEARCHANCHOR, 0, 0);
			int    result = CallFunc(_func, sci_srch_flags, (int)(const char *)pattern);
			if(result >= 0) {
				ok = 1;
				int selend = CallFunc(SCI_GETSELECTIONEND, 0, 0);
				SetCurrentPos(selend);
				CallFunc(SCI_SCROLLCARET, 0, 0);
				IntRange sel;
				SetSelection(&sel.Set(result, selend));
				CallFunc(SCI_SEARCHANCHOR, 0, 0);
			}
			else {
				SetSelection(&preserve_sel);
			}
		}
	}
	return ok;
}

int STextBrowser::ProcessCommand(uint ppvCmd, const void * pHdr, void * pBrw)
{
	int    ok = -2;
	switch(ppvCmd) {
		case PPVCMD_OPEN:
			{
				SString file_name = Doc.FileName;
				if(PPOpenFile(PPTXT_TEXTBROWSER_FILETYPES, file_name, 0, H()) > 0)
					ok = FileLoad(file_name, cpANSI, 0);
			}
			break;
		case PPVCMD_SAVE:
			ok = FileSave(0, 0);
			break;
		case PPVCMD_SAVEAS:
			ok = FileSave(0, ofInteractiveSaveAs);
			break;
		case PPVCMD_SELCODEPAGE:
			ok = SetEncoding(cpUndef);
			break;
		case PPVCMD_PROCESSTEXT:
			{
				uint8 * p_buf = (uint8 *)CallFunc(SCI_GETCHARACTERPOINTER, 0, 0);
				const size_t len = (size_t)CallFunc(SCI_GETLENGTH, 0, 0);
				TidyProcessBlock blk;
				blk.InputBuffer.Set(p_buf, len);
				blk.TidyOptions.Add(TidyInCharEncoding, "utf8");
				blk.TidyOptions.Add(TidyOutCharEncoding, "utf8");
				blk.TidyOptions.Add(TidyWrapLen, "200");
				blk.TidyOptions.Add(TidyIndentContent, "yes");
				blk.TidyOptions.Add(TidyIndentSpaces, "4");
				blk.TidyOptions.Add(TidyBodyOnly, "yes");
				if(TidyProcessText(blk) > 0) {
					CallFunc(SCI_CLEARALL, 0, 0);
					CallFunc(SCI_APPENDTEXT, (int)blk.Output.Len(), (int)(const char*)blk.Output);
					ok = 1;
				}
			}
			break;
		case PPVCMD_SEARCH:
			SearchAndReplace(srfUseDialog);
			break;
		case PPVCMD_SEARCHNEXT:
			SearchAndReplace(0);
			break;
		case PPVCMD_INSERTLINK:
			InsertWorkbookLink();
			break;
		case PPVCMD_BRACEHTMLTAG:
			BraceHtmlTag();
			break;
	}
	return ok;
}

int STextBrowser::SaveChanges()
{
	int    ok = 1;
	if(Doc.State & stDirty) {
		if(CONFIRM(PPCFM_DATACHANGED))
			ok = FileSave(0, 0);
		else
			ok = -1;
	}
	return ok;
}

int STextBrowser::FileClose()
{
	int    ok = -1;
	if(Doc.SciDoc) {
		CallFunc(SCI_CLEARALL, 0, 0);
		CallFunc(SCI_RELEASEDOCUMENT, 0, (int)Doc.SciDoc);
		Doc.Reset();
		ok = 1;
	}
	return ok;
}

int STextBrowser::FileLoad(const char * pFileName, SCodepage orgCp, long flags)
{
	int    ok = 1;
	SString file_name;
	(file_name = pFileName).Strip();
	THROW_SL(fileExists(file_name));
	{
		size_t block_size = 8 * 1024 * 1024;
		int64  _fsize = 0;
		SFile _f(file_name, SFile::mRead|SFile::mBinary);
		THROW_SL(_f.IsValid());
		THROW_SL(_f.CalcSize(&_fsize));
		{
			const uint64 bufsize_req = _fsize + MIN(1<<20, _fsize/6);
			THROW(bufsize_req <= 1024*1024*1025);
			{
				Doc.SciDoc = (SciDocument)CallFunc(SCI_CREATEDOCUMENT, 0, 0);
				//Setup scratchtilla for new filedata
				CallFunc(SCI_SETSTATUS, SC_STATUS_OK, 0); // reset error status
				CallFunc(SCI_SETDOCPOINTER, 0, (int)Doc.SciDoc);
				const int ro = CallFunc(SCI_GETREADONLY, 0, 0);
				if(ro) {
					CallFunc(SCI_SETREADONLY, 0, 0);
				}
				CallFunc(SCI_CLEARALL, 0, 0);
				/*
					Здесь следует установить LEXER
				*/
				if(orgCp != cpANSI) {
					CallFunc(SCI_SETCODEPAGE, SC_CP_UTF8, 0);
				}
				CallFunc(SCI_ALLOCATE, (WPARAM)bufsize_req, 0);
				THROW(CallFunc(SCI_GETSTATUS, 0, 0) == SC_STATUS_OK);
				{
					int    first_block = 1;
					STextEncodingStat tes;
					SStringU ubuf;
					SString utfbuf;
					size_t incomplete_multibyte_char = 0;
					size_t actual_size = 0;
					int64  _fsize_rest = _fsize;
					STempBuffer buffer(block_size+8);
					THROW_SL(buffer.IsValid());
					while(_fsize_rest > 0) {
						actual_size = 0;
						THROW_SL(_f.Read(buffer+incomplete_multibyte_char, block_size-incomplete_multibyte_char, &actual_size));
						_fsize_rest -= actual_size;
						actual_size += incomplete_multibyte_char;
						if(first_block) {
							tes.Add(buffer, actual_size);
							if(tes.Flags & tes.fLegalUtf8Only) {
								if(_fsize_rest > 0) {
									//
									// Если все символы первого блока utf8, но проанализирован не весь
									// файл, то исходной кодовой страницей должна быть заданная из-вне (если определена).
									// Ибо, попытавшись установить utf8 как исходную страницу мы рискуем
									// исказить символы в следующих блоках файла.
									//
									Doc.OrgCp = (orgCp == cpUndef) ? cpUTF8 : orgCp;
								}
								else {
									//
									// Если мы проанализировали весь файл и все символы - utf8, то
									// можно смело устанавливать исходную страницу как utf8
									//
									Doc.OrgCp = cpUTF8;
								}
								Doc.Cp = cpUTF8;
							}
							else {
								Doc.OrgCp = (orgCp == cpUndef) ? cpANSI : orgCp;
								Doc.Cp = cpUTF8;
							}
							Doc.Eolf = tes.Eolf;
							CallFunc(SCI_SETCODEPAGE, SC_CP_UTF8, 0);
							{
								int    sci_eol = SC_EOL_CRLF;
								if(Doc.Eolf == eolWindows)
									sci_eol = SC_EOL_CRLF;
								else if(Doc.Eolf == eolUnix)
									sci_eol = SC_EOL_LF;
								else if(Doc.Eolf == eolMac)
									sci_eol = SC_EOL_CR;
								CallFunc(SCI_SETEOLMODE, sci_eol, 0);
							}
							first_block = 0;
						}
						if(Doc.OrgCp == cpUTF8) {
							// Pass through UTF-8 (this does not check validity of characters, thus inserting a multi-byte character in two halfs is working)
							CallFunc(SCI_APPENDTEXT, actual_size, (LPARAM)(const char *)buffer);
						}
						else {
							ubuf.CopyFromMb(Doc.OrgCp, buffer, actual_size);
							ubuf.CopyToUtf8(utfbuf, 0);
							CallFunc(SCI_APPENDTEXT, utfbuf.Len(), (LPARAM)(const char *)utfbuf);
						}
						THROW(CallFunc(SCI_GETSTATUS, 0, 0) == SC_STATUS_OK);
					}
				}
				CallFunc(SCI_EMPTYUNDOBUFFER, 0, 0);
				CallFunc(SCI_SETSAVEPOINT, 0, 0);
				if(ro) {
					CallFunc(SCI_SETREADONLY, 1, 0);
				}
				Doc.SetState(stDirty, 0);
				//CallFunc(SCI_SETDOCPOINTER, 0, _scratchDocDefault);
			}
		}
	}
	CATCH
		Doc.Reset();
		ok = 0;
	ENDCATCH
	return ok;
}

int STextBrowser::FileSave(const char * pFileName, long flags)
{
	int    ok = -1, skip = 0;
	SString path = isempty(pFileName) ? Doc.FileName : pFileName;
	if((flags & ofInteractiveSaveAs) || !path.NotEmptyS()) {
		if(PPOpenFile(PPTXT_TEXTBROWSER_FILETYPES, path, ofilfNExist, H()) > 0) {
			;
		}
		else
			skip = 1;
	}
	if(!skip) {
		const  uint8 * p_buf = (const uint8 *)CallFunc(SCI_GETCHARACTERPOINTER, 0, 0); // to get characters directly from Scintilla buffer;
		const  size_t len = (size_t)CallFunc(SCI_GETLENGTH, 0, 0);
		SFile file;
		THROW_SL(file.Open(path, SFile::mWrite|SFile::mBinary));
		if(Doc.OrgCp == Doc.Cp) {
			THROW_SL(file.Write(p_buf, len));
		}
		else if(Doc.Cp == cpUTF8) {
			SString temp_buf;
			temp_buf.CatN((char *)p_buf, len);
			temp_buf.Utf8ToCp(Doc.OrgCp);
			THROW_SL(file.Write(temp_buf, temp_buf.Len()));
		}
		else {
			THROW_SL(file.Write(p_buf, len));
		}
		Doc.FileName = path;
		Doc.SetState(stDirty, 0);
		ok = 1;
	}
	CATCHZOK
	return ok;
}

int STextBrowser::CmpFileName(const char * pFileName)
{
	return Doc.FileName.Cmp(pFileName, 1);
}

int STextBrowser::BraceHtmlTag()
{
	int    ok = -1;
	SString tag, text;
	TDialog * dlg = new TDialog(DLG_SELHTMLTAG);
	if(CheckDialogPtr(&dlg, 1)) {
		dlg->setCtrlString(CTL_SELHTMLTAG_TAGTEXT, tag);
		if(ExecView(dlg) == cmOK) {
			dlg->getCtrlString(CTL_SELHTMLTAG_TAGTEXT, tag);
			if(tag.NotEmptyS()) {
				IntRange sel_range;
				GetSelection(sel_range);
				if(sel_range.low >= 0 && sel_range.upp >= 0) {
					if(tag == "*") { // comment
						(text = 0).Cat("-->");
						CallFunc(SCI_INSERTTEXT, sel_range.upp, (int)(const char *)text);
						(text = 0).Cat("<!--");
						CallFunc(SCI_INSERTTEXT, sel_range.low, (int)(const char *)text);
					}
					else {
						(text = 0).CatChar('<').CatChar('/').Cat(tag).CatChar('>');
						CallFunc(SCI_INSERTTEXT, sel_range.upp, (int)(const char *)text);
						(text = 0).CatChar('<').Cat(tag).CatChar('>');
						CallFunc(SCI_INSERTTEXT, sel_range.low, (int)(const char *)text);
					}
					ok = 1;
				}
			}
		}
	}
	else
		ok = 0;
	delete dlg;
	return ok;
}
