// This file is part of Notepad++ project
// Copyright (C)2021 Don HO <don.h@free.fr>
// @licence GNU GPL
//
#include <npp-internal.h>
#pragma hdrstop

void GoToLineDlg::init(HINSTANCE hInst, HWND hPere, ScintillaEditView ** ppEditView) 
{
	Window::init(hInst, hPere);
	if(!ppEditView)
		throw std::runtime_error("GoToLineDlg::init : ppEditView is null.");
	_ppEditView = ppEditView;
}
	
/*virtual*/void GoToLineDlg::create(int dialogID, bool isRTL, bool msgDestParent)
{
	StaticDialog::create(dialogID, isRTL, msgDestParent);
}

void GoToLineDlg::doDialog(bool isRTL)
{
	if(!isCreated())
		create(IDD_GOLINE, isRTL);
	display();
}

/*virtual*/void GoToLineDlg::display(bool toShow) const 
{
	Window::display(toShow);
	if(toShow)
		::SetFocus(::GetDlgItem(_hSelf, ID_GOLINE_EDIT));
}

INT_PTR CALLBACK GoToLineDlg::run_dlgProc(UINT message, WPARAM wParam, LPARAM)
{
	switch(message) {
		case WM_INITDIALOG:
		    NppDarkMode::autoSubclassAndThemeChildControls(_hSelf);
		    ::SendDlgItemMessage(_hSelf, IDC_RADIO_GOTOLINE, BM_SETCHECK, TRUE, 0);
		    goToCenter();
		    return TRUE;
		case WM_CTLCOLOREDIT:
		    if(NppDarkMode::isEnabled()) {
			    return NppDarkMode::onCtlColorSofter(reinterpret_cast<HDC>(wParam));
		    }
		    break;
		case WM_CTLCOLORDLG:
		case WM_CTLCOLORSTATIC:
		    if(NppDarkMode::isEnabled()) {
			    return NppDarkMode::onCtlColorDarker(reinterpret_cast<HDC>(wParam));
		    }
		    break;
		case WM_PRINTCLIENT:
		    if(NppDarkMode::isEnabled()) {
			    return TRUE;
		    }
		    break;
		case WM_ERASEBKGND:
		    if(NppDarkMode::isEnabled()) {
			    RECT rc = { 0 };
			    getClientRect(rc);
			    ::FillRect(reinterpret_cast<HDC>(wParam), &rc, NppDarkMode::getDarkerBackgroundBrush());
			    return TRUE;
		    }
		    break;
		case NPPM_INTERNAL_REFRESHDARKMODE:
		    NppDarkMode::autoThemeChildControls(_hSelf);
		    return TRUE;
		case WM_COMMAND:
	    {
		    switch(wParam) {
				case IDCANCEL:      // Close
					display(false);
					cleanLineEdit();
					return TRUE;
			    case IDOK:
					{
						int line = getLine();
						if(line != -1) {
							display(false);
							cleanLineEdit();
							if(_mode == go2line) {
								(*_ppEditView)->execute(SCI_ENSUREVISIBLE, line-1);
								(*_ppEditView)->execute(SCI_GOTOLINE, line-1);
							}
							else {
								int posToGoto = 0;
								if(line > 0) {
									// make sure not jumping into the middle of a multibyte
									// character
									// or into the middle of a CR/LF pair for Windows files
									auto before = (*_ppEditView)->execute(SCI_POSITIONBEFORE, line);
									posToGoto = static_cast<int>((*_ppEditView)->execute(SCI_POSITIONAFTER, before));
								}
								auto sci_line = (*_ppEditView)->execute(SCI_LINEFROMPOSITION, posToGoto);
								(*_ppEditView)->execute(SCI_ENSUREVISIBLE, sci_line);
								(*_ppEditView)->execute(SCI_GOTOPOS, posToGoto);
							}
						}

						SCNotification notification = {};
						notification.nmhdr.code = SCN_PAINTED;
						notification.nmhdr.hwndFrom = _hSelf;
						notification.nmhdr.idFrom = ::GetDlgCtrlID(_hSelf);
						::SendMessage(_hParent, WM_NOTIFY, LINKTRIGGERED, reinterpret_cast<LPARAM>(&notification));
						(*_ppEditView)->getFocus();
						return TRUE;
					}
			    case IDC_RADIO_GOTOLINE:
			    case IDC_RADIO_GOTOOFFSET:
					{
						bool isLine, isOffset;
						if(wParam == IDC_RADIO_GOTOLINE) {
							isLine = true;
							isOffset = false;
							_mode = go2line;
						}
						else {
							isLine = false;
							isOffset = true;
							_mode = go2offsset;
						}
						::SendDlgItemMessage(_hSelf, IDC_RADIO_GOTOLINE, BM_SETCHECK, isLine, 0);
						::SendDlgItemMessage(_hSelf, IDC_RADIO_GOTOOFFSET, BM_SETCHECK, isOffset, 0);
						updateLinesNumbers();
						return TRUE;
					}
			    default:
					switch(HIWORD(wParam)) {
						case EN_SETFOCUS:
						case BN_SETFOCUS:
							updateLinesNumbers();
							return TRUE;
						default:
							return TRUE;
					}
					break;
		    }
	    }
		default:
		    return FALSE;
	}
	return FALSE;
}

void GoToLineDlg::updateLinesNumbers() const
{
	uint current = 0;
	uint limit = 0;
	if(_mode == go2line) {
		current = static_cast<uint>((*_ppEditView)->getCurrentLineNumber() + 1);
		limit = static_cast<uint>((*_ppEditView)->execute(SCI_GETLINECOUNT));
	}
	else {
		current = static_cast<uint>((*_ppEditView)->execute(SCI_GETCURRENTPOS));
		int currentDocLength = (*_ppEditView)->getCurrentDocLen();
		limit = static_cast<uint>(currentDocLength > 0 ? currentDocLength - 1 : 0);
	}
	::SetDlgItemInt(_hSelf, ID_CURRLINE, current, FALSE);
	::SetDlgItemInt(_hSelf, ID_LASTLINE, limit, FALSE);
}

int GoToLineDlg::getLine() const 
{
	BOOL isSuccessful;
	int line = ::GetDlgItemInt(_hSelf, ID_GOLINE_EDIT, &isSuccessful, FALSE);
	return (isSuccessful ? line : -1);
}
