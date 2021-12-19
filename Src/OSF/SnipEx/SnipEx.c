// SnipEx.c
// Author: Joseph Ryan Ries, 2017-2020
// The snip.exe that comes bundled with Microsoft Windows is *almost* good enough. So I made one just a little better.
//
// @sobolev: Правки @20201025 внесены в соответсвии с изменениями в оригинальном github-репозитории автора
//
#include <slib.h>
#ifndef UNICODE
	#define UNICODE                                                                 // 100% Unicode.
#endif
#ifndef _UNICODE
	#define _UNICODE
#endif

//#pragma warning(push, 0) // Temporarily disable warnings in header files over which I have no control.
#include <windowsx.h> // Windows API
#include <ShObjIdl.h> // For save dialog
#include <ShellScalingApi.h> // For detecting monitor DPI (Win 8.1 or above)
#include <initguid.h> // For doing stuff with GUIDs (Needed for COM interop)
//#pragma warning(pop) // Restore warnings.
#pragma comment(lib, "Msimg32.lib") // For TransparentBlt
#pragma comment(lib, "Shcore.lib")  // For detecting monitor DPI (Win 8.1 or above)
//#pragma warning(disable: 4820)      // Disable compiler warning about padding bytes being added to structs
//#pragma warning(disable: 4710)      // Disable compiler warning about functions not being inlined
//#pragma warning(disable: 5045)      // Disable Spectre mitigation informational warning
//#include <stdio.h>                  // For doing stuff with strings
//#include <math.h>                   // Needed to introduce some math to draw the arrow head for the arrow tool
#include "resource.h"               // Images, cursors, etc.
//#include "SnipEx.h"                 // My custom definitions
	#define CRASH(Expression) if (!(Expression)) { MessageBoxW(0, L"Something has failed that should never have failed and now the program will crash. If you want to help debug this, report this crash to ryanries09@gmail.com.", L"Fatal Error", MB_ICONERROR | MB_OK | MB_SYSTEMMODAL); *(int *)0 = 0; }
	#define REG_DROPSHADOWNAME	 L"DropShadow"
	#define REG_REMEMBERTOOLNAME L"RememberLastTool"
	#define REG_LASTTOOLNAME     L"LastTool"
	#define REG_AUTOCOPYNAME	 L"AutoCopy"

	// You could refer to an individual button like gButtons[BUTTON_NEW - 10001], gButtons[BUTTON_DELAY - 10001], etc.
	#define BUTTON_NEW     10001
	#define BUTTON_DELAY   10002
	#define BUTTON_SAVE    10003
	#define BUTTON_COPY    10004
	#define BUTTON_HILIGHT 10005
	#define BUTTON_BOX     10006
	#define BUTTON_ARROW   10007
	#define BUTTON_REDACT  10008
	#define BUTTON_TEXT	   10009

	#define COLOR_NONE		0	// For buttons for which color is not applicable, such as the Save button for example
	#define COLOR_RED		1
	#define COLOR_GREEN		2
	#define COLOR_BLUE		3
	#define COLOR_BLACK		4
	#define COLOR_WHITE		5
	#define COLOR_YELLOW	6
	#define COLOR_PINK		7
	#define COLOR_ORANGE	8

	#define SYSCMD_REPLACE  20001
	#define SYSCMD_RESTORE  20002
	#define SYSCMD_SHADOW   20003
	#define SYSCMD_UNDO     20004
	#define SYSCMD_REMEMBER 20005
	#define SYSCMD_AUTOCOPY 20006

	#define DELAY_TIMER    30001

	// This is like Microsoft's POINT struct, but smaller.
	// It's just for efficiency's sake since I don't need LONGs for this, yet...
	struct SMALLPOINT {
		UINT16 x;
		UINT16 y;
	};

	enum BUTTONSTATE {
		BUTTONSTATE_NORMAL,
		BUTTONSTATE_PRESSED
	};

	struct BUTTON {
		void   Set(UINT8 clr, int enabledIconId, int cursorId)
		{
			Color = clr;
			EnabledIconId = enabledIconId;
			CursorId = cursorId;
		}
		RECT   Rectangle;      // left, top, right, bottom
		const  wchar_t * Caption;
		HBITMAP EnabledIcon;
		HBITMAP DisabledIcon;
		int    EnabledIconId;
		int    DisabledIconId;
		BUTTONSTATE State;          // Is the button pressed or not
		UINT16 Hotkey;
		HWND   Handle;
		BOOL   Enabled;
		LONGLONG Id;
		BOOL   SelectedTool;   // If the button is selected as a tool it should stay pressed
		int    CursorId;
		HCURSOR Cursor;
		UINT8  Color;
	};

	enum APPSTATE {
		APPSTATE_BEFORECAPTURE,
		APPSTATE_DURINGCAPTURE,
		APPSTATE_DELAYCOOKING,
		APPSTATE_AFTERCAPTURE
	};

	#pragma region DECLARATIONS

	int CALLBACK WinMain(_In_ HINSTANCE Instance, _In_opt_ HINSTANCE PreviousInstance, _In_ LPSTR CommandLine, _In_ int WindowShowCode);
	LRESULT CALLBACK MainWindowCallback(_In_ HWND Window, _In_ UINT Message, _In_ WPARAM WParam, _In_ LPARAM LParam);
	void DrawButton(_In_ DRAWITEMSTRUCT* DrawItemStruct, _In_ BUTTON Button);
	LRESULT CALLBACK CaptureWindowCallback(_In_ HWND Window, _In_ UINT Message, _In_ WPARAM WParam, _In_ LPARAM LParam);
	BOOL CALLBACK TextEditCallback(_In_ HWND Dialog, _In_ UINT Message, _In_ WPARAM WParam, _In_ LPARAM LParam);
	//void CaptureWindow_OnLeftButtonUp(void);
	// Returns TRUE if we were successful in creating the capture window. FALSE if it fails.
	BOOL NewButton_Click(void);
	// Returns TRUE if the snip was saved. Returns FALSE if there was an error or if user cancelled.
	BOOL SaveButton_Click(void);
	// Returns TRUE if the snip was copied to the clipboard. FALSE if it fails.
	BOOL CopyButton_Click(void);
	//BOOL TextButton_Click(void);
	// Save a 32-bit bitmap to a file. Returns FALSE if it fails.
	BOOL SaveBitmapToFile(_In_ wchar_t* FilePath);
	// Save png image to a file. Returns FALSE if it fails.
	BOOL SavePngToFile(_In_ wchar_t* FilePath);
	//HRESULT AddAllMenuItems(_In_ HINSTANCE Instance);
	BOOL IsAppRunningElevated(void);
	// OutputDebugStringW enhanced with varargs. Only works in debug builds.
	void MyOutputDebugStringW(_In_ wchar_t* Message, _In_ ...);
	BOOL AdjustForCustomScaling(void);
	LSTATUS SetSnipExRegValue(_In_ wchar_t* ValueName, _In_ DWORD* ValueData);
	LSTATUS GetSnipExRegValue(_In_ wchar_t* ValueName, _In_ DWORD* ValueData);
	LSTATUS DeleteSnipExRegValue(_In_ wchar_t* ValueName);
//
//
//
//#include "ButtonDefs.h"             // Buttons!
//
	// Buttons!
	// You could refer to an individual button like gButtons[BUTTON_NEW - 10001], gButtons[BUTTON_DELAY - 10001], etc.
	//
	static BUTTON gNewButton = {
		{ 2,  0, 72,  52 }, L"New",
		NULL,               // EnabledIcon
		NULL,               // DisabledIcon
		IDB_SCISSORS32x32E, // EnabledIconId
		0,                  // DisabledIconId
		BUTTONSTATE_NORMAL, // State
		0x4E,               // Hotkey (N)
		NULL,				// HWND Handle
		TRUE,               // Enabled
		BUTTON_NEW,         // Id
		FALSE,              // SelectedTool
		0,                  // CursorId
		NULL,               // Cursor
		COLOR_NONE
	};

	static BUTTON gDelayButton = {
		{ 74, 0, 144, 52 }, L"Delay",
		NULL,               // EnabledIcon
		NULL,               // DisabledIcon
		IDB_DELAY32x32E,    // EnabledIconId
		0,                  // DisabledIconId
		BUTTONSTATE_NORMAL, // State
		0x44,               // Hotkey (D)
		NULL,               // HWND Handle
		TRUE,               // Enabled
		BUTTON_DELAY,       // Id
		FALSE,              // SelectedTool
		0,                  // CursorId
		NULL,               // HCURSOR
		COLOR_NONE
	};

	static BUTTON gSaveButton = {
		{ 146, 0, 216, 52 }, L"Save",
		NULL,
		NULL,
		IDB_SAVE32x32E,
		IDB_SAVE32x32D,
		BUTTONSTATE_NORMAL,
		0x53,
		NULL,
		FALSE,				// Initially disabled
		BUTTON_SAVE,
		FALSE,
		0,
		NULL,
		COLOR_NONE
	};

	static BUTTON gCopyButton = {
		{ 218, 0, 288, 52 }, L"Copy",
		NULL,
		NULL,
		IDB_COPY32x32E,
		IDB_COPY32x32D,
		BUTTONSTATE_NORMAL,
		0x43,
		NULL,
		FALSE,
		BUTTON_COPY,
		FALSE,
		0,
		NULL,
		COLOR_NONE
	};

	static BUTTON gHilighterButton = {
		{ 290, 0, 360, 52 }, L"Hilight",
		NULL,
		NULL,
		IDB_YELLOWHILIGHT32x32,
		IDB_HILIGHT32x32D,
		BUTTONSTATE_NORMAL,
		0x48,
		NULL,
		FALSE,
		BUTTON_HILIGHT,
		FALSE,
		IDC_YELLOWHILIGHTCURSOR,
		NULL,
		COLOR_YELLOW
	};

	static BUTTON gRectangleButton = {
		{ 362, 0, 432, 52 }, L"Box",
		NULL,					// EnalbedIcon
		NULL,					// DisabledIcon
		IDB_BOX32x32RED,		// EnabledIconId
		IDB_BOX32x32D,			// DisabledIconId
		BUTTONSTATE_NORMAL,		// State
		0x42,					// Hotkey (B)
		NULL,					// HWND Handle
		FALSE,					// Enabled
		BUTTON_BOX,				// Id
		FALSE,					// SelectedTool
		IDC_REDCROSSHAIR,		// CursorId
		NULL,					// HCURSOR
		COLOR_RED
	};

	static BUTTON gArrowButton = {
		{ 434, 0, 504, 52 }, L"Arrow",
		NULL,					// EnalbedIcon
		NULL,					// DisabledIcon
		IDB_ARROW32x32RED,		// EnabledIconId
		IDB_ARROW32x32D,		// DisabledIconId
		BUTTONSTATE_NORMAL,		// State
		0x41,					// Hotkey (A)
		NULL,					// HWND Handle
		FALSE,					// Enabled
		BUTTON_ARROW,			// Id
		FALSE,					// SelectedTool
		IDC_REDCROSSHAIR,		// CursorId
		NULL,					// HCURSOR
		COLOR_RED
	};

	static BUTTON gRedactButton = {
		{ 506, 0, 576, 52 }, L"Redact",
		NULL,
		NULL,
		IDB_REDACT32x32E,
		IDB_REDACT32x32D,
		BUTTONSTATE_NORMAL,
		0x52,
		NULL,
		FALSE,
		BUTTON_REDACT,
		FALSE,
		IDC_REDACTCURSOR,
		NULL,
		COLOR_BLACK
	};

	static BUTTON gTextButton = {
		{ 578, 0, 648, 52 }, L"Text",
		NULL,
		NULL,
		IDB_TEXT32x32E,
		IDB_TEXT32x32D,
		BUTTONSTATE_NORMAL,
		0x54,
		NULL,
		FALSE,
		BUTTON_TEXT,
		FALSE,
		IDC_TEXTCURSOR,
		NULL,
		COLOR_NONE
	};

	static BUTTON * gButtons[] = { &gNewButton, &gDelayButton, &gSaveButton, &gCopyButton, &gHilighterButton, &gRectangleButton, &gArrowButton, &gRedactButton, &gTextButton };
//
//#include "GdiPlusInterop.h"         // Stuff to make GDI+ work in pure C
	DEFINE_GUID(gEncoderCompressionGuid, 0xe09d739d, 0xccd4, 0x44ee, 0x8e, 0xba, 0x3f, 0xbf, 0x8b, 0xe4, 0xfc, 0x58);

	enum EncoderParameterValueType {
		EncoderParameterValueTypeByte = 1,
		EncoderParameterValueTypeASCII = 2,
		EncoderParameterValueTypeShort = 3,
		EncoderParameterValueTypeLong = 4,
		EncoderParameterValueTypeRational = 5,
		EncoderParameterValueTypeLongRange = 6,
		EncoderParameterValueTypeUndefined = 7,
		EncoderParameterValueTypeRationalRange = 8,
		EncoderParameterValueTypePointer = 9
	};

	struct EncoderParameter {
		GUID  Guid;               // GUID of the parameter
		ULONG NumberOfValues;     // Number of the parameter values
		ULONG Type;               // Value type, like ValueTypeLONG  etc.
		VOID * Value;              // A pointer to the parameter values
	};

	struct EncoderParameters {
		UINT Count;                      // Number of parameters in this structure
		EncoderParameter Parameter[1];   // Parameter values
	};

	struct GdiplusStartupInput {
		UINT32 GdiplusVersion;
		void * DebugEventCallback;
		BOOL   SuppressBackgroundThread;
		BOOL   SuppressExternalCodecs;
	};

	int (WINAPI* GdiplusStartup)(ULONG_PTR* Token, struct GdiplusStartupInput* Size, void*);
	int (WINAPI* GdiplusShutdown)(ULONG_PTR Token);
	int (WINAPI* GdipCreateBitmapFromHBITMAP)(HBITMAP hBitmap, HPALETTE hPalette, ULONG** Bitmap);
	int (WINAPI* GdipDisposeImage)(ULONG* Bitmap);
	int (WINAPI* GdipSaveImageToFile)(ULONG* Image, const WCHAR* Filename, const CLSID* clsidEncoder, const EncoderParameters* EncoderParams);
