// MSGBOX.CPP  Turbo Vision 1.0
// Copyright (c) 1991 by Borland International
//
#include <slib.h>
#include <tv.h>
#pragma hdrstop

static const char * P_Titles[] = {
	"warn", "error", "info", "confirm"
};

struct MsgBoxDlgFuncParam {
	const char * P_Msg;
	ushort Options;
};

static SString & FASTCALL ConvertMsgString(const char * pMsg, SString & rBuf)
{
	rBuf = pMsg;
	uint _p = 0;
	while(rBuf.StrChr('\003', &_p))
		rBuf.Excise(_p, 1);
	return rBuf.Transf(CTRANSF_INNER_TO_OUTER);
}

BOOL CALLBACK MessageBoxDialogFunc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	struct StdButton {
		const  char * P_Symb;
		ushort Cmd;
	};
	static const StdButton button_list[] = {
		{ "but_yes",    cmYes },
		{ "but_no",     cmNo  },
		{ "but_ok",     cmOK  },
		{ "but_cancel", cmCancel },
		{ "but_all",    cmaAll }
	};
	static const int8 button_n[4][4] = {
		{6,0,0,0},
		{2,3,0,0},
		{5,6,7,0},
		{1,2,3,4}
	};
	int    ret = FALSE;
	int    i, j;
	SString temp_buf;
	MsgBoxDlgFuncParam * p_param = (MsgBoxDlgFuncParam *)lParam;
	switch(uMsg) {
		case WM_INITDIALOG:
			{
				int    button_count;
				SString title_buf;
				if(p_param && p_param->P_Msg) {
					// @v9.1.5 char   buf[1024];
					ConvertMsgString(p_param->P_Msg, title_buf);
					// @v9.1.5 ::SendDlgItemMessage(hwndDlg, CTL_WMSGBOX_TEXT, WM_SETTEXT, 0, (LPARAM)buf);
					TView::SSetWindowText(GetDlgItem(hwndDlg, CTL_WMSGBOX_TEXT), title_buf); // @v9.1.5
				}
				uint   msg_idx = (p_param->Options & 0x03);
				if(msg_idx >= SIZEOFARRAY(P_Titles))
					msg_idx = 2; // "info"
				SLS.LoadString(P_Titles[msg_idx], title_buf);
				// @v9.1.5 ::SendMessage(hwndDlg, WM_SETTEXT, 0, (LPARAM)(const char *)title_buf.Transf(CTRANSF_INNER_TO_OUTER));
				TView::SSetWindowText(hwndDlg, title_buf.Transf(CTRANSF_INNER_TO_OUTER)); // @v9.1.5
				for(i = 0, button_count = 0; i < SIZEOFARRAY(button_list); i++)
					if(p_param->Options & (0x0100 << i))
						button_count++;
				for(i = 0, j = 0; i < SIZEOFARRAY(button_list); i++)
					if(p_param->Options & (0x0100 << i)) {
						int    id = CTL_WMSGBOX_FIRSTBUTTON + button_n[button_count-1][j++] - 1;
						HWND   w_ctl = GetDlgItem(hwndDlg, id);
						if(SLS.LoadString(button_list[i].P_Symb, title_buf) > 0) {
							// @v9.1.5 ::SendMessage(w_ctl, WM_SETTEXT, 0, (LPARAM)(const char *)title_buf.Transf(CTRANSF_INNER_TO_OUTER));
							TView::SSetWindowText(w_ctl, title_buf.Transf(CTRANSF_INNER_TO_OUTER)); // @v9.1.5
							long   wl = TView::GetWindowStyle(w_ctl);
							TView::SetWindowProp(w_ctl, GWL_STYLE, wl|WS_VISIBLE);
						}
					}
				if((p_param->Options & mfConf)  == mfConf)
					TDialog::centerDlg(hwndDlg);
			}
			ret = TRUE;
			break;
		case WM_COMMAND:
			if(LOWORD(wParam) == 2 && HIWORD(wParam) == 0)
				EndDialog(hwndDlg, cmCancel);
			else if(HIWORD(wParam) == BN_CLICKED) {
				SString title_buf;
				// @v9.1.5 SendDlgItemMessage(hwndDlg, LOWORD(wParam), WM_GETTEXT, sizeof(buf)-1, (long)buf);
				TView::SGetWindowText(::GetDlgItem(hwndDlg, LOWORD(wParam)), temp_buf); // @v9.1.5
				for(i = 0; i < SIZEOFARRAY(button_list); i++) {
					if(SLS.LoadString(button_list[i].P_Symb, title_buf) > 0) {
						if(title_buf.Transf(CTRANSF_INNER_TO_OUTER) == temp_buf) {
							EndDialog(hwndDlg, button_list[i].Cmd);
							break;
						}
					}
				}
			}
			break;
	}
	return ret;
}

