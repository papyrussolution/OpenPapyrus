// This file is part of Notepad++ project
// Copyright (C)2021 Don HO <don.h@free.fr>
// @licence GNU GPL
//
#include <npp-internal.h>
#pragma hdrstop

const TCHAR Notepad_plus_Window::_className[32] = TEXT("Notepad++");
HWND Notepad_plus_Window::gNppHWND = NULL;

//const TCHAR COMMAND_ARG_HELP[]
const TCHAR * Notepad_plus_Window::P_CommandArgHelp = TEXT(
"Usage :\r\
\r\
notepad++ [--help] [-multiInst] [-noPlugin] [-lLanguage] [-udl=\"My UDL Name\"] [-LlangCode] [-nLineNumber] [-cColumnNumber] [-pPosition] [-xLeftPos] [-yTopPos] [-nosession] [-notabbar] [-ro] [-systemtray] [-loadingTime] [-alwaysOnTop] [-openSession] [-r] [-qn=\"Easter egg name\" | -qt=\"a text to display.\" | -qf=\"D:\\my quote.txt\"] [-qSpeed1|2|3] [-quickPrint] [-settingsDir=\"d:\\your settings dir\\\"] [-openFoldersAsWorkspace]  [-titleAdd=\"additional title bar text\"][filePath]\r\
\r\
--help : This help message\r\
-multiInst : Launch another Notepad++ instance\r\
-noPlugin : Launch Notepad++ without loading any plugin\r\
-l : Open file or Ghost type with syntax highlighting of choice\r\
-udl=\"My UDL Name\": Open file by applying User Defined Language\r\
-L : Apply indicated localization, langCode is browser language code\r\
-n : Scroll to indicated line on filePath\r\
-c : Scroll to indicated column on filePath\r\
-p : Scroll to indicated position on filePath\r\
-x : Move Notepad++ to indicated left side position on the screen\r\
-y : Move Notepad++ to indicated top position on the screen\r\
-nosession : Launch Notepad++ without previous session\r\
-notabbar : Launch Notepad++ without tabbar\r\
-ro : Make the filePath read only\r\
-systemtray : Launch Notepad++ directly in system tray\r\
-loadingTime : Display Notepad++ loading time\r\
-alwaysOnTop : Make Notepad++ always on top\r\
-openSession : Open a session. filePath must be a session file\r\
-r : Open files recursively. This argument will be ignored\r\
     if filePath contain no wildcard character\r\
-qn=\"Easter egg name\": Ghost type easter egg via its name\r\
-qt=\"text to display.\": Ghost type the given text\r\
-qf=\"D:\\my quote.txt\": Ghost type a file content via the file path\r\
-qSpeed : Ghost typing speed. Value from 1 to 3 for slow, fast and fastest\r\
-quickPrint : Print the file given as argument then quit Notepad++\r\
-settingsDir=\"d:\\your settings dir\\\": Override the default settings dir\r\
-openFoldersAsWorkspace: open filePath of folder(s) as workspace\r\
-titleAdd=\"string\": add string to Notepad++ title bar\r\
filePath : file or folder name to open (absolute or relative path name)\r\
");

namespace // anonymous
{
struct PaintLocker final {
	explicit PaintLocker(HWND handle)
		: handle(handle)
	{
		// disallow drawing on the window
		LockWindowUpdate(handle);
	}

	~PaintLocker()
	{
		// re-allow drawing for the window
		LockWindowUpdate(NULL);

		// force re-draw
		InvalidateRect(handle, nullptr, TRUE);
		RedrawWindow(handle, nullptr, NULL, RDW_ERASE | RDW_ALLCHILDREN | RDW_FRAME | RDW_INVALIDATE);
	}

	HWND handle;
};
} // anonymous namespace

void Notepad_plus_Window::init(HINSTANCE hInst, HWND parent, const TCHAR * cmdLine, CmdLineParams * cmdLineParams)
{
	time_t timestampBegin = 0;
	if(cmdLineParams->_showLoadingTime)
		timestampBegin = time(NULL);

	Window::init(hInst, parent);
	WNDCLASS nppClass;

	nppClass.style = CS_BYTEALIGNWINDOW | CS_DBLCLKS;
	nppClass.lpfnWndProc = Notepad_plus_Proc;
	nppClass.cbClsExtra = 0;
	nppClass.cbWndExtra = 0;
	nppClass.hInstance = _hInst;
	nppClass.hIcon = ::LoadIcon(hInst, MAKEINTRESOURCE(IDI_M30ICON));
	nppClass.hCursor = ::LoadCursor(NULL, IDC_ARROW);
	nppClass.hbrBackground = ::CreateSolidBrush(::GetSysColor(COLOR_MENU));
	nppClass.lpszMenuName = MAKEINTRESOURCE(IDR_M30_MENU);
	nppClass.lpszClassName = _className;

	_isPrelaunch = cmdLineParams->_isPreLaunch;

	if(!::RegisterClass(&nppClass)) {
		throw std::runtime_error("Notepad_plus_Window::init : RegisterClass() function failed");
	}

	NppParameters& nppParams = NppParameters::getInstance();
	NppGUI & nppGUI = nppParams.getNppGUI();

	if(cmdLineParams->_isNoPlugin)
		_notepad_plus_plus_core._pluginsManager.disable();

	nppGUI._isCmdlineNosessionActivated = cmdLineParams->_isNoSession;

	_hIconAbsent = ::LoadIcon(hInst, MAKEINTRESOURCE(IDI_ICONABSENT));

	_hSelf = ::CreateWindowEx(
		WS_EX_ACCEPTFILES | (_notepad_plus_plus_core._nativeLangSpeaker.isRTL() ? WS_EX_LAYOUTRTL : 0),
		_className,
		TEXT("Notepad++"),
		(WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN),
		// CreateWindowEx bug : set all 0 to walk around the pb
		0, 0, 0, 0,
		_hParent, nullptr, _hInst,
		(LPVOID)this);  // pass the ptr of this instantiated object
	// for retrieve it in Notepad_plus_Proc from
	// the CREATESTRUCT.lpCreateParams afterward.

	if(NULL == _hSelf)
		throw std::runtime_error("Notepad_plus_Window::init : CreateWindowEx() function return null");

	PaintLocker paintLocker{_hSelf};

	_notepad_plus_plus_core.staticCheckMenuAndTB();

	gNppHWND = _hSelf;

	if(cmdLineParams->isPointValid()) {
		::MoveWindow(_hSelf, cmdLineParams->_point.x, cmdLineParams->_point.y, nppGUI._appPos.right, nppGUI._appPos.bottom, TRUE);
	}
	else {
		WINDOWPLACEMENT posInfo;
		posInfo.length = sizeof(WINDOWPLACEMENT);
		posInfo.flags = 0;
		if(_isPrelaunch)
			posInfo.showCmd = SW_HIDE;
		else
			posInfo.showCmd = nppGUI._isMaximized ? SW_SHOWMAXIMIZED : SW_SHOWNORMAL;

		posInfo.ptMinPosition.x = (LONG)-1;
		posInfo.ptMinPosition.y = (LONG)-1;
		posInfo.ptMaxPosition.x = (LONG)-1;
		posInfo.ptMaxPosition.y = (LONG)-1;
		posInfo.rcNormalPosition.left   = nppGUI._appPos.left;
		posInfo.rcNormalPosition.top    = nppGUI._appPos.top;
		posInfo.rcNormalPosition.bottom = nppGUI._appPos.top + nppGUI._appPos.bottom;
		posInfo.rcNormalPosition.right  = nppGUI._appPos.left + nppGUI._appPos.right;

		//SetWindowPlacement will take care of situations, where saved position was in no longer available
		// monitor
		::SetWindowPlacement(_hSelf, &posInfo);
	}

	if((nppGUI._tabStatus & TAB_MULTILINE) != 0)
		::SendMessage(_hSelf, WM_COMMAND, IDM_VIEW_DRAWTABBAR_MULTILINE, 0);

	if(!nppGUI._menuBarShow)
		::SetMenu(_hSelf, NULL);

	if(cmdLineParams->_isNoTab || (nppGUI._tabStatus & TAB_HIDE)) {
		const int tabStatusOld = nppGUI._tabStatus;
		::SendMessage(_hSelf, NPPM_HIDETABBAR, 0, TRUE);
		if(cmdLineParams->_isNoTab) {
			// Restore old settings when tab bar has been hidden from tab bar.
			nppGUI._tabStatus = tabStatusOld;
		}
	}

	if(cmdLineParams->_alwaysOnTop)
		::SendMessage(_hSelf, WM_COMMAND, IDM_VIEW_ALWAYSONTOP, 0);

	if(nppGUI._rememberLastSession && !nppGUI._isCmdlineNosessionActivated)
		_notepad_plus_plus_core.loadLastSession();

	if(nppParams.doFunctionListExport() || nppParams.doPrintAndExit()) {
		::ShowWindow(_hSelf, SW_HIDE);
	}
	else if(!cmdLineParams->_isPreLaunch) {
		if(cmdLineParams->isPointValid())
			::ShowWindow(_hSelf, SW_SHOW);
		else
			::ShowWindow(_hSelf, nppGUI._isMaximized ? SW_MAXIMIZE : SW_SHOW);
	}
	else {
		_notepad_plus_plus_core._pTrayIco =
		    new trayIconControler(_hSelf,
			IDI_M30ICON,
			NPPM_INTERNAL_MINIMIZED_TRAY,
			::LoadIcon(_hInst, MAKEINTRESOURCE(IDI_M30ICON)),
			TEXT(""));
		_notepad_plus_plus_core._pTrayIco->doTrayIcon(ADD);
	}
	std::vector<generic_string> fileNames;
	std::vector<generic_string> patterns;
	patterns.push_back(TEXT("*.xml"));

	generic_string nppDir = nppParams.getNppPath();

	LocalizationSwitcher & localizationSwitcher = nppParams.getLocalizationSwitcher();
	std::wstring localizationDir = nppDir;
	PathAppend(localizationDir, TEXT("localization\\"));

	_notepad_plus_plus_core.getMatchedFileNames(localizationDir.c_str(), patterns, fileNames, false, false);
	for(size_t i = 0, len = fileNames.size(); i < len; ++i)
		localizationSwitcher.addLanguageFromXml(fileNames[i]);

	fileNames.clear();
	ThemeSwitcher & themeSwitcher = nppParams.getThemeSwitcher();

	//  Get themes from both npp install themes dir and app data themes dir with the per user
	//  overriding default themes of the same name.

	generic_string appDataThemeDir;
	if(nppParams.getAppDataNppDir() && nppParams.getAppDataNppDir()[0]) {
		appDataThemeDir = nppParams.getAppDataNppDir();
		PathAppend(appDataThemeDir, TEXT("themes\\"));
		_notepad_plus_plus_core.getMatchedFileNames(appDataThemeDir.c_str(), patterns, fileNames, false, false);
		for(size_t i = 0, len = fileNames.size(); i < len; ++i) {
			themeSwitcher.addThemeFromXml(fileNames[i]);
		}
	}

	fileNames.clear();

	generic_string nppThemeDir;
	nppThemeDir = nppDir.c_str(); // <- should use the pointer to avoid the constructor of copy
	PathAppend(nppThemeDir, TEXT("themes\\"));

	// Set theme directory to their installation directory
	themeSwitcher.setThemeDirPath(nppThemeDir);

	_notepad_plus_plus_core.getMatchedFileNames(nppThemeDir.c_str(), patterns, fileNames, false, false);
	for(size_t i = 0, len = fileNames.size(); i < len; ++i) {
		generic_string themeName(themeSwitcher.getThemeFromXmlFileName(fileNames[i].c_str()));
		if(!themeSwitcher.themeNameExists(themeName.c_str())) {
			themeSwitcher.addThemeFromXml(fileNames[i]);

			if(!appDataThemeDir.empty()) {
				generic_string appDataThemePath = appDataThemeDir;

				if(!::PathFileExists(appDataThemePath.c_str())) {
					::CreateDirectory(appDataThemePath.c_str(), NULL);
				}

				TCHAR* fn = PathFindFileName(fileNames[i].c_str());
				PathAppend(appDataThemePath, fn);
				themeSwitcher.addThemeStylerSavePath(fileNames[i], appDataThemePath);
			}
		}
	}

	// Restore all dockable panels from the last session
	for(size_t i = 0, len = _notepad_plus_plus_core._internalFuncIDs.size(); i < len; ++i)
		::SendMessage(_hSelf, WM_COMMAND, _notepad_plus_plus_core._internalFuncIDs[i], 0);

	std::vector<generic_string> fns;
	if(cmdLine)
		fns = _notepad_plus_plus_core.loadCommandlineParams(cmdLine, cmdLineParams);

	// Launch folder as workspace after all this dockable panel being restored from the last session
	// To avoid dockable panel toggle problem.
	if(cmdLineParams->_openFoldersAsWorkspace) {
		generic_string emptyStr;
		_notepad_plus_plus_core.launchFileBrowser(fns, emptyStr, true);
	}
	::SendMessage(_hSelf, WM_ACTIVATE, WA_ACTIVE, 0);

	// Notify plugins that Notepad++ is ready
	SCNotification scnN;
	scnN.nmhdr.code = NPPN_READY;
	scnN.nmhdr.hwndFrom = _hSelf;
	scnN.nmhdr.idFrom = 0;
	_notepad_plus_plus_core._pluginsManager.notify(&scnN);

	if(!cmdLineParams->_easterEggName.empty()) {
		if(cmdLineParams->_quoteType == 0) { // Easter Egg Name
			int iQuote = _notepad_plus_plus_core.getQuoteIndexFrom(cmdLineParams->_easterEggName.c_str());
			if(iQuote != -1) {
				_notepad_plus_plus_core.showQuoteFromIndex(iQuote);
			}
		}
		else if(cmdLineParams->_quoteType == 1) { // command line quote
			_userQuote = cmdLineParams->_easterEggName;
			_quoteParams.reset();
			_quoteParams._quote = _userQuote.c_str();
			_quoteParams._quoter = TEXT("Anonymous #999");
			_quoteParams._shouldBeTrolling = false;
			_quoteParams._lang = cmdLineParams->_langType;
			if(cmdLineParams->_ghostTypingSpeed == 1)
				_quoteParams._speed = QuoteParams::slow;
			else if(cmdLineParams->_ghostTypingSpeed == 2)
				_quoteParams._speed = QuoteParams::rapid;
			else if(cmdLineParams->_ghostTypingSpeed == 3)
				_quoteParams._speed = QuoteParams::speedOfLight;

			_notepad_plus_plus_core.showQuote(&_quoteParams);
		}
		else if(cmdLineParams->_quoteType == 2) { // content drom file
			if(::PathFileExists(cmdLineParams->_easterEggName.c_str())) {
				std::string content = getFileContent(cmdLineParams->_easterEggName.c_str());
				WcharMbcsConvertor& wmc = WcharMbcsConvertor::getInstance();
				_userQuote = wmc.char2wchar(content.c_str(), SC_CP_UTF8);
				if(!_userQuote.empty()) {
					_quoteParams.reset();
					_quoteParams._quote = _userQuote.c_str();
					_quoteParams._quoter = TEXT("Anonymous #999");
					_quoteParams._shouldBeTrolling = false;
					_quoteParams._lang = cmdLineParams->_langType;
					if(cmdLineParams->_ghostTypingSpeed == 1)
						_quoteParams._speed = QuoteParams::slow;
					else if(cmdLineParams->_ghostTypingSpeed == 2)
						_quoteParams._speed = QuoteParams::rapid;
					else if(cmdLineParams->_ghostTypingSpeed == 3)
						_quoteParams._speed = QuoteParams::speedOfLight;

					_notepad_plus_plus_core.showQuote(&_quoteParams);
				}
			}
		}
	}

	if(cmdLineParams->_showLoadingTime) {
		time_t timestampEnd = time(NULL);
		double loadTime = difftime(timestampEnd, timestampBegin);

		char dest[256];
		sprintf(dest, "Loading time : %.0lf seconds", loadTime);
		::MessageBoxA(NULL, dest, "", MB_OK);
	}

	bool isSnapshotMode = nppGUI.isSnapshotMode();
	if(isSnapshotMode) {
		_notepad_plus_plus_core.checkModifiedDocument(false);
		// Lauch backup task
		_notepad_plus_plus_core.launchDocumentBackupTask();
	}

	// Make this call later to take effect
	::SendMessage(_hSelf, NPPM_INTERNAL_SETWORDCHARS, 0, 0);

	if(nppParams.doFunctionListExport())
		::SendMessage(_hSelf, NPPM_INTERNAL_EXPORTFUNCLISTANDQUIT, 0, 0);

	if(nppParams.doPrintAndExit())
		::SendMessage(_hSelf, NPPM_INTERNAL_PRNTANDQUIT, 0, 0);
}

bool Notepad_plus_Window::isDlgsMsg(MSG * msg) const
{
	for(size_t i = 0, len = _notepad_plus_plus_core._hModelessDlgs.size(); i < len; ++i) {
		if(_notepad_plus_plus_core.processIncrFindAccel(msg))
			return true;

		if(_notepad_plus_plus_core.processFindAccel(msg))
			return true;

		if(::IsDialogMessageW(_notepad_plus_plus_core._hModelessDlgs[i], msg))
			return true;
	}
	return false;
}
