// CALENDAR.CPP
// Copyright (c) A.Fedotkov, A.Sobolev, A.Starodub 2001, 2002, 2003, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2016, 2017
//
#include <pp.h>
// #include <ppdlgs.h>
#pragma hdrstop

static INT_PTR CALLBACK CalendarWndProc(HWND, UINT, WPARAM, LPARAM);
static INT_PTR CALLBACK PeriodWndProc(HWND, UINT, WPARAM, LPARAM);

class TCalendar {
public:
	TCalendar()
	{
		THISZERO();
	}
	void   SetupCalendar();
	void   Validate();
	void   CheckTimer();
	void   SendToEditBox(LDATE d1, LDATE d2, int);
	void   SendToEditBox(int, int, int, int);
	void   SendToEditBox(int, int, int, int, int);
	void   SendToEditBox(int, int);

	int    seltype;
	HWND   c_hWnd;
	HWND   parent_hWnd;
	LDATE  D;
	LDATE  D1;
	LDATE  D2;
	int    PeriodSelect;
	int    NDays;
	int    FirstMonthDow;
	int    Top;
	int    Left;
	int    TimerFreq;
	int	   y_firstyear;
	int    SelStarted1;
	int    SelStarted2;
	int    SelStarted3;
	int    RowCount;
	
	int    c_i;
	int    c_j;
	int    c_minfirst;
	int    c_maxlast;
	int    c_maxrow;
	int    y_tw;
	int    y_bl;
	int    y_br;
	int    y_bw;
	int    y_th;
	int    y_t;
	int    y_w;

	_TCHAR C[7][7][12];
	TDialog * P_Dlg;
	uint   DateCtlID;
	int    IsLarge;
};

static const _TCHAR __Days[7][3] = {_T("ѕн"), _T("¬т"), _T("—р"), _T("„т"), _T("ѕт"), _T("—б"), _T("¬с") };

class TDateCalendar : public TCalendar {
public:
	static INT_PTR CALLBACK CalendarDlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

	TDateCalendar(TDialog * pDlg, uint pDateCtlID)
	{
		LDATE d;
		PeriodSelect = 0;
		P_Dlg = pDlg;
		parent_hWnd = P_Dlg ? P_Dlg->H() : 0;
		DateCtlID = pDateCtlID;
		IsLarge = BIN(P_Dlg && P_Dlg->CheckFlag(TDialog::fLarge));
		Top  = IsLarge ? 144 : 80;
		Left = 10;
		RetCmd = 0;
		if(DateCtlID == -1 || !P_Dlg || !P_Dlg->getCtrlData(DateCtlID, &D)) {
			D = getcurdate_();
			Validate();
		}
		else if(P_Dlg && P_Dlg->getCtrlData(DateCtlID, &D)) {
			d = getcurdate_();
			if(D.year() < 1800 || D.year() > 5000)
				D.setyear(d.year());
			if(D.month() < 1 || D.month() > 12)
				D.setmonth(d.month());
			if(D.day() < 1 || D.day() > 31)
				D.setday(d.day());
			Validate();
		}
		SetupCalendar();
	}
	void   ShowCalendar(HWND hParent)
	{
		TView * p_ctl = P_Dlg ? P_Dlg->getCtrlView(DateCtlID) : 0;
		if(!p_ctl || !p_ctl->IsInState(sfDisabled)) {
			int r = APPL->DlgBoxParam(IsLarge ? DLGW_CALENDAR_L : DLGW_CALENDAR,
				NZOR(hParent, parent_hWnd), TDateCalendar::CalendarDlgProc, (long)this);
			if(r > 0 && P_Dlg)
				TView::messageBroadcast(P_Dlg, cmCommitInput, P_Dlg->getCtrlView(DateCtlID));
		}
	}
	void   CloseCalendar();
	void   OnPaint(HWND);
	void   OnLButtonDown(HWND, LPARAM);
	void   OnLButtonUp(HWND, LPARAM);
	int    OnRButtonDown(HWND);
	void   OnMouseMove(HWND, WPARAM, LPARAM);
	void   OnTimer(HWND);
	int    OnTodaySelection();
	int    StepMonth(HWND hWnd, int forward);
	int    IsDayBar(int x, int y) const;
	int    setDTS(LDATE);
	int    getDTS(LDATE * pDt);
	int    GetRetCmd() const { return RetCmd; }
private:
	void   DrawBackground(HDC hdc);
	void   DrawDayOfWeekHeader(HDC hdc);
	void   DrawMonthGrid(HDC hdc);
	void   DrawMonthCells(HDC hdc, int decr);
	void   PaintMonthRect(TCanvas & rCanv, TPoint p, uint j, COLORREF color, COLORREF textColor);
	void   DrawSelectedYearRect(HDC hdc, int brushType, int);
	void   InvalidateMonthBar(HWND);
	int    IsYearMarker(int x, int y) const;
	int    IsYearBar(int x, int y) const;
	int    IsMonthBar(int x, int y) const;

	int    SelectYear(HWND hWnd, int n /* 1..NumYearInBar */);
	int    ScrollYear(HWND hWnd, int dir);
	int    SelectMonth(HWND hWnd, int x, int y);
	int    SelectQuart(HWND hWnd, int x, int y);
	int    SelectDay(HWND hWnd, int x, int y);
	int    SelectWeek(HWND hWnd, int x, int y);
	int    RetCmd;
};

class TCalendarP : public TCalendar {
public:
	TCalendarP(LDATE d1, LDATE d2)
	{
		PeriodSelect = 1;
		D  = checkdate(d1, 0) ? d1 : getcurdate_();
		D1 = d1;
		D2 = d2;
		Validate();
		SetupCalendar();
	}
	~TCalendarP()
	{
		DestroyWindow(c_hWnd);
	}
	void   ShowCalendar(HWND hwParent);
	void   CloseCalendar()
	{
	}
	int    OnSelectionType(int aSelType);
	int    OnResetButton(int kind /* -1 - left, 0 - all, 1 - right */);
};

class TPeriodCalendar {
public:
	TPeriodCalendar::TPeriodCalendar(TDialog * pDlg, uint dateCtlID)
	{
		P_Inner = 0;
		P_Dlg = pDlg;
		DateCtlID = dateCtlID;
		CALLPTRMEMB(P_Dlg, getCtrlString(DateCtlID, Period));
		UpdatePeriod();
	}
	void   Show()
	{
		APPL->DlgBoxParam(DLGW_PERIODCALENDAR, P_Dlg ? P_Dlg->H() : 0, PeriodWndProc, (long)this);
	}
	void   UpdatePeriod();
	int    SelectByFastPrd(HWND hWnd);

	int    seltype;
	TDialog * P_Dlg;
	uint   DateCtlID;
	LDATE  D1;
	LDATE  D2;
	//char   Period[128];
	SString Period;
	TCalendarP * P_Inner;
};

#define C_SELCOL     RGB(0, 0, 0)       // ÷вет выделени€ //
#define	C_SELPCOL    RGB(90, 90, 90)    // ÷вет выделени€ периода
#define	C_BACKCOL    RGB(223, 223, 223) // ÷вет фона
#define	M_SELCOL     RGB(255, 255, 255) // ÷вет шрифта выделени€ мес€ца
#define	M_DEFCOL     RGB(0, 0, 0)       // ÷вет шрифта мес€ца
#define	Y_TEXTCOL    RGB(0, 0, 0)       // ÷вет шрифта года
#define	C_TEXT2COL   RGB(255, 255, 100) // ÷вет шрифта дн€ начала периода
#define	C_TEXTSMCOL  RGB(0, 0, 255)     // ÷вет шрифта мес€ца, вход€щего в период
#define C_TEXTSMCCOL RGB(0, 130, 255)   // ÷вет шрифта мес€ца, вход€щего в период и €вл€ющегос€ текущим

#define B_OKLEFT    30
#define B_OKTOP    240
#define B_OKWIDTH   65
#define B_OKHEIGHT  20
#define B_CNCLLEFT 105

#define SEL_DAYS     0
#define SEL_MONTHS   1
#define SEL_QUARTALS 2
#define SEL_YEARS    3
#define SEL_WEEKS    4

void TCalendar::Validate()
{
	NDays = D.dayspermonth();
	LDATE  dt = D;
	dt.setday(1);
	FirstMonthDow = dayofweek(&dt, 1)-1;
}

void TCalendar::SetupCalendar()
{
	RowCount = 2;
	int    i, j, count = 1;
	_TCHAR s[3], t[3];
	for(j = 0; j <= 6; j++)
		sstrcpy(C[0][j], __Days[j]);
	for(i = 1; i <= 6; i++) {
		for(j = 0; j <= 6; j++) {
			if((i == 1 && FirstMonthDow > j) || count > NDays)
				sstrcpy(C[i][j], _T("  "));
			else {
				if(count == 1)
					c_minfirst = j;
				if(count == NDays) {
					c_maxlast = j;
					c_maxrow = i;
				}
				if(count < 10)
					sstrcpy(s, _T(" "));
				else
					s[0] = 0;
				strcat(s, _itoa(count, t, 10));
				sstrcpy(C[i][j], s);
				if((int)count == D.day()) {
					c_j = j;
					c_i = i;
				}
				count++;
			}
		}
		if(C[i][0][1] != ' ' && i != 1)
			RowCount++;
	}
}

void TDateCalendar::CloseCalendar()
{
	CALLPTRMEMB(P_Dlg, setCtrlData(DateCtlID, &D));
}

int TDateCalendar::setDTS(LDATE dt)
{
	D = dt;
	y_firstyear = D.year() - 2;
	Validate();
	SetupCalendar();
	InvalidateRect(c_hWnd, 0, true);
	SetFocus(c_hWnd);
	return 1;
}

int TDateCalendar::getDTS(LDATE * pDt)
{
	ASSIGN_PTR(pDt, D);
	return 1;
}

int TDateCalendar::OnTodaySelection()
{
	return setDTS(getcurdate_());
}

