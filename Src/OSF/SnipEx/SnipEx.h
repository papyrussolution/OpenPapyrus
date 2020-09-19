// SnipEx.h
// Author: Joseph Ryan Ries, 2017-2020
// The snip.exe that comes bundled with Microsoft Windows is *almost* good enough. So I made one just a little better.

#pragma once

#define CRASH(Expression) if (!(Expression)) { MessageBoxW(NULL, L"Something has failed that should never have failed and now the program will crash. If you want to help debug this, report this crash to ryanries09@gmail.com.", L"Fatal Error", MB_ICONERROR | MB_OK | MB_SYSTEMMODAL); *(int *)0 = 0; }
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
typedef struct SMALLPOINT {
	UINT16 x;
	UINT16 y;
} SMALLPOINT;

typedef enum BUTTONSTATE {
	BUTTONSTATE_NORMAL,
	BUTTONSTATE_PRESSED
} BUTTONSTATE;

typedef struct BUTTON {
	RECT        Rectangle;      // left, top, right, bottom
	wchar_t*    Caption;
	HBITMAP     EnabledIcon;
	HBITMAP     DisabledIcon;
	int         EnabledIconId;
	int         DisabledIconId;
	BUTTONSTATE State;          // Is the button pressed or not
	UINT16      Hotkey;
	HWND        Handle;
	BOOL        Enabled;
	LONGLONG    Id;
	BOOL        SelectedTool;   // If the button is selected as a tool it should stay pressed
	int         CursorId;
	HCURSOR     Cursor;
	UINT8		Color;
} BUTTON;

typedef enum APPSTATE {
	APPSTATE_BEFORECAPTURE,
	APPSTATE_DURINGCAPTURE,
	APPSTATE_DELAYCOOKING,
	APPSTATE_AFTERCAPTURE
} APPSTATE;

#pragma region DECLARATIONS

int CALLBACK WinMain(_In_ HINSTANCE Instance, _In_opt_ HINSTANCE PreviousInstance, _In_ LPSTR CommandLine, _In_ int WindowShowCode);
LRESULT CALLBACK MainWindowCallback(_In_ HWND Window, _In_ UINT Message, _In_ WPARAM WParam, _In_ LPARAM LParam);
void DrawButton(_In_ DRAWITEMSTRUCT* DrawItemStruct, _In_ BUTTON Button);
LRESULT CALLBACK CaptureWindowCallback(_In_ HWND Window, _In_ UINT Message, _In_ WPARAM WParam, _In_ LPARAM LParam);
BOOL CALLBACK TextEditCallback(_In_ HWND Dialog, _In_ UINT Message, _In_ WPARAM WParam, _In_ LPARAM LParam);
void CaptureWindow_OnLeftButtonUp(void);
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
HRESULT AddAllMenuItems(_In_ HINSTANCE Instance);
BOOL IsAppRunningElevated(void);
// OutputDebugStringW enhanced with varargs. Only works in debug builds.
void MyOutputDebugStringW(_In_ wchar_t* Message, _In_ ...);
BOOL AdjustForCustomScaling(void);
LSTATUS SetSnipExRegValue(_In_ wchar_t* ValueName, _In_ DWORD* ValueData);
LSTATUS GetSnipExRegValue(_In_ wchar_t* ValueName, _In_ DWORD* ValueData);
LSTATUS DeleteSnipExRegValue(_In_ wchar_t* ValueName);

#pragma endregion