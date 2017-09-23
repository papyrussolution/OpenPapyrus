// PPCOLORS.CPP
// Copyright (c) A.Starodub 2007, 2009, 2010, 2011, 2013, 2016, 2017
//
#include <pp.h>
#pragma hdrstop

#define RECT_WIDTH	5
#define RADIUS	    100

#define TOSCALE(x)	  (((x)*RADIUS)/255.0)
#define SCALETOMAX(x) (((x)*255.0)/RADIUS)
#define DISTANCE(pt1, pt2) sqrt((double)((pt1.x - pt2.x) * (pt1.x - pt2.x) + (pt1.y - pt2.y) * (pt1.y - pt2.y)))
#define CALC_LINE_K(pt1, pt2) (pt2.x != pt1.x) ? (double)(pt2.y - pt1.y) / (double)(pt2.x - pt1.x) : 0
#define CALC_LINE_B(pt, k) (double)pt.y - (double)pt.x * k

struct HSV {
	COLORREF ToRGB();
	int H;
	int S;
	int V;
};

COLORREF HSV::ToRGB()
{
	int    r = 0, g = 0, b = 0;
	double min = 0, max = 0, delta = 0, hue = 0;
	if(!H  && !S)
		r = g = b = V;
	max = V;
	delta = (max * S) / 255.0;
	min = max - delta;
	hue = H;
	if(H > 300 || H <= 60) {
		r = (int)max;
		if(H > 300) {
			g = (int)min;
			hue = (hue - 360.0) / 60.0;
			b = (int)((hue * delta - min) * -1);
		}
		else {
			b = (int)min;
			hue = hue / 60.0;
			g = (int)(hue * delta + min);
		}
	}
	else if(H > 60 && H < 180) {
		g = (int)max;
		if(H < 120) {
			b = (int)min;
			hue = (hue / 60.0 - 2.0 ) * delta;
			r = (int)(min - hue);
		}
		else {
			r = (int)min;
			hue = (hue / 60 - 2.0) * delta;
			b = (int)(min + hue);
		}
	}
	else {
		b = (int)max;
		if(H < 240) {
			r = (int)min;
			hue = (hue/60.0 - 4.0 ) * delta;
			g = (int)(min - hue);
		}
		else {
			g = (int)min;
			hue = (hue / 60 - 4.0) * delta;
			r = (int)(min + hue);
		}
	}
	return RGB(r, g, b);
}

HSV RGBToHSV(COLORREF rgb)
{
	int    r = GetRValue(rgb), g = GetGValue(rgb), b = GetBValue(rgb);
	double min = 0, max = 0, delta = 0, temp = 0;
	HSV    hsv;
	min = __min(r, __min(g, b));
	max = __max(r, __max(g, b));
	delta = max - min;

	hsv.V = (int)max;
	if(!delta)
		hsv.H = hsv.S = 0;
	else {
		temp = (max != 0) ? delta / max : 0;
		hsv.S = (int)(temp * 255);
		if(r == (int)max)
			temp = (double)(g - b)/delta;
		else if(g == (int)max)
			temp = 2.0 + ((double)(b - r)/delta);
		else
			temp = 4.0 + ((double)(r - g)/delta);
		temp *= 60;
		if(temp < 0)
			temp += 360;
		if(temp == 360)
			temp = 0;
		hsv.H = (int)temp;
	}
	return hsv;

}

POINT PtFromAngle(double angle, double sat, POINT center)
{
	POINT pt;
	angle = degtorad(angle);
	sat = TOSCALE(sat);
	double x = sat * cos(angle);
	double y = sat * sin(angle);
	pt.x = (long)x;
	pt.y = (long)y * -1;
	pt.x += center.x;
	pt.y += center.y;
	return pt;
}

double AngleFromPoint(POINT pt, POINT center)
{
	double y = -1.0 * (pt.y - center.y);
	double x = pt.x - center.x;
	return (x == 0.0 && y == 0.0) ? 0.0 : atan2(y, x);
}

POINT PointOnLine(POINT pt1, POINT pt2, int len, int maxlen)
{
	POINT ret;
	if(pt1.x != pt2.x) {
		double x = 0, k = (double)CALC_LINE_K(pt1, pt2), y = 0, b = CALC_LINE_B(pt1, k), D = 0, A = 0, C = 0, B = 0;
		double x1 = (double)pt1.x, y1 = (double)pt1.y;

		A = k * k + 1;
		B = 2 * k * b - 2 * k * y1 - 2 * x1;
		C = b * b - 2 * b * y1 + y1 * y1 + x1 * x1 - len * len;
		D = round(B * B - 4 * A * C, 4);
		x = (-B + sqrt(D)) / (2 * A);
		y = k * x + b;
		ret.x = (long)x;
		ret.y = (long)y;
		if(DISTANCE(ret, pt1) > maxlen || DISTANCE(ret, pt2) > maxlen) {
			x = (-B - sqrt(D)) / (2 * A);
			y = k * x + b;
			ret.x = (long)x;
			ret.y = (long)y;
		}
	}
	else {
		ret.x = pt1.x;
		ret.y = pt1.y - len;
	}
	return ret;
}

#define GRP_REDSPIN   1L
#define GRP_GREENSPIN 2L
#define GRP_BLUESPIN  3L
#define GRP_HUESPIN   4L
#define GRP_SATSPIN   5L
#define GRP_VALSPIN   6L

#define BRIGHT_MARKER_HEIGHT  8

#define RGB_CENTER_X 	    102
#define RGB_CENTER_Y        109
#define RGB_MAX_RED_X       102
#define RGB_MAX_RED_Y       8
#define RGB_MAX_GREEN_X     22
#define RGB_MAX_GREEN_Y     149
#define RGB_MAX_BLUE_X      181
#define RGB_MAX_BLUE_Y      148

class PPColorPickerDialog : public TDialog {
public:
	PPColorPickerDialog();

	int setDTS(const long * pColor);
	int getDTS(long * pColor);

	void  Paint();
private:
	DECL_HANDLE_EVENT;

	void  CalcRects();
	void  SetHSVCtrls(HSV c);
	void  SetRGBCtrls(COLORREF c);
	void  DrawColorRect(COLORREF c, uint ctl);
	void  DrawBrightRect();
	void  DrawRGB();
	POINT DrawRGBMarker(HDC hdc, HBRUSH brush, POINT ptMax, int color);
	RECT NormalizeRect(RECT rect);

	void   GetBrightRect(RECT * pRect, RECT * pModifRect);
	double GetCircleDistance(POINT pt, POINT * pPt);
	void   CirclePosByMousePos(POINT pt);
	void   BrightPosByMousePos(POINT pt);
	void   RGBPosByMousePos(POINT pt);
	int    InCircle(POINT pt);
	int    InRGB(POINT pt);
	int    InBright(POINT pt);

	int BeginCircleMove;
	int BeginRGBMove;     // 1 - move Red marker, 2 - move Green marker, 3 - move Blue marker
	int BeginBrightMove;
	COLORREF OldColor;
	COLORREF Data;
	HSV HsvColor;
	POINT CircleCenter;
	POINT RGBCenter;
	POINT RMax;
	POINT GMax;
	POINT BMax;
	RECT  CircleMark;
	int  BrightRectWidth;
	int  BrightRectHeight;
	int  BrightRectTop;
	int  BrightMark;
};

struct BrightStruc {
	WNDPROC PrevWindowProc;
	PPColorPickerDialog * P_Dlg;
};

static BOOL CALLBACK BrightViewProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	BrightStruc * p_struc = (BrightStruc *)TView::GetWindowUserData(hWnd);
	WNDPROC prev_proc = (p_struc) ? p_struc->PrevWindowProc : 0;
	switch(uMsg) {
		case WM_DESTROY:
			if(p_struc && p_struc->PrevWindowProc) {
				TView::SetWindowProp(hWnd, GWLP_WNDPROC, p_struc->PrevWindowProc);
			}
			ZDELETE(p_struc);
			break;
		case WM_PAINT:
				if(p_struc && p_struc->P_Dlg) {
					PAINTSTRUCT ps;
					BeginPaint(hWnd, (LPPAINTSTRUCT)&ps);
					p_struc->P_Dlg->Paint();
					EndPaint(hWnd, (LPPAINTSTRUCT)&ps);
					return 0;
				}
			break;
		case WM_SETFOCUS:
		case WM_KILLFOCUS:
			DialogProc(GetParent(hWnd), uMsg, wParam, (long)hWnd);
			break;
	}
	return (prev_proc) ? CallWindowProc(prev_proc, hWnd, uMsg, wParam, lParam) : 0;
}

PPColorPickerDialog::PPColorPickerDialog() : TDialog(DLG_COLORS)
{
	RECT r, hsb_r;
	BeginCircleMove = BeginRGBMove = BeginBrightMove = 0;
	addGroup(GRP_REDSPIN,   new SpinCtrlGroup(CTL_COLORS_RED,   cmUp, CTL_COLORS_RUP, cmDown, CTL_COLORS_RDOWN, 0, 255));
	addGroup(GRP_GREENSPIN, new SpinCtrlGroup(CTL_COLORS_GREEN, cmUp, CTL_COLORS_GUP, cmDown, CTL_COLORS_GDOWN, 0, 255));
	addGroup(GRP_BLUESPIN,  new SpinCtrlGroup(CTL_COLORS_BLUE,  cmUp, CTL_COLORS_BUP, cmDown, CTL_COLORS_BDOWN, 0, 255));
	addGroup(GRP_HUESPIN,   new SpinCtrlGroup(CTL_COLORS_HUE,   cmUp, CTL_COLORS_HUP, cmDown, CTL_COLORS_HDOWN, 0, 359));
	addGroup(GRP_SATSPIN,   new SpinCtrlGroup(CTL_COLORS_SAT,   cmUp, CTL_COLORS_SUP, cmDown, CTL_COLORS_SDOWN, 0, 255));
	addGroup(GRP_VALSPIN,   new SpinCtrlGroup(CTL_COLORS_VAL,   cmUp, CTL_COLORS_VUP, cmDown, CTL_COLORS_VDOWN, 0, 255));

	SetCtrlBitmap(CTL_COLORS_HSBRECT, BM_HSB);
	SetCtrlBitmap(CTL_COLORS_RGBRECT, BM_RGB);
	GetWindowRect(GetDlgItem(H(), CTL_COLORS_HSBRECT), &hsb_r);
	CircleCenter.y = CircleCenter.x = (hsb_r.right - hsb_r.left) / 2;
	GetWindowRect(GetDlgItem(H(), CTL_COLORS_BRIGHTRECT), &r);
	BrightRectTop    = hsb_r.top - r.top;
	BrightRectWidth  = r.right - r.left;
	BrightRectHeight = r.bottom - hsb_r.top - 2 - BRIGHT_MARKER_HEIGHT / 2;
	RMax.x = RGB_MAX_RED_X;
	RMax.y = RGB_MAX_RED_Y;
	GMax.x = RGB_MAX_GREEN_X;
	GMax.y = RGB_MAX_GREEN_Y;
	BMax.x = RGB_MAX_BLUE_X;
	BMax.y = RGB_MAX_BLUE_Y;
	RGBCenter.x = RGB_CENTER_X;
	RGBCenter.y = RGB_CENTER_Y;
	{
		BrightStruc * p_s = new BrightStruc;
		//p_s->PrevWindowProc = (WNDPROC)SetWindowLong(GetDlgItem(hWnd, CTL_COLORS_BRIGHTRECT), GWLP_WNDPROC, (long)BrightViewProc);
		p_s->PrevWindowProc = (WNDPROC)TView::SetWindowProp(::GetDlgItem(H(), CTL_COLORS_BRIGHTRECT), GWLP_WNDPROC, BrightViewProc);
		p_s->P_Dlg = this;
		TView::SetWindowUserData(::GetDlgItem(H(), CTL_COLORS_BRIGHTRECT), p_s);
	}
}