// static
INT_PTR CALLBACK TDateCalendar::CalendarDlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	TDateCalendar * dc = 0;
	RECT r;
	switch(message) {
		case WM_INITDIALOG:
			dc = (TDateCalendar *)lParam;
			TView::SetWindowUserData(hWnd, dc);
			if(dc && dc->P_Dlg && dc->DateCtlID) {
				HWND   h_ctl = GetDlgItem(dc->P_Dlg->H(), dc->DateCtlID);
				if(h_ctl) {
					RECT rect, this_rect;
					GetWindowRect(h_ctl, &rect);
					GetWindowRect(hWnd, &this_rect);
					int y = rect.bottom;
					int sizey = this_rect.bottom - this_rect.top;
					int sy = GetSystemMetrics(SM_CYFULLSCREEN);
					if((y + sizey) > sy)
						y = ((rect.top < sizey) ? sy : rect.top) - sizey;
					MoveWindow(hWnd, rect.left, y, this_rect.right - this_rect.left, sizey, 0);
				}
			}
			TView::PreprocessWindowCtrlText(hWnd); // @v9.1.1
			break;
		case WM_CREATE:
			dc = (TDateCalendar *)TView::GetWindowUserData(hWnd);
			dc->y_firstyear = dc->D.year() - 2;
			break;
		case WM_SHOWWINDOW:
			if(wParam) {
				dc = (TDateCalendar *)TView::GetWindowUserData(hWnd);
				int  left = dc->IsLarge ? 14 : 8;
				int  top  = dc->IsLarge ? 14 : 10;
				int  from_right  = dc->IsLarge ? 28 : 16;
				int  from_bottom = dc->IsLarge ? 75 : 44;
				//const char * p_classname = "PpyDateCalendar";
				LPCTSTR p_classname = _T("PpyDateCalendar");
				{
					WNDCLASSEX wc;
					MEMSZERO(wc);
					wc.cbSize = sizeof(wc);
					wc.lpszClassName = p_classname;
					wc.hInstance = TProgram::GetInst();
					wc.lpfnWndProc = (WNDPROC)CalendarWndProc;
					wc.style = CS_DBLCLKS;
					wc.hCursor = LoadCursor(NULL, IDC_ARROW);
					wc.hbrBackground = (HBRUSH)GetStockObject(LTGRAY_BRUSH);
					::RegisterClassEx(&wc); // @unicodeproblem
				}
				::GetClientRect(hWnd, &r);
				dc->c_hWnd = ::CreateWindow(p_classname, NULL, WS_VISIBLE | WS_CHILD | WS_TABSTOP | WS_GROUP,
					left, top, r.right - from_right, r.bottom - from_bottom, hWnd, NULL, TProgram::GetInst(), dc); // @unicodeproblem
				::ShowWindow(dc->c_hWnd, SW_SHOWNORMAL);
				::SetFocus(dc->c_hWnd);
			}
			break;
		case WM_COMMAND:
			dc = (TDateCalendar *)TView::GetWindowUserData(hWnd);
			switch(LOWORD(wParam)) {
				case IDCANCEL:
					KillTimer(dc->c_hWnd, 1);
					DestroyWindow(dc->c_hWnd);
					EndDialog(hWnd, 0);
					dc->RetCmd = cmCancel;
					return 1;
				case IDOK:
					KillTimer(dc->c_hWnd, 1);
					DestroyWindow(dc->c_hWnd);
					EndDialog(hWnd, 1);
					dc->RetCmd = cmOK;
					dc->CloseCalendar();
					return 1;
				case CTL_CALENDAR_TODAY:
					dc->OnTodaySelection();
					return 1;
			}
			break;
	}
	return 0;
}

void TCalendar::CheckTimer()
{
	if(TimerFreq == 50) {
		TimerFreq = 300;
		SetTimer(c_hWnd, 1, 300, 0);
	}
	else {
		TimerFreq = 52;
		SetTimer(c_hWnd, 1, 52, 0);
	}
}

const int C_CELLH = 16;
const int C_CELLW = 25;
const int M_CELLW = C_CELLW * 7 / 6;
const int M_CELLH = 15;
const int M_DIFFY = 45;
// const int Y_DIFFY = 70;

void TDateCalendar::PaintMonthRect(TCanvas & rCanv, TPoint p, uint j, COLORREF color, COLORREF textColor)
{
	HPEN   pen = CreatePen(PS_SOLID, 3, color);
	rCanv.SelectObjectAndPush(pen);
	HBRUSH br = CreateSolidBrush(color);
	rCanv.SelectObjectAndPush(br);
	TRect rect(p.x + 1, p.y, j < 6 ? p.x + (y_br - y_bl + 1) / 7 - 1 : p.x + (y_br - y_bl + 1) / 7 - 2, p.y + (C_CELLH * y_th / 13 - 2));
	rCanv.Rectangle(rect);
	rCanv.PopObjectN(2);
	ZDeleteWinGdiObject(&pen);
	ZDeleteWinGdiObject(&br);
	rCanv.SetTextColor(textColor);
}
//
// Draw grey Day-of-week header
//
void TDateCalendar::DrawDayOfWeekHeader(HDC hdc)
{
	HPEN   pen    = CreatePen(PS_SOLID, 1, RGB(170, 170, 170));
	HPEN   oldpen = (HPEN)SelectObject(hdc, pen);
	HBRUSH br     = CreateSolidBrush(RGB(170, 170, 170));
	HBRUSH oldbr  = (HBRUSH)SelectObject(hdc, br);
	Rectangle(hdc, Left, Top, Left + (y_br - y_bl), Top + (C_CELLH * y_th / 13 - 2));
	SelectObject(hdc, oldpen);
	SelectObject(hdc, oldbr);
	ZDeleteWinGdiObject(&br);
	ZDeleteWinGdiObject(&pen);
}
//
// Print Month calendar
//
void TDateCalendar::DrawMonthGrid(HDC hdc)
{
	const int  c_cell_w = (y_br - y_bl + 1) / 7;
	const int  c_cell_h = C_CELLH * y_th / 13;
	TCanvas canv(hdc);
	for(int i = 0; i <= 6; i++)
		for(int j = 0; j <= 6; j++)
			if(C[i][j][1] != ' ') {
				canv.SetTextColor(0);
				TPoint p, txt_sz;
				p.Set(Left + j * c_cell_w, Top  + i * c_cell_h);
				int    tmpd = atoi(C[i][j]);
				int    is_year = (D1.year() >= 1970 || D2.year() < 2500) ? 1 : 0;
				LDATE  dd1 = D1; // encodedate(Day1, Month1, Year1);
				LDATE  dd2 = D2; // encodedate(Day2, Month2, Year2);
				LDATE  dd = D;
				dd.setday(tmpd);
				if(PeriodSelect && D1.day() && D1.month() && D1.year() &&
					D2.day() && D2.month() && D2.year() && i && is_year &&
					((dd1 < dd2 && dd1 <= dd && dd2 >= dd) || (dd2 < dd1 && dd2 <= dd && dd1 >= dd))) {
					PaintMonthRect(canv, p, j, C_SELPCOL, C_TEXT2COL);
				}
				if(i == c_i && j == c_j)
					PaintMonthRect(canv, p, c_j, C_SELCOL, RGB(255, 255, 255));
				if(PeriodSelect == 1) {
					if(i > 0) {
						if(dd == dd1 || dd == dd2 || (is_year && tmpd == D.day()))
							canv.SetTextColor(C_TEXT2COL);
					}
				}
				txt_sz = canv.GetTextSize(C[i][j]);
				p.x += (c_cell_w - txt_sz.x) / 2;
				p.y += (c_cell_h - txt_sz.y) / 2;
				canv.TextOut(p, C[i][j]);
			}
}

void TDateCalendar::DrawSelectedYearRect(HDC hdc, int brushType, int i)
{
	TCanvas canv(hdc);
	canv.SelectObjectAndPush(GetStockObject(brushType));
	TPoint p;
	p.Set(Left + (y_br - y_bl - y_w) / 2, y_t);
	TRect r(p.x + (i - y_firstyear - 2) * y_w, p.y - 1, p.x + (i - y_firstyear - 1) * y_w, p.y + y_th + 2);
	canv.Rectangle(r);
	canv.PopObject();
}

void TDateCalendar::DrawMonthCells(HDC hdc, int decr)
{
	int  m_cell_w = (y_br - y_bl) / 6;
	int  m_cell_h = M_CELLH * y_th / 13;
	int  m_diff_y = M_DIFFY * y_th / 13;
	MoveToEx(hdc, Left, Top - m_diff_y + m_cell_h - decr, 0);
	LineTo(hdc, Left + (y_br - y_bl) - 1, Top - (m_diff_y - m_cell_h + decr));
	for(int i = 1; i < 6; i++) {
		MoveToEx(hdc, Left + (i * m_cell_w - decr), Top - (m_diff_y + 1), 0);
		LineTo(hdc, Left + (i * m_cell_w - decr), Top - (m_diff_y - m_cell_h * 2 - 3));
	}
}