//
//
//
struct SnipExGlobals {
	SnipExGlobals() : gAppState(APPSTATE_BEFORECAPTURE), gStartingDelayCountdown(6), gCurrentDelayCountdown(6),
		gStartingMainWindowWidth(668), gStartingMainWindowHeight(92)
	{
	}
	void    SetCurrentSnipState(HBITMAP bm) { gSnipStates[gCurrentSnipState] = bm; }
	HBITMAP GetCurrentSnipState() const { return gSnipStates[gCurrentSnipState]; }
	void    PopupMessageErr(LPCWSTR pMsg)
	{
		MessageBoxW(gMainWindowHandle, pMsg, L"Error", MB_OK|MB_ICONERROR|MB_SYSTEMMODAL);
	}
	HRESULT AddAllMenuItems(_In_ HINSTANCE Instance)
	{
		HRESULT Result = S_OK;
		HMENU SystemMenu = GetSystemMenu(gMainWindowHandle, FALSE);
		HKEY SnipExKey = NULL;
		DWORD SnipExKeyDisposition = 0;
		DWORD ValueSize = sizeof(DWORD);
		BOOL AlreadyReplaced = FALSE;
		HKEY IFEOKey = NULL;
		wchar_t DebuggerValue[256] = { 0 };
		UINT16 ReplaceCommand = 0;
		wchar_t ReplacementText[64] = { 0 };
		if(SystemMenu == NULL) {
			Result = E_FAIL;
			MyOutputDebugStringW(L"[%s] Line %d: ERROR: GetSystemMenu failed!\n", __FUNCTIONW__, __LINE__);
			goto Exit;
		}
		AppendMenuW(SystemMenu, MF_SEPARATOR, 0, NULL);
		if((Result = GetSnipExRegValue(REG_DROPSHADOWNAME, &gShouldAddDropShadow)) != ERROR_SUCCESS) {
			goto Exit;
		}
		if((Result = GetSnipExRegValue(REG_REMEMBERTOOLNAME, &gRememberLastTool)) != ERROR_SUCCESS) {
			goto Exit;
		}
		if((Result = GetSnipExRegValue(REG_LASTTOOLNAME, &gLastTool)) != ERROR_SUCCESS) {
			goto Exit;
		}
		if((Result = GetSnipExRegValue(REG_AUTOCOPYNAME, &gAutoCopy)) != ERROR_SUCCESS) {
			goto Exit;
		}
		if(gShouldAddDropShadow > 0)
			AppendMenuW(SystemMenu, MF_STRING | MF_CHECKED, SYSCMD_SHADOW, L"Drop Shadow Effect");
		else
			AppendMenuW(SystemMenu, MF_STRING | MF_UNCHECKED, SYSCMD_SHADOW, L"Drop Shadow Effect");
		if(gAutoCopy > 0)
			AppendMenuW(SystemMenu, MF_STRING | MF_CHECKED, SYSCMD_AUTOCOPY, L"Automatically copy snip to clipboard");
		else
			AppendMenuW(SystemMenu, MF_STRING | MF_UNCHECKED, SYSCMD_AUTOCOPY, L"Automatically copy snip to clipboard");
		if(gRememberLastTool > 0)
			AppendMenuW(SystemMenu, MF_STRING | MF_CHECKED, SYSCMD_REMEMBER, L"Remember Last Tool Used");
		else {
			AppendMenuW(SystemMenu, MF_STRING | MF_UNCHECKED, SYSCMD_REMEMBER, L"Remember Last Tool Used");
			if((Result = DeleteSnipExRegValue(REG_LASTTOOLNAME)) != ERROR_SUCCESS)
				goto Exit;
		}
		AppendMenuW(SystemMenu, MF_STRING, SYSCMD_UNDO, L"Undo (Ctrl+Z)");
		SnipExKeyDisposition = 0;
		ValueSize = sizeof(DebuggerValue);
		// This key should always exist. Something is very wrong if we can't open it.
		if(RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options", 0,
			KEY_QUERY_VALUE, &IFEOKey) != ERROR_SUCCESS) {
			Result = GetLastError();
			MyOutputDebugStringW(L"[%s] Line %d: ERROR: RegOpenKeyEx failed! 0x%lx\n", __FUNCTIONW__, __LINE__, Result);
			goto Exit;
		}
		// If this subkey is not present, then that is OK, it just means it has never been added before
		if(RegCreateKeyExW(IFEOKey, L"SnippingTool.exe", 0, NULL, 0, KEY_QUERY_VALUE, NULL, &SnipExKey, &SnipExKeyDisposition) == ERROR_SUCCESS) {
			RegQueryValueExW(SnipExKey, L"Debugger", NULL, NULL, (LPBYTE)&DebuggerValue, &ValueSize);
			if(wcslen(DebuggerValue) > 0) {
				// Does the file specified by the "Debugger" value actually exist?
				const DWORD FileAttributes = GetFileAttributesW(DebuggerValue);
				if(FileAttributes != INVALID_FILE_ATTRIBUTES && !(FileAttributes & FILE_ATTRIBUTE_DIRECTORY))
					AlreadyReplaced = TRUE;
			}
		}
		if(AlreadyReplaced) {
			ReplaceCommand = SYSCMD_RESTORE;
			wcscpy_s(ReplacementText, _countof(ReplacementText), L"Restore Windows Snipping Tool");
		}
		else {
			ReplaceCommand = SYSCMD_REPLACE;
			wcscpy_s(ReplacementText, _countof(ReplacementText), L"Replace Windows Snipping Tool with SnipEx");
		}
		AppendMenuW(SystemMenu, MF_STRING, ReplaceCommand, ReplacementText);
		// Only works with 8bpp bitmaps
		gUACIcon = (HBITMAP)LoadImageW(Instance, MAKEINTRESOURCEW(IDB_UAC), IMAGE_BITMAP, 0, 0, LR_LOADTRANSPARENT | LR_LOADMAP3DCOLORS | LR_SHARED);
		if(gUACIcon) {
			MENUITEMINFOW MenuItemInfo = { sizeof(MENUITEMINFOW) };
			MenuItemInfo.fMask = MIIM_BITMAP;
			MenuItemInfo.hbmpItem = gUACIcon;
			SetMenuItemInfoW(SystemMenu, ReplaceCommand, FALSE, &MenuItemInfo);
		}
		else {
			Result = GetLastError();
			MyOutputDebugStringW(L"[%s] Line %d: ERROR: Attempting to load the UAC icon failed! 0x%lx\n", __FUNCTIONW__, __LINE__, Result);
		}
	Exit:
		::RegCloseKey(IFEOKey);
		return Result;
	}
	void AdjustWindowSizeForThickTitleBars() // @20201025
	{
		// If a custom DPI/scaling level is set, the title bar and borders will get thicker
		// as scaling level increases and that will eat into the client area of the window.
		// Result is that buttons will get cropped as the client area of the window gets smaller
		// as the title bar and borders get thicker. We have to compensate for that.	
		RECT WindowRect = { 0 };
		RECT ClientRect = { 0 };
		GetWindowRect(gMainWindowHandle, &WindowRect);
		GetClientRect(gMainWindowHandle, &ClientRect);
		const int AdjustedHeight = (WindowRect.bottom - WindowRect.top) - (ClientRect.bottom - ClientRect.top);
		const int AdjustedWidth = (WindowRect.right - WindowRect.left) - (ClientRect.right - ClientRect.left);
		SetWindowPos(gMainWindowHandle, HWND_TOP, 0, 0, gButtons[_countof(gButtons) - 1]->Rectangle.right + AdjustedWidth + 2,
			gButtons[_countof(gButtons) - 1]->Rectangle.bottom + AdjustedHeight + 2, SWP_NOMOVE | SWP_NOOWNERZORDER);
	}
	void CaptureWindow_OnLeftButtonUp()
	{
		MyOutputDebugStringW(L"[%s] Line %d: Left mouse button up over capture window. Selection complete.\n", __FUNCTIONW__, __LINE__);
		if((gCaptureSelectionRectangle.left != gCaptureSelectionRectangle.right) && (gCaptureSelectionRectangle.bottom != gCaptureSelectionRectangle.top)) {
			MyOutputDebugStringW(L"[%s] Line %d: Left mouse button was released with a valid capture region selected.\n", __FUNCTIONW__, __LINE__);
			gAppState = APPSTATE_AFTERCAPTURE;
			ShowWindow(gCaptureWindowHandle, SW_HIDE);
			ShowWindow(gMainWindowHandle, SW_RESTORE);
			RECT CurrentWindowPos = { 0 };
			// Includes both client and non-client area. In other words it returns the same values that I passed in
			// to CreateWindowEx
			GetWindowRect(gMainWindowHandle, &CurrentWindowPos);
			// The reason behind all this is because depending on how the user dragged the selection rectangle, it
			// might be inverted,
			// i.e. the right could actually be the left and the top could be the bottom.
			const int PreviousWindowWidth  = CurrentWindowPos.right - CurrentWindowPos.left;
			const int PreviousWindowHeight = CurrentWindowPos.bottom - CurrentWindowPos.top;
			{
				const int _shadow = static_cast<int>(gShouldAddDropShadow * 8);
				const RECT & r_captr = gCaptureSelectionRectangle;
				gCaptureWidth  = (r_captr.right - r_captr.left) > 0 ? (r_captr.right - r_captr.left) + _shadow : (r_captr.left - r_captr.right) + _shadow;
				gCaptureHeight = (r_captr.bottom - r_captr.top) > 0 ? (r_captr.bottom - r_captr.top) + _shadow : (r_captr.top - r_captr.bottom) + _shadow;
			}
			const int NewWindowWidth = (gCaptureWidth > (PreviousWindowWidth - 20)) ? (gCaptureWidth + 20) : PreviousWindowWidth;
			const int NewWindowHeight = gCaptureHeight + PreviousWindowHeight + 7;
			SetWindowPos(gMainWindowHandle, HWND_TOP, CurrentWindowPos.left, CurrentWindowPos.top, NewWindowWidth, NewWindowHeight, 0);
			gSnipStates[0] = CreateBitmap(gCaptureWidth, gCaptureHeight, 1, 32, NULL);
			HDC SnipDC = CreateCompatibleDC(NULL);
			SelectObject(SnipDC, gSnipStates[0]);
			HDC BigDC = CreateCompatibleDC(NULL);
			SelectObject(BigDC, gCleanScreenShot);
			BitBlt(SnipDC, 0, 0, gCaptureWidth, gCaptureHeight, BigDC, MIN(gCaptureSelectionRectangle.left, gCaptureSelectionRectangle.right),
				MIN(gCaptureSelectionRectangle.top, gCaptureSelectionRectangle.bottom), SRCCOPY);
			if(gShouldAddDropShadow) {
				MyOutputDebugStringW(L"[%s] Line %d: Adding shadow effect.\n", __FUNCTIONW__, __LINE__);
				HPEN Pen0 = CreatePen(PS_SOLID, 1, RGB(128, 128, 128));
				HPEN Pen1 = CreatePen(PS_SOLID, 1, RGB(159, 159, 159));
				HPEN Pen2 = CreatePen(PS_SOLID, 1, RGB(172, 172, 172));
				HPEN Pen3 = CreatePen(PS_SOLID, 1, RGB(192, 192, 192));
				HPEN Pen4 = CreatePen(PS_SOLID, 1, RGB(215, 215, 215));
				HPEN Pen5 = CreatePen(PS_SOLID, 1, RGB(234, 234, 234));
				HPEN Pen6 = CreatePen(PS_SOLID, 1, RGB(245, 245, 245));
				HPEN Pen7 = CreatePen(PS_SOLID, 1, RGB(250, 250, 250));
				//
				SelectObject(SnipDC, Pen0);
				MoveToEx(SnipDC, 0, gCaptureHeight - 8, NULL);
				LineTo(SnipDC, gCaptureWidth - 8, gCaptureHeight - 8);
				LineTo(SnipDC, gCaptureWidth - 8, -1);
				//
				SelectObject(SnipDC, Pen1);
				MoveToEx(SnipDC, 0, gCaptureHeight - 7, NULL);
				LineTo(SnipDC, gCaptureWidth - 7, gCaptureHeight - 7);
				LineTo(SnipDC, gCaptureWidth - 7, -1);
				//
				SelectObject(SnipDC, Pen2);
				MoveToEx(SnipDC, 0, gCaptureHeight - 6, NULL);
				LineTo(SnipDC, gCaptureWidth - 6, gCaptureHeight - 6);
				LineTo(SnipDC, gCaptureWidth - 6, -1);
				//
				SelectObject(SnipDC, Pen3);
				MoveToEx(SnipDC, 0, gCaptureHeight - 5, NULL);
				LineTo(SnipDC, gCaptureWidth - 5, gCaptureHeight - 5);
				LineTo(SnipDC, gCaptureWidth - 5, -1);
				//
				SelectObject(SnipDC, Pen4);
				MoveToEx(SnipDC, 0, gCaptureHeight - 4, NULL);
				LineTo(SnipDC, gCaptureWidth - 4, gCaptureHeight - 4);
				LineTo(SnipDC, gCaptureWidth - 4, -1);
				//
				SelectObject(SnipDC, Pen5);
				MoveToEx(SnipDC, 0, gCaptureHeight - 3, NULL);
				LineTo(SnipDC, gCaptureWidth - 3, gCaptureHeight - 3);
				LineTo(SnipDC, gCaptureWidth - 3, -1);
				//
				SelectObject(SnipDC, Pen6);
				MoveToEx(SnipDC, 0, gCaptureHeight - 2, NULL);
				LineTo(SnipDC, gCaptureWidth - 2, gCaptureHeight - 2);
				LineTo(SnipDC, gCaptureWidth - 2, -1);
				//
				SelectObject(SnipDC, Pen7);
				MoveToEx(SnipDC, 0, gCaptureHeight - 1, NULL);
				LineTo(SnipDC, gCaptureWidth - 1, gCaptureHeight - 1);
				LineTo(SnipDC, gCaptureWidth - 1, -1);
				// MSDN says that DeleteObject will fail and return 0 if the object is still selected into a DC.
				// Yet Pen7 is still selected, yet DeleteObject does not fail when we try to delete Pen7, and I don't know why.
				DeleteObject(Pen0);
				DeleteObject(Pen1);
				DeleteObject(Pen2);
				DeleteObject(Pen3);
				DeleteObject(Pen4);
				DeleteObject(Pen5);
				DeleteObject(Pen6);
				DeleteObject(Pen7);
				//
				//////////////////////////////////////
				// Set the alpha to 0 on the shadowed bits
				//GetDIBits()
			}
			DeleteDC(BigDC);
			DeleteDC(SnipDC);
			for(UINT8 Counter = 0; Counter < _countof(gButtons); Counter++) {
				gButtons[Counter]->Enabled = TRUE;
			}
			if(gRememberLastTool) {
				if(GetSnipExRegValue(REG_LASTTOOLNAME, &gLastTool) != ERROR_SUCCESS) {
					CRASH(0);
				}
				if(gLastTool >= BUTTON_HILIGHT) {
					MyOutputDebugStringW(L"[%s] Line %d: Selecting previously used tool: %d\n", __FUNCTIONW__, __LINE__, gLastTool);
					SendMessageW(gMainWindowHandle, WM_COMMAND, gLastTool, 0);
				}
				else {
					MyOutputDebugStringW(L"[%s] Line %d: No previously used tool detected.\n", __FUNCTIONW__, __LINE__);
				}
			}
			wchar_t TitleBuffer[128] = { 0 };
			(void)_snwprintf_s(TitleBuffer, _countof(TitleBuffer), _TRUNCATE, L"SnipEx - Current Snip: %dx%d", gCaptureWidth, gCaptureHeight);
			SetWindowTextW(gMainWindowHandle, TitleBuffer);
			::SetCursor(::LoadCursor(NULL, IDC_ARROW));
			gNewButton.SelectedTool = FALSE;
			gNewButton.State = BUTTONSTATE_NORMAL;
			gDelayButton.SelectedTool = FALSE;
			gDelayButton.State = BUTTONSTATE_NORMAL;
			if(gAutoCopy) {
				MyOutputDebugStringW(L"[%s] Line %d: Auto copy enabled. Copying snip to clipboard.\n", __FUNCTIONW__, __LINE__);
				if(CopyButton_Click() == FALSE) {
					MyOutputDebugStringW(L"[%s] Line %d: Auto copy failed!\n", __FUNCTIONW__, __LINE__);
					CRASH(0);
				}
			}
		}
	}
	APPSTATE gAppState; //= APPSTATE_BEFORECAPTURE; // To track the overall state of the application
	BOOL   gMainWindowIsRunning;                 // Set this to FALSE to exit the app immediately.
	HWND   gMainWindowHandle;                    // Handle to the main window, i.e. the window with all the buttons on it.
	HWND   gCaptureWindowHandle;                 // Handle to the capture window, i.e. the grey selection window that overlays the entire desktop.
	const  INT8 gStartingDelayCountdown;// = 6;     // Default seconds to wait after clicking the 'Delay' button.
	INT8   gCurrentDelayCountdown;// = 6;           // The value of the countdown timer at this moment.
	UINT16 gDisplayWidth;                        // Display width, accounting for multiple monitors.
	UINT16 gDisplayHeight;                       // Display height, accounting for multiple monitors.
	INT16  gDisplayLeft;                         // Depending on how the monitors are arranged, the left-most coordinate might not be zero.
	INT16  gDisplayTop; // Depending on how the monitors are arranged, the top-most coordinate might not be zero.
	UINT16 gStartingMainWindowWidth;//  = 668; // The beginning width of the tool window - just enough to fit all the buttons.
	UINT16 gStartingMainWindowHeight;// = 92;  // The beginning height of the tool window - just enough to fit the buttons.
	HBITMAP gCleanScreenShot;        // A clean copy of the screenshot from before we started drawing on it.
	HBITMAP gScratchBitmap;          // For use during drawing.
	RECT   gCaptureSelectionRectangle; // The rectangle the user draws with the mouse to select a subsection of the screen.
	int    gCaptureWidth;           // Width in pixels of the user's captured snip.
	int    gCaptureHeight;          // Height in pixels of the user's captured snip.
	BOOL   gLeftMouseButtonIsDown; // When the user is drawing with the mouse, the left mouse button is down.
	UINT8  gCurrentSnipState;    // An array of bitmap states that we can revert back to, so we can undo changes. ctrl-z.
	HBITMAP gSnipStates[32];    // Snip state 0 will always be the unaltered clip right as the user first took it.
	HBITMAP gUACIcon; // The UAC icon that sits next to the "Replace Windows Snipping Tool with SnipEx" menu item.
	DWORD  gShouldAddDropShadow; // Does the user want to add a drop-shadow effect to the snip?
	DWORD  gRememberLastTool;    // Does the user want to remember the previously-used tool?
	DWORD  gAutoCopy;            // Does the user want to automatically copy each snip to the clipboard?
	DWORD  gLastTool;            // What was the last tool that the user used? (NOT save, copy, new, or delay.)
	HFONT  gFont;                // The font the user selects for the Text tool.
	COLORREF gFontColor;        // The color of the font that the user selects for the Text tool.
	POINT  gTextBoxLocation;     // The coordinates where we will spawn text
	wchar_t gTextBuffer[1024];  // Buffer for Text tool.
};

