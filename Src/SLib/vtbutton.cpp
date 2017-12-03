// VTBUTTON.CPP
// Copyright (c) V.Nasonov 2002, 2003, 2005, 2006, 2007, 2010, 2011, 2014, 2015, 2016, 2017
//
#include <slib.h>
#include <tv.h>
#pragma hdrstop
#include <pp.h>
//
// @v8.3.3 Удалены условия __WIN32__
//
// Prototype
int SLAPI showInputLineCalc(TDialog *, uint);

#pragma warn -par

VirtButtonWndEx::VirtButtonWndEx(const char * pSignature) : P_Dlg(0), FieldCtrlId(0), ButtonCtrlId(0), PrevWndProc(0), HBmp(0)
{
	STRNSCPY(Signature, pSignature);
}

TCalcInputLine::TCalcInputLine(uint inputId, uint buttonId, TRect & bounds, TYPEID aType, long fmt) :
	TInputLine(bounds, aType, fmt), Vbwe("papyruscalculator"), VirtButtonId(inputId)
{
	Vbwe.ButtonCtrlId = buttonId;
}

#pragma warn .par

TCalcInputLine::~TCalcInputLine()
{
	TView::SetWindowProp(GetDlgItem(Parent, Vbwe.ButtonCtrlId), GWLP_WNDPROC, Vbwe.PrevWndProc);
	ZDeleteWinGdiObject(&Vbwe.HBmp);
}

IMPL_HANDLE_EVENT(TCalcInputLine)
{
	TInputLine::handleEvent(event);
	if(TVBROADCAST && TVCMD == cmSearchVirtButton && TVINFOVIEW == this) {
		event.what = TEvent::evNothing;
		event.message.infoPtr = (void *)Vbwe.ButtonCtrlId;
	}
}

LRESULT CALLBACK InLnCalcWindProc(HWND, UINT, WPARAM, LPARAM);

int TCalcInputLine::handleWindowsMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LRESULT (CALLBACK *virtButtonProc[])(HWND, UINT, WPARAM, LPARAM) = { 0, InLnCalcWindProc, InLnCalcWindProc };
	const int virtButtonBitmapId[] = { 0, IDB_INLNCALC, IDB_INLNCALCL };
	int    ok = TInputLine::handleWindowsMessage(uMsg, wParam, lParam);
	if(ok > 0) {
		if(uMsg == WM_INITDIALOG) {
			Vbwe.P_Dlg = (TDialog *)owner;
			Vbwe.FieldCtrlId = Id;
			HWND hwnd = GetDlgItem(Parent, Vbwe.ButtonCtrlId);
			Vbwe.PrevWndProc = (WNDPROC)TView::GetWindowProp(hwnd, GWLP_WNDPROC);
			TView::SetWindowUserData(hwnd, &Vbwe);
			const uint bmp_id = virtButtonBitmapId[VirtButtonId];
			TView::SetWindowProp(hwnd, GWLP_WNDPROC, virtButtonProc[VirtButtonId]);
			Vbwe.HBmp = APPL->LoadBitmap(bmp_id);
			SendDlgItemMessage(Parent, Vbwe.ButtonCtrlId, BM_SETIMAGE, IMAGE_BITMAP, (long)Vbwe.HBmp);
			{
				TButton * p_button = (TButton *)Vbwe.P_Dlg->getCtrlView(Vbwe.ButtonCtrlId);
				if(p_button && p_button->IsSubSign(TV_SUBSIGN_BUTTON))
					p_button->SetBitmap(bmp_id);
			}
		}
	}
	return ok;
}

class InputLineCalc : public TDialog {
public:
	InputLineCalc(uint dlgID, TDialog *pParentDlg, uint fieldCtlId);
	~InputLineCalc()
	{
		InLnCalcUncapture();
		ShowNumber();
	}
private:
	DECL_HANDLE_EVENT;
	void   InLnCalcCapture();                  // Win32
	void   InLnCalcUncapture();                // Win32
	int    IsWindowArea(RECT &, int x, int y); // Win32
	void   ShowNumber();
	int    Calculate();
	void   ProcessCommand(uint cmd);
	uint   TranslateCharToCmd(int chr);

