#include <windows.h>

#include "events.h"
#include "video.h"
#include "logging.h"

// The window we use for everything...
HWND vDosHwnd = NULL;

// Private data -- Action queue
#define MAXACTIONS	64
static struct {
	int head;
	int tail;
	userAction uAct[MAXACTIONS];
} ActionQ;


bool vPollEvent(userAction *uAct)
	{
	MSG msg;

	while (PeekMessage(&msg, NULL,  0, 0, PM_REMOVE))								// Get uActs from Windows
		DispatchMessage(&msg);

	if (ActionQ.head == ActionQ.tail)
		return false;
	*uAct = ActionQ.uAct[ActionQ.head];
	ActionQ.head = (++ActionQ.head)%MAXACTIONS;
	return true;
	}

void vPushEvent(userAction *uAct, WPARAM vkey, UINT scancode, bool pressed)
	{
	static userAction prevUnicode;
	Bit8u kbState[256];

	GetKeyboardState(kbState);
	uAct->flags1 = kbState[VK_RSHIFT]>>7;											// Flags are mostly masked by shifting
	uAct->flags1 |= kbState[VK_LSHIFT]>>6;											// We let them set by the routine adding keys to the Bios buffer
	uAct->flags1 |= kbState[VK_CONTROL]>>5;											// So they are actual at that moment
	uAct->flags1 |= kbState[VK_MENU]>>4;											// And can be omitted there if an alternative IRQ1/Int9 is used (XYwrite)
	uAct->flags1 |= kbState[VK_SCROLL]<<4;
	uAct->flags1 |= kbState[VK_NUMLOCK]<<5;
	uAct->flags1 |= kbState[VK_CAPITAL]<<6;
	uAct->flags1 |= kbState[VK_INSERT]<<7;

	if (kbState[VK_RMENU]&128)														// Right Alt is reported as Left Ctrl + Right Menu
		uAct->flags2 = 0;
	else
		uAct->flags2 = kbState[VK_LCONTROL]>>7;
	uAct->flags2 |= kbState[VK_LMENU]>>6;
	uAct->flags2 |= kbState[VK_SCROLL]>>3;
	uAct->flags2 |= kbState[VK_NUMLOCK]>>2;
	uAct->flags2 |= kbState[VK_CAPITAL]>>1;
	uAct->flags2 |= (kbState[VK_INSERT] & 0x80);

	uAct->flags3 = 0x10;															// Enhanced keyboard installed
	uAct->flags3 |= kbState[VK_RCONTROL]>>5;
	uAct->flags3 |= kbState[VK_RMENU]>>4;

	uAct->leds = (kbState[VK_SCROLL] & 0x01);
	uAct->leds |= kbState[VK_NUMLOCK]<<1;
	uAct->leds |= kbState[VK_CAPITAL]<<2;

	uAct->scancode = (unsigned char)(scancode&0xff);
	uAct->flags = scancode&0x100 ? 1 : 0;
	uAct->flags += ((kbState[VK_LWIN]|kbState[VK_RWIN])&0x80) ? 2 : 0;
	uAct->virtKey = vkey;
	uAct->unicode = 0;

	if (uAct->type == WM_MOUSEMOVE)												// If already a mouse move in queue, replace it
		{
		for (int idx = ActionQ.head; idx != ActionQ.tail; idx = (++idx)%MAXACTIONS)
			if (ActionQ.uAct[idx].type == uAct->type)
				{
				ActionQ.uAct[idx] = *uAct;
				return;
				}
		}
	else if (scancode && pressed)
		{
		Bit16u wchars[4];

		int n = ToUnicode((UINT)vkey, scancode, kbState, (LPWSTR)wchars, sizeof(wchars)/sizeof(wchars[0]), 0);
		if (n < 0)																	// -1 indicates a dead key
			{
			prevUnicode = *uAct;													// Save to use if second key doesn't compose a character
			return;
			}
		if (n > 1)																	// More than 1 unicode generally indicates a dead key was used before
			{
			prevUnicode.unicode = wchars[0];
		    prevUnicode.type = WM_KEYDOWN;
			int tail = (ActionQ.tail+1)%MAXACTIONS;
			if (tail != ActionQ.head)												// If overflow, drop uAct
				{
				ActionQ.uAct[ActionQ.tail] = prevUnicode;
				ActionQ.tail = tail;
				}
			uAct->unicode = wchars[1];
			}
		else
			uAct->unicode = wchars[0];
		}

	int tail = (ActionQ.tail+1)%MAXACTIONS;
	if (tail != ActionQ.head)														// If overflow, drop uAct
		{
		ActionQ.uAct[ActionQ.tail] = *uAct;
		ActionQ.tail = tail;
		}
	return;
	}

// The main Win32 uAct handler
LRESULT CALLBACK WinMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
	userAction uAct;

	switch (msg)
		{
	case WM_SYSKEYDOWN:
	case WM_KEYDOWN:
		{
		if (lParam&(1<<30))
			{
			if ((wParam >= VK_SHIFT && wParam <= VK_KANJI)							// No sense to repeat modifiers
				|| (wParam >= VK_F1 && wParam <= VK_F24))							// or F Keys
				return 0;
			}
	    uAct.type = WM_KEYDOWN;
		vPushEvent(&uAct, wParam, HIWORD(lParam), true);
		}
		return 0;
	case WM_SYSKEYUP:
	case WM_KEYUP:
		{
		if (wParam == VK_LWIN || wParam == VK_RWIN)									// We don't relay the Window key to DOS
			return 0;
		if (wParam != VK_SNAPSHOT)													// Windows only reports keyup for print screen, for now just drop it
			{
		    uAct.type = WM_KEYUP;
			vPushEvent(&uAct, wParam, HIWORD(lParam), false);
			}
		}
		return 0;
	case WM_MOUSEMOVE:
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
		uAct.type = msg;
		uAct.x = LOWORD(lParam);
		uAct.y = HIWORD(lParam);
		vPushEvent(&uAct, 0, 0, false);
		return 0;
	case WM_PAINT:
		{
		HDC hdc, mdc;
		PAINTSTRUCT ps;

		hdc = BeginPaint(vDosHwnd, &ps);
		mdc = CreateCompatibleDC(hdc);
		SelectObject(mdc, DIBSection);
		BitBlt(hdc, 0, 0, window.surface->w,  window.surface->h, mdc, 0, 0, SRCCOPY);
		DeleteDC(mdc);
		EndPaint(vDosHwnd, &ps);
		return 0;
		}
//	case WM_ERASEBKGND:
//		if (vga.mode != M_TEXT || !ttf.fullScrn)
//			return 0;
//		RECT rect;
//		rect.left = rect.top = 0;
//		rect.right = rect.bottom = 300;
//		ValidateRect(vDosHwnd, &rect);
//		break;
	case WM_CLOSE:
		uAct.type = WM_CLOSE;
		vPushEvent(&uAct, 0, 0, false);
		PostQuitMessage(0);
		return 0;
	case WM_DESTROY:
		PostQuitMessage(0);
	default:
		break;
		}
	return DefWindowProc(hwnd, msg, wParam, lParam);
	}