void PPColorPickerDialog::SetHSVCtrls(HSV c)
{
	HsvColor = c;
	setGroupData(GRP_HUESPIN, &HsvColor.H);
	setGroupData(GRP_SATSPIN, &HsvColor.S);
	setGroupData(GRP_VALSPIN, &HsvColor.V);
}

void PPColorPickerDialog::SetRGBCtrls(COLORREF c)
{
	Data = c;
	long r = GetRValue(Data);
	long g = GetGValue(Data);
	long b = GetBValue(Data);
	setGroupData(GRP_REDSPIN,   &r);
	setGroupData(GRP_GREENSPIN, &g);
	setGroupData(GRP_BLUESPIN,  &b);
}

int PPColorPickerDialog::setDTS(const long * pColor)
{
	SString path;
	Data = pColor ? ((COLORREF)*pColor) : GetColorRef(SClrWhite);
	OldColor = Data;
	SetRGBCtrls(Data);
	SetHSVCtrls(RGBToHSV(Data));
	return 1;
}

int PPColorPickerDialog::getDTS(long * pColor)
{
	long r = getCtrlLong(CTL_COLORS_RED);
	long g = getCtrlLong(CTL_COLORS_GREEN);
	long b = getCtrlLong(CTL_COLORS_BLUE);
	Data = RGB(r, g, b);
	ASSIGN_PTR(pColor, (long)Data);
	return 1;
}

IMPL_HANDLE_EVENT(PPColorPickerDialog)
{
	int test = 0;
	TDialog::handleEvent(event);
	if(event.what == TEvent::evMouseDown) {
		uint ctl = 0;
		RECT r;
		POINT pt;
		pt.x = event.mouse.WhereX;
		pt.y = event.mouse.WhereY;
		if((BeginCircleMove = InCircle(pt)))
			GetWindowRect(::GetDlgItem(H(), CTL_COLORS_HSBRECT), &r);
		else if((BeginRGBMove = InRGB(pt)))
			GetWindowRect(::GetDlgItem(H(), CTL_COLORS_RGBRECT), &r);
		else if((BeginBrightMove = InBright(pt)))
			GetBrightRect(&r, 0);
		if(BeginCircleMove || BeginRGBMove || BeginBrightMove)
			ClipCursor(&r);
	}
	else if(event.what == TEvent::evMouseUp) {
		POINT pt;
		ClipCursor(0);
		pt.x = event.mouse.WhereX;
		pt.y = event.mouse.WhereY;
		if(BeginCircleMove)
			CirclePosByMousePos(pt);
		else if(BeginRGBMove)
			RGBPosByMousePos(pt);
		else if(BeginBrightMove)
			BrightPosByMousePos(pt);
		if(BeginCircleMove || BeginRGBMove || BeginBrightMove)
			Paint();
		BeginCircleMove = BeginRGBMove = BeginBrightMove = 0;
	}
	else if(event.what == TEvent::evMouseMove) {
		POINT pt;
		pt.x = event.mouse.WhereX;
		pt.y = event.mouse.WhereY;
		if(BeginCircleMove)
			CirclePosByMousePos(pt);
		else if(BeginRGBMove)
			RGBPosByMousePos(pt);
		else if(BeginBrightMove)
			BrightPosByMousePos(pt);
		if(BeginCircleMove || BeginRGBMove || BeginBrightMove)
			Paint();
	}
	else if(TVBROADCAST) {
		if(TVCMD == cmReceivedFocus)
			Paint();
		else
			return;
	}
	else if(TVCOMMAND) {
		if((TVCMD == cmInputUpdatedByBtn || oneof2(TVCMD, cmUp, cmDown))) {
			const uint ctl_id = TVINFOVIEW->GetId();
			if(oneof9(ctl_id, CTL_COLORS_RUP, CTL_COLORS_GUP, CTL_COLORS_BUP, CTL_COLORS_RDOWN, CTL_COLORS_GDOWN, 
				CTL_COLORS_BDOWN, CTL_COLORS_RED, CTL_COLORS_GREEN, CTL_COLORS_BLUE)) {
				long r = getCtrlLong(CTL_COLORS_RED);
				long g = getCtrlLong(CTL_COLORS_GREEN);
				long b = getCtrlLong(CTL_COLORS_BLUE);
				Data = RGB(r, g, b);
				SetHSVCtrls(RGBToHSV(Data));
				Paint();
			}
			else if(oneof9(ctl_id, CTL_COLORS_HUP, CTL_COLORS_SUP, CTL_COLORS_VUP, CTL_COLORS_HDOWN, CTL_COLORS_SDOWN, 
				CTL_COLORS_VDOWN, CTL_COLORS_HUE, CTL_COLORS_SAT, CTL_COLORS_VAL)) {
				HsvColor.H = getCtrlLong(CTL_COLORS_HUE);
				HsvColor.S = getCtrlLong(CTL_COLORS_SAT);
				HsvColor.V = getCtrlLong(CTL_COLORS_VAL);
				Data = HsvColor.ToRGB();
				SetRGBCtrls(Data);
				Paint();
			}
		}
		else
			return;
	}
	else
		return;
	clearEvent(event);
}

