// NPP-ETC.CPP
//
#include <npp-internal.h>
#pragma hdrstop
//
//
//
void DPIManager::init() 
{
	HDC hdc = GetDC(NULL);
	if(hdc) {
		// Initialize the DPIManager member variable
		// This will correspond to the DPI setting
		// With all Windows OS's to date the X and Y DPI will be identical
		_dpiX = GetDeviceCaps(hdc, LOGPIXELSX);
		_dpiY = GetDeviceCaps(hdc, LOGPIXELSY);
		ReleaseDC(NULL, hdc);
	}
}

int FASTCALL DPIManager::scaledSystemMetricX(int nIndex) const { return MulDiv(GetSystemMetrics(nIndex), 96, _dpiX); }
int FASTCALL DPIManager::scaledSystemMetricY(int nIndex) const { return MulDiv(GetSystemMetrics(nIndex), 96, _dpiY); }
int FASTCALL DPIManager::scaleX(int x) const { return MulDiv(x, _dpiX, 96); }
int FASTCALL DPIManager::scaleY(int y) const { return MulDiv(y, _dpiY, 96); }
int FASTCALL DPIManager::unscaleX(int x) const { return MulDiv(x, 96, _dpiX); }
int FASTCALL DPIManager::unscaleY(int y) const { return MulDiv(y, 96, _dpiY); }
int DPIManager::scaledScreenWidth() const { return scaledSystemMetricX(SM_CXSCREEN); }
int DPIManager::scaledScreenHeight() const { return scaledSystemMetricY(SM_CYSCREEN); }
int FASTCALL DPIManager::pointsToPixels(int pt) const { return MulDiv(pt, _dpiY, 72); };

void FASTCALL DPIManager::scaleRect(__inout RECT * pRect) const
{
	pRect->left = scaleX(pRect->left);
	pRect->right = scaleX(pRect->right);
	pRect->top = scaleY(pRect->top);
	pRect->bottom = scaleY(pRect->bottom);
}

void FASTCALL DPIManager::scalePoint(__inout POINT * pPoint) const
{
	pPoint->x = scaleX(pPoint->x);
	pPoint->y = scaleY(pPoint->y);
}

void FASTCALL DPIManager::scaleSize(__inout SIZE * pSize) const
{
	pSize->cx = scaleX(pSize->cx);
	pSize->cy = scaleY(pSize->cy);
}
//
//
//
CReadFileChanges::CReadFileChanges()
{
	_szFile = NULL;
	_dwNotifyFilter = 0;
}

CReadFileChanges::~CReadFileChanges()
{
}

BOOL CReadFileChanges::DetectChanges() 
{
	WIN32_FILE_ATTRIBUTE_DATA fInfo;
	BOOL rValue = FALSE;
	::GetFileAttributesEx(_szFile, GetFileExInfoStandard, &fInfo);
	if((_dwNotifyFilter & FILE_NOTIFY_CHANGE_SIZE) &&
	    (fInfo.nFileSizeHigh != _lastFileInfo.nFileSizeHigh || fInfo.nFileSizeLow != _lastFileInfo.nFileSizeLow)) {
		rValue = TRUE;
	}

	if((_dwNotifyFilter & FILE_NOTIFY_CHANGE_LAST_WRITE) &&
	    (fInfo.ftLastWriteTime.dwHighDateTime != _lastFileInfo.ftLastWriteTime.dwHighDateTime ||
	    fInfo.ftLastWriteTime.dwLowDateTime != _lastFileInfo.ftLastWriteTime.dwLowDateTime)) {
		rValue = TRUE;
	}

	_lastFileInfo = fInfo;
	return rValue;
}

void CReadFileChanges::AddFile(LPCTSTR szFile, DWORD dwNotifyFilter)
{
	_szFile = szFile;
	_dwNotifyFilter = dwNotifyFilter;
	::GetFileAttributesEx(szFile, GetFileExInfoStandard, &_lastFileInfo);
}

