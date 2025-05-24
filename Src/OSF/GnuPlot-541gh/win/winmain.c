// GNUPLOT - win/winmain.c 
// Copyright 1992, 1993, 1998, 2004   Maurice Castro, Russell Lang
// This file implements the initialization code for running gnuplot under Microsoft Windows.
// 
// The modifications to allow Gnuplot to run under Windows were made
// by Maurice Castro. (maurice@bruce.cs.monash.edu.au)  3 Jul 1992
// and Russell Lang (rjl@monu1.cc.monash.edu.au) 30 Nov 1992
// 
#include <gnuplot.h>
#pragma hdrstop
#define STRICT
#include <windowsx.h>
#include <commctrl.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <htmlhelp.h>
#include <dos.h>
#include <fcntl.h>
#include "winmain.h"
#include "wcommon.h"
#ifdef HAVE_GDIPLUS
	#include "wgdiplus.h"
#endif
#ifdef HAVE_D2D
	#include "wd2d.h"
#endif
#ifdef WXWIDGETS
	#include "wxterminal/wxt_term.h"
#endif
#ifdef HAVE_LIBCACA
	#define TERM_PUBLIC_PROTO
	#include "caca.trm"
	#undef TERM_PUBLIC_PROTO
#endif
#ifndef CSIDL_APPDATA
	#define CSIDL_APPDATA (0x001a) // workaround for old header files 
#endif
/* limits */
#define MAXSTR 255
#define MAXPRINTF 1024
/* used if vsnprintf(NULL,0,...) returns zero (MingW 3.4) */
//
// globals 
//
GpWinMainBlock _WinM;

const char * authors[] = {
	"Colin Kelley",
	"Thomas Williams"
};

void WinExit();
//static void WinCloseHelp();
int CALLBACK ShutDown();
#ifdef WGP_CONSOLE
	static int ConsolePutS(const char * str);
	static int ConsolePutCh(int ch);
#endif

static void CheckMemory(LPTSTR str)
{
	if(!str) {
		MessageBox(NULL, TEXT("out of memory"), TEXT("gnuplot"), MB_ICONSTOP | MB_OK);
		gp_exit(EXIT_FAILURE);
	}
}

int Pause(GpTermEntry * pTerm, LPSTR str)
{
	//_WinM.pausewin.Message = UnicodeText(str, encoding);
	_WinM.pausewin.Message = sstrdup(SUcSwitch(str));
	int rc = PauseBox(pTerm, &_WinM.pausewin) == IDOK;
	ZFREE(_WinM.pausewin.Message);
	return rc;
}

void kill_pending_Pause_dialog()
{
	if(_WinM.pausewin.bPause) { // no Pause dialog displayed 
		// Pause dialog displayed, thus kill it 
		DestroyWindow(_WinM.pausewin.hWndPause);
		_WinM.pausewin.bPause = FALSE;
	}
}
//
// atexit procedure 
//
void WinExit()
{
	// Last chance to close Windows help, call before anything else to avoid a crash. 
	_WinM.WinCloseHelp();
	// clean-up call for printing system 
	_WinM.PrintingCleanup();
	GPO.TermReset(GPT.P_Term);
	_fcloseall();
	// Close all graph windows 
	for(GW * lpgw = _WinM.P_ListGraphs; lpgw; lpgw = lpgw->next) {
		if(GraphHasWindow(lpgw))
			GraphClose(lpgw);
	}
#ifndef WGP_CONSOLE
	TextMessage(); /* process messages */
#ifndef __WATCOMC__
	// revert C++ stream redirection 
	RedirectOutputStreams(FALSE);
#endif
#endif
#ifdef HAVE_GDIPLUS
	gdiplusCleanup();
#endif
#ifdef HAVE_D2D
	d2dCleanup();
#endif
	CoUninitialize();
}
//
// call back function from Text Window WM_CLOSE 
//
int CALLBACK ShutDown()
{
	// First chance for wgnuplot to close help system. 
	_WinM.WinCloseHelp();
	gp_exit(EXIT_SUCCESS);
	return 0;
}
#if 0 // (replaced with SDynLibrary::GetVersion) {
//
// This function can be used to retrieve version information from
// Window's Shell and common control libraries (Comctl32.dll,
// Shell32.dll, and Shlwapi.dll) The code was copied from the MSDN
// article "Shell and Common Controls Versions" 
// 
DWORD GetDllVersion(LPCTSTR lpszDllName)
{
	DWORD dwVersion = 0;
	// For security purposes, LoadLibrary should be provided with a
	// fully-qualified path to the DLL. The lpszDllName variable should be
	// tested to ensure that it is a fully qualified path before it is used. */
	HINSTANCE hinstDll = LoadLibrary(lpszDllName);
	if(hinstDll) {
		DLLGETVERSIONPROC pDllGetVersion = (DLLGETVERSIONPROC)GetProcAddress(hinstDll, "DllGetVersion");
		/* Because some DLLs might not implement this function, you
		   must test for it explicitly. Depending on the particular
		   DLL, the lack of a DllGetVersion function can be a useful
		   indicator of the version. */
		if(pDllGetVersion) {
			DLLVERSIONINFO dvi;
			HRESULT hr;
			INITWINAPISTRUCT(dvi);
			hr = (*pDllGetVersion)(&dvi);
			if(SUCCEEDED(hr))
				dwVersion = PACKVERSION(dvi.dwMajorVersion, dvi.dwMinorVersion);
		}
		FreeLibrary(hinstDll);
	}
	return dwVersion;
}
#endif // } (replaced with SDynLibrary::GetVersion)

BOOL IsWindowsXPorLater()
{
	/* @v12.3.4
	OSVERSIONINFO versionInfo;
	// get Windows version 
	memzero(&versionInfo, sizeof(OSVERSIONINFO));
	versionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx(&versionInfo);
	return ((versionInfo.dwMajorVersion > 5) || ((versionInfo.dwMajorVersion == 5) && (versionInfo.dwMinorVersion >= 1)));
	*/
	return IsWindowsXPOrGreater(); // @v12.3.4
}