void PPColorPickerDialog::Paint()
{
	HWND   hwnd = ::GetDlgItem(H(), CTL_COLORS_HSBRECT);
	HDC    dlg_hdc = GetDC(H());
	HDC    hdc = GetDC(hwnd);
	HBRUSH brush = 0;
	RECT   rect;
	CalcRects();
	GetClientRect(hwnd, &rect);
	InvalidateRect(hwnd, &rect, true);
	UpdateWindow(hwnd);
	brush = CreateSolidBrush(GetColorRef(SClrBlack));
	FrameRect(hdc, &CircleMark, brush);
	ZDeleteWinGdiObject(&brush);
	DrawColorRect(OldColor, CTL_COLORS_OLDCOLOR);
	DrawColorRect(Data,     CTL_COLORS_NEWCOLOR);
	DrawBrightRect();
	DrawRGB();
	ReleaseDC(hwnd, hdc);
	::ReleaseDC(H(), dlg_hdc);
}

void PPColorPickerDialog::CalcRects()
{
	POINT pt;
	pt = PtFromAngle(HsvColor.H, HsvColor.S, CircleCenter);
	CircleMark.left   = pt.x - RECT_WIDTH;
	CircleMark.top    = pt.y - RECT_WIDTH;
	CircleMark.right  = pt.x + RECT_WIDTH;
	CircleMark.bottom = pt.y + RECT_WIDTH;
}

void PPColorPickerDialog::DrawColorRect(COLORREF c, uint ctl)
{
	RECT   rect;
	HWND   hwnd = ::GetDlgItem(H(), ctl);
	HDC    hdc = ::GetDC(hwnd);
	HPEN   pen = 0;
	HPEN   old_pen = 0;
	HBRUSH brush = 0;
	HBRUSH old_brush = 0;
	::GetClientRect(hwnd, &rect);
	rect.left   += 2;
	rect.right  -= 2;
	rect.top    += 16; // @v9.1.11 8-->16
	rect.bottom -= 2;
	brush = CreateSolidBrush(c);
	FillRect(hdc, &rect, brush);
	ZDeleteWinGdiObject(&brush);
	ReleaseDC(hwnd, hdc);
}

RECT PPColorPickerDialog::NormalizeRect(RECT rect)
{
	RECT dlg_rect;
	GetWindowRect(H(), &dlg_rect);
	rect.left   -= (dlg_rect.left + GetSystemMetrics(SM_CXBORDER) * 2);
	rect.top    -= (dlg_rect.top + GetSystemMetrics(SM_CYCAPTION) + GetSystemMetrics(SM_CYBORDER) * 2);
	rect.right  -= (dlg_rect.left + GetSystemMetrics(SM_CXBORDER) * 2);
	rect.bottom -= (dlg_rect.top + GetSystemMetrics(SM_CYCAPTION) + GetSystemMetrics(SM_CYBORDER) * 2);
	return rect;
}

double PPColorPickerDialog::GetCircleDistance(POINT pt, POINT * pPt)
{
	double d = 0.0;
	RECT r;
	::GetWindowRect(::GetDlgItem(H(), CTL_COLORS_HSBRECT), &r);
	r = NormalizeRect(r);
	pt.x -= r.left;
	pt.y -= r.top;
	d = DISTANCE(pt, CircleCenter);
	ASSIGN_PTR(pPt, pt);
	return d;
}