void CReadFileChanges::Terminate()
{
	_szFile = NULL;
	_dwNotifyFilter = 0;
}
//
// The namespace is a convenience to emphasize that these are internals
// interfaces.  The namespace can be safely removed if you need to.
//
namespace ReadDirectoryChangesPrivate
{
	//
	// CReadChangesRequest
	//
	CReadChangesRequest::CReadChangesRequest(CReadChangesServer* pServer, LPCTSTR sz, BOOL b, DWORD dw, DWORD size)
	{
		m_pServer       = pServer;
		m_dwFilterFlags = dw;
		m_bIncludeChildren      = b;
		m_wstrDirectory = sz;
		m_hDirectory    = 0;
		memzero(&m_Overlapped, sizeof(OVERLAPPED));
		// The hEvent member is not used when there is a completion
		// function, so it's ok to use it to point to the object.
		m_Overlapped.hEvent = this;
		m_Buffer.resize(size);
		m_BackupBuffer.resize(size);
	}
	CReadChangesRequest::~CReadChangesRequest()
	{
		// RequestTermination() must have been called successfully.
		assert(m_hDirectory == NULL);
	}
	bool CReadChangesRequest::OpenDirectory()
	{
		// Allow this routine to be called redundantly.
		if(m_hDirectory)
			return true;
		m_hDirectory = ::CreateFileW( m_wstrDirectory.c_str()/*pointer to the file name*/,
			FILE_LIST_DIRECTORY,                // access (read/write) mode
			FILE_SHARE_READ                                         // share mode
			| FILE_SHARE_WRITE
			| FILE_SHARE_DELETE,
			NULL,                               // security descriptor
			OPEN_EXISTING,                      // how to create
			FILE_FLAG_BACKUP_SEMANTICS                      // file attributes
			| FILE_FLAG_OVERLAPPED,
			NULL);                              // file with attributes to copy
		if(m_hDirectory == INVALID_HANDLE_VALUE) {
			return false;
		}
		return true;
	}
	void CReadChangesRequest::BeginRead()
	{
		DWORD dwBytes = 0;
		// This call needs to be reissued after every APC.
		::ReadDirectoryChangesW(m_hDirectory/*handle to directory*/, &m_Buffer[0]/*read results buffer*/,
			static_cast<DWORD>(m_Buffer.size())/*length of buffer*/, m_bIncludeChildren/*monitoring option*/,
			m_dwFilterFlags/*filter conditions*/, &dwBytes/*bytes returned*/, &m_Overlapped/*overlapped buffer*/,
			&NotificationCompletion/*completion routine*/);
	}
	//static
	VOID CALLBACK CReadChangesRequest::NotificationCompletion(DWORD dwErrorCode, // completion code
		DWORD dwNumberOfBytesTransfered, // number of bytes transferred
		LPOVERLAPPED lpOverlapped) // I/O information buffer
	{
		CReadChangesRequest* pBlock = static_cast<CReadChangesRequest*>(lpOverlapped->hEvent);
		if(dwErrorCode == ERROR_OPERATION_ABORTED) {
			::InterlockedDecrement(&pBlock->m_pServer->m_nOutstandingRequests);
			delete pBlock;
			return;
		}
		// Can't use sizeof(FILE_NOTIFY_INFORMATION) because
		// the structure is padded to 16 bytes.
		assert((dwNumberOfBytesTransfered == 0) || (dwNumberOfBytesTransfered >= offsetof(FILE_NOTIFY_INFORMATION, FileName) + sizeof(WCHAR)));
		pBlock->BackupBuffer(dwNumberOfBytesTransfered);
		// Get the new read issued as fast as possible. The documentation
		// says that the original OVERLAPPED structure will not be used
		// again once the completion routine is called.
		pBlock->BeginRead();
		pBlock->ProcessNotification();
	}

	void CReadChangesRequest::ProcessNotification()
	{
		BYTE* pBase = m_BackupBuffer.data();
		for(;;) {
			FILE_NOTIFY_INFORMATION& fni = (FILE_NOTIFY_INFORMATION&)*pBase;
			std::wstring wstrFilename(fni.FileName, fni.FileNameLength/sizeof(wchar_t));
			// Handle a trailing backslash, such as for a root directory.
			if(!wstrFilename.empty() && wstrFilename.back() != L'\\')
				wstrFilename = m_wstrDirectory + L"\\" + wstrFilename;
			else
				wstrFilename = m_wstrDirectory + wstrFilename;
			// If it could be a short filename, expand it.
			LPCWSTR wszFilename = ::PathFindFileNameW(wstrFilename.c_str());
			int len = sstrleni(wszFilename);
			// The maximum length of an 8.3 filename is twelve, including the dot.
			if(len <= 12 && wcschr(wszFilename, L'~')) {
				// Convert to the long filename form. Unfortunately, this
				// does not work for deletions, so it's an imperfect fix.
				wchar_t wbuf[MAX_PATH];
				if(::GetLongPathNameW(wstrFilename.c_str(), wbuf, _countof(wbuf)) > 0)
					wstrFilename = wbuf;
			}
			m_pServer->m_pBase->Push(fni.Action, wstrFilename);
			if(!fni.NextEntryOffset)
				break;
			pBase += fni.NextEntryOffset;
		}
	}
}
//
//
//
using namespace ReadDirectoryChangesPrivate;
//
// CReadDirectoryChanges
//
CReadDirectoryChanges::CReadDirectoryChanges() : m_Notifications()
{
	m_hThread       = NULL;
	m_dwThreadId = 0;
	m_pServer       = new CReadChangesServer(this);
}

CReadDirectoryChanges::~CReadDirectoryChanges()
{
	Terminate();
	delete m_pServer;
}

void CReadDirectoryChanges::Init()
{
	//
	// Kick off the worker thread, which will be
	// managed by CReadChangesServer.
	//
	m_hThread = (HANDLE)_beginthreadex(NULL, 0, CReadChangesServer::ThreadStartProc, m_pServer, 0, &m_dwThreadId);
}

void CReadDirectoryChanges::Terminate()
{
	if(m_hThread) {
		::QueueUserAPC(CReadChangesServer::TerminateProc, m_hThread, (ULONG_PTR)m_pServer);
		::WaitForSingleObjectEx(m_hThread, 10000, true);
		::CloseHandle(m_hThread);
		m_hThread = NULL;
		m_dwThreadId = 0;
	}
}

void CReadDirectoryChanges::AddDirectory(LPCTSTR szDirectory, BOOL bWatchSubtree, DWORD dwNotifyFilter, DWORD dwBufferSize)
{
	if(!m_hThread)
		Init();
	CReadChangesRequest* pRequest = new CReadChangesRequest(m_pServer, szDirectory, bWatchSubtree, dwNotifyFilter, dwBufferSize);
	QueueUserAPC(CReadChangesServer::AddDirectoryProc, m_hThread, (ULONG_PTR)pRequest);
}

void CReadDirectoryChanges::Push(DWORD dwAction, std::wstring& wstrFilename)
{
	TDirectoryChangeNotification dirChangeNotif = TDirectoryChangeNotification(dwAction, wstrFilename);
	m_Notifications.push(dirChangeNotif);
}

bool CReadDirectoryChanges::Pop(DWORD& dwAction, std::wstring& wstrFilename)
{
	TDirectoryChangeNotification pair;
	if(!m_Notifications.pop(pair))
		return false;
	dwAction = pair.first;
	wstrFilename = pair.second;
	return true;
}
//
//
//
/*virtual*/void Window::init(HINSTANCE hInst, HWND parent)
{
	_hInst = hInst;
	_hParent = parent;
}

/*virtual*/void Window::display(bool toShow) const
{
	::ShowWindow(_hSelf, toShow ? SW_SHOW : SW_HIDE);
}
	
/*virtual*/void Window::reSizeTo(RECT & rc) // should NEVER be const !!!
{
	::MoveWindow(_hSelf, rc.left, rc.top, rc.right, rc.bottom, TRUE);
	redraw();
}
	
/*virtual*/void Window::reSizeToWH(RECT & rc) // should NEVER be const !!!
{
	::MoveWindow(_hSelf, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, TRUE);
	redraw();
}
	