SnipExGlobals SnExG;
//
// Application entry-point.
// MSDN says that if WinMain fails before reaching the message loop, we should return zero.
//
int CALLBACK WinMain(_In_ HINSTANCE Instance, _In_opt_ HINSTANCE PreviousInstance, _In_ LPSTR CommandLine, _In_ int WindowShowCode)
{
	MyOutputDebugStringW(L"[%s] Line %d: Entered.\n", __FUNCTIONW__, __LINE__);
	UNREFERENCED_PARAMETER(PreviousInstance);
	UNREFERENCED_PARAMETER(CommandLine);
	UNREFERENCED_PARAMETER(WindowShowCode);
	if((SnExG.gFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT)) == NULL) {
		MessageBoxW(0, L"Failed to retrieve default GUI font!", L"Error", MB_OK|MB_ICONERROR|MB_SYSTEMMODAL);
		return 0;
	}
	SnExG.gFontColor = RGB(255, 0, 255);
	// NOTE: This width and height are the bounding box that includes all of the user's monitors.
	// I have only tested with two monitors of equal dimensions, and with a primary monitor that is larger
	// than the secondary monitor. No other configurations were tested, so this may not work correctly
	// if the user has several irregularly-arranged monitors. Also, if the monitors are arranged
	// in reverse order, the left-most X coordinate may be e.g. negative 1920! In other words, 0,0 may not
	// necessarily be the top-left corner of the user's viewing area.
	SnExG.gDisplayWidth  = (UINT16)GetSystemMetrics(SM_CXVIRTUALSCREEN);
	SnExG.gDisplayHeight = (UINT16)GetSystemMetrics(SM_CYVIRTUALSCREEN);
	SnExG.gDisplayLeft   = (INT16)GetSystemMetrics(SM_XVIRTUALSCREEN);
	SnExG.gDisplayTop    = (INT16)GetSystemMetrics(SM_YVIRTUALSCREEN);
	if(!SnExG.gDisplayWidth || !SnExG.gDisplayHeight) {
		MessageBoxW(0, L"Failed to retrieve display area via GetSystemMetrics!", L"Error", MB_OK|MB_ICONERROR|MB_SYSTEMMODAL);
		return 0;
	}
	MyOutputDebugStringW(L"[%s] Line %d: Detected a screen area of %dx%d.\n", __FUNCTIONW__, __LINE__, SnExG.gDisplayWidth, SnExG.gDisplayHeight);
	// If the user had a custom scaling/DPI level set, this app was originally not high-DPI aware, and so what would happen
	// is that the buttons would get cropped as the non-client area of the window got bigger, and the screen would "zoom in"
	// whenever the user clicked "New"!
	// @20201025 if(AdjustForCustomScaling() == FALSE) { return 0; }
	WNDCLASSEXW CaptureWindowClass; // = { sizeof(WNDCLASSEXW) };
	INITWINAPISTRUCT(CaptureWindowClass);
	CaptureWindowClass.style = CS_HREDRAW | CS_VREDRAW;
	CaptureWindowClass.hInstance     = Instance;
	CaptureWindowClass.lpszClassName = L"SnipExCaptureWindowClass";
	CaptureWindowClass.hCursor       = LoadCursorW(NULL, IDC_CROSS);
	CaptureWindowClass.hbrBackground = GetSysColorBrush(COLOR_DESKTOP);
	CaptureWindowClass.lpfnWndProc   = CaptureWindowCallback;
	if(RegisterClassEx(&CaptureWindowClass) == 0) {
		MyOutputDebugStringW(L"[%s] Line %d: RegisterClassEx failed with 0x%lx!\n", __FUNCTIONW__, __LINE__, GetLastError());
		MessageBoxW(0, L"Failed to register CaptureWindowClass with error 0x%lx!", L"Error", MB_OK|MB_ICONERROR|MB_SYSTEMMODAL);
		return 0;
	}
	// This window will capture the entire display surface, including multiple monitors. It will then display an
	// exact
	// screenshot of the desktop over the real desktop. Then the user will be able to select a subsection of that
	// using
	// the mouse. This could also be used to play a prank on somebody and make them think their desktop was hung.
	SnExG.gCaptureWindowHandle = CreateWindowExW(WS_EX_TOPMOST | WS_EX_TOOLWINDOW,       // No taskbar icon
		CaptureWindowClass.lpszClassName, L"", 0/*Not visible*/, CW_USEDEFAULT, CW_USEDEFAULT, 0/*Will size it later*/, 0, NULL, NULL, Instance, NULL);
	if(SnExG.gCaptureWindowHandle == NULL) {
		MyOutputDebugStringW(L"[%s] Line %d: CreateWindowEx (capture window) failed with 0x%lx!\n", __FUNCTIONW__, __LINE__, GetLastError());
		MessageBoxW(0, L"Failed to create Capture Window!", L"Error", MB_OK|MB_ICONERROR|MB_SYSTEMMODAL);
		return 0;
	}
	// Remove all window style, including title bar. We're trying to make the "capture window" indistinguishable
	// from the real desktop that lay underneath it.
	if(SetWindowLongPtrW(SnExG.gCaptureWindowHandle, GWL_STYLE, 0) == 0) {
		MyOutputDebugStringW(L"[%s] Line %d: SetWindowLongPtwW failed with 0x%lx!\n", __FUNCTIONW__, __LINE__, GetLastError());
		MessageBoxW(0, L"SetWindowLongPtrW failed!", L"Error", MB_OK|MB_ICONERROR|MB_SYSTEMMODAL);
		return 0;
	}
	// The main window class that appears when the application is first launched.
	WNDCLASSEXW MainWindowClass;// = { sizeof(WNDCLASSEXW) };
	INITWINAPISTRUCT(MainWindowClass);
	MainWindowClass.style = CS_HREDRAW | CS_VREDRAW;
	MainWindowClass.hInstance     = Instance;
	MainWindowClass.lpszClassName = L"SnipExWindowClass";
	MainWindowClass.hCursor       = LoadCursorW(NULL, IDC_ARROW);
	MainWindowClass.hbrBackground = GetSysColorBrush(COLOR_WINDOW);
	MainWindowClass.lpfnWndProc   = MainWindowCallback;
	MainWindowClass.hIcon = LoadIconW(Instance, MAKEINTRESOURCEW(IDI_MAINAPPICON));
	MainWindowClass.hIconSm       = LoadIconW(Instance, MAKEINTRESOURCEW(IDI_MAINAPPICON));
	if(RegisterClassExW(&MainWindowClass) == 0) {
		MyOutputDebugStringW(L"[%s] Line %d: RegisterClassExW failed with 0x%lx!\n", __FUNCTIONW__, __LINE__, GetLastError());
		MessageBoxW(0, L"RegisterClassExW failed!", L"Error", MB_OK|MB_ICONERROR|MB_SYSTEMMODAL);
		return 0;
	}
	SnExG.gMainWindowHandle = CreateWindowExW(0, MainWindowClass.lpszClassName, L"SnipEx",
		WS_VISIBLE | (WS_OVERLAPPEDWINDOW ^ (WS_MAXIMIZEBOX | WS_THICKFRAME)),
		CW_USEDEFAULT, CW_USEDEFAULT, SnExG.gStartingMainWindowWidth, SnExG.gStartingMainWindowHeight, NULL, NULL, Instance, NULL);
	if(SnExG.gMainWindowHandle == NULL) {
		MyOutputDebugStringW(L"[%s] Line %d: CreateWindowEx (main window) failed with 0x%lx!\n", __FUNCTIONW__, __LINE__, GetLastError());
		MessageBoxW(0, L"CreateWindowEx (main window) failed!", L"Error", MB_OK|MB_ICONERROR|MB_SYSTEMMODAL);
		return 0;
	}
	#if _DEBUG
	wchar_t TitleBarBuffer[64] = { 0 };
	GetWindowTextW(SnExG.gMainWindowHandle, TitleBarBuffer, _countof(TitleBarBuffer));
	wcscat_s(TitleBarBuffer, _countof(TitleBarBuffer), L" - *DEBUG BUILD*");
	SetWindowTextW(SnExG.gMainWindowHandle, TitleBarBuffer);
	MyOutputDebugStringW(L"[%s] Line %d: Setting window text for *DEBUG BUILD*.\n", __FUNCTIONW__, __LINE__);
	#endif
	// Create all the buttons.
	for(UINT8 Counter = 0; Counter < _countof(gButtons); Counter++) {
		const uint _idx = Counter;
		BUTTON * p_button = gButtons[_idx];
		HWND ButtonHandle = CreateWindowExW(0, L"BUTTON", p_button->Caption, BS_OWNERDRAW | WS_VISIBLE | WS_CHILD, 
			p_button->Rectangle.left, p_button->Rectangle.top, p_button->Rectangle.right - p_button->Rectangle.left,
			p_button->Rectangle.bottom, SnExG.gMainWindowHandle, (HMENU)p_button->Id, (HINSTANCE)GetWindowLongPtr(SnExG.gMainWindowHandle, GWLP_HINSTANCE), NULL);
		if(ButtonHandle == NULL) {
			MyOutputDebugStringW(L"[%s] Line %d: Failed to create %s button with error 0x%lx\n", __FUNCTIONW__, __LINE__, p_button->Caption, GetLastError());
			MessageBoxW(0, L"Failed to create button!", L"Error", MB_OK|MB_ICONERROR|MB_SYSTEMMODAL);
			return 0;
		}
		p_button->Handle = ButtonHandle;
		if(p_button->EnabledIconId > 0) {
			p_button->EnabledIcon = (HBITMAP)LoadImageW(Instance, MAKEINTRESOURCEW(p_button->EnabledIconId), IMAGE_BITMAP, 0, 0, 0);
			if(p_button->EnabledIcon == NULL) {
				MyOutputDebugStringW(L"[%s] Line %d: Loading resource %d failed! Error: 0x%lx\n", __FUNCTIONW__, __LINE__, p_button->EnabledIconId, GetLastError());
				MessageBoxW(0, L"Failed to create button!", L"Error", MB_OK|MB_ICONERROR|MB_SYSTEMMODAL);
				return 0;
			}
		}
		if(p_button->DisabledIconId > 0) {
			p_button->DisabledIcon = (HBITMAP)LoadImageW(Instance, MAKEINTRESOURCEW(p_button->DisabledIconId),
				IMAGE_BITMAP, 0, 0, 0);
			if(p_button->DisabledIcon == NULL) {
				MyOutputDebugStringW(L"[%s] Line %d: Loading resource %d failed! Error: 0x%lx\n", __FUNCTIONW__, __LINE__, p_button->DisabledIconId, GetLastError());
				MessageBoxW(0, L"Failed to create button!", L"Error", MB_OK|MB_ICONERROR|MB_SYSTEMMODAL);
				return 0;
			}
		}
		if(p_button->CursorId > 0) {
			p_button->Cursor = LoadCursor(Instance, MAKEINTRESOURCE(p_button->CursorId));
			if(p_button->Cursor == NULL) {
				MyOutputDebugStringW(L"[%s] Line %d: Loading resource %d failed! Error: 0x%lx\n", __FUNCTIONW__, __LINE__, p_button->CursorId, GetLastError());
				MessageBoxW(0, L"Failed to create button!", L"Error", MB_OK|MB_ICONERROR|MB_SYSTEMMODAL);
				return 0;
			}
		}
		MyOutputDebugStringW(L"[%s] Line %d: %s button created.\n", __FUNCTIONW__, __LINE__, p_button->Caption);
	}
	// Add custom menu items to the form's system menu in the top left
	if(SnExG.AddAllMenuItems(Instance) != S_OK) {
		MyOutputDebugStringW(L"[%s] Line %d: ERROR: Adding drop-down menu items failed!\n", __FUNCTIONW__, __LINE__);
		MessageBoxW(0, L"Failed to create drop-down menu items!", L"Error", MB_OK|MB_ICONERROR|MB_SYSTEMMODAL);
		return 0;
	}
	SnExG.AdjustWindowSizeForThickTitleBars(); // @20201025
	SnExG.gMainWindowIsRunning = TRUE;
	MSG MainWindowMessage      = { 0 };
	MSG CaptureWindowMessage   = { 0 };
	MyOutputDebugStringW(L"[%s] Line %d: Setup is finished. Entering message loop.\n", __FUNCTIONW__, __LINE__);
	//
	// MAIN MESSAGE LOOP //
	//
	while(SnExG.gMainWindowIsRunning) {
		// Drain message queue for main window.
		while(PeekMessageW(&MainWindowMessage, SnExG.gMainWindowHandle, 0, 0, PM_REMOVE)) {
			DispatchMessageW(&MainWindowMessage);
		}
		// Drain message queue for capture window.
		while(PeekMessageW(&CaptureWindowMessage, SnExG.gCaptureWindowHandle, 0, 0, PM_REMOVE)) {
			DispatchMessageW(&CaptureWindowMessage);
		}
		if((SnExG.gAppState == APPSTATE_AFTERCAPTURE) && SnExG.gLeftMouseButtonIsDown) {
			if(GetAsyncKeyState(VK_LBUTTON) == 0) {
				SnExG.gLeftMouseButtonIsDown = FALSE;
				SendMessageW(SnExG.gMainWindowHandle, WM_LBUTTONUP, 0, 0);
			}
		}
		Sleep(1); // Could be anywhere from 0.5ms to 15.6ms
	}
	return 0;
}

