#include "video.h"
#include "events.h"
#include "logging.h"
#include "support.h"

vWinSpecs window;
vSurface WinSurface;
extern HWND vDosHwnd;
HBITMAP DIBSection;

static LPSTR Appname = "vDos";

void InitWindow(void)
	{
	WNDCLASS wc;
	HINSTANCE hInst;

	hInst = GetModuleHandle(NULL);
	memset((void *)&wc, 0, sizeof(WNDCLASS));

	wc.hCursor			= LoadCursor(NULL, IDC_ARROW);								// Register the application class
	wc.lpszClassName	= Appname;
	wc.hInstance		= hInst;
	wc.lpfnWndProc		= WinMessage;
	wc.hbrBackground	= (HBRUSH)GetStockObject(BLACK_BRUSH);						// Black background, mostly for filling if "full-screen"
	if (RegisterClass(&wc))
		{
		vDosHwnd = CreateWindow(Appname, Appname,
			(WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX),
			CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, NULL, NULL, hInst, NULL);
		if (vDosHwnd)
			return;
		}
	E_Exit("Could't create vDos window");
	}

vSurface *SetVideoMode(int width, int height, bool framed)
{
	BITMAPINFO binfo;
	DWORD style;
	const DWORD directstyle = (WS_POPUP);
	const DWORD windowstyle = (WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX);
	HDC hdc;
	RECT bounds;
	// Fill in part of the video surface
	WinSurface.w = width;
	WinSurface.h = height;
	style = GetWindowLong(vDosHwnd, GWL_STYLE)&(~(windowstyle|directstyle));
	if (framed)
		style |= windowstyle;
	else
		style |= directstyle;
	SetWindowLong(vDosHwnd, GWL_STYLE, style);
	// Delete the old bitmap if needed
	if(DIBSection != NULL)
		DeleteObject(DIBSection);
	memzero(&(binfo.bmiHeader), sizeof(BITMAPINFOHEADER));
	binfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	binfo.bmiHeader.biWidth = width;
	binfo.bmiHeader.biHeight = -height;												// -ve for topdown bitmap
	binfo.bmiHeader.biPlanes = 1;
	binfo.bmiHeader.biSizeImage = height*width*4;
	binfo.bmiHeader.biBitCount = 32;

	// Create the offscreen bitmap buffer
	hdc = GetDC(vDosHwnd);
	DIBSection = CreateDIBSection(hdc, &binfo, DIB_RGB_COLORS, (void **)(&WinSurface.pixels), NULL, 0);
	ReleaseDC(vDosHwnd, hdc);
	if (DIBSection == NULL)
		return NULL;

	// Resize the window
	bounds.left = bounds.top = 0;
	bounds.right = width;
	bounds.bottom = height;
	AdjustWindowRectEx(&bounds, style, false, 0);
	width = bounds.right-bounds.left;
	height = bounds.bottom-bounds.top;
	HMONITOR monitor = MonitorFromWindow(vDosHwnd, MONITOR_DEFAULTTONEAREST);
	MONITORINFO info;
	info.cbSize = sizeof(MONITORINFO);
	GetMonitorInfo(monitor, &info);
	SetWindowPos(vDosHwnd, NULL,
		(info.rcMonitor.right+info.rcMonitor.left-width)/2, (info.rcMonitor.bottom+info.rcMonitor.top-height)/2,	// Always center
		width, height, SWP_NOCOPYBITS|SWP_SHOWWINDOW);
	return &WinSurface;
}

// Update a specific portion of the physical screen
void vUpdateRect(Bit32s x, Bit32s y, Bit32u w, Bit32u h)
{
	HDC hdc = GetDC(vDosHwnd);
	HDC mdc = CreateCompatibleDC(hdc);
	SelectObject(mdc, DIBSection);
	BitBlt(hdc, x, y, w, h, mdc, x, y, SRCCOPY);
	DeleteDC(mdc);
	ReleaseDC(vDosHwnd, hdc);
}

// Update screen
void vUpdateWin(void)
{
	vUpdateRect(0, 0, window.width, window.height);
}

//Update video buffer
void vBlitSurface(vSurface *src, RECT *srcrect, RECT *dstrect)
{
	Bit8u * srcpix = (Bit8u *)src->pixels+srcrect->top*src->w;
	Bit32u * dstpix = (Bit32u *)((Bit8u *)window.surface->pixels + dstrect->top * window.surface->w * 4 + dstrect->left * 4);
	int dstskip = window.surface->w-(srcrect->right-srcrect->left);
	for(int h = srcrect->bottom-srcrect->top; h; h--) {
		for(int w = srcrect->right-srcrect->left; w; --w)
			*dstpix++ =  *(Bit32u*)(&src->P_Colors[*srcpix++]);
		dstpix += dstskip;
	}
}

// Clean up the video subsystem
void VideoQuit (void)
{
	if (vDosHwnd) {
		if (DIBSection) {
			DeleteObject(DIBSection);
			DIBSection = NULL;
		}
		DestroyWindow(vDosHwnd);
		vDosHwnd = NULL;
	}
	return;
}