char * appdata_directory()
{
	static char dir[MAX_PATH] = "";
	if(dir[0])
		return dir;
	// FIMXE: "ANSI" Version, no Unicode support 
	// Make sure that SHGetSpecialFolderPath is supported. 
	HMODULE hShell32 = LoadLibrary(TEXT("shell32.dll"));
	if(hShell32) {
		typedef BOOL (WINAPI * SHGETSPECIALFOLDERPATHA)(HWND hwndOwner, LPSTR lpszPath, int nFolder, BOOL fCreate);
		SHGETSPECIALFOLDERPATHA pSHGetSpecialFolderPath = (SHGETSPECIALFOLDERPATHA)GetProcAddress(hShell32, "SHGetSpecialFolderPathA");
		if(pSHGetSpecialFolderPath)
			(*pSHGetSpecialFolderPath)(NULL, dir, CSIDL_APPDATA, FALSE);
		FreeModule(hShell32);
		return dir;
	}
	if(!dir[0]) { // use APPDATA environment variable as fallback 
		char * appdata = getenv("APPDATA");
		if(appdata) {
			strcpy(dir, appdata);
			return dir;
		}
	}
	return NULL;
}
//
// retrieve path relative to gnuplot executable 
//
LPSTR RelativePathToGnuplot(const char * path)
{
#ifdef UNICODE
	LPSTR ansi_dir = AnsiText(_WinM.szPackageDir, GPT._Encoding);
	LPSTR rel_path = (char *)SAlloc::R(ansi_dir, strlen(ansi_dir) + strlen(path) + 1);
	if(rel_path == NULL) {
		SAlloc::F(ansi_dir);
		return (LPSTR)path;
	}
#else
	char * rel_path = (char *)SAlloc::M(strlen(szPackageDir) + strlen(path) + 1);
	strcpy(rel_path, szPackageDir);
#endif
	// szPackageDir is guaranteed to have a trailing backslash 
	strcat(rel_path, path);
	return rel_path;
}

void GpWinMainBlock::WinCloseHelp()
{
	// Due to a known bug in the HTML help system we have to
	// call this as soon as possible before the end of the program.
	// See e.g. http://helpware.net/FAR/far_faq.htm#HH_CLOSE_ALL
	if(IsWindow(help_window))
		SendMessage(help_window, WM_CLOSE, 0, 0);
	Sleep(0);
}

static LPTSTR GetLanguageCode()
{
	static TCHAR lang[6] = TEXT("");
	if(!lang[0]) {
		GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SABBREVLANGNAME, lang, sizeof(lang));
		//strcpy(lang, "JPN"); //TEST
		// language definition files for Japanese already use "ja" as abbreviation 
		if(_tcscmp(lang, TEXT("JPN")) == 0)
			lang[1] = 'A';
		// prefer lower case 
		lang[0] = tolower((uchar)lang[0]);
		lang[1] = tolower((uchar)lang[1]);
		// only use two character sequence 
		lang[2] = '\0';
	}
	return lang;
}

LPTSTR GpWinMainBlock::LocalisedFile(LPCTSTR name, LPCTSTR ext, LPCTSTR defaultname)
{
	// Allow user to override language detection. 
	LPTSTR lang = NZOR(szLanguageCode, GetLanguageCode());
	LPTSTR filename = (LPTSTR)SAlloc::M((_tcslen(szModuleName) + _tcslen(name) + _tcslen(lang) + _tcslen(ext) + 1) * sizeof(TCHAR));
	if(filename) {
		_tcscpy(filename, szModuleName);
		_tcscat(filename, name);
		_tcscat(filename, lang);
		_tcscat(filename, ext);
		if(!PathFileExists(filename)) {
			_tcscpy(filename, szModuleName);
			_tcscat(filename, defaultname);
		}
	}
	return filename;
}

void GpWinMainBlock::ReadMainIni(LPCTSTR file, LPCTSTR section)
{
	TCHAR profile[81] = TEXT("");
	const TCHAR hlpext[] = TEXT(".chm");
	const TCHAR name[] = TEXT("wgnuplot-");
	// Language code override 
	GetPrivateProfileString(section, TEXT("Language"), TEXT(""), profile, 80, file);
	szLanguageCode = profile[0] ? _tcsdup(profile) : NULL;
	// help file name 
	GetPrivateProfileString(section, TEXT("HelpFile"), TEXT(""), profile, 80, file);
	if(profile[0]) {
		winhelpname = (LPTSTR)SAlloc::M((_tcslen(szModuleName) + _tcslen(profile) + 1) * sizeof(TCHAR));
		if(winhelpname) {
			_tcscpy(winhelpname, szModuleName);
			_tcscat(winhelpname, profile);
		}
	}
	else {
		// default name is "wgnuplot-LL.chm" 
		winhelpname = LocalisedFile(name, hlpext, TEXT(HELPFILE));
	}
	// menu file name 
	GetPrivateProfileString(section, TEXT("MenuFile"), TEXT(""), profile, 80, file);
	if(profile[0]) {
		szMenuName = (LPTSTR)SAlloc::M((_tcslen(szModuleName) + _tcslen(profile) + 1) * sizeof(TCHAR));
		if(szMenuName) {
			_tcscpy(szMenuName, szModuleName);
			_tcscat(szMenuName, profile);
		}
	}
	else {
		// default name is "wgnuplot-LL.mnu" 
		szMenuName = LocalisedFile(name, TEXT(".mnu"), TEXT("wgnuplot.mnu"));
	}
}

