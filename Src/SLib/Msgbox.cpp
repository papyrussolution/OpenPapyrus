// MSGBOX.CPP
// Copyright (c) A.Sobolev 1996-2025
// @codepage UTF-8
//
#include <slib-internal.h>
#pragma hdrstop

static const char * P_Titles[] = { "warn", "error", "info", "confirm" };

struct MsgBoxDlgFuncParam {
	const char * P_Msg;
	ushort Options;
};

static SString & FASTCALL ConvertMsgString(const char * pMsg, SString & rBuf)
{
	rBuf = pMsg;
	size_t _p = 0;
	while(rBuf.SearchChar('\003', &_p))
		rBuf.Excise(_p, 1);
	return rBuf.Transf(CTRANSF_INNER_TO_OUTER);
}

BOOL CALLBACK MessageBoxDialogFunc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	/*struct StdButton {
		const  char * P_Symb;
		ushort Cmd;
	};*/
	// @v12.3.3 static const StdButton button_list[] = { { "but_yes", cmYes }, { "but_no", cmNo  }, { "but_ok", cmOK  }, { "but_cancel", cmCancel }, { "but_all", cmaAll } };
	// @v12.3.3 {
	static const SIntToSymbTabEntry button_list[] = {
		{ cmYes, "but_yes"}, 
		{ cmNo, "but_no" }, 
		{ cmOK, "but_ok" }, 
		{ cmCancel, "but_cancel" }, 
		{ cmaAll, "but_all" }
	};
	// } @v12.3.3 
	static const int8 button_n[4][4] = { {6,0,0,0}, {2,3,0,0}, {5,6,7,0}, {1,2,3,4} };
	int    ret = FALSE;
	int    i;
	int    j;
	SString temp_buf;
	MsgBoxDlgFuncParam * p_param = reinterpret_cast<MsgBoxDlgFuncParam *>(lParam);
	switch(uMsg) {
		case WM_INITDIALOG:
			if(p_param) {
				SString title_buf;
				if(p_param->P_Msg) {
					ConvertMsgString(p_param->P_Msg, title_buf);
					TView::SSetWindowText(GetDlgItem(hwndDlg, CTL_WMSGBOX_TEXT), title_buf);
				}
				uint   msg_idx = (p_param->Options & 0x03);
				if(msg_idx >= SIZEOFARRAY(P_Titles))
					msg_idx = 2; // "info"
				SLS.LoadString_(P_Titles[msg_idx], title_buf);
				TView::SSetWindowText(hwndDlg, title_buf.Transf(CTRANSF_INNER_TO_OUTER));
				HWND   h_prev_focus = 0;
				uint   button_count = 0;
				for(i = 0; i < SIZEOFARRAY(button_list); i++) {
					if(p_param->Options & (0x0100 << i))
						button_count++;
				}
				if(button_count > 0) {
					for(i = 0, j = 0; i < SIZEOFARRAY(button_list); i++) {
						if(p_param->Options & (0x0100 << i)) {
							int    id = CTL_WMSGBOX_FIRSTBUTTON + button_n[button_count-1][j++] - 1;
							HWND   w_ctl = GetDlgItem(hwndDlg, id);
							if(SLS.LoadString_(button_list[i].P_Symb, title_buf) > 0) {
								TView::SSetWindowText(w_ctl, title_buf.Transf(CTRANSF_INNER_TO_OUTER));
								const long wl = TView::SGetWindowStyle(w_ctl);
								TView::SetWindowProp(w_ctl, GWL_STYLE, reinterpret_cast<void *>(wl|WS_VISIBLE));
							}
						}
					}
				}
				if((p_param->Options & mfConf) == mfConf)
					TDialog::centerDlg(hwndDlg);
				ret = (p_param->Options & mfNoFocus) ? FALSE : TRUE;
			}
			break;
		case WM_COMMAND:
			if(LOWORD(wParam) == 2 && HIWORD(wParam) == 0)
				EndDialog(hwndDlg, cmCancel);
			else if(HIWORD(wParam) == BN_CLICKED) {
				SString title_buf;
				TView::SGetWindowText(::GetDlgItem(hwndDlg, LOWORD(wParam)), temp_buf);
				for(i = 0; i < SIZEOFARRAY(button_list); i++) {
					if(SLS.LoadString_(button_list[i].P_Symb, title_buf) > 0) {
						if(title_buf.Transf(CTRANSF_INNER_TO_OUTER) == temp_buf) {
							EndDialog(hwndDlg, button_list[i].Id);
							break;
						}
					}
				}
			}
			break;
	}
	return ret;
}

ushort SMessageBox(const char * pMsg, ushort options)
{
	ushort ret = cmCancel;
	uint   msg_flags = 0;
	SString title_buf;
	SString temp_buf;
	HWND   hw_parent = APPL->H_TopOfStack;
	if(!(options & mfAll) && ((options & 0xf) == mfInfo)) {
		msg_flags |= (MB_OK | MB_ICONINFORMATION | MB_TASKMODAL);
		SLS.LoadString_(P_Titles[2], title_buf);
		::MessageBox(hw_parent, SUcSwitch(ConvertMsgString(pMsg, temp_buf)), SUcSwitch(title_buf.Transf(CTRANSF_INNER_TO_OUTER)), msg_flags); // @unicodeproblem
		SetForegroundWindow(hw_parent);
		ret = cmOK;
	}
	else if((options & (mfConf | mfLargeBox)) == mfConf) {
		// аналогично else if((aOptions & mfConf) == mfConf && !(aOptions & mfLargeBox)) {
		msg_flags = (MB_ICONEXCLAMATION | MB_TASKMODAL);
		if((options & mfYesNoCancel) == mfYesNoCancel)
			msg_flags |= MB_YESNOCANCEL;
		else
			msg_flags |= MB_YESNO;
		if(options & mfDefaultYes)
			msg_flags |= MB_DEFBUTTON1;
		else
			msg_flags |= MB_DEFBUTTON2;
		SLS.LoadString_(P_Titles[3], title_buf);
		int    ok = ::MessageBox(hw_parent, SUcSwitch(ConvertMsgString(pMsg, temp_buf)), SUcSwitch(title_buf.Transf(CTRANSF_INNER_TO_OUTER)), msg_flags); // @unicodeproblem
		::SetForegroundWindow(hw_parent);
		ret = (ok == IDYES) ? cmYes : ((ok == IDCANCEL) ? cmCancel : cmNo);
	}
	else {
		MsgBoxDlgFuncParam msg_param;
		msg_param.P_Msg = pMsg;
		msg_param.Options = options;
		ret = APPL->DlgBoxParam((options & mfLargeBox) ? DLGW_MSGBOX_L : DLGW_MSGBOX, hw_parent, 
			reinterpret_cast<DLGPROC>(MessageBoxDialogFunc), reinterpret_cast<LPARAM>(&msg_param));
	}
	return ret;
}