	TDialog * P_ParentDlg;
	uint	CtlId;
	TInputLine *P_Il;
	char	NumSym[20];
	double	Number[2];
	char	CharNumber[256];
	char	InLnNumber[256];
	int     MaxLen;
	TYPEID	NType;
	long	NFormat;
	int     IsNumber;
	int     IsDot;
	int     Command;
	HWND	PrevCaptureWnd;
	RECT	DlgRect;
	RECT	ItemRect;
};

#define ILC_NO_COMMAND  0
#define ILC_PLUS        (CTL_INLNCALC_PLUS - CTL_INLNCALC_CLEAR)
#define ILC_MINUS       (CTL_INLNCALC_MINUS - CTL_INLNCALC_CLEAR)
#define ILC_MULTIPLE    (CTL_INLNCALC_MULT - CTL_INLNCALC_CLEAR)
#define ILC_DIVIDE      (CTL_INLNCALC_DIVD - CTL_INLNCALC_CLEAR)
#define ILC_EQVAL       (CTL_INLNCALC_EQV - CTL_INLNCALC_CLEAR)
#define ILC_INVERSE     (CTL_INLNCALC_INVERSE - CTL_INLNCALC_CLEAR)

InputLineCalc::InputLineCalc(uint dlgID, TDialog *pParentDlg, uint fieldCtlId) : TDialog(dlgID), P_ParentDlg(pParentDlg), CtlId(fieldCtlId)
{
	NumSym[0] = '0';
	NumSym[abs(CTL_INLNCALC_1 - CTL_INLNCALC_0)] = '1';
	NumSym[abs(CTL_INLNCALC_2 - CTL_INLNCALC_0)] = '2';
	NumSym[abs(CTL_INLNCALC_3 - CTL_INLNCALC_0)] = '3';
	NumSym[abs(CTL_INLNCALC_4 - CTL_INLNCALC_0)] = '4';
	NumSym[abs(CTL_INLNCALC_5 - CTL_INLNCALC_0)] = '5';
	NumSym[abs(CTL_INLNCALC_6 - CTL_INLNCALC_0)] = '6';
	NumSym[abs(CTL_INLNCALC_7 - CTL_INLNCALC_0)] = '7';
	NumSym[abs(CTL_INLNCALC_8 - CTL_INLNCALC_0)] = '8';
	NumSym[abs(CTL_INLNCALC_9 - CTL_INLNCALC_0)] = '9';
	DlgFlags &= ~fCentered;
	if(GetWindowRect(H(), &DlgRect) && GetWindowRect(GetDlgItem(P_ParentDlg->H(), CtlId), &ItemRect)) {
		int sizex = DlgRect.right - DlgRect.left;
		int sizey = DlgRect.bottom - DlgRect.top;
		int sx = GetSystemMetrics(SM_CXFULLSCREEN);
		int sy = GetSystemMetrics(SM_CYFULLSCREEN);
		int x;
		int y = ItemRect.bottom;
		if(y + sizey > sy)
			y = ItemRect.top - sizey;
		if(y < 0) {
			y = sy - sizey;
			x = ((ItemRect.right + sizex) > sx) ? (ItemRect.left - sizex) : ItemRect.right;
		}
		else {
			x = (sizex > (ItemRect.right - ItemRect.left)) ? ItemRect.left : (ItemRect.right - sizex);
			if((x + sizex) > sx)
				x = sx - sizex;
		}
		if(x < 0)
			x = 0;
		MoveWindow(H(), x, y, sizex, sizey, TRUE);
		GetWindowRect(H(), &DlgRect);
	}
	P_ParentDlg->getCtrlData(CtlId, InLnNumber);
	P_Il = (TInputLine *)P_ParentDlg->getCtrlView(CtlId);
	const long ln_fmt = P_Il->getFormat();
	MaxLen  = SFMTLEN(ln_fmt);
	NType   = MKSTYPE(S_FLOAT, 8);
	NFormat = MKSFMTD(0, SFMTPRC(ln_fmt), ((SFMTFLAG(ln_fmt) | NMBF_NOTRAILZ) & ~SFALIGNMASK));
	sttostr(P_Il->getType(), InLnNumber, ln_fmt, CharNumber);
	stfromstr(NType, &Number[0], NFormat, CharNumber);
	//
	origin.x = P_ParentDlg->origin.x + P_Il->origin.x;
	if(size.x < P_Il->size.x)
		origin.x += P_Il->size.x - size.x;
	if(origin.x + size.x > 80)
		origin.x = 80 - size.x;
	origin.y = P_ParentDlg->origin.y + P_Il->origin.y + P_Il->size.y;
	if(origin.y + size.y > 23)
		origin.y = P_ParentDlg->origin.y + P_Il->origin.y - size.y -1;
	//
	IsNumber = 0;
	IsDot = 0;
	Command = ILC_NO_COMMAND;
	InLnCalcCapture();
}

