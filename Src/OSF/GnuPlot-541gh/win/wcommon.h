/* GNUPLOT - wcommon.h */

/*[
 * Copyright 1992 - 1993, 1998, 2004 Maurice Castro, Russell Lang
 *
 * Permission to use, copy, and distribute this software and its
 * documentation for any purpose with or without fee is hereby granted,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.
 *
 * Permission to modify the software is granted, but not the right to
 * distribute the complete modified source code.  Modifications are to
 * be distributed as patches to the released version.  Permission to
 * distribute binaries produced by compiling modified sources is granted,
 * provided you
 *   1. distribute the corresponding source modifications from the
 *    released version in the form of a patch file along with the binaries,
 *   2. add special version identification to distinguish your version
 *    in addition to the base release version number,
 *   3. provide your name and address as the primary contact for the
 *    support of your modified version, and
 *   4. retain our contact information in regard to use of the base
 *    software.
 * Permission to distribute the released version of the source code along
 * with corresponding source modifications in the form of a patch file is
 * granted with same provisions 2 through 4 for binary distributions.
 *
 * This software is provided "as is" without express or implied warranty
 * to the extent permitted by applicable law.
]*/
/*
 * AUTHORS
 *   Maurice Castro
 *   Russell Lang
 */
#ifndef GNUPLOT_WCOMMON_H
#define GNUPLOT_WCOMMON_H

#include "winmain.h"

#ifndef CLEARTYPE_QUALITY
	#define CLEARTYPE_QUALITY       5
#endif
#define MAXPLOTSHIDE 10 // maximum number of plots which can be enabled/disabled via toolbar 

#ifdef __cplusplus
extern "C" {
#endif

// winmain.c 
// #define PACKVERSION(major,minor) MAKELONG(minor,major)
// (replaced with SDynLibrary::GetVersion) extern DWORD GetDllVersion(LPCTSTR lpszDllName);
extern char * appdata_directory();
extern FILE * open_printer();
extern BOOL cp_changed;
extern UINT cp_input;
extern UINT cp_output;
//
// wgnuplib.c 
//
extern HINSTANCE hdllInstance;
extern const LPCWSTR szParentClass;
extern const LPCWSTR szTextClass;
extern const LPCWSTR szToolbarClass;
extern const LPCWSTR szSeparatorClass;
extern const LPCWSTR szPauseClass;
extern LPTSTR szAboutClass;

void * LocalAllocPtr(UINT flags, UINT size);
void * LocalReAllocPtr(void * ptr, UINT flags, UINT size);
void   LocalFreePtr(void * ptr);
LPTSTR GetInt(LPTSTR str, LPINT pval);

/* wtext.c */
#ifndef WGP_CONSOLE
void WriteTextIni(TW * lptw);
void ReadTextIni(TW * lptw);
void DragFunc(TW * lptw, HDROP hdrop);
void TextShow(TW * lptw);
void TextUpdateStatus(TW * lptw);
void TextSuspend(TW * lptw);
void TextResume(TW * lptw);
void DockedUpdateLayout(TW * lptw);
void DockedGraphSize(TW * lptw, SIZE *size, BOOL newwindow);

/* wmenu.c - Menu */
void SendMacro(TW * lptw, UINT m);
void LoadMacros(TW * lptw);
void CloseMacros(TW * lptw);
#endif

/* wprinter.c - Printer setup and dump */
//extern HGLOBAL hDevNames;
//extern HGLOBAL hDevMode;

//void PrintingCleanup();
void * PrintingCallbackCreate(GP_PRINT * lpr);
void PrintingCallbackFree(void * callback);
void PrintRegister(GP_PRINT * lpr);
void PrintUnregister(GP_PRINT * lpr);
BOOL CALLBACK PrintAbortProc(HDC hdcPrn, int code);
INT_PTR CALLBACK PrintDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam); // @callback(DLGPROC)
INT_PTR CALLBACK PrintSizeDlgProc(HWND hdlg, UINT wmsg, WPARAM wparam, LPARAM lparam); // @callback(DLGPROC)

/* wgraph.c */
uint luma_from_color(uint red, uint green, uint blue);
void add_tooltip(GW * lpgw, PRECT rect, LPWSTR text);
void clear_tooltips(GW * lpgw);
void draw_update_keybox(GW * lpgw, uint plotno, int x, int y);
int draw_enhanced_text(GW * lpgw, LPRECT rect, int x, int y, const char * str);
void draw_get_enhanced_text_extend(PRECT extend);
HBITMAP GraphGetBitmap(GW * lpgw);

#ifdef __cplusplus
}
#endif

#endif /* GNUPLOT_WCOMMON_H */