#ifndef WGP_CONSOLE
#ifndef __WATCOMC__
int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
#else
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
#endif
#else
int _tmain(int argc, TCHAR ** argv)
#endif
{
	LPTSTR tail;
	int i;
	SLS.Init("GnuPlot"); // @v11.1.12
#ifdef WGP_CONSOLE
	HINSTANCE hInstance = GetModuleHandle(NULL), hPrevInstance = NULL;
#endif
#ifndef _UNICODE
	#ifndef WGP_CONSOLE
		#if defined(__MINGW32__) && !defined(_W64)
			#define argc _argc
			#define argv _argv
		#else /* MSVC, WATCOM, MINGW-W64 */
			#define argc __argc
			#define argv __argv
		#endif
	#endif /* WGP_CONSOLE */
#else
	#define argc __argc
	#define argv argv_u8
	// create an UTF-8 encoded copy of all arguments 
	char ** argv_u8 = (char **)SAlloc::C(__argc, sizeof(char *));
	for(i = 0; i < __argc; i++)
		argv_u8[i] = AnsiText(__wargv[i], S_ENC_UTF8);
#endif
	GnuPlot * p_gp = &GPO; // !!! GnuPlot entry point
	_WinM.szModuleName = (LPTSTR)SAlloc::M((MAXSTR + 1) * sizeof(TCHAR));
	CheckMemory(_WinM.szModuleName);
	// get path to gnuplot executable  
	GetModuleFileName(hInstance, _WinM.szModuleName, MAXSTR);
	tail = _tcsrchr(_WinM.szModuleName, '\\');
	if(tail) {
		tail++;
		*tail = 0;
	}
	_WinM.szModuleName = (LPTSTR)SAlloc::R(_WinM.szModuleName, (_tcslen(_WinM.szModuleName) + 1) * sizeof(TCHAR));
	CheckMemory(_WinM.szModuleName);
	if(_tcslen(_WinM.szModuleName) >= 5 && _tcsnicmp(&_WinM.szModuleName[_tcslen(_WinM.szModuleName)-5], TEXT("\\bin\\"), 5) == 0) {
		size_t len = _tcslen(_WinM.szModuleName) - 4;
		_WinM.szPackageDir = (LPTSTR)SAlloc::M((len + 1) * sizeof(TCHAR));
		CheckMemory(_WinM.szPackageDir);
		_tcsncpy(_WinM.szPackageDir, _WinM.szModuleName, len);
		_WinM.szPackageDir[len] = '\0';
	}
	else {
		_WinM.szPackageDir = _WinM.szModuleName;
	}
#ifndef WGP_CONSOLE
	_WinM.TxtWin.hInstance = hInstance;
	_WinM.TxtWin.hPrevInstance = hPrevInstance;
	_WinM.TxtWin.nCmdShow = nCmdShow;
	_WinM.TxtWin.Title = L"gnuplot";
#endif
	// create structure of first graph window 
	_WinM.P_GraphWin = (GW *)SAlloc::C(1, sizeof(GW));
	_WinM.P_ListGraphs = _WinM.P_GraphWin;
	// locate ini file 
	{
		char * inifile;
#ifdef UNICODE
		LPWSTR winifile;
#endif
		p_gp->GetUserEnv(); /* this hasn't been called yet */
		inifile = sstrdup("~\\wgnuplot.ini");
		p_gp->GpExpandTilde(&inifile);
		// if tilde expansion fails use current directory as default - that was the previous default behaviour 
		if(inifile[0] == '~') {
			FREEANDASSIGN(inifile, "wgnuplot.ini");
		}
#ifdef UNICODE
		_WinM.P_GraphWin->IniFile = winifile = UnicodeText(inifile, S_ENC_DEFAULT);
#else
		_WinM.P_GraphWin->IniFile = inifile;
#endif
#ifndef WGP_CONSOLE
		_WinM.TxtWin.IniFile = _WinM.P_GraphWin->IniFile;
#endif
		_WinM.ReadMainIni(_WinM.P_GraphWin->IniFile, TEXT("WGNUPLOT"));
	}
#ifndef WGP_CONSOLE
	_WinM.TxtWin.IniSection = TEXT("WGNUPLOT");
	_WinM.TxtWin.DragPre = L"load '";
	_WinM.TxtWin.DragPost = L"'\n";
	_WinM.TxtWin.P_LpMw = &_WinM.MnuWin;
	_WinM.TxtWin.ScreenSize.x = 80;
	_WinM.TxtWin.ScreenSize.y = 80;
	_WinM.TxtWin.KeyBufSize = 2048;
	_WinM.TxtWin.CursorFlag = 1; /* scroll to cursor after \n & \r */
	_WinM.TxtWin.shutdown = MakeProcInstance((FARPROC)ShutDown, hInstance);
	_WinM.TxtWin.AboutText = (LPTSTR)SAlloc::M(1024 * sizeof(TCHAR));
	CheckMemory(_WinM.TxtWin.AboutText);
	wsprintf(_WinM.TxtWin.AboutText, TEXT("Version %hs patchlevel %hs\n") \
	    TEXT("last modified %hs\n%hs\n%hs, %hs and many others\n""gnuplot home:     http://www.gnuplot.info\n"),
	    gnuplot_version, gnuplot_patchlevel, gnuplot_date, gnuplot_copyright, authors[1], authors[0]);
	_WinM.TxtWin.AboutText = (LPTSTR)SAlloc::R(_WinM.TxtWin.AboutText, (_tcslen(_WinM.TxtWin.AboutText) + 1) * sizeof(TCHAR));
	CheckMemory(_WinM.TxtWin.AboutText);
	_WinM.MnuWin.szMenuName = _WinM.szMenuName;
#endif
	_WinM.pausewin.hInstance = hInstance;
	_WinM.pausewin.hPrevInstance = hPrevInstance;
	_WinM.pausewin.Title = L"gnuplot pause";
	_WinM.P_GraphWin->hInstance = hInstance;
	_WinM.P_GraphWin->hPrevInstance = hPrevInstance;
#ifdef WGP_CONSOLE
	P_GraphWin->lptw = NULL;
#else
	_WinM.P_GraphWin->lptw = &_WinM.TxtWin;
#endif
	// COM Initialization 
	if(!SUCCEEDED(CoInitialize(NULL))) {
		// FIXME: Need to abort
	}
	// init common controls 
	{
		INITCOMMONCONTROLSEX initCtrls;
		initCtrls.dwSize = sizeof(INITCOMMONCONTROLSEX);
		initCtrls.dwICC = ICC_WIN95_CLASSES;
		InitCommonControlsEx(&initCtrls);
	}
#ifndef WGP_CONSOLE
	if(TextInit(&_WinM.TxtWin))
		gp_exit(EXIT_FAILURE);
	_WinM.TxtWin.hIcon = LoadIcon(hInstance, TEXT("TEXTICON"));
	SetClassLongPtr(_WinM.TxtWin.hWndParent, GCLP_HICON, (LONG_PTR)_WinM.TxtWin.hIcon);
	// Note: we want to know whether this is an interactive session so that we can
	// decide whether or not to write status information to stderr.  The old test
	// for this was to see if (argc > 1) but the addition of optional command line
	// switches broke this.  What we really wanted to know was whether any of the
	// command line arguments are file names or an explicit in-line "-e command".
	// (This is a copy of a code snippet from plot.c)
	for(i = 1; i < argc; i++) {
		if(sstreqi_ascii(argv[i], "/noend"))
			continue;
		if((argv[i][0] != '-') || (argv[i][1] == 'e')) {
			p_gp->_Plt.interactive = false;
			break;
		}
	}
	if(p_gp->_Plt.interactive)
		ShowWindow(_WinM.TxtWin.hWndParent, _WinM.TxtWin.nCmdShow);
	if(IsIconic(_WinM.TxtWin.hWndParent)) { // update icon 
		RECT rect;
		GetClientRect(_WinM.TxtWin.hWndParent, (LPRECT)&rect);
		InvalidateRect(_WinM.TxtWin.hWndParent, (LPRECT)&rect, 1);
		UpdateWindow(_WinM.TxtWin.hWndParent);
	}
#ifndef __WATCOMC__
	// Finally, also redirect C++ standard output streams. 
	RedirectOutputStreams(TRUE);
#endif
#else  /* !WGP_CONSOLE */
	#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
		#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
	#endif
	{
		/* Enable Windows 10 Console Virtual Terminal Sequences */
		HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
		DWORD mode;
		GetConsoleMode(handle, &mode);
		SetConsoleMode(handle, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
	}
	// set console mode handler to catch "abort" signals
	SetConsoleCtrlHandler(ConsoleHandler, TRUE);
#endif
	gp_atexit(WinExit);
	if(!_isatty(_fileno(stdin)))
		_setmode(_fileno(stdin), O_BINARY);
	//gnu_main(argc, argv);
	p_gp->ImplementMain(argc, argv);
	// First chance to close help system for console gnuplot, second for wgnuplot 
	_WinM.WinCloseHelp();
	gp_exit_cleanup();
	return 0;
}

void MultiByteAccumulate(BYTE ch, LPWSTR wstr, int * count)
{
	static char mbstr[4] = "";
	static int mbwait = 0;
	static int mbcount = 0;
	*count = 0;
	/* try to re-sync on control characters */
	/* works for utf8 and sjis */
	if(ch < 32) {
		mbwait = mbcount = 0;
		mbstr[0] = '\0';
	}
	if(GPT._Encoding == S_ENC_UTF8) { // combine UTF8 byte sequences
		if(mbwait == 0) {
			// first byte 
			mbcount = 0;
			mbstr[mbcount] = ch;
			if((ch & 0xE0) == 0xC0) {
				mbwait = 1; // expect one more byte
			}
			else if((ch & 0xF0) == 0xE0) {
				mbwait = 2; // expect two more bytes
			}
			else if((ch & 0xF8) == 0xF0) {
				mbwait = 3; // expect three more bytes
			}
		}
		else {
			/* subsequent byte */
			/*assert((ch & 0xC0) == 0x80);*/
			if((ch & 0xC0) == 0x80) {
				mbcount++;
				mbwait--;
			}
			else {
				/* invalid sequence */
				mbcount = 0;
				mbwait = 0;
			}
			mbstr[mbcount] = ch;
		}
		if(mbwait == 0) {
			*count = MultiByteToWideChar(CP_UTF8, 0, mbstr, mbcount + 1, wstr, 2);
		}
	}
	else if(GPT._Encoding == S_ENC_SJIS) { // combine S-JIS sequences 
		if(mbwait == 0) {
			// first or single byte 
			mbcount = 0;
			mbstr[mbcount] = ch;
			if(is_sjis_lead_byte(ch)) {
				mbwait = 1; /* first byte */
			}
		}
		else {
			if((ch >= 0x40) && (ch <= 0xfc)) {
				mbcount++; /* valid */
			}
			else {
				mbcount = 0; /* invalid */
			}
			mbwait = 0; /* max. double byte sequences */
			mbstr[mbcount] = ch;
		}
		if(mbwait == 0) {
			*count = MultiByteToWideChar(932, 0, mbstr, mbcount + 1, wstr, 2);
		}
	}
	else {
		mbcount = 0;
		mbwait = 0;
		mbstr[0] = (char)ch;
		*count = MultiByteToWideChar(WinGetCodepage(GPT._Encoding), 0, mbstr, mbcount + 1, wstr, 2);
	}
}

/* replacement stdio routines that use
 *  -  Text Window for stdin/stdout (wgnuplot)
 *  -  Unicode console APIs to handle encodings (console gnuplot)
 * WARNING: Do not write to stdout/stderr with functions not listed
 * in win/wtext.h
 */

#undef kbhit
#undef getche
#undef getch
#undef putch

#undef fgetc
#undef getchar
#undef getc
#undef fgets
#undef gets

#undef fputc
#undef putchar
#undef putc
#undef fputs
#undef puts

#undef fprintf
#undef printf
#undef vprintf
#undef vfprintf
#undef fwrite
#undef fread

#ifndef WGP_CONSOLE
	#define TEXTMESSAGE TextMessage()
	#define GETCH() TextGetChE(&_WinM.TxtWin)
	#define PUTS(s) TextPutS(&_WinM.TxtWin, (char *)s)
	#define PUTCH(c) TextPutCh(&_WinM.TxtWin, (BYTE)c)
	#define isterm(f) oneof3(f, stdin, stdout, stderr)
#else
	#define TEXTMESSAGE
	#define GETCH() ConsoleReadCh()
	#define PUTS(s) ConsolePutS(s)
	#define PUTCH(c) ConsolePutCh(c)
	#define isterm(f) _isatty(_fileno(f))
#endif
int MyPutCh(int ch) { return PUTCH(ch); }
#ifndef WGP_CONSOLE
	int MyKBHit() { return _WinM.TxtWin.TextKBHit(); }
	int MyGetCh() { return _WinM.TxtWin.TextGetCh(); }
	int MyGetChE() { return TextGetChE(&_WinM.TxtWin); }
#endif
int MyFGetC(FILE * file) { return isterm(file) ? GETCH() : fgetc(file); }

char * MyGetS(char * str)
{
	MyFGetS(str, 80, stdin);
	if(strlen(str) > 0 && str[strlen(str)-1] == '\n')
		str[strlen(str)-1] = '\0';
	return str;
}

char * MyFGetS(char * str, uint size, FILE * file)
{
	if(isterm(file)) {
#ifndef WGP_CONSOLE
		char * p = TextGetS(&_WinM.TxtWin, str, size);
		return p ? str : NULL;
#else
		uint i;
		int c = ConsoleGetch();
		if(c == EOF)
			return NULL;
		for(i = 1; i < size - 1; i++) {
			c = ConsoleGetch();
			if(str[i] == EOF)
				break;
			str[i] = c;
			if(str[i] == '\n')
				break;
		}
		str[i] = '\0';
		return str;
#endif
	}
	return fgets(str, size, file);
}

int FASTCALL MyFPutC(int ch, FILE * file)
{
	if(isterm(file)) {
		PUTCH(ch);
		TEXTMESSAGE;
		return ch;
	}
	else
		return fputc(ch, file);
}

int FASTCALL MyFPutS(const char * str, FILE * file)
{
	if(isterm(file)) {
		PUTS(str);
		TEXTMESSAGE;
		return (*str);
	}
	else
		return fputs(str, file);
}

int MyPutS(const char * str)
{
	PUTS(str);
	PUTCH('\n');
	TEXTMESSAGE;
	return 0;
}

int MyFPrintF(FILE * file, const char * fmt, ...)
{
	int count;
	va_list args;
	va_start(args, fmt);
	if(isterm(file)) {
		char * buf;
		count = vsnprintf(NULL, 0, fmt, args) + 1;
		SETIFZ(count, MAXPRINTF);
		va_end(args);
		va_start(args, fmt);
		buf = (char *)SAlloc::M(count * sizeof(char));
		count = vsnprintf(buf, count, fmt, args);
		PUTS(buf);
		SAlloc::F(buf);
	}
	else {
		count = vfprintf(file, fmt, args);
	}
	va_end(args);
	return count;
}

int MyVFPrintF(FILE * file, const char * fmt, va_list args)
{
	int count;
	if(isterm(file)) {
		char * buf;
		va_list args_copied;
		va_copy(args_copied, args);
		count = vsnprintf(NULL, 0U, fmt, args) + 1;
		SETIFZ(count, MAXPRINTF);
		va_end(args_copied);
		buf = (char *)SAlloc::M(count * sizeof(char));
		count = vsnprintf(buf, count, fmt, args);
		PUTS(buf);
		SAlloc::F(buf);
	}
	else {
		count = vfprintf(file, fmt, args);
	}
	return count;
}

int MyPrintF(const char * fmt, ...)
{
	int count;
	char * buf;
	va_list args;
	va_start(args, fmt);
	count = vsnprintf(NULL, 0, fmt, args) + 1;
	SETIFZ(count, MAXPRINTF);
	va_end(args);
	va_start(args, fmt);
	buf = (char *)SAlloc::M(count * sizeof(char));
	count = vsnprintf(buf, count, fmt, args);
	PUTS(buf);
	SAlloc::F(buf);
	va_end(args);
	return count;
}

size_t MyFWrite(const void * ptr, size_t size, size_t n, FILE * file)
{
	if(isterm(file)) {
		for(size_t i = 0; i < n; i++)
			PUTCH(((BYTE*)ptr)[i]);
		TEXTMESSAGE;
		return n;
	}
	return fwrite(ptr, size, n, file);
}

size_t MyFRead(void * ptr, size_t size, size_t n, FILE * file)
{
	if(isterm(file)) {
		for(size_t i = 0; i < n; i++)
			((BYTE*)ptr)[i] = GETCH();
		TEXTMESSAGE;
		return n;
	}
	return fread(ptr, size, n, file);
}

#ifdef USE_FAKEPIPES

static char pipe_type = '\0';
static char * pipe_filename = NULL;
static char * pipe_command = NULL;

FILE * fake_popen(const char * command, const char * type)
{
	FILE * f = NULL;
	char tmppath[MAX_PATH];
	char tmpfile[MAX_PATH];
	DWORD ret;
	if(type == NULL)
		return NULL;
	pipe_type = '\0';
	SAlloc::F(pipe_filename);
	// Random temp file name in %TEMP% 
	ret = GetTempPathA(sizeof(tmppath), tmppath);
	if((ret == 0) || (ret > sizeof(tmppath)))
		return NULL;
	ret = GetTempFileNameA(tmppath, "gpp", 0, tmpfile);
	if(ret == 0)
		return NULL;
	pipe_filename = sstrdup(tmpfile);
	if(*type == 'r') {
		char * cmd;
		int rc;
		LPWSTR wcmd;
		pipe_type = *type;
		/* Execute command with redirection of stdout to temporary file. */
#ifndef HAVE_BROKEN_WSYSTEM
		cmd = (char *)SAlloc::M(strlen(command) + strlen(pipe_filename) + 5);
		sprintf(cmd, "%s > %s", command, pipe_filename);
		wcmd = UnicodeText(cmd, encoding);
		rc = _wsystem(wcmd);
		SAlloc::F(wcmd);
#else
		cmd = (char *)SAlloc::M(strlen(command) + strlen(pipe_filename) + 15);
		sprintf(cmd, "cmd /c %s > %s", command, pipe_filename);
		rc = system(cmd);
#endif
		SAlloc::F(cmd);
		/* Now open temporary file. */
		/* system() returns 1 if the command could not be executed. */
		if(rc != 1) {
			f = fopen(pipe_filename, "r");
		}
		else {
			remove(pipe_filename);
			ZFREE(pipe_filename);
			errno = EINVAL;
		}
	}
	else if(*type == 'w') {
		pipe_type = *type;
		/* Write output to temporary file and handle the rest in fake_pclose. */
		if(type[1] == 'b')
			GPO.IntError(NO_CARET, "Could not execute pipe '%s'. Writing to binary pipes is not supported.", command);
		else
			f = fopen(pipe_filename, "w");
		pipe_command = sstrdup(command);
	}
	return f;
}

int fake_pclose(FILE * stream)
{
	int rc = 0;
	if(!stream)
		return ECHILD;
	// Close temporary file 
	fclose(stream);
	// Finally, execute command with redirected stdin. 
	if(pipe_type == 'w') {
		char * cmd;
		LPWSTR wcmd;
#ifndef HAVE_BROKEN_WSYSTEM
		cmd = (char *)SAlloc::M(strlen(pipe_command) + strlen(pipe_filename) + 10, "fake_pclose");
		// FIXME: this won't work for binary data. We need a proper `cat` replacement. 
		sprintf(cmd, "type %s | %s", pipe_filename, pipe_command);
		wcmd = UnicodeText(cmd, encoding);
		rc = _wsystem(wcmd);
		SAlloc::F(wcmd);
#else
		cmd = (char *)SAlloc::M(strlen(pipe_command) + strlen(pipe_filename) + 20, "fake_pclose");
		sprintf(cmd, "cmd/c type %s | %s", pipe_filename, pipe_command);
		rc = system(cmd);
#endif
		SAlloc::F(cmd);
	}
	// Delete temp file again. 
	if(pipe_filename) {
		remove(pipe_filename);
		errno = 0;
		ZFREE(pipe_filename);
	}
	if(pipe_command) {
		// system() returns 255 if the command could not be executed.
		// The real popen would have returned an error already. 
		if(rc == 255)
			GPO.IntError(NO_CARET, "Could not execute pipe '%s'.", pipe_command);
		SAlloc::F(pipe_command);
	}
	return rc;
}
#endif

#ifdef WGP_CONSOLE

int ConsoleGetch()
{
	int fd = _fileno(stdin);
	DWORD waitResult;
	HANDLE h = (HANDLE)_get_osfhandle(fd);
	if(h == INVALID_HANDLE_VALUE)
		fprintf(stderr, "ERROR: Invalid stdin handle value!\n");

	do {
		waitResult = MsgWaitForMultipleObjects(1, &h, FALSE, INFINITE, QS_ALLINPUT);
		if(waitResult == WAIT_OBJECT_0) {
			if(_isatty(fd)) {
				DWORD c = ConsoleReadCh();
				if(c)
					return c;
			}
			else {
				uchar c;
				if(fread(&c, 1, 1, stdin) == 1)
					return c;
				else
					return EOF;
			}
		}
		else if(waitResult == WAIT_OBJECT_0+1) {
			WinMessageLoop();
			if(ctrlc_flag)
				return '\r';
		}
		else
			break;
	} while(1);

	return '\r';
}

#endif /* WGP_CONSOLE */

int ConsoleReadCh()
{
	const int max_input = 8;
	static char console_input[8];
	static int first_input_char = 0;
	static int last_input_char = 0;
	INPUT_RECORD rec;
	DWORD recRead;
	HANDLE h;
	if(first_input_char != last_input_char) {
		int c = console_input[first_input_char];
		first_input_char++;
		first_input_char %= max_input;
		return c;
	}
	h = GetStdHandle(STD_INPUT_HANDLE);
	if(!h)
		return '\0';
	ReadConsoleInputW(h, &rec, 1, &recRead);
	/* FIXME: We should handle rec.Event.KeyEvent.wRepeatCount > 1, too. */
	if(recRead == 1 && rec.EventType == KEY_EVENT && rec.Event.KeyEvent.bKeyDown &&
	    (rec.Event.KeyEvent.wVirtualKeyCode < VK_SHIFT || rec.Event.KeyEvent.wVirtualKeyCode > VK_MENU)) {
		if(rec.Event.KeyEvent.uChar.UnicodeChar) {
			if((rec.Event.KeyEvent.dwControlKeyState == SHIFT_PRESSED) && (rec.Event.KeyEvent.wVirtualKeyCode == VK_TAB)) {
				return 034; /* remap Shift-Tab */
			}
			else {
				char mbchar[8];
				const int count = WideCharToMultiByte(WinGetCodepage(GPT._Encoding), 0, &rec.Event.KeyEvent.uChar.UnicodeChar, 1, mbchar, sizeof(mbchar), NULL, NULL);
				for(int i = 1; i < count; i++) {
					console_input[last_input_char] = mbchar[i];
					last_input_char++;
					last_input_char %= max_input;
				}
				return mbchar[0];
			}
		}
		else {
			switch(rec.Event.KeyEvent.wVirtualKeyCode) {
				case VK_UP: return 020;
				case VK_DOWN: return 016;
				case VK_LEFT: return 002;
				case VK_RIGHT: return 006;
				case VK_HOME: return 001;
				case VK_END: return 005;
				case VK_DELETE: return 0117;
			}
		}
	}
	/* Error reading event or, key up or, one of the following event records:
	    MOUSE_EVENT_RECORD, WINDOW_BUFFER_SIZE_RECORD, MENU_EVENT_RECORD, FOCUS_EVENT_RECORD */
	return '\0';
}

#ifdef WGP_CONSOLE

static int ConsolePutS(const char * str)
{
	LPWSTR wstr = UnicodeText(str, encoding);
	// Use standard file IO instead of Console API
	// to enable word-wrapping on Windows 10 and
	// allow for redirection of stdout/stderr.
	//HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
	//WriteConsoleW(h, wstr, wcslen(wstr), NULL, NULL);
	fputws(wstr, stdout);
	SAlloc::F(wstr);
	return 0;
}

static int ConsolePutCh(int ch)
{
	WCHAR w[4];
	int count;
	MultiByteAccumulate(ch, w, &count);
	if(count > 0) {
		// Use standard file IO instead of Console API
		// to enable word-wrapping on Windows 10.
		//HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
		//WriteConsoleW(h, w, count, NULL, NULL);
		w[count] = 0;
		fputws(w, stdout);
	}
	return ch;
}

#endif
//
// This is called by the system to signal various events.
// Note that it is executed in a separate thread.  
//
BOOL WINAPI ConsoleHandler(DWORD dwType)
{
	switch(dwType) {
		case CTRL_CLOSE_EVENT:
		case CTRL_LOGOFF_EVENT:
		case CTRL_SHUTDOWN_EVENT: {
#ifdef WGP_CONSOLE
		    HANDLE h;
		    INPUT_RECORD rec;
		    DWORD written;
#endif

		    // NOTE: returning from this handler terminates the application.
		    // Instead, we signal the main thread to clean up and exit and
		    // then idle by sleeping.
#ifndef WGP_CONSOLE
		    // close the main window to exit gnuplot
		    PostMessage(_WinM.TxtWin.hWndParent, WM_CLOSE, 0, 0);
#else
		    terminate_flag = TRUE;
		    // send ^D to main thread input queue
		    h = GetStdHandle(STD_INPUT_HANDLE);
		    memzero(&rec, sizeof(rec));
		    rec.EventType = KEY_EVENT;
		    rec.Event.KeyEvent.bKeyDown = TRUE;
		    rec.Event.KeyEvent.wRepeatCount = 1;
		    rec.Event.KeyEvent.uChar.AsciiChar = 004;
		    WriteConsoleInput(h, &rec, 1, &written);
#endif
		    // give the main thread time to exit
		    Sleep(10000);
		    return TRUE;
	    }
		default:
		    break;
	}
	return FALSE;
}

/* public interface to printer routines : Windows PRN emulation
 * (formerly in win.trm)
 */

#define MAX_PRT_LEN 256
static char win_prntmp[MAX_PRT_LEN+1];

FILE * open_printer()
{
	char * temp;
	if((temp = getenv("TEMP")) == NULL)
		*win_prntmp = '\0';
	else {
		strnzcpy(win_prntmp, temp, MAX_PRT_LEN);
		// stop X's in path being converted by _mktemp 
		for(temp = win_prntmp; *temp; temp++)
			*temp = tolower((uchar)*temp);
		if(strlen(win_prntmp) && win_prntmp[strlen(win_prntmp)-1] != '\\')
			strcat(win_prntmp, "\\");
	}
	strncat(win_prntmp, "_gptmp", MAX_PRT_LEN - strlen(win_prntmp));
	strncat(win_prntmp, "XXXXXX", MAX_PRT_LEN - strlen(win_prntmp));
	_mktemp(win_prntmp);
	return fopen(win_prntmp, "wb");
}

//void close_printer(FILE * outfile)
void GnuPlot::ClosePrinter(GpTermEntry * pTerm, FILE * outfile)
{
	HWND hwnd;
	TCHAR title[100];
//#ifdef UNICODE
	//LPTSTR fname = UnicodeText(win_prntmp, S_ENC_DEFAULT);
//#else
	//LPTSTR fname = win_prntmp;
//#endif
	fclose(outfile);
#ifndef WGP_CONSOLE
	hwnd = _WinM.TxtWin.hWndParent;
#else
	hwnd = GetDesktopWindow();
#endif
	if(pTerm->GetName())
		wsprintf(title, TEXT("gnuplot graph (%hs)"), pTerm->GetName());
	else
		_tcscpy(title, TEXT("gnuplot graph"));
	DumpPrinter(hwnd, title, SUcSwitch(win_prntmp));
//#ifdef UNICODE
	//SAlloc::F(fname);
//#endif
}

//void screen_dump()
void GnuPlot::ScreenDump(GpTermEntry * pTerm)
{
	if(!pTerm) {
		IntErrorCurToken("");
	}
	else {
		if(sstreq(pTerm->GetName(), "windows"))
			GraphPrint(_WinM.P_GraphWin);
#ifdef WXWIDGETS
		else if(sstreq(pTerm->GetName(), "wxt"))
			wxt_screen_dump();
#endif
#ifdef QTTERM
		//else if (sstreq(pTerm->GetName(), "qt"))
#endif
		else
			IntErrorCurToken("screendump not supported for terminal `%s`", pTerm->GetName());
	}
}

void win_raise_terminal_window(int id)
{
	GW * lpgw = _WinM.P_ListGraphs;
	while(lpgw && lpgw->Id != id)
		lpgw = lpgw->next;
	if(lpgw) {
		if(IsIconic(lpgw->hWndGraph))
			ShowWindow(lpgw->hWndGraph, SW_SHOWNORMAL);
		BringWindowToTop(lpgw->hWndGraph);
	}
}

void win_raise_terminal_group()
{
	for(GW * lpgw = _WinM.P_ListGraphs; lpgw; lpgw = lpgw->next) {
		if(IsIconic(lpgw->hWndGraph))
			ShowWindow(lpgw->hWndGraph, SW_SHOWNORMAL);
		BringWindowToTop(lpgw->hWndGraph);
	}
}

void win_lower_terminal_window(int id)
{
	GW * lpgw = _WinM.P_ListGraphs;
	while(lpgw && lpgw->Id != id)
		lpgw = lpgw->next;
	if(lpgw)
		SetWindowPos(lpgw->hWndGraph, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE);
}

void win_lower_terminal_group()
{
	for(GW * lpgw = _WinM.P_ListGraphs; lpgw; lpgw = lpgw->next) {
		SetWindowPos(lpgw->hWndGraph, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE);
	}
}
//
// returns true if there are any graph windows open (win terminal) 
//
static bool WinWindowOpened()
{
	for(GW * lpgw = _WinM.P_ListGraphs; lpgw; lpgw = lpgw->next) {
		if(GraphHasWindow(lpgw))
			return TRUE;
	}
	return FALSE;
}
//
// returns true if there are any graph windows open (wxt/caca/win terminals) 
// Note: This routine is used to handle "persist". Do not test for qt windows here since they run in a separate process 
//
bool WinAnyWindowOpen()
{
	bool window_opened = WinWindowOpened();
#ifdef WXWIDGETS
	window_opened |= wxt_window_opened();
#endif
#ifdef HAVE_LIBCACA
	window_opened |= CACA_window_opened();
#endif
	return window_opened;
}

#ifndef WGP_CONSOLE
	void WinPersistTextClose()
	{
		if(!WinAnyWindowOpen() && _WinM.TxtWin.hWndParent && !IsWindowVisible(_WinM.TxtWin.hWndParent))
			PostMessage(_WinM.TxtWin.hWndParent, WM_CLOSE, 0, 0);
	}
#endif

void WinMessageLoop()
{
	MSG msg;
	while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
		/* HBB 19990505: Petzold says we should check this: */
		if(msg.message == WM_QUIT)
			return;
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}

#ifndef WGP_CONSOLE
	void WinOpenConsole()
	{
		/* Try to attach to an existing console window. */
		if(AttachConsole(ATTACH_PARENT_PROCESS) == 0) {
			if(GetLastError() != ERROR_ACCESS_DENIED) {
				/* Open new console if we are are not attached to one already.
				   Note that closing this console window will end wgnuplot, too. */
				AllocConsole();
			}
		}
		SetConsoleCtrlHandler(ConsoleHandler, TRUE);
	}
#endif

void WinRaiseConsole()
{
#ifndef WGP_CONSOLE
	HWND console = _WinM.TxtWin.hWndParent;
	if(_WinM.pausewin.bPause && IsWindow(_WinM.pausewin.hWndPause))
		console = _WinM.pausewin.hWndPause;
#else
	HWND console = GetConsoleWindow();
#endif
	if(console) {
		if(IsIconic(console))
			ShowWindow(console, SW_SHOWNORMAL);
		BringWindowToTop(console);
	}
}
//
// WinGetCodepage:
// Map gnuplot's internal character encoding to Windows codepage codes.
//
UINT WinGetCodepage(enum set_encoding_id encoding)
{
	UINT codepage;
	/* For a list of code page identifiers see
	    http://msdn.microsoft.com/en-us/library/dd317756%28v=vs.85%29.aspx
	 */
	switch(encoding) {
		case S_ENC_DEFAULT:    codepage = CP_ACP; break;
		case S_ENC_ISO8859_1:  codepage = 28591; break;
		case S_ENC_ISO8859_2:  codepage = 28592; break;
		case S_ENC_ISO8859_9:  codepage = 28599; break;
		case S_ENC_ISO8859_15: codepage = 28605; break;
		case S_ENC_CP437:      codepage =   437; break;
		case S_ENC_CP850:      codepage =   850; break;
		case S_ENC_CP852:      codepage =   852; break;
		case S_ENC_CP950:      codepage =   950; break;
		case S_ENC_CP1250:     codepage =  1250; break;
		case S_ENC_CP1251:     codepage =  1251; break;
		case S_ENC_CP1252:     codepage =  1252; break;
		case S_ENC_CP1254:     codepage =  1254; break;
		case S_ENC_KOI8_R:     codepage = 20866; break;
		case S_ENC_KOI8_U:     codepage = 21866; break;
		case S_ENC_SJIS:       codepage =   932; break;
		case S_ENC_UTF8:       codepage = CP_UTF8; break;
		default: {
		    /* unknown encoding, fall back to default "ANSI" codepage */
		    codepage = CP_ACP;
		    FPRINTF((stderr, "unknown encoding: %i\n", encoding));
	    }
	}
	return codepage;
}

LPWSTR UnicodeText(LPCSTR str, enum set_encoding_id encoding)
{
	LPWSTR strw = NULL;
	UINT codepage = WinGetCodepage(encoding);
	int length;
	// sanity check 
	if(str) {
		// get length of converted string 
		length = MultiByteToWideChar(codepage, 0, str, -1, NULL, 0);
		strw = (LPWSTR)SAlloc::M(sizeof(WCHAR) * length);
		// convert string to UTF-16 
		length = MultiByteToWideChar(codepage, 0, str, -1, strw, length);
	}
	return strw;
}

LPSTR AnsiText(LPCWSTR strw,  enum set_encoding_id encoding)
{
	UINT codepage = WinGetCodepage(encoding);
	// get length of converted string 
	int length = WideCharToMultiByte(codepage, 0, strw, -1, NULL, 0, NULL, 0);
	LPSTR str = (LPSTR)SAlloc::M(sizeof(char) * length);
	// convert string to "Ansi" 
	length = WideCharToMultiByte(codepage, 0, strw, -1, str, length, NULL, 0);
	return str;
}

FILE * win_fopen(const char * filename, const char * mode)
{
	LPWSTR wfilename = UnicodeText(filename, GPT._Encoding);
	LPWSTR wmode = UnicodeText(mode, GPT._Encoding);
	FILE * file = _wfopen(wfilename, wmode);
	if(file == NULL) {
		// "encoding" didn't work, try UTF-8 instead 
		FREEANDASSIGN(wfilename, UnicodeText(filename, S_ENC_UTF8));
		file = _wfopen(wfilename, wmode);
	}
	SAlloc::F(wfilename);
	SAlloc::F(wmode);
	return file;
}

#ifndef USE_FAKEPIPES
	FILE * win_popen(const char * filename, const char * mode)
	{
		//LPWSTR wfilename = UnicodeText(filename, encoding);
		//LPWSTR wmode = UnicodeText(mode, encoding);
		FILE * file = _wpopen(SUcSwitch(filename), SUcSwitch(mode));
		//SAlloc::F(wfilename);
		//SAlloc::F(wmode);
		return file;
	}
#endif

UINT GetDPI()
{
	HDC hdc_screen = GetDC(NULL);
	if(hdc_screen) {
		UINT dpi = GetDeviceCaps(hdc_screen, LOGPIXELSX);
		ReleaseDC(NULL, hdc_screen);
		return dpi;
	}
	else
		return 96;
}