void PPColorPickerDialog::GetBrightRect(RECT * pRect, RECT * pModifRect)
{
	RECT r;
	::GetWindowRect(::GetDlgItem(H(), CTL_COLORS_BRIGHTRECT), &r);
	r.top += BrightRectTop + 1;
	r.bottom = r.top + BrightRectHeight + 1;
	ASSIGN_PTR(pRect, r);
	if(pModifRect) {
		r = NormalizeRect(r);
		r.top--;
		r.bottom--;
		ASSIGN_PTR(pModifRect, r);
	}
}

int PPColorPickerDialog::InCircle(POINT pt)
{
	return BIN(GetCircleDistance(pt, 0) <= RADIUS);
}

int PPColorPickerDialog::InRGB(POINT pt)
{
	int ret = 0;
	RECT r;
	::GetWindowRect(::GetDlgItem(H(), CTL_COLORS_RGBRECT), &r);
	r = NormalizeRect(r);
	r.left   += 5;
	r.top    += 5;
	r.right  -= 5;
	r.bottom -= 5;
	if(pt.x >= r.left && pt.x <= r.right && pt.y >= r.top && pt.y <= r.bottom) {
		int maxlen = 0;
		POINT pt_onl;
		pt.x -= (r.left - 5);
		pt.y -= (r.top  - 5);
#define IN_COLOR_RECT(max_pt, len, ret_val) maxlen = (int)DISTANCE(RGBCenter, max_pt); \
			pt_onl = PointOnLine(RGBCenter, max_pt, (len * maxlen) / 255, maxlen); \
			if(pt.x >= pt_onl.x - RECT_WIDTH && pt.x <= pt_onl.x + RECT_WIDTH && \
				pt.y >= pt_onl.y - RECT_WIDTH && pt.y <= pt_onl.y + RECT_WIDTH) \
				return ret_val;
			IN_COLOR_RECT(RMax, GetRValue(Data), 1);
			IN_COLOR_RECT(GMax, GetGValue(Data), 2);
			IN_COLOR_RECT(BMax, GetBValue(Data), 3);
#undef IN_COLOR_RECT
	}
	return ret;
}

int PPColorPickerDialog::InBright(POINT pt)
{
	RECT r;
	GetBrightRect(0, &r);
	return (pt.x >= r.left && pt.x <= r.right && pt.y >= r.top && pt.y <= r.bottom) ? 1 : 0;
}

void PPColorPickerDialog::CirclePosByMousePos(POINT pt)
{
	double d = GetCircleDistance(pt, &pt);
	HsvColor.H = (int)radtodeg(AngleFromPoint(pt, CircleCenter));
	HsvColor.H = (HsvColor.H < 0)   ? HsvColor.H + 360 : HsvColor.H;
	HsvColor.H = (HsvColor.H > 359) ? 359 : HsvColor.H;
	HsvColor.S = (int)SCALETOMAX(d);
	HsvColor.S = (HsvColor.S > 255) ? 255 : HsvColor.S;
	HsvColor.S = (HsvColor.S < 0)   ? 0 : HsvColor.S;
	SetHSVCtrls(HsvColor);
	SetRGBCtrls(HsvColor.ToRGB());
}

void PPColorPickerDialog::BrightPosByMousePos(POINT pt)
{
	RECT r;
	GetBrightRect(0, &r);
	pt.y -= r.top;
	HsvColor.V = 255 - ((BrightRectHeight) ? (pt.y * 255) / BrightRectHeight : 0);
	HsvColor.V = (HsvColor.V > 255) ? 255 : HsvColor.V;
	HsvColor.V = (HsvColor.V < 0)   ? 0   : HsvColor.V;
	SetHSVCtrls(HsvColor);
	SetRGBCtrls(HsvColor.ToRGB());
}

void PPColorPickerDialog::RGBPosByMousePos(POINT pt)
{
	int    red = GetRValue(Data);
	int    green = GetGValue(Data);
	int    blue = GetBValue(Data);
	RECT   rect;
	::GetWindowRect(::GetDlgItem(H(), CTL_COLORS_RGBRECT), &rect);
	rect = NormalizeRect(rect);
	pt.x -= rect.left;
	pt.y -= rect.top;
	if(BeginRGBMove == 1) {
		pt.x    = RGB_CENTER_X;
		pt.y = (pt.y > RGB_CENTER_Y)  ? RGB_CENTER_Y  : pt.y;
		pt.y = (pt.y < RGB_MAX_RED_Y) ? RGB_MAX_RED_Y : pt.y;
		red = (int)((DISTANCE(pt, RGBCenter) * 255)  / DISTANCE(RMax, RGBCenter));
	}
	else if(BeginRGBMove == 2) {
		double k = CALC_LINE_K(RGBCenter, GMax), b = CALC_LINE_B(RGBCenter, k);
		pt.y = (pt.y < RGB_CENTER_Y)    ? RGB_CENTER_Y    : pt.y;
		pt.y = (pt.y > RGB_MAX_GREEN_Y) ? RGB_MAX_GREEN_Y : pt.y;
		pt.x = (k != 0) ? (int)(((double)pt.y - b) / k) : 0;
		green = (int)((DISTANCE(pt, RGBCenter) * 255)  / DISTANCE(GMax, RGBCenter));
	}
	else if(BeginRGBMove == 3) {
		double k = CALC_LINE_K(RGBCenter, BMax), b = CALC_LINE_B(RGBCenter, k);
		pt.x = (pt.x < RGB_CENTER_X)   ? RGB_CENTER_X     : pt.x;
		pt.x = (pt.x > RGB_MAX_BLUE_X) ? RGB_MAX_BLUE_X   : pt.x;
		pt.y = (long)(k * (double)pt.x + b);
		blue = (int)((DISTANCE(pt, RGBCenter) * 255)  / DISTANCE(BMax, RGBCenter));
	}
	Data = RGB(red, green, blue);
	SetRGBCtrls(Data);
	SetHSVCtrls(RGBToHSV(Data));
}