ushort messageBoxRect(const TRect & r, const char * msg, ushort aOptions)
{
	ushort ret = cmCancel;
	//char   buf[1024];
	uint   msg_flags = 0;
	SString title_buf;
	SString temp_buf;
	HWND   hw_parent = APPL->H_TopOfStack;
	if(!(aOptions & mfAll) && ((aOptions & 0xf) == mfInfo)) {
		msg_flags |= (MB_OK | MB_ICONINFORMATION | MB_TASKMODAL);
		SLS.LoadString(P_Titles[2], title_buf);
		MessageBox(hw_parent, ConvertMsgString(msg, temp_buf), title_buf.Transf(CTRANSF_INNER_TO_OUTER), msg_flags);
		SetForegroundWindow(hw_parent);
		ret = cmOK;
	}
	else if((aOptions & (mfConf | mfLargeBox)) == mfConf) {
		// аналогично else if((aOptions & mfConf) == mfConf && !(aOptions & mfLargeBox)) {
		msg_flags = (MB_ICONEXCLAMATION | MB_TASKMODAL);
		if((aOptions & mfYesNoCancel) == mfYesNoCancel)
			msg_flags |= MB_YESNOCANCEL;
		else
			msg_flags |= MB_YESNO;
		if(aOptions & mfDefaultYes)
			msg_flags |= MB_DEFBUTTON1;
		else
			msg_flags |= MB_DEFBUTTON2;
		SLS.LoadString(P_Titles[3], title_buf);
		int    ok = MessageBox(hw_parent, ConvertMsgString(msg, temp_buf), title_buf.Transf(CTRANSF_INNER_TO_OUTER), msg_flags);
		SetForegroundWindow(hw_parent);
		ret = (ok == IDYES) ? cmYes : ((ok == IDCANCEL) ? cmCancel : cmNo);
	}
	else {
		MsgBoxDlgFuncParam msg_param;
		msg_param.P_Msg = msg;
		msg_param.Options = aOptions;
		ret = APPL->DlgBoxParam((aOptions & mfLargeBox) ? DLGW_MSGBOX_L : DLGW_MSGBOX, hw_parent, (DLGPROC)MessageBoxDialogFunc, (long)&msg_param);
	}
	return ret;
}

/* @v9.3.4 ushort messageBoxRect(const TRect & r, ushort aOptions, const char *fmt, ...)
{
	va_list argptr;
	va_start(argptr, fmt);
	char msg[1024];
	vsprintf(msg, fmt, argptr);
	return messageBoxRect(r, msg, aOptions);
}*/

/* @v9.3.4 ushort messageBox(ushort aOptions, const char *fmt,...)
{
	va_list argptr;
	va_start(argptr, fmt);
	char msg[1024];
	vsprintf(msg, fmt, argptr);
	return messageBoxRect(APPL->MakeCenterRect(60, 10), msg, aOptions);
}*/

ushort messageBox(const char *msg, ushort aOptions)
{
	return messageBoxRect(APPL->MakeCenterRect(60, 10), msg, aOptions);
}