void InputLineCalc::InLnCalcCapture()
{
	if(GetCapture() != H())
		PrevCaptureWnd = SetCapture(H());
}

void InputLineCalc::InLnCalcUncapture()
{
	if(PrevCaptureWnd)
		SetCapture(PrevCaptureWnd);
	else
		ReleaseCapture();
}

int InputLineCalc::IsWindowArea(RECT & r, int x, int y)
{
	return (x >= r.left && x < r.right && y >= r.top && y < r.bottom);
}

void InputLineCalc::ShowNumber()
{
	int    i = (Command && IsNumber) ? 1 : 0;
	if(Command || IsNumber)
		stfromstr(NType, &Number[i], NFormat, CharNumber);
	sttostr(NType, &Number[i], NFormat, CharNumber);
	if(IsNumber)
		IsNumber = strlen(CharNumber);
	stfromstr(P_Il->getType(), InLnNumber, P_Il->getFormat(), CharNumber);
	P_ParentDlg->setCtrlData(CtlId, InLnNumber);
}

int InputLineCalc::Calculate()
{
	int    ok = 1;
	if(IsNumber == 0)
		Number[1] = Number[0];
	switch(Command) {
		case ILC_PLUS:
			Number[0] += Number[1];
			break;
		case ILC_MINUS:
			Number[0] -= Number[1];
			break;
		case ILC_MULTIPLE:
			Number[0] *= Number[1];
			break;
		case ILC_DIVIDE:
			if(Number[1])
				Number[0] /= Number[1];
			else {
				Number[0] = 0;
				P_Il->setText("Error");
				ok = 0;
			}
			break;
		case ILC_INVERSE:
			Number[0] = -Number[1];
			break;
	}
	Number[1] = 0;
	Command = ILC_NO_COMMAND;
	IsNumber = 0;
	return ok;
}

uint InputLineCalc::TranslateCharToCmd(int chr)
{
	if(isdigit(chr))
		return (CTL_INLNCALC_0 + chr - '0');
	else if(chr == '.')
		return CTL_INLNCALC_DOT;
	else if(oneof4(chr, 'C', 'c', 'Ñ', 'ñ'))
		return CTL_INLNCALC_CLEAR;
	else if(oneof4(chr, 'E', 'e', 'Å', 'å'))
		return CTL_INLNCALC_CLEAR_ENTER;
	else if(oneof4(chr, 'I', 'i', 'È', 'è'))
		return CTL_INLNCALC_INVERSE;
	else if(chr == '+')
		return CTL_INLNCALC_PLUS;
	else if(chr == '-')
		return CTL_INLNCALC_MINUS;
	else if(chr == '*')
		return CTL_INLNCALC_MULT;
	else if(chr == '/')
		return CTL_INLNCALC_DIVD;
	else if(chr == '=')
		return CTL_INLNCALC_EQV;
	else 
		return 0;
}