POINT PPColorPickerDialog::DrawRGBMarker(HDC hdc, HBRUSH brush, POINT ptMax, int color)
{
	int    maxlen = (int)DISTANCE(RGBCenter, ptMax);
	int    len = (color * maxlen) / 255;
	POINT  pt = PointOnLine(RGBCenter, ptMax, len, maxlen);
	RECT   rect;
	rect.top    = pt.y - RECT_WIDTH;
	rect.bottom = pt.y + RECT_WIDTH;
	rect.left   = pt.x - RECT_WIDTH;
	rect.right  = pt.x + RECT_WIDTH;
	FrameRect(hdc, &rect, brush);
	return pt;
}

void PPColorPickerDialog::DrawRGB()
{
	int    r_len = GetRValue(Data), g_len = GetGValue(Data), b_len = GetBValue(Data), len = 0;
	HWND   hwnd = ::GetDlgItem(H(), CTL_COLORS_RGBRECT);
	HDC    hdc = ::GetDC(hwnd);
	HPEN   pen = ::CreatePen(PS_SOLID, 1, GetColorRef(SClrWhite));
	HPEN   old_pen = 0;
	HBRUSH brush = ::CreateSolidBrush(GetColorRef(SClrWhite));
	POINT pt_r, pt_g, pt_b, pt, pt_rb, pt_rg, pt_gb;
	RECT rect;

	GetClientRect(hwnd, &rect);
	InvalidateRect(hwnd, &rect, true);
	UpdateWindow(hwnd);

	pt_r = DrawRGBMarker(hdc, brush, RMax, r_len);
	pt_g = DrawRGBMarker(hdc, brush, GMax, g_len);
    pt_b = DrawRGBMarker(hdc, brush, BMax, b_len);
	old_pen = (HPEN)SelectObject(hdc, pen);
#define DRAW_RGB_LINE(pt1, pt2, ptc) pt.x = pt1.x + (pt2.x - ptc.x); pt.y = pt1.y + (pt2.y - ptc.y); \
								MoveToEx(hdc, pt1.x, pt1.y, 0);	LineTo(hdc, pt.x, pt.y); LineTo(hdc, pt2.x, pt2.y);

	DRAW_RGB_LINE(pt_b, pt_g, RGBCenter);
	pt_gb = pt;
	DRAW_RGB_LINE(pt_r, pt_b, RGBCenter);
	pt_rb = pt;
	DRAW_RGB_LINE(pt_r, pt_g, RGBCenter);
	pt_rg = pt;
	DRAW_RGB_LINE(pt_rb, pt_rg, pt_r);
	MoveToEx(hdc, pt_gb.x, pt_gb.y, 0);
	LineTo(hdc, pt.x, pt.y);
#undef DRAW_RGB_LINE
	if(old_pen)
		SelectObject(hdc, old_pen);
	ZDeleteWinGdiObject(&brush);
	ZDeleteWinGdiObject(&pen);
	ReleaseDC(hwnd, hdc);
}

void PPColorPickerDialog::DrawBrightRect()
{
	double d = 0.0;
	RECT   r;
	HWND   hwnd = ::GetDlgItem(H(), CTL_COLORS_BRIGHTRECT);
	HDC    hdc = ::GetDC(hwnd);
	BYTE   palette[768], * p = 0;
	HSV    hsv = HsvColor;

	r.left   = 2;
	r.top    = BrightRectTop + BrightMark;
	r.bottom = r.top + BRIGHT_MARKER_HEIGHT;
	r.right  = BrightRectWidth - 2;
    FrameRect(hdc, &r, GetSysColorBrush(COLOR_SCROLLBAR));

	d = (BrightRectHeight != 0) ? (255.0 / BrightRectHeight) : 0;
	p = palette;
	for(int i = BrightRectHeight; i >= 0 ; i--, p += 3) {
		COLORREF rgb;
		hsv.V = (int)((double)i * d);
		rgb = hsv.ToRGB();
		p[0] = GetRValue(rgb);
		p[1] = GetGValue(rgb);
		p[2] = GetBValue(rgb);
	}
	for(int i = 0, j = 0; i < BrightRectHeight; i++, j += 3) {
		HBRUSH brush = CreateSolidBrush(RGB(palette[j], palette[j + 1], palette[j + 2]));
		r.top    = BrightRectTop + i;
		r.left   = 4;
		r.right  = BrightRectWidth - 4;
		r.bottom = r.top + 1;
		FillRect(hdc, &r, brush);
		ZDeleteWinGdiObject(&brush);
	}
	{
		HBRUSH brush = CreateSolidBrush(GetColorRef(SClrBlack));
		BrightMark = BrightRectHeight - (int)(HsvColor.V * BrightRectHeight) / 255 - BRIGHT_MARKER_HEIGHT / 2;
		r.left   = 2;
		r.top    = BrightRectTop + BrightMark;
		r.bottom = r.top + BRIGHT_MARKER_HEIGHT;
		r.right  = BrightRectWidth - 2;
        FrameRect(hdc, &r, brush);
		ZDeleteWinGdiObject(&brush);
	}
	ReleaseDC(hwnd, hdc);
}

