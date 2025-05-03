// CALENDAR.CPP
// Copyright (c) A.Fedotkov, A.Sobolev, A.Starodub 2001, 2002, 2003, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2016, 2017, 2018, 2019, 2020, 2021, 2022, 2023, 2024, 2025
// @codepage UTF-8
//
#include <pp.h>
#pragma hdrstop

//#define USE_NEW_CALENDAR 1 // @v11.2.4

const char * GetCalCtrlSignature(int type) { return type ? "papyruscalendarperiod" : "papyruscalendardate"; }

//#if (USE_NEW_CALENDAR == 0) // @v11.2.4 {

static LRESULT CALLBACK CalendarWndProc(HWND, UINT, WPARAM, LPARAM);
static INT_PTR CALLBACK PeriodWndProc(HWND, UINT, WPARAM, LPARAM);

class TCalendar {
public:
	TCalendar()
	{
		THISZERO();
	}
	void   SetupCalendar();
	void   Normalize();
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
	int    c_i;
	int    c_j;
	int    c_minfirst;
	int    c_maxlast;
	int    c_maxrow;
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

// @v10.7.10 static const _TCHAR __Days[7][3] = {_T("Пн"), _T("Вт"), _T("Ср"), _T("Чт"), _T("Пт"), _T("Сб"), _T("Вс") };

class TDateCalendar : public TCalendar {
public:
	static INT_PTR CALLBACK CalendarDlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam); // @callback(DLGPROC)

	TDateCalendar(TDialog * pDlg, uint pDateCtlID) : TCalendar(), RetCmd(0)
	{
		LDATE d;
		//PeriodSelect = 0;
		P_Dlg = pDlg;
		parent_hWnd = P_Dlg ? P_Dlg->H() : 0;
		DateCtlID = pDateCtlID;
		IsLarge = BIN(P_Dlg && P_Dlg->CheckFlag(TDialog::fLarge));
		Top  = IsLarge ? 144 : 80;
		Left = 10;
		if(DateCtlID == -1 || !P_Dlg || !P_Dlg->getCtrlData(DateCtlID, &D)) {
			D = getcurdate_();
			Normalize();
		}
		else if(P_Dlg && P_Dlg->getCtrlData(DateCtlID, &D)) {
			d = getcurdate_();
			if(D.year() < 1800 || D.year() > 5000)
				D.setyear(d.year());
			if(D.month() < 1 || D.month() > 12)
				D.setmonth(d.month());
			if(D.day() < 1 || D.day() > 31)
				D.setday(d.day());
			Normalize();
		}
		SetupCalendar();
	}
	int    setDTS(LDATE dt)
	{
		D = dt;
		y_firstyear = D.year() - 2;
		Normalize();
		SetupCalendar();
		::InvalidateRect(c_hWnd, 0, true);
		::SetFocus(c_hWnd);
		return 1;
	}
	int    getDTS(LDATE * pDt)
	{
		ASSIGN_PTR(pDt, D);
		return 1;
	}
	void   ShowCalendar(HWND hParent)
	{
		TView * p_ctl = P_Dlg ? P_Dlg->getCtrlView(DateCtlID) : 0;
		if(!p_ctl || !p_ctl->IsInState(sfDisabled)) {
			int r = APPL->DlgBoxParam(IsLarge ? DLGW_CALENDAR_L : DLGW_CALENDAR,
				NZOR(hParent, parent_hWnd), TDateCalendar::CalendarDlgProc, reinterpret_cast<LPARAM>(this));
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
	int    GetRetCmd() const { return RetCmd; }
private:
	void   DrawBackground(HDC hdc);
	void   DrawDayOfWeekHeader(HDC hdc);
	void   DrawMonthGrid(HDC hdc);
	void   DrawMonthCells(HDC hdc, int decr);
	void   PaintMonthRect(TCanvas & rCanv, SPoint2S p, uint j, COLORREF color, COLORREF textColor);
	void   DrawSelectedYearRect(HDC hdc, int brushType, int);
	void   InvalidateMonthBar(HWND);
	int    IsYearMarker(const SPoint2S pt) const;
	int    IsYearBar(const SPoint2S pt) const;
	int    IsMonthBar(const SPoint2S pt) const;
	int    SelectYear(HWND hWnd, int n /* 1..NumYearInBar */);
	int    ScrollYear(HWND hWnd, int dir);
	void   SelectMonth(HWND hWnd, const SPoint2S pt);
	void   SelectQuart(HWND hWnd, const SPoint2S pt);
	void   SelectDay(HWND hWnd, int x, int y);
	void   SelectWeek(HWND hWnd, SPoint2S pt);
	int    RetCmd;
};

class TCalendarP : public TCalendar {
public:
	TCalendarP(LDATE d1, LDATE d2) : TCalendar()
	{
		PeriodSelect = 1;
		D  = ValidDateOr(d1, getcurdate_());
		D1 = d1;
		D2 = d2;
		Normalize();
		SetupCalendar();
	}
	~TCalendarP()
	{
		::DestroyWindow(c_hWnd);
	}
	void   ShowCalendar(HWND hwParent);
	void   CloseCalendar()
	{
	}
	void   OnSelectionType(int aSelType);
	int    OnResetButton(int kind /* -1 - left, 0 - all, 1 - right */);
};

class TPeriodCalendar {
public:
	TPeriodCalendar(TDialog * pDlg, uint dateCtlID) : P_Inner(0), P_Dlg(pDlg), DateCtlID(dateCtlID), SelType(-1)
	{
		CALLPTRMEMB(P_Dlg, getCtrlString(DateCtlID, Period));
		UpdatePeriod();
	}
	void   Show()
	{
		APPL->DlgBoxParam(DLGW_PERIODCALENDAR, P_Dlg ? P_Dlg->H() : 0, PeriodWndProc, (LPARAM)this);
	}
	void   UpdatePeriod()
	{
		DateRange range;
		char   period_buf[128];
		strtoperiod(Period, &range, 0);
		const  LDATE cur_dt = getcurdate_();
		D1 = ValidDateOr(range.low, cur_dt);
		D2 = ValidDateOr(range.upp, cur_dt);
		range.Set(D1, D2);
		const int r = periodfmtex(&range, period_buf, sizeof(period_buf));
		Period = period_buf;
		switch(r) {
			case PRD_DAY: SelType = CTL_CALENDAR_DAYS; break;
			case PRD_WEEK: SelType = CTL_CALENDAR_WEEKS; break;
			case PRD_MONTH: SelType = CTL_CALENDAR_MONTHS; break;
			case PRD_QUART: SelType = CTL_CALENDAR_QUARTALS; break;
			case PRD_ANNUAL: SelType = CTL_CALENDAR_YEARS; break;
			default: SelType = -1; break;
		}
	}
	void   SelectByFastPrd(HWND hWnd);

	int    SelType;
	TDialog * P_Dlg;
	uint   DateCtlID;
	LDATE  D1;
	LDATE  D2;
	SString Period;
	TCalendarP * P_Inner;
};

#define C_SELCOL     RGB(0, 0, 0)       // Цвет выделения //
#define	C_SELPCOL    RGB(90, 90, 90)    // Цвет выделения периода
#define	C_BACKCOL    RGB(223, 223, 223) // Цвет фона
#define	M_SELCOL     RGB(255, 255, 255) // Цвет шрифта выделения месяца
#define	M_DEFCOL     RGB(0, 0, 0)       // Цвет шрифта месяца
#define	Y_TEXTCOL    RGB(0, 0, 0)       // Цвет шрифта года
#define	C_TEXT2COL   RGB(255, 255, 100) // Цвет шрифта дня начала периода
#define	C_TEXTSMCOL  RGB(0, 0, 255)     // Цвет шрифта месяца, входящего в период
#define C_TEXTSMCCOL RGB(0, 130, 255)   // Цвет шрифта месяца, входящего в период и являющегося текущим

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

void TCalendar::Normalize()
{
	NDays = D.dayspermonth();
	LDATE  dt = D;
	dt.setday(1);
	FirstMonthDow = dayofweek(&dt, 1)-1;
}

void TCalendar::SetupCalendar()
{
	//RowCount = 2;
	int    i, j, count = 1;
	//_TCHAR s[3], t[3];
	SString temp_buf;
	SStringU temp_buf_u; // @v10.7.10
	static const char * p_wd_symb[] = { "monday_s", "tuesday_s", "wednesday_s", "thursday_s", "friday_s", "saturday_s", "sunday_s" }; // @v10.7.10
	STATIC_ASSERT(SIZEOFARRAY(p_wd_symb) == 7); // @v10.7.10
	for(j = 0; j <= SIZEOFARRAY(p_wd_symb); j++) {
		// @v10.7.10 sstrcpy(C[0][j], __Days[j]);
		// @v10.7.10 {
		PPLoadString(p_wd_symb[j], temp_buf);
		temp_buf_u.CopyFromUtf8(temp_buf.Transf(CTRANSF_INNER_TO_UTF8));
		STRNSCPY(C[0][j], temp_buf_u);
		// } @v10.7.10 
	}
	for(i = 1; i <= 6; i++) {
		for(j = 0; j <= 6; j++) {
			if((i == 1 && FirstMonthDow > j) || count > NDays)
				sstrcpy(C[i][j], _T("  "));
			else {
				temp_buf.Z();
				if(count == 1)
					c_minfirst = j;
				if(count == NDays) {
					c_maxlast = j;
					c_maxrow = i;
				}
				if(count < 10) {
					//sstrcpy(s, _T(" "));
					temp_buf.Space();
				}
				else {
					//s[0] = 0;
				}
				//strcat(s, _itoa(count, t, 10)); // @unicodeproblem
				temp_buf.Cat(count);
				sstrcpy(C[i][j], /*s*/SUcSwitch(temp_buf));
				if(count == D.day()) {
					c_j = j;
					c_i = i;
				}
				count++;
			}
		}
		/*if(C[i][0][1] != ' ' && i != 1)
			RowCount++;*/
	}
}

void TDateCalendar::CloseCalendar()
{
	CALLPTRMEMB(P_Dlg, setCtrlData(DateCtlID, &D));
}

int TDateCalendar::OnTodaySelection()
{
	return setDTS(getcurdate_());
}

/*static*/INT_PTR CALLBACK TDateCalendar::CalendarDlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) // @callback(DLGPROC)
{
	TDateCalendar * dc = 0;
	RECT r;
	switch(message) {
		case WM_INITDIALOG:
			dc = reinterpret_cast<TDateCalendar *>(lParam);
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
			TView::PreprocessWindowCtrlText(hWnd);
			break;
		case WM_CREATE:
			dc = static_cast<TDateCalendar *>(TView::GetWindowUserData(hWnd));
			dc->y_firstyear = dc->D.year() - 2;
			break;
		case WM_SHOWWINDOW:
			if(wParam) {
				dc = static_cast<TDateCalendar *>(TView::GetWindowUserData(hWnd));
				int  left = dc->IsLarge ? 14 : 8;
				int  top  = dc->IsLarge ? 14 : 10;
				int  from_right  = dc->IsLarge ? 28 : 16;
				int  from_bottom = dc->IsLarge ? 75 : 44;
				//const char * p_classname = "PpyDateCalendar";
				LPCTSTR p_classname = _T("PpyDateCalendar");
				{
					WNDCLASSEX wc;
					INITWINAPISTRUCT(wc);
					wc.lpszClassName = p_classname;
					wc.hInstance = TProgram::GetInst();
					wc.lpfnWndProc = static_cast<WNDPROC>(CalendarWndProc);
					wc.style = CS_DBLCLKS;
					wc.hCursor = LoadCursor(NULL, IDC_ARROW);
					wc.hbrBackground = static_cast<HBRUSH>(GetStockObject(LTGRAY_BRUSH));
					::RegisterClassEx(&wc);
				}
				::GetClientRect(hWnd, &r);
				dc->c_hWnd = ::CreateWindowEx(0, p_classname, NULL, WS_VISIBLE | WS_CHILD | WS_TABSTOP | WS_GROUP,
					left, top, r.right - from_right, r.bottom - from_bottom, hWnd, NULL, TProgram::GetInst(), dc);
				::ShowWindow(dc->c_hWnd, SW_SHOWNORMAL);
				::SetFocus(dc->c_hWnd);
			}
			break;
		case WM_COMMAND:
			dc = static_cast<TDateCalendar *>(TView::GetWindowUserData(hWnd));
			switch(LOWORD(wParam)) {
				case IDCANCEL:
					::KillTimer(dc->c_hWnd, 1);
					::DestroyWindow(dc->c_hWnd);
					::EndDialog(hWnd, 0);
					dc->RetCmd = cmCancel;
					return 1;
				case IDOK:
					::KillTimer(dc->c_hWnd, 1);
					::DestroyWindow(dc->c_hWnd);
					::EndDialog(hWnd, 1);
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
		::SetTimer(c_hWnd, 1, 300, 0);
	}
	else {
		TimerFreq = 52;
		::SetTimer(c_hWnd, 1, 52, 0);
	}
}

const int C_CELLH = 16;
const int C_CELLW = 25;
const int M_CELLW = C_CELLW * 7 / 6;
const int M_CELLH = 15;
const int M_DIFFY = 45;
// const int Y_DIFFY = 70;

void TDateCalendar::PaintMonthRect(TCanvas & rCanv, SPoint2S p, uint j, COLORREF color, COLORREF textColor)
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
	HPEN   oldpen = static_cast<HPEN>(SelectObject(hdc, pen));
	HBRUSH br     = CreateSolidBrush(RGB(170, 170, 170));
	HBRUSH oldbr  = static_cast<HBRUSH>(SelectObject(hdc, br));
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
				SPoint2S p, txt_sz;
				p.Set(Left + j * c_cell_w, Top  + i * c_cell_h);
				int    tmpd = satoi(C[i][j]);
				int    is_year = BIN(D1.year() >= 1970 || D2.year() < 2500);
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
					PaintMonthRect(canv, p, c_j, C_SELCOL, GetColorRef(SClrWhite));
				if(PeriodSelect == 1) {
					if(i > 0) {
						if(dd == dd1 || dd == dd2 || (is_year && tmpd == D.day()))
							canv.SetTextColor(C_TEXT2COL);
					}
				}
				txt_sz = canv.GetTextSize(SUcSwitch(C[i][j])); // @unicodeproblem
				p.x += (c_cell_w - txt_sz.x) / 2;
				p.y += (c_cell_h - txt_sz.y) / 2;
				canv.TextOut_(p, SUcSwitch(C[i][j])); // @unicodeproblem
			}
}

void TDateCalendar::DrawSelectedYearRect(HDC hdc, int brushType, int i)
{
	TCanvas canv(hdc);
	canv.SelectObjectAndPush(GetStockObject(brushType));
	SPoint2S p;
	p.Set(Left + (y_br - y_bl - y_w) / 2, y_t);
	TRect r(p.x + (i - y_firstyear - 2) * y_w, p.y - 1, p.x + (i - y_firstyear - 1) * y_w, p.y + y_th + 2);
	canv.Rectangle(r);
	canv.PopObject();
}