void InputLineCalc::ProcessCommand(uint cmd)
{
	switch(cmd) {
		case CTL_INLNCALC_0:
		case CTL_INLNCALC_1:
		case CTL_INLNCALC_2:
		case CTL_INLNCALC_3:
		case CTL_INLNCALC_4:
		case CTL_INLNCALC_5:
		case CTL_INLNCALC_6:
		case CTL_INLNCALC_7:
		case CTL_INLNCALC_8:
		case CTL_INLNCALC_9:
			if(IsNumber == 1 && CharNumber[0] == '0')
				IsNumber = 0;
			if(IsNumber < MaxLen) {
				CharNumber[IsNumber++] = NumSym[abs((int)cmd - CTL_INLNCALC_0)];
				CharNumber[IsNumber] = 0;
				P_Il->setText(CharNumber);
			}
			break;
		case CTL_INLNCALC_DOT:
			if(IsDot == 0 && IsNumber < MaxLen) {
				if(IsNumber == 0)
					CharNumber[IsNumber++] = '0';
				CharNumber[IsNumber++] = '.';
				CharNumber[IsNumber] = 0;
				IsDot = 1;
				P_Il->setText(CharNumber);
			}
			break;
		case CTL_INLNCALC_CLEAR:
			Command = ILC_NO_COMMAND;
		case CTL_INLNCALC_CLEAR_ENTER:
			if(Command && IsNumber == 0)
				Command = ILC_NO_COMMAND;
			else {
				CharNumber[0] = '0';
				CharNumber[1] = 0;
				IsNumber = 1;
				IsDot = 0;
				P_Il->setText(CharNumber);
			}
			break;
		case CTL_INLNCALC_INVERSE:
			Command = ILC_INVERSE;
			Calculate();
			ShowNumber();
			break;
		case CTL_INLNCALC_PLUS:
		case CTL_INLNCALC_MINUS:
		case CTL_INLNCALC_MULT:
		case CTL_INLNCALC_DIVD:
		case CTL_INLNCALC_EQV:
			if(Command) {
				ShowNumber();
				if(Calculate() == 0)
					break;
			}
			ShowNumber();
			IsNumber = 0;
			IsDot = 0;
			Command = cmd - CTL_INLNCALC_CLEAR;
			if(Command == ILC_EQVAL)
				Command = ILC_NO_COMMAND;
			if(cmd == CTL_INLNCALC_EQV) {
				if(P_Il && P_ParentDlg)
					TView::messageBroadcast(P_ParentDlg, cmCommitInput, P_Il);
				TView::messageCommand(this, cmOK);
			}
			break;
	}
}

IMPL_HANDLE_EVENT(InputLineCalc)
{
	TDialog::handleEvent(event);
	switch(event.what) {
		case TEvent::evCommand:
			if(TVCMD == cmValid) {
				InLnCalcCapture();
				ProcessCommand(TVINFOVIEW->GetId());
			}
			else
				return;
			break;
		case TEvent::evKeyDown:
			ProcessCommand(TranslateCharToCmd(TVCHR));
			break;
		case TEvent::evMouseDown:
			if(event.mouse.buttons == MK_LBUTTON) {
				int    x = event.mouse.WhereX + DlgRect.left + 3;
				int    y = event.mouse.WhereY + DlgRect.top + 3;
				if(IsWindowArea(DlgRect, x, y)) {
					TView * v;
					if((v = P_Last) != 0)
						do {
							HWND hItemWnd = GetDlgItem(H(), v->GetId());
							if(GetWindowRect(hItemWnd, &ItemRect) && IsWindowArea(ItemRect, x, y)) {
								x -= ItemRect.left;
								y -= ItemRect.top;
								InLnCalcUncapture();
								PostMessage(hItemWnd, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(x, y));
								break;
							}
							v = v->prev();
						} while(v != P_Last);
				}
				else {
					if(P_Il && P_ParentDlg)
						TView::messageBroadcast(P_ParentDlg, cmCommitInput, P_Il);
					TView::messageCommand(this, cmOK);
				}
			}
			break;
		default:
			return;
	}
	clearEvent(event);
}

int SLAPI showInputLineCalc(TDialog * pParentDlg, uint fieldCtlId)
{
	int    ok = -1;
	if(APPL && APPL->P_DeskTop) {
		TView * v = pParentDlg ? pParentDlg->getCtrlView(fieldCtlId) : 0;
		if(v && !v->IsInState(sfDisabled)) {
			InputLineCalc * p_dlg = new InputLineCalc(pParentDlg->CheckFlag(TDialog::fLarge) ? DLG_INLNCALC_L : DLG_INLNCALC, pParentDlg, fieldCtlId);
			if(p_dlg && APPL->validView(p_dlg)) {
				if(APPL->P_DeskTop->execView(p_dlg) == cmOK)
					ok = 1;
			}
			else
				ok = 0;
			delete p_dlg;
		}
	}
	return ok;
}

LRESULT CALLBACK InLnCalcWindProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	VirtButtonWndEx * p_vbwe = (VirtButtonWndEx *)TView::GetWindowUserData(hWnd);
	if(msg == WM_LBUTTONUP)
		showInputLineCalc(p_vbwe->P_Dlg, p_vbwe->FieldCtrlId);
	return CallWindowProc(p_vbwe->PrevWndProc, hWnd, msg, wParam, lParam);
}