/*virtual*/void Window::redraw(bool forceUpdate) const
{
	::InvalidateRect(_hSelf, nullptr, TRUE);
	if(forceUpdate)
		::UpdateWindow(_hSelf);
}
	
/*virtual*/void Window::getClientRect(RECT & rc) const
{
	::GetClientRect(_hSelf, &rc);
}

/*virtual*/void Window::getWindowRect(RECT & rc) const
{
	::GetWindowRect(_hSelf, &rc);
}

/*virtual*/int Window::getWidth() const
{
	RECT rc;
	::GetClientRect(_hSelf, &rc);
	return (rc.right - rc.left);
}

/*virtual*/int Window::getHeight() const
{
	RECT rc;
	::GetClientRect(_hSelf, &rc);
	return ::IsWindowVisible(_hSelf) ? (rc.bottom - rc.top) : 0;
}

/*virtual*/bool Window::isVisible() const { return (::IsWindowVisible(_hSelf) ? true : false); }
//
//
//
static const TCHAR * babyGridClassName = TEXT("BABYGRID");
bool BabyGridWrapper::_isRegistered = false;

void BabyGridWrapper::init(HINSTANCE hInst, HWND parent, int16_t id)
{
	Window::init(hInst, parent);
	if(!_isRegistered)
		RegisterGridClass(_hInst);
	_hSelf = ::CreateWindowEx(WS_EX_CLIENTEDGE, babyGridClassName, TEXT(""), WS_CHILD | WS_VISIBLE | WS_TABSTOP, 
		CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, _hParent, reinterpret_cast<HMENU>(id), _hInst, NULL);
}
//
// Construct from args
//
WINRECT::WINRECT(WORD f, int id, LONG p)
{
	THISZERO();
	flags = f;
	nID = (UINT)id;
	param = p;
}
//
// Get the parent of a given WINRECT. To find the parent, chase the prev
// pointer to the start of the list, then take the item before that in memory.
//
WINRECT* WINRECT::Parent()
{
	WINRECT* pEntry = NULL;
	for(pEntry = this; pEntry->Prev(); pEntry = pEntry->Prev()) {
		// go backwards to the end
	}
	// the entry before the first child is the group
	WINRECT * parent = pEntry-1;
	assert(parent->IsGroup());
	return parent;
}
//
// Get group margins
//
BOOL WINRECT::GetMargins(int& w, int& h)
{
	if(IsGroup()) {
		w = (short)LOWORD(param);
		h = (short)HIWORD(param);
		return TRUE;
	}
	w = h = 0;
	return FALSE;
}
//
// Initialize map: set up all the next/prev pointers. This converts the
// linear array to a more convenient linked list. Called from END_WINDOW_MAP.
//
WINRECT* WINRECT::InitMap(WINRECT* pWinMap, WINRECT* parent)
{
	assert(pWinMap);
	WINRECT* pwrc = pWinMap;  // current table entry
	WINRECT* prev = NULL;     // previous entry starts out none
	while(!pwrc->IsEndGroup()) {
		pwrc->prev = prev;
		pwrc->next = NULL;
		if(prev)
			prev->next = pwrc;
		prev = pwrc;
		if(pwrc->IsGroup()) {
			pwrc = InitMap(pwrc+1, pwrc); // recurse! Returns end-of-grp
			assert(pwrc->IsEndGroup());
		}
		++pwrc;
	}
	// safety checks
	assert(pwrc->IsEndGroup());
	assert(prev);
	assert(prev->next==NULL);
	return parent ? pwrc : NULL;
}
//
//
//
SizeableDlg::SizeableDlg(WINRECT * pWinMap) : MyBaseClass(), _winMgr(pWinMap)
{
}

BOOL SizeableDlg::onInitDialog()
{
	_winMgr.InitToFitSizeFromCurrent(_hSelf);
	_winMgr.CalcLayout(_hSelf);
	_winMgr.SetWindowPositions(_hSelf);
	//getClientRect(_rc);
	return TRUE;
}

void SizeableDlg::onSize(UINT, int cx, int cy)
{
	_winMgr.CalcLayout(cx, cy, _hSelf);
	_winMgr.SetWindowPositions(_hSelf);
}

void SizeableDlg::onGetMinMaxInfo(MINMAXINFO* lpMMI)
{
	_winMgr.GetMinMaxInfo(_hSelf, lpMMI);
}

LRESULT SizeableDlg::onWinMgr(WPARAM, LPARAM)
{
	return 0;
}

INT_PTR CALLBACK SizeableDlg::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message) {
		case WM_INITDIALOG:
		    return onInitDialog();
		case WM_GETMINMAXINFO:
		    onGetMinMaxInfo((MINMAXINFO*)lParam);
		    return TRUE;
		case WM_SIZE:
		    onSize(static_cast<UINT>(wParam), LOWORD(lParam), HIWORD(lParam));
		    return TRUE;
		default:
		    if(message == WM_WINMGR)
			    return (BOOL)onWinMgr(wParam, lParam);
		    break;
	}
	return FALSE;
}
//
//
//
Process::Process(const TCHAR * cmd, const TCHAR * args, const TCHAR * cDir) : _command(cmd), _args(args), _curDir(cDir)
{
}

void Process::run(bool isElevationRequired) const
{
	const TCHAR * opVerb = isElevationRequired ? TEXT("runas") : TEXT("open");
	::ShellExecute(NULL, opVerb, _command.c_str(), _args.c_str(), _curDir.c_str(), SW_SHOWNORMAL);
}

ulong Process::runSync(bool isElevationRequired) const
{
	SHELLEXECUTEINFO ShExecInfo;
	INITWINAPISTRUCT(ShExecInfo);
	ShExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
	ShExecInfo.hwnd = NULL;
	ShExecInfo.lpVerb = isElevationRequired ? TEXT("runas") : TEXT("open");
	ShExecInfo.lpFile = _command.c_str();
	ShExecInfo.lpParameters = _args.c_str();
	ShExecInfo.lpDirectory = _curDir.c_str();
	ShExecInfo.nShow = SW_SHOWNORMAL;
	ShExecInfo.hInstApp = NULL;
	ShellExecuteEx(&ShExecInfo);
	if(!ShExecInfo.hProcess) {
		// throw exception
		throw GetLastErrorAsString(GetLastError());
	}
	WaitForSingleObject(ShExecInfo.hProcess, INFINITE);
	ulong exitCode;
	if(::GetExitCodeProcess(ShExecInfo.hProcess, &exitCode) == FALSE) {
		// throw exception
		throw GetLastErrorAsString(GetLastError());
	}
	return exitCode;
}
//
//
//
HWND ScintillaCtrls::createSintilla(HWND hParent)
{
	_hParent = hParent;
	ScintillaEditView * scint = new ScintillaEditView;
	scint->init(_hInst, _hParent);
	_scintVector.push_back(scint);
	return scint->getHSelf();
}

int ScintillaCtrls::getIndexFrom(HWND handle2Find)
{
	for(size_t i = 0, len = _scintVector.size(); i < len; ++i) {
		if(_scintVector[i]->getHSelf() == handle2Find) {
			return static_cast<int32_t>(i);
		}
	}
	return -1;
}

ScintillaEditView * ScintillaCtrls::getScintillaEditViewFrom(HWND handle2Find)
{
	int i = getIndexFrom(handle2Find);
	return (i == -1 || size_t(i) >= _scintVector.size()) ? NULL : _scintVector[i];
}
/*
   bool ScintillaCtrls::destroyScintilla(HWND handle2Destroy)
   {
        int i = getIndexFrom(handle2Destroy);
        if(i == -1)
                return false;
        _scintVector[i]->destroy();
        delete _scintVector[i];
        std::vector<ScintillaEditView *>::iterator it2delete = _scintVector.begin()+ i;
        _scintVector.erase(it2delete);
        return true;
   }
 */
void ScintillaCtrls::destroy()
{
	for(size_t i = 0, len = _scintVector.size(); i < len; ++i) {
		_scintVector[i]->destroy();
		delete _scintVector[i];
	}
}
//
//
//
void ControlsTab::createTabs(WindowVector & winVector)
{
	_pWinVector = &winVector;
	for(size_t i = 0, len = winVector.size(); i < len; ++i)
		TabBar::insertAtEnd(winVector[i]._name.c_str());
	TabBar::activateAt(0);
	activateWindowAt(0);
}

void ControlsTab::activateWindowAt(int index)
{
	if(index == _current) 
		return;
	(*_pWinVector)[_current]._dlg->display(false);
	(*_pWinVector)[index]._dlg->display(true);
	_current = index;
}

void ControlsTab::reSizeTo(RECT & rc)
{
	TabBar::reSizeTo(rc);
	rc.left += marge;
	rc.top += marge;
	//-- We do those dirty things
	//-- because it's a "vertical" tab control
	if(_isVertical) {
		rc.right -= 40;
		rc.bottom -= 20;
		if(getRowCount() == 2) {
			rc.right -= 20;
		}
	}
	//-- end of dirty things
	rc.bottom -= 55;
	rc.right -= 20;
	(*_pWinVector)[_current]._dlg->reSizeTo(rc);
	(*_pWinVector)[_current]._dlg->redraw();
}

bool ControlsTab::renameTab(const TCHAR * internalName, const TCHAR * newName)
{
	bool foundIt = false;
	size_t i = 0;
	for(size_t len = _pWinVector->size(); i < len; ++i) {
		if((*_pWinVector)[i]._internalName == internalName) {
			foundIt = true;
			break;
		}
	}
	if(!foundIt)
		return false;
	renameTab(i, newName);
	return true;
}

void ControlsTab::renameTab(size_t index, const TCHAR * newName)
{
	TCITEM tie;
	tie.mask = TCIF_TEXT;
	tie.pszText = (TCHAR*)newName;
	TabCtrl_SetItem(_hSelf, index, &tie);
}
//
//
//
Win32Exception::Win32Exception(EXCEPTION_POINTERS * info)
{
	_location = info->ExceptionRecord->ExceptionAddress;
	_code = info->ExceptionRecord->ExceptionCode;
	_info = info;
	switch(_code) {
		case EXCEPTION_ACCESS_VIOLATION: _event = "Access violation"; break;
		case EXCEPTION_FLT_DIVIDE_BY_ZERO:
		case EXCEPTION_INT_DIVIDE_BY_ZERO: _event = "Division by zero"; break;
		default: _event = "Unlisted exception";
	}
}

void Win32Exception::installHandler()
{
#ifndef __MINGW32__
	_set_se_translator(Win32Exception::translate);
#endif
}

void Win32Exception::removeHandler()
{
#ifndef __MINGW32__
	_set_se_translator(NULL);
#endif
}

void Win32Exception::translate(unsigned code, EXCEPTION_POINTERS * info)
{
	// Windows guarantees that *(info->ExceptionRecord) is valid
	switch(code) {
		case EXCEPTION_ACCESS_VIOLATION:
		    throw Win32AccessViolation(info);
		    break;
		default:
		    throw Win32Exception(info);
	}
}

Win32AccessViolation::Win32AccessViolation(EXCEPTION_POINTERS * info) : Win32Exception(info)
{
	_isWrite = info->ExceptionRecord->ExceptionInformation[0] == 1;
	_badAddress = reinterpret_cast<ExceptionAddress>(info->ExceptionRecord->ExceptionInformation[1]);
}
//
//
//
void ToolTip::destroy()
{
	::DestroyWindow(_hSelf);
	_hSelf = NULL;
}