// Handles window messages for the main window.
LRESULT CALLBACK MainWindowCallback(_In_ HWND Window, _In_ UINT Message, _In_ WPARAM WParam, _In_ LPARAM LParam)
{
	LRESULT Result = 0;
	static BOOL CurrentlyDrawing;
	static POINT MousePosWhenDrawingStarted;
	static POINT PreviousMousePos;
	static SMALLPOINT HilighterPixelsAlreadyDrawn[32768] = { 0 }; // 128k of memory
	static UINT16 HilighterPixelsAlreadyDrawnCounter = 0;
	switch(Message) {
		case WM_CLOSE:
		    PostQuitMessage(0);
		    SnExG.gMainWindowIsRunning = FALSE;
		    break;
		case WM_KEYDOWN:
	    {
		    // Ctrl+Z, Undo
		    if((WParam == 0x5A) && GetKeyState(VK_CONTROL) && (SnExG.gAppState == APPSTATE_AFTERCAPTURE) && !CurrentlyDrawing) {
			    if(SnExG.gCurrentSnipState > 0) {
				    SnExG.gCurrentSnipState--;
				    MyOutputDebugStringW(L"[%s] Line %d: Deleting gSnipStates[%d]\n", __FUNCTIONW__, __LINE__, SnExG.gCurrentSnipState + 1);
				    memzero(HilighterPixelsAlreadyDrawn, sizeof(HilighterPixelsAlreadyDrawn));
				    HilighterPixelsAlreadyDrawnCounter = 0;
				    if(DeleteObject(SnExG.gSnipStates[SnExG.gCurrentSnipState + 1]) == 0) {
					    MyOutputDebugStringW(L"[%s] Line %d: DeleteObject failed!\n", __FUNCTIONW__, __LINE__);
					    CRASH(0);
				    }
				    SnExG.gSnipStates[SnExG.gCurrentSnipState + 1] = NULL;
				    if(SnExG.gAutoCopy) {
					    MyOutputDebugStringW(L"[%s] Line %d: Auto copy enabled. Copying snip to clipboard.\n", __FUNCTIONW__, __LINE__);
					    if(CopyButton_Click() == FALSE) {
						    MyOutputDebugStringW(L"[%s] Line %d: Auto copy failed!\n", __FUNCTIONW__, __LINE__);
						    CRASH(0);
					    }
				    }
			    }
		    }
		    // Allow Escape to terminate the app
		    if((WParam == VK_ESCAPE) && oneof2(SnExG.gAppState, APPSTATE_AFTERCAPTURE, APPSTATE_BEFORECAPTURE)) {
			    MyOutputDebugStringW(L"[%s] Line %d: Escape key was pressed. Exiting application.\n", __FUNCTIONW__, __LINE__);
			    PostQuitMessage(0);
			    SnExG.gMainWindowIsRunning = FALSE;
		    }
		    if((WParam == VK_ESCAPE) && (SnExG.gAppState == APPSTATE_DELAYCOOKING)) {
			    // Cancel a current delay countdown.
			    MyOutputDebugStringW(L"[%s] Line %d: Escape key was pressed during delay countdown. Cancelling countdown.\n", __FUNCTIONW__, __LINE__);
			    SnExG.gAppState = APPSTATE_BEFORECAPTURE;
			    KillTimer(SnExG.gMainWindowHandle, DELAY_TIMER);
			    SnExG.gCurrentDelayCountdown = SnExG.gStartingDelayCountdown;
		    }
		    InvalidateRect(Window, NULL, FALSE);
		    break;
	    }
		case WM_KEYUP:
	    {
		    for(UINT8 Counter = 0; Counter < _countof(gButtons); Counter++) {
			    if(WParam == gButtons[Counter]->Hotkey && gButtons[Counter]->Enabled == TRUE) {
				    MyOutputDebugStringW(L"[%s] Line %d: Hotkey released, executing button '%s'\n", __FUNCTIONW__, __LINE__, gButtons[Counter]->Caption);
				    SendMessageW(SnExG.gMainWindowHandle, WM_COMMAND, static_cast<WPARAM>(gButtons[Counter]->Id), 0);
			    }
		    }
		    break;
	    }
		case WM_SETCURSOR: break; // To keep Windows from automatically trying to set my cursor for me.
		case WM_LBUTTONDOWN:
	    {
		    MyOutputDebugStringW(L"[%s] Line %d: Left mouse button down.\n", __FUNCTIONW__, __LINE__);
		    SnExG.gLeftMouseButtonIsDown = TRUE;
		    if(CurrentlyDrawing == FALSE && SnExG.gAppState == APPSTATE_AFTERCAPTURE) {
			    BOOL DrawingToolSelected = FALSE;
			    for(UINT8 Counter = 0; Counter < _countof(gButtons); Counter++) {
				    if(gButtons[Counter]->SelectedTool == TRUE) {
					    DrawingToolSelected = TRUE;
				    }
			    }
			    if(DrawingToolSelected == FALSE) {
				    MyOutputDebugStringW(L"[%s] Line %d: No drawing tool is selected. Won't start drawing.\n", __FUNCTIONW__, __LINE__);
				    break;
			    }
			    POINT Mouse = { 0 };
			    GetCursorPos(&Mouse);
			    ScreenToClient(SnExG.gMainWindowHandle, &Mouse);
			    MousePosWhenDrawingStarted = Mouse;
			    if(SnExG.gCurrentSnipState >= _countof(SnExG.gSnipStates) - 1) {
				    SnExG.PopupMessageErr(L"Maximum number of changes exceeded. Ctrl+Z to undo a change first.");
				    break;
			    }
			    if(Mouse.x < 2 || Mouse.y < 52) {
				    MyOutputDebugStringW(L"[%s] Line %d: Mouse was not over the screen capture area. Will not start drawing.\n", __FUNCTIONW__, __LINE__);
				    break;
			    }
			    CurrentlyDrawing = TRUE;
			    if(gTextButton.SelectedTool == TRUE)
				    MyOutputDebugStringW(L"[%s] Line %d: Text entry mode started.\n", __FUNCTIONW__, __LINE__);
			    else
				    MyOutputDebugStringW(L"[%s] Line %d: Drawing started.\n", __FUNCTIONW__, __LINE__);
			    if(SnExG.gScratchBitmap != NULL)
				    MyOutputDebugStringW(L"[%s] Line %d: gScratchBitmap was not null, but it was expected to be!\n", __FUNCTIONW__, __LINE__);
			    SnExG.gScratchBitmap = (HBITMAP)CopyImage(SnExG.GetCurrentSnipState(), IMAGE_BITMAP, 0, 0, 0);
			    SnExG.gCurrentSnipState++;
			    MyOutputDebugStringW(L"[%s] Line %d: Snips: %i\n", __FUNCTIONW__, __LINE__, SnExG.gCurrentSnipState);
		    }
		    break;
	    }
		case WM_LBUTTONUP:
	    {
		    if(gTextButton.SelectedTool == FALSE)
			    MyOutputDebugStringW(L"[%s] Line %d: Left mouse button up. Drawing stopped.\n", __FUNCTIONW__, __LINE__);
		    SnExG.gLeftMouseButtonIsDown = FALSE;
		    if(gTextButton.SelectedTool == TRUE) {
			    if(MousePosWhenDrawingStarted.x < 2 || MousePosWhenDrawingStarted.y < 52) {
				    MyOutputDebugStringW(L"[%s] Line %d: Mouse was not over the screen capture area. Will not place text.\n", __FUNCTIONW__, __LINE__);
				    break;
			    }
			    MyOutputDebugStringW(L"[%s] Line %d: Placing text at %dx%d.\n", __FUNCTIONW__, __LINE__, MousePosWhenDrawingStarted.x, MousePosWhenDrawingStarted.y);
			    HDC ScratchDC = CreateCompatibleDC(NULL);
			    SetBkMode(ScratchDC, TRANSPARENT);
			    SelectObject(ScratchDC, SnExG.gScratchBitmap);
			    SelectObject(ScratchDC, SnExG.gFont);
			    SetTextColor(ScratchDC, SnExG.gFontColor);
			    SnExG.gTextBoxLocation.x = MousePosWhenDrawingStarted.x;
			    SnExG.gTextBoxLocation.y = MousePosWhenDrawingStarted.y;
			    DialogBoxW(NULL, MAKEINTRESOURCEW(IDD_DIALOG1), SnExG.gMainWindowHandle, (DLGPROC)TextEditCallback);
			    // Get the size of the font that the user has selected, since the font size will determine
			    // the exact coordinates of where the text box and text will go.
			    HDC DC = CreateCompatibleDC(NULL);
			    SelectObject(DC, (HFONT)SnExG.gFont);
			    TEXTMETRICW TextMetrics = { 0 };
			    GetTextMetricsW(DC, &TextMetrics);
			    DeleteDC(DC);
			    TextOutW(ScratchDC, MousePosWhenDrawingStarted.x - 6,
				MousePosWhenDrawingStarted.y - 58 - (TextMetrics.tmHeight / 2), SnExG.gTextBuffer, (int)wcslen(SnExG.gTextBuffer));
			    DeleteDC(ScratchDC);
			    RECT SnipRect = { 0 };
			    GetClientRect(Window, &SnipRect);
			    SnipRect.top = 54;
			    InvalidateRect(Window, &SnipRect, FALSE);
			    UpdateWindow(SnExG.gMainWindowHandle);
		    }
		    MousePosWhenDrawingStarted.x = 0;
		    MousePosWhenDrawingStarted.y = 0;
		    PreviousMousePos.x = 0;
		    PreviousMousePos.y = 0;
		    CurrentlyDrawing = FALSE;
		    if(SnExG.gScratchBitmap) {
			    SnExG.SetCurrentSnipState((HBITMAP)CopyImage(SnExG.gScratchBitmap, IMAGE_BITMAP, 0, 0, 0));
			    if(DeleteObject(SnExG.gScratchBitmap) == 0) {
				    MyOutputDebugStringW(L"[%s] Line %d: DeleteObject failed! Was the bitmap still selected into a DC?\n", __FUNCTIONW__, __LINE__);
				    CRASH(0);
			    }
			    else
				    MyOutputDebugStringW(L"[%s] Line %d: gScratchBitmap deleted.\n", __FUNCTIONW__, __LINE__);
			    SnExG.gScratchBitmap = NULL;
			    if(SnExG.gAutoCopy) {
				    MyOutputDebugStringW(L"[%s] Line %d: Auto copy enabled. Copying snip to clipboard.\n", __FUNCTIONW__, __LINE__);
				    if(CopyButton_Click() == FALSE) {
					    MyOutputDebugStringW(L"[%s] Line %d: Auto copy failed!\n", __FUNCTIONW__, __LINE__);
					    CRASH(0);
				    }
			    }
		    }
		    break;
	    }
		case WM_MOUSEMOVE:
		case WM_NCMOUSEMOVE:
	    {
		    // Handle drawing first
		    if(CurrentlyDrawing) {
			    if(SnExG.gScratchBitmap == NULL) {
				    MyOutputDebugStringW(L"[%s] Line %d: gScratchBitmap is NULL\n", __FUNCTIONW__, __LINE__);
				    break;
			    }
			    if(gHilighterButton.SelectedTool == TRUE) {
				    POINT Mouse = { 0 };
				    GetCursorPos(&Mouse);
				    ScreenToClient(SnExG.gMainWindowHandle, &Mouse);
				    // Adjust for snip area, maintain Y axis
				    Mouse.x -= 7;
				    Mouse.y = MousePosWhenDrawingStarted.y - 66;
				    HDC ScratchDC = CreateCompatibleDC(NULL);
				    HDC HilightDC = CreateCompatibleDC(NULL);
				    SelectObject(ScratchDC, SnExG.gScratchBitmap);
				    BYTE HilightBits[] = { 0, 0, 0, 0 };
				    switch(gHilighterButton.Color) {
					    case COLOR_YELLOW:
							memset(&HilightBits[0], 0x00, 1);
							memset(&HilightBits[1], 0xFF, 1);
							memset(&HilightBits[2], 0xFF, 1);
							memset(&HilightBits[3], 0xFF, 1);
							break;
					    case COLOR_PINK:
							memset(&HilightBits[0], 0xDC, 1);
							memset(&HilightBits[1], 0x00, 1);
							memset(&HilightBits[2], 0xFF, 1);
							memset(&HilightBits[3], 0xFF, 1);
							break;
					    case COLOR_ORANGE:
							memset(&HilightBits[0], 0x00, 1);
							memset(&HilightBits[1], 0x6A, 1);
							memset(&HilightBits[2], 0xFF, 1);
							memset(&HilightBits[3], 0xFF, 1);
							break;
					    case COLOR_GREEN:
							memset(&HilightBits[0], 0x00, 1);
							memset(&HilightBits[1], 0xFF, 1);
							memset(&HilightBits[2], 0x00, 1);
							memset(&HilightBits[3], 0xFF, 1);
							break;
					    default:
							MyOutputDebugStringW(L"[%s] Line %d: BUG: Unknown color in hilighter function!\n", __FUNCTIONW__, __LINE__);
				    }
				    HBITMAP HilightPixel = CreateBitmap(1, 1, 1, 32, HilightBits);
				    SelectObject(HilightDC, HilightPixel);
				    BLENDFUNCTION InverseBlendFunction = { AC_SRC_OVER, 0, 0, AC_SRC_ALPHA };
				    for(UINT8 XPixel = 0; XPixel < 10; XPixel++) {
					    for(UINT8 YPixel = 0; YPixel < 20; YPixel++) {
						    BOOL PixelAlreadyDrawn = FALSE;
						    for(UINT16 Counter = 0; Counter < _countof(HilighterPixelsAlreadyDrawn); Counter++) {
							    if((HilighterPixelsAlreadyDrawn[Counter].x == Mouse.x + XPixel) && (HilighterPixelsAlreadyDrawn[Counter].y == Mouse.y + YPixel))
								    PixelAlreadyDrawn = TRUE;
						    }
						    if(PixelAlreadyDrawn == FALSE) {
							    const COLORREF CurrentPixelColor = GetPixel(ScratchDC, Mouse.x + XPixel, Mouse.y + YPixel);
							    const UINT16 CurrentPixelBrightness = GetRValue(CurrentPixelColor) + GetGValue(CurrentPixelColor) + GetBValue(CurrentPixelColor);
							    // x/3, 765 = 255, 382.5 = 128, 0 = 0
							    InverseBlendFunction.SourceConstantAlpha = (BYTE)(CurrentPixelBrightness / 3);
							    GdiAlphaBlend(ScratchDC, Mouse.x + XPixel, Mouse.y + YPixel, 1, 1, HilightDC, 0, 0, 1, 1, InverseBlendFunction);
							    HilighterPixelsAlreadyDrawn[HilighterPixelsAlreadyDrawnCounter].x = (UINT16)(Mouse.x + XPixel);
							    HilighterPixelsAlreadyDrawn[HilighterPixelsAlreadyDrawnCounter].y = (UINT16)(Mouse.y + YPixel);
							    HilighterPixelsAlreadyDrawnCounter++;
							    if(HilighterPixelsAlreadyDrawnCounter >= _countof(HilighterPixelsAlreadyDrawn) - 1)
								    HilighterPixelsAlreadyDrawnCounter = 0;
						    }
					    }
				    }
				    if(DeleteDC(HilightDC) == 0)
					    MyOutputDebugStringW(L"[%s] Line %d: DeleteDC(HilightDC) failed!\n", __FUNCTIONW__, __LINE__);
				    if(DeleteDC(ScratchDC) == 0)
					    MyOutputDebugStringW(L"[%s] Line %d: DeleteDC(gScratchDC) failed!\n", __FUNCTIONW__, __LINE__);
				    if(DeleteObject(HilightPixel) == 0)
					    MyOutputDebugStringW(L"[%s] Line %d: DeleteObject(HilightPixel) failed!\n", __FUNCTIONW__, __LINE__);
				    PreviousMousePos.x = Mouse.x;
				    RECT SnipRect = { 0 };
				    GetClientRect(Window, &SnipRect);
				    SnipRect.top = MousePosWhenDrawingStarted.y - 10;
				    SnipRect.bottom = MousePosWhenDrawingStarted.y + 10;
				    InvalidateRect(Window, &SnipRect, FALSE);
				    UpdateWindow(SnExG.gMainWindowHandle);
			    }
			    else if(gRectangleButton.SelectedTool == TRUE) {
				    HBITMAP CleanCopy = (HBITMAP)CopyImage(SnExG.gSnipStates[SnExG.gCurrentSnipState - 1], IMAGE_BITMAP, 0, 0, 0);
				    HDC CopyDC = CreateCompatibleDC(NULL);
				    SelectObject(CopyDC, CleanCopy);
				    HPEN Pen = NULL;     //CreatePen(PS_SOLID, 2, RGB(255, 0, 0));
				    switch(gRectangleButton.Color) {
					    case COLOR_RED: Pen = CreatePen(PS_SOLID, 2, RGB(255, 0, 0)); break;
					    case COLOR_GREEN: Pen = CreatePen(PS_SOLID, 2, RGB(0, 255, 0)); break;
					    case COLOR_BLUE: Pen = CreatePen(PS_SOLID, 2, RGB(0, 0, 255)); break;
					    case COLOR_BLACK: Pen = CreatePen(PS_SOLID, 2, RGB(0, 0, 0)); break;
					    case COLOR_WHITE: Pen = CreatePen(PS_SOLID, 2, RGB(255, 255, 255)); break;
					    case COLOR_YELLOW: Pen = CreatePen(PS_SOLID, 2, RGB(255, 255, 0)); break;
					    default: MyOutputDebugStringW(L"[%s] Line %d: BUG: Unknown color for arrow tool!\n", __FUNCTIONW__, __LINE__);
				    }
				    SelectObject(CopyDC, Pen);
				    SelectObject(CopyDC, (HBRUSH)GetStockObject(NULL_BRUSH));
				    POINT CurrentMousePos = { 0 };
				    GetCursorPos(&CurrentMousePos);
				    ScreenToClient(SnExG.gMainWindowHandle, &CurrentMousePos);
				    Rectangle(CopyDC, MousePosWhenDrawingStarted.x, MousePosWhenDrawingStarted.y - 56, CurrentMousePos.x, CurrentMousePos.y - 56);
				    HDC ScratchDC = CreateCompatibleDC(NULL);
				    SelectObject(ScratchDC, SnExG.gScratchBitmap);
				    BitBlt(ScratchDC, 0, 0, SnExG.gCaptureWidth, SnExG.gCaptureHeight, CopyDC, 0, 0, SRCCOPY);
				    // DeleteObject will fail if the object is still selected into a DC.
				    if(DeleteDC(CopyDC) == 0)
					    MyOutputDebugStringW(L"[%s] Line %d: DeleteDC(CopyDC) failed!\n", __FUNCTIONW__, __LINE__);
				    if(DeleteDC(ScratchDC) == 0)
					    MyOutputDebugStringW(L"[%s] Line %d: DeleteDC(gScratchDC) failed!\n", __FUNCTIONW__, __LINE__);
				    if(DeleteObject(Pen) == 0)
					    MyOutputDebugStringW(L"[%s] Line %d: DeleteObject(Pen) failed!\n", __FUNCTIONW__, __LINE__);
				    if(DeleteObject(CleanCopy) == 0)
					    MyOutputDebugStringW(L"[%s] Line %d: DeleteObject(CleanCopy) failed!\n", __FUNCTIONW__, __LINE__);
				    RECT SnipRect = { 0 };
				    GetClientRect(Window, &SnipRect);
				    SnipRect.top = 54;
				    InvalidateRect(Window, &SnipRect, FALSE);
				    UpdateWindow(SnExG.gMainWindowHandle);
			    }
			    else if(gArrowButton.SelectedTool == TRUE) {
				    HBITMAP CleanCopy = (HBITMAP)CopyImage(SnExG.gSnipStates[SnExG.gCurrentSnipState - 1], IMAGE_BITMAP, 0, 0, 0);
				    HDC CopyDC = CreateCompatibleDC(NULL);
				    SelectObject(CopyDC, CleanCopy);
				    HPEN Pen = NULL;
				    HBRUSH Brush = NULL;
					COLORREF _color = RGB(0, 0, 0);
				    switch(gArrowButton.Color) {
					    case COLOR_RED: _color = RGB(255, 0, 0); break;
					    case COLOR_GREEN: _color = RGB(0, 255, 0); break;
					    case COLOR_BLUE: _color = RGB(0, 0, 255); break;
					    case COLOR_BLACK: _color = RGB(0, 0, 0); break;
					    case COLOR_WHITE: _color = RGB(255, 255, 255); break;
					    case COLOR_YELLOW: _color = RGB(255, 255, 0); break;
					    default: MyOutputDebugStringW(L"[%s] Line %d: BUG: Unknown color for arrow tool!\n", __FUNCTIONW__, __LINE__);
				    }
					Pen = CreatePen(PS_SOLID, 2, _color);
					Brush = CreateSolidBrush(_color);
				    SelectObject(CopyDC, Pen);
				    SelectObject(CopyDC, Brush);
				    POINT CurrentMousePos = { 0 };
				    GetCursorPos(&CurrentMousePos);
				    ScreenToClient(SnExG.gMainWindowHandle, &CurrentMousePos);
				    POINT p0 = { 0 };
				    POINT p1 = { 0 };
				    p0.x = MousePosWhenDrawingStarted.x;
				    p0.y = MousePosWhenDrawingStarted.y - 56;
				    p1.x = CurrentMousePos.x;
				    p1.y = CurrentMousePos.y - 56;
				    MoveToEx(CopyDC, p0.x, p0.y, NULL);
				    LineTo(CopyDC, p1.x, p1.y);
				    const float dx = (float)(p1.x - p0.x);
				    const float dy = (float)(p1.y - p0.y);
				    const float ArrowLength = (float)sqrt((double)dx * (double)dx + (double)dy * (double)dy);
				    // unit vector parallel to the line.
				    const float ux = dx / ArrowLength;
				    const float uy = dy / ArrowLength;
				    // unit vector perpendicular to ux,uy
				    const float vx = -uy;
				    const float vy = ux;
				    const int head_length = 10;
				    const int head_width  = 10;
				    const float half_width = 0.5f * head_width;
				    const POINT ArrowHead = p1;
				    POINT ArrowCorner1 = { 0 };
				    ArrowCorner1.x = (LONG)roundf(p1.x - head_length*ux + half_width*vx);
				    ArrowCorner1.y = (LONG)roundf(p1.y - head_length*uy + half_width*vy);
				    POINT ArrowCorner2 = { 0 };
				    ArrowCorner2.x = (LONG)roundf(p1.x - head_length*ux - half_width*vx);
				    ArrowCorner2.y = (LONG)roundf(p1.y - head_length*uy - half_width*vy);
				    POINT Arrow[3];
				    Arrow[0] = ArrowHead;
				    Arrow[1] = ArrowCorner1;
				    Arrow[2] = ArrowCorner2;
				    Polygon(CopyDC, Arrow, 3);
				    HDC ScratchDC = CreateCompatibleDC(NULL);
				    SelectObject(ScratchDC, SnExG.gScratchBitmap);
				    BitBlt(ScratchDC, 0, 0, SnExG.gCaptureWidth, SnExG.gCaptureHeight, CopyDC, 0, 0, SRCCOPY);
				    // DeleteObject will fail if the object is still selected into a DC.
				    if(DeleteDC(CopyDC) == 0)
					    MyOutputDebugStringW(L"[%s] Line %d: DeleteDC(CopyDC) failed!\n", __FUNCTIONW__, __LINE__);
				    if(DeleteDC(ScratchDC) == 0)
					    MyOutputDebugStringW(L"[%s] Line %d: DeleteDC(gScratchDC) failed!\n", __FUNCTIONW__, __LINE__);
				    if(DeleteObject(Pen) == 0)
					    MyOutputDebugStringW(L"[%s] Line %d: DeleteObject(RedPen) failed!\n", __FUNCTIONW__, __LINE__);
				    if(DeleteObject(Brush) == 0)
					    MyOutputDebugStringW(L"[%s] Line %d: DeleteObject(RedBrush) failed!\n", __FUNCTIONW__, __LINE__);
				    if(DeleteObject(CleanCopy) == 0)
					    MyOutputDebugStringW(L"[%s] Line %d: DeleteObject(CleanCopy) failed!\n", __FUNCTIONW__, __LINE__);
				    RECT SnipRect = { 0 };
				    GetClientRect(Window, &SnipRect);
				    SnipRect.top = 54;
				    InvalidateRect(Window, &SnipRect, FALSE);
				    UpdateWindow(SnExG.gMainWindowHandle);
			    }
			    else if(gRedactButton.SelectedTool == TRUE) {
				    POINT Mouse = { 0 };
				    GetCursorPos(&Mouse);
				    ScreenToClient(SnExG.gMainWindowHandle, &Mouse);
				    // Adjust for snip area, maintain Y axis
				    Mouse.x -= 7;
				    Mouse.y = MousePosWhenDrawingStarted.y - 66;
				    HDC ScratchDC = CreateCompatibleDC(NULL);
				    SelectObject(ScratchDC, SnExG.gScratchBitmap);
				    for(UINT8 XPixel = 0; XPixel < 10; XPixel++) {
					    for(UINT8 YPixel = 0; YPixel < 20; YPixel++) {
						    PatBlt(ScratchDC, Mouse.x + XPixel, Mouse.y + YPixel, 1, 1, BLACKNESS);
					    }
				    }
				    if(DeleteDC(ScratchDC) == 0) {
					    MyOutputDebugStringW(L"[%s] Line %d: DeleteDC(gScratchDC) failed!\n", __FUNCTIONW__, __LINE__);
				    }
				    PreviousMousePos.x = Mouse.x;
				    RECT SnipRect = { 0 };
				    GetClientRect(Window, &SnipRect);
				    SnipRect.top = MousePosWhenDrawingStarted.y - 10;
				    SnipRect.bottom = MousePosWhenDrawingStarted.y + 10;
				    InvalidateRect(Window, &SnipRect, FALSE);
				    UpdateWindow(SnExG.gMainWindowHandle);
			    }
		    }
		    // We only receive this message when the mouse is not over a button. So if we get this message, no
		    // button should be pressed.
		    // Unless it's a selected tool which should stay pressed.
		    for(UINT8 Counter = 0; Counter < _countof(gButtons); Counter++) {
			    if(gButtons[Counter]->State != BUTTONSTATE_NORMAL && gButtons[Counter]->SelectedTool == FALSE) {
				    gButtons[Counter]->State = BUTTONSTATE_NORMAL;
				    InvalidateRect(Window, NULL, FALSE);
			    }
			    if(gButtons[Counter]->SelectedTool == TRUE && SnExG.gAppState == APPSTATE_AFTERCAPTURE && SnExG.gSnipStates[0] && gButtons[Counter]->Cursor) {
				    POINT Mouse = { 0 };
				    GetCursorPos(&Mouse);
				    ScreenToClient(SnExG.gMainWindowHandle, &Mouse);
					{
						HCURSOR local_hcur = (Mouse.x >= 2 && Mouse.y >= 54) ? gButtons[Counter]->Cursor : LoadCursorW(NULL, IDC_ARROW);
						if(GetCursor() != local_hcur)
							SetCursor(local_hcur);
					}
			    }
		    }
		    break;
	    }
		case WM_PARENTNOTIFY:
	    {
		    switch(LOWORD(WParam)) {
			    case WM_LBUTTONDOWN:
			    case WM_RBUTTONDOWN:
				{
					POINT Mouse = { 0 };
					Mouse.x = GET_X_LPARAM(LParam);
					Mouse.y = GET_Y_LPARAM(LParam);
					const HWND Control = ChildWindowFromPoint(Window, Mouse);
					for(UINT8 Counter = 0; Counter < _countof(gButtons); Counter++) {
						BUTTON * p_button = gButtons[Counter];
						if(Control == p_button->Handle) {
							if(LOWORD(WParam) == WM_LBUTTONDOWN) {
								MyOutputDebugStringW(L"[%s] Line %d: Left mouse button pressed over '%s' button.\n", __FUNCTIONW__, __LINE__, p_button->Caption);
							}
							else {
								MyOutputDebugStringW(L"[%s] Line %d: Right mouse button pressed over '%s' button.\n", __FUNCTIONW__, __LINE__, p_button->Caption);
							}
							if(p_button->Enabled == TRUE) {
								if(LOWORD(WParam) == WM_LBUTTONDOWN) {
									p_button->State = BUTTONSTATE_PRESSED;
								}
								else {
									if(p_button->Id == BUTTON_HILIGHT) {
										switch(p_button->Color) {
											case COLOR_YELLOW: p_button->Set(COLOR_PINK, IDB_PINKHILIGHT32x32, IDC_PINKHILIGHTCURSOR); break;
											case COLOR_PINK: p_button->Set(COLOR_ORANGE, IDB_ORANGEHILIGHT32x32, IDC_ORANGEHILIGHTCURSOR); break;
											case COLOR_ORANGE: p_button->Set(COLOR_GREEN, IDB_GREENHILIGHT32x32, IDC_GREENHILIGHTCURSOR); break;
											case COLOR_GREEN: p_button->Set(COLOR_YELLOW, IDB_YELLOWHILIGHT32x32, IDC_YELLOWHILIGHTCURSOR); break;
											default: MyOutputDebugStringW(L"[%s] Line %d: BUG: Unknown color when trying to change hilighter color!\n", __FUNCTIONW__, __LINE__);
										}
										DeleteObject(p_button->EnabledIcon);
										DeleteObject(p_button->Cursor);
										p_button->EnabledIcon = (HBITMAP)LoadImageW(GetModuleHandleW(NULL), MAKEINTRESOURCEW(p_button->EnabledIconId),
											IMAGE_BITMAP, 0, 0, 0);
										p_button->Cursor = LoadCursorW(GetModuleHandleW(NULL), MAKEINTRESOURCEW(p_button->CursorId));
									}
									else if(p_button->Id == BUTTON_BOX) {
										switch(p_button->Color) {
											case COLOR_RED: p_button->Set(COLOR_GREEN, IDB_BOX32x32GREEN, IDC_GREENCROSSHAIR); break;
											case COLOR_GREEN: p_button->Set(COLOR_BLUE, IDB_BOX32x32BLUE, IDC_BLUECROSSHAIR); break;
											case COLOR_BLUE: p_button->Set(COLOR_BLACK, IDB_BOX32x32BLACK, IDC_BLACKCROSSHAIR); break;
											case COLOR_BLACK: p_button->Set(COLOR_WHITE, IDB_BOX32x32WHITE, IDC_WHITECROSSHAIR); break;
											case COLOR_WHITE: p_button->Set(COLOR_YELLOW, IDB_BOX32x32YELLOW, IDC_YELLOWCROSSHAIR); break;
											case COLOR_YELLOW: p_button->Set(COLOR_RED, IDB_BOX32x32RED, IDC_REDCROSSHAIR); break;
											default: MyOutputDebugStringW(L"[%s] Line %d: BUG: Unknown color when trying to change box color!\n", __FUNCTIONW__, __LINE__);
										}
										DeleteObject(p_button->EnabledIcon);
										DeleteObject(p_button->Cursor);
										p_button->EnabledIcon = (HBITMAP)LoadImageW(GetModuleHandleW(NULL), MAKEINTRESOURCEW(p_button->EnabledIconId),
											IMAGE_BITMAP, 0, 0, 0);
										p_button->Cursor = LoadCursorW(GetModuleHandleW(NULL), MAKEINTRESOURCEW(p_button->CursorId));
									}
									else if(p_button->Id == BUTTON_ARROW) {
										switch(p_button->Color) {
											case COLOR_RED: p_button->Set(COLOR_GREEN, IDB_ARROW32x32GREEN, IDC_GREENCROSSHAIR); break;
											case COLOR_GREEN: p_button->Set(COLOR_BLUE, IDB_ARROW32x32BLUE, IDC_BLUECROSSHAIR); break;
											case COLOR_BLUE: p_button->Set(COLOR_BLACK, IDB_ARROW32x32BLACK, IDC_BLACKCROSSHAIR); break;
											case COLOR_BLACK: p_button->Set(COLOR_WHITE, IDB_ARROW32x32WHITE, IDC_WHITECROSSHAIR); break;
											case COLOR_WHITE: p_button->Set(COLOR_YELLOW, IDB_ARROW32x32YELLOW, IDC_YELLOWCROSSHAIR); break;
											case COLOR_YELLOW: p_button->Set(COLOR_RED, IDB_ARROW32x32RED, IDC_REDCROSSHAIR); break;
											default: MyOutputDebugStringW(L"[%s] Line %d: BUG: Unknown color when trying to change arrow color!\n", __FUNCTIONW__, __LINE__);
										}
										DeleteObject(p_button->EnabledIcon);
										DeleteObject(p_button->Cursor);
										p_button->EnabledIcon = (HBITMAP)LoadImageW(GetModuleHandleW(NULL), MAKEINTRESOURCEW(p_button->EnabledIconId),
											IMAGE_BITMAP, 0, 0, 0);
										p_button->Cursor = LoadCursorW(GetModuleHandleW(NULL), MAKEINTRESOURCEW(p_button->CursorId));
									}
									else if(p_button->Id == BUTTON_TEXT) {
										CHOOSEFONTW FontChoice = { sizeof(CHOOSEFONTW) };
										LOGFONTW LogFont = { 0 };
										GetObject(SnExG.gFont, sizeof(LOGFONTW), &LogFont);
										FontChoice.Flags = CF_EFFECTS | CF_INITTOLOGFONTSTRUCT | CF_SCREENFONTS;
										FontChoice.hwndOwner = SnExG.gMainWindowHandle;
										FontChoice.lpLogFont = &LogFont;
										FontChoice.rgbColors = RGB(0, 0, 0);
										if(ChooseFont(&FontChoice)) {
											MyOutputDebugStringW(L"[%s] Line %d: ChooseFont successful.\n", __FUNCTIONW__, __LINE__);
											HFONT TmpFont = CreateFontIndirectW(&LogFont);
											if(TmpFont) {
												DeleteObject(SnExG.gFont);
												SnExG.gFont = TmpFont;
												SnExG.gFontColor = FontChoice.rgbColors;
											}
										}
									}
								}
							}
							else
								MyOutputDebugStringW(L"[%s] Line %d: ...but it was disabled.\n", __FUNCTIONW__, __LINE__);
						}
						else if(p_button->SelectedTool == FALSE)
							p_button->State = BUTTONSTATE_NORMAL;
					}
					// Redraw the toolbar.
					RECT ToolbarRect = { 0 };
					GetClientRect(SnExG.gMainWindowHandle, &ToolbarRect);
					ToolbarRect.bottom = gButtons[0]->Rectangle.bottom;
					InvalidateRect(SnExG.gMainWindowHandle, &ToolbarRect, FALSE);
					break;
				}
		    }
		    break;
	    }
		case WM_DRAWITEM:
	    {
		    for(UINT8 Counter = 0; Counter < _countof(gButtons); Counter++) {
			    if(LOWORD(WParam) == gButtons[Counter]->Id) {
				    DrawButton((DRAWITEMSTRUCT*)LParam, *gButtons[Counter]);
				    return TRUE;
			    }
		    }
		    break;
	    }
		case WM_COMMAND:
	    {
		    MyOutputDebugStringW(L"[%s] Line %d: Mouse button released over '%s' button.\n", __FUNCTIONW__, __LINE__,
			gButtons[LOWORD(WParam) - 10001]->Caption);
		    // User clicked a button - return all buttons to non-pressed state.
		    for(UINT8 Counter = 0; Counter < _countof(gButtons); Counter++) {
			    if(gButtons[Counter]->SelectedTool == FALSE) {
				    gButtons[Counter]->State = BUTTONSTATE_NORMAL;
			    }
		    }
		    // Redraw the toolbar.
		    RECT ToolbarRect = { 0 };
		    GetClientRect(SnExG.gMainWindowHandle, &ToolbarRect);
		    ToolbarRect.bottom = gButtons[0]->Rectangle.bottom;
		    InvalidateRect(SnExG.gMainWindowHandle, &ToolbarRect, FALSE);
		    // We just left-clicked a button, but we need to set focus back on the main window or else
		    // the main window will stop receiving window messages such as WM_KEYDOWN.
		    SetFocus(SnExG.gMainWindowHandle);
		    for(UINT8 Counter = 0; Counter < _countof(gButtons); Counter++) {
				BUTTON * p_button = gButtons[Counter];
			    if(p_button->Id == LOWORD(WParam)) {
				    if(p_button->Enabled == TRUE) {
					    p_button->SelectedTool = TRUE;
					    p_button->State = BUTTONSTATE_PRESSED;
				    }
			    }
			    else {
				    if(p_button->SelectedTool == TRUE) {
					    p_button->SelectedTool = FALSE;
					    p_button->State = BUTTONSTATE_NORMAL;
				    }
			    }
		    }
		    switch(LOWORD(WParam)) {
			    case BUTTON_NEW:
					memzero(HilighterPixelsAlreadyDrawn, sizeof(HilighterPixelsAlreadyDrawn));
					HilighterPixelsAlreadyDrawnCounter = 0;
					if(NewButton_Click() == FALSE) {
						SnExG.gMainWindowIsRunning = FALSE;
						CRASH(0);
					}
					break;
			    case BUTTON_DELAY:
					{
						wchar_t TitleBuffer[64] = { 0 };
						_snwprintf_s(TitleBuffer, _countof(TitleBuffer), _TRUNCATE, L"SnipEx");
						SetWindowTextW(SnExG.gMainWindowHandle, TitleBuffer);
						SnExG.gAppState = APPSTATE_DELAYCOOKING;
						SnExG.AdjustWindowSizeForThickTitleBars(); // @20201025
						/* @20201025
						RECT CurrentWindowPos = { 0 };
						GetWindowRect(gMainWindowHandle, &CurrentWindowPos);
						SetWindowPos(gMainWindowHandle, HWND_TOP, CurrentWindowPos.left, CurrentWindowPos.top, gStartingMainWindowWidth, gStartingMainWindowHeight, 0);
						*/
						KillTimer(SnExG.gMainWindowHandle, DELAY_TIMER);
						SetTimer(SnExG.gMainWindowHandle, DELAY_TIMER, 1000, NULL);
					}
					break;
			    case BUTTON_SAVE:
					if(gSaveButton.Enabled == TRUE) {
						if(SaveButton_Click() == FALSE) {
							MyOutputDebugStringW(L"[%s] Line %d: SaveButton_Click either failed or was cancelled by the user.\n", __FUNCTIONW__, __LINE__);
						}
					}
					break;
			    case BUTTON_COPY:
					if(gCopyButton.Enabled == TRUE) {
						if(CopyButton_Click() == FALSE) {
							MyOutputDebugStringW(L"[%s] Line %d: CopyButton_Click failed!\n", __FUNCTIONW__, __LINE__);
						}
					}
					break;
			    case BUTTON_HILIGHT:
			    case BUTTON_BOX:
			    case BUTTON_ARROW:
			    case BUTTON_REDACT:
			    case BUTTON_TEXT:
					if(gButtons[LOWORD(WParam) - 10001]->Enabled == TRUE) {
						SnExG.gLastTool = (DWORD)gButtons[LOWORD(WParam) - 10001]->Id;
						if(SnExG.gRememberLastTool) {
							if(SetSnipExRegValue(REG_LASTTOOLNAME, &SnExG.gLastTool) != ERROR_SUCCESS) {
								CRASH(0);
							}
						}
					}
					break;
			    default:
					MyOutputDebugStringW(L"[%s] Line %d: Unrecognized command.\n", __FUNCTIONW__, __LINE__);
					CRASH(0);
		    }
		    break;
	    }
		case WM_SYSCOMMAND:
	    {
		    // Default system messages are >= 0xf000
		    if(WParam >= 0xF000) {
			    Result = DefWindowProcW(Window, Message, WParam, LParam);
		    }
		    else if(WParam == SYSCMD_REPLACE) {
			    MyOutputDebugStringW(L"[%s] Line %d: User clicked on 'Replace Snipping Tool'.\n", __FUNCTIONW__, __LINE__);
			    // Need admin and UAC elevation to write to the image file execution options registry key
			    if(IsAppRunningElevated()) {
				    MyOutputDebugStringW(L"[%s] Line %d: Already UAC elevated.\n", __FUNCTIONW__, __LINE__);
				    HKEY IFEOKey = NULL;
				    HKEY SnippingToolKey = NULL;
				    if(RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options", 0,
						KEY_ALL_ACCESS, &IFEOKey) != ERROR_SUCCESS) {
					    SnExG.PopupMessageErr(L"Error while attempting to open the Image File Execution Options subkey!");
				    }
				    else {
					    DWORD SnippingToolKeyDisposition = 0;
					    DWORD Error = RegCreateKeyExW(IFEOKey, L"SnippingTool.exe", 0, NULL, 0, KEY_WRITE, NULL, &SnippingToolKey, &SnippingToolKeyDisposition);
					    if(Error == ERROR_SUCCESS) {
						    wchar_t ModulePath[MAX_PATH] = { 0 };
						    GetModuleFileNameW(NULL, ModulePath, _countof(ModulePath));
						    Error = RegSetValueExW(SnippingToolKey, L"Debugger", 0, REG_SZ, (const BYTE*)ModulePath, (DWORD)wcslen(ModulePath) * 2);
						    if(Error == ERROR_SUCCESS) {
							    HMENU SystemMenu = GetSystemMenu(SnExG.gMainWindowHandle, FALSE);
							    RemoveMenu(SystemMenu, SYSCMD_REPLACE, MF_BYCOMMAND);
							    AppendMenuW(SystemMenu, MF_STRING, SYSCMD_RESTORE, L"Restore Windows Snipping Tool\n");
							    if(SnExG.gUACIcon) {
								    MENUITEMINFOW MenuItemInfo = { sizeof(MENUITEMINFOW) };
								    MenuItemInfo.fMask = MIIM_BITMAP;
								    MenuItemInfo.hbmpItem = SnExG.gUACIcon;
								    SetMenuItemInfoW(SystemMenu, SYSCMD_RESTORE, FALSE, &MenuItemInfo);
							    }
							    MessageBoxW(SnExG.gMainWindowHandle, L"The built-in SnippingTool.exe has been replaced.", L"Success", MB_OK | MB_ICONINFORMATION | MB_SYSTEMMODAL);
						    }
						    else
							    SnExG.PopupMessageErr(L"Error while creating Debugger registry value!");
					    }
					    else
						    SnExG.PopupMessageErr(L"Error while creating SnippingTool.exe registry key!");
				    }
				    ::RegCloseKey(SnippingToolKey);
				    ::RegCloseKey(IFEOKey);
			    }
			    else {
				    MyOutputDebugStringW(L"[%s] Line %d: Need to UAC elevate first.\n", __FUNCTIONW__, __LINE__);
				    MessageBoxW(SnExG.gMainWindowHandle, L"This action requires UAC elevation. After clicking OK, you will be prompted to elevate. Then retry the operation.",
						L"UAC Elevation Required", MB_OK | MB_ICONINFORMATION);
				    wchar_t ModulePath[MAX_PATH] = { 0 };
				    GetModuleFileNameW(NULL, ModulePath, _countof(ModulePath));
				    SHELLEXECUTEINFOW ShellExecuteInfo = { sizeof(SHELLEXECUTEINFOW) };
				    ShellExecuteInfo.lpVerb = L"runas";
				    ShellExecuteInfo.lpFile = ModulePath;
				    ShellExecuteInfo.hwnd = NULL;
				    ShellExecuteInfo.nShow = SW_NORMAL;
				    if(!ShellExecuteExW(&ShellExecuteInfo)) {
					    if(GetLastError() != ERROR_CANCELLED) {
						    SnExG.PopupMessageErr(L"Error when attempting to re-launch the application with UAC elevation.");
					    }
				    }
				    else {
					    // We just re-launched the app with UAC elevation - need to exit this
					    // instance.
					    PostQuitMessage(0);
					    SnExG.gMainWindowIsRunning = FALSE;
				    }
			    }
		    }
		    else if(WParam == SYSCMD_RESTORE) {
			    MyOutputDebugStringW(L"[%s] Line %d: User clicked on 'Restore Snipping Tool'.\n", __FUNCTIONW__, __LINE__);
			    // To "restore" the built-in snipping tool, all we have to do is delete the snippingtool.exe
			    // Image File Execution Options registry key.
			    if(IsAppRunningElevated()) {
				    MyOutputDebugStringW(L"[%s] Line %d: Already UAC elevated.\n", __FUNCTIONW__, __LINE__);
				    HKEY IFEOKey = NULL;
				    if(RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options", 0, KEY_QUERY_VALUE, &IFEOKey) != ERROR_SUCCESS) {
					    SnExG.PopupMessageErr(L"Error while attempting to open the Image File Execution Options subkey!");
				    }
				    else {
					    // This will fail if the subkey has subkeys
					    DWORD Error = RegDeleteKeyW(IFEOKey, L"SnippingTool.exe");
					    if(Error != ERROR_SUCCESS) {
						    SnExG.PopupMessageErr(L"Error while attempting to delete the SnippingTool.exe subkey!");
					    }
					    else {
						    HMENU SystemMenu = GetSystemMenu(SnExG.gMainWindowHandle, FALSE);
						    RemoveMenu(SystemMenu, SYSCMD_RESTORE, MF_BYCOMMAND);
						    AppendMenuW(SystemMenu, MF_STRING, SYSCMD_REPLACE, L"Replace Windows Snipping Tool with SnipEx\n");
						    if(SnExG.gUACIcon) {
							    MENUITEMINFOW MenuItemInfo = { sizeof(MENUITEMINFOW) };
							    MenuItemInfo.fMask = MIIM_BITMAP;
							    MenuItemInfo.hbmpItem = SnExG.gUACIcon;
							    SetMenuItemInfoW(SystemMenu, SYSCMD_REPLACE, FALSE, &MenuItemInfo);
						    }
						    MessageBoxW(SnExG.gMainWindowHandle, L"The built-in SnippingTool.exe has been restored.", L"Success", MB_OK | MB_ICONINFORMATION | MB_SYSTEMMODAL);
					    }
				    }
					::RegCloseKey(IFEOKey);
			    }
			    else {
				    MyOutputDebugStringW(L"[%s] Line %d: Need to UAC elevate first.\n", __FUNCTIONW__, __LINE__);
				    MessageBoxW(SnExG.gMainWindowHandle, L"This action requires UAC elevation. After clicking OK, you will be prompted to elevate. Then, retry the operation.",
						L"UAC Elevation Required", MB_OK | MB_ICONINFORMATION);
				    wchar_t ModulePath[MAX_PATH] = { 0 };
				    GetModuleFileNameW(NULL, ModulePath, _countof(ModulePath));
				    SHELLEXECUTEINFOW ShellExecuteInfo = { sizeof(SHELLEXECUTEINFOW) };
				    ShellExecuteInfo.lpVerb = L"runas";
				    ShellExecuteInfo.lpFile = ModulePath;
				    ShellExecuteInfo.hwnd   = NULL;
				    ShellExecuteInfo.nShow  = SW_NORMAL;
				    if(!ShellExecuteExW(&ShellExecuteInfo)) {
					    const DWORD Error = GetLastError();
					    if(Error != ERROR_CANCELLED) {
						    SnExG.PopupMessageErr(L"Error when attempting to re-launch the application with UAC elevation.");
					    }
				    }
				    else {
					    // We just re-launched the app with UAC elevation - need to exit this instance.
					    PostQuitMessage(0);
					    SnExG.gMainWindowIsRunning = FALSE;
				    }
			    }
		    }
		    else if(WParam == SYSCMD_SHADOW) {
			    MyOutputDebugStringW(L"[%s] Line %d: User clicked on 'Drop Shadow' menu item.\n", __FUNCTIONW__, __LINE__);
			    if(SnExG.gShouldAddDropShadow) {
				    CheckMenuItem(GetSystemMenu(SnExG.gMainWindowHandle, FALSE), SYSCMD_SHADOW, MF_BYCOMMAND | MF_UNCHECKED);
				    SnExG.gShouldAddDropShadow = FALSE;
			    }
			    else {
				    CheckMenuItem(GetSystemMenu(SnExG.gMainWindowHandle, FALSE), SYSCMD_SHADOW, MF_BYCOMMAND | MF_CHECKED);
				    SnExG.gShouldAddDropShadow = TRUE;
			    }
			    if(SetSnipExRegValue(REG_DROPSHADOWNAME, &SnExG.gShouldAddDropShadow) != ERROR_SUCCESS) {
				    CRASH(0);
			    }
		    }
		    else if(WParam == SYSCMD_REMEMBER) {
			    MyOutputDebugStringW(L"[%s] Line %d: User clicked on 'Remember Last Tool' menu item.\n", __FUNCTIONW__, __LINE__);
			    if(SnExG.gRememberLastTool) {
				    CheckMenuItem(GetSystemMenu(SnExG.gMainWindowHandle, FALSE), SYSCMD_REMEMBER, MF_BYCOMMAND | MF_UNCHECKED);
				    SnExG.gRememberLastTool = FALSE;
			    }
			    else {
				    CheckMenuItem(GetSystemMenu(SnExG.gMainWindowHandle, FALSE), SYSCMD_REMEMBER, MF_BYCOMMAND | MF_CHECKED);
				    SnExG.gRememberLastTool = TRUE;
				    if(SnExG.gLastTool) {
					    if(SetSnipExRegValue(REG_LASTTOOLNAME, &SnExG.gLastTool) != ERROR_SUCCESS) {
						    CRASH(0);
					    }
				    }
			    }
			    if(SetSnipExRegValue(REG_REMEMBERTOOLNAME, &SnExG.gRememberLastTool) != ERROR_SUCCESS) {
				    CRASH(0);
			    }
		    }
		    else if(WParam == SYSCMD_AUTOCOPY) {
			    MyOutputDebugStringW(L"[%s] Line %d: User clicked on 'Automatically copy snip to clipboard' menu item.\n", __FUNCTIONW__, __LINE__);
			    if(SnExG.gAutoCopy) {
				    CheckMenuItem(GetSystemMenu(SnExG.gMainWindowHandle, FALSE), SYSCMD_AUTOCOPY, MF_BYCOMMAND | MF_UNCHECKED);
				    SnExG.gAutoCopy = FALSE;
			    }
			    else {
				    CheckMenuItem(GetSystemMenu(SnExG.gMainWindowHandle, FALSE), SYSCMD_AUTOCOPY, MF_BYCOMMAND | MF_CHECKED);
				    SnExG.gAutoCopy = TRUE;
			    }
			    if(SetSnipExRegValue(REG_AUTOCOPYNAME, &SnExG.gAutoCopy) != ERROR_SUCCESS) {
				    CRASH(0);
			    }
		    }
		    else if(WParam == SYSCMD_UNDO) {
			    MyOutputDebugStringW(L"[%s] Line %d: User clicked on 'Undo' menu item.\n", __FUNCTIONW__, __LINE__);
			    if(SnExG.gCurrentSnipState > 0 && (SnExG.gAppState == APPSTATE_AFTERCAPTURE)) {
				    SnExG.gCurrentSnipState--;
				    MyOutputDebugStringW(L"[%s] Line %d: Deleting gSnipStates[gCurrentSnipState + 1]\n", __FUNCTIONW__, __LINE__);
				    memzero(HilighterPixelsAlreadyDrawn, sizeof(HilighterPixelsAlreadyDrawn));
				    HilighterPixelsAlreadyDrawnCounter = 0;
				    if(DeleteObject(SnExG.gSnipStates[SnExG.gCurrentSnipState + 1]) == 0) {
					    MyOutputDebugStringW(L"[%s] Line %d: DeleteObject failed!\n", __FUNCTIONW__, __LINE__);
					    CRASH(0);
				    }
				    SnExG.gSnipStates[SnExG.gCurrentSnipState + 1] = NULL;
				    InvalidateRect(Window, NULL, FALSE);
				    if(SnExG.gAutoCopy) {
					    MyOutputDebugStringW(L"[%s] Line %d: Auto copy enabled. Copying snip to clipboard.\n", __FUNCTIONW__, __LINE__);
					    if(CopyButton_Click() == FALSE) {
						    MyOutputDebugStringW(L"[%s] Line %d: Auto copy failed!\n", __FUNCTIONW__, __LINE__);
						    CRASH(0);
					    }
				    }
			    }
		    }

		    break;
	    }
		case WM_TIMER:
		    if(WParam == DELAY_TIMER) {
			    SnExG.gCurrentDelayCountdown--;
			    MyOutputDebugStringW(L"[%s] Line %d: WM_TIMER DELAY_TIMER received. %d seconds.\n", __FUNCTIONW__, __LINE__, SnExG.gCurrentDelayCountdown);
			    InvalidateRect(SnExG.gMainWindowHandle, NULL, FALSE);
			    if(SnExG.gCurrentDelayCountdown <= 0)
				    SendMessageW(SnExG.gMainWindowHandle, WM_COMMAND, BUTTON_NEW, 0);
			    else
				    SetTimer(SnExG.gMainWindowHandle, DELAY_TIMER, 1000, NULL);
		    }
		    break;
		case WM_PAINT:
	    {
		    PAINTSTRUCT PaintStruct = { 0 };
		    BeginPaint(Window, &PaintStruct);
		    if(SnExG.gAppState == APPSTATE_AFTERCAPTURE) {
			    HDC MemDC = CreateCompatibleDC(PaintStruct.hdc);
			    if(CurrentlyDrawing == TRUE) {
				    if(SnExG.gScratchBitmap)
					    SelectObject(MemDC, SnExG.gScratchBitmap);
			    }
			    else {
				    if(SnExG.GetCurrentSnipState())
					    SelectObject(MemDC, SnExG.GetCurrentSnipState());
			    }
			    BitBlt(PaintStruct.hdc, 2, 56, SnExG.gCaptureWidth, SnExG.gCaptureHeight, MemDC, 0, 0, SRCCOPY);
			    DeleteDC(MemDC);
		    }
		    EndPaint(Window, &PaintStruct);
		    break;
	    }
		default:
		    Result = DefWindowProcW(Window, Message, WParam, LParam);
	}
	return Result;
}