void TDateCalendar::OnPaint(HWND hWnd)
{
	RECT   c_r;
	PAINTSTRUCT ps;
	HFONT  hf, hf1;
	SIZE   ts;
	HBRUSH br, oldbr;
	HPEN   pen, oldpen;
	LDATE  dd1, dd2, dd;
	int    i, x;
	LDATE  save1, save2;
	SString s;
	SString temp_buf;
	HDC    hdc = BeginPaint(hWnd, &ps);
	SetBkMode(hdc, TRANSPARENT);
	if(D1.year() == 0) {
		save1 = D1;
		D1 = encodedate(31, 12, 1969);
	}
	if(D2.year() == 0) {
		save2 = D2;
		D2 = encodedate(31, 12, 2500);
	}
	//
	// Print Year's bar
	//
	hf1 = (HFONT)GetStockObject(ANSI_VAR_FONT);
	hf = CreateFont(IsLarge ? 24 : 8, 0, 0, 0, FW_NORMAL, 0, 0, 0, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
		DEFAULT_QUALITY, DEFAULT_PITCH | FW_DONTCARE, _T("MS Sans Serif")); // @unicodeproblem
	SelectObject(hdc, hf);
	y_bl = Left;
	GetClientRect(c_hWnd, &c_r);
	y_br = c_r.right - y_bl;
	::GetTextExtentPoint32(hdc, _T("<<"), 2, &ts); // @unicodeproblem
	y_bw = ts.cx;
	y_th = ts.cy;
	y_t  = IsLarge ? 14 : 10;
	y_w  = (y_br - y_bl - y_bw * 2) / 5;
	::TextOut(hdc, y_bl, y_t, _T("<<"), 2); // @unicodeproblem
	::TextOut(hdc, y_br - y_bw, y_t, _T(">>"), 2); // @unicodeproblem

	pen = CreatePen(PS_NULL, 1, 0);
	oldpen = (HPEN)SelectObject(hdc, pen);
	for(i = y_firstyear; (int)i <= y_firstyear + 4; i++) {
		int is_year = (D1.year() >= 1970 || D2.year() < 2500) ? 1 : 0;
		if(PeriodSelect && is_year && ((D1.year() <= D2.year() && i >= D1.year() && i <= D2.year()) ||
			(D2.year() < D1.year() && i >= D2.year() && i <= D1.year()))) {
			SetTextColor(hdc, C_TEXT2COL);
			DrawSelectedYearRect(hdc, GRAY_BRUSH, i);
		}
		else if(i != D.year()) {
			DrawSelectedYearRect(hdc, LTGRAY_BRUSH, i);
			SetTextColor(hdc, 0);
		}
		if(i == D.year()) {
			DrawSelectedYearRect(hdc, BLACK_BRUSH, i);
			SetTextColor(hdc, RGB(255, 255, 255));
		}
		(s = 0).Cat(i);
		::GetTextExtentPoint32(hdc, s, s.Len(), &ts); // @unicodeproblem
		::TextOut(hdc, Left + (y_br - y_bl - y_w) / 2 + (i - y_firstyear - 2) * y_w + (y_w - ts.cx) / 2, y_t, s, s.Len()); // @unicodeproblem
	}
	SelectObject(hdc, oldpen);
	ZDeleteWinGdiObject(&pen);
	{
		HPEN   bg_pen = CreatePen(PS_SOLID, 1, RGB(127, 127, 127));
		HPEN   old_bg_pen = (HPEN)SelectObject(hdc, bg_pen);
		HBRUSH bg_br = CreateSolidBrush(C_BACKCOL);
		HBRUSH old_bg_br = (HBRUSH)SelectObject(hdc, bg_br);
		Rectangle(hdc, Left - 2, Top - 1, Left + (y_br - y_bl), Top + C_CELLH * y_th / 13 * 7);
		SelectObject(hdc, old_bg_pen);
		SelectObject(hdc, old_bg_br);
		ZDeleteWinGdiObject(&bg_pen);
		ZDeleteWinGdiObject(&bg_br);
	}
	DrawDayOfWeekHeader(hdc);
	DrawMonthGrid(hdc);
	//
	// Draw days frames
	//
	int  m_cell_w = (y_br - y_bl) / 6;
	int  m_cell_h = M_CELLH * y_th / 13;
	int  m_diff_y = M_DIFFY * y_th / 13;
	{
		int    c_cell_h = C_CELLH * y_th / 13;
		HPEN   black_pen = CreatePen(PS_SOLID, 1, 0);
		HPEN   white_pen = CreatePen(PS_SOLID, 1, RGB(255, 255, 255));
		HPEN   gray_pen  = CreatePen(PS_SOLID, 1, RGB(127, 127, 127));

		oldpen = (HPEN)SelectObject(hdc, black_pen);
		MoveToEx(hdc, Left - 1, Top + 7 * c_cell_h - 1, 0);
		LineTo(hdc, Left - 1, Top);
		LineTo(hdc, Left - 1 + (y_br - y_bl), Top);

		SelectObject(hdc, white_pen);
		MoveToEx(hdc, Left, Top + 7 * c_cell_h - 1, 0);
		LineTo  (hdc, Left + (y_br - y_bl) - 1, Top + 7 * c_cell_h - 1);
		LineTo  (hdc, Left + (y_br - y_bl) - 1, Top - 1);
		//
		// Draw months background rectangle
		//
		SelectObject(hdc, gray_pen);
		{
			br = CreateSolidBrush(RGB(212, 208, 200));
			oldbr = (HBRUSH)SelectObject(hdc, br);
			Rectangle(hdc, Left - 2, Top - m_diff_y - 3, Left + (y_br - y_bl), Top - m_diff_y + m_cell_h * 2 + 3);
			SelectObject(hdc, oldbr);
			ZDeleteWinGdiObject(&br);
		}
		//
		// Draw months frames
		//
		SelectObject(hdc, white_pen);
		MoveToEx(hdc, Left, Top - m_diff_y + m_cell_h * 2 + 3, 0);
		LineTo(hdc, Left + (y_br - y_bl), Top - m_diff_y + m_cell_h * 2 + 3);
		LineTo(hdc, Left + (y_br - y_bl), Top - m_diff_y - 3);
		DrawMonthCells(hdc, 0);
		SelectObject(hdc, gray_pen);
		DrawMonthCells(hdc, 1);
		//
		SelectObject(hdc, white_pen);
		MoveToEx(hdc, Left - 1, Top - m_diff_y + m_cell_h * 2 + 3, 0);
		LineTo  (hdc, Left - 1, Top - m_diff_y - 2);
		LineTo  (hdc, Left + (y_br - y_bl), Top - m_diff_y - 2);
		SelectObject(hdc, oldpen);

		ZDeleteWinGdiObject(&black_pen);
		ZDeleteWinGdiObject(&white_pen);
		ZDeleteWinGdiObject(&gray_pen);
	}
	//
	// Print Month names
	//
	for(i = 1; i <= 12; i++) {
		//char   buf[64];
		//getMonthText(i, MONF_SHORT, buf);
		SGetMonthText(i, MONF_SHORT, temp_buf = 0);
		::GetTextExtentPoint32(hdc, temp_buf, 3, &ts); // @unicodeproblem
		x = Left + (((i <= 6) ? i : i - 6) - 1) * m_cell_w;
		(dd1 = D1).setday(1);
		(dd2 = D2).setday(1);
		dd = encodedate(1, i, D.year());
		if(PeriodSelect && (D1.year() >= 1970 || D2.year() < 2500) && (
			(dd1 <= dd2 && dd <= dd2 && dd >= dd1) || (dd2 < dd1 && dd <= dd1 && dd >= dd2)))
			SetTextColor(hdc, (i == D.month()) ? C_TEXTSMCCOL : C_TEXTSMCOL);
		else
			SetTextColor(hdc, (i == D.month()) ? M_SELCOL : M_DEFCOL);
		::TextOut(hdc, x + (m_cell_w - ts.cx) / 2, Top - ((i <= 6) ? m_diff_y : m_diff_y - m_cell_h - 2), temp_buf, 3); // @unicodeproblem
	}
	SelectObject(hdc, hf1);
	ZDeleteWinGdiObject(&hf);
	EndPaint(hWnd, &ps);
	if(D1.year() == 1969)
		D1 = save1;
	if(D2.year() == 2500)
		D2 = save2;
}

void TDateCalendar::OnMouseMove(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	if(PeriodSelect && wParam == MK_LBUTTON) {
		int x = LOWORD(lParam);
		int y = HIWORD(lParam);
		if((seltype == SEL_DAYS && IsDayBar(x, y)) ||
			(oneof2(seltype, SEL_MONTHS, SEL_QUARTALS) && IsMonthBar(x, y)) ||
			(seltype == SEL_YEARS && IsYearBar(x, y)))
			SendMessage(hWnd, WM_LBUTTONDOWN, wParam, lParam);
	}
}

void TDateCalendar::OnLButtonUp(HWND hWnd, LPARAM lParam)
{
	int x = LOWORD(lParam);
	int y = HIWORD(lParam);
	if(IsMonthBar(x, y) || IsYearBar(x, y))
		InvalidateRect(hWnd, NULL, false);
}

int TDateCalendar::SelectYear(HWND hWnd, int n /* 1..NumYearInBar */)
{
	D.setyear(y_firstyear + n - 1);
	Validate();
	SetupCalendar();
	if(PeriodSelect && seltype == SEL_YEARS) {
		if(SelStarted3) {
			D2 = encodedate(31, 12, D.year());
			if(D1 > D2) {
				D1.setday(31);
				D1.setmonth(12);
				D2.setday(1);
				D2.setmonth(1);
			}
			else {
				D1.setday(1);
				D1.setmonth(1);
				D2.setday(31);
				D2.setmonth(12);
			}
		}
		else {
			SelStarted3 = 1;
			D1 = encodedate(1, 1, D.year());
			D2 = encodedate(31, 12, D.year());
		}
		SendToEditBox(D1.year(), D2.year());
	}
	InvalidateRect(hWnd, NULL, false);
	return 1;
}

int TDateCalendar::ScrollYear(HWND hWnd, int dir)
{
	int ok = -1;
	if(dir < 0) {
		if(y_firstyear > 1970) {
			y_firstyear--;
			ok = 1;
		}
	}
	else if(dir > 0) {
		if((y_firstyear + 4) < 2300) {
			y_firstyear++;
			ok = 1;
		}
	}
	if(ok > 0) {
		RECT rr;
		Validate();
		SetupCalendar();
		rr.left   = y_bl;
		rr.right  = y_br;
		rr.top    = y_t - 3;
		rr.bottom = y_t + y_th + 3;
		InvalidateRect(hWnd, &rr, true);
	}
	return ok;
}
//
// Returns:
//    -1 - left Year marker
//    +1 - right Year marker
//
int TDateCalendar::IsYearMarker(int x, int y) const
{
	if(y >= y_t && y <= (y_t + y_th)) {
		if(x >= y_bl && x <= (y_bl + y_bw))
			return -1;
		else if(x >= (y_br - y_bw) && x <= y_br)
			return 1;
	}
	return 0;
}

int TDateCalendar::IsYearBar(int x, int y) const
{
	if(x >= (y_bl + y_bw) && x <= (y_br - y_bw) && y >= y_t && y <= (y_t + y_th))
		return 1 + (x - (y_bl + y_bw)) / y_w;
	else
		return 0;
}