void ToolTip::init(HINSTANCE hInst, HWND hParent)
{
	if(_hSelf == NULL) {
		Window::init(hInst, hParent);
		_hSelf = CreateWindowEx(0, TOOLTIPS_CLASS, NULL, WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, NULL, NULL);
		if(!_hSelf) {
			throw std::runtime_error("ToolTip::init : CreateWindowEx() function return null");
		}
		NppDarkMode::setDarkTooltips(_hSelf, NppDarkMode::ToolTipsType::tooltip);
		::SetWindowLongPtr(_hSelf, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
		_defaultProc = reinterpret_cast<WNDPROC>(::SetWindowLongPtr(_hSelf, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(staticWinProc)));
	}
}

void ToolTip::Show(RECT rectTitle, const TCHAR * pszTitle, int iXOff, int iWidthOff)
{
	if(isVisible())
		destroy();
	if(lstrlen(pszTitle)) {
		// INITIALIZE MEMBERS OF THE TOOLINFO STRUCTURE
		INITWINAPISTRUCT(_ti);
		_ti.uFlags = TTF_TRACK | TTF_ABSOLUTE;
		_ti.hwnd   = ::GetParent(_hParent);
		_ti.hinst  = _hInst;
		_ti.uId    = 0;
		_ti.rect.left   = rectTitle.left;
		_ti.rect.top    = rectTitle.top;
		_ti.rect.right  = rectTitle.right;
		_ti.rect.bottom = rectTitle.bottom;
		HFONT _hFont = (HFONT)::SendMessage(_hParent, WM_GETFONT, 0, 0);
		::SendMessage(_hSelf, WM_SETFONT, reinterpret_cast<WPARAM>(_hFont), TRUE);
		// Bleuargh...  const_cast.  Will have to do for now.
		_ti.lpszText  = const_cast<TCHAR *>(pszTitle);
		::SendMessage(_hSelf, TTM_ADDTOOL, 0, reinterpret_cast<LPARAM>(&_ti));
		::SendMessage(_hSelf, TTM_TRACKPOSITION, 0, MAKELONG(_ti.rect.left + iXOff, _ti.rect.top + iWidthOff));
		::SendMessage(_hSelf, TTM_TRACKACTIVATE, true, reinterpret_cast<LPARAM>(&_ti));
	}
}

/*static*/LRESULT CALLBACK ToolTip::staticWinProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam) 
{
	return (((ToolTip*)(::GetWindowLongPtr(hwnd, GWLP_USERDATA)))->runProc(Message, wParam, lParam));
}

LRESULT ToolTip::runProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	return ::CallWindowProc(_defaultProc, _hSelf, message, wParam, lParam);
}
//
//
//
/*virtual*/void DockingDlgInterface::init(HINSTANCE hInst, HWND parent) 
{
	StaticDialog::init(hInst, parent);
	TCHAR temp[MAX_PATH];
	::GetModuleFileName(reinterpret_cast<HMODULE>(hInst), temp, MAX_PATH);
	_moduleName = ::PathFindFileName(temp);
}

void DockingDlgInterface::create(tTbData * data, bool isRTL)
{
	assert(data != nullptr);
	StaticDialog::create(_dlgID, isRTL);
	TCHAR temp[MAX_PATH];
	::GetWindowText(_hSelf, temp, MAX_PATH);
	_pluginName = temp;
	// user information
	data->hClient = _hSelf;
	data->pszName = _pluginName.c_str();
	// supported features by plugin
	data->uMask = 0;
	// additional info
	data->pszAddInfo = NULL;
}

/*virtual*/INT_PTR CALLBACK DockingDlgInterface::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam) 
{
	switch(message) {
		case WM_ERASEBKGND:
			if(NppDarkMode::isEnabled()) {
				RECT rc = { 0 };
				getClientRect(rc);
				::FillRect(reinterpret_cast<HDC>(wParam), &rc, NppDarkMode::getDarkerBackgroundBrush());
				return TRUE;
			}
			break;
		case WM_NOTIFY:
			{
				LPNMHDR pnmh = reinterpret_cast<LPNMHDR>(lParam);
				if(pnmh->hwndFrom == _hParent) {
					switch(LOWORD(pnmh->code)) {
						case DMN_CLOSE:
							break;
						case DMN_FLOAT:
							_isFloating = true;
							break;
						case DMN_DOCK:
							_iDockedPos = HIWORD(pnmh->code);
							_isFloating = false;
							break;
						default:
							break;
					}
				}
			}
			break;
		default:
			break;
	}
	return FALSE;
}