void DrawButton(_In_ DRAWITEMSTRUCT* DrawItemStruct, _In_ BUTTON Button)
{
	HPEN DarkPen = CreatePen(PS_SOLID, 1, RGB(32, 32, 32));
	HPEN LighterPen = CreatePen(PS_SOLID, 1, RGB(224, 224, 224));
	SIZE StringSizeInPixels = { 0 };
	const UINT16 ButtonWidth = (UINT16)(Button.Rectangle.right - Button.Rectangle.left);
	HFONT ButtonFont = NULL;
	ButtonFont = CreateFontW(-12, 0, 0, 0, FW_NORMAL, 0, 0, 0, ANSI_CHARSET,
		OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, L"Segoe UI");
	SelectObject(DrawItemStruct->hDC, ButtonFont);
	SetDCBrushColor(DrawItemStruct->hDC, GetSysColor(COLOR_BTNFACE));
	SelectObject(DrawItemStruct->hDC, GetStockObject(DC_BRUSH));
	GetTextExtentPoint32W(DrawItemStruct->hDC, Button.Caption, (int)wcslen(Button.Caption), &StringSizeInPixels);
	const UINT16 TextX = (UINT16)((ButtonWidth / 2) - (StringSizeInPixels.cx / 2));
	UINT16 TextY = 34;
	if(Button.State == BUTTONSTATE_PRESSED) {
		TextY += 1;
		SelectObject(DrawItemStruct->hDC, LighterPen);
	}
	else {
		SelectObject(DrawItemStruct->hDC, DarkPen);
	}
	MoveToEx(DrawItemStruct->hDC, 0, Button.Rectangle.bottom - 1, NULL);
	LineTo(DrawItemStruct->hDC, ButtonWidth - 1, Button.Rectangle.bottom - 1);
	LineTo(DrawItemStruct->hDC, ButtonWidth - 1, 0);
	SelectObject(DrawItemStruct->hDC, (Button.State == BUTTONSTATE_PRESSED) ? DarkPen : LighterPen);
	MoveToEx(DrawItemStruct->hDC, ButtonWidth - 2, Button.Rectangle.top, NULL);
	LineTo(DrawItemStruct->hDC, 0, Button.Rectangle.top);
	LineTo(DrawItemStruct->hDC, 0, Button.Rectangle.bottom);
	if(Button.Enabled == TRUE) {
		SetTextColor(DrawItemStruct->hDC, RGB(0, 0, 0));
		SelectObject(DrawItemStruct->hDC, DarkPen);
	}
	else {
		SetTextColor(DrawItemStruct->hDC, RGB(160, 160, 160));
		SelectObject(DrawItemStruct->hDC, LighterPen);
	}
	TextOutW(DrawItemStruct->hDC, TextX, TextY, Button.Caption, (int)wcslen(Button.Caption));
	MoveToEx(DrawItemStruct->hDC, TextX + 6, TextY + 13, NULL);
	LineTo(DrawItemStruct->hDC, TextX, TextY + 13);
	if(!DeleteObject(ButtonFont)) {
		MyOutputDebugStringW(L"[%s] Line %d: DeleteObject(ButtonFont) failed!\n", __FUNCTIONW__, __LINE__);
	}
	if(!DeleteObject(LighterPen)) {
		MyOutputDebugStringW(L"[%s] Line %d: DeleteObject(LighterPen) failed!\n", __FUNCTIONW__, __LINE__);
	}
	if(!DeleteObject(DarkPen)) {
		MyOutputDebugStringW(L"[%s] Line %d: DeleteObject(DarkPen) failed!\n", __FUNCTIONW__, __LINE__);
	}
	if(SnExG.gAppState == APPSTATE_DELAYCOOKING && Button.Id == BUTTON_DELAY) {
		wchar_t CountdownBuffer[4] = { 0 };
		_itow_s(SnExG.gCurrentDelayCountdown, CountdownBuffer, _countof(CountdownBuffer), 10);
		TextOutW(DrawItemStruct->hDC, TextX + StringSizeInPixels.cx + 4, TextY, CountdownBuffer, (int)wcslen(CountdownBuffer));
	}
	if(Button.Enabled == TRUE) {
		if(Button.EnabledIcon != NULL) {
			HDC IconDC = CreateCompatibleDC(DrawItemStruct->hDC);
			SelectObject(IconDC, Button.EnabledIcon);
			TransparentBlt(DrawItemStruct->hDC, 19, TextY - 32, 32, 32, IconDC, 0, 0, 32, 32, RGB(255, 255, 255));
			DeleteDC(IconDC);
		}
	}
	else {
		if(Button.DisabledIcon != NULL) {
			HDC IconDC = CreateCompatibleDC(DrawItemStruct->hDC);
			SelectObject(IconDC, Button.DisabledIcon);
			TransparentBlt(DrawItemStruct->hDC, 19, TextY - 32, 32, 32, IconDC, 0, 0, 32, 32, RGB(255, 255, 255));
			DeleteDC(IconDC);
		}
	}
}