int TDateCalendar::IsMonthBar(int x, int y) const
{
	int  m_cell_h = M_CELLH * y_th / 13;
	int  m_diff_y = M_DIFFY * y_th / 13;
	return BIN((x > Left + 2) && (x < Left + (y_br - y_bl) - 3) &&
		(y > Top - m_diff_y - 2) && (y < Top - m_diff_y + 2 * m_cell_h + 2));
}

void TDateCalendar::InvalidateMonthBar(HWND hWnd)
{
	int  m_cell_h = M_CELLH * y_th / 13;
	int  m_diff_y = M_DIFFY * y_th / 13;
	RECT rr;
	rr.left   = Left;
	rr.right  = Left + (y_br - y_bl) - 2;
	rr.top    = Top - m_diff_y - 1;
	rr.bottom = Top - m_diff_y + 2 * m_cell_h + 1;
	InvalidateRect(hWnd, &rr, false);
}

int TDateCalendar::IsDayBar(int x, int y) const
{
	int  c_cell_h = C_CELLH * y_th / 13;
	return ((x > Left + 2) && (x < Left + (y_br - y_bl) - 3) &&
		(y > Top + c_cell_h + 2) && (y < Top + 7 * c_cell_h - 3)) ? 1 : 0;
}

int TDateCalendar::SelectMonth(HWND hWnd, int x, int y)
{
	int    j = (x - Left) * 6 / (y_br - y_bl);
	int    i = (y - (Top - M_DIFFY * y_th / 13 - 2)) / (M_CELLH * y_th / 13 + 2);
	LDATE  dd1 = D2; dd1.setday(1);
	LDATE  dd2 = encodedate(1, j + 1 + i * 6, D.year());
	if(dd1 != dd2 || (dd1 == dd2 && !SelStarted2)) {
		D.setmonth(j + 1 + i * 6);
		Validate();
		SetupCalendar();
		if(PeriodSelect && seltype == SEL_MONTHS) {
			if(SelStarted2) {
				D2 = encodedate(D2.day(), D.month(), D.year());
				if(D1 > D2) {
					i = D.month();
					D.setmonth(D1.month());
					Validate();
					D1.setday(NDays);
					D2.setday(1);
					D.setmonth(i);
					Validate();
				}
				else {
					i = D.month();
					D.setmonth(D2.month());
					Validate();
					D1.setday(1);
					D2.setday(NDays);
					D.setmonth(i);
					Validate();
				}
			}
			else {
				SelStarted2 = 1;
				D1 = encodedate(1, D.month(), D.year());
				D2 = encodedate(NDays, D.month(), D.year());
			}
			SendToEditBox(D1.month(), D1.year(), D2.month(), D2.year(), 0);
		}
		InvalidateRect(hWnd, NULL, false);
	}
	return 1;
}

int TDateCalendar::SelectQuart(HWND hWnd, int x, int y)
{
	int    j = (x - Left) * 6 / (y_br - y_bl);
	int    i = (y - (Top - M_DIFFY * y_th / 13 - 2)) / (M_CELLH * y_th / 13 + 2);
	int    q = (j + i * 6) / 3 + 1;
	LDATE  dd1 = encodedate(1, D2.month(), D.year());
	LDATE  dd  = encodedate(1, D1.month(), D.year());
	LDATE  dd2 = encodedate(1, (q - 1) * 3 + 1, D.year());
	if(dd1 != dd2 || (dd1 == dd2 && !SelStarted2)) {
		D.setmonth((q - 1) * 3 + 1);
		Validate();
		SetupCalendar();
		if(SelStarted2) {
			D2.setmonth(D.month() + 2);
			D2.setyear(D.year());
			dd1 = D1;
			dd2 = D2;
			if(dd1 > dd2) {
				i = D.month();
				D2.setmonth(i);
				int mon1 = D1.month();
				if(oneof4(mon1, 1, 4, 7, 10))
					D1.setmonth(mon1+2);
				D.setmonth(D1.month());
				Validate();
				D1.setday(NDays);
				D2.setday(1);
				D.setmonth(i);
				Validate();
			}
			else {
				i = D.month();
				int mon1 = D1.month();
				if(oneof4(mon1, 3, 6, 9, 12))
					D1.setmonth(mon1-2);
				D.setmonth(D2.month());
				Validate();
				D1.setday(1);
				D2.setday(NDays);
				D.setmonth(i);
				Validate();
			}
		}
		else {
			SelStarted2 = 1;
			D1 = encodedate(1, D.month(), D.year());
			D2 = encodedate(NDays, D.month() + 2, D.year());
		}
		SendToEditBox((D1.month() - 1) / 3 + 1, D1.year(), (D2.month() - 1) / 3 + 1, D2.year());
		InvalidateMonthBar(hWnd);
	}
	return 1;
}

static int hashd(LDATE d)
{
	return (d.month() * 31 + (d.year() - 1800) * 366);
}

int TDateCalendar::SelectDay(HWND hWnd, int x, int y)
{
	int    c_cell_w = (y_br - y_bl + 1) / 7;
	int    c_cell_h = C_CELLH * y_th / 13;
	int    j = (x - Left) / c_cell_w;
	int    i = (y - Top)  / c_cell_h - 1;
	if(!i && j < c_minfirst) {
		LDATE temp_date = D;
		temp_date.setday(1);
		setDTS(plusdate(temp_date, j-c_minfirst));
	}
	else if(i == (c_maxrow-1) && j > c_maxlast) {
		LDATE temp_date = D;
		temp_date.setday(D.dayspermonth());
		setDTS(plusdate(temp_date, j-c_maxlast));
	}
	else if(((i && i < (c_maxrow-1)) || (!i && j >= c_minfirst) || (i == (c_maxrow-1) && j <= c_maxlast)) &&
		(!PeriodSelect || (atoi(C[i+1][j]) + hashd(D) != D2.day() + hashd(D2)))) {
		//
		// Move the selection
		//
		HDC    hdc = GetDC(hWnd);
		HFONT  hf = CreateFont(IsLarge ? 24 : 8, 0, 0, 0, FW_NORMAL, 0, 0, 0, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
			DEFAULT_QUALITY, DEFAULT_PITCH | FW_DONTCARE, _T("MS Sans Serif")); // @unicodeproblem
		HFONT  hf_old = (HFONT)SelectObject(hdc, hf);
		SetBkMode(hdc, TRANSPARENT);
		SetTextColor(hdc, RGB(0, 0, 0));
		if(PeriodSelect && D == D1)
			SetTextColor(hdc, C_TEXT2COL);
		x = Left + c_j * c_cell_w;
		y = Top + c_i * c_cell_h;
		HPEN   pen = CreatePen(PS_SOLID, 3, C_BACKCOL);
		HPEN   oldpen = (HPEN)SelectObject(hdc, pen);
		HBRUSH br = CreateSolidBrush(C_BACKCOL);
		HBRUSH oldbr = (HBRUSH)SelectObject(hdc, br);
		Rectangle(hdc, x + 1, y, (c_j < 6) ? (x + c_cell_w - 1) : (x + c_cell_w - 2), y + c_cell_h - 2);
		SelectObject(hdc, oldpen);
		SelectObject(hdc, oldbr);
		ZDeleteWinGdiObject(&pen);
		ZDeleteWinGdiObject(&br);
		SIZE ts;
		::GetTextExtentPoint32(hdc, C[c_i][c_j], 2, &ts); // @unicodeproblem
		::TextOut(hdc, x + (c_cell_w - ts.cx) / 2, y + (c_cell_h - ts.cy) / 2, C[c_i][c_j], 2); // @unicodeproblem
		c_i = i + 1;
		c_j = j;
		D.setday(atoi(C[c_i][c_j]));
		if(PeriodSelect == 1) {
			if(D1.day()) {
				if(D2.year() != D.year() || D2.month() != D.month()) {
					RECT rr;
					rr.left   = Left;
					rr.right  = Left + (y_br - y_bl) + 1;
					rr.top    = y_t - 3;
					rr.bottom = Top - M_DIFFY * y_th / 13 + M_CELLH * y_th / 13 * 2 + 10;
					InvalidateRect(hWnd, &rr, true);
				}
				D2 = D;
				//
				// «десь отрисовывать выделение
				//
				for(int ii = 1; ii <= c_maxrow; ii++) {
					for(int jj = 0; jj <= 6; jj++) {
						int    cur_entry = atoi(C[ii][jj]);
						int    eq_ym = BIN(D2.month() == D1.month() && D2.year() == D1.year());
						long   nd1 = D1.day() + hashd(D1);
						long   nd2 = D2.day() + hashd(D2);
						int    is_bound = BIN((ii == c_maxrow && jj > c_maxlast) || (ii == 1 && jj < c_minfirst));
						const  int xx = Left + jj * c_cell_w;
						const  int yy = Top + ii * c_cell_h;
						const  int day1 = D1.day();
						const  int day2 = D2.day();
						if((cur_entry >= (eq_ym ? day1 : 1) && cur_entry <= day2 && !is_bound && nd1 < nd2) ||
							(cur_entry <= (eq_ym ? day1 : 31) && cur_entry >= day2 && !is_bound && nd1 >= nd2)) {
							pen = CreatePen(PS_SOLID, 3, C_SELPCOL);
							br  = CreateSolidBrush(C_SELPCOL);
							SetTextColor(hdc, C_TEXT2COL);
						}
						else {
							pen = CreatePen(PS_SOLID, 3, C_BACKCOL);
							br  = CreateSolidBrush(C_BACKCOL);
							SetTextColor(hdc, 0);
						}
						oldpen = (HPEN)SelectObject(hdc, pen);
						oldbr  = (HBRUSH)SelectObject(hdc, br);
						Rectangle(hdc, xx + 1, yy, (jj < 6) ? (xx + c_cell_w - 1) : (xx + c_cell_w - 2), yy + c_cell_h - 2);
						SelectObject(hdc, oldpen);
						SelectObject(hdc, oldbr);
						ZDeleteWinGdiObject(&pen);
						ZDeleteWinGdiObject(&br);
						::GetTextExtentPoint32(hdc, C[ii][jj], 2, &ts); // @unicodeproblem
						::TextOut(hdc, xx + (c_cell_w - ts.cx) / 2, yy + (c_cell_h - ts.cy) / 2, C[ii][jj], 2); // @unicodeproblem
					}
				}
				SetTextColor(hdc, C_TEXT2COL);
			}
			else {
				D1 = D2 = D;
				SetTextColor(hdc, C_TEXT2COL);
			}
			//
			// ѕередача текущего периода в едитбокс
			//
			if(D1 > D2)
				SendToEditBox(D2, D1, 1);
			else
				SendToEditBox(D1, D2, 1);
		}
		else
			SetTextColor(hdc, RGB(255, 255, 255));
		x = Left + c_j * c_cell_w;
		y = Top  + c_i * c_cell_h;
		pen = CreatePen(PS_SOLID, 3, C_SELCOL);
		oldpen = (HPEN)SelectObject(hdc, pen);
		br = CreateSolidBrush(C_SELCOL);
		oldbr = (HBRUSH)SelectObject(hdc, br);
		Rectangle(hdc, x + 1, y, c_j < 6 ? x + c_cell_w - 1 : x + c_cell_w - 2, y + c_cell_h - 2);
		SelectObject(hdc, oldpen);
		SelectObject(hdc, oldbr);
		ZDeleteWinGdiObject(&pen);
		ZDeleteWinGdiObject(&br);
		::GetTextExtentPoint32(hdc, C[c_i][c_j], 2, &ts); // @unicodeproblem
		::TextOut(hdc, x + (c_cell_w - ts.cx) / 2, y + (c_cell_h - ts.cy) / 2, C[c_i][c_j], 2); // @unicodeproblem
		SelectObject(hdc, hf_old);
		ZDeleteWinGdiObject(&hf);
		ReleaseDC(hWnd, hdc);
		if(!SelStarted1)
			InvalidateRect(hWnd, NULL, false);
		SelStarted1 = 1;
	}
	return 1;
}

