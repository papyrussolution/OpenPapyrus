#pragma once
// ButtonDefs.h
// Author: Joseph Ryan Ries, 2017
// The snip.exe that comes bundled with Microsoft Windows is *almost* good enough. So I made one just a little better.
//
// Buttons!
// You could refer to an individual button like gButtons[BUTTON_NEW - 10001], gButtons[BUTTON_DELAY - 10001], etc.

BUTTON gNewButton = {
	{ 2,  0, 72,  52 }, // Rectangle left, top, right, bottom
	L"New",             // Caption
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

BUTTON gDelayButton = {
	{ 74, 0, 144, 52 }, // Rectangle left, top, right, bottom
	L"Delay",           // Caption
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

BUTTON gSaveButton = {
	{ 146, 0, 216, 52 },
	L"Save",
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

BUTTON gCopyButton = {
	{ 218, 0, 288, 52 },
	L"Copy",
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

BUTTON gHilighterButton = {
	{ 290, 0, 360, 52 },
	L"Hilight",
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

BUTTON gRectangleButton = {
	{ 362, 0, 432, 52 },	// Rectangle left, top, right, bottom
	L"Box",					// Caption
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

BUTTON gArrowButton = {
	{ 434, 0, 504, 52 },	// Rectangle left, top, right, bottom
	L"Arrow",			    // Caption
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

BUTTON gRedactButton = {
	{ 506, 0, 576, 52 },
	L"Redact",
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

BUTTON gTextButton = {
	{ 578, 0, 648, 52 },
	L"Text",
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

BUTTON* gButtons[] = { &gNewButton, &gDelayButton, &gSaveButton, &gCopyButton, &gHilighterButton, &gRectangleButton, &gArrowButton, &gRedactButton, &gTextButton };