LRESULT CALLBACK CaptureWindowCallback(_In_ HWND Window, _In_ UINT Message, _In_ WPARAM WParam, _In_ LPARAM LParam)
{
	LRESULT Result = 0;
	static BOOL LMouseButtonDown = FALSE;
	static BOOL MouseHasMovedWhileLeftMouseButtonWasDown = FALSE;
	switch(Message) {
		case WM_KEYDOWN:
		    if(WParam == VK_ESCAPE) {
			    MyOutputDebugStringW(L"[%s] Line %d: Escape pressed during capture. Reverting to pre-capture state.\n", __FUNCTIONW__, __LINE__);
			    SnExG.gAppState = APPSTATE_BEFORECAPTURE;
			    for(UINT8 Counter = 0; Counter < _countof(gButtons); Counter++) {
				    if(gButtons[Counter]->Id == BUTTON_NEW || gButtons[Counter]->Id == BUTTON_DELAY) {
					    continue;
				    }
				    gButtons[Counter]->Enabled = FALSE;
			    }
			    ShowWindow(SnExG.gCaptureWindowHandle, SW_HIDE);
			    ShowWindow(SnExG.gMainWindowHandle, SW_RESTORE);
			    MEMSZERO(SnExG.gCaptureSelectionRectangle);
				SnExG.AdjustWindowSizeForThickTitleBars(); // @20201025
				/* @20201025
			    RECT CurrentWindowPos = { 0 };
			    GetWindowRect(gMainWindowHandle, &CurrentWindowPos);
			    SetWindowPos(gMainWindowHandle, HWND_TOP, CurrentWindowPos.left, CurrentWindowPos.top, gStartingMainWindowWidth, gStartingMainWindowHeight, 0);
				*/
			    gNewButton.State = BUTTONSTATE_NORMAL;
			    gNewButton.SelectedTool = FALSE;
		    }
		    break;
		case WM_PAINT:
		    if(SnExG.gCleanScreenShot) {
			    PAINTSTRUCT PaintStruct = { 0 };
			    BeginPaint(Window, &PaintStruct);
			    HDC BackBufferDC = CreateCompatibleDC(PaintStruct.hdc);
			    HBITMAP ScreenShotCopy = (HBITMAP)CopyImage(SnExG.gCleanScreenShot, IMAGE_BITMAP, 0, 0, 0);
			    SelectObject(BackBufferDC, ScreenShotCopy);
			    if(MouseHasMovedWhileLeftMouseButtonWasDown) {
				    SelectObject(BackBufferDC, (HBRUSH)GetStockObject(NULL_BRUSH));
				    Rectangle(BackBufferDC, SnExG.gCaptureSelectionRectangle.left, SnExG.gCaptureSelectionRectangle.top, 
						SnExG.gCaptureSelectionRectangle.right, SnExG.gCaptureSelectionRectangle.bottom);
			    }
			    BLENDFUNCTION BlendFunction = { AC_SRC_OVER, 0, 128, AC_SRC_ALPHA };
			    HDC AlphaDC = CreateCompatibleDC(PaintStruct.hdc);
			    // B G R A
			    const UCHAR BitmapBits[] = { 0xAA, 0xAA, 0xAA, 0xFF };
			    // Create 1 32-bit pixel.
			    HBITMAP AlphaBitmap = CreateBitmap(1, 1, 1, 32, BitmapBits);
			    SelectObject(AlphaDC, AlphaBitmap);
			    // This will darken the entire display area if nothing is selected.
			    // Otherwise, it will darken everything to the right side of the selection rectangle.
			    GdiAlphaBlend(BackBufferDC, MAX(SnExG.gCaptureSelectionRectangle.left, SnExG.gCaptureSelectionRectangle.right), 0,
				    SnExG.gDisplayWidth - MAX(SnExG.gCaptureSelectionRectangle.left, SnExG.gCaptureSelectionRectangle.right),
				    MAX(SnExG.gDisplayHeight, SnExG.gCaptureSelectionRectangle.top),
				    AlphaDC, 0, 0, 1, 1, BlendFunction);

			    // This darkens everything to the left side of the selection rectangle
			    // Not needed if the user is not holding down the left mouse button
			    if(LMouseButtonDown) {
				    GdiAlphaBlend(BackBufferDC, 0, 0, MIN(SnExG.gCaptureSelectionRectangle.left, SnExG.gCaptureSelectionRectangle.right),
					    MAX(SnExG.gDisplayHeight, SnExG.gCaptureSelectionRectangle.top), AlphaDC, 0, 0, 1, 1, BlendFunction);
			    }
			    // This darkens everything directly above the selection rectangle
			    if(LMouseButtonDown) {
				    GdiAlphaBlend(BackBufferDC, MIN(SnExG.gCaptureSelectionRectangle.left, SnExG.gCaptureSelectionRectangle.right),
					    0, MAX(SnExG.gCaptureSelectionRectangle.left,
					    SnExG.gCaptureSelectionRectangle.right) - MIN(SnExG.gCaptureSelectionRectangle.left, SnExG.gCaptureSelectionRectangle.right),
					    MIN(SnExG.gCaptureSelectionRectangle.top, SnExG.gCaptureSelectionRectangle.bottom),
					    AlphaDC, 0, 0, 1, 1, BlendFunction);
			    }
			    //// This darkens everything directly below the selection rectangle
			    if(LMouseButtonDown) {
				    GdiAlphaBlend(BackBufferDC, MIN(SnExG.gCaptureSelectionRectangle.left, SnExG.gCaptureSelectionRectangle.right),
					    MAX(SnExG.gCaptureSelectionRectangle.top, SnExG.gCaptureSelectionRectangle.bottom),
					    MAX(SnExG.gCaptureSelectionRectangle.left, SnExG.gCaptureSelectionRectangle.right) - MIN(SnExG.gCaptureSelectionRectangle.left, SnExG.gCaptureSelectionRectangle.right),
					    MAX(SnExG.gCaptureSelectionRectangle.bottom, SnExG.gDisplayHeight),
					    AlphaDC, 0, 0, 1, 1, BlendFunction);
			    }
			    // Finally, blit the fully-drawn backbuffer to the screen.
			    BitBlt(PaintStruct.hdc, 0, 0, SnExG.gDisplayWidth, SnExG.gDisplayHeight, BackBufferDC, 0, 0, SRCCOPY);
			    DeleteObject(ScreenShotCopy);
			    DeleteDC(BackBufferDC);
			    DeleteObject(AlphaBitmap);
			    DeleteDC(AlphaDC);
			    EndPaint(Window, &PaintStruct);
		    }
		    break;
		case WM_LBUTTONDOWN:
			{
				MyOutputDebugStringW(L"[%s] Line %d: Left mouse buttown down over capture window, capture selection beginning.\n", __FUNCTIONW__, __LINE__);
				MouseHasMovedWhileLeftMouseButtonWasDown = FALSE;
				SnExG.gCaptureSelectionRectangle.left   = 0;
				SnExG.gCaptureSelectionRectangle.right  = 0;
				SnExG.gCaptureSelectionRectangle.bottom = 0;
				SnExG.gCaptureSelectionRectangle.top    = 0;
				LMouseButtonDown = TRUE;
				InvalidateRect(Window, NULL, FALSE);
				UpdateWindow(SnExG.gCaptureWindowHandle);
				POINT Mouse = { 0 };
				Mouse.x = GET_X_LPARAM(LParam);
				Mouse.y = GET_Y_LPARAM(LParam);
				SnExG.gCaptureSelectionRectangle.left = Mouse.x;
				SnExG.gCaptureSelectionRectangle.right = Mouse.x;
				SnExG.gCaptureSelectionRectangle.top = Mouse.y;
				SnExG.gCaptureSelectionRectangle.bottom = Mouse.y;
				break;
			}
		case WM_LBUTTONUP:
		    LMouseButtonDown = FALSE;
		    SnExG.CaptureWindow_OnLeftButtonUp();
		    break;
		case WM_MOUSEMOVE:
		    if(LMouseButtonDown) {
			    MouseHasMovedWhileLeftMouseButtonWasDown = TRUE;
			    POINT Mouse = { 0 };
			    Mouse.x = GET_X_LPARAM(LParam);
			    Mouse.y = GET_Y_LPARAM(LParam);
			    SnExG.gCaptureSelectionRectangle.right = Mouse.x;
			    SnExG.gCaptureSelectionRectangle.bottom = Mouse.y;
			    InvalidateRect(Window, NULL, FALSE);
			    UpdateWindow(SnExG.gCaptureWindowHandle);
		    }
		    break;
		default:
		    Result = DefWindowProcW(Window, Message, WParam, LParam);
	}
	return Result;
}