int  TDateCalendar::SelectWeek(HWND hWnd, int x, int y)
{
	int    c_cell_w = (y_br - y_bl + 1) / 7;
	int    c_cell_h = C_CELLH * y_th / 13;
	int    j = (x - Left) / c_cell_w;
	int    i = (y - Top)  / c_cell_h - 1;
	if(((i && i < (c_maxrow - 1)) || (!i && j >= c_minfirst) || (i == (c_maxrow - 1) && j <= c_maxlast)) && (atoi(C[i+1][j]) + hashd(D) != D2.day() + hashd(D2))) {
		// Move the selection
		int draw_months = 0;
		HDC    hdc = GetDC(hWnd);
		HFONT  hf = CreateFont(IsLarge ? 24 : 8, 0, 0, 0, FW_NORMAL, 0, 0, 0, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
			DEFAULT_QUALITY, DEFAULT_PITCH | FW_DONTCARE, _T("MS Sans Serif")); // @unicodeproblem
		HFONT  hf_old = (HFONT)SelectObject(hdc, hf);
		SetBkMode(hdc, TRANSPARENT);
		SetTextColor(hdc, RGB(0, 0, 0));
		if(PeriodSelect && D == D1)
			SetTextColor(hdc, C_TEXT2COL);
		x = Left + c_j * c_cell_w /* C_CELLW */;
		y = Top + c_i * c_cell_h /* C_CELLH */;
		HPEN   pen = CreatePen(PS_SOLID, 3, C_BACKCOL);
		HPEN   oldpen = (HPEN)SelectObject(hdc, pen);
		HBRUSH br = CreateSolidBrush(C_BACKCOL);
		HBRUSH oldbr = (HBRUSH)SelectObject(hdc, br);
		Rectangle(hdc, x + 1, y, c_j < 6 ? x + c_cell_w - 1 : x + c_cell_w - 2, y + c_cell_h - 2);
		SelectObject(hdc, oldpen);
		SelectObject(hdc, oldbr);
		ZDeleteWinGdiObject(&pen);
		ZDeleteWinGdiObject(&br);
		SIZE ts;
		::GetTextExtentPoint32(hdc, C[c_i][c_j], 2, &ts); // @unicodeproblem
		::TextOut(hdc, x + (c_cell_w - ts.cx) / 2, y + (c_cell_h - ts.cy) / 2, C[c_i][c_j], 2); // @unicodeproblem
		c_i = i + 1;
		c_j = j;
		D.setday(atoi(C[c_i][c_j]));
		draw_months = (D2.year() != D.year() || D2.month() != D.month()) ? 1 : 0;
		{
			LDATE  beg_dt = encodedate(1, 1, 1);
			long   days = ((diffdate(D, beg_dt) - 1)/ 7) * 7 + 1;
			LDATE  dt = plusdate(beg_dt, days);
			if(!SelStarted1) {
				D1 = dt;
				D2 = plusdate(dt, 6);
			}
			else if(D1 > D2) {
				if(dt > D1) {
					D1 = plusdate(D1, -6);
					D2 = plusdate(dt, 6);
				}
				else {
					LDATE temp_dt = D2;
					D1 = (D2 > D1) ? D2 : D1;
					D2 = dt;
				}
			}
			else if(D2 > D1) {
				if(D1 > dt) {
					D2 = dt;
					D1 = plusdate(D1, 6);
				}
				else
					D2 = plusdate(dt, 6);
			}
		}
		if(D1.day()) {
			LDATE dt1 = (D2 > D1) ? D1 : D2, dt2 = (D2 > D1) ? D2 : D1;
			if(draw_months) {
				RECT rr;
				rr.left   = Left;
				rr.right  = Left + (y_br - y_bl) + 1;
				rr.top    = y_t - 3;
				rr.bottom = Top - M_DIFFY * y_th / 13 + M_CELLH * y_th / 13 * 2 + 10;
				InvalidateRect(hWnd, &rr, true);
			}
			//
			// «десь отрисовывать выделение
			//
			for(int ii = 1; ii <= c_maxrow; ii++) {
				for(int jj = 0; jj <= 6; jj++) {
					int    cur_entry = atoi(C[ii][jj]);
					int    is_bound = BIN((ii == c_maxrow && jj > c_maxlast) || (ii == 1 && jj < c_minfirst));
					const  int xx = Left + jj * c_cell_w;
					const  int yy = Top + ii * c_cell_h;
					LDATE  cur_dt = encodedate(cur_entry, D.month(), D.year());
					if(cur_dt >= dt1 && cur_dt <= dt2 && !is_bound) {
						pen = CreatePen(PS_SOLID, 3, C_SELPCOL);
						br  = CreateSolidBrush(C_SELPCOL);
						SetTextColor(hdc, C_TEXT2COL);
					}
					else {
						pen = CreatePen(PS_SOLID, 3, C_BACKCOL);
						br  = CreateSolidBrush(C_BACKCOL);
						SetTextColor(hdc, 0);
					}
					oldpen = (HPEN)SelectObject(hdc, pen);
					oldbr  = (HBRUSH)SelectObject(hdc, br);
					Rectangle(hdc, xx + 1, yy, (jj < 6) ? (xx + c_cell_w - 1) : (xx + c_cell_w - 2), yy + c_cell_h - 2);
					SelectObject(hdc, oldpen);
					SelectObject(hdc, oldbr);
					ZDeleteWinGdiObject(&pen);
					ZDeleteWinGdiObject(&br);
					::GetTextExtentPoint32(hdc, C[ii][jj], 2, &ts); // @unicodeproblem
					::TextOut(hdc, xx + (c_cell_w - ts.cx) / 2, yy + (c_cell_h - ts.cy) / 2, C[ii][jj], 2); // @unicodeproblem
				}
			}
			SetTextColor(hdc, C_TEXT2COL);
		}
		else {
			D1 = D2 = D;
			SetTextColor(hdc, C_TEXT2COL);
		}
		if(D1 > D2)
			SendToEditBox(D2, D1, 1);
		else
			SendToEditBox(D1, D2, 1);
		x = Left + c_j * c_cell_w;
		y = Top  + c_i * c_cell_h;
		pen = CreatePen(PS_SOLID, 3, C_SELCOL);
		oldpen = (HPEN)SelectObject(hdc, pen);
		br = CreateSolidBrush(C_SELCOL);
		oldbr = (HBRUSH)SelectObject(hdc, br);
		Rectangle(hdc, x + 1, y, (c_j < 6) ? (x + c_cell_w - 1) : (x + c_cell_w - 2), y + c_cell_h - 2);
		SelectObject(hdc, oldpen);
		SelectObject(hdc, oldbr);
		ZDeleteWinGdiObject(&pen);
		ZDeleteWinGdiObject(&br);
		::GetTextExtentPoint32(hdc, C[c_i][c_j], 2, &ts); // @unicodeproblem
		::TextOut(hdc, x + (c_cell_w - ts.cx) / 2, y + (c_cell_h - ts.cy) / 2, C[c_i][c_j], 2); // @unicodeproblem
		SelectObject(hdc, hf_old);
		ZDeleteWinGdiObject(&hf);
		ReleaseDC(hWnd, hdc);
		if(!SelStarted1)
			InvalidateRect(hWnd, NULL, false);
		SelStarted1 = 1;
	}
	return 1;
}

void TDateCalendar::OnLButtonDown(HWND hWnd, LPARAM lParam)
{
	int i;
	int x = LOWORD(lParam);
	int y = HIWORD(lParam);
	SetFocus(hWnd);
	if((i = IsYearBar(x, y)) != 0)
		SelectYear(hWnd, i);
	else if(IsYearMarker(x, y) < 0)
		ScrollYear(hWnd, -1);
	else if(IsYearMarker(x, y) > 0)
		ScrollYear(hWnd, 1);
	else if(IsDayBar(x, y) && (!PeriodSelect || seltype == SEL_DAYS || seltype == SEL_WEEKS)) {
		if(!PeriodSelect || seltype == SEL_DAYS)
			SelectDay(hWnd, x, y);
		else if(seltype == SEL_WEEKS)
			SelectWeek(hWnd, x, y);
	}
	else if(IsMonthBar(x, y))
		if(seltype != SEL_QUARTALS)
			SelectMonth(hWnd, x, y);
		else if(PeriodSelect)
			SelectQuart(hWnd, x, y);
	{
		HWND hwnd_ctl = GetDlgItem(GetParent(hWnd), CTL_CALENDAR_FASTPRD);
		SendMessage(hwnd_ctl, CB_SETCURSEL, -1, 0);
	}
}