int EditColor(long * pColor)
{
	int    ok = -1;
	PPColorPickerDialog * p_dlg = 0;
	THROW_MEM(p_dlg = new PPColorPickerDialog);
	THROW(CheckDialogPtr(&p_dlg));
	p_dlg->setDTS(pColor);
	if(ExecView(p_dlg) == cmOK) {
		p_dlg->getDTS(pColor);
		ok = 1;
	}
	CATCHZOK
	delete p_dlg;
	return ok;
}

#define COLOR_SELLIST   0x00ff6633L
#define COLOR_FRAMELIST 0x002D9DFFL

//static
const COLORREF ColorCtrlGroup::Rec::UndefC = 0x00000000U;

ColorCtrlGroup::Rec::Rec()
{
	C = UndefC;
}

int ColorCtrlGroup::Rec::AddColorItem(SColor c, const char * pName)
{
	return AddColorItem((COLORREF)c, pName);
}

int ColorCtrlGroup::Rec::AddColorItem(COLORREF rgb, const char * pName)
{
	return ColorList.Add((long)rgb, NZOR(pName, ""), 1);
}

int ColorCtrlGroup::Rec::AddUndefColorItem()
{
	int    ok = 1;
	SString userdef;
	PPLoadText(PPTXT_USERDEF, userdef);
	uint   cpos = 0;
	if(ColorList.Search((long)UndefC, &cpos)) {
		if(cpos == (ColorList.getCount()-1))
			ok = -1;
		else {
			ColorList.atFree(cpos);
			AddColorItem(UndefC, userdef);
		}
	}
	else
		AddColorItem(UndefC, userdef);
	return 1;
}

void ColorCtrlGroup::Rec::SetupStdColorList()
{
	ColorList.Clear();
	AddColorItem(SClrRed,      0);
	AddColorItem(SClrOrange,   0);
	AddColorItem(0x00ccff,     0); // Насыщенный желтый
	AddColorItem(SClrOlive,    0);
	AddColorItem(SClrBlue,     0);
	AddColorItem(SClrSeagreen, 0);
	AddColorItem(0x996633,     0);
	AddColorItem(SClrCoral,    0);
	AddColorItem(PPDesktop::GetDefaultBgColor(), 0);
}

uint ColorCtrlGroup::Rec::GetColorItemsCount() const
{
	return ColorList.getCount();
}

int ColorCtrlGroup::Rec::GetColorItem(uint pos, COLORREF & rC, SString & rNameBuf) const
{
	int    ok = 0;
	if(pos < ColorList.getCount()) {
		StrAssocArray::Item item = ColorList.at_WithoutParent(pos);
		rC = item.Id;
		rNameBuf = item.Txt;
		ok = (rC == UndefC) ? -1 : 1;
	}
	else {
		rC = UndefC;
		rNameBuf = 0;
		ok = 0;
	}
	return ok;
}

int ColorCtrlGroup::Rec::SearchColorItem(COLORREF c, uint * pPos) const
{
	return ColorList.Search((long)c, pPos);
}

ColorCtrlGroup::ColorCtrlGroup(uint ctl, uint ctlSel, uint cmNewColor, uint ctlNewColor) : CtrlGroup()
{
	Ctl            = ctl;
	CtlSel         = ctlSel;
	CmNewColor     = cmNewColor;
	CtlNewColor    = ctlNewColor;
	H_BrBkList     = CreateSolidBrush(GetColorRef(SClrWhite));
	H_BrSelList    = CreateSolidBrush((COLORREF)COLOR_SELLIST);
	H_BrFrameList  = CreateSolidBrush((COLORREF)COLOR_FRAMELIST);
	H_BrBkInput    = 0;
}

ColorCtrlGroup::~ColorCtrlGroup()
{
	ZDeleteWinGdiObject(&H_BrBkList);
	ZDeleteWinGdiObject(&H_BrSelList);
	ZDeleteWinGdiObject(&H_BrFrameList);
	ZDeleteWinGdiObject(&H_BrBkInput);
	for(int i = 0; i < 32; i++)
		MEMSZERO(OwnerDrawCtrls[i]);
}

int ColorCtrlGroup::setData(TDialog * pDlg, void * pData)
{
	for(int i = 0; i < 32; i++)
		if(OwnerDrawCtrls[i].CtrlType == 0 && OwnerDrawCtrls[i].CtrlID == 0) {
			OwnerDrawCtrls[i].CtrlType = ctListBox;
			OwnerDrawCtrls[i].CtrlID   = CTL_OWNDRAWLBX_LIST;
			OwnerDrawCtrls[i].ExtraParam = 22;
			break;
		}
	Data = *(ColorCtrlGroup::Rec*)pData;
	{
		SString userdef;
		PPLoadText(PPTXT_USERDEF, userdef);
		if(!Data.SearchColorItem(Data.C, 0)) {
			Data.AddColorItem(Data.C, userdef);
		}
		else
			Data.AddColorItem(GetColorRef(SClrWhite), userdef);
	}
	SetupStrAssocCombo(pDlg, CtlSel, &Data.ColorList, Data.C, 0, 0, 1);
	//SetupTaggedStringCombo(pDlg, CtlSel, &Data.ColorList, Data.ColorId, 0, 0, 1);
	return 1;
}

int ColorCtrlGroup::getData(TDialog * pDlg, void * pData)
{
	pDlg->getCtrlData(CtlSel, &Data.C);
	ASSIGN_PTR((ColorCtrlGroup::Rec*)pData, Data);
	return 1;
}

// virtual
void ColorCtrlGroup::handleEvent(TDialog * pDlg, TEvent & event)
{
	if(TVCOMMAND) {
		if(TVCMD == CmNewColor && event.isCtlEvent(CtlNewColor)) {
			const uint cc = Data.ColorList.getCount();
			if(cc) {
				long c = Data.ColorList.at(cc-1).Id;
				if(EditColor(&c) > 0) {
					uint   cpos = 0;
					if(!Data.SearchColorItem(c, &cpos)) {
						Data.ColorList.UpdateByPos(cc-1, c);
						Data.AddUndefColorItem();
						SetupStrAssocCombo(pDlg, CtlSel, &Data.ColorList, c, 0, 0, 1);
					}
					pDlg->setCtrlData(CtlSel, &c);
				}
			}
		}
		else if(TVCMD == cmCtlColor) {
			TDrawCtrlData * p_dc = (TDrawCtrlData *)TVINFOPTR;
			if(p_dc) {
				TView * v = pDlg->getCtrlView(Ctl);
				HWND hwnd = v ? v->getHandle() : 0;
				if(hwnd == p_dc->H_Ctl) {
					long c = pDlg->getCtrlLong(CtlSel);
					if(c == Rec::UndefC)
						c = GetColorRef(SClrWhite);
					ZDeleteWinGdiObject(&H_BrBkInput);
					H_BrBkInput = CreateSolidBrush(c);
					p_dc->H_Br = H_BrBkInput;
					SetBkMode(p_dc->H_DC, TRANSPARENT);
					pDlg->clearEvent(event);
				}
			}
		}
		else if(TVCMD == cmDrawItem) {
			TDrawItemData * p_di = (TDrawItemData *)TVINFOPTR;
			if(p_di && p_di->P_View && p_di->CtlType == ODT_LISTBOX/*ctListBox*/) {
				ListWindowSmartListBox * p_lbx = (ListWindowSmartListBox *)p_di->P_View;
				if(p_lbx && p_lbx->combo->TestId(CtlSel)) {
					HDC      h_dc = p_di->H_DC;
					HBRUSH   old_brush   = 0, brush = 0;
					COLORREF old_color = 0;
					RECT   rc = p_di->ItemRect;
					if(p_di->ItemAction & TDrawItemData::iaBackground) {
						FillRect(h_dc, &rc, H_BrBkList);
						p_di->ItemAction = 0;
					}
					else if(p_di->ItemID != 0xffffffff && p_lbx && p_lbx->def && (long)p_di->ItemID < p_lbx->def->getRecsCount()) {
						long   item_color = 0;
						//char   temp_buf[256];
						//p_lbx->getText((long)p_di->ItemID, temp_buf, sizeof(temp_buf));
						//SOemToChar(temp_buf);
						SString temp_buf;
						p_lbx->getText((long)p_di->ItemID, temp_buf);
						temp_buf.Transf(CTRANSF_INNER_TO_OUTER);
						p_lbx->getID((long)p_di->ItemID, &item_color);
						COLORREF c = (item_color == Rec::UndefC) ? GetColorRef(SClrWhite) : (COLORREF)item_color;
						brush = CreateSolidBrush(c);
						if(p_di->ItemState & (ODS_FOCUS | ODS_SELECTED)) {
							old_color = SetBkColor(h_dc, RGB(0x33, 0x66, 0xFF));
							FillRect(h_dc, &rc, H_BrSelList);
						}
						else {
							old_color = SetBkColor(h_dc, GetColorRef(SClrWhite));
							FillRect(h_dc, &rc, H_BrBkList);
							FrameRect(h_dc, &rc, H_BrFrameList);
						}
						SetBkColor(h_dc, (COLORREF)c);
						old_brush = (HBRUSH)SelectObject(h_dc, brush);
						rc.top    += 2;
						rc.left   += 2;
						rc.right  -= 2;
						rc.bottom -= 2;
						FillRect(h_dc, &rc, brush);
						if(temp_buf.Len())
							::DrawText(h_dc, temp_buf.cptr(), temp_buf.Len(), &rc, DT_LEFT|DT_VCENTER|DT_SINGLELINE); // @unicodeproblem
					}
					if(old_brush)
						SelectObject(h_dc, old_brush);
					ZDeleteWinGdiObject(&brush);
					if(old_color)
						SetBkColor(h_dc, old_color);
					pDlg->clearEvent(event);
				}
			}
		}
	}
}