BOOL NewButton_Click(void)
{
	BOOL Result      = FALSE;
	RECT CurrentWindowPos    = { 0 };
	wchar_t TitleBuffer[64]  = { 0 };
	HDC ScreenDC     = NULL;
	HDC MemoryDC     = NULL;
	// Now the "capture window" comes to life. It is a duplicate of the entire display surface,
	// including multiple monitors. Take a screenshot of it, then overlay it on top of the real
	// desktop, and then allow the user to select a subsection of the screenshot with the mouse.
	KillTimer(SnExG.gMainWindowHandle, DELAY_TIMER);
	SnExG.gCurrentDelayCountdown = SnExG.gStartingDelayCountdown;
	(void)_snwprintf_s(TitleBuffer, _countof(TitleBuffer), _TRUNCATE, L"SnipEx");
	SetWindowTextW(SnExG.gMainWindowHandle, TitleBuffer);
	GetWindowRect(SnExG.gMainWindowHandle, &CurrentWindowPos);
	SetWindowPos(SnExG.gMainWindowHandle, HWND_TOP, CurrentWindowPos.left, CurrentWindowPos.top, SnExG.gStartingMainWindowWidth, SnExG.gStartingMainWindowHeight, 0);
	ShowWindow(SnExG.gMainWindowHandle, SW_MINIMIZE);
	SnExG.gAppState = APPSTATE_DURINGCAPTURE;
	RtlZeroMemory(&SnExG.gCaptureSelectionRectangle, sizeof(RECT));
	if(SnExG.gCleanScreenShot != NULL) {
		if(DeleteObject(SnExG.gCleanScreenShot) == 0) {
			MyOutputDebugStringW(L"[%s] Line %d: Failed to DeleteObject(gCleanScreenShot!)!\n", __FUNCTIONW__, __LINE__);
			CRASH(0);
		}
		SnExG.gCleanScreenShot = NULL;
	}
	if(SnExG.gScratchBitmap != NULL) {
		if(DeleteObject(SnExG.gScratchBitmap) == 0) {
			MyOutputDebugStringW(L"[%s] Line %d: Failed to DeleteObject(gScratchBitmap!)!\n", __FUNCTIONW__, __LINE__);
			CRASH(0);
		}
		SnExG.gScratchBitmap = NULL;
	}
	for(INT8 SnipState = 0; SnipState < _countof(SnExG.gSnipStates) - 1; SnipState++) {
		if(SnExG.gSnipStates[SnipState] != NULL) {
			if(DeleteObject(SnExG.gSnipStates[SnipState]) == 0) {
				MyOutputDebugStringW(L"[%s] Line %d: Failed to DeleteObject(gSnipStates[%d]!)!\n", __FUNCTIONW__, __LINE__, SnipState);
				CRASH(0);
			}
			SnExG.gSnipStates[SnipState] = NULL;
		}
	}
	SnExG.gCurrentSnipState = 0;
	// We just minimized the main window. Allow a brief moment for the minimize animation to finish before capturing
	// the screen.
	Sleep(250);
	ScreenDC = GetDC(NULL);
	if(ScreenDC == NULL) {
		MessageBoxW(0, L"GetDC(NULL) failed!", L"Error", MB_OK|MB_ICONERROR|MB_SYSTEMMODAL);
		goto Cleanup;
	}
	MemoryDC = CreateCompatibleDC(ScreenDC);
	if(MemoryDC == NULL) {
		MessageBoxW(0, L"CreateCompatibleDC(ScreenDC) failed!", L"Error", MB_OK|MB_ICONERROR|MB_SYSTEMMODAL);
		goto Cleanup;
	}
	SnExG.gCleanScreenShot = CreateCompatibleBitmap(ScreenDC, SnExG.gDisplayWidth, SnExG.gDisplayHeight);
	if(SnExG.gCleanScreenShot == NULL) {
		MessageBoxW(0, L"CreateCompatibleBitmap failed!", L"Error", MB_OK|MB_ICONERROR|MB_SYSTEMMODAL);
		goto Cleanup;
	}
	SelectObject(MemoryDC, SnExG.gCleanScreenShot);
	BitBlt(MemoryDC, 0, 0, SnExG.gDisplayWidth, SnExG.gDisplayHeight, ScreenDC, SnExG.gDisplayLeft, SnExG.gDisplayTop, SRCCOPY);
	ShowWindow(SnExG.gCaptureWindowHandle, SW_SHOW);
	SetWindowPos(SnExG.gCaptureWindowHandle, HWND_TOP, SnExG.gDisplayLeft, SnExG.gDisplayTop, SnExG.gDisplayWidth, SnExG.gDisplayHeight, SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
	SetForegroundWindow(SnExG.gCaptureWindowHandle);
	Result = TRUE;
Cleanup:
	DeleteDC(MemoryDC);
	ReleaseDC(NULL, ScreenDC);
	return Result;
}

// Returns TRUE if the snip was saved. Returns FALSE if there was an error or if user cancelled.
BOOL SaveButton_Click(void)
{
	HRESULT COMError = 0;
	IFileSaveDialog* DialogInterface = NULL;
	UINT SelectedFileTypeIndex = 0;
	const COMDLG_FILTERSPEC FileTypeFilters[] = {
		{ L"Portable Network Graphics (PNG)", L"*.png" },
		{ L"32bpp Bitmap", L"*.bmp" }
	};
	IShellItem* ResultItem = NULL;
	LPOLESTR FilePathFromDialogW = NULL;
	wchar_t FinalFilePathW[MAX_PATH] = { 0 };
	BOOL Result = FALSE;
	COMError = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
	if(FAILED(COMError)) {
		MessageBoxW(0, L"Failed to initialize COM!", L"Error", MB_OK|MB_ICONERROR|MB_SYSTEMMODAL);
		goto Cleanup;
	}
	COMError = CoCreateInstance(CLSID_FileSaveDialog, NULL, CLSCTX_INPROC_SERVER, IID_IFileSaveDialog, (void**)&DialogInterface);
	if(FAILED(COMError)) {
		MessageBoxW(0, L"Failed to create COM instance of IFileDialog!", L"Error", MB_OK|MB_ICONERROR|MB_SYSTEMMODAL);
		goto Cleanup;
	}
	DialogInterface->/*lpVtbl->*/SetFileTypes(_countof(FileTypeFilters), FileTypeFilters);
	DialogInterface->/*lpVtbl->*/SetFileTypeIndex(1); // 1-based array, does not start at 0
	DialogInterface->/*lpVtbl->*/Show(SnExG.gMainWindowHandle);
	DialogInterface->/*lpVtbl->*/GetResult(&ResultItem);
	if(ResultItem == NULL) {
		// User probably hit cancel.
		goto Cleanup;
	}
	ResultItem->/*lpVtbl->*/GetDisplayName(SIGDN_FILESYSPATH, &FilePathFromDialogW);
	if(wcslen(FilePathFromDialogW) <= 3 || wcslen(FilePathFromDialogW) >= MAX_PATH - 5) {
		MessageBoxW(0, L"File path was too short or too long!", L"Error", MB_OK|MB_ICONERROR|MB_SYSTEMMODAL);
		goto Cleanup;
	}
	wcscpy_s(FinalFilePathW, MAX_PATH, FilePathFromDialogW);
	DialogInterface->/*lpVtbl->*/GetFileTypeIndex(&SelectedFileTypeIndex);
	switch(SelectedFileTypeIndex) {
		case 1:
	    {
		    if(wcslen(FinalFilePathW) < 5) {
			    wcscat_s(FinalFilePathW, MAX_PATH, L".png");
		    }
		    else {
			    if(_wcsicmp(&FinalFilePathW[wcslen(FinalFilePathW) - 4], L".png") != 0) {
				    wcscat_s(FinalFilePathW, MAX_PATH, L".png");
			    }
		    }
		    MyOutputDebugStringW(L"[%s] Line %d: Attempting to save file %s\n", __FUNCTIONW__, __LINE__, FinalFilePathW);
		    if(SavePngToFile(FinalFilePathW) == FALSE) {
			    goto Cleanup;
		    }
		    break;
	    }
		case 2:
	    {
		    if(wcslen(FinalFilePathW) < 5) {
			    wcscat_s(FinalFilePathW, MAX_PATH, L".bmp");
		    }
		    else {
			    if(_wcsicmp(&FinalFilePathW[wcslen(FinalFilePathW) - 4], L".bmp") != 0) {
				    wcscat_s(FinalFilePathW, MAX_PATH, L".bmp");
			    }
		    }
		    MyOutputDebugStringW(L"[%s] Line %d: Attempting to save file %s\n", __FUNCTIONW__, __LINE__, FinalFilePathW);
		    if(SaveBitmapToFile(FinalFilePathW) == FALSE) {
			    goto Cleanup;
		    }
		    break;
	    }
		default:
		    MessageBoxW(0, L"File type selection was not in the expected range of values!", L"Error", MB_OK|MB_ICONERROR|MB_SYSTEMMODAL);
		    goto Cleanup;
	}
	// If nothing has gone wrong up to this point, set Success to TRUE.
	Result = TRUE;
Cleanup:
	if(ResultItem) {
		ResultItem->/*lpVtbl->*/Release();
	}
	if(DialogInterface) {
		DialogInterface->/*lpVtbl->*/Release();
	}
	CoUninitialize();
	gSaveButton.SelectedTool = FALSE;
	gSaveButton.State = BUTTONSTATE_NORMAL;
	return Result;
}

BOOL CopyButton_Click(void)
{
	BOOL Result   = FALSE;
	BITMAP Bitmap = { 0 };
	HBITMAP ClipboardCopy = NULL;
	HDC SourceDC  = NULL;
	HDC DestinationDC     = NULL;
	if(SnExG.GetCurrentSnipState() == NULL) {
		MyOutputDebugStringW(L"[%s] Line %d: gSnipStates[gCurrentSnipState] is NULL. This is a bug.\n", __FUNCTIONW__, __LINE__);
		goto Cleanup;
	}
	GetObjectW(SnExG.GetCurrentSnipState(), sizeof(BITMAP), &Bitmap);
	ClipboardCopy = CreateBitmap(Bitmap.bmWidth, Bitmap.bmHeight, 1, 32, NULL);
	SourceDC = CreateCompatibleDC(NULL);
	DestinationDC = CreateCompatibleDC(NULL);
	SelectObject(SourceDC, SnExG.GetCurrentSnipState());
	SelectObject(DestinationDC, ClipboardCopy);
	BitBlt(DestinationDC, 0, 0, Bitmap.bmWidth, Bitmap.bmHeight, SourceDC, 0, 0, SRCCOPY);
	if(OpenClipboard(SnExG.gMainWindowHandle) == 0) {
		MessageBoxW(0, L"OpenClipboard failed!", L"Error", MB_OK|MB_ICONERROR|MB_SYSTEMMODAL);
		goto Cleanup;
	}
	if(EmptyClipboard() == 0) {
		MessageBoxW(0, L"EmptyClipboard failed!", L"Error", MB_OK|MB_ICONERROR|MB_SYSTEMMODAL);
		goto Cleanup;
	}
	if(SetClipboardData(CF_BITMAP, ClipboardCopy) == NULL) {
		MessageBoxW(0, L"SetClipboardData failed!", L"Error", MB_OK|MB_ICONERROR|MB_SYSTEMMODAL);
		goto Cleanup;
	}
	Result = TRUE;
Cleanup:
	ZDeleteWinGdiObject(&SourceDC);
	ZDeleteWinGdiObject(&DestinationDC);
	ZDeleteWinGdiObject(&ClipboardCopy);
	CloseClipboard();
	gCopyButton.SelectedTool = FALSE;
	gCopyButton.State = BUTTONSTATE_NORMAL;
	return Result;
}

BOOL SaveBitmapToFile(_In_ wchar_t* FilePath)
{
	BOOL Success = FALSE;
	BITMAPFILEHEADER BitmapFileHeader = { 0 };
	PBITMAPINFOHEADER BitmapInfoHeaderPointer = { 0 };
	BITMAP Bitmap = { 0 };
	PBITMAPINFO BitmapInfoPointer = { 0 };
	WORD cClrBits = 0;
	LPBYTE Bits = { 0 };
	HDC DC = NULL;
	DWORD BytesWritten = 0;
	DWORD TotalBytes = 0;
	DWORD IncrementalBytes = 0;
	BYTE * BytePointer = 0;
	HANDLE FileHandle = CreateFileW(FilePath, GENERIC_READ | GENERIC_WRITE, (DWORD)0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if(FileHandle == INVALID_HANDLE_VALUE) {
		MessageBoxW(0, L"Failed to create bitmap file!", L"Error", MB_OK|MB_ICONERROR|MB_SYSTEMMODAL);
		goto Cleanup;
	}
	if(GetObject(SnExG.GetCurrentSnipState(), sizeof(BITMAP), &Bitmap) == 0) {
		MyOutputDebugStringW(L"[%s] Line %d: GetObject failed!\n", __FUNCTIONW__, __LINE__);
		goto Cleanup;
	}
	// Convert the color format to a count of bits.
	cClrBits = (WORD)(Bitmap.bmPlanes * Bitmap.bmBitsPixel);
	if(cClrBits == 1)
		cClrBits = 1;
	else if(cClrBits <= 4)
		cClrBits = 4;
	else if(cClrBits <= 8)
		cClrBits = 8;
	else if(cClrBits <= 16)
		cClrBits = 16;
	else if(cClrBits <= 24)
		cClrBits = 24;
	else
		cClrBits = 32;
	// Allocate memory for the BITMAPINFO structure. (This structure
	// contains a BITMAPINFOHEADER structure and an array of RGBQUAD
	// data structures.)
	if(cClrBits < 24) {
		BitmapInfoPointer = (PBITMAPINFO)LocalAlloc(LPTR, sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * (1i64 << cClrBits));
	}
	else {
		// There is no RGBQUAD array for these formats: 24-bit-per-pixel or 32-bit-per-pixel
		BitmapInfoPointer = (PBITMAPINFO)LocalAlloc(LPTR, sizeof(BITMAPINFOHEADER));
	}
	if(BitmapInfoPointer == NULL) {
		MessageBoxW(0, L"Failed to allocate memory!", L"Error", MB_OK|MB_ICONERROR|MB_SYSTEMMODAL);
		MyOutputDebugStringW(L"[%s] Line %d: LocalAlloc failed!\n", __FUNCTIONW__, __LINE__);
		goto Cleanup;
	}
	BitmapInfoPointer->bmiHeader.biSize     = sizeof(BITMAPINFOHEADER);
	BitmapInfoPointer->bmiHeader.biWidth    = Bitmap.bmWidth;
	BitmapInfoPointer->bmiHeader.biHeight   = Bitmap.bmHeight;
	BitmapInfoPointer->bmiHeader.biPlanes   = Bitmap.bmPlanes;
	BitmapInfoPointer->bmiHeader.biBitCount = Bitmap.bmBitsPixel;
	if(cClrBits < 24) {
		BitmapInfoPointer->bmiHeader.biClrUsed = (1 << cClrBits);
	}
	// If the bitmap is not compressed, set the BI_RGB flag.
	BitmapInfoPointer->bmiHeader.biCompression = BI_RGB;
	// Compute the number of bytes in the array of color
	// indices and store the result in biSizeImage.
	// The width must be DWORD aligned unless the bitmap is RLE
	// compressed.
	BitmapInfoPointer->bmiHeader.biSizeImage = ((BitmapInfoPointer->bmiHeader.biWidth * cClrBits + 31) & ~31) / 8 * BitmapInfoPointer->bmiHeader.biHeight;
	// Set biClrImportant to 0, indicating that all of the
	// device colors are important.
	BitmapInfoPointer->bmiHeader.biClrImportant = 0;
	BitmapInfoHeaderPointer = (PBITMAPINFOHEADER)BitmapInfoPointer;
	Bits = (LPBYTE)GlobalAlloc(GMEM_FIXED, BitmapInfoHeaderPointer->biSizeImage);
	if(Bits == 0) {
		MessageBoxW(0, L"Failed to allocate memory!", L"Error", MB_OK|MB_ICONERROR|MB_SYSTEMMODAL);
		MyOutputDebugStringW(L"[%s] Line %d: GlobalAlloc failed!\n", __FUNCTIONW__, __LINE__);
		goto Cleanup;
	}
	DC = CreateCompatibleDC(NULL);
	SelectObject(DC, SnExG.GetCurrentSnipState());
	// Retrieve the color table (RGBQUAD array) and the bits (array of palette indices) from the DIB.
	GetDIBits(DC, SnExG.GetCurrentSnipState(), 0, (WORD)BitmapInfoHeaderPointer->biHeight, Bits, BitmapInfoPointer, DIB_RGB_COLORS);
	BitmapFileHeader.bfType = 0x4d42;       // "BM"
	BitmapFileHeader.bfReserved1 = 0;
	BitmapFileHeader.bfReserved2 = 0;
	BitmapFileHeader.bfSize = (DWORD)(sizeof(BITMAPFILEHEADER) + BitmapInfoHeaderPointer->biSize + BitmapInfoHeaderPointer->biClrUsed * sizeof(RGBQUAD) +
	    BitmapInfoHeaderPointer->biSizeImage);
	// Compute the offset to the array of color indices.
	BitmapFileHeader.bfOffBits = (DWORD)sizeof(BITMAPFILEHEADER) + BitmapInfoHeaderPointer->biSize + BitmapInfoHeaderPointer->biClrUsed * sizeof(RGBQUAD);
	WriteFile(FileHandle, (LPVOID)&BitmapFileHeader, sizeof(BITMAPFILEHEADER), (LPDWORD)&BytesWritten, NULL);
	// Copy the BITMAPINFOHEADER and RGBQUAD array into the file.
	WriteFile(FileHandle, (LPVOID)BitmapInfoHeaderPointer,
	    sizeof(BITMAPINFOHEADER) + BitmapInfoHeaderPointer->biClrUsed * sizeof(RGBQUAD), (LPDWORD)&BytesWritten, (NULL));
	// Copy the array of color indices into the .BMP file.
	TotalBytes = IncrementalBytes = BitmapInfoHeaderPointer->biSizeImage;
	BytePointer = Bits;
	WriteFile(FileHandle, (LPSTR)BytePointer, (int)IncrementalBytes, (LPDWORD)&BytesWritten, NULL);
	// If nothing has failed up to this point, set Success to TRUE.
	Success = TRUE;
Cleanup:
	GlobalFree(Bits);
	LocalFree(BitmapInfoPointer);
	DeleteDC(DC);
	CloseHandle(FileHandle);
	if(Success == TRUE) {
		MyOutputDebugStringW(L"[%s] Line %d: Returning successfully.\n", __FUNCTIONW__, __LINE__);
		return TRUE;
	}
	else {
		MyOutputDebugStringW(L"[%s] Line %d: Returning failure!\n", __FUNCTIONW__, __LINE__);
		return FALSE;
	}
}

BOOL SavePngToFile(_In_ wchar_t* FilePath)
{
	BOOL Result       = FALSE;
	CLSID PngCLSID     = { 0 };
	HRESULT Error        = 0;
	HMODULE GDIPlus      = LoadLibraryW(L"GDIPlus.dll");
	ULONG_PTR GdiplusToken = 0;
	ULONG*    GdipBitmap   = 0;
	if(GDIPlus == NULL) {
		SnExG.PopupMessageErr(L"LoadLibrary(\"GDIPlus.dll\") failed!");
		goto Cleanup;
	}
	Error = CLSIDFromString(L"{557CF406-1A04-11D3-9A73-0000F81EF32E}", &PngCLSID);
	if(Error != NOERROR) {
		SnExG.PopupMessageErr(L"CLSIDFromString failed!");
		goto Cleanup;
	}
	GdiplusStartup = (int (WINAPI*)(ULONG_PTR*, struct GdiplusStartupInput*, void*))GetProcAddress(GDIPlus, "GdiplusStartup");
	if(GdiplusStartup == NULL) {
		SnExG.PopupMessageErr(L"Failed to load GdiplusStartup from gdiplus.dll!");
		goto Cleanup;
	}
	GdiplusShutdown = (int (WINAPI*)(ULONG_PTR))GetProcAddress(GDIPlus, "GdiplusShutdown");
	if(GdiplusShutdown == NULL) {
		SnExG.PopupMessageErr(L"Failed to load GdiplusShutdown from gdiplus.dll!");
		goto Cleanup;
	}
	GdipCreateBitmapFromHBITMAP = (int (WINAPI*)(HBITMAP, HPALETTE hPalette, ULONG**Bitmap))GetProcAddress(GDIPlus, "GdipCreateBitmapFromHBITMAP");
	if(GdipCreateBitmapFromHBITMAP == NULL) {
		SnExG.PopupMessageErr(L"Failed to load GdipCreateBitmapFromHBITMAP from gdiplus.dll!");
		goto Cleanup;
	}
	GdipSaveImageToFile = (int (WINAPI*)(ULONG*, const WCHAR*, const CLSID*, const EncoderParameters*))GetProcAddress(GDIPlus, "GdipSaveImageToFile");
	if(GdipSaveImageToFile == NULL) {
		SnExG.PopupMessageErr(L"Failed to load GdipSaveImageToFile from gdiplus.dll!");
		goto Cleanup;
	}
	GdipDisposeImage = (int (WINAPI*)(ULONG*))GetProcAddress(GDIPlus, "GdipDisposeImage");
	if(GdipDisposeImage == NULL) {
		SnExG.PopupMessageErr(L"Failed to load GdipDisposeImage from gdiplus.dll!");
		goto Cleanup;
	}
	{
		GdiplusStartupInput GdiplusStartupInput = { 0 };
		GdiplusStartupInput.GdiplusVersion   = 1;
		GdiplusStartupInput.DebugEventCallback       = NULL;
		GdiplusStartupInput.SuppressBackgroundThread = FALSE;
		GdiplusStartupInput.SuppressExternalCodecs   = FALSE;
		if((Error = GdiplusStartup(&GdiplusToken, &GdiplusStartupInput, NULL)) != S_OK) {
			SnExG.PopupMessageErr(L"GdiplusStartup failed!");
			goto Cleanup;
		}
	}
	if((Error = GdipCreateBitmapFromHBITMAP(SnExG.GetCurrentSnipState(), NULL, &GdipBitmap)) != 0) {
		SnExG.PopupMessageErr(L"GdipCreateBitmapFromHBITMAP failed!");
		goto Cleanup;
	}
	{
		EncoderParameters EncoderParams = { 0 };
		LONG Compression = 5;
		EncoderParams.Count = 1;
		EncoderParams.Parameter[0].NumberOfValues = 1;
		EncoderParams.Parameter[0].Guid = gEncoderCompressionGuid;
		EncoderParams.Parameter[0].Type = EncoderParameterValueTypeLong;
		EncoderParams.Parameter[0].Value = &Compression;
		if((Error = GdipSaveImageToFile(GdipBitmap, FilePath, &PngCLSID, &EncoderParams)) != 0) {
			SnExG.PopupMessageErr(L"GdipSaveImageToFile failed!");
			goto Cleanup;
		}
	}
	Result = TRUE;
Cleanup:
	if(GdipBitmap != NULL)
		GdipDisposeImage(GdipBitmap);
	if(GdiplusToken != 0)
		GdiplusShutdown(GdiplusToken);
	FreeLibrary(GDIPlus);
	return Result;
}

BOOL IsAppRunningElevated(void)
{
	MyOutputDebugStringW(L"[%s] Line %d: Entered.\n", __FUNCTIONW__, __LINE__);
	BOOL IsElevated = FALSE;
	DWORD Error = ERROR_SUCCESS;
	PSID AdministratorsGroup = NULL;
	// Allocate and initialize a SID of the administrators group.
	SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
	if(!AllocateAndInitializeSid(&NtAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &AdministratorsGroup)) {
		Error = GetLastError();
		goto Cleanup;
	}
	// Determine whether the SID of administrators group is enabled in
	// the primary access token of the process.
	if(!CheckTokenMembership(NULL, AdministratorsGroup, &IsElevated)) {
		Error = GetLastError();
		goto Cleanup;
	}
Cleanup:
	// Centralized cleanup for all allocated resources.
	if(AdministratorsGroup) {
		FreeSid(AdministratorsGroup);
		AdministratorsGroup = NULL;
	}
	// Throw the error if something failed in the function.
	if(ERROR_SUCCESS != Error) {
		SnExG.PopupMessageErr(L"Error checking for UAC Elevation!");
	}
	MyOutputDebugStringW(L"[%s] Line %d: Process is elevated: %d.\n", __FUNCTIONW__, __LINE__, IsElevated);
	return(IsElevated);
}

// OutputDebugString enhanced with varargs. Only works in debug builds.
void MyOutputDebugStringW(_In_ wchar_t* Message, _In_ ...)
{
	#if _DEBUG
	wchar_t Buffer[512] = { 0 };
	va_list VarArgPointer = NULL;
	va_start(VarArgPointer, Message);
	_vsnwprintf_s(Buffer, _countof(Buffer), _TRUNCATE, Message, VarArgPointer);
	va_end(VarArgPointer);
	OutputDebugStringW(Buffer);
	#else
	UNREFERENCED_PARAMETER(Message);
	#endif
}

BOOL AdjustForCustomScaling(void)
{
	// We need to do some adjusting for non-default DPI monitors/scaling factors.
	UINT DPIx = 0;
	UINT DPIy = 0;
	HMONITOR h_mon = MonitorFromWindow(GetDesktopWindow(), MONITOR_DEFAULTTOPRIMARY);
	MONITORINFOEXA moninfo;
	INITWINAPISTRUCT(moninfo);
	if(GetMonitorInfoA(h_mon, &moninfo)) {
		HDC h_dc = CreateDCA("DISPLAY", moninfo.szDevice, 0, 0);
		DPIx = GetDeviceCaps(h_dc, LOGPIXELSX);
		DPIy = GetDeviceCaps(h_dc, LOGPIXELSY);
		DeleteDC(h_dc);
	}
	if(!DPIx || !DPIy) {
		MessageBoxW(0, L"Unable to determine the monitor DPI of your primary monitor!", L"Error", MB_OK|MB_ICONERROR|MB_SYSTEMMODAL);
		return FALSE;
	}
	/*if(GetDpiForMonitor(MonitorFromWindow(GetDesktopWindow(), MONITOR_DEFAULTTOPRIMARY), MDT_EFFECTIVE_DPI, &DPIx, &DPIy) != S_OK) {
		MessageBoxW(0, L"Unable to determine the monitor DPI of your primary monitor!", L"Error", MB_OK|MB_ICONERROR|MB_SYSTEMMODAL);
		return FALSE;
	}*/
	MyOutputDebugStringW(L"[%s] Line %d: Detected a monitor DPI of %d.\n", __FUNCTIONW__, __LINE__, DPIx);
	// Pct    DPI
	// 100% =  96 X
	// 105% = 101
	// 110% = 106
	// 115% = 110
	// 120% = 115
	// 125% = 120
	// 130% = 125
	// 135% = 130
	// 140% = 134
	// 145% = 139
	// 150% = 144
	// 155% = 149
	// 160% = 154
	// 165% = 158
	// 170% = 163
	// 175% = 168
	// 180% = 173
	// 185% = 178
	// 190% = 182
	// 195% = 187
	// 200% = 192
	if(DPIx == 96) {
		return TRUE;
	}
	if(DPIx > 96 && DPIx <= 106) {
		SnExG.gStartingMainWindowHeight += 2;
		SnExG.gStartingMainWindowWidth  += 2;
	}
	else if(DPIx > 106 && DPIx <= 110) {
		SnExG.gStartingMainWindowHeight += 5;
		SnExG.gStartingMainWindowWidth  += 3;
	}
	else if(DPIx > 110 && DPIx <= 115) {
		SnExG.gStartingMainWindowHeight += 6;
		SnExG.gStartingMainWindowWidth  += 3;
	}
	else if(DPIx > 115 && DPIx <= 120) {
		SnExG.gStartingMainWindowHeight += 8;
		SnExG.gStartingMainWindowWidth  += 3;
	}
	else if(DPIx > 120 && DPIx <= 125) {
		SnExG.gStartingMainWindowHeight += 10;
		SnExG.gStartingMainWindowWidth  += 3;
	}
	else if(DPIx > 125 && DPIx <= 130) {
		SnExG.gStartingMainWindowHeight += 10;
		SnExG.gStartingMainWindowWidth  += 3;
	}
	else if(DPIx > 130 && DPIx <= 134) {
		SnExG.gStartingMainWindowHeight += 15;
		SnExG.gStartingMainWindowWidth  += 5;
	}
	else if(DPIx > 134 && DPIx <= 139) {
		SnExG.gStartingMainWindowHeight += 15;
		SnExG.gStartingMainWindowWidth  += 5;
	}
	else if(DPIx > 139 && DPIx <= 144) {
		SnExG.gStartingMainWindowHeight += 17;
		SnExG.gStartingMainWindowWidth  += 7;
	}
	else if(DPIx > 144 && DPIx <= 149) {
		SnExG.gStartingMainWindowHeight += 18;
		SnExG.gStartingMainWindowWidth  += 7;
	}
	else if(DPIx > 149 && DPIx <= 154) {
		SnExG.gStartingMainWindowHeight += 19;
		SnExG.gStartingMainWindowWidth  += 7;
	}
	else if(DPIx > 154 && DPIx <= 158) {
		SnExG.gStartingMainWindowHeight += 22;
		SnExG.gStartingMainWindowWidth  += 9;
	}
	else if(DPIx > 158 && DPIx <= 163) {
		SnExG.gStartingMainWindowHeight += 23;
		SnExG.gStartingMainWindowWidth  += 9;
	}
	else if(DPIx > 163 && DPIx <= 168) {
		SnExG.gStartingMainWindowHeight += 25;
		SnExG.gStartingMainWindowWidth  += 9;
	}
	else if(DPIx > 168 && DPIx <= 173) {
		SnExG.gStartingMainWindowHeight += 26;
		SnExG.gStartingMainWindowWidth  += 9;
	}
	else if(DPIx > 173 && DPIx <= 178) {
		SnExG.gStartingMainWindowHeight += 27;
		SnExG.gStartingMainWindowWidth  += 9;
	}
	else if(DPIx > 178 && DPIx <= 182) {
		SnExG.gStartingMainWindowHeight += 30;
		SnExG.gStartingMainWindowWidth  += 10;
	}
	else if(DPIx > 182 && DPIx <= 187) {
		SnExG.gStartingMainWindowHeight += 31;
		SnExG.gStartingMainWindowWidth  += 10;
	}
	else if(DPIx > 187 && DPIx <= 192) {
		SnExG.gStartingMainWindowHeight += 32;
		SnExG.gStartingMainWindowWidth  += 10;
	}
	else {
		MessageBoxW(0, L"Unable to deal with your custom scaling level. I can only handle up to 200% scaling. Contact me at ryanries09@gmail.com if you want me to add support for your scaling level.",
		    L"Error", MB_OK|MB_ICONERROR|MB_SYSTEMMODAL);
		MyOutputDebugStringW(L"[%s] Line %d: ERROR! Unsupported DPI!\n", __FUNCTIONW__, __LINE__);
		return FALSE;
	}
	return TRUE;
}

LSTATUS DeleteSnipExRegValue(_In_ wchar_t* ValueName)
{
	LSTATUS Result = ERROR_SUCCESS;
	HKEY SoftwareKey = NULL;
	HKEY SnipExKey = NULL;
	DWORD SnipExKeyDisposition = 0;
	// This key should always exist. Something is very wrong if we can't open it.
	if((Result = RegOpenKeyExW(HKEY_CURRENT_USER, L"SOFTWARE", 0, KEY_ALL_ACCESS, &SoftwareKey)) != ERROR_SUCCESS) {
		MessageBoxW(0, L"Unable to read HKCU\\SOFTWARE registry key!", L"Error", MB_OK|MB_ICONERROR|MB_SYSTEMMODAL);
		MyOutputDebugStringW(L"[%s] Line %d: ERROR! Unable to read HKCU\\SOFTWARE registry key! LSTATUS = 0x%lx\n", __FUNCTIONW__, __LINE__, Result);
	}
	else {
		if((Result = RegCreateKeyExW(SoftwareKey, L"SnipEx", 0, NULL, 0, KEY_ALL_ACCESS, NULL, &SnipExKey, &SnipExKeyDisposition)) == ERROR_SUCCESS) {
			if(SnipExKeyDisposition == REG_CREATED_NEW_KEY) {
				MyOutputDebugStringW(L"[%s] Line %d: Created SnipEx registry key. Aborting function since their can be no values to delete.\n", __FUNCTIONW__, __LINE__);
				goto Exit;
			}
			else
				MyOutputDebugStringW(L"[%s] Line %d: Opened SnipEx registry key.\n", __FUNCTIONW__, __LINE__);
			if((Result = RegDeleteValueW(SnipExKey, ValueName)) != ERROR_SUCCESS) {
				if(Result == ERROR_FILE_NOT_FOUND) {
					MyOutputDebugStringW(L"[%s] Line %d: Tried to delete the '%s' registry value but it did not exist.\n", __FUNCTIONW__, __LINE__, ValueName);
					Result = ERROR_SUCCESS;
				}
				else
					MyOutputDebugStringW(L"[%s] Line %d: Failed to delete the '%s' registry value!\n", __FUNCTIONW__, __LINE__, ValueName);
			}
			else
				MyOutputDebugStringW(L"[%s] Line %d: Registry value %s deleted.\n", __FUNCTIONW__, __LINE__, ValueName);
		}
		else {
			SnExG.PopupMessageErr(L"Could not open the SnipEx registry key for writing!");
			MyOutputDebugStringW(L"[%s] Line %d: ERROR! Unable to read the SnipEx registry key! LSTATUS = 0x%lx\n", __FUNCTIONW__, __LINE__, Result);
		}
	}
Exit:
	::RegCloseKey(SnipExKey);
	::RegCloseKey(SoftwareKey);
	return Result;
}

LSTATUS SetSnipExRegValue(_In_ wchar_t* ValueName, _In_ DWORD* ValueData)
{
	LSTATUS Result = ERROR_SUCCESS;
	HKEY SoftwareKey = NULL;
	HKEY SnipExKey = NULL;
	DWORD SnipExKeyDisposition = 0;
	// This key should always exist. Something is very wrong if we can't open it.
	if((Result = RegOpenKeyExW(HKEY_CURRENT_USER, L"SOFTWARE", 0, KEY_ALL_ACCESS, &SoftwareKey)) != ERROR_SUCCESS) {
		MessageBoxW(0, L"Unable to read HKCU\\SOFTWARE registry key!", L"Error", MB_OK|MB_ICONERROR|MB_SYSTEMMODAL);
		MyOutputDebugStringW(L"[%s] Line %d: ERROR! Unable to read HKCU\\SOFTWARE registry key! LSTATUS = 0x%lx\n", __FUNCTIONW__, __LINE__, Result);
	}
	else {
		if((Result = RegCreateKeyExW(SoftwareKey, L"SnipEx", 0, NULL, 0, KEY_ALL_ACCESS, NULL, &SnipExKey, &SnipExKeyDisposition)) == ERROR_SUCCESS) {
			if(SnipExKeyDisposition == REG_CREATED_NEW_KEY) {
				MyOutputDebugStringW(L"[%s] Line %d: Created SnipEx registry key.\n", __FUNCTIONW__, __LINE__);
			}
			else {
				MyOutputDebugStringW(L"[%s] Line %d: Opened SnipEx registry key.\n", __FUNCTIONW__, __LINE__);
			}
			if((Result = RegSetValueExW(SnipExKey, ValueName, 0, REG_DWORD, (const BYTE*)ValueData, sizeof(DWORD))) != ERROR_SUCCESS) {
				SnExG.PopupMessageErr(L"Could not set registry value!");
				MyOutputDebugStringW(L"[%s] Line %d: Could not set the '%s' registry value! LSTATUS = 0x%lx\n", __FUNCTIONW__, __LINE__, ValueName, Result);
			}
			else {
				MyOutputDebugStringW(L"[%s] Line %d: Set the '%s' registry value to %d.\n", __FUNCTIONW__, __LINE__, ValueName, *ValueData);
			}
		}
		else {
			SnExG.PopupMessageErr(L"Could not open the SnipEx registry key for writing!"); 
			MyOutputDebugStringW(L"[%s] Line %d: ERROR! Unable to read the SnipEx registry key! LSTATUS = 0x%lx\n", __FUNCTIONW__, __LINE__, Result);
		}
	}
	::RegCloseKey(SnipExKey);
	::RegCloseKey(SoftwareKey);
	return Result;
}

LSTATUS GetSnipExRegValue(_In_ wchar_t* ValueName, _In_ DWORD* ValueData)
{
	LSTATUS Result = ERROR_SUCCESS;
	HKEY SoftwareKey = NULL;
	HKEY SnipExKey = NULL;
	DWORD SnipExKeyDisposition = 0;
	DWORD ValueSize = sizeof(DWORD);
	// This key should always exist. Something is very wrong if we can't open it.
	if((Result = RegOpenKeyExW(HKEY_CURRENT_USER, L"SOFTWARE", 0, KEY_ALL_ACCESS, &SoftwareKey)) != ERROR_SUCCESS) {
		MessageBoxW(0, L"Unable to read HKCU\\SOFTWARE registry key!", L"Error", MB_OK|MB_ICONERROR|MB_SYSTEMMODAL);
		MyOutputDebugStringW(L"[%s] Line %d: ERROR! Unable to read HKCU\\SOFTWARE registry key! LSTATUS = 0x%lx\n", __FUNCTIONW__, __LINE__, Result);
	}
	else {
		if((Result = RegCreateKeyExW(SoftwareKey, L"SnipEx", 0, NULL, 0, KEY_READ, NULL, &SnipExKey, &SnipExKeyDisposition)) == ERROR_SUCCESS) {
			if(SnipExKeyDisposition == REG_CREATED_NEW_KEY)
				MyOutputDebugStringW(L"[%s] Line %d: Created SnipEx registry key.\n", __FUNCTIONW__, __LINE__);
			else
				MyOutputDebugStringW(L"[%s] Line %d: Opened SnipEx registry key.\n", __FUNCTIONW__, __LINE__);
			if((Result = RegQueryValueExW(SnipExKey, ValueName, NULL, NULL, (BYTE*)ValueData, &ValueSize)) != ERROR_SUCCESS) {
				if(Result == ERROR_FILE_NOT_FOUND) {
					MyOutputDebugStringW(L"[%s] Line %d: Tried to read the '%s' registry value but it did not exist.\n", __FUNCTIONW__, __LINE__, ValueName);
					Result = ERROR_SUCCESS;
				}
				else {
					SnExG.PopupMessageErr(L"Could not get registry value!");
					MyOutputDebugStringW(L"[%s] Line %d: Could not get the '%s' registry value! LSTATUS = 0x%lx\n", __FUNCTIONW__, __LINE__, ValueName, Result);
				}
			}
			else
				MyOutputDebugStringW(L"[%s] Line %d: Read the '%s' registry value as %d.\n", __FUNCTIONW__, __LINE__, ValueName, *ValueData);
		}
		else {
			SnExG.PopupMessageErr(L"Could not open the SnipEx registry key for reading!");
			MyOutputDebugStringW(L"[%s] Line %d: ERROR! Unable to read from the SnipEx registry key! LSTATUS = 0x%lx\n", __FUNCTIONW__, __LINE__, Result);
		}
	}
	::RegCloseKey(SnipExKey);
	::RegCloseKey(SoftwareKey);
	return Result;
}

BOOL CALLBACK TextEditCallback(_In_ HWND Dialog, _In_ UINT Message, _In_ WPARAM WParam, _In_ LPARAM LParam)
{
	UNREFERENCED_PARAMETER(LParam);
	switch(Message) {
		case WM_INITDIALOG:
	    {
		    HWND TextBox = GetDlgItem(Dialog, IDC_EDIT1);
		    // Get the size of the font that the user has selected, since the font size will determine the exact
		    // coordinates of where the text box and text will go.
		    HDC DC = CreateCompatibleDC(NULL);
		    SelectObject(DC, SnExG.gFont);
		    TEXTMETRICW TextMetrics = { 0 };
		    GetTextMetricsW(DC, &TextMetrics);
		    DeleteDC(DC);
		    SetFocus(TextBox);          // Set focus on the text box.
		    SendMessageW(TextBox, WM_SETFONT, (WPARAM)SnExG.gFont, 0);        // Change the font of the text box.
		    ClientToScreen(SnExG.gMainWindowHandle, &SnExG.gTextBoxLocation);
			// Move the entire dialog box to the approximate location of the user's mouse cursor.
		    SetWindowPos(Dialog, HWND_TOP, SnExG.gTextBoxLocation.x, SnExG.gTextBoxLocation.y - (TextMetrics.tmHeight / 2), 0, 0, SWP_NOSIZE | SWP_NOZORDER); 
			// Move the text box to the very top-left corner of the parent dialog box, and adjust its height based on the font that the user chose.
		    SetWindowPos(TextBox, HWND_TOP, 0, 0, 200, TextMetrics.tmHeight, SWP_NOZORDER);
		    SetWindowPos(Dialog, HWND_TOP, 0, 0, 200, TextMetrics.tmHeight, SWP_NOZORDER | SWP_NOMOVE); // Shrink the dialog box to exactly match the text box control.
	    }
		case WM_COMMAND:
		    switch(LOWORD(WParam)) {
			    case IDOK: 
					GetDlgItemTextW(Dialog, IDC_EDIT1, SnExG.gTextBuffer, _countof(SnExG.gTextBuffer));
					EndDialog(Dialog, WParam);
					break;
		    }
	}
	return FALSE;
}