static void FASTCALL SendLButtDownMsg(HWND hWnd, TPoint p)
{
	SendMessage(hWnd, WM_LBUTTONDOWN, MK_LBUTTON, p.towparam());
}

void TDateCalendar::OnTimer(HWND hWnd)
{
	int    c_cell_w = (y_br - y_bl + 1) / 7;
	int    c_cell_h = C_CELLH * y_th / 13;
	TPoint p;
	unsigned short r = GetKeyState(VK_LEFT);
	if(r & 0x8000) {
		CheckTimer();
		if(c_j > 0) {
			p.y = c_i * c_cell_h + Top;
			if(!SelStarted1) {
				p.x = c_j * c_cell_w + Left;
				SendLButtDownMsg(hWnd, p+3);
			}
			p.x = (c_j - 1) * c_cell_w + Left;
			SendLButtDownMsg(hWnd, p+3);
		}
		else if(c_i > 1) {
			if(!SelStarted1)
				SendLButtDownMsg(hWnd, p.Set(6 * c_cell_w + Left, c_i * c_cell_h + Top) + 3);
			SendLButtDownMsg(hWnd, p.Set(6 * c_cell_w + Left, (c_i - 1) * c_cell_h + Top) + 3);
		}
		return;
	}
	r = GetKeyState(VK_RIGHT);
	if(r & 0x8000) {
		CheckTimer();
		if(c_j < 6) {
			if(!SelStarted1)
				SendLButtDownMsg(hWnd, p.Set(c_j * c_cell_w + Left, c_i * c_cell_h + Top) + 3);
			SendLButtDownMsg(hWnd, p.Set((c_j + 1) * c_cell_w + Left, c_i * c_cell_h + Top) + 3);
		}
		else if(c_i < c_maxrow) {
			if(!SelStarted1)
				SendLButtDownMsg(hWnd, p.Set(Left, c_i * c_cell_h + Top) + 3);
			SendLButtDownMsg(hWnd, p.Set(Left, (c_i + 1) * c_cell_h + Top) + 3);
		}
	}
	r = GetKeyState(VK_UP);
	if(r & 0x8000) {
		CheckTimer();
		if(c_i > 0) {
			if(!SelStarted1)
				SendLButtDownMsg(hWnd, p.Set(c_j * c_cell_w + Left, c_i * c_cell_h + Top) + 3);
			SendLButtDownMsg(hWnd, p.Set(c_j * c_cell_w + Left, (c_i - 1) * c_cell_h + Top) + 3);
		}
	}
	r = GetKeyState(VK_DOWN);
	if(r & 0x8000) {
		CheckTimer();
		if(c_i < c_maxrow) {
			if(!SelStarted1)
				SendLButtDownMsg(hWnd, p.Set(c_j * c_cell_w + Left, c_i * c_cell_h + Top) + 3);
			SendLButtDownMsg(hWnd, p.Set(c_j * c_cell_w + Left, (c_i + 1) * c_cell_h + Top) + 3);
		}
	}
}

int TDateCalendar::OnRButtonDown(HWND hWnd)
{
	if(D1.day() && PeriodSelect) {
		D1 = ZERODATE;
		D2 = ZERODATE;
		Validate();
		SetupCalendar();
		SelStarted1 = 0;
		SelStarted2 = 0;
		SelStarted3 = 0;
		HWND ctl_edit = GetDlgItem(parent_hWnd, 1031);
		// @v9.1.5 SendMessage(ctl_edit, WM_SETTEXT, 0, (LPARAM) "..");
		SString temp_buf = ".."; // @v9.1.5 
		TView::SSetWindowText(ctl_edit, temp_buf); // @v9.1.5 
		InvalidateRect(hWnd, NULL, true);
	}
	return 1;
}

int TDateCalendar::StepMonth(HWND hWnd, int forward)
{
	int    ok = 1;
	int    mon = D.month();
	int    m_cell_w = (y_br - y_bl) / 6;
	int    m_cell_h = M_CELLH * y_th / 13;
	int    m_diff_y = M_DIFFY * y_th / 13;
	if(forward)
		if(mon < 12)
			D.setmonth(++mon);
		else
			ok = -1;
	else
		if(mon > 1)
			D.setmonth(--mon);
		else
			ok = -1;
	if(ok > 0) {
		TPoint p;
		if(mon <= 6)
			p.Set((mon - 1) * m_cell_w + Left + 3, Top - m_diff_y + 3);
		else
			p.Set((mon - 7) * m_cell_w + Left + 3, Top - m_diff_y + 5 + m_cell_h);
		SendMessage(hWnd, WM_LBUTTONDOWN, MK_LBUTTON, p.towparam());
	}
	return ok;
}

INT_PTR CALLBACK CalendarWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	TDateCalendar * dc = (TDateCalendar *)TView::GetWindowUserData(hWnd);
	switch(message) {
		case WM_CREATE:
			{
				LPCREATESTRUCT p_cr_data = (LPCREATESTRUCT)lParam;
				dc = (TDateCalendar *)p_cr_data->lpCreateParams;
				TView::SetWindowUserData(hWnd, dc);
				dc->SelStarted1 = 0;
				dc->SelStarted2 = 0;
				dc->SelStarted3 = 0;
				dc->y_firstyear = dc->D.year() - 2;
			}
			break;
		case WM_SETFOCUS:
			dc->TimerFreq = 50;
			SetTimer(dc->c_hWnd, 1, 50, 0);
			break;
		case WM_KILLFOCUS:
			KillTimer(dc->c_hWnd, 1);
			break;
		case WM_TIMER:
			dc->OnTimer(hWnd);
			break;
		case WM_KEYUP:
			dc->TimerFreq = 50;
			SetTimer(dc->c_hWnd, 1, 50, 0);
			break;
		case WM_KEYDOWN:
			switch((int)wParam) {
				case VK_ESCAPE:
					DestroyWindow(dc->c_hWnd);
					EndDialog(hWnd, 0);
					break;
				case VK_END:
					SendMessage(hWnd, WM_LBUTTONDOWN, MK_LBUTTON, MAKEWPARAM(dc->y_br - 2, dc->y_t + 2));
					break;
				case VK_HOME:
					SendMessage(hWnd, WM_LBUTTONDOWN, MK_LBUTTON, MAKEWPARAM(dc->y_bl + 2, dc->y_t + 2));
					break;
				case VK_NEXT:
					dc->StepMonth(hWnd, 1);
					break;
				case VK_PRIOR:
					dc->StepMonth(hWnd, 0);
					break;
				case VK_DELETE:
					SendMessage(hWnd, WM_RBUTTONDOWN, 0, MAKEWPARAM(dc->Left + 5, dc->Top + 5));
					break;
			}
			break;
		case WM_PAINT:
			dc->OnPaint(hWnd);
			break;
		case WM_LBUTTONDBLCLK:
			if(dc->IsDayBar(LOWORD(lParam), HIWORD(lParam)))
  				SendMessage(GetParent(hWnd), WM_COMMAND, MAKEWPARAM(IDOK, 0), (long)hWnd);
			else
				SendMessage(hWnd, WM_LBUTTONDOWN, wParam, lParam);
			break;
		case WM_RBUTTONDOWN: dc->OnRButtonDown(hWnd); break;
		case WM_MOUSEMOVE:   dc->OnMouseMove(hWnd, wParam, lParam); break;
		case WM_LBUTTONDOWN: dc->OnLButtonDown(hWnd, lParam); break;
		//case WM_LBUTTONUP:   dc->OnLButtonDown(hWnd, lParam); break;
		default: return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}
//
// Days
//
void TCalendar::SendToEditBox(LDATE d1, LDATE d2, int need_check)
{
	DateRange period;
	period.Set(d1, d2);
	if(need_check)
		period.CheckAndSwap();
	// @v9.1.5 char s[64];
	// @v9.1.5 periodfmt(&period, s);
	// @v9.1.5 HWND   edit = GetDlgItem(parent_hWnd, CTL_CALENDAR_PERIODEDIT);
	// @v9.1.5 SendMessage(edit, WM_SETTEXT, 0, (LPARAM)s);
	// @v9.1.5 {
	SString temp_buf;
	temp_buf.Cat(period, 0).Transf(CTRANSF_INNER_TO_OUTER);
	TView::SSetWindowText(GetDlgItem(parent_hWnd, CTL_CALENDAR_PERIODEDIT), temp_buf);
	// } @v9.1.5 
}
//
// Weeks
//
//
// Quartals
//
void TCalendar::SendToEditBox(int q1, int y1, int q2, int y2)
{
	if(y2 < y1 || (y2 == y1 && q2 < q1)) {
		Exchange(&q1, &q2);
		Exchange(&y1, &y2);
	}
	char   qt1[16], qt2[16];
	memzero(qt1, sizeof(qt1));
	memzero(qt2, sizeof(qt2));
	if(q1 == 1)
		qt1[0] = 'I';
	else if(q1 == 2)
		qt1[0] = qt1[1] = 'I';
	else if(q1 == 3)
		qt1[0] = qt1[1] = qt1[2] = 'I';
	else if(q1 == 4) {
		qt1[0] = 'I';
		qt1[1] = 'V';
	}
	if(q2 == 1)
		qt2[0] = 'I';
	else if(q2 == 2)
		qt2[0] = qt2[1] = 'I';
	else if(q2 == 3)
		qt2[0] = qt2[1] = qt2[2] = 'I';
	else if(q2 == 4) {
		qt2[0] = 'I';
		qt2[1] = 'V';
	}
	SString s;
	s.Cat(qt1).CatChar('/').Cat(y1);
	if(y1 != y2 || q1 != q2)
		s.CatCharN('.', 2).Cat(qt2).CatChar('/').Cat(y2);
	// @v9.1.5 HWND   edit = GetDlgItem(parent_hWnd, CTL_CALENDAR_PERIODEDIT);
	// @v9.1.5 SendMessage(edit, WM_SETTEXT, 0, (LPARAM)(const char *)s);
	TView::SSetWindowText(GetDlgItem(parent_hWnd, CTL_CALENDAR_PERIODEDIT), s); // @v9.1.5
}
//
// Monthes
//
void TCalendar::SendToEditBox(int m1, int y1, int m2, int y2, int opt)
{
	LDATE  dd1 = encodedate(1, m1, y1);
	LDATE  dd2 = encodedate(1, m2, y2);
	if(dd1 > dd2)
		Exchange(&dd1.v, &dd2.v);
	SString s;
	s.Cat(dd1.month()).CatChar('/').Cat(dd1.year());
	if(dd1 != dd2)
		s.CatCharN('.', 2).Cat(dd2.month()).CatChar('/').Cat(dd2.year());
	// @v9.1.5 HWND   edit = GetDlgItem(parent_hWnd, CTL_CALENDAR_PERIODEDIT);
	// @v9.1.5 SendMessage(edit, WM_SETTEXT, 0, (LPARAM)(const char *)s);
	TView::SSetWindowText(GetDlgItem(parent_hWnd, CTL_CALENDAR_PERIODEDIT), s); // @v9.1.5 
}
//
// Years
//
void TCalendar::SendToEditBox(int y1, int y2)
{
	if(y1 > y2)
		Exchange(&y1, &y2);
	SString s;
	if(y1 == y2)
		s.Cat(y1);
	else
		s.Cat(y1).CatCharN('.', 2).Cat(y2);
	TView::SSetWindowText(GetDlgItem(parent_hWnd, CTL_CALENDAR_PERIODEDIT), s); // @v9.1.5 
}