void TDateCalendar::DrawMonthCells(HDC hdc, int decr)
{
	const int  m_cell_w = (y_br - y_bl) / 6;
	const int  m_cell_h = M_CELLH * y_th / 13;
	const int  m_diff_y = M_DIFFY * y_th / 13;
	::MoveToEx(hdc, Left, Top - m_diff_y + m_cell_h - decr, 0);
	::LineTo(hdc, Left + (y_br - y_bl) - 1, Top - (m_diff_y - m_cell_h + decr));
	for(int i = 1; i < 6; i++) {
		::MoveToEx(hdc, Left + (i * m_cell_w - decr), Top - (m_diff_y + 1), 0);
		::LineTo(hdc, Left + (i * m_cell_w - decr), Top - (m_diff_y - m_cell_h * 2 - 3));
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
	hf1 = static_cast<HFONT>(GetStockObject(ANSI_VAR_FONT));
	hf = CreateFont(IsLarge ? 24 : 8, 0, 0, 0, FW_NORMAL, 0, 0, 0, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FW_DONTCARE, _T("MS Sans Serif"));
	SelectObject(hdc, hf);
	y_bl = Left;
	GetClientRect(c_hWnd, &c_r);
	y_br = c_r.right - y_bl;
	::GetTextExtentPoint32(hdc, _T("<<"), 2, &ts);
	y_bw = ts.cx;
	y_th = ts.cy;
	y_t  = IsLarge ? 14 : 10;
	y_w  = (y_br - y_bl - y_bw * 2) / 5;
	::TextOut(hdc, y_bl, y_t, _T("<<"), 2);
	::TextOut(hdc, y_br - y_bw, y_t, _T(">>"), 2);

	pen = CreatePen(PS_NULL, 1, 0);
	oldpen = (HPEN)SelectObject(hdc, pen);
	for(i = y_firstyear; (int)i <= y_firstyear + 4; i++) {
		int is_year = BIN(D1.year() >= 1970 || D2.year() < 2500);
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
		s.Z().Cat(i);
		::GetTextExtentPoint32(hdc, SUcSwitch(s), s.Len(), &ts);
		::TextOut(hdc, Left + (y_br - y_bl - y_w) / 2 + (i - y_firstyear - 2) * y_w + (y_w - ts.cx) / 2, y_t, SUcSwitch(s), s.Len());
	}
	SelectObject(hdc, oldpen);
	ZDeleteWinGdiObject(&pen);
	{
		HPEN   bg_pen = CreatePen(PS_SOLID, 1, RGB(127, 127, 127));
		HPEN   old_bg_pen = static_cast<HPEN>(::SelectObject(hdc, bg_pen));
		HBRUSH bg_br = CreateSolidBrush(C_BACKCOL);
		HBRUSH old_bg_br = static_cast<HBRUSH>(::SelectObject(hdc, bg_br));
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
		HPEN   black_pen = ::CreatePen(PS_SOLID, 1, 0);
		HPEN   white_pen = ::CreatePen(PS_SOLID, 1, GetColorRef(SClrWhite));
		HPEN   gray_pen  = ::CreatePen(PS_SOLID, 1, RGB(127, 127, 127));

		oldpen = (HPEN)::SelectObject(hdc, black_pen);
		::MoveToEx(hdc, Left - 1, Top + 7 * c_cell_h - 1, 0);
		::LineTo(hdc, Left - 1, Top);
		::LineTo(hdc, Left - 1 + (y_br - y_bl), Top);

		::SelectObject(hdc, white_pen);
		::MoveToEx(hdc, Left, Top + 7 * c_cell_h - 1, 0);
		::LineTo(hdc, Left + (y_br - y_bl) - 1, Top + 7 * c_cell_h - 1);
		::LineTo(hdc, Left + (y_br - y_bl) - 1, Top - 1);
		//
		// Draw months background rectangle
		//
		::SelectObject(hdc, gray_pen);
		{
			br = ::CreateSolidBrush(RGB(212, 208, 200));
			oldbr = static_cast<HBRUSH>(::SelectObject(hdc, br));
			::Rectangle(hdc, Left - 2, Top - m_diff_y - 3, Left + (y_br - y_bl), Top - m_diff_y + m_cell_h * 2 + 3);
			::SelectObject(hdc, oldbr);
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
		LineTo(hdc, Left - 1, Top - m_diff_y - 2);
		LineTo(hdc, Left + (y_br - y_bl), Top - m_diff_y - 2);
		SelectObject(hdc, oldpen);

		ZDeleteWinGdiObject(&black_pen);
		ZDeleteWinGdiObject(&white_pen);
		ZDeleteWinGdiObject(&gray_pen);
	}
	//
	// Print Month names
	//
	for(i = 1; i <= 12; i++) {
		SGetMonthText(i, MONF_SHORT, temp_buf.Z());
		::GetTextExtentPoint32(hdc, SUcSwitch(temp_buf), 3, &ts); // @unicodeproblem
		x = Left + (((i <= 6) ? i : i - 6) - 1) * m_cell_w;
		(dd1 = D1).setday(1);
		(dd2 = D2).setday(1);
		dd = encodedate(1, i, D.year());
		if(PeriodSelect && (D1.year() >= 1970 || D2.year() < 2500) && ((dd1 <= dd2 && dd <= dd2 && dd >= dd1) || (dd2 < dd1 && dd <= dd1 && dd >= dd2)))
			SetTextColor(hdc, (i == D.month()) ? C_TEXTSMCCOL : C_TEXTSMCOL);
		else
			SetTextColor(hdc, (i == D.month()) ? M_SELCOL : M_DEFCOL);
		::TextOut(hdc, x + (m_cell_w - ts.cx) / 2, Top - ((i <= 6) ? m_diff_y : m_diff_y - m_cell_h - 2), SUcSwitch(temp_buf), 3); // @unicodeproblem
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
		SPoint2S pt;
		pt.setwparam(lParam);
		if((seltype == SEL_DAYS && IsDayBar(pt.x, pt.y)) ||
			(oneof2(seltype, SEL_MONTHS, SEL_QUARTALS) && IsMonthBar(pt)) ||
			(seltype == SEL_YEARS && IsYearBar(pt)))
			::SendMessage(hWnd, WM_LBUTTONDOWN, wParam, lParam);
	}
}

void TDateCalendar::OnLButtonUp(HWND hWnd, LPARAM lParam)
{
	SPoint2S pt;
	pt.setwparam(lParam);
	if(IsMonthBar(pt) || IsYearBar(pt))
		InvalidateRect(hWnd, NULL, false);
}

int TDateCalendar::SelectYear(HWND hWnd, int n /* 1..NumYearInBar */)
{
	D.setyear(y_firstyear + n - 1);
	Normalize();
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
	int    ok = -1;
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
		Normalize();
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
int TDateCalendar::IsYearMarker(const SPoint2S pt) const
{
	if(pt.y >= y_t && pt.y <= (y_t + y_th)) {
		if(pt.x >= y_bl && pt.x <= (y_bl + y_bw))
			return -1;
		else if(pt.x >= (y_br - y_bw) && pt.x <= y_br)
			return 1;
	}
	return 0;
}

int TDateCalendar::IsYearBar(const SPoint2S pt) const
{
	if(pt.x >= (y_bl + y_bw) && pt.x <= (y_br - y_bw) && pt.y >= y_t && pt.y <= (y_t + y_th))
		return 1 + (pt.x - (y_bl + y_bw)) / y_w;
	else
		return 0;
}

int TDateCalendar::IsMonthBar(const SPoint2S pt) const
{
	int  m_cell_h = M_CELLH * y_th / 13;
	int  m_diff_y = M_DIFFY * y_th / 13;
	return BIN((pt.x > Left + 2) && (pt.x < Left + (y_br - y_bl) - 3) && (pt.y > Top - m_diff_y - 2) && (pt.y < Top - m_diff_y + 2 * m_cell_h + 2));
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
	const int cht = C_CELLH * y_th / 13;
	return BIN((x > Left + 2) && (x < Left + (y_br - y_bl) - 3) && (y > Top + cht + 2) && (y < Top + 7 * cht - 3));
}

void TDateCalendar::SelectMonth(HWND hWnd, const SPoint2S pt)
{
	int    j = (pt.x - Left) * 6 / (y_br - y_bl);
	int    i = (pt.y - (Top - M_DIFFY * y_th / 13 - 2)) / (M_CELLH * y_th / 13 + 2);
	LDATE  dd1 = D2; dd1.setday(1);
	LDATE  dd2 = encodedate(1, j + 1 + i * 6, D.year());
	if(dd1 != dd2 || (dd1 == dd2 && !SelStarted2)) {
		D.setmonth(j + 1 + i * 6);
		Normalize();
		SetupCalendar();
		if(PeriodSelect && seltype == SEL_MONTHS) {
			if(SelStarted2) {
				D2 = encodedate(D2.day(), D.month(), D.year());
				if(D1 > D2) {
					i = D.month();
					D.setmonth(D1.month());
					Normalize();
					D1.setday(NDays);
					D2.setday(1);
					D.setmonth(i);
					Normalize();
				}
				else {
					i = D.month();
					D.setmonth(D2.month());
					Normalize();
					D1.setday(1);
					D2.setday(NDays);
					D.setmonth(i);
					Normalize();
				}
			}
			else {
				SelStarted2 = 1;
				D1 = encodedate(1, D.month(), D.year());
				D2 = encodedate(NDays, D.month(), D.year());
			}
			SendToEditBox(D1.month(), D1.year(), D2.month(), D2.year(), 0);
		}
		::InvalidateRect(hWnd, NULL, false);
	}
}

void TDateCalendar::SelectQuart(HWND hWnd, const SPoint2S pt)
{
	int    j = (pt.x - Left) * 6 / (y_br - y_bl);
	int    i = (pt.y - (Top - M_DIFFY * y_th / 13 - 2)) / (M_CELLH * y_th / 13 + 2);
	int    q = (j + i * 6) / 3 + 1;
	LDATE  dd1 = encodedate(1, D2.month(), D.year());
	LDATE  dd  = encodedate(1, D1.month(), D.year());
	LDATE  dd2 = encodedate(1, (q - 1) * 3 + 1, D.year());
	if(dd1 != dd2 || (dd1 == dd2 && !SelStarted2)) {
		D.setmonth((q - 1) * 3 + 1);
		Normalize();
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
				Normalize();
				D1.setday(NDays);
				D2.setday(1);
				D.setmonth(i);
				Normalize();
			}
			else {
				i = D.month();
				int mon1 = D1.month();
				if(oneof4(mon1, 3, 6, 9, 12))
					D1.setmonth(mon1-2);
				D.setmonth(D2.month());
				Normalize();
				D1.setday(1);
				D2.setday(NDays);
				D.setmonth(i);
				Normalize();
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
}

static int hashd(LDATE d)
{
	return (d.month() * 31 + (d.year() - 1800) * 366);
}

void TDateCalendar::SelectDay(HWND hWnd, int x, int y)
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
	else if(((i && i < (c_maxrow-1)) || (!i && j >= c_minfirst) || (i == (c_maxrow-1) && j <= c_maxlast)) && (!PeriodSelect || (satoi(C[i+1][j]) + hashd(D) != D2.day() + hashd(D2)))) {
		//
		// Move the selection
		//
		HDC    hdc = ::GetDC(hWnd);
		HFONT  hf = CreateFont(IsLarge ? 24 : 8, 0, 0, 0, FW_NORMAL, 0, 0, 0, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
			DEFAULT_QUALITY, DEFAULT_PITCH | FW_DONTCARE, _T("MS Sans Serif"));
		HFONT  hf_old = static_cast<HFONT>(SelectObject(hdc, hf));
		::SetBkMode(hdc, TRANSPARENT);
		::SetTextColor(hdc, RGB(0, 0, 0));
		if(PeriodSelect && D == D1)
			::SetTextColor(hdc, C_TEXT2COL);
		x = Left + c_j * c_cell_w;
		y = Top + c_i * c_cell_h;
		HPEN   pen = ::CreatePen(PS_SOLID, 3, C_BACKCOL);
		HPEN   oldpen = static_cast<HPEN>(::SelectObject(hdc, pen));
		HBRUSH br = ::CreateSolidBrush(C_BACKCOL);
		HBRUSH oldbr = static_cast<HBRUSH>(::SelectObject(hdc, br));
		::Rectangle(hdc, x + 1, y, (c_j < 6) ? (x + c_cell_w - 1) : (x + c_cell_w - 2), y + c_cell_h - 2);
		::SelectObject(hdc, oldpen);
		::SelectObject(hdc, oldbr);
		ZDeleteWinGdiObject(&pen);
		ZDeleteWinGdiObject(&br);
		SIZE ts;
		::GetTextExtentPoint32(hdc, C[c_i][c_j], 2, &ts); // @unicodeproblem
		::TextOut(hdc, x + (c_cell_w - ts.cx) / 2, y + (c_cell_h - ts.cy) / 2, C[c_i][c_j], 2); // @unicodeproblem
		c_i = i + 1;
		c_j = j;
		D.setday(satoi(C[c_i][c_j])); // @unicodeproblem
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
				// Здесь отрисовывать выделение
				//
				for(int ii = 1; ii <= c_maxrow; ii++) {
					for(int jj = 0; jj <= 6; jj++) {
						int    cur_entry = satoi(C[ii][jj]); // @unicodeproblem
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
							pen = ::CreatePen(PS_SOLID, 3, C_SELPCOL);
							br  = ::CreateSolidBrush(C_SELPCOL);
							::SetTextColor(hdc, C_TEXT2COL);
						}
						else {
							pen = ::CreatePen(PS_SOLID, 3, C_BACKCOL);
							br  = ::CreateSolidBrush(C_BACKCOL);
							::SetTextColor(hdc, 0);
						}
						oldpen = static_cast<HPEN>(::SelectObject(hdc, pen));
						oldbr  = static_cast<HBRUSH>(::SelectObject(hdc, br));
						::Rectangle(hdc, xx + 1, yy, (jj < 6) ? (xx + c_cell_w - 1) : (xx + c_cell_w - 2), yy + c_cell_h - 2);
						::SelectObject(hdc, oldpen);
						::SelectObject(hdc, oldbr);
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
			// Передача текущего периода в едитбокс
			//
			if(D1 > D2)
				SendToEditBox(D2, D1, 1);
			else
				SendToEditBox(D1, D2, 1);
		}
		else
			::SetTextColor(hdc, GetColorRef(SClrWhite));
		x = Left + c_j * c_cell_w;
		y = Top  + c_i * c_cell_h;
		pen = ::CreatePen(PS_SOLID, 3, C_SELCOL);
		oldpen = static_cast<HPEN>(::SelectObject(hdc, pen));
		br = ::CreateSolidBrush(C_SELCOL);
		oldbr = static_cast<HBRUSH>(::SelectObject(hdc, br));
		::Rectangle(hdc, x + 1, y, c_j < 6 ? x + c_cell_w - 1 : x + c_cell_w - 2, y + c_cell_h - 2);
		::SelectObject(hdc, oldpen);
		::SelectObject(hdc, oldbr);
		ZDeleteWinGdiObject(&pen);
		ZDeleteWinGdiObject(&br);
		::GetTextExtentPoint32(hdc, C[c_i][c_j], 2, &ts);
		::TextOut(hdc, x + (c_cell_w - ts.cx) / 2, y + (c_cell_h - ts.cy) / 2, C[c_i][c_j], 2);
		::SelectObject(hdc, hf_old);
		ZDeleteWinGdiObject(&hf);
		::ReleaseDC(hWnd, hdc);
		if(!SelStarted1)
			::InvalidateRect(hWnd, NULL, false);
		SelStarted1 = 1;
	}
}

void TDateCalendar::SelectWeek(HWND hWnd, SPoint2S pt)
{
	const int c_cell_w = (y_br - y_bl + 1) / 7;
	const int c_cell_h = C_CELLH * y_th / 13;
	const int j = (pt.x - Left) / c_cell_w;
	const int i = (pt.y - Top)  / c_cell_h - 1;
	if(((i && i < (c_maxrow - 1)) || (!i && j >= c_minfirst) || (i == (c_maxrow-1) && j <= c_maxlast)) && (satoi(C[i+1][j]) + hashd(D) != D2.day() + hashd(D2))) {
		// Move the selection
		HDC    hdc = ::GetDC(hWnd);
		HFONT  hf = CreateFont(IsLarge ? 24 : 8, 0, 0, 0, FW_NORMAL, 0, 0, 0, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
			DEFAULT_QUALITY, DEFAULT_PITCH|FW_DONTCARE, _T("MS Sans Serif"));
		HFONT  hf_old = static_cast<HFONT>(SelectObject(hdc, hf));
		SetBkMode(hdc, TRANSPARENT);
		SetTextColor(hdc, RGB(0, 0, 0));
		if(PeriodSelect && D == D1)
			SetTextColor(hdc, C_TEXT2COL);
		pt.x = Left + c_j * c_cell_w /* C_CELLW */;
		pt.y = Top + c_i * c_cell_h /* C_CELLH */;
		HPEN   pen = ::CreatePen(PS_SOLID, 3, C_BACKCOL);
		HPEN   oldpen = static_cast<HPEN>(::SelectObject(hdc, pen));
		HBRUSH br = ::CreateSolidBrush(C_BACKCOL);
		HBRUSH oldbr = static_cast<HBRUSH>(::SelectObject(hdc, br));
		::Rectangle(hdc, pt.x + 1, pt.y, (c_j < 6) ? (pt.x + c_cell_w - 1) : (pt.x + c_cell_w - 2), pt.y + c_cell_h - 2);
		::SelectObject(hdc, oldpen);
		::SelectObject(hdc, oldbr);
		ZDeleteWinGdiObject(&pen);
		ZDeleteWinGdiObject(&br);
		SIZE ts;
		::GetTextExtentPoint32(hdc, C[c_i][c_j], 2, &ts);
		::TextOut(hdc, pt.x + (c_cell_w - ts.cx) / 2, pt.y + (c_cell_h - ts.cy) / 2, C[c_i][c_j], 2);
		c_i = i + 1;
		c_j = j;
		D.setday(satoi(C[c_i][c_j]));
		const int draw_months = BIN(D2.year() != D.year() || D2.month() != D.month());
		{
			const LDATE  beg_dt = encodedate(1, 1, 1);
			long   days = ((diffdate(D, beg_dt) - 1)/ 7) * 7/* + 1*/; // @v10.0.1 (+1) removed
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
					D1 = MAX(D2, D1);
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
			const LDATE dt1 = MIN(D1, D2);
			const LDATE dt2 = MAX(D2, D1);
			if(draw_months) {
				RECT rr;
				rr.left   = Left;
				rr.right  = Left + (y_br - y_bl) + 1;
				rr.top    = y_t - 3;
				rr.bottom = Top - M_DIFFY * y_th / 13 + M_CELLH * y_th / 13 * 2 + 10;
				::InvalidateRect(hWnd, &rr, true);
			}
			//
			// Здесь отрисовывать выделение
			//
			for(int ii = 1; ii <= c_maxrow; ii++) {
				for(int jj = 0; jj <= 6; jj++) {
					int    cur_entry = satoi(C[ii][jj]); // @unicodeproblem
					int    is_bound = BIN((ii == c_maxrow && jj > c_maxlast) || (ii == 1 && jj < c_minfirst));
					const  int xx = Left + jj * c_cell_w;
					const  int yy = Top + ii * c_cell_h;
					const  LDATE cur_dt = encodedate(cur_entry, D.month(), D.year());
					const  int is_sel = BIN(cur_dt >= dt1 && cur_dt <= dt2 && !is_bound);
					pen = ::CreatePen(PS_SOLID, 3, is_sel ? C_SELPCOL : C_BACKCOL);
					br  = ::CreateSolidBrush(is_sel ? C_SELPCOL : C_BACKCOL);
					::SetTextColor(hdc, is_sel ? C_TEXT2COL : 0);
					oldpen = static_cast<HPEN>(::SelectObject(hdc, pen));
					oldbr  = static_cast<HBRUSH>(::SelectObject(hdc, br));
					::Rectangle(hdc, xx + 1, yy, (jj < 6) ? (xx + c_cell_w - 1) : (xx + c_cell_w - 2), yy + c_cell_h - 2);
					::SelectObject(hdc, oldpen);
					::SelectObject(hdc, oldbr);
					ZDeleteWinGdiObject(&pen);
					ZDeleteWinGdiObject(&br);
					::GetTextExtentPoint32(hdc, C[ii][jj], 2, &ts);
					::TextOut(hdc, xx + (c_cell_w - ts.cx) / 2, yy + (c_cell_h - ts.cy) / 2, C[ii][jj], 2);
				}
			}
			::SetTextColor(hdc, C_TEXT2COL);
		}
		else {
			D1 = D2 = D;
			::SetTextColor(hdc, C_TEXT2COL);
		}
		if(D1 > D2)
			SendToEditBox(D2, D1, 1);
		else
			SendToEditBox(D1, D2, 1);
		pt.x = Left + c_j * c_cell_w;
		pt.y = Top  + c_i * c_cell_h;
		pen = ::CreatePen(PS_SOLID, 3, C_SELCOL);
		oldpen = static_cast<HPEN>(::SelectObject(hdc, pen));
		br = ::CreateSolidBrush(C_SELCOL);
		oldbr = static_cast<HBRUSH>(::SelectObject(hdc, br));
		::Rectangle(hdc, pt.x + 1, pt.y, (c_j < 6) ? (pt.x + c_cell_w - 1) : (pt.x + c_cell_w - 2), pt.y + c_cell_h - 2);
		::SelectObject(hdc, oldpen);
		::SelectObject(hdc, oldbr);
		ZDeleteWinGdiObject(&pen);
		ZDeleteWinGdiObject(&br);
		::GetTextExtentPoint32(hdc, C[c_i][c_j], 2, &ts);
		::TextOut(hdc, pt.x + (c_cell_w - ts.cx) / 2, pt.y + (c_cell_h - ts.cy) / 2, C[c_i][c_j], 2);
		::SelectObject(hdc, hf_old);
		ZDeleteWinGdiObject(&hf);
		::ReleaseDC(hWnd, hdc);
		if(!SelStarted1)
			::InvalidateRect(hWnd, NULL, false);
		SelStarted1 = 1;
	}
}

void TDateCalendar::OnLButtonDown(HWND hWnd, LPARAM lParam)
{
	int i;
	//int x = LOWORD(lParam);
	//int y = HIWORD(lParam);
	SPoint2S pt;
	pt.setwparam(lParam);
	SetFocus(hWnd);
	if((i = IsYearBar(pt)) != 0)
		SelectYear(hWnd, i);
	else if(IsYearMarker(pt) < 0)
		ScrollYear(hWnd, -1);
	else if(IsYearMarker(pt) > 0)
		ScrollYear(hWnd, 1);
	else if(IsDayBar(pt.x, pt.y) && (!PeriodSelect || oneof2(seltype, SEL_DAYS, SEL_WEEKS))) {
		if(!PeriodSelect || seltype == SEL_DAYS)
			SelectDay(hWnd, pt.x, pt.y);
		else if(seltype == SEL_WEEKS)
			SelectWeek(hWnd, pt);
	}
	else if(IsMonthBar(pt))
		if(seltype != SEL_QUARTALS)
			SelectMonth(hWnd, pt);
		else if(PeriodSelect)
			SelectQuart(hWnd, pt);
	{
		HWND hwnd_ctl = GetDlgItem(GetParent(hWnd), CTL_CALENDAR_FASTPRD);
		SendMessage(hwnd_ctl, CB_SETCURSEL, -1, 0);
	}
}

static void FASTCALL SendLButtDownMsg(HWND hWnd, SPoint2S p)
{
	::SendMessage(hWnd, WM_LBUTTONDOWN, MK_LBUTTON, p.towparam());
}

void TDateCalendar::OnTimer(HWND hWnd)
{
	int    c_cell_w = (y_br - y_bl + 1) / 7;
	int    c_cell_h = C_CELLH * y_th / 13;
	SPoint2S p;
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
		Normalize();
		SetupCalendar();
		SelStarted1 = 0;
		SelStarted2 = 0;
		SelStarted3 = 0;
		HWND ctl_edit = GetDlgItem(parent_hWnd, 1031);
		TView::SSetWindowText(ctl_edit, "..");
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
		SPoint2S p;
		if(mon <= 6)
			p.Set((mon - 1) * m_cell_w + Left + 3, Top - m_diff_y + 3);
		else
			p.Set((mon - 7) * m_cell_w + Left + 3, Top - m_diff_y + 5 + m_cell_h);
		::SendMessage(hWnd, WM_LBUTTONDOWN, MK_LBUTTON, p.towparam());
	}
	return ok;
}

LRESULT CALLBACK CalendarWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	TDateCalendar * dc = static_cast<TDateCalendar *>(TView::GetWindowUserData(hWnd));
	switch(message) {
		case WM_CREATE:
			{
				LPCREATESTRUCT p_cr_data = reinterpret_cast<LPCREATESTRUCT>(lParam);
				dc = static_cast<TDateCalendar *>(p_cr_data->lpCreateParams);
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
		case WM_KILLFOCUS: KillTimer(dc->c_hWnd, 1); break;
		case WM_TIMER: dc->OnTimer(hWnd); break;
		case WM_KEYUP:
			dc->TimerFreq = 50;
			SetTimer(dc->c_hWnd, 1, 50, 0);
			break;
		case WM_KEYDOWN:
			switch((int)wParam) {
				case VK_ESCAPE:
					::DestroyWindow(dc->c_hWnd);
					EndDialog(hWnd, 0);
					break;
				case VK_END: ::SendMessage(hWnd, WM_LBUTTONDOWN, MK_LBUTTON, MAKEWPARAM(dc->y_br - 2, dc->y_t + 2)); break;
				case VK_HOME: ::SendMessage(hWnd, WM_LBUTTONDOWN, MK_LBUTTON, MAKEWPARAM(dc->y_bl + 2, dc->y_t + 2)); break;
				case VK_DELETE: ::SendMessage(hWnd, WM_RBUTTONDOWN, 0, MAKEWPARAM(dc->Left + 5, dc->Top + 5)); break;
				case VK_NEXT: dc->StepMonth(hWnd, 1); break;
				case VK_PRIOR: dc->StepMonth(hWnd, 0); break;
			}
			break;
		case WM_PAINT: dc->OnPaint(hWnd); break;
		case WM_LBUTTONDBLCLK:
			if(dc->IsDayBar(LOWORD(lParam), HIWORD(lParam)))
  				::SendMessage(GetParent(hWnd), WM_COMMAND, MAKEWPARAM(IDOK, 0), (LPARAM)hWnd);
			else
				::SendMessage(hWnd, WM_LBUTTONDOWN, wParam, lParam);
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
	SString temp_buf;
	temp_buf.Cat(period, 0).Transf(CTRANSF_INNER_TO_OUTER);
	TView::SSetWindowText(GetDlgItem(parent_hWnd, CTL_CALENDAR_PERIODEDIT), temp_buf);
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
		SExchange(&q1, &q2);
		SExchange(&y1, &y2);
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
	s.Cat(qt1).Slash().Cat(y1);
	if(y1 != y2 || q1 != q2)
		s.CatCharN('.', 2).Cat(qt2).Slash().Cat(y2);
	TView::SSetWindowText(GetDlgItem(parent_hWnd, CTL_CALENDAR_PERIODEDIT), s);
}
//
// Monthes
//
void TCalendar::SendToEditBox(int m1, int y1, int m2, int y2, int opt)
{
	LDATE  dd1 = encodedate(1, m1, y1);
	LDATE  dd2 = encodedate(1, m2, y2);
	SExchangeForOrder(&dd1.v, &dd2.v);
	SString s;
	s.Cat(dd1.month()).Slash().Cat(dd1.year());
	if(dd1 != dd2)
		s.CatCharN('.', 2).Cat(dd2.month()).Slash().Cat(dd2.year());
	TView::SSetWindowText(GetDlgItem(parent_hWnd, CTL_CALENDAR_PERIODEDIT), s);
}
//
// Years
//
void TCalendar::SendToEditBox(int y1, int y2)
{
	SExchangeForOrder(&y1, &y2);
	SString s;
	s.Cat(y1);
	if(y1 != y2)
		s.CatCharN('.', 2).Cat(y2);
	TView::SSetWindowText(GetDlgItem(parent_hWnd, CTL_CALENDAR_PERIODEDIT), s);
}
//
// TPeriodCalendar
//
void TPeriodCalendar::SelectByFastPrd(HWND hWnd)
{
	long   fastprd_sel = 0;
	HWND   cbx_hwnd = GetDlgItem(hWnd, CTL_CALENDAR_FASTPRD);
	if(P_Inner && cbx_hwnd && (fastprd_sel = SendMessage(cbx_hwnd, CB_GETCURSEL, 0, 0)) != CB_ERR) {
		LDATE cur_dt = getcurdate_();
		LDATE d1 = ZERODATE;
		LDATE d2 = ZERODATE;
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
			const int plus_days = (fastprd_sel == PPFASTPRD_LASTWORKWEEK) ? 4 : 6;
			plusdate(&cur_dt, 1 - (7 + dayofweek(&cur_dt, 1)), 0);
			d1 = cur_dt;
			plusdate(&(d2 = cur_dt), plus_days, 0);
		}
		else if(oneof2(fastprd_sel, PPFASTPRD_THISMONTH, PPFASTPRD_LASTMONTH)) {
			if(fastprd_sel == PPFASTPRD_LASTMONTH)
				plusperiod(&cur_dt, PRD_MONTH, -1, 0);
			(d1 = cur_dt).setday(1);
			(d2 = cur_dt).setday(cur_dt.dayspermonth());
		}
		if(d1 != ZERODATE && d2 != ZERODATE) {
			P_Inner->D  = d1;
			P_Inner->D1 = d1;
			P_Inner->D2 = d2;
			P_Inner->Normalize();
			P_Inner->SetupCalendar();
			P_Inner->SendToEditBox(P_Inner->D1, P_Inner->D2, 0);
			InvalidateRect(hWnd, NULL, false);
		}
	}
}

static INT_PTR CALLBACK PeriodWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int    ret = 0;
	HWND   edit, but1;
	SString temp_buf;
	TPeriodCalendar * pc = 0;
	switch(message) {
		case WM_INITDIALOG:
			pc = reinterpret_cast<TPeriodCalendar *>(lParam);
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
					for(uint p = 0; ss.get(&p, temp_buf);)
						SendDlgItemMessage(hWnd, CTL_CALENDAR_FASTPRD, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(SUcSwitch(temp_buf.cptr())));
					// SendDlgItemMessage(hwndDlg, CTL_WPRINT_REPORT, CB_SETCURSEL, 0, 0);
				}
				TView::PreprocessWindowCtrlText(hWnd);
			}
			break;
		case WM_SHOWWINDOW:
			pc = (TPeriodCalendar *)TView::GetWindowUserData(hWnd);
			edit = GetDlgItem(hWnd, CTL_CALENDAR_PERIODEDIT);
			TView::SSetWindowText(edit, pc->Period);
			pc->UpdatePeriod();
			but1 = GetDlgItem(hWnd, pc->SelType); // CTL_CALENDAR_DAYS CTL_CALENDAR_QUARTALS
			SendMessage(but1, BM_SETCHECK, (WPARAM) BST_CHECKED, 0);
			if(!pc->P_Inner) {
				pc->P_Inner = new TCalendarP(pc->D1, pc->D2);
				if(pc->P_Inner) {
					switch(pc->SelType) {
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
						TView::SGetWindowText(edit, pc->Period);
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
						if(HIWORD(wParam) == CBN_SELENDOK && pc)
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
void TCalendarP::OnSelectionType(int aSelType)
{
	if(aSelType == SEL_DAYS) {
		seltype = SEL_DAYS;
		SendToEditBox(D1, D2, 0);
	}
	else if(aSelType == SEL_WEEKS) {
		seltype = SEL_WEEKS;
		LDATE beg_dt = encodedate(1, 1, 1);
		long days = ((diffdate(D1, beg_dt) - 1) / 7) * 7 + 1;
		LDATE dt1 = plusdate(beg_dt, days);
		LDATE dt2 = ZERODATE;
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
		return;
	InvalidateRect(c_hWnd, NULL, false);
	SetFocus(c_hWnd);
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
	Normalize();
	SetupCalendar();
	LPCTSTR p_classname = _T("PpyPeriodCalendar");
	{
		WNDCLASSEX wc;
		INITWINAPISTRUCT(wc);
		wc.lpszClassName = p_classname;
		wc.hInstance = TProgram::GetInst();
		wc.lpfnWndProc = static_cast<WNDPROC>(CalendarWndProc);
		wc.style = CS_DBLCLKS;
		wc.hCursor = LoadCursor(NULL, IDC_ARROW);
		wc.hbrBackground = static_cast<HBRUSH>(::GetStockObject(LTGRAY_BRUSH));
		::RegisterClassEx(&wc);
	}
	c_hWnd = ::CreateWindowEx(0, p_classname, NULL, WS_VISIBLE|WS_CHILD|WS_TABSTOP|WS_GROUP,
		8, 85, 188, 200, parent_hWnd, NULL, TProgram::GetInst(), this);
	Top  = 80;
	Left = 7;
	::ShowWindow(c_hWnd, SW_SHOWNORMAL);
	::SetFocus(c_hWnd);
	TimerFreq = 50;
	SetTimer(c_hWnd, 1, 50, 0);
}
//#else
//#endif // } @v11.2.4 (USE_NEW_CALENDAR == 0) 

int ExecDateCalendar(void * hParentWnd, LDATE * pDate)
{
	const bool use_new_calendar = !(APPL->GetUiSettings().Flags & UserInterfaceSettings::fDateTimePickerBefore1124);
	int    ok = -1;
	if(use_new_calendar) {
		SCalendarPicker::DataBlock data;
		RVALUEPTR(data.Dtm.d, pDate);
		ok = SCalendarPicker::Exec(SCalendarPicker::kDate, data);
		if(ok > 0)
			ASSIGN_PTR(pDate, data.Dtm.d);
	}
	else {
		LDATE  dt = DEREFPTROR(pDate, getcurdate_());
		TDateCalendar * p_pc = new TDateCalendar(0, 0);
		if(p_pc) {
			p_pc->setDTS(dt);
			p_pc->ShowCalendar(static_cast<HWND>(hParentWnd));
			if(p_pc->GetRetCmd() == cmOK) {
				p_pc->getDTS(&dt);
				ok = 1;
			}
			delete p_pc;
		}
		ASSIGN_PTR(pDate, dt);
	}
	return ok;
}
//
//
//
void ShowCalCtrl(int buttCtlID, TDialog * pDlg, int show)
	{ ShowWindow(GetDlgItem(pDlg->H(), buttCtlID), show ? SW_SHOW : SW_HIDE); }

static void STDCALL SetupCalCtrl(int buttCtlID, TDialog * pDlg, uint editCtlID, uint T)
{
	struct CalButtonWndEx {
		CalButtonWndEx(TDialog * pDlg, uint editCtlId, uint calType, WNDPROC fPrevWndProc) : Dlg(pDlg), EditID(editCtlId), CalType(calType), PrevWndProc(fPrevWndProc)
		{
			STRNSCPY(Signature, GetCalCtrlSignature(calType));
		}
		static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
		{
			CalButtonWndEx * p_cbwe = static_cast<CalButtonWndEx *>(TView::GetWindowUserData(hWnd));
			switch(message) {
				case WM_DESTROY:
					TView::SetWindowUserData(hWnd, (void *)0);
					if(p_cbwe->PrevWndProc) {
						TView::SetWindowProp(hWnd, GWLP_WNDPROC, p_cbwe->PrevWndProc);
					}
					delete p_cbwe;
					return 0;
				case WM_LBUTTONUP:
					{
						const bool use_new_calendar = !(APPL->GetUiSettings().Flags & UserInterfaceSettings::fDateTimePickerBefore1124);
						if(p_cbwe->CalType) {
							if(use_new_calendar)
								SCalendarPicker::Exec(SCalendarPicker::kPeriod, p_cbwe->Dlg, p_cbwe->EditID);
							else {
								TPeriodCalendar * pc = new TPeriodCalendar(p_cbwe->Dlg, p_cbwe->EditID);
								if(pc) {
									pc->Show();
									delete pc;
								}
							}
						}
						else {
							if(use_new_calendar)
								SCalendarPicker::Exec(SCalendarPicker::kDate, p_cbwe->Dlg, p_cbwe->EditID);
							else {
								TDateCalendar * pc = new TDateCalendar(p_cbwe->Dlg, p_cbwe->EditID);
								if(pc) {
									pc->ShowCalendar(0);
									delete pc;
								}
							}
						}
					}
					break;
			}
			return CallWindowProc(p_cbwe->PrevWndProc, hWnd, message, wParam, lParam);
		}
		char   Signature[24];
		TDialog * Dlg;
		const uint EditID;
		const uint CalType;
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
		CalButtonWndEx * p_cbwe = new CalButtonWndEx(pDlg, editCtlID, cal_type, static_cast<WNDPROC>(TView::GetWindowProp(hwnd, GWLP_WNDPROC)));
		TView::SetWindowUserData(hwnd, p_cbwe);
		TView::SetWindowProp(hwnd, GWLP_WNDPROC, CalButtonWndEx::WndProc);
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
		if(!hbm_daterange || !hbm_calendar) {
			ENTER_CRITICAL_SECTION
			SETIFZ(hbm_daterange, APPL->LoadBitmap_(IDB_DATERANGE));
			SETIFZ(hbm_calendar,  APPL->LoadBitmap_(IDB_CALENDAR));
			LEAVE_CRITICAL_SECTION
		}
		::SendMessage(hwnd, BM_SETIMAGE, IMAGE_BITMAP, reinterpret_cast<LPARAM>(oneof3(T, 1, 2, 3) ? hbm_daterange : hbm_calendar));
	}
}

//void TDialog::SetupCalendar(uint calCtlID, uint inputCtlID, int kind) { SetupCalCtrl(calCtlID, this, inputCtlID, kind); }
void TDialog::SetupCalDate(uint calCtlID, uint inputCtlID) { SetupCalCtrl(calCtlID, this, inputCtlID, 4); } //{ SetupCalendar(calCtlID, inputCtlID, 4); }
void TDialog::SetupCalPeriod(uint calCtlID, uint inputCtlID) { SetupCalCtrl(calCtlID, this, inputCtlID, 1); } //{ SetupCalendar(calCtlID, inputCtlID, 1); }
//
static const uint NumYearsInView = 5;
static const float FixedCtrlHeight = 21.0f; // @v12.2.3 20.0f-->21.0f

SCalendarPicker::DataBlock::DataBlock() : Dtm(ZERODATETIME)
{
	Period.Z();
}

SCalendarPicker::LayoutExtra::LayoutExtra(int ident, uint value) : Ident(ident), Value(value)
{
}

/*static*/const SVector & SCalendarPicker::GetLayoutExtraVector()
{
	static SVector vec(sizeof(SCalendarPicker::LayoutExtra));
	ENTER_CRITICAL_SECTION
	if(!vec.getCount()) {
		vec.insert(&LayoutExtra(loiFrame_Main, 0));
		vec.insert(&LayoutExtra(loiFrame_Years, 0));
		vec.insert(&LayoutExtra(loiFrame_Monthes, 0));
		vec.insert(&LayoutExtra(loiFrame_Days, 0));
		{
			for(uint i = 0; i < NumYearsInView; i++)
				vec.insert(&LayoutExtra(loiYear, i));
		}
		vec.insert(&LayoutExtra(loiYearArrow, SIDE_LEFT));
		vec.insert(&LayoutExtra(loiYearArrow, SIDE_RIGHT));
		{
			for(uint i = 1; i <= 12; i++)
				vec.insert(&LayoutExtra(loiMonth, i));
		}
		{
			for(uint i = 1; i <= 7; i++)
				vec.insert(&LayoutExtra(loiWeekday, i));
		}
		{
			for(uint i = 1; i <= 31; i++)
				vec.insert(&LayoutExtra(loiDay, i));
		}
		vec.insert(&LayoutExtra(loiFrame_Buttons, 0)); // buttons frame
		vec.insert(&LayoutExtra(loiFrame_Buttons, 1)); // now button
		vec.insert(&LayoutExtra(loiFrame_Buttons, 2)); // ok button
		vec.insert(&LayoutExtra(loiFrame_Buttons, 3)); // cancel button
		vec.insert(&LayoutExtra(loiFrame_PeriodTypeButtons, 0)); // period buttons frame
		vec.insert(&LayoutExtra(loiFrame_PeriodTypeButtons, PRD_DAY)); // period button 
		vec.insert(&LayoutExtra(loiFrame_PeriodTypeButtons, PRD_WEEK)); // period button 
		vec.insert(&LayoutExtra(loiFrame_PeriodTypeButtons, PRD_MONTH)); // period button 
		vec.insert(&LayoutExtra(loiFrame_PeriodTypeButtons, PRD_QUART)); // period button 
		vec.insert(&LayoutExtra(loiFrame_PeriodTypeButtons, PRD_ANNUAL)); // period button 
		vec.insert(&LayoutExtra(loiFrame_Hours, 0));
		{
			for(uint i = 1; i <= 24; i++)
				vec.insert(&LayoutExtra(loiFrame_Hours, i));
		}
		vec.insert(&LayoutExtra(loiFrame_Minuts, 0));
		{
			vec.insert(&LayoutExtra(loiFrame_Minuts, 0xffffU)); // zero
			for(uint i = 1; i <= 60; i++)
				vec.insert(&LayoutExtra(loiFrame_Minuts, i));
		}
	}
	LEAVE_CRITICAL_SECTION
	return vec;
}

/*static*/const char * SCalendarPicker::GetWindowTitle(int kind)
{
	const char * p_symb = 0;
	switch(kind) {
		case kPeriod: p_symb = "selectdaterange"; break;
		case kDate: p_symb = "selectdate"; break;
		case kTime: p_symb = "selection_time"; break;
	}
	return p_symb ? PPLoadStringS(p_symb, SLS.AcquireRvlStr()) : "";
}

SCalendarPicker::SCalendarPicker(int kind) : TWindowBase(SUcSwitch(SCalendarPicker::GetWindowTitle(kind)), wbcDrawBuffer), 
	Kind(kind), LoExtraList(GetLayoutExtraVector()), FontId(0), CStyleId(0), CStyleFocusId(0), P_LoFocused(0), StartLoYear(0),
	PeriodTerm(PRD_DAY), PeriodPredef(0)
{
	Data.Dtm = getcurdatetime_();
	Data.Period.Z();
	assert(oneof4(kind, kDate, kPeriod, kTime, kDateTime));
}

SCalendarPicker::~SCalendarPicker()
{
}

LDATE SCalendarPicker::ISD() const
{
	return checkdate(Data.Dtm.d) ? Data.Dtm.d : getcurdate_();
}

IMPL_DIALOG_SETDTS(SCalendarPicker)
{
	int    ok = 1;
	RVALUEPTR(Data, pData);
	if(H()) { // @v11.2.7
		invalidateAll(true);
		::UpdateWindow(H());
	}
	return ok;
}
	
IMPL_DIALOG_GETDTS(SCalendarPicker)
{
	int    ok = 1;
	ASSIGN_PTR(pData, Data);
	return ok;
}

const SCalendarPicker::LayoutExtra * SCalendarPicker::GetLayoutExtra(int ident, uint val) const
{
	for(uint i = 0; i < LoExtraList.getCount(); i++) {
		const LayoutExtra * p_item = static_cast<const LayoutExtra *>(LoExtraList.at(i));
		if(p_item->Ident == ident && p_item->Value == val)
			return p_item;
	}
	return 0;
}

void SCalendarPicker::CreateFont_()
{
	if(FontId <= 0) {
		HFONT  hf = 0; // reinterpret_cast<HFONT>(SendMessage(p_draw_item->H_Item, WM_GETFONT, 0, 0));
		{
			LOGFONT log_font;
			MEMSZERO(log_font);
			log_font.lfCharSet = DEFAULT_CHARSET;
			STRNSCPY(log_font.lfFaceName, SUcSwitch("Arial"));
			log_font.lfHeight = 8;
			hf = ::CreateFontIndirect(&log_font);
		}
		SPaintToolBox * p_tb = APPL->GetUiToolBox();
		if(p_tb)
			FontId = p_tb->CreateFont_(0, hf, 12);
	}
}

void SCalendarPicker::SetupStartLoYear()
{
	if(StartLoYear <= 1600 || StartLoYear >= 2200)
		StartLoYear = ISD().year()-2;
}

void SCalendarPicker::CreateLayout(LDATE selectedDate)
{
	static const float def_margin = 2.0f;
	class InnerBlock {
	public:
		static void __stdcall CalendarItem_SetupLayoutItemFrameProc(SUiLayout * pItem, const SUiLayout::Result & rR)
		{
			if(pItem) {
				TView * p = static_cast<TView *>(SUiLayout::GetManagedPtr(pItem));
				if(p) {
					p->changeBounds(TRect(pItem->GetFrameAdjustedToParent()));
				}	
			}
		}
		static SUiLayout * MakeMonthLayoutEntry(SCalendarPicker * pMaster, SUiLayout * pLoParent, uint mon)
		{
			assert(checkirange(mon, 1U, 12U));
			SUiLayoutParam alb;
			alb.GrowFactor = 1.0f;
			alb.SetVariableSizeY(SUiLayoutParam::szByContainer, 1.0f);
			alb.Margin.Set(def_margin);
			return pLoParent->InsertItem(const_cast<LayoutExtra *>(pMaster->GetLayoutExtra(loiMonth, mon)), &alb);
		}
		static SUiLayout * MakeDayLayoutEntry(SCalendarPicker * pMaster, SUiLayout * pLoParent, uint day)
		{
			assert(checkirange(day, 0U, 31U)); // нулевое значение используется для ячеек, с которыми не сопоставлен календарный день данного месяца
			SUiLayoutParam alb;
			alb.GrowFactor = 1.0f;
			alb.SetVariableSizeY(SUiLayoutParam::szByContainer, 1.0f);
			alb.Margin.Set(def_margin);
			return pLoParent->InsertItem(const_cast<LayoutExtra *>(pMaster->GetLayoutExtra(loiDay, day)), &alb);
		}
		static void MakeYearsLayout(SCalendarPicker * pMaster, SUiLayout * pLoParent)
		{
			// Года. Располагаются в 1 строку со служебными элементами по краям (стрелки для скроллинга)
			SUiLayout * p_lo_years = pLoParent->InsertItem(const_cast<LayoutExtra *>(pMaster->GetLayoutExtra(loiFrame_Years, 0)), 0);
			{
				SUiLayoutParam alb(DIREC_HORZ, 0, SUiLayoutParam::alignStretch);
				alb.GrowFactor = 1.0f;
				alb.SetVariableSizeX(SUiLayoutParam::szByContainer, 1.0f);
				alb.Margin.Set(def_margin);
				p_lo_years->SetLayoutBlock(alb);
			}
			{
				SUiLayoutParam alb;
				alb.GrowFactor = 0.8f;
				alb.SetVariableSizeY(SUiLayoutParam::szByContainer, 1.0f);
				alb.Margin.Set(def_margin);
				p_lo_years->InsertItem(const_cast<LayoutExtra *>(pMaster->GetLayoutExtra(loiYearArrow, SIDE_LEFT)), &alb);
			}
			for(uint i = 0; i < NumYearsInView; i++) {
				SUiLayoutParam alb;
				alb.GrowFactor = 1.0f;
				alb.SetVariableSizeY(SUiLayoutParam::szByContainer, 1.0f);
				alb.Margin.Set(def_margin);
				p_lo_years->InsertItem(const_cast<LayoutExtra *>(pMaster->GetLayoutExtra(loiYear, i)), &alb);
			}
			{
				SUiLayoutParam alb;
				alb.GrowFactor = 0.8f;
				alb.SetVariableSizeY(SUiLayoutParam::szByContainer, 1.0f);
				alb.Margin.Set(def_margin);
				p_lo_years->InsertItem(const_cast<LayoutExtra *>(pMaster->GetLayoutExtra(loiYearArrow, SIDE_RIGHT)), &alb);
			}
		}
		static void MakeMonthesLayout(SCalendarPicker * pMaster, SUiLayout * pLoParent)
		{
			// Месяцы. Располагаются в 2 строки по 6 ячеек
			SUiLayout * p_lo_monthes = pLoParent->InsertItem(const_cast<LayoutExtra *>(pMaster->GetLayoutExtra(loiFrame_Monthes, 0)), 0);
			{
				SUiLayoutParam alb(DIREC_VERT, 0, SUiLayoutParam::alignStretch);
				alb.GrowFactor = 2.0f;
				alb.SetVariableSizeX(SUiLayoutParam::szByContainer, 1.0f);
				alb.Margin.Set(def_margin);
				p_lo_monthes->SetLayoutBlock(alb);
			}
			{
				SUiLayoutParam alb_row(DIREC_HORZ);
				alb_row.GrowFactor = 1.0f;
				alb_row.SetVariableSizeX(SUiLayoutParam::szByContainer, 1.0f);
				SUiLayout * p_lo_row = p_lo_monthes->InsertItem(0, &alb_row);
				for(uint i = 1; i <= 12; i++) {
					InnerBlock::MakeMonthLayoutEntry(pMaster, p_lo_row, i);
					if(i == 6)
						p_lo_row = p_lo_monthes->InsertItem(0, &alb_row);
				}
			}
		}
		static void MakeWeekdaysLayout(SCalendarPicker * pMaster, SUiLayout * pLoParent)
		{
			// Заголовки дней недели		
			SUiLayout * p_lo_weekdays = pLoParent->InsertItem();
			{
				SUiLayoutParam alb(DIREC_HORZ, 0, SUiLayoutParam::alignStart);
				alb.GrowFactor = 1.0f;
				alb.SetVariableSizeX(SUiLayoutParam::szByContainer, 1.0f);
				p_lo_weekdays->SetLayoutBlock(alb);
			}
			{
				for(uint i = 1; i <= 7; i++) {
					SUiLayoutParam alb;
					alb.GrowFactor = 1.0f;
					alb.SetVariableSizeY(SUiLayoutParam::szByContainer, 1.0f);
					alb.Margin.Set(def_margin);
					p_lo_weekdays->InsertItem(const_cast<LayoutExtra *>(pMaster->GetLayoutExtra(loiWeekday, i)), &alb);
				}
			}
		}
		static void MakeMonthdaysLayout(SCalendarPicker * pMaster, SUiLayout * pLoParent, LDATE selectedDate)
		{
			// Дни месяца. Располагаются в 6-ти строковых лейаутах
			SUiLayout * p_lo_days = pLoParent->InsertItem();
			{
				SUiLayoutParam alb(DIREC_VERT, 0, SUiLayoutParam::alignStart);
				alb.GrowFactor = 6.0;
				alb.SetVariableSizeX(SUiLayoutParam::szByContainer, 1.0f);
				p_lo_days->SetLayoutBlock(alb);
			}
			{
				SUiLayoutParam alb_row(DIREC_HORZ, 0, SUiLayoutParam::alignStart);
				alb_row.GrowFactor = 1.0f;
				alb_row.SetVariableSizeX(SUiLayoutParam::szByContainer, 1.0f);
				const uint dpm = selectedDate.dayspermonth();
				// Дни месяца будем располагать строго в 6 строк 
				SUiLayout * p_lo_row = p_lo_days->InsertItem();
				p_lo_row->SetLayoutBlock(alb_row);
				for(uint i = 1; i <= dpm; i++) {
					const LDATE dt = encodedate(i, selectedDate.month(), selectedDate.year());
					const uint  dow = dayofweek(&dt, 1);
					if(i == 1 && dow > 1) {
						for(uint k = 1; k < dow; k++)
							InnerBlock::MakeDayLayoutEntry(pMaster, p_lo_row, 0);
					}
					InnerBlock::MakeDayLayoutEntry(pMaster, p_lo_row, i);
					if(i == dpm && dow < 7) {
						for(uint k = dow+1; k <= 7; k++)
							InnerBlock::MakeDayLayoutEntry(pMaster, p_lo_row, 0);
					}
					if(dow == 7 && i < dpm)
						p_lo_row = p_lo_days->InsertItem(0, &alb_row);
				}
			}
		}
		static void InsertButtonLayout(SCalendarPicker * pMaster, SUiLayout * pLoParent, ushort ctlId, SUiLayoutParam & rP, float growFactor)
		{
			TView * p = pMaster->getCtrlView(ctlId);
			if(p) {
				rP.GrowFactor = growFactor;
				SUiLayout * p_lo_item = pLoParent->InsertItem(p, &rP);
				p_lo_item->SetCallbacks(0, CalendarItem_SetupLayoutItemFrameProc, p);
			}
		}
	};
	SetupStartLoYear();
	SUiLayout * p_lo_result = new SUiLayout(SUiLayoutParam(DIREC_VERT, 0, SUiLayoutParam::alignStretch));
	if(Kind == kDate) {
		InnerBlock::MakeYearsLayout(this, p_lo_result); // Года. Располагаются в 1 строку со служебными элементами по краям (стрелки для скроллинга)
		InnerBlock::MakeMonthesLayout(this, p_lo_result); // Месяцы. Располагаются в 2 строки по 6 ячеек
		{
			SUiLayout * p_lo_days_group = p_lo_result->InsertItem(const_cast<LayoutExtra *>(GetLayoutExtra(loiFrame_Days, 0)), 0);
			{
				// Группа дней недели и дней месяца
				SUiLayoutParam alb(DIREC_VERT, 0, SUiLayoutParam::alignStart);
				alb.SetGrowFactor(6.0f).SetVariableSizeX(SUiLayoutParam::szByContainer, 1.0f).SetMargin(def_margin);
				p_lo_days_group->SetLayoutBlock(alb);
			}
			InnerBlock::MakeWeekdaysLayout(this, p_lo_days_group); // Заголовки дней недели
			InnerBlock::MakeMonthdaysLayout(this, p_lo_days_group, selectedDate); // Дни месяца. Располагаются в 6-ти строковых лейаутах
		}
		{
			SUiLayoutParam alb_buttons(DIREC_HORZ, 0, SUiLayoutParam::alignEnd);
			alb_buttons.SetGrowFactor(1.0f).SetVariableSizeX(SUiLayoutParam::szByContainer, 1.0f);
			SUiLayout * p_lo_buttons = p_lo_result->InsertItem(const_cast<LayoutExtra *>(GetLayoutExtra(loiFrame_Buttons, 0)), &alb_buttons);
			{
				SUiLayoutParam alb;
				alb.SetGrowFactor(1.0f).SetFixedSizeY(FixedCtrlHeight).SetMargin(def_margin);
				InnerBlock::InsertButtonLayout(this, p_lo_buttons, CTL_CALENDAR_TODAY, alb, 1.0f);
				InnerBlock::InsertButtonLayout(this, p_lo_buttons, STDCTL_OKBUTTON, alb, 1.0f);
				InnerBlock::InsertButtonLayout(this, p_lo_buttons, STDCTL_CANCELBUTTON, alb, 1.0f);
			}
		}
	}
	else if(Kind == kPeriod) {
		{
			TView * p = getCtrlView(CTLSEL_CALENDAR_FASTPRD);
			if(p && p->GetSubSign() == TV_SUBSIGN_COMBOBOX) {
				SUiLayout * p_lo_inp2 = SUiLayout::CreateComplexLayout(SUiLayout::cmplxtInpLblBtn, /*SUiLayout::clfLabelLeft*/0, FixedCtrlHeight, p_lo_result);
				if(p_lo_inp2) {
					TInputLine * p_il = static_cast<ComboBox *>(p)->link();
					{
						SUiLayoutParam alb = p_lo_inp2->GetLayoutBlock();
						alb.SetGrowFactor(1.4f).SetMargin(def_margin).SetVariableSizeX(SUiLayoutParam::szByContainer, 1.0f);
						p_lo_inp2->SetLayoutBlock(alb);
					}
					SUiLayout * p_lo_inp = p_lo_inp2->FindComplexComponentId(SUiLayout::cmlxcInput);
					if(p_lo_inp && p_il)
						p_lo_inp->SetCallbacks(0, InnerBlock::CalendarItem_SetupLayoutItemFrameProc, p_il);
					SUiLayout * p_lo_btn = p_lo_inp2->FindComplexComponentId(SUiLayout::cmlxcButton1);
					if(p_lo_btn)
						p_lo_btn->SetCallbacks(0, InnerBlock::CalendarItem_SetupLayoutItemFrameProc, p);
					SUiLayout * p_lo_lbl = p_lo_inp2->FindComplexComponentId(SUiLayout::cmlxcLabel);
					TLabel * p_lbl = getCtlLabel(CTL_CALENDAR_FASTPRD);
					if(p_lo_lbl && p_lbl)
						p_lo_lbl->SetCallbacks(0, InnerBlock::CalendarItem_SetupLayoutItemFrameProc, p_lbl);							
				}
			}
		}
		{
			// Псевдо-кнопки выбора типа периода: дни, недели, месяцы, кварталы, годы
			SUiLayoutParam alb_row(DIREC_HORZ);
			alb_row.SetGrowFactor(1.0f).SetVariableSizeX(SUiLayoutParam::szByContainer, 1.0f);
			SUiLayout * p_lo_prdtypes = p_lo_result->InsertItem(const_cast<LayoutExtra *>(GetLayoutExtra(loiFrame_PeriodTypeButtons, 0)), &alb_row);
			{
				SUiLayoutParam alb;
				alb.SetGrowFactor(1.0f).SetVariableSizeY(SUiLayoutParam::szByContainer, 1.0f).SetMargin(def_margin);
				const uint pl[] = {PRD_DAY, PRD_WEEK, PRD_MONTH, PRD_QUART, PRD_ANNUAL};
				for(uint i = 0; i < SIZEOFARRAY(pl); i++)
					p_lo_prdtypes->InsertItem(const_cast<LayoutExtra *>(GetLayoutExtra(loiFrame_PeriodTypeButtons, pl[i])), &alb);
			}
		}
		{
			TView * p = getCtrlView(CTL_CALENDAR_PERIODEDIT);
			if(p) {
				SUiLayout * p_lo_inp2 = SUiLayout::CreateComplexLayout(SUiLayout::cmplxtInpLbl, /*SUiLayout::clfLabelLeft*/0, FixedCtrlHeight, p_lo_result);
				if(p_lo_inp2) {
					{
						SUiLayoutParam alb = p_lo_inp2->GetLayoutBlock();
						alb.SetGrowFactor(1.8f).SetVariableSizeX(SUiLayoutParam::szByContainer, 1.0f);
						p_lo_inp2->SetLayoutBlock(alb);
					}
					SUiLayout * p_lo_inp = p_lo_inp2->FindComplexComponentId(SUiLayout::cmlxcInput);
					if(p_lo_inp)
						p_lo_inp->SetCallbacks(0, InnerBlock::CalendarItem_SetupLayoutItemFrameProc, p);							
					SUiLayout * p_lo_lbl = p_lo_inp2->FindComplexComponentId(SUiLayout::cmlxcLabel);
					TLabel * p_lbl = getCtlLabel(CTL_CALENDAR_PERIODEDIT);
					if(p_lo_lbl && p_lbl)
						p_lo_lbl->SetCallbacks(0, InnerBlock::CalendarItem_SetupLayoutItemFrameProc, p_lbl);							
				}
			}
		}
		InnerBlock::MakeYearsLayout(this, p_lo_result); // Года. Располагаются в 1 строку со служебными элементами по краям (стрелки для скроллинга)
		InnerBlock::MakeMonthesLayout(this, p_lo_result); // Месяцы. Располагаются в 2 строки по 6 ячеек
		{
			SUiLayout * p_lo_days_group = p_lo_result->InsertItem(const_cast<LayoutExtra *>(GetLayoutExtra(loiFrame_Days, 0)), 0);
			{
				// Группа дней недели и дней месяца
				SUiLayoutParam alb(DIREC_VERT, 0, SUiLayoutParam::alignStart);
				alb.SetGrowFactor(6.0f).SetVariableSizeX(SUiLayoutParam::szByContainer, 1.0f).SetMargin(def_margin);
				p_lo_days_group->SetLayoutBlock(alb);
			}
			InnerBlock::MakeWeekdaysLayout(this, p_lo_days_group); // Заголовки дней недели		
			InnerBlock::MakeMonthdaysLayout(this, p_lo_days_group, selectedDate); // Дни месяца. Располагаются в 6-ти строковых лейаутах
		}
		{
			SUiLayoutParam alb_buttons(DIREC_HORZ, 0, SUiLayoutParam::alignEnd);
			alb_buttons.SetGrowFactor(1.0f).SetVariableSizeX(SUiLayoutParam::szByContainer, 1.0f);
			SUiLayout * p_lo_buttons = p_lo_result->InsertItem(const_cast<LayoutExtra *>(GetLayoutExtra(loiFrame_Buttons, 0)), &alb_buttons);
			{
				SUiLayoutParam alb;
				alb.SetGrowFactor(1.0f).SetFixedSizeY(FixedCtrlHeight).SetMargin(def_margin);
				//alb.SetVariableSizeY(SUiLayoutParam::szByContainer, 1.0f);
				InnerBlock::InsertButtonLayout(this, p_lo_buttons, CTL_CALENDAR_LEFTRESET, alb, 0.4f);
				InnerBlock::InsertButtonLayout(this, p_lo_buttons, CTL_CALENDAR_RESET, alb, 0.4f);
				InnerBlock::InsertButtonLayout(this, p_lo_buttons, CTL_CALENDAR_RIGHTRESET, alb, 0.4f);
				InnerBlock::InsertButtonLayout(this, p_lo_buttons, STDCTL_OKBUTTON, alb, 1.0f);
				InnerBlock::InsertButtonLayout(this, p_lo_buttons, STDCTL_CANCELBUTTON, alb, 1.0f);
			}
		}
	}
	else if(Kind == kTime) {
		{
			// Группа из 24-х часов
			//
			SUiLayoutParam alb_frame(DIREC_VERT, 0, SUiLayoutParam::alignStretch);
			alb_frame.Flags |= SUiLayoutParam::fContainerWrap;
			alb_frame.GrowFactor = 1.0f;
			alb_frame.SetVariableSizeX(SUiLayoutParam::szByContainer, 1.0f);
			alb_frame.Margin.Set(def_margin);
			SUiLayout * p_lo_frame = p_lo_result->InsertItem(const_cast<LayoutExtra *>(GetLayoutExtra(loiFrame_Hours, 0)), &alb_frame);
			//
			// 3 ряда часов: ночная [0..7] выравнивание влево; дневная [8..18] выравнивание вразбежку; вечерняя [19..24] выравнивание вправо
			//
			{
				SUiLayoutParam alb_row(DIREC_HORZ, SUiLayoutParam::alignStart, 0);
				alb_row.GrowFactor = 1.0f;
				alb_row.SetVariableSizeX(SUiLayoutParam::szByContainer, 1.0f);
				SUiLayout * p_lo_row = p_lo_frame->InsertItem(0, &alb_row);
				for(uint i = 0; i <= 7; i++) {
					SUiLayoutParam alb;
					//alb.GrowFactor = 1.0f;
					alb.AspectRatio = 1.0f;
					alb.ShrinkFactor = 1.0f;
					alb.SetVariableSizeX(SUiLayoutParam::szUndef, 0.0f);
					alb.SetVariableSizeY(SUiLayoutParam::szByContainer, 1.0f);
					alb.Margin.Set(def_margin);
					p_lo_row->InsertItem(const_cast<LayoutExtra *>(GetLayoutExtra(loiFrame_Hours, i+1)), &alb);
				}
			}
			{
				SUiLayoutParam alb_row(DIREC_HORZ, SUiLayoutParam::alignCenter, 0);
				alb_row.GrowFactor = 1.0f;
				alb_row.SetVariableSizeX(SUiLayoutParam::szByContainer, 1.0f);
				SUiLayout * p_lo_row = p_lo_frame->InsertItem(0, &alb_row);
				for(uint i = 8; i <= 18; i++) {
					SUiLayoutParam alb;
					//alb.GrowFactor = 1.0f;
					alb.AspectRatio = 1.0f;
					alb.ShrinkFactor = 1.0f;
					alb.SetVariableSizeX(SUiLayoutParam::szUndef, 0.0f);
					alb.SetVariableSizeY(SUiLayoutParam::szByContainer, 1.0f);
					alb.Margin.Set(def_margin);
					p_lo_row->InsertItem(const_cast<LayoutExtra *>(GetLayoutExtra(loiFrame_Hours, i+1)), &alb);
				}
			}
			{
				SUiLayoutParam alb_row(DIREC_HORZ, SUiLayoutParam::alignEnd, 0);
				alb_row.GrowFactor = 1.0f;
				alb_row.SetVariableSizeX(SUiLayoutParam::szByContainer, 1.0f);
				SUiLayout * p_lo_row = p_lo_frame->InsertItem(0, &alb_row);
				for(uint i = 19; i <= 23; i++) {
					SUiLayoutParam alb;
					//alb.GrowFactor = 1.0f;
					alb.AspectRatio = 1.0f;
					alb.ShrinkFactor = 1.0f;
					alb.SetVariableSizeX(SUiLayoutParam::szUndef, 0.0f);
					alb.SetVariableSizeY(SUiLayoutParam::szByContainer, 1.0f);
					alb.Margin.Set(def_margin);
					p_lo_row->InsertItem(const_cast<LayoutExtra *>(GetLayoutExtra(loiFrame_Hours, i+1)), &alb);
				}
			}
		}
		{
			//
			// 4 ряда минут по 15 минут каждый. Ряд разбит на 3 группы по 5 минут. Пятая минута акцентирована, остальные - мелкими прямоугольниками. 
			//
			SUiLayoutParam alb_frame(DIREC_HORZ, 0, SUiLayoutParam::alignStretch);
			alb_frame.GrowFactor = 1.0f;
			alb_frame.SetVariableSizeX(SUiLayoutParam::szByContainer, 1.0f);
			alb_frame.Margin.Set(def_margin);
			SUiLayout * p_lo_frame_m = p_lo_result->InsertItem(const_cast<LayoutExtra *>(GetLayoutExtra(loiFrame_Minuts, 0)), &alb_frame);
			{
				uint _cur_minute = 0;
				{
					// Zero column
					SUiLayoutParam alb;
					alb.GrowFactor = 0.06f;
					alb.SetVariableSizeY(SUiLayoutParam::szByContainer, 1.0f);
					alb.AlignSelf = SUiLayoutParam::alignCenter;
					alb.Margin.Set(def_margin);
					p_lo_frame_m->InsertItem(const_cast<LayoutExtra *>(GetLayoutExtra(loiFrame_Minuts, 0xffffU)), &alb);
				}
				{
					SUiLayoutParam alb_frame2(DIREC_VERT, 0, SUiLayoutParam::alignCenter);
					alb_frame2.GrowFactor = 1.0f;
					alb_frame2.SetVariableSizeY(SUiLayoutParam::szByContainer, 1.0f);
					alb_frame2.Margin.Set(def_margin);
					SUiLayout * p_lo_frame = p_lo_frame_m->InsertItem(0, &alb_frame2);
					for(uint j = 0; j < 4; j++) {
						SUiLayoutParam alb_row(DIREC_HORZ, SUiLayoutParam::alignCenter, SUiLayoutParam::alignCenter);
						alb_row.GrowFactor = 1.0f;
						alb_row.SetVariableSizeX(SUiLayoutParam::szByContainer, 1.0f);
						alb_row.Margin.Set(def_margin);
						alb_row.Margin.a.x = 0.0f;
						SUiLayout * p_lo_row = p_lo_frame->InsertItem(0, &alb_row);
						for(uint i = 0; i <= 3; i++) {
							SUiLayoutParam alb_5m(DIREC_HORZ, SUiLayoutParam::alignStart, SUiLayoutParam::alignCenter);
							alb_5m.GrowFactor = 1.0f;
							alb_5m.SetVariableSizeY(SUiLayoutParam::szByContainer, 1.0f);
							SUiLayout * p_lo_5 = p_lo_row->InsertItem(0, &alb_5m);
							for(uint k = 1; k <= 5; k++) {
								_cur_minute++;
								SUiLayoutParam alb;
								if(k == 5) {
									alb.GrowFactor = 4.0f;
									alb.SetVariableSizeY(SUiLayoutParam::szByContainer, 1.0f);
								}
								else {
									alb.GrowFactor = 1.0f;
									alb.SetVariableSizeY(SUiLayoutParam::szByContainer, 0.5f);
								}
								alb.AlignSelf = SUiLayoutParam::alignCenter;
								alb.Margin.Set(def_margin);
								alb_row.Margin.a.x = 0.0f;
								SUiLayout * p_lo = p_lo_row->InsertItem(const_cast<LayoutExtra *>(GetLayoutExtra(loiFrame_Minuts, _cur_minute)), &alb);
							}
						}					
					}
				}
			}
		}
		{
			SUiLayoutParam alb_buttons(DIREC_HORZ, 0, SUiLayoutParam::alignEnd);
			alb_buttons.GrowFactor = 0.2f;
			alb_buttons.SetVariableSizeX(SUiLayoutParam::szByContainer, 1.0f);
			SUiLayout * p_lo_buttons = p_lo_result->InsertItem(const_cast<LayoutExtra *>(GetLayoutExtra(loiFrame_Buttons, 0)), &alb_buttons);
			{
				SUiLayoutParam alb;
				alb.GrowFactor = 1.0f;
				alb.AlignSelf = SUiLayoutParam::alignEnd;
				//alb.SetVariableSizeY(SUiLayoutParam::szByContainer, 1.0f);
				alb.SetFixedSizeY(FixedCtrlHeight);
				alb.Margin.Set(def_margin);
				InnerBlock::InsertButtonLayout(this, p_lo_buttons, CTL_CALENDAR_TODAY, alb, 1.0f);
				InnerBlock::InsertButtonLayout(this, p_lo_buttons, STDCTL_OKBUTTON, alb, 1.0f);
				InnerBlock::InsertButtonLayout(this, p_lo_buttons, STDCTL_CANCELBUTTON, alb, 1.0f);
			}
		}
	}
	else if(Kind == kDateTime) {
	}
	SetLayout(p_lo_result);
}

void SCalendarPicker::DrawLayout(TCanvas2 & rCanv, const SUiLayout * pLo)
{
	if(Kind == kPeriod) {
		SString temp_buf;
		temp_buf.Cat(Data.Period, 0);
		setCtrlString(CTL_CALENDAR_PERIODEDIT, temp_buf);
	}
	if(pLo) {
		{
			static const SIntToSymbTabEntry month_symb_list[] = {
				{ 1, "month_jan" }, { 2, "month_feb" }, { 3, "month_mar" }, { 4, "month_apr" }, { 5, "month_may" }, { 6, "month_jun" },
				{ 7, "month_jul" }, { 8, "month_aug" }, { 9, "month_sep" }, { 10, "month_oct" }, { 11, "month_nov" }, { 12, "month_dec" },
			};
			static const SIntToSymbTabEntry month_symb_s_list[] = {
				{ 1, "month_jan_s" }, { 2, "month_feb_s" }, { 3, "month_mar_s" }, { 4, "month_apr_s" }, { 5, "month_may_s" }, { 6, "month_jun_s" },
				{ 7, "month_jul_s" }, { 8, "month_aug_s" }, { 9, "month_sep_s" }, { 10, "month_oct_s" }, { 11, "month_nov_s" }, { 12, "month_dec_s" },
			};
			static const SIntToSymbTabEntry wd_symb_list[] = {
				{ 1, "monday" }, { 2, "tuesday" }, { 3, "wednesday" }, { 4, "thursday" }, { 5, "friday" }, { 6, "saturday" }, { 7, "sunday" },
			};
			static const SIntToSymbTabEntry wd_symb_s_list[] = {
				{ 1, "monday_s" }, { 2, "tuesday_s" }, { 3, "wednesday_s" }, { 4, "thursday_s" }, { 5, "friday_s" }, { 6, "saturday_s" }, { 7, "sunday_s" },
			};
			FRect lo_rect = pLo->GetFrameAdjustedToParent();
			const LayoutExtra * p_lo_extra = static_cast<const LayoutExtra *>(SUiLayout::GetManagedPtr(const_cast<SUiLayout *>(pLo)));
			int   pen_ident = 0;
			int   brush_ident = 0;
			TWhatmanToolArray::Item tool_item;
			const SDrawFigure * p_fig = 0;
			SPaintToolBox * p_tb = APPL->GetUiToolBox();
			if(p_lo_extra && p_tb) {
				SString text_utf8;
				SString symb;
				switch(p_lo_extra->Ident) {
					case loiFrame_PeriodTypeButtons:
						{
							const int prd = static_cast<int>(p_lo_extra->Value);
							if(prd == PeriodTerm) {
								pen_ident = TProgram::tbiIconAlertColor;
								brush_ident = TProgram::tbiListFocBrush;
							}
							else
								pen_ident = TProgram::tbiIconRegColor;
							switch(prd) {
								case PRD_DAY: symb = "day_pl"; break;
								case PRD_WEEK: symb = "week_pl"; break;
								case PRD_MONTH: symb = "month_pl"; break;
								case PRD_QUART: symb = "calquarter_pl"; break;
								case PRD_ANNUAL: symb = "year_pl"; break;
							}
							if(symb.NotEmpty()) {
								PPLoadString(symb, text_utf8);
							}
						}
						break;
					case loiYear:
						{
							const int year = p_lo_extra->Value+StartLoYear;
							const LDATE _isd = ISD();
							//brush_ident = TProgram::tbiIconRegColor;
							if(Kind == kPeriod && !Data.Period.IsZero()) {
								if(Data.Period.CheckDate(encodedate(1, 1, year)) && Data.Period.CheckDate(encodedate(31, 12, year))) {
									pen_ident = TProgram::tbiIconAlertColor;
									brush_ident = TProgram::tbiListSelBrush;								
								}
							}
							if(year == _isd.year()) {
								pen_ident = TProgram::tbiIconAlertColor;
								brush_ident = TProgram::tbiListFocBrush;
							}
							else
								pen_ident = TProgram::tbiIconRegColor;
							text_utf8.Cat(year);
						}
						break;
					case loiYearArrow:
						if(p_lo_extra->Value == SIDE_LEFT)
							p_fig = APPL->LoadDrawFigureById(PPDV_ANGLEARROWLEFT01, &tool_item);
						else if(p_lo_extra->Value == SIDE_RIGHT)
							p_fig = APPL->LoadDrawFigureById(PPDV_ANGLEARROWRIGHT01, &tool_item);
						break;
					case loiMonth:
						//brush_ident = TProgram::tbiIconRegColor;
						pen_ident = TProgram::tbiIconRegColor;
						if(checkirange(p_lo_extra->Value, 1U, 12U)) {
							const LDATE _isd = ISD();
							if(Kind == kPeriod && !Data.Period.IsZero()) {
								int y = _isd.year();
								int m = p_lo_extra->Value;
								int d = _isd.day();
								int dpm = dayspermonth(m, y);
								if(Data.Period.CheckDate(encodedate(1, m, y)) && Data.Period.CheckDate(encodedate(dpm, m, y))) {
									pen_ident = TProgram::tbiIconAlertColor;
									brush_ident = TProgram::tbiListSelBrush;								
								}
							}
							if(p_lo_extra->Value == _isd.month()) {
								pen_ident = TProgram::tbiIconAlertColor;
								brush_ident = TProgram::tbiListFocBrush;
							}
							if(lo_rect.Width() > 60.0f)
								SIntToSymbTab_GetSymb(month_symb_list, SIZEOFARRAY(month_symb_list), p_lo_extra->Value, symb);
							else 
								SIntToSymbTab_GetSymb(month_symb_s_list, SIZEOFARRAY(month_symb_s_list), p_lo_extra->Value, symb);
							PPLoadString(symb, text_utf8);
							//text_utf8.Transf(CTRANSF_INNER_TO_UTF8);
						}
						break;
					case loiWeekday:
						//brush_ident = TProgram::tbiIconRegColor;
						pen_ident = TProgram::tbiIconRegColor;
						if(checkirange(p_lo_extra->Value, 1U, 7U)) {
							const LDATE _isd = ISD();
							if(p_lo_extra->Value == dayofweek(&_isd, 1)) {
								pen_ident = TProgram::tbiIconAlertColor;
								brush_ident = TProgram::tbiListFocBrush;
							}
							if(lo_rect.Width() > 60.0f)
								SIntToSymbTab_GetSymb(wd_symb_list, SIZEOFARRAY(wd_symb_list), p_lo_extra->Value, symb);
							else
								SIntToSymbTab_GetSymb(wd_symb_s_list, SIZEOFARRAY(wd_symb_s_list), p_lo_extra->Value, symb);
							PPLoadString(symb, text_utf8);
							//text_utf8.Transf(CTRANSF_INNER_TO_UTF8);
						}
						break;
					case loiDay:
						//brush_ident = TProgram::tbiIconRegColor;
						pen_ident = TProgram::tbiIconRegColor;
						if(checkirange(p_lo_extra->Value, 1U, 31U)) {
							LDATE _d;
							const LDATE _isd = ISD();
							_d.encode(p_lo_extra->Value, _isd.month(), _isd.year());
							if(Kind == kPeriod && !Data.Period.IsZero() && Data.Period.CheckDate(_d)) {
								pen_ident = TProgram::tbiIconAlertColor;
								brush_ident = TProgram::tbiListSelBrush;								
							}
							if(p_lo_extra->Value == _isd.day()) {
								pen_ident = TProgram::tbiIconAlertColor;
								brush_ident = TProgram::tbiListFocBrush;
							}
							text_utf8.Cat(p_lo_extra->Value);
						}
						break;
					case loiFrame_Years: pen_ident = TProgram::tbiIconRegColor; break;
					case loiFrame_Monthes: pen_ident = TProgram::tbiIconRegColor; break;
					case loiFrame_Days: pen_ident = TProgram::tbiIconRegColor; break;
					case loiFrame_Hours:
						pen_ident = TProgram::tbiIconRegColor; 
						if(checkirange(p_lo_extra->Value, 1U, 24U) && Data.Dtm.t.hour() == (p_lo_extra->Value-1)) {
							pen_ident = TProgram::tbiIconAlertColor;
							brush_ident = TProgram::tbiListFocBrush;								
						}
						if(checkirange(p_lo_extra->Value, 1U, 24U)) {
							text_utf8.Cat(p_lo_extra->Value-1);
						}
						break;
					case loiFrame_Minuts: 
						if(p_lo_extra->Value != 60) {
							pen_ident = TProgram::tbiIconRegColor; 
							if(checkirange(p_lo_extra->Value, 1U, 59U) && Data.Dtm.t.minut() == p_lo_extra->Value) {
								pen_ident = TProgram::tbiIconAlertColor;
								brush_ident = TProgram::tbiListFocBrush;								
							}
							else if(p_lo_extra->Value == 0xffffU && Data.Dtm.t.minut() == 0) {
								pen_ident = TProgram::tbiIconAlertColor;
								brush_ident = TProgram::tbiListFocBrush;								
							}
						}
						if(p_lo_extra->Value) {
							if(p_lo_extra->Value == 0xffffU)
								text_utf8.Cat(":00");
							else if(p_lo_extra->Value > 0 && p_lo_extra->Value < 60 && (p_lo_extra->Value % 5) == 0)
								text_utf8.Cat(p_lo_extra->Value);
						}
						break;
				}
				// Прежде всего закрасим фон
				rCanv.Rect(lo_rect, 0, TProgram::tbiListBkgBrush);
				if(pen_ident) {
					if(pLo == P_LoFocused) {
						pen_ident = TProgram::tbiIconAccentColor;
					}
					rCanv.Rect(lo_rect);
					rCanv.Stroke(pen_ident, 0);
				}
				if(brush_ident) {
					rCanv.Rect(lo_rect);
					rCanv.Fill(brush_ident, 0);
				}
				if(p_fig) {
					const uint _w = 16;
					const uint _h = 16;
					SImageBuffer ib(_w, _h);
					{
						if(!tool_item.ReplacedColor.IsEmpty()) {
							SColor replacement_color = p_tb->GetColor(TProgram::tbiIconRegColor);
							rCanv.SetColorReplacement(tool_item.ReplacedColor, replacement_color);
						}
						LMatrix2D mtx;
						SViewPort vp;
						rCanv.Fill(SColor(192, 192, 192, 255), 0); // Прозрачный фон
						rCanv.PushTransform();
						p_fig->GetViewPort(&vp);
						rCanv.PushTransform();
						rCanv.AddTransform(vp.GetMatrix(lo_rect, mtx));
						rCanv.Draw(p_fig);
						rCanv.PopTransform();
					}
				}
				if(text_utf8.NotEmpty()) {
					if(FontId > 0) {
						SDrawContext dctx = rCanv;
						if(CStyleId <= 0) {
							int    tool_text_brush_id = 0; //SPaintToolBox::rbr3DFace;
							CStyleId = p_tb->CreateCStyle(0, FontId, TProgram::tbiBlackPen, tool_text_brush_id);
						}
						if(CStyleFocusId <= 0) {
							int    tool_text_brush_id = 0; //SPaintToolBox::rbr3DFace;
							CStyleFocusId = p_tb->CreateCStyle(0, FontId, TProgram::tbiWhitePen, tool_text_brush_id);
						}
						SParaDescr pd;
						pd.Flags |= SParaDescr::fJustCenter;
						int    tid_para = p_tb->CreateParagraph(0, &pd);
						STextLayout tlo;
						tlo.SetText(text_utf8);
						tlo.SetOptions(STextLayout::fOneLine|STextLayout::fVCenter, tid_para, (brush_ident == TProgram::tbiListFocBrush) ? CStyleFocusId : CStyleId);
						tlo.SetBounds(lo_rect);
						tlo.Arrange(dctx, *p_tb);
						rCanv.DrawTextLayout(&tlo);
					}					
				}
			}
			else {
				//rCanv.Rect(lo_rect);
				//rCanv.Stroke(TProgram::tbiBlackPen, 1);
			}
		}
		for(uint ci = 0; ci < pLo->GetChildrenCount(); ci++) {
			DrawLayout(rCanv, pLo->GetChildC(ci)); // @recursion
		}
	}
}

SUiLayout * SCalendarPicker::Helper_FindLayout(SUiLayout * pItem, int extraIdent, uint extraValue)
{
	SUiLayout * p_result = 0;
	if(extraIdent && pItem->IsConsistent()) {
		const LayoutExtra * p_mp = static_cast<const LayoutExtra *>(SUiLayout::GetManagedPtr(pItem));
		if(p_mp && p_mp->Ident == extraIdent && p_mp->Value == extraValue)
			p_result = pItem;
		else {
			for(uint i = 0; !p_result && i < pItem->GetChildrenCount(); i++)
				p_result = Helper_FindLayout(pItem->GetChild(i), extraIdent, extraValue); // @recursion
		}
	}
	return p_result;
}

SUiLayout * SCalendarPicker::FindLayout(int extraIdent, uint extraValue)
{
	return Helper_FindLayout(P_Lfc, extraIdent, extraValue);
}

LDATE SCalendarPicker::AdjustLeftDate(int prdType, LDATE d) const
{
	if(checkdate(d)) {
		switch(prdType) {
			case PRD_WEEK:
				{
					const int dow = dayofweek(&d, 1);
					if(dow > 1)
						d = plusdate(d, -(dow-1));
				}
				break;
			case PRD_MONTH:
				if(d.day() > 1)
					d.setday(1);
				break;
			case PRD_QUART:
				{
					const int m = d.month();
					if(d.day() > 1 || !oneof4(m, 1, 4, 7, 10)) {
						d.setday(1);
						d.setmonth(((m/4)*3)+1);
						assert(checkdate(d));
					}
				}
				break;
			case PRD_ANNUAL:
				if(d.day() != 1 || d.month() != 1) {
					d.setday(1);
					d.setmonth(1);
				}
				break;
		}
	}
	return d;
}

LDATE SCalendarPicker::AdjustRightDate(int prdType, LDATE d) const
{
	if(checkdate(d)) {
		switch(prdType) {
			case PRD_WEEK:
				{
					const int dow = dayofweek(&d, 1);
					if(dow >= 1 && dow <= 6)
						d = plusdate(d, (7-dow));
				}
				break;
			case PRD_MONTH:
				{
					const int lastday = d.dayspermonth();
					if(d.day() < lastday)
						d.setday(lastday);
				}
				break;
			case PRD_QUART:
				{
					const int m = d.month();
					const int dpm = dayspermonth(m, d.year());
					if(d.day() < dpm || !oneof4(m, 3, 6, 9, 12)) {
						const int m2 = ((m/4)*3)+3;
						d.setmonth(m2);
						d.setday(dayspermonth(m2, d.year()));
					}
				}
				break;
			case PRD_ANNUAL:
				if(d.day() != 31 || d.month() != 12) {
					d.setday(31);
					d.setmonth(12);
				}
				break;
		}
	}
	return d;
}

void SCalendarPicker::UpdateSelectedPeriod(const DateRange * pNewPeriod)
{
	if(Kind == kPeriod) {
		RVALUEPTR(Data.Period, pNewPeriod);
		SUiLayout::RefCollection redraw_lo_list;
		redraw_lo_list.Add(FindLayout(loiFrame_Days, 0));
		redraw_lo_list.Add(FindLayout(loiFrame_Monthes, 0));
		redraw_lo_list.Add(FindLayout(loiFrame_Years, 0));
		redraw_lo_list.Add(FindLayout(loiFrame_PeriodTypeButtons, 0));
		InvalidateLayoutRefList(redraw_lo_list, 1);
		::UpdateWindow(H());
		{
			DateRange np;
			long predefprdid = getCtrlLong(CTLSEL_CALENDAR_FASTPRD);
			if(np.SetPredefined(predefprdid, ZERODATE)) {
				if(np != Data.Period)
					setCtrlLong(CTLSEL_CALENDAR_FASTPRD, 0L);
			}
		}
	}
}

IMPL_HANDLE_EVENT(SCalendarPicker)
{
	TWindowBase::handleEvent(event);
	if(event.isKeyDown(kbEsc)) {
		if(IsInState(sfModal)) {
			EndModalCmd = cmCancel;
			clearEvent(event);
		}
	}
	else if(TVINFOPTR) {
		if(event.isCmd(cmInit)) {
			CreateBlock * p_blk = static_cast<CreateBlock *>(TVINFOPTR);
			{
				const uint icon_size = 48;
				{
					TWhatmanToolArray::Item tool_item;
					uint fig_id = 0;
					if(Kind == kTime)
						fig_id = PPDV_CLOCK02;
					else if(Kind == kPeriod)
						fig_id = PPDV_CALENDAR03;
					else if(Kind == kDate)
						fig_id = PPDV_CALENDARDAY01;
					if(fig_id) {
						const SDrawFigure * p_fig = APPL->LoadDrawFigureById(fig_id, &tool_item);
						SPaintToolBox * p_tb = APPL->GetUiToolBox();
						if(p_tb && p_fig) {
							const uint _w = icon_size;
							const uint _h = icon_size;
							SImageBuffer ib(_w, _h);
							{
								TCanvas2 canv_temp(*p_tb, ib);
								if(!tool_item.ReplacedColor.IsEmpty()) {
									SColor replacement_color;
									replacement_color = p_tb->GetColor(TProgram::tbiIconRegColor);
									canv_temp.SetColorReplacement(tool_item.ReplacedColor, replacement_color);
								}
								LMatrix2D mtx;
								SViewPort vp;
								FRect pic_bounds(static_cast<float>(_w), static_cast<float>(_h));
								//pic_bounds.a.SetZero();
								//pic_bounds.b.Set(static_cast<float>(_w), static_cast<float>(_h));
								//
								canv_temp.Rect(pic_bounds);
								canv_temp.Fill(SColor(SClrWhite), 0);
								//canv_temp.Fill(SColor(0xd4, 0xf0, 0xf0, 255), 0); // Прозрачный фон
								canv_temp.PushTransform();
								p_fig->GetViewPort(&vp);
								canv_temp.AddTransform(vp.GetMatrix(pic_bounds, mtx));
								canv_temp.Draw(p_fig);
							}
							HICON h_icon = static_cast<HICON>(ib.TransformToIcon());
							if(h_icon) {
								::SendMessage(H(), WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(h_icon));
								::DestroyIcon(h_icon);
							}
						}
					}
				}
			}
			{
				const TRect _def_rect(0, 0, 10, 10);
				if(Kind == kDate)
					InsertCtlWithCorrespondingNativeItem(new TButton(_def_rect, "@today", cmNow, 0, 0), CTL_CALENDAR_TODAY, 0, /*extraPtr*/0);
				if(Kind == kTime)
					InsertCtlWithCorrespondingNativeItem(new TButton(_def_rect, "@currenttime", cmNow, 0, 0), CTL_CALENDAR_TODAY, 0, /*extraPtr*/0);
				else if(Kind == kPeriod) {
					{
						TInputLine * p_il = new TInputLine(_def_rect, S_ZSTRING, MKSFMT(128, 0));
						ComboBox * p_cb = new ComboBox(_def_rect, cbxAllowEmpty|cbxDisposeData|cbxListOnly, p_il);
						p_il->SetId(CTL_CALENDAR_FASTPRD);
						p_cb->SetId(CTLSEL_CALENDAR_FASTPRD);
						InsertCtlWithCorrespondingNativeItem(p_cb, CTLSEL_CALENDAR_FASTPRD, 0, /*extraPtr*/0);
						{
							StrAssocArray list;
							SString temp_buf, left_buf, right_buf;
							PPLoadText(PPTXT_FASTPRD2, temp_buf);
							StringSet ss(';', temp_buf);
							for(uint p = 0; ss.get(&p, temp_buf);) {
								if(temp_buf.Divide(',', left_buf, right_buf) > 0) {
									list.Add(left_buf.ToLong(), right_buf);
								}
								//SendDlgItemMessage(hWnd, CTL_CALENDAR_FASTPRD, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(SUcSwitch(temp_buf.cptr())));
							}
							SetupStrAssocCombo(this, CTLSEL_CALENDAR_FASTPRD, list, 0, 0, 0, 0);
						}
					}
					{
						TInputLine * p_il = new TInputLine(_def_rect, MKSTYPE(S_ZSTRING, 256), MKSFMT(256, 0));
						TLabel * p_lbl = new TLabel(_def_rect, "@daterange", p_il);
						InsertCtlWithCorrespondingNativeItem(p_il, CTL_CALENDAR_PERIODEDIT, 0, /*extraPtr*/0);
						InsertCtlWithCorrespondingNativeItem(p_lbl, 0, 0, /*extraPtr*/0);
					}
					{
						InsertCtlWithCorrespondingNativeItem(new TButton(_def_rect, "<<..", cmPeriodResetLeft, 0, 0), CTL_CALENDAR_LEFTRESET, 0, /*extraPtr*/0);
						InsertCtlWithCorrespondingNativeItem(new TButton(_def_rect, "...", cmPeriodReset, 0, 0), CTL_CALENDAR_RESET, 0, /*extraPtr*/0);
						InsertCtlWithCorrespondingNativeItem(new TButton(_def_rect, "..>>", cmPeriodResetRight, 0, 0), CTL_CALENDAR_RIGHTRESET, 0, /*extraPtr*/0);
					}
				}
				InsertCtlWithCorrespondingNativeItem(new TButton(_def_rect, "@but_ok", cmOK, TButton::spcfDefault, 0), STDCTL_OKBUTTON, 0, /*extraPtr*/0);
				InsertCtlWithCorrespondingNativeItem(new TButton(_def_rect, "@but_cancel", cmCancel, 0, 0), STDCTL_CANCELBUTTON, 0, /*extraPtr*/0);
			}
			CreateLayout(ISD());
			EvaluateLayout(p_blk->Coord);
			// @v11.2.6 invalidateAll(true);
			//::UpdateWindow(H());
		}
		else if(event.isCbSelected(CTLSEL_CALENDAR_FASTPRD)) {
			if(Kind == kPeriod) {
				long predefprdid = getCtrlLong(CTLSEL_CALENDAR_FASTPRD);
				DateRange np;
				if(np.SetPredefined(predefprdid, ZERODATE))
					UpdateSelectedPeriod(&np);
			}
		}
		else if(event.isCmd(cmClose)) {
			if(IsInState(sfModal)) {
				EndModalCmd = cmCancel;
				clearEvent(event);
			}
		}
		else if(event.isCmd(cmOK) || event.isCmd(cmCancel)) {
			if(IsInState(sfModal)) {
				EndModalCmd = event.message.command;
				clearEvent(event);
			}
			else if(event.message.command == cmCancel) {
				close();
				return; // Окно разрушено - делать в этой процедуре больше нечего!
			}
		}
		else if(event.isCmd(cmNow)) {
			const LDATETIME now_dtm = getcurdatetime_();
			if(Kind == kDate || Kind == kPeriod) {
				if(Data.Dtm.d != now_dtm.d) {
					Data.Dtm.d = now_dtm.d;
					StartLoYear = Data.Dtm.d.year()-2;
					CreateLayout(Data.Dtm.d);
					if(P_Lfc) {
						EvaluateLayout(getClientRect());
						invalidateAll(true);
						::UpdateWindow(H());
					}
				}
			}
			else if(Kind == kTime) {
				if(Data.Dtm.t != now_dtm.t) {
					Data.Dtm.t = now_dtm.t;
					invalidateAll(true);
					::UpdateWindow(H());
				}
			}
		}
		else if(event.isCmd(cmPeriodReset)) {
			if(Kind == kPeriod && !Data.Period.IsZero()) {
				Data.Period.Z();
				UpdateSelectedPeriod(0);
			}
		}
		else if(event.isCmd(cmPeriodResetLeft)) {
			if(Kind == kPeriod && checkdate(Data.Period.low)) {
				Data.Period.low = ZERODATE;
				UpdateSelectedPeriod(0);
			}
		}
		else if(event.isCmd(cmPeriodResetRight)) {
			if(Kind == kPeriod && checkdate(Data.Period.upp)) {
				Data.Period.upp = ZERODATE;
				UpdateSelectedPeriod(0);
			}
		}
		else if(event.isCmd(cmMouse)) {
			MouseEvent * p_blk = static_cast<MouseEvent *>(TVINFOPTR);
			bool  do_redraw = false;
			bool  do_rebuild = false;
			SUiLayout::RefCollection redraw_lo_list;
			const SUiLayout * p_prev_focused = P_LoFocused;
			const DateRange prev_selected_period = Data.Period;
			P_LoFocused = 0;
			if(P_Lfc) {
				P_LoFocused = P_Lfc->FindMinimalItemAroundPoint(p_blk->Coord.x, p_blk->Coord.y);
			}
			if(p_blk->Type == MouseEvent::tWeel) {
				if(p_blk->WeelDelta != 0) {
					LDATE _d = Data.Dtm;
					plusperiod(&_d, PRD_MONTH, (p_blk->WeelDelta/WHEEL_DELTA), 0);
					if(checkdate(_d)) {
						Data.Dtm.d = _d;
						do_rebuild = true;
					}
				}
			}
			else if(p_blk->Type == MouseEvent::tMove) { // @v11.2.8
				const LayoutExtra * p_lo_extra = static_cast<const LayoutExtra *>(SUiLayout::GetManagedPtr(const_cast<SUiLayout *>(P_LoFocused)));
				if(p_lo_extra && p_lo_extra->Ident)
					::SetCursor(::LoadCursor(0, IDC_ARROW));
			}
			else if(p_blk->Type == MouseEvent::tLDblClk) {
				if(IsInState(sfModal)) {
					if(Kind == kDate) {
						const LayoutExtra * p_lo_extra = static_cast<const LayoutExtra *>(SUiLayout::GetManagedPtr(const_cast<SUiLayout *>(P_LoFocused)));
						if(p_lo_extra->Ident == loiDay) {
							const int d = p_lo_extra->Value;
							const LDATE _isd = ISD();
							const int y = _isd.year();
							const int m = _isd.month();
							const int dpm = dayspermonth(m, y);
							if(checkirange(d, 1, dpm)) {
								Data.Dtm.d.encode(d, m, y);
								EndModalCmd = cmOK;
								clearEvent(event);
							}
						}
					}
					else if(Kind == kPeriod) {
						const LayoutExtra * p_lo_extra = static_cast<const LayoutExtra *>(SUiLayout::GetManagedPtr(const_cast<SUiLayout *>(P_LoFocused)));
						if(PeriodTerm == PRD_MONTH && p_lo_extra->Ident == loiMonth) {
							const int m = p_lo_extra->Value;
							if(m >= 1 && m <= 12) {
								const LDATE _isd = ISD();
								const int y = _isd.year();
								Data.Period.low = encodedate(1, m, y);
								Data.Period.upp = encodedate(dayspermonth(m, y), m, y);
								EndModalCmd = cmOK;
								clearEvent(event);
							}
						}
						else if(PeriodTerm == PRD_ANNUAL && p_lo_extra->Ident == loiYear) {
							const int y = p_lo_extra->Value+StartLoYear;
							if(y > 1600 && y < 2200) {
								Data.Period.low = encodedate(1, 1, y);
								Data.Period.upp = encodedate(31, 12, y);								
								EndModalCmd = cmOK;
								clearEvent(event);
							}
						}
					}
					else if(Kind == kTime) {
						const LayoutExtra * p_lo_extra = static_cast<const LayoutExtra *>(SUiLayout::GetManagedPtr(const_cast<SUiLayout *>(P_LoFocused)));
						if(p_lo_extra->Ident == loiFrame_Minuts) {
							const int m = p_lo_extra->Value;
							bool do_finish = false;
							if(m == 0xffffU) {
								Data.Dtm.t.encode(Data.Dtm.t.hour(), 0, 0, 0);
								do_finish = true;
							}
							else if(checkirange(m, 1, 59)) {
								Data.Dtm.t.encode(Data.Dtm.t.hour(), m, 0, 0);
								do_finish = true;
							}
							if(do_finish) {
								EndModalCmd = cmOK;
								clearEvent(event);								
							}
						}
					}
				}
			}
			else if(p_blk->Type == MouseEvent::tRDown) {
				if(Kind == kPeriod && !Data.Period.IsZero()) {
					Data.Period.Z();
					UpdateSelectedPeriod(0);
				}
			}
			else if(p_blk->Type == MouseEvent::tLDown) {
				const LayoutExtra * p_lo_extra = static_cast<const LayoutExtra *>(SUiLayout::GetManagedPtr(const_cast<SUiLayout *>(P_LoFocused)));
				if(p_lo_extra) {
					switch(p_lo_extra->Ident) {
						case loiFrame_PeriodTypeButtons:
							if(oneof5(p_lo_extra->Value, PRD_DAY, PRD_WEEK, PRD_MONTH, PRD_QUART, PRD_ANNUAL)) {	
								const long prev_period_term = PeriodTerm;
								PeriodTerm = static_cast<long>(p_lo_extra->Value);
								if(PeriodTerm != prev_period_term) {
									if(!Data.Period.IsZero()) {
										Data.Period.low = AdjustLeftDate(PeriodTerm, Data.Period.low);
										Data.Period.upp = AdjustRightDate(PeriodTerm, Data.Period.upp);
									}
									UpdateSelectedPeriod(0);
								}
							}
							break;
						case loiYearArrow:
							{
								const uint prev_start_year = StartLoYear;
								SetupStartLoYear();
								if(p_lo_extra->Value == SIDE_LEFT) {
									if(StartLoYear > 1600)
										StartLoYear--;
								}
								else if(p_lo_extra->Value == SIDE_RIGHT) {
									if(StartLoYear < 2200)
										StartLoYear++;
								}
								if(StartLoYear != prev_start_year) {
									redraw_lo_list.Add(FindLayout(loiFrame_Days, 0));
									redraw_lo_list.Add(FindLayout(loiFrame_Monthes, 0));
									redraw_lo_list.Add(FindLayout(loiFrame_Years, 0));
									redraw_lo_list.Add(FindLayout(loiFrame_PeriodTypeButtons, 0));
									//invalidateAll(true);
									do_redraw = true;
								}
							}
							break;
						case loiYear:
							{
								const int year = p_lo_extra->Value+StartLoYear;
								const LDATE _isd = ISD();
								/*if(year != _isd.year())*/{
									int m = _isd.month();
									int d = _isd.day();
									const int dpm = dayspermonth(m, year);
									SETMIN(d, dpm);
									Data.Dtm.d.encode(d, m, year);
									if(Kind == kPeriod) {
										if(PeriodTerm == PRD_ANNUAL) {
											DateRange _p;
											_p.Set(AdjustLeftDate(PeriodTerm, encodedate(1, 1, year)), AdjustRightDate(PeriodTerm, encodedate(31, 12, year)));
											if(checkdate(_p.low) && checkdate(_p.upp)) {
												if(_p.low < Data.Period.low || !Data.Period.low) {
													if(Data.Period.upp < _p.low) // @v11.2.5
														Data.Period.upp = NZOR(AdjustRightDate(PeriodTerm, Data.Period.low), _p.upp);
													Data.Period.low = _p.low;
												}
												else if(_p.upp > Data.Period.upp) {
													if(Data.Period.low > _p.upp)
														Data.Period.low = NZOR(AdjustLeftDate(PeriodTerm, Data.Period.upp), _p.low);
													Data.Period.upp = _p.upp;
												}
												UpdateSelectedPeriod(0);
											}
										}
									}
									do_rebuild = true;
								}
							}
							break;
						case loiMonth:
							{
								const int month = p_lo_extra->Value;
								const LDATE _isd = ISD();
								if(/*month != _isd.month() &&*/checkirange(month, 1, 12)) {
									int y = _isd.year();
									int d = _isd.day();
									const int dpm = dayspermonth(month, y);
									SETMIN(d, dpm);
									Data.Dtm.d.encode(d, month, y);
									if(Kind == kPeriod) {
										if(oneof3(PeriodTerm, PRD_MONTH, PRD_QUART, PRD_ANNUAL)) {
											DateRange _p;
											_p.Set(AdjustLeftDate(PeriodTerm, encodedate(1, month, y)), AdjustRightDate(PeriodTerm, encodedate(dpm, month, y)));
											if(checkdate(_p.low) && checkdate(_p.upp)) {
												if(_p.low < Data.Period.low || !Data.Period.low) {
													if(Data.Period.upp < _p.low) // @v11.2.5
														Data.Period.upp = NZOR(AdjustRightDate(PeriodTerm, Data.Period.low), _p.upp);
													Data.Period.low = _p.low;
												}
												else if(_p.upp > Data.Period.upp) {
													if(Data.Period.low > _p.upp)
														Data.Period.low = NZOR(AdjustLeftDate(PeriodTerm, Data.Period.upp), _p.low);
													Data.Period.upp = _p.upp;
												}
												UpdateSelectedPeriod(0);
											}
										}
									}
									do_rebuild = true;
								}
							}
							break;
						case loiWeekday:
							break;
						case loiDay:
							{
								const int d = p_lo_extra->Value;
								const LDATE _isd = ISD();
								/*if(d != _isd.day())*/{
									int y = _isd.year();
									int m = _isd.month();
									const int dpm = dayspermonth(m, y);
									if(checkirange(d, 1, dpm)) {
										Data.Dtm.d.encode(d, m, y);
										if(Kind == kPeriod) {
											DateRange _p;
											const LDATE _isd = ISD();
											_p.Set(AdjustLeftDate(PeriodTerm, _isd), AdjustRightDate(PeriodTerm, _isd));
											if(checkdate(_p.low) && checkdate(_p.upp)) {
												if(_p.low < Data.Period.low || !Data.Period.low) {
													if(Data.Period.upp < _p.low) // @v11.2.5
														Data.Period.upp = NZOR(AdjustRightDate(PeriodTerm, Data.Period.low), _p.upp);
													Data.Period.low = _p.low;
												}
												else if(_p.upp > Data.Period.upp) {
													if(Data.Period.low > _p.upp)
														Data.Period.low = NZOR(AdjustLeftDate(PeriodTerm, Data.Period.upp), _p.low);
													Data.Period.upp = _p.upp;
												}
												UpdateSelectedPeriod(0);
											}
											else {
												redraw_lo_list.Add(FindLayout(loiFrame_Days, 0));
												do_redraw = true;
											}
										}
										else {
											redraw_lo_list.Add(FindLayout(loiFrame_Days, 0));
											do_redraw = true;
										}
									}
								}
							}
							break;
						case loiFrame_Hours:
							{
								const int h = p_lo_extra->Value;
								if(checkirange(h, 1, 24)) {
									Data.Dtm.t.encode(h-1, Data.Dtm.t.minut(), 0, 0);
									redraw_lo_list.Add(FindLayout(loiFrame_Hours, 0));
									redraw_lo_list.Add(FindLayout(loiFrame_Minuts, 0));
									do_redraw = true;
								}
							}
							break;
						case loiFrame_Minuts:
							{
								const int m = p_lo_extra->Value;
								if(m == 0xffffU) {
									Data.Dtm.t.encode(Data.Dtm.t.hour(), 0, 0, 0);
									redraw_lo_list.Add(FindLayout(loiFrame_Hours, 0));
									redraw_lo_list.Add(FindLayout(loiFrame_Minuts, 0));
									do_redraw = true;
								}
								else if(checkirange(m, 1, 59)) {
									Data.Dtm.t.encode(Data.Dtm.t.hour(), m, 0, 0);
									redraw_lo_list.Add(FindLayout(loiFrame_Hours, 0));
									redraw_lo_list.Add(FindLayout(loiFrame_Minuts, 0));
									do_redraw = true;
								}
							}
							break;
					}
				}
			}
			if(do_rebuild) {
				CreateLayout(ISD());
				if(P_Lfc) {
					EvaluateLayout(getClientRect());
					invalidateAll(true);
					do_redraw = true;
				}
			}
			if(p_prev_focused != P_LoFocused) {
				redraw_lo_list.Add(p_prev_focused);
				redraw_lo_list.Add(P_LoFocused);
				do_redraw = true;
			}
			if(do_redraw) {
				InvalidateLayoutRefList(redraw_lo_list, 1);
				::UpdateWindow(H());
			}
		}
		else if(event.isCmd(cmPaint)) {
			PaintEvent * p_blk = static_cast<PaintEvent *>(TVINFOPTR);
			CreateFont_();
			if(oneof2(p_blk->PaintType, PaintEvent::tPaint, PaintEvent::tEraseBackground)) {
				SPaintToolBox * p_tb = APPL->GetUiToolBox();
				if(p_tb) {
					if(GetWbCapability() & wbcDrawBuffer) {
						// Если используется буферизованная отрисовка, то фон нужно перерисовать в любом случае а на событие PaintEvent::tEraseBackground
						// не реагировать
						if(p_blk->PaintType == PaintEvent::tPaint) {
							TCanvas2 canv(*p_tb, static_cast<HDC>(p_blk->H_DeviceContext));
							canv.Rect(p_blk->Rect, 0, TProgram::tbiListBkgBrush);
							DrawLayout(canv, P_Lfc);
						}
					}
					else {
						TCanvas2 canv(*p_tb, static_cast<HDC>(p_blk->H_DeviceContext));
						if(p_blk->PaintType == PaintEvent::tEraseBackground)
							canv.Rect(p_blk->Rect, 0, TProgram::tbiListBkgBrush);
						if(p_blk->PaintType == PaintEvent::tPaint)
							DrawLayout(canv, P_Lfc);
					}
				}
				clearEvent(event);
			}
		}
	}
}
//
/*static*/int SCalendarPicker::Exec(const int kind, DataBlock & rData)
{
	int    ok = -1;
	TRect  b;
	switch(kind) {
		case SCalendarPicker::kDate: b.Set(0, 0, 295, 340); break;
		case SCalendarPicker::kPeriod: b.Set(0, 0, 295, 440); break;
		case SCalendarPicker::kTime: b.Set(0, 0, 452, 306); break;
	}
	SCalendarPicker * p_win = new SCalendarPicker(kind);
	THROW_MEM(p_win);
	p_win->setBounds(b);
	p_win->setDTS(&rData);
	ok = APPL->P_DeskTop->execView(p_win);
	if(ok > 0) {
		p_win->getDTS(&rData);
	}
	CATCHZOK
	delete p_win;
	return ok;
}

/*static*/int SCalendarPicker::Exec(const int kind, TDialog * pParentDlg, uint inputCtlId)
{
	int    ok = -1;
	SCalendarPicker * p_win = 0;
	if(pParentDlg) {
		TView * p_view = pParentDlg->getCtrlView(inputCtlId);
		if(p_view) {
			TRect  b;
			DataBlock blk;
			blk.Dtm = getcurdatetime_();
			switch(kind) {
				case SCalendarPicker::kDate: 
					b.Set(0, 0, 295, 340);
					blk.Dtm.d = pParentDlg->getCtrlDate(inputCtlId);
					break;
				case SCalendarPicker::kPeriod: 
					b.Set(0, 0, 295, 440); 
					GetPeriodInput(pParentDlg, inputCtlId, &blk.Period);
					if(checkdate(blk.Period.low))
						blk.Dtm.d = blk.Period.low;
					else if(checkdate(blk.Period.upp))
						blk.Dtm.d = blk.Period.upp;
					break;
				case SCalendarPicker::kTime: 
					b.Set(0, 0, 452, 306); 
					blk.Dtm.t = pParentDlg->getCtrlTime(inputCtlId);
					break;
				default:
					CALLEXCEPT();
			}
			{
				HWND   h_ctl = GetDlgItem(pParentDlg->H(), inputCtlId);
				if(h_ctl) {
					RECT rect;
					GetWindowRect(h_ctl, &rect);
					int y = rect.bottom;
					// @v11.3.4 {
					const int sizey = b.height();
					const int sy = GetSystemMetrics(SM_CYFULLSCREEN);
					if((y + sizey) > sy)
						y = ((rect.top < sizey) ? sy : rect.top) - sizey;
					// } @v11.3.4
					b.move(rect.left, y);
				}
			}
			SlBreakpointCondition[0] = true; // @debug
			p_win = new SCalendarPicker(kind);
			THROW_MEM(p_win);
			p_win->setBounds(b);
			p_win->setDTS(&blk);
			ok = APPL->P_DeskTop->execView(p_win);
			SlBreakpointCondition[0] = false; // @debug
			if(ok > 0) {
				p_win->getDTS(&blk);
				switch(kind) {
					case SCalendarPicker::kDate: 
						pParentDlg->setCtrlDate(inputCtlId, blk.Dtm.d);
						break;
					case SCalendarPicker::kPeriod: 
						SetPeriodInput(pParentDlg, inputCtlId, &blk.Period);
						break;
					case SCalendarPicker::kTime: 
						pParentDlg->setCtrlTime(inputCtlId, blk.Dtm.t);
						break;
					default:
						assert(0);
				}
			}
		}
	}
	CATCHZOK
	delete p_win;
	return ok;
}

#if 0 // @construction finished {
int Test_Launch_SCalendarPicker()
{
	int    ok = 1;
	int    kind = SCalendarPicker::kDate/*kPeriod*//*kTime*/;
	TRect b;
	switch(kind) {
		case SCalendarPicker::kDate: b.set(0, 0, 295, 340); break;
		case SCalendarPicker::kPeriod: b.set(0, 0, 295, 440); break;
		case SCalendarPicker::kTime: b.set(0, 0, 452, 306); break;
	}
	SCalendarPicker * p_win = new SCalendarPicker(kind);
	THROW_MEM(p_win);
	p_win->setBounds(b);
	ok = APPL->P_DeskTop->execView(p_win);
	CATCHZOK
	delete p_win;
	return ok;
}
#endif // } @construction finished