const char * GetCalCtrlSignature(int type)
{
	return type ? "papyruscalendarperiod" : "papyruscalendardate";
}
//
// SetupCalCtrl
//
void SLAPI SetupCalCtrl(int buttCtlID, TDialog * pDlg, uint editCtlID, uint T)
{
	struct CalButtonWndEx {
		CalButtonWndEx(TDialog * pDlg, uint editCtlId, uint calType, WNDPROC fPrevWndProc)
		{
			STRNSCPY(Signature, GetCalCtrlSignature(calType));
			Dlg = pDlg;
			EditID = editCtlId;
			CalType = calType;
			PrevWndProc = fPrevWndProc;
		}
		static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
		{
			CalButtonWndEx * p_cbwe = (CalButtonWndEx *)TView::GetWindowUserData(hWnd);
			switch(message) {
				case WM_DESTROY:
					TView::SetWindowUserData(hWnd, (void *)0);
					if(p_cbwe->PrevWndProc) {
						TView::SetWindowProp(hWnd, GWLP_WNDPROC, p_cbwe->PrevWndProc);
					}
					delete p_cbwe;
					return 0;
				case WM_LBUTTONUP:
					if(p_cbwe->CalType) {
						TPeriodCalendar * pc = new TPeriodCalendar(p_cbwe->Dlg, p_cbwe->EditID);
						if(pc) {
							pc->Show();
							delete pc;
						}
					}
					else {
						TDateCalendar * pc = new TDateCalendar(p_cbwe->Dlg, p_cbwe->EditID);
						if(pc) {
							pc->ShowCalendar(0);
							delete pc;
						}
					}
					break;
			}
			return CallWindowProc(p_cbwe->PrevWndProc, hWnd, message, wParam, lParam);
		}
		char   Signature[24];
		TDialog * Dlg;
		uint   EditID;
		uint   CalType;
		WNDPROC PrevWndProc;
	};
	HWND   hwnd = GetDlgItem(pDlg->H(), buttCtlID);
	if(hwnd && T >= 0 && T <= 6) {
		HWND   hwnd_input = GetDlgItem(pDlg->H(), editCtlID);

		SString cls_name;
		TView::SGetWindowClassName(hwnd, cls_name);

		static HBITMAP hbm_daterange = 0; // @global @threadsafe
		static HBITMAP hbm_calendar = 0;  // @global @threadsafe
		const int cal_type = BIN(oneof3(T, 1, 2, 3)); // 1 - period, 0 - date
		CalButtonWndEx * p_cbwe = new CalButtonWndEx(pDlg, editCtlID, cal_type, (WNDPROC)TView::GetWindowProp(hwnd, GWLP_WNDPROC));
		TView::SetWindowUserData(hwnd, p_cbwe);
		TView::SetWindowProp(hwnd, GWLP_WNDPROC, CalButtonWndEx::WndProc);
		// @v9.1.11 {
		{
			RECT r, cr;
			if(::GetWindowRect(hwnd_input, &r)) {
				::GetWindowRect(hwnd, &cr);
				POINT p;
				p.x = r.right;
				p.y = r.top;
				::ScreenToClient(pDlg->H(), &p);
				::MoveWindow(hwnd, p.x, p.y, cr.right-cr.left, r.bottom-r.top, TRUE);
			}
		}
		// } @v9.1.11 
		if(!hbm_daterange || !hbm_calendar) {
			ENTER_CRITICAL_SECTION
			SETIFZ(hbm_daterange, APPL->LoadBitmap(IDB_DATERANGE));
			SETIFZ(hbm_calendar,  APPL->LoadBitmap(IDB_CALENDAR));
			LEAVE_CRITICAL_SECTION
		}
		SendMessage(hwnd, BM_SETIMAGE, IMAGE_BITMAP, (long)(oneof3(T, 1, 2, 3) ? hbm_daterange : hbm_calendar));
	}
}

int ExecDateCalendar(/*HWND*/uint32 hParent, LDATE * pDate)
{
	int    ok = -1;
	LDATE  dt = pDate ? *pDate : getcurdate_();
	TDateCalendar * p_pc = new TDateCalendar(0, 0);
	if(p_pc) {
		p_pc->setDTS(dt);
		p_pc->ShowCalendar((HWND)hParent);
		if(p_pc->GetRetCmd() == cmOK) {
			p_pc->getDTS(&dt);
			ok = 1;
		}
		delete p_pc;
	}
	ASSIGN_PTR(pDate, dt);
	return ok;
}
//
// TPeriodCalendar
//
void TPeriodCalendar::UpdatePeriod()
{
	DateRange range;
	char   period_buf[128];
	strtoperiod(Period, &range, 0);
	const  LDATE cur_dt = getcurdate_();
	D1 = checkdate(range.low, 0) ? range.low : cur_dt;
	D2 = checkdate(range.upp, 0) ? range.upp : cur_dt;
	range.Set(D1, D2);
	int    r = periodfmtex(&range, period_buf, sizeof(period_buf));
	Period = period_buf;
	if(r == PRD_DAY)
		seltype = CTL_CALENDAR_DAYS;
	else if(r == PRD_WEEK)
		seltype = CTL_CALENDAR_WEEKS;
	else if(r == PRD_MONTH)
		seltype = CTL_CALENDAR_MONTHS;
	else if(r == PRD_QUART)
		seltype = CTL_CALENDAR_QUARTALS;
	else if(r == PRD_ANNUAL)
		seltype = CTL_CALENDAR_YEARS;
	else
		seltype = -1;
}

int TPeriodCalendar::SelectByFastPrd(HWND hWnd)
{
	long   fastprd_sel = 0;
	HWND   cbx_hwnd = GetDlgItem(hWnd, CTL_CALENDAR_FASTPRD);
	if(P_Inner && cbx_hwnd && (fastprd_sel = SendMessage(cbx_hwnd, CB_GETCURSEL, 0, 0)) != CB_ERR) {
		LDATE cur_dt = ZERODATE, d1 = ZERODATE, d2 = ZERODATE;
		getcurdate(&cur_dt);
		if(oneof2(fastprd_sel, PPFASTPRD_TODAY, PPFASTPRD_YESTERDAY)) {
			if(fastprd_sel == PPFASTPRD_YESTERDAY)
				plusdate(&cur_dt, -1, 0);
			d1 = d2 = cur_dt;
		}
		else if(fastprd_sel == PPFASTPRD_LAST7DAYS) {
			plusdate(&(d1 = cur_dt), -7, 0);
			plusdate(&(d2 = cur_dt), -1, 0);
		}
		else if(oneof2(fastprd_sel, PPFASTPRD_LASTWEEK, PPFASTPRD_LASTWORKWEEK)) {
			int plus_days = (fastprd_sel == PPFASTPRD_LASTWORKWEEK) ? 4 : 6;
			plusdate(&cur_dt, 1 - (7 + dayofweek(&cur_dt, 1)), 0);
			d1 = cur_dt;
			plusdate(&(d2 = cur_dt), plus_days, 0);
		}
		else if(oneof2(fastprd_sel, PPFASTPRD_THISMONTH, PPFASTPRD_LASTMONTH)) {
			if(fastprd_sel == PPFASTPRD_LASTMONTH)
				plusperiod(&cur_dt, PRD_MONTH, -1, 0);
			(d1 = cur_dt).setday(1);
			(d2 = cur_dt).setday(dayspermonth(cur_dt.month(), cur_dt.year()));
		}
		if(d1 != ZERODATE && d2 != ZERODATE) {
			P_Inner->D  = d1;
			P_Inner->D1 = d1;
			P_Inner->D2 = d2;
			P_Inner->Validate();
			P_Inner->SetupCalendar();
			P_Inner->SendToEditBox(P_Inner->D1, P_Inner->D2, 0);
			InvalidateRect(hWnd, NULL, false);
		}
	}
	return 1;
}

static INT_PTR CALLBACK PeriodWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int    ret = 0;
	HWND   edit, but1;
	SString temp_buf;
	TPeriodCalendar * pc = 0;
	switch(message) {
		case WM_INITDIALOG:
			pc = (TPeriodCalendar *)lParam;
			TView::SetWindowUserData(hWnd, pc);
			if(pc && pc->P_Dlg && pc->DateCtlID) {
				HWND   h_ctl = GetDlgItem(pc->P_Dlg->H(), pc->DateCtlID);
				if(h_ctl) {
					RECT   rect, this_rect;
					GetWindowRect(h_ctl, &rect);
					GetWindowRect(hWnd, &this_rect);
					int    sizey = this_rect.bottom - this_rect.top;
					int    sy = GetSystemMetrics(SM_CYFULLSCREEN);
					int    y = rect.bottom;
					if((y + sizey) > sy)
						y = (rect.top < sizey) ? (sy - sizey) : (rect.top - sizey);
					MoveWindow(hWnd, rect.left, y, this_rect.right - this_rect.left, sizey, 0);
				}
				if(h_ctl = GetDlgItem(hWnd, CTL_CALENDAR_FASTPRD)) {
					StringSet ss(";");
					PPLoadText(PPTXT_FASTPRD, temp_buf);
					temp_buf.Transf(CTRANSF_INNER_TO_OUTER);
					ss.setBuf(temp_buf, temp_buf.Len() + 1);
					for(uint p = 0; ss.get(&p, (temp_buf = 0)) > 0;)
						SendDlgItemMessage(hWnd, CTL_CALENDAR_FASTPRD, CB_ADDSTRING, 0, (LPARAM)(const char*)temp_buf);
					// SendDlgItemMessage(hwndDlg, CTL_WPRINT_REPORT, CB_SETCURSEL, 0, 0);
				}
				TView::PreprocessWindowCtrlText(hWnd); // @v9.1.1
			}
			break;
		case WM_SHOWWINDOW:
			pc = (TPeriodCalendar *)TView::GetWindowUserData(hWnd);
			edit = GetDlgItem(hWnd, CTL_CALENDAR_PERIODEDIT);
			// @v9.1.5 ::SendMessage(edit, WM_SETTEXT, 0, (LPARAM)pc->Period);
			TView::SSetWindowText(edit, pc->Period); // @v9.1.5
			pc->UpdatePeriod();
			but1 = GetDlgItem(hWnd, pc->seltype); // CTL_CALENDAR_DAYS CTL_CALENDAR_QUARTALS
			SendMessage(but1, BM_SETCHECK, (WPARAM) BST_CHECKED, 0);
			if(!pc->P_Inner) {
				pc->P_Inner = new TCalendarP(pc->D1, pc->D2);
				if(pc->P_Inner) {
					switch(pc->seltype){
						case CTL_CALENDAR_DAYS:     pc->P_Inner->seltype = SEL_DAYS;     break;
						case CTL_CALENDAR_WEEKS:    pc->P_Inner->seltype = SEL_WEEKS;    break;
						case CTL_CALENDAR_MONTHS:   pc->P_Inner->seltype = SEL_MONTHS;   break;
						case CTL_CALENDAR_QUARTALS: pc->P_Inner->seltype = SEL_QUARTALS; break;
						case CTL_CALENDAR_YEARS:    pc->P_Inner->seltype = SEL_YEARS;    break;
					}
					pc->P_Inner->ShowCalendar(hWnd);
				}
			}
			break;
		case WM_COMMAND:
			{
				int    on_param = 0;
				int    on_call = 0; // 1 - OnSelectionType, 2 - OnResetButton
				pc = (TPeriodCalendar *)TView::GetWindowUserData(hWnd);
				switch(LOWORD(wParam)) {
					case IDCANCEL:
						ZDELETE(pc->P_Inner);
						EndDialog(hWnd, 0);
						ret = 1;
						break;
					case IDOK:
						edit = GetDlgItem(hWnd, CTL_CALENDAR_PERIODEDIT);
						// @v9.1.5 SendMessage(edit, WM_GETTEXT, (WPARAM)30, (LPARAM)pc->Period);
						TView::SGetWindowText(edit, pc->Period); // @v9.1.5
						CALLPTRMEMB(pc->P_Dlg, setCtrlString(pc->DateCtlID, pc->Period));
						ZDELETE(pc->P_Inner);
						EndDialog(hWnd, 0);
						ret = 1;
						break;
					case CTL_CALENDAR_DAYS:       on_call = 1; on_param = SEL_DAYS; ret = 1; break;
					case CTL_CALENDAR_WEEKS:      on_call = 1; on_param = SEL_WEEKS; ret = 1; break;
					case CTL_CALENDAR_MONTHS:     on_call = 1; on_param = SEL_MONTHS;   break;
					case CTL_CALENDAR_QUARTALS:   on_call = 1; on_param = SEL_QUARTALS; break;
					case CTL_CALENDAR_YEARS:      on_call = 1; on_param = SEL_YEARS;    break;
					case CTL_CALENDAR_LEFTRESET:  on_call = 2; on_param = -1; break;
					case CTL_CALENDAR_RESET:      on_call = 2; on_param = 0;  break;
					case CTL_CALENDAR_RIGHTRESET: on_call = 2; on_param = 1;  break;
					case CTL_CALENDAR_FASTPRD:
						if(HIWORD(wParam) == CBN_SELENDOK && pc && pc)
							pc->SelectByFastPrd(hWnd);
				}
				if(on_call && pc->P_Inner) {
					if(on_call == 1)
						pc->P_Inner->OnSelectionType(on_param);
					else if(on_call == 2)
						pc->P_Inner->OnResetButton(on_param);
					SendDlgItemMessage(hWnd, CTL_CALENDAR_FASTPRD, CB_SETCURSEL, -1, 0);
				}
			}
			break;
		}
	return ret;
}
//
// TCalendarP
//
int TCalendarP::OnSelectionType(int aSelType)
{
	if(aSelType == SEL_DAYS) {
		seltype = SEL_DAYS;
		SendToEditBox(D1, D2, 0);
	}
	else if(aSelType == SEL_WEEKS) {
		long days = 0;
		LDATE dt1 = ZERODATE, dt2 = ZERODATE, beg_dt = ZERODATE;
		seltype = SEL_WEEKS;
		beg_dt = encodedate(1, 1, 1);
		days = ((diffdate(D1, beg_dt) - 1) / 7) * 7 + 1;
		dt1 = plusdate(beg_dt, days);
		if(D2 == ZERODATE || D2 == D1)
			dt2 = plusdate(dt1, 6);
		else {
			days = ((diffdate(D2, beg_dt) -1) / 7) * 7 + 1;
			dt2 = plusdate(beg_dt, days);
		}
		SendToEditBox(dt1, dt2, 1);
	}
	else if(aSelType == SEL_MONTHS) {
		seltype = SEL_MONTHS;
		SendToEditBox(D1.month(), D1.year(), D1.month(), D1.year(), 0);
	}
	else if(aSelType == SEL_QUARTALS) {
		seltype = SEL_QUARTALS;
		SendToEditBox((D1.month() - 1) / 3 + 1, D1.year(), (D1.month() - 1) / 3 + 1, D1.year());
	}
	else if(aSelType == SEL_YEARS) {
		seltype = SEL_YEARS;
		SendToEditBox(D1.year(), D1.year());
	}
	else
		return 0;
	InvalidateRect(c_hWnd, NULL, false);
	SetFocus(c_hWnd);
	return 1;
}

int TCalendarP::OnResetButton(int kind /* -1 - left, 0 - all, 1 - right */)
{
	int    ok = -1;
	if(kind == -1) {
		if(D1 && D2) {
			if(D1 > D2)
				D2 = D1;
			D1 = ZERODATE;
			SendToEditBox(D1, D2, 0);
			ok = 1;
		}
	}
	else if(kind == 0) {
		if(D1 || D2) {
			D1 = D2 = ZERODATE;
			SendToEditBox(D1, D2, 0);
			ok = 1;
		}
	}
	else if(kind == 1) {
		if(D1 && D2) {
			if(D1 > D2)
				D1 = D2;
			D2 = ZERODATE;
			SendToEditBox(D1, D2, 0);
			ok = 1;
		}
	}
	else
		ok = 0;
	if(ok > 0) {
		InvalidateRect(c_hWnd, NULL, false);
		SetFocus(c_hWnd);
		SelStarted1 = 0;
	}
	return ok;
}

void TCalendarP::ShowCalendar(HWND hwParent)
{
	parent_hWnd = hwParent;
	Validate();
	SetupCalendar();
	LPCTSTR p_classname = _T("PpyPeriodCalendar");
	{
		WNDCLASSEX wc;
		MEMSZERO(wc);
		wc.cbSize = sizeof(wc);
		wc.lpszClassName = p_classname;
		wc.hInstance = TProgram::GetInst();
		wc.lpfnWndProc = (WNDPROC)CalendarWndProc;
		wc.style = CS_DBLCLKS;
		wc.hCursor = LoadCursor(NULL, IDC_ARROW);
		wc.hbrBackground = (HBRUSH)GetStockObject(LTGRAY_BRUSH);
		::RegisterClassEx(&wc); // @unicodeproblem
	}
	c_hWnd = ::CreateWindow(p_classname, NULL, WS_VISIBLE | WS_CHILD | WS_TABSTOP | WS_GROUP,
		8, 85, 188, 200, parent_hWnd, NULL, TProgram::GetInst(), this); // @unicodeproblem
	Top  = 80;
	Left = 7;
	::ShowWindow(c_hWnd, SW_SHOWNORMAL);
	::SetFocus(c_hWnd);
	TimerFreq = 50;
	SetTimer(c_hWnd, 1, 50, 0);
}
//
//
//
void SLAPI ShowCalCtrl(int buttCtlID, TDialog * pDlg, int show)
{
	ShowWindow(GetDlgItem(pDlg->H(), buttCtlID), show ? SW_SHOW : SW_HIDE);
}
//
//
//
int TDialog::SetupCalendar(uint calCtlID, uint inputCtlID, int kind)
{
	SetupCalCtrl(calCtlID, this, inputCtlID, kind);
	return 1;
}

int TDialog::SetupCalDate(uint calCtlID, uint inputCtlID)
{
	return SetupCalendar(calCtlID, inputCtlID, 4);
}

int TDialog::SetupCalPeriod(uint calCtlID, uint inputCtlID)
{
	return SetupCalendar(calCtlID, inputCtlID, 1);
}
