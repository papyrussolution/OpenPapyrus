// NPP-INTERNAL.H
//
#ifndef __NPP_INTERNAL_H // {
#define __NPP_INTERNAL_H

#define _CRT_SECURE_NO_WARNINGS
#define __STDC_LIMIT_MACROS
#define _WIN32_WINNT 0x0600
#define _USE_64BIT_TIME_T
#define TIXML_USE_STL
#define TIXMLA_USE_STL
#define _CRT_NONSTDC_NO_DEPRECATE
#define _CRT_NON_CONFORMING_SWPRINTFS 1
#define _CRT_NON_CONFORMING_WCSTOK
#define __STDC_FORMAT_MACROS

//..\OSF\npp-gh20210917\scintilla\include
/*
SCE_USER_KWLIST_TOTAL
SCE_USER_TOTAL_KEYWORD_GROUPS
SCE_UNIVERSAL_FOUND_STYLE_SMART
SCE_UNIVERSAL_FOUND_STYLE
SCE_UNIVERSAL_FOUND_STYLE_INC
SCE_UNIVERSAL_TAGMATCH
SCE_UNIVERSAL_TAGATTR
SCE_UNIVERSAL_FOUND_STYLE_EXT1
SCE_UNIVERSAL_FOUND_STYLE_EXT2
SCE_UNIVERSAL_FOUND_STYLE_EXT3
SCE_UNIVERSAL_FOUND_STYLE_EXT4
SCE_UNIVERSAL_FOUND_STYLE_EXT5
SCE_SEARCHRESULT_FILE_HEADER
SCE_SEARCHRESULT_SEARCH_HEADER
*/

#define SLIB_DONT_UNDEF_MINMAX
#include <slib.h>
#include <iso646.h>

#include <list>
#include <deque>
#include <vector>
#include <map>
#include <unordered_map>
#include <algorithm>
#include <locale>
#include <iostream>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <stdexcept>
#include <cassert>
#include <codecvt>
#include <memory>
#include <regex>
#include <unordered_set>
#include <array>
#include <stack>
#include <string>
#include <cctype>
#include <cinttypes>
#include <functional>
#include <mutex>
#include <cstdint>
#include <exception>
#include <utility>
#include <random>
#include <cwctype>

#include <windowsx.h>
#include <shobjidl.h>
#include <shlwapi.h>
#include <shlobj.h>
#include <commctrl.h>
#include <wininet.h>
#include <dbghelp.h>
#include <oleacc.h>
#include <uxtheme.h>
#include <wintrust.h>
#include <softpub.h>
#include <wincrypt.h>
#include <sensapi.h>
#include <Vssym32.h>
#include <comdef.h>
#include <comip.h>

class ScintillaEditView;
class TiXmlDocument;
class TiXmlNode;
class DockingCont;
class DockingManager;
class CustomFileDialog;
class Buffer;
class CReadDirectoryChanges;
struct MapPosition;
class FileBrowser;
class FolderInfo;
class UserLangContainer;
struct NppStyle;
struct TaskListInfo;
class Notepad_plus;
class Notepad_plus_Window;
class AnsiCharPanel;
class ClipboardHistoryPanel;
class VerticalFileSwitcher;
class ProjectPanel;
class DocumentMap;
class FunctionListPanel;
struct QuoteParams;
class PluginsManager;
class FindReplaceDlg;
class DockingSplitter;
class DocTabView;
class ReBar;
struct StatusBarSubclassInfo;
class NativeLangSpeaker;
class NppParameters;
class ColourPopup;
class Window;
class ISorter;

#define PM_PROJECTPANELTITLE     TEXT("Project Panel")
#define PM_WORKSPACEROOTNAME     TEXT("Workspace")
#define PM_NEWFOLDERNAME         TEXT("Folder Name")
#define PM_NEWPROJECTNAME        TEXT("Project Name")
#define PM_NEWWORKSPACE            TEXT("New Workspace")
#define PM_OPENWORKSPACE           TEXT("Open Workspace")
#define PM_RELOADWORKSPACE         TEXT("Reload Workspace")
#define PM_SAVEWORKSPACE           TEXT("Save")
#define PM_SAVEASWORKSPACE         TEXT("Save As...")
#define PM_SAVEACOPYASWORKSPACE    TEXT("Save a Copy As...")
#define PM_NEWPROJECTWORKSPACE     TEXT("Add New Project")
#define PM_FINDINFILESWORKSPACE    TEXT("Find in Projects...")
#define PM_EDITRENAME              TEXT("Rename")
#define PM_EDITNEWFOLDER           TEXT("Add Folder")
#define PM_EDITADDFILES            TEXT("Add Files...")
#define PM_EDITADDFILESRECUSIVELY  TEXT("Add Files from Directory...")
#define PM_EDITREMOVE              TEXT("Remove\tDEL")
#define PM_EDITMODIFYFILE          TEXT("Modify File Path")
#define PM_WORKSPACEMENUENTRY      TEXT("Workspace")
#define PM_EDITMENUENTRY           TEXT("Edit")
#define PM_MOVEUPENTRY             TEXT("Move Up\tCtrl+Up")
#define PM_MOVEDOWNENTRY           TEXT("Move Down\tCtrl+Down")
#define DM_PANELTITLE              TEXT("Document Map")
#define MDLG_CLASS_NAME TEXT("moveDlg")
#define AI_PROJECTPANELTITLE            TEXT("ASCII Codes Insertion Panel")
#define CH_PROJECTPANELTITLE            TEXT("Clipboard History")
#define FS_PROJECTPANELTITLE            TEXT("Document List")
#define FL_PANELTITLE                   TEXT("Function List")
#define FL_FUCTIONLISTROOTNODE          "FunctionList"
//#include "clipboardFormats.h"
#define CF_HTML       TEXT("HTML Format")
#define CF_RTF        TEXT("Rich Text Format")
#define CF_NPPTEXTLEN TEXT("Notepad++ Binary Text Length")
//
#include "npp-resource.h"
#include <nlohmann/json.hpp>
//#include "Common.h"
const bool dirUp = true;
const bool dirDown = false;

#define NPP_CP_WIN_1252           1252
#define NPP_CP_DOS_437            437
#define NPP_CP_BIG5               950

#define LINKTRIGGERED WM_USER+555

#define BCKGRD_COLOR (RGB(255, 102, 102))
#define TXT_COLOR    (RGB(255, 255, 255))

#define generic_strtol wcstol
#define generic_strncpy wcsncpy
#define generic_stricmp wcsicmp
#define generic_strncmp wcsncmp
#define generic_strnicmp wcsnicmp
#define generic_strncat wcsncat
#define generic_strchr wcschr
#define generic_atoi _wtoi
#define generic_itoa _itow
#define generic_atof _wtof
#define generic_strtok wcstok
#define generic_strftime wcsftime
#define generic_fprintf fwprintf
#define generic_sprintf swprintf
#define generic_sscanf swscanf
#define generic_fopen _wfopen
#define generic_fgets fgetws
#define COPYDATA_FILENAMES COPYDATA_FILENAMESW
#define NPP_INTERNAL_FUCTION_STR TEXT("Notepad++::InternalFunction")

typedef std::basic_string<TCHAR> generic_string;
typedef std::basic_stringstream<TCHAR> generic_stringstream;

generic_string folderBrowser(HWND parent, const generic_string & title = TEXT(""), int outputCtrlID = 0, const TCHAR * defaultStr = NULL);
generic_string getFolderName(HWND parent, const TCHAR * defaultDir = NULL);
void printInt(int int2print);
void printStr(const TCHAR * str2print);
generic_string commafyInt(size_t n);
void writeLog(const TCHAR * logFileName, const char * log2write);
int filter(uint code, struct _EXCEPTION_POINTERS * ep);
generic_string purgeMenuItemString(const TCHAR * menuItemStr, bool keepAmpersand = false);
std::vector<generic_string> tokenizeString(const generic_string & tokenString, const char delim);
void ClientRectToScreenRect(HWND hWnd, RECT* rect);
void ScreenRectToClientRect(HWND hWnd, RECT* rect);
std::wstring string2wstring(const std::string & rString, UINT codepage);
std::string wstring2string(const std::wstring & rwString, UINT codepage);
bool isInList(const TCHAR * token, const TCHAR * list);
generic_string BuildMenuFileName(int filenameLen, uint pos, const generic_string &filename);
std::string getFileContent(const TCHAR * file2read);
generic_string relativeFilePathToFullFilePath(const TCHAR * relativeFilePath);
void writeFileContent(const TCHAR * file2write, const char * content2write);
bool matchInList(const TCHAR * fileName, const std::vector<generic_string> & patterns);
bool allPatternsAreExclusion(const std::vector<generic_string> patterns);

class WcharMbcsConvertor final {
public:
	static WcharMbcsConvertor & getInstance() 
	{
		static WcharMbcsConvertor instance;
		return instance;
	}
	const wchar_t * char2wchar(const char * mbStr, UINT codepage, int lenIn = -1, int * pLenOut = NULL, int * pBytesNotProcessed = NULL);
	const wchar_t * char2wchar(const char * mbcs2Convert, UINT codepage, int * mstart, int * mend);
	const char * wchar2char(const wchar_t * wcStr, UINT codepage, int lenIn = -1, int * pLenOut = NULL);
	const char * wchar2char(const wchar_t * wcStr, UINT codepage, long * mstart, long * mend);
	const char * encode(UINT fromCodepage, UINT toCodepage, const char * txt2Encode, int lenIn = -1, int * pLenOut = NULL, int * pBytesNotProcessed = NULL)
	{
		int lenWc = 0;
		const wchar_t * strW = char2wchar(txt2Encode, fromCodepage, lenIn, &lenWc, pBytesNotProcessed);
		return wchar2char(strW, toCodepage, lenWc, pLenOut);
	}
private:
	WcharMbcsConvertor() = default;
	~WcharMbcsConvertor() = default;
	// Since there's no public ctor, we need to void the default assignment operator and copy ctor.
	// Since these are marked as deleted does not matter under which access specifier are kept
	WcharMbcsConvertor(const WcharMbcsConvertor&) = delete;
	WcharMbcsConvertor& operator = (const WcharMbcsConvertor&) = delete;
	// No move ctor and assignment
	WcharMbcsConvertor(WcharMbcsConvertor&&) = delete;
	WcharMbcsConvertor& operator = (WcharMbcsConvertor&&) = delete;

	template <class T> class StringBuffer final {
	public:
		~StringBuffer() 
		{
			if(_allocLen) 
				delete[] _str;
		}
		void sizeTo(size_t size)
		{
			if(_allocLen < size) {
				if(_allocLen)
					delete[] _str;
				_allocLen = MAX(size, initSize);
				_str = new T[_allocLen];
			}
		}
		void empty()
		{
			static T nullStr = 0; // routines may return an empty string, with null terminator, without
				// allocating memory; a pointer to this null character will be returned in that case
			if(_allocLen == 0)
				_str = &nullStr;
			else
				_str[0] = 0;
		}
		operator T*() { return _str; }
		operator const T*() const { return _str; }
protected:
		static const int initSize = 1024;
		size_t _allocLen = 0;
		T* _str = nullptr;
	};
	StringBuffer<char> _multiByteStr;
	StringBuffer<wchar_t> _wideCharStr;
};

#define MACRO_RECORDING_IN_PROGRESS 1
#define MACRO_RECORDING_HAS_STOPPED 2
#define REBARBAND_SIZE sizeof(REBARBANDINFO)

generic_string PathRemoveFileSpec(generic_string & path);
generic_string PathAppend(generic_string &strDest, const generic_string & str2append);
COLORREF getCtrlBgColor(HWND hWnd);
generic_string stringToUpper(generic_string strToConvert);
generic_string stringToLower(generic_string strToConvert);
generic_string stringReplace(generic_string subject, const generic_string& search, const generic_string& replace);
std::vector<generic_string> stringSplit(const generic_string& input, const generic_string& delimiter);
bool str2numberVector(generic_string str2convert, std::vector<size_t>& numVect);
generic_string stringJoin(const std::vector<generic_string>& strings, const generic_string& separator);
generic_string stringTakeWhileAdmissable(const generic_string& input, const generic_string& admissable);
double stodLocale(const generic_string& str, _locale_t loc, size_t* idx = NULL);
int OrdinalIgnoreCaseCompareStrings(LPCTSTR sz1, LPCTSTR sz2);
bool str2Clipboard(const generic_string &str2cpy, HWND hwnd);
generic_string GetLastErrorAsString(DWORD errorCode = 0);
generic_string intToString(int val);
generic_string uintToString(uint val);
HWND CreateToolTip(int toolID, HWND hDlg, HINSTANCE hInst, const PTSTR pszText);
HWND CreateToolTipRect(int toolID, HWND hWnd, HINSTANCE hInst, const PTSTR pszText, const RECT rc);
bool isCertificateValidated(const generic_string & fullFilePath, const generic_string & subjectName2check);
bool isAssoCommandExisting(LPCTSTR FullPathName);
std::wstring s2ws(const std::string & str);
std::string ws2s(const std::wstring& wstr);
bool deleteFileOrFolder(const generic_string& f2delete);
void getFilesInFolder(std::vector<generic_string>& files, const generic_string& extTypeFilter, const generic_string& inFolder);

template <typename T> size_t vecRemoveDuplicates(std::vector<T>& vec, bool isSorted = false, bool canSort = false)
{
	if(!isSorted && canSort) {
		std::sort(vec.begin(), vec.end());
		isSorted = true;
	}
	if(isSorted) {
		typename std::vector<T>::iterator it;
		it = std::unique(vec.begin(), vec.end());
		vec.resize(distance(vec.begin(), it));  // unique() does not shrink the vector
	}
	else {
		std::unordered_set<T> seen;
		auto newEnd = std::remove_if(vec.begin(), vec.end(), [&seen](const T& value) { return !seen.insert(value).second; });
		vec.erase(newEnd, vec.end());
	}
	return vec.size();
}

void trim(generic_string& str);
bool endsWith(const generic_string& s, const generic_string& suffix);
int nbDigitsFromNbLines(size_t nbLines);
generic_string getDateTimeStrFrom(const generic_string& dateTimeFormat, const SYSTEMTIME& st);
//
#include "tinyxmlA.h"
#include "tinyxml.h"
#include "Scintilla.h"
//#include "ScintillaRef.h"
enum folderStyle {
	FOLDER_TYPE, 
	FOLDER_STYLE_SIMPLE, 
	FOLDER_STYLE_ARROW, 
	FOLDER_STYLE_CIRCLE, 
	FOLDER_STYLE_BOX, 
	FOLDER_STYLE_NONE
};

enum lineWrapMethod {
	LINEWRAP_DEFAULT, 
	LINEWRAP_ALIGNED, 
	LINEWRAP_INDENT
};
//
#include "SciLexer.h"
#include "BoostRegexSearch.h"
#include "Notepad_plus_msgs.h"
//#include "Window.h"
class Window {
public:
	//! \name Constructors & Destructor
	//@{
	Window() = default;
	Window(const Window &) = delete;
	virtual ~Window() = default;
	//@}
	virtual void init(HINSTANCE hInst, HWND parent);
	virtual void destroy() = 0;
	virtual void display(bool toShow = true) const;
	virtual void reSizeTo(RECT & rc); // should NEVER be const !!!
	virtual void reSizeToWH(RECT& rc); // should NEVER be const !!!
	virtual void redraw(bool forceUpdate = false) const;
	virtual void getClientRect(RECT & rc) const;
	virtual void getWindowRect(RECT & rc) const;
	virtual int getWidth() const;
	virtual int getHeight() const;
	virtual bool isVisible() const;
	HWND getHSelf() const { return _hSelf; }
	HWND getHParent() const { return _hParent; }
	void getFocus() const { ::SetFocus(_hSelf); }
	HINSTANCE getHinst() const
	{
		//assert(_hInst != 0);
		return _hInst;
	}
	Window & operator = (const Window&) = delete;
protected:
	HINSTANCE _hInst = NULL;
	HWND _hParent = NULL;
	HWND _hSelf = NULL;
};
//
//#include "ImageListSet.h"
#define IDI_SEPARATOR_ICON -1

class IconList {
public:
	IconList() = default;
	void init(HINSTANCE hInst, int iconSize);
	void create(int iconSize, HINSTANCE hInst, int * iconIDArray, int iconIDArraySize);
	void destroy() { ImageList_Destroy(_hImglst); };
	HIMAGELIST getHandle() const { return _hImglst; };
	void addIcon(int iconID) const;
	void addIcon(HICON hIcon) const;
	bool changeIcon(int index, const TCHAR * iconLocation) const;
	void addIcons(int size) const;
private:
	HIMAGELIST _hImglst = nullptr;
	HINSTANCE _hInst = nullptr;
	int * _pIconIDArray = nullptr;
	int _iconIDArraySize = 0;
	int _iconSize = 0;
};

typedef struct {
	int _cmdID;
	int _defaultIcon;
	int _grayIcon;
	int _defaultIcon2;
	int _grayIcon2;
	int _defaultDarkModeIcon;
	int _grayDarkModeIcon;
	int _defaultDarkModeIcon2;
	int _grayDarkModeIcon2;
	int _stdIcon;
} ToolBarButtonUnit;

struct DynamicCmdIcoBmp {
	UINT _message = 0;         // identification of icon in tool bar (menu ID)
	HBITMAP _hBmp = nullptr;   // bitmap for toolbar
	HICON _hIcon = nullptr;    // icon for toolbar
	HICON _hIcon_DM = nullptr; // dark mode icon for toolbar
};

typedef std::vector<ToolBarButtonUnit> ToolBarIconIDs;

// Light Mode list
const int HLIST_DEFAULT = 0;
const int HLIST_DISABLE = 1;
const int HLIST_DEFAULT2 = 2;
const int HLIST_DISABLE2 = 3;
// Dark Mode list
const int HLIST_DEFAULT_DM = 4;
const int HLIST_DISABLE_DM = 5;
const int HLIST_DEFAULT_DM2 = 6;
const int HLIST_DISABLE_DM2 = 7;

class ToolBarIcons {
public:
	ToolBarIcons() = default;
	void init(ToolBarButtonUnit * buttonUnitArray, int arraySize, std::vector<DynamicCmdIcoBmp> cmds2add);
	void create(HINSTANCE hInst, int iconSize);
	void destroy();
	HIMAGELIST getDefaultLst() const { return _iconListVector[HLIST_DEFAULT].getHandle(); };
	HIMAGELIST getDisableLst() const { return _iconListVector[HLIST_DISABLE].getHandle(); };
	HIMAGELIST getDefaultLstSet2() const { return _iconListVector[HLIST_DEFAULT2].getHandle(); };
	HIMAGELIST getDisableLstSet2() const { return _iconListVector[HLIST_DISABLE2].getHandle(); };
	HIMAGELIST getDefaultLstDM() const { return _iconListVector[HLIST_DEFAULT_DM].getHandle(); };
	HIMAGELIST getDisableLstDM() const { return _iconListVector[HLIST_DISABLE_DM].getHandle(); };
	HIMAGELIST getDefaultLstSetDM2() const { return _iconListVector[HLIST_DEFAULT_DM2].getHandle(); };
	HIMAGELIST getDisableLstSetDM2() const { return _iconListVector[HLIST_DISABLE_DM2].getHandle(); };
	void resizeIcon(int size) { reInit(size); }
	void reInit(int size);
	int getStdIconAt(int i) const { return _tbiis[i]._stdIcon; }
	bool replaceIcon(int witchList, int iconIndex, const TCHAR * iconLocation) const 
	{
		if(!oneof4(witchList, HLIST_DEFAULT, HLIST_DISABLE, HLIST_DEFAULT2, HLIST_DISABLE2))
			return false;
		return _iconListVector[witchList].changeIcon(iconIndex, iconLocation);
	}
private:
	ToolBarIconIDs _tbiis;
	std::vector<DynamicCmdIcoBmp> _moreCmds;
	std::vector<IconList> _iconListVector;
};
//
//#include "ToolBar.h"
#define REBAR_BAR_TOOLBAR               0
#define REBAR_BAR_SEARCH                1
#define REBAR_BAR_EXTERNAL              10
#ifndef _WIN32_IE
	#define _WIN32_IE       0x0600
#endif

enum toolBarStatusType {
	TB_SMALL, 
	TB_LARGE, 
	TB_SMALL2, 
	TB_LARGE2, 
	TB_STANDARD
};

struct iconLocator {
	int listIndex = 0;
	int iconIndex = 0;
	generic_string iconLocation;
	iconLocator(int iList, int iIcon, const generic_string& iconLoc) : listIndex(iList), iconIndex(iIcon), iconLocation(iconLoc)
	{
	}
};

class ToolBar : public Window {
public:
	ToolBar() = default;
	~ToolBar() = default;
	void initTheme(TiXmlDocument * toolIconsDocRoot);
	virtual bool init(HINSTANCE hInst, HWND hPere, toolBarStatusType type, ToolBarButtonUnit * buttonUnitArray, int arraySize);
	virtual void destroy();
	void enable(int cmdID, bool doEnable) const;
	int getWidth() const;
	int getHeight() const;
	void reduce();
	void enlarge();
	void reduceToSet2();
	void enlargeToSet2();
	void setToBmpIcons();
	bool getCheckState(int ID2Check) const;
	void setCheck(int ID2Check, bool willBeChecked) const;
	toolBarStatusType getState() const { return _state; }
	bool changeIcons();
	bool changeIcons(int whichLst, int iconIndex, const TCHAR * iconLocation);
	void registerDynBtn(UINT message, toolbarIcons* iconHandles, HICON absentIco);
	void registerDynBtnDM(UINT message, toolbarIconsWithDarkMode* iconHandles);
	void doPopop(POINT chevPoint);  //show the popup if buttons are hidden
	void addToRebar(ReBar * rebar);
private:
	TBBUTTON * _pTBB = nullptr;
	ToolBarIcons _toolBarIcons;
	toolBarStatusType _state = TB_SMALL;
	std::vector<DynamicCmdIcoBmp> _vDynBtnReg;
	size_t _nbButtons = 0;
	size_t _nbDynButtons = 0;
	size_t _nbTotalButtons = 0;
	size_t _nbCurrentButtons = 0;
	ReBar * _pRebar = nullptr;
	REBARBANDINFO _rbBand;
	std::vector<iconLocator> _customIconVect;
	TiXmlNode * _toolIcons = nullptr;
	void setDefaultImageList();
	void setDisableImageList();
	void setDefaultImageList2();
	void setDisableImageList2();
	void setDefaultImageListDM();
	void setDisableImageListDM();
	void setDefaultImageListDM2();
	void setDisableImageListDM2();
	void reset(bool create = false);
	void setState(toolBarStatusType state) { _state = state; }
};

class ReBar : public Window {
public:
	ReBar();
	virtual void destroy();
	void init(HINSTANCE hInst, HWND hPere);
	bool addBand(REBARBANDINFO * rBand, bool useID); // useID true if ID from info should be used (false for plugins). wID in bandinfo will be set to used ID
	void reNew(int id, REBARBANDINFO * rBand); // wID from bandinfo is used for update
	void removeBand(int id);
	void setIDVisible(int id, bool show);
	bool getIDVisible(int id);
	void setGrayBackground(int id);
private:
	std::vector<int> usedIDs;
	int  getNewID();
	void releaseID(int id);
	bool isIDTaken(int id);
};
//
//#include "UserDefineLangReference.h"
const int langNameLenMax = 64;
const int extsLenMax = 256;
const int max_char = 1024*30;
//
//#include "colors.h"
//const COLORREF red___               = RGB(0xFF,    0,    0);
//const COLORREF darkRed           = RGB(0x80,    0,    0);
//const COLORREF offWhite          = RGB(0xFF, 0xFB, 0xF0);
//const COLORREF darkGreen         = RGB(0,    0x80,    0); // SClrGreen
//const COLORREF liteGreen         = RGB(0,    0xFF,    0); // SClrLime
//const COLORREF blueGreen         = RGB(0,    0x80, 0x80);
//const COLORREF liteRed           = RGB(0xFF, 0xAA, 0xAA);
//const COLORREF liteBlueGreen     = RGB(0xAA, 0xFF, 0xC8);
//const COLORREF liteBlue          = RGB(0xA6, 0xCA, 0xF0);
//const COLORREF veryLiteBlue      = RGB(0xC4, 0xF9, 0xFD);
//const COLORREF extremeLiteBlue   = RGB(0xF2, 0xF4, 0xFF);
//const COLORREF darkBlue          = RGB(0,       0, 0x80);
//const COLORREF blue              = RGB(0,       0, 0xFF);
//const COLORREF black___             = RGB(0,       0,    0);
//const COLORREF white___             = RGB(0xFF, 0xFF, 0xFF);
//const COLORREF darkGrey          = RGB(64,     64,   64);
//const COLORREF grey___              = RGB(128,   128,  128);
//const COLORREF liteGrey          = RGB(192,   192,  192); // SClrSilver
//const COLORREF veryLiteGrey      = RGB(224,   224,  224);
//const COLORREF brown             = RGB(128,    64,    0);
//const COLORREF greenBlue       = RGB(192,   128,   64);
//const COLORREF darkYellow        = RGB(0xFF, 0xC0,    0);
//const COLORREF yellow            = RGB(0xFF, 0xFF,    0);
//const COLORREF lightYellow       = RGB(0xFF, 0xFF, 0xD5);
//const COLORREF cyan              = RGB(0,    0xFF, 0xFF);
//const COLORREF orange            = RGB(0xFF, 0x80, 0x00);
//const COLORREF purple            = RGB(0x80, 0x00, 0xFF);
//const COLORREF deepPurple        = RGB(0x87, 0x13, 0x97);
//const COLORREF extremeLitePurple = RGB(0xF8, 0xE8, 0xFF);
//const COLORREF veryLitePurple    = RGB(0xE7, 0xD8, 0xE9);
//const COLORREF liteBerge         = RGB(0xFE, 0xFC, 0xF5);
//const COLORREF berge             = RGB(0xFD, 0xF8, 0xE3);
//
//#include "StaticDialog.h"
typedef HRESULT (WINAPI * ETDTProc)(HWND, DWORD);

enum class PosAlign { 
	left, 
	right, 
	top, 
	bottom 
};

struct DLGTEMPLATEEX {
	WORD dlgVer = 0;
	WORD signature = 0;
	DWORD helpID = 0;
	DWORD exStyle = 0;
	DWORD style = 0;
	WORD cDlgItems = 0;
	short x = 0;
	short y = 0;
	short cx = 0;
	short cy = 0;
	// The structure has more fields but are variable length
};

class StaticDialog : public Window {
public:
	virtual ~StaticDialog();
	virtual void create(int dialogID, bool isRTL = false, bool msgDestParent = true);
	virtual bool isCreated() const { return (_hSelf != NULL); }
	void goToCenter();
	void display(bool toShow = true, bool enhancedPositioningCheckWhenShowing = false) const;
	RECT getViewablePositionRect(RECT testRc) const;
	POINT getTopPoint(HWND hwnd, bool isLeft = true) const;
	bool isCheckedOrNot(int checkControlID) const;
	void setChecked(int checkControlID, bool checkOrNot = true) const;
	virtual void destroy() override;
protected:
	RECT _rc = { 0 };
	static INT_PTR CALLBACK dlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
	virtual INT_PTR CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam) = 0;
	HGLOBAL makeRTLResource(int dialogID, DLGTEMPLATE ** ppMyDlgTemplate);
};
//
//#include "shortcut.h"
const size_t nameLenMax = 64;

void getKeyStrFromVal(UCHAR keyVal, generic_string & str);
void getNameStrFromCmd(DWORD cmd, generic_string & str);

struct KeyCombo {
	bool _isCtrl = false;
	bool _isAlt = false;
	bool _isShift = false;
	UCHAR _key = 0;
};

class Shortcut : public StaticDialog {
public:
	Shortcut();
	Shortcut(const TCHAR * name, bool isCtrl, bool isAlt, bool isShift, UCHAR key);
	Shortcut(const Shortcut & sc);
	BYTE getAcceleratorModifiers();
	Shortcut & operator = (const Shortcut & sc);
	friend inline const bool operator==(const Shortcut & a, const Shortcut & b) 
	{
		return (sstreq(a.getMenuName(), b.getMenuName()) && (a._keyCombo._isCtrl == b._keyCombo._isCtrl) &&
		       (a._keyCombo._isAlt == b._keyCombo._isAlt) && (a._keyCombo._isShift == b._keyCombo._isShift) && (a._keyCombo._key == b._keyCombo._key));
	}
	friend inline const bool operator!=(const Shortcut & a, const Shortcut & b) { return !(a == b); }
	virtual INT_PTR doDialog();
	// valid should only be used in cases where the shortcut isEnabled().
	virtual bool isValid() const;
	// true if _keyCombo != 0, false if _keyCombo == 0, in which case no accelerator should be made
	virtual bool isEnabled() const;
	generic_string toString() const;
	generic_string toMenuItemString() const; // generic_string suitable for menu
	const KeyCombo & getKeyCombo() const { return _keyCombo; }
	const TCHAR * getName() const { return _name; }
	const TCHAR * getMenuName() const { return _menuName; }
	void setName(const TCHAR * menuName, const TCHAR * shortcutName = NULL);
	void clear()
	{
		_keyCombo._isCtrl = false;
		_keyCombo._isAlt = false;
		_keyCombo._isShift = false;
		_keyCombo._key = 0;
	}
protected:
	KeyCombo _keyCombo;
	virtual INT_PTR CALLBACK run_dlgProc(UINT Message, WPARAM wParam, LPARAM lParam);
	bool _canModifyName = false;
	TCHAR _name[nameLenMax] = {'\0'};               //normal name is plain text (for display purposes)
	TCHAR _menuName[nameLenMax] = { '\0' }; //menu name has ampersands for quick keys
	void updateConflictState(const bool endSession = false) const;
};

class CommandShortcut : public Shortcut {
public:
	CommandShortcut(const Shortcut& sc, long id);
	ulong getID() const { return _id; }
	void setID(ulong id) { _id = id; }
	const TCHAR * getCategory() const { return _category.c_str(); }
	const TCHAR * getShortcutName() const { return _shortcutName.c_str(); }
private:
	ulong _id;
	generic_string _category;
	generic_string _shortcutName;
};

class ScintillaKeyMap : public Shortcut {
public:
	ScintillaKeyMap(const Shortcut& sc, ulong scintillaKeyID, ulong id) : Shortcut(sc), _menuCmdID(id), _scintillaKeyID(scintillaKeyID) 
	{
		_keyCombos.clear();
		_keyCombos.push_back(_keyCombo);
		_keyCombo._key = 0;
		_size = 1;
	}
	ulong getScintillaKeyID() const { return _scintillaKeyID; }
	int getMenuCmdID() const { return _menuCmdID; }
	size_t FASTCALL toKeyDef(size_t index) const;
	KeyCombo getKeyComboByIndex(size_t index) const;
	void setKeyComboByIndex(int index, KeyCombo combo);
	void removeKeyComboByIndex(size_t index);
	void clearDups() 
	{
		if(_size > 1)
			_keyCombos.erase(_keyCombos.begin()+1, _keyCombos.end());
		_size = 1;
	};
	int addKeyCombo(KeyCombo combo);
	bool isEnabled() const;
	size_t getSize() const;
	generic_string toString() const;
	generic_string toString(size_t index) const;
	INT_PTR doDialog();
	//
	// only compares the internal KeyCombos, nothing else
	//
	friend inline const bool operator == (const ScintillaKeyMap & a, const ScintillaKeyMap & b) 
	{
		bool equal = (a._size == b._size);
		if(equal) {
			size_t i = 0;
			while(equal && (i < a._size)) {
				equal = (a._keyCombos[i]._isCtrl == b._keyCombos[i]._isCtrl) &&
					(a._keyCombos[i]._isAlt == b._keyCombos[i]._isAlt) &&
					(a._keyCombos[i]._isShift == b._keyCombos[i]._isShift) &&
					(a._keyCombos[i]._key == b._keyCombos[i]._key);
				++i;
			}
		}
		return equal;
	}
	friend inline const bool operator!=(const ScintillaKeyMap & a, const ScintillaKeyMap & b) { return !(a == b); }
private:
	ulong _scintillaKeyID;
	int _menuCmdID;
	std::vector<KeyCombo> _keyCombos;
	size_t _size;
	void applyToCurrentIndex();
	void validateDialog();
	void showCurrentSettings();
	void updateListItem(int index);
protected:
	INT_PTR CALLBACK run_dlgProc(UINT Message, WPARAM wParam, LPARAM lParam);
};

struct recordedMacroStep {
	enum MacroTypeIndex {
		mtUseLParameter, 
		mtUseSParameter, 
		mtMenuCommand, 
		mtSavedSnR
	};
	int _message = 0;
	uptr_t _wParameter = 0;
	uptr_t _lParameter = 0;
	generic_string _sParameter;
	MacroTypeIndex _macroType = mtMenuCommand;

	recordedMacroStep(int iMessage, uptr_t wParam, uptr_t lParam, int codepage);
	explicit recordedMacroStep(int iCommandID) : _wParameter(iCommandID) 
	{
	}
	recordedMacroStep(int iMessage, uptr_t wParam, uptr_t lParam, const TCHAR * sParam, int type) : 
		_message(iMessage), _wParameter(wParam), _lParameter(lParam), _macroType(MacroTypeIndex(type))
	{
		_sParameter = (sParam) ? generic_string(sParam) : TEXT("");
	}
	bool isValid() const { return true; }
	bool isScintillaMacro() const { return _macroType <= mtMenuCommand; }
	bool isMacroable() const;

	void PlayBack(Window* pNotepad, ScintillaEditView * pEditView);
};

typedef std::vector<recordedMacroStep> Macro;

class MacroShortcut : public CommandShortcut {
	friend class NppParameters;
public:
	MacroShortcut(const Shortcut& sc, const Macro& macro, int id) : CommandShortcut(sc, id), _macro(macro) 
	{
		_canModifyName = true;
	}
	Macro & getMacro() { return _macro; }
private:
	Macro _macro;
};

class UserCommand : public CommandShortcut {
	friend class NppParameters;
public:
	UserCommand(const Shortcut& sc, const TCHAR * cmd, int id) : CommandShortcut(sc, id), _cmd(cmd) 
	{
		_canModifyName = true;
	}
	const TCHAR* getCmd() const { return _cmd.c_str(); }
private:
	generic_string _cmd;
};

class PluginCmdShortcut : public CommandShortcut {
//friend class NppParameters;
public:
	PluginCmdShortcut(const Shortcut& sc, int id, const TCHAR * moduleName, unsigned short internalID) :
		CommandShortcut(sc, id), _id(id), _moduleName(moduleName), _internalID(internalID) 
	{
	}
	bool isValid() const 
	{
		if(!Shortcut::isValid())
			return false;
		if((!_moduleName[0]) || (_internalID == -1))
			return false;
		return true;
	}
	const TCHAR * getModuleName() const { return _moduleName.c_str(); }
	int getInternalID() const { return _internalID; }
	ulong getID() const { return _id; }
private:
	ulong _id;
	generic_string _moduleName;
	int _internalID;
};

class Accelerator { //Handles accelerator keys for Notepad++ menu, including custom commands
	friend class ShortcutMapper;
public:
	Accelerator() = default;
	~Accelerator();
	void init(HMENU hMenu, HWND menuParent) 
	{
		_hAccelMenu = hMenu;
		_hMenuParent = menuParent;
		updateShortcuts();
	}
	HACCEL getAccTable() const { return _hAccTable; }
	HACCEL getIncrFindAccTable() const { return _hIncFindAccTab; }
	HACCEL getFindAccTable() const { return _hFindAccTab; }
	void updateShortcuts();
	void updateFullMenu();
private:
	HMENU _hAccelMenu = nullptr;
	HWND _hMenuParent = nullptr;
	HACCEL _hAccTable = nullptr;
	HACCEL _hIncFindAccTab = nullptr;
	HACCEL _hFindAccTab = nullptr;
	ACCEL * _pAccelArray = nullptr;
	int _nbAccelItems = 0;

	void updateMenuItemByCommand(const CommandShortcut& csc);
};

class ScintillaAccelerator {    //Handles accelerator keys for scintilla
public:
	ScintillaAccelerator() = default;
	void init(std::vector<HWND> * vScintillas, HMENU hMenu, HWND menuParent);
	void updateKeys();
	size_t nbScintillas() { return _vScintillas.size(); }
private:
	HMENU _hAccelMenu = nullptr;
	HWND _hMenuParent = nullptr;
	std::vector<HWND> _vScintillas;

	void updateMenuItemByID(const ScintillaKeyMap& skm, int id);
};
//
//#include "ContextMenu.h"
struct MenuItemUnit final {
	ulong _cmdID = 0;
	generic_string _itemName;
	generic_string _parentFolderName;
	MenuItemUnit() = default;
	MenuItemUnit(ulong cmdID, const generic_string& itemName, const generic_string& parentFolderName = generic_string()) : 
		_cmdID(cmdID), _itemName(itemName), _parentFolderName(parentFolderName)
	{
	}
	MenuItemUnit(ulong cmdID, const TCHAR * itemName, const TCHAR * parentFolderName = nullptr);
};

class ContextMenu final {
public:
	~ContextMenu();
	void create(HWND hParent, const std::vector<MenuItemUnit> & menuItemArray, const HMENU mainMenuHandle = NULL, bool copyLink = false);
	bool isCreated() const { return _hMenu != NULL; }
	void display(const POINT & p) const;
	void enableItem(int cmdID, bool doEnable) const;
	void checkItem(int cmdID, bool doCheck) const;
	HMENU getMenuHandle() const { return _hMenu; }
private:
	HWND _hParent = NULL;
	HMENU _hMenu = NULL;
	std::vector<HMENU> _subMenus;
};
//
//#include "dpiManager.h"
class DPIManager {
public:
	DPIManager() 
	{
		init();
	}
	// Get screen DPI.
	int getDPIX() const { return _dpiX; }
	int getDPIY() const { return _dpiY; }
	// Convert between raw pixels and relative pixels.
	int FASTCALL scaleX(int x) const;
	int FASTCALL scaleY(int y) const;
	int FASTCALL unscaleX(int x) const;
	int FASTCALL unscaleY(int y) const;
	// Determine the screen dimensions in relative pixels.
	int scaledScreenWidth() const;
	int scaledScreenHeight() const;
	// Scale rectangle from raw pixels to relative pixels.
	void FASTCALL scaleRect(__inout RECT * pRect) const;
	// Scale Point from raw pixels to relative pixels.
	void FASTCALL scalePoint(__inout POINT * pPoint) const;
	// Scale Size from raw pixels to relative pixels.
	void FASTCALL scaleSize(__inout SIZE * pSize) const;
	// Determine if screen resolution meets minimum requirements in relative pixels.
	bool isResolutionAtLeast(int cxMin, int cyMin) const { return (scaledScreenWidth() >= cxMin) && (scaledScreenHeight() >= cyMin); }
	// Convert a point size (1/72 of an inch) to raw pixels.
	int FASTCALL pointsToPixels(int pt) const;
	// Invalidate any cached metrics.
	void Invalidate() 
	{
		init();
	}
private:
	// X and Y DPI values are provided, though to date all
	// Windows OS releases have equal X and Y scale values
	int _dpiX = 0;
	int _dpiY = 0;
	void init();
	// This returns a 96-DPI scaled-down equivalent value for nIndex
	// For example, the value 120 at 120 DPI setting gets scaled down to 96
	// X and Y versions are provided, though to date all Windows OS releases
	// have equal X and Y scale values
	int FASTCALL scaledSystemMetricX(int nIndex) const;
	// This returns a 96-DPI scaled-down equivalent value for nIndex
	// For example, the value 120 at 120 DPI setting gets scaled down to 96
	// X and Y versions are provided, though to date all Windows OS releases
	// have equal X and Y scale values
	int FASTCALL scaledSystemMetricY(int nIndex) const;
};
//
//#include "NppDarkMode.h"
constexpr COLORREF HEXRGB(DWORD rrggbb) 
{
	// from 0xRRGGBB like natural #RRGGBB
	// to the little-endian 0xBBGGRR
	return ((rrggbb & 0xFF0000) >> 16) | ((rrggbb & 0x00FF00) ) | ((rrggbb & 0x0000FF) << 16);
}

namespace NppDarkMode { // Copyright (c) 2021 adzm / Adam D. Walling
	struct Colors {
		COLORREF background = 0;
		COLORREF softerBackground = 0;
		COLORREF hotBackground = 0;
		COLORREF pureBackground = 0;
		COLORREF errorBackground = 0;
		COLORREF text = 0;
		COLORREF darkerText = 0;
		COLORREF disabledText = 0;
		COLORREF linkText = 0;
		COLORREF edge = 0;
	};
	struct Options {
		bool enable = false;
		bool enableMenubar = false;
	};
	enum class ToolTipsType {
		tooltip,
		toolbar,
		listview,
		treeview,
		tabbar
	};

	enum ColorTone {
		blackTone  = 0,
		redTone    = 1,
		greenTone  = 2,
		blueTone   = 3,
		purpleTone = 4,
		cyanTone   = 5,
		oliveTone  = 6,
		customizedTone = 32
	};
	enum class TreeViewStyle {
		classic = 0,
		light = 1,
		dark = 2
	};

	void initDarkMode(); // pulls options from NppParameters
	void refreshDarkMode(HWND hwnd, bool forceRefresh = false);	// attempts to apply new options from NppParameters, sends NPPM_INTERNAL_REFRESHDARKMODE to hwnd's top level parent
	bool isEnabled();
	bool isDarkMenuEnabled();
	bool isExperimentalSupported();
	COLORREF invertLightness(COLORREF c);
	COLORREF invertLightnessSofter(COLORREF c);
	double calculatePerceivedLighness(COLORREF c);
	void setDarkTone(ColorTone colorToneChoice);
	COLORREF getBackgroundColor();
	COLORREF getSofterBackgroundColor();
	COLORREF getHotBackgroundColor();
	COLORREF getDarkerBackgroundColor();
	COLORREF getErrorBackgroundColor();
	COLORREF getTextColor();
	COLORREF getDarkerTextColor();
	COLORREF getDisabledTextColor();
	COLORREF getLinkTextColor();
	COLORREF getEdgeColor();
	HBRUSH getBackgroundBrush();
	HBRUSH getDarkerBackgroundBrush();
	HBRUSH getSofterBackgroundBrush();
	HBRUSH getHotBackgroundBrush();
	HBRUSH getErrorBackgroundBrush();
	HPEN getDarkerTextPen();
	HPEN getEdgePen();
	void setBackgroundColor(COLORREF c);
	void setSofterBackgroundColor(COLORREF c);
	void setHotBackgroundColor(COLORREF c);
	void setDarkerBackgroundColor(COLORREF c);
	void setErrorBackgroundColor(COLORREF c);
	void setTextColor(COLORREF c);
	void setDarkerTextColor(COLORREF c);
	void setDisabledTextColor(COLORREF c);
	void setLinkTextColor(COLORREF c);
	void setEdgeColor(COLORREF c);
	Colors getDarkModeDefaultColors();
	void changeCustomTheme(const Colors& colors);
	// handle events
	void handleSettingChange(HWND hwnd, LPARAM lParam);
	// processes messages related to UAH / custom menubar drawing.
	// return true if handled, false to continue with normal processing in your wndproc
	bool runUAHWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam, LRESULT* lr);
	void drawUAHMenuNCBottomLine(HWND hWnd);
	// from DarkMode.h
	void initExperimentalDarkMode();
	void setDarkMode(bool useDark, bool fixDarkScrollbar);
	void allowDarkModeForApp(bool allow);
	bool allowDarkModeForWindow(HWND hWnd, bool allow);
	void setTitleBarThemeColor(HWND hWnd);
	// enhancements to DarkMode.h
	void enableDarkScrollBarForWindowAndChildren(HWND hwnd);
	void subclassButtonControl(HWND hwnd);
	void subclassGroupboxControl(HWND hwnd);
	void subclassToolbarControl(HWND hwnd);
	void subclassTabControl(HWND hwnd);
	void subclassComboBoxControl(HWND hwnd);
	void autoSubclassAndThemeChildControls(HWND hwndParent, bool subclass = true, bool theme = true);
	void autoThemeChildControls(HWND hwndParent);
	void setDarkTitleBar(HWND hwnd);
	void setDarkExplorerTheme(HWND hwnd);
	void setDarkScrollBar(HWND hwnd);
	void setDarkTooltips(HWND hwnd, ToolTipsType type);
	void setDarkLineAbovePanelToolbar(HWND hwnd);
	void setDarkListView(HWND hwnd);
	void disableVisualStyle(HWND hwnd, bool doDisable);
	void calculateTreeViewStyle();
	void setTreeViewStyle(HWND hwnd);
	void setBorder(HWND hwnd, bool border = true);
	LRESULT onCtlColor(HDC hdc);
	LRESULT onCtlColorSofter(HDC hdc);
	LRESULT onCtlColorDarker(HDC hdc);
	LRESULT onCtlColorError(HDC hdc);
}
//
//#include "Parameters.h"
#ifdef _WIN64
	#ifdef _M_ARM64
		#define ARCH_TYPE IMAGE_FILE_MACHINE_ARM64
	#else
		#define ARCH_TYPE IMAGE_FILE_MACHINE_AMD64
	#endif
#else
	#define ARCH_TYPE IMAGE_FILE_MACHINE_I386
#endif

const bool POS_VERTICAL = true;
const bool POS_HORIZOTAL = false;
const int UDD_SHOW   = 1; // 0000 0001
const int UDD_DOCKED = 2; // 0000 0010

// 0 : 0000 0000 hide & undocked
// 1 : 0000 0001 show & undocked
// 2 : 0000 0010 hide & docked
// 3 : 0000 0011 show & docked

const int TAB_DRAWTOPBAR = 1;      //0000 0000 0001
const int TAB_DRAWINACTIVETAB = 2; //0000 0000 0010
const int TAB_DRAGNDROP = 4;       //0000 0000 0100
const int TAB_REDUCE = 8;          //0000 0000 1000
const int TAB_CLOSEBUTTON = 16;    //0000 0001 0000
const int TAB_DBCLK2CLOSE = 32;    //0000 0010 0000
const int TAB_VERTICAL = 64;       //0000 0100 0000
const int TAB_MULTILINE = 128;     //0000 1000 0000
const int TAB_HIDE = 256;          //0001 0000 0000
const int TAB_QUITONEMPTY = 512;   //0010 0000 0000
const int TAB_ALTICONS = 1024;     //0100 0000 0000

enum class EolType : std::uint8_t{
	windows,
	macos,
	unix,
	// special values
	unknown, // can not be the first value for legacy code
	osdefault = windows,
};

/*!
** \brief Convert an int into a FormatType
** \param value An arbitrary int
** \param defvalue The default value to use if an invalid value is provided
*/
EolType convertIntToFormatType(int value, EolType defvalue = EolType::osdefault);

enum UniMode {uni8Bit = 0, uniUTF8 = 1, uni16BE = 2, uni16LE = 3, uniCookie = 4, uni7Bit = 5, uni16BE_NoBOM = 6, uni16LE_NoBOM = 7, uniEnd};
enum ChangeDetect { cdDisabled = 0x0, cdEnabledOld = 0x01, cdEnabledNew = 0x02, cdAutoUpdate = 0x04, cdGo2end = 0x08 };
enum BackupFeature {bak_none = 0, bak_simple = 1, bak_verbose = 2};
enum OpenSaveDirSetting {dir_followCurrent = 0, dir_last = 1, dir_userDef = 2};
enum MultiInstSetting {monoInst = 0, multiInstOnSession = 1, multiInst = 2};
enum writeTechnologyEngine {defaultTechnology = 0, directWriteTechnology = 1};

enum urlMode {
	urlDisable = 0,
	urlNoUnderLineFg,
	urlUnderLineFg,
	urlNoUnderLineBg,
	urlUnderLineBg,
	urlMin = urlDisable,
	urlMax = urlUnderLineBg
};

const int LANG_INDEX_INSTR = 0;
const int LANG_INDEX_INSTR2 = 1;
const int LANG_INDEX_TYPE = 2;
const int LANG_INDEX_TYPE2 = 3;
const int LANG_INDEX_TYPE3 = 4;
const int LANG_INDEX_TYPE4 = 5;
const int LANG_INDEX_TYPE5 = 6;
const int LANG_INDEX_TYPE6 = 7;
const int LANG_INDEX_TYPE7 = 8;

const int COPYDATA_PARAMS = 0;
const int COPYDATA_FILENAMESA = 1;
const int COPYDATA_FILENAMESW = 2;

#define PURE_LC_NONE    0
#define PURE_LC_BOL      1
#define PURE_LC_WSP      2

#define DECSEP_DOT        0
#define DECSEP_COMMA    1
#define DECSEP_BOTH      2

#define DROPBOX_AVAILABLE 1
#define ONEDRIVE_AVAILABLE 2
#define GOOGLEDRIVE_AVAILABLE 4

const TCHAR fontSizeStrs[][3] = {TEXT(""), TEXT("5"), TEXT("6"), TEXT("7"), TEXT("8"), TEXT("9"), TEXT("10"), TEXT("11"), TEXT("12"), TEXT(
					 "14"), TEXT("16"), TEXT("18"), TEXT("20"), TEXT("22"), TEXT("24"), TEXT("26"), TEXT("28")};

const TCHAR localConfFile[] = TEXT("doLocalConf.xml");
const TCHAR notepadStyleFile[] = TEXT("asNotepad.xml");
const TCHAR pluginsForAllUsersFile[] = TEXT("pluginsForAllUsers.xml");

void cutString(const TCHAR * str2cut, std::vector<generic_string> & patternVect);

struct Position {
	int _firstVisibleLine = 0;
	int _startPos = 0;
	int _endPos = 0;
	int _xOffset = 0;
	int _selMode = 0;
	int _scrollWidth = 1;
	int _offset = 0;
	int _wrapCount = 0;
};

struct MapPosition {
	int32_t _firstVisibleDisplayLine = -1;
	int32_t _firstVisibleDocLine = -1; // map
	int32_t _lastVisibleDocLine = -1;  // map
	int32_t _nbLine = -1;              // map
	int32_t _higherPos = -1;           // map
	int32_t _width = -1;
	int32_t _height = -1;
	int32_t _wrapIndentMode = -1;
	int64_t _KByteInDoc = _maxPeekLenInKB;
	bool _isWrap = false;
	bool isValid() const { return (_firstVisibleDisplayLine != -1); }
	bool canScroll() const { return (_KByteInDoc < _maxPeekLenInKB); } // _nbCharInDoc < _maxPeekLen : Don't scroll the document for the performance issue
private:
	int64_t _maxPeekLenInKB = 512; // 512 KB
};

struct sessionFileInfo : public Position {
	sessionFileInfo(const TCHAR * fn, const TCHAR * ln, int encoding, bool userReadOnly,
	    const Position& pos, const TCHAR * backupFilePath, FILETIME originalFileLastModifTimestamp, const MapPosition & mapPos) :
		_isUserReadOnly(userReadOnly), _encoding(encoding), Position(pos), _originalFileLastModifTimestamp(originalFileLastModifTimestamp), _mapPos(mapPos)
	{
		if(fn) _fileName = fn;
		if(ln) _langName = ln;
		if(backupFilePath) _backupFilePath = backupFilePath;
	}
	sessionFileInfo(generic_string fn) : _fileName(fn) 
	{
	}
	generic_string _fileName;
	generic_string _langName;
	std::vector<size_t> _marks;
	std::vector<size_t> _foldStates;
	int _encoding = -1;
	bool _isUserReadOnly = false;
	bool _isMonitoring = false;
	generic_string _backupFilePath;
	FILETIME _originalFileLastModifTimestamp = {};
	MapPosition _mapPos;
};

struct Session {
	size_t nbMainFiles() const { return _mainViewFiles.size(); }
	size_t nbSubFiles() const { return _subViewFiles.size(); }
	size_t _activeView = 0;
	size_t _activeMainIndex = 0;
	size_t _activeSubIndex = 0;
	bool _includeFileBrowser = false;
	generic_string _fileBrowserSelectedItem;
	std::vector<sessionFileInfo> _mainViewFiles;
	std::vector<sessionFileInfo> _subViewFiles;
	std::vector<generic_string> _fileBrowserRoots;
};

struct CmdLineParams {
	bool _isNoPlugin = false;
	bool _isReadOnly = false;
	bool _isNoSession = false;
	bool _isNoTab = false;
	bool _isPreLaunch = false;
	bool _showLoadingTime = false;
	bool _alwaysOnTop = false;
	int _line2go   = -1;
	int _column2go = -1;
	int _pos2go = -1;
	POINT _point = { 0 };
	bool _isPointXValid = false;
	bool _isPointYValid = false;
	bool _isSessionFile = false;
	bool _isRecursive = false;
	bool _openFoldersAsWorkspace = false;
	LangType _langType = L_EXTERNAL;
	generic_string _localizationPath;
	generic_string _udlName;
	generic_string _easterEggName;
	uchar _quoteType = 0;
	int _ghostTypingSpeed = -1; // -1: initial value  1: slow  2: fast  3: speed of light
	CmdLineParams()
	{
		_point.x = 0;
		_point.y = 0;
	}
	bool isPointValid() const { return _isPointXValid && _isPointYValid; }
};

// A POD class to send CmdLineParams through WM_COPYDATA and to Notepad_plus::loadCommandlineParams
struct CmdLineParamsDTO {
	bool _isReadOnly = false;
	bool _isNoSession = false;
	bool _isSessionFile = false;
	bool _isRecursive = false;
	bool _openFoldersAsWorkspace = false;

	int _line2go = 0;
	int _column2go = 0;
	int _pos2go = 0;
	LangType _langType = L_EXTERNAL;
	generic_string _udlName;

	static CmdLineParamsDTO FromCmdLineParams(const CmdLineParams& params)
	{
		CmdLineParamsDTO dto;
		dto._isReadOnly = params._isReadOnly;
		dto._isNoSession = params._isNoSession;
		dto._isSessionFile = params._isSessionFile;
		dto._isRecursive = params._isRecursive;
		dto._openFoldersAsWorkspace = params._openFoldersAsWorkspace;
		dto._line2go = params._line2go;
		dto._column2go = params._column2go;
		dto._pos2go = params._pos2go;
		dto._langType = params._langType;
		dto._udlName = params._udlName;
		return dto;
	}
};

struct FloatingWindowInfo {
	int _cont = 0;
	RECT _pos = { 0 };
	FloatingWindowInfo(int cont, int x, int y, int w, int h) : _cont(cont)
	{
		_pos.left       = x;
		_pos.top        = y;
		_pos.right      = w;
		_pos.bottom = h;
	}
};

struct PluginDlgDockingInfo final {
	generic_string _name;
	int _internalID = -1;
	int _currContainer = -1;
	int _prevContainer = -1;
	bool _isVisible = false;
	PluginDlgDockingInfo(const TCHAR* pluginName, int id, int curr, int prev, bool isVis) : 
		_internalID(id), _currContainer(curr), _prevContainer(prev), _isVisible(isVis), _name(pluginName)
	{
	}
	bool operator ==(const PluginDlgDockingInfo& rhs) const
	{
		return _internalID == rhs._internalID and _name == rhs._name;
	}
};

struct ContainerTabInfo final {
	int _cont = 0;
	int _activeTab = 0;
	ContainerTabInfo(int cont, int activeTab) : _cont(cont), _activeTab(activeTab) 
	{
	}
};

struct DockingManagerData final {
	int _leftWidth = 200;
	int _rightWidth = 200;
	int _topHeight = 200;
	int _bottomHight = 200;

	std::vector<FloatingWindowInfo> _flaotingWindowInfo;
	std::vector<PluginDlgDockingInfo> _pluginDockInfo;
	std::vector<ContainerTabInfo> _containerTabInfo;

	bool getFloatingRCFrom(int floatCont, RECT& rc) const
	{
		for(size_t i = 0, fwiLen = _flaotingWindowInfo.size(); i < fwiLen; ++i) {
			if(_flaotingWindowInfo[i]._cont == floatCont) {
				rc.left   = _flaotingWindowInfo[i]._pos.left;
				rc.top  = _flaotingWindowInfo[i]._pos.top;
				rc.right  = _flaotingWindowInfo[i]._pos.right;
				rc.bottom = _flaotingWindowInfo[i]._pos.bottom;
				return true;
			}
		}
		return false;
	}
};

const int FONTSTYLE_NONE = 0;
const int FONTSTYLE_BOLD = 1;
const int FONTSTYLE_ITALIC = 2;
const int FONTSTYLE_UNDERLINE = 4;
const int STYLE_NOT_USED = -1;
const int COLORSTYLE_FOREGROUND = 0x01;
const int COLORSTYLE_BACKGROUND = 0x02;
const int COLORSTYLE_ALL = COLORSTYLE_FOREGROUND|COLORSTYLE_BACKGROUND;

struct NppStyle final {
	int _styleID = STYLE_NOT_USED;
	generic_string _styleDesc;
	COLORREF _fgColor = COLORREF(STYLE_NOT_USED);
	COLORREF _bgColor = COLORREF(STYLE_NOT_USED);
	int _colorStyle = COLORSTYLE_ALL;
	generic_string _fontName;
	int _fontStyle = FONTSTYLE_NONE;
	int _fontSize = STYLE_NOT_USED;
	int _nesting = FONTSTYLE_NONE;
	int _keywordClass = STYLE_NOT_USED;
	generic_string _keywords;
};

struct GlobalOverride final {
	bool isEnable() const { return (enableFg || enableBg || enableFont || enableFontSize || enableBold || enableItalic || enableUnderLine); }
	bool enableFg = false;
	bool enableBg = false;
	bool enableFont = false;
	bool enableFontSize = false;
	bool enableBold = false;
	bool enableItalic = false;
	bool enableUnderLine = false;
};

struct StyleArray {
	//auto size() const { return _styleVect.size(); }
	auto begin() { return _styleVect.begin(); }
	auto end() { return _styleVect.end(); }
	void clear() 
	{
		_styleVect.clear();
	}
	NppStyle & getStyler(size_t index)
	{
		assert(index < _styleVect.size());
		return _styleVect[index];
	}
	void addStyler(int styleID, TiXmlNode * styleNode);
	void addStyler(int styleID, const generic_string & styleName);
	NppStyle * findByID(int id);
	NppStyle * findByName(const generic_string & name);
protected:
	std::vector <NppStyle> _styleVect;
};

struct LexerStyler : public StyleArray {
public:
	LexerStyler & FASTCALL operator = (const LexerStyler & ls)
	{
		if(this != &ls) {
			*(static_cast<StyleArray *>(this)) = ls;
			this->_lexerName = ls._lexerName;
			this->_lexerDesc = ls._lexerDesc;
			this->_lexerUserExt = ls._lexerUserExt;
		}
		return *this;
	}
	void setLexerName(const TCHAR * lexerName) { _lexerName = lexerName; }
	void setLexerDesc(const TCHAR * lexerDesc) { _lexerDesc = lexerDesc; }
	void setLexerUserExt(const TCHAR * lexerUserExt) { _lexerUserExt = lexerUserExt; }
	const TCHAR * getLexerName() const { return _lexerName.c_str(); }
	const TCHAR * getLexerDesc() const { return _lexerDesc.c_str(); }
	const TCHAR * getLexerUserExt() const { return _lexerUserExt.c_str(); }
private:
	generic_string _lexerName;
	generic_string _lexerDesc;
	generic_string _lexerUserExt;
};

struct LexerStylerArray {
	size_t getNbLexer() const { return _lexerStylerVect.size(); }
	void clear() 
	{
		_lexerStylerVect.clear();
	}
	LexerStyler & getLexerFromIndex(size_t index)
	{
		assert(index < _lexerStylerVect.size());
		return _lexerStylerVect[index];
	}
	const TCHAR * getLexerNameFromIndex(size_t index) const { return _lexerStylerVect[index].getLexerName(); }
	const TCHAR * getLexerDescFromIndex(size_t index) const { return _lexerStylerVect[index].getLexerDesc(); }
	LexerStyler * getLexerStylerByName(const TCHAR * lexerName);
	void addLexerStyler(const TCHAR * lexerName, const TCHAR * lexerDesc, const TCHAR * lexerUserExt, TiXmlNode * lexerNode);
private:
	std::vector<LexerStyler> _lexerStylerVect;
};

struct NewDocDefaultSettings final {
	EolType _format = EolType::osdefault;
	UniMode _unicodeMode = uniCookie;
	bool _openAnsiAsUtf8 = true;
	LangType _lang = L_TEXT;
	int _codepage = -1; // -1 when not using
};

struct LangMenuItem final {
	LangType _langType = L_TEXT;
	int _cmdID = -1;
	generic_string _langName;
	LangMenuItem(LangType lt, int cmdID = 0, const generic_string& langName = TEXT("")) : _langType(lt), _cmdID(cmdID), _langName(langName)
	{
	}
};

struct PrintSettings final {
	bool _printLineNumber = true;
	int _printOption = SC_PRINT_COLOURONWHITE;
	generic_string _headerLeft;
	generic_string _headerMiddle;
	generic_string _headerRight;
	generic_string _headerFontName;
	int _headerFontStyle = 0;
	int _headerFontSize = 0;
	generic_string _footerLeft;
	generic_string _footerMiddle;
	generic_string _footerRight;
	generic_string _footerFontName;
	int _footerFontStyle = 0;
	int _footerFontSize = 0;
	RECT _marge = {0};
	PrintSettings() 
	{
		_marge.left = 0; _marge.top = 0; _marge.right = 0; _marge.bottom = 0;
	}
	bool isHeaderPresent() const { return ((_headerLeft != TEXT("")) || (_headerMiddle != TEXT("")) || (_headerRight != TEXT(""))); }
	bool isFooterPresent() const { return ((_footerLeft != TEXT("")) || (_footerMiddle != TEXT("")) || (_footerRight != TEXT(""))); }
	bool isUserMargePresent() const { return ((_marge.left != 0) || (_marge.top != 0) || (_marge.right != 0) || (_marge.bottom != 0)); }
};

class NppDate final {
public:
	NppDate() = default;
	NppDate(ulong year, ulong month, ulong day);
	explicit NppDate(const TCHAR * dateStr);
	// The constructor which makes the date of number of days from now
	// nbDaysFromNow could be negative if user want to make a date in the past
	// if the value of nbDaysFromNow is 0 then the date will be now
	NppDate(int nbDaysFromNow);
	void now();
	generic_string toString() const; // Return Notepad++ date format : YYYYMMDD
	bool operator <(const NppDate & compare) const;
	bool operator >(const NppDate & compare) const;
	bool operator ==(const NppDate & compare) const;
	bool operator !=(const NppDate & compare) const;
private:
	ulong _year  = 2008;
	ulong _month = 4;
	ulong _day   = 26;
};

class MatchedPairConf final {
public:
	bool hasUserDefinedPairs() const { return _matchedPairs.size() != 0; }
	bool hasDefaultPairs() const { return _doParentheses||_doBrackets||_doCurlyBrackets||_doQuotes||_doDoubleQuotes||_doHtmlXmlTag; }
	bool hasAnyPairsPair() const { return hasUserDefinedPairs() || hasDefaultPairs(); }
public:
	std::vector<std::pair<char, char> > _matchedPairs;
	std::vector<std::pair<char, char> > _matchedPairsInit; // used only on init
	bool _doHtmlXmlTag = false;
	bool _doParentheses = false;
	bool _doBrackets = false;
	bool _doCurlyBrackets = false;
	bool _doQuotes = false;
	bool _doDoubleQuotes = false;
};

struct DarkModeConf final {
	bool _isEnabled = false;
	NppDarkMode::ColorTone _colorTone = NppDarkMode::blackTone;
	NppDarkMode::Colors _customColors = NppDarkMode::getDarkModeDefaultColors();
};

struct NppGUI final {
	NppGUI()
	{
		_appPos.left = 0;
		_appPos.top = 0;
		_appPos.right = 1100;
		_appPos.bottom = 700;
		_findWindowPos.left = 0;
		_findWindowPos.top = 0;
		_findWindowPos.right = 0;
		_findWindowPos.bottom = 0;
		_defaultDir[0] = 0;
		_defaultDirExp[0] = 0;
	}

	toolBarStatusType _toolBarStatus = TB_STANDARD;
	bool _toolbarShow = true;
	bool _statusBarShow = true;
	bool _menuBarShow = true;

	// 1st bit : draw top bar;
	// 2nd bit : draw inactive tabs
	// 3rd bit : enable drag & drop
	// 4th bit : reduce the height
	// 5th bit : enable vertical
	// 6th bit : enable multiline

	// 0:don't draw; 1:draw top bar 2:draw inactive tabs 3:draw both 7:draw both+drag&drop
	int _tabStatus = (TAB_DRAWTOPBAR | TAB_DRAWINACTIVETAB | TAB_DRAGNDROP | TAB_REDUCE | TAB_CLOSEBUTTON);
	bool _splitterPos = POS_VERTICAL;
	int _userDefineDlgStatus = UDD_DOCKED;
	int _tabSize = 4;
	bool _tabReplacedBySpace = false;
	bool _finderLinesAreCurrentlyWrapped = false;
	bool _finderPurgeBeforeEverySearch = false;
	int _fileAutoDetection = cdEnabledNew;
	bool _checkHistoryFiles = false;
	RECT _appPos = {0};
	RECT _findWindowPos = { 0 };
	bool _isMaximized = false;
	bool _isMinimizedToTray = false;
	bool _rememberLastSession = true; // remember next session boolean will be written in the settings
	bool _isCmdlineNosessionActivated = false; // used for if -nosession is indicated on the launch time
	bool _detectEncoding = true;
	bool _saveAllConfirm = true;
	bool _setSaveDlgExtFiltToAllTypes = false;
	bool _doTaskList = true;
	bool _maitainIndent = true;
	bool _enableSmartHilite = true;
	bool _smartHiliteCaseSensitive = false;
	bool _smartHiliteWordOnly = true;
	bool _smartHiliteUseFindSettings = false;
	bool _smartHiliteOnAnotherView = false;
	bool _markAllCaseSensitive = false;
	bool _markAllWordOnly = true;
	bool _disableSmartHiliteTmp = false;
	bool _enableTagsMatchHilite = true;
	bool _enableTagAttrsHilite = true;
	bool _enableHiliteNonHTMLZone = false;
	bool _styleMRU = true;
	char _leftmostDelimiter = '(';
	char _rightmostDelimiter = ')';
	bool _delimiterSelectionOnEntireDocument = false;
	bool _backSlashIsEscapeCharacterForSql = true;
	bool _stopFillingFindField = false;
	bool _monospacedFontFindDlg = false;
	bool _findDlgAlwaysVisible = false;
	bool _confirmReplaceInAllOpenDocs = true;
	bool _replaceStopsWithoutFindingNext = false;
	bool _muteSounds = false;
	writeTechnologyEngine _writeTechnologyEngine = defaultTechnology;
	bool _isWordCharDefault = true;
	std::string _customWordChars;
	urlMode _styleURL = urlUnderLineFg;
	generic_string _uriSchemes = TEXT(
		"svn:// cvs:// git:// imap:// irc:// irc6:// ircs:// ldap:// ldaps:// news: telnet:// gopher:// ssh:// sftp:// smb:// skype: snmp:// spotify: steam:// sms: slack:// chrome:// bitcoin:");
	NewDocDefaultSettings _newDocDefaultSettings;
	generic_string _dateTimeFormat = TEXT("yyyy-MM-dd HH:mm:ss");
	bool _dateTimeReverseDefaultOrder = false;
	void setTabReplacedBySpace(bool b) { _tabReplacedBySpace = b; };
	const NewDocDefaultSettings & getNewDocDefaultSettings() const { return _newDocDefaultSettings; };
	std::vector<LangMenuItem> _excludedLangList;
	bool _isLangMenuCompact = true;

	PrintSettings _printSettings;
	BackupFeature _backup = bak_none;
	bool _useDir = false;
	generic_string _backupDir;
	DockingManagerData _dockingData;
	GlobalOverride _globalOverride;
	enum AutocStatus {autoc_none, autoc_func, autoc_word, autoc_both};

	AutocStatus _autocStatus = autoc_both;
	size_t _autocFromLen = 1;
	bool _autocIgnoreNumbers = true;
	bool _funcParams = true;
	MatchedPairConf _matchedPairConf;
	generic_string _definedSessionExt;
	generic_string _definedWorkspaceExt;
	generic_string _commandLineInterpreter = TEXT("cmd");

	struct AutoUpdateOptions {
		bool _doAutoUpdate = true;
		int _intervalDays = 15;
		NppDate _nextUpdateDate;
		AutoUpdateOptions() : _nextUpdateDate(NppDate()) 
		{
		}
	} _autoUpdateOpt;
	bool _doesExistUpdater = false;
	int _caretBlinkRate = 600;
	int _caretWidth = 1;
	bool _enableMultiSelection = false;
	bool _shortTitlebar = false;
	OpenSaveDirSetting _openSaveDir = dir_followCurrent;
	TCHAR _defaultDir[MAX_PATH];
	TCHAR _defaultDirExp[MAX_PATH]; //expanded environment variables
	generic_string _themeName;
	MultiInstSetting _multiInstSetting = monoInst;
	bool _fileSwitcherWithoutExtColumn = true;
	int _fileSwitcherExtWidth = 50;
	bool _fileSwitcherWithoutPathColumn = true;
	int _fileSwitcherPathWidth = 50;
	bool isSnapshotMode() const { return _isSnapshotMode && _rememberLastSession && !_isCmdlineNosessionActivated; };
	bool _isSnapshotMode = true;
	size_t _snapshotBackupTiming = 7000;
	generic_string _cloudPath; // this option will never be read/written from/to config.xml
	uchar _availableClouds = '\0'; // this option will never be read/written from/to config.xml
	enum SearchEngineChoice { se_custom = 0, se_duckDuckGo = 1, se_google = 2, se_bing = 3, se_yahoo = 4, se_stackoverflow = 5 };
	SearchEngineChoice _searchEngineChoice = se_google;
	generic_string _searchEngineCustom;
	bool _isFolderDroppedOpenFiles = false;
	bool _isDocPeekOnTab = false;
	bool _isDocPeekOnMap = false;
	DarkModeConf _darkmode;
};

struct ScintillaViewParams {
	bool _lineNumberMarginShow = true;
	bool _lineNumberMarginDynamicWidth = true;
	bool _bookMarkMarginShow = true;
	folderStyle _folderStyle = FOLDER_STYLE_BOX;  //"simple", "arrow", "circle", "box" and "none"
	lineWrapMethod _lineWrapMethod = LINEWRAP_ALIGNED;
	bool _foldMarginShow = true;
	bool _indentGuideLineShow = true;
	bool _currentLineHilitingShow = true;
	bool _wrapSymbolShow = false;
	bool _doWrap = false;
	bool _isEdgeBgMode = false;
	std::vector<size_t> _edgeMultiColumnPos;
	int _zoom = 0;
	int _zoom2 = 0;
	bool _whiteSpaceShow = false;
	bool _eolShow = false;
	int _borderWidth = 2;
	bool _scrollBeyondLastLine = true;
	bool _rightClickKeepsSelection = false;
	bool _disableAdvancedScrolling = false;
	bool _doSmoothFont = false;
	bool _showBorderEdge = true;
	uchar _paddingLeft = 0;  // 0-9 pixel
	uchar _paddingRight = 0; // 0-9 pixel
	// distractionFreeDivPart is used for divising the fullscreen pixel width.
	// the result of division will be the left & right padding in Distraction Free mode
	uchar _distractionFreeDivPart = 4;     // 3-9 parts
	int getDistractionFreePadding(int editViewWidth) const 
	{
		const int defaultDiviser = 4;
		int diviser = _distractionFreeDivPart > 2 ? _distractionFreeDivPart : defaultDiviser;
		int paddingLen = editViewWidth / diviser;
		if(paddingLen <= 0)
			paddingLen = editViewWidth / defaultDiviser;
		return paddingLen;
	};
};

const int NB_LIST = 20;
const int NB_MAX_LRF_FILE = 30;
const int NB_MAX_USER_LANG = 30;
const int NB_MAX_EXTERNAL_LANG = 30;
const int NB_MAX_IMPORTED_UDL = 50;

const int NB_MAX_FINDHISTORY_FIND       = 30;
const int NB_MAX_FINDHISTORY_REPLACE = 30;
const int NB_MAX_FINDHISTORY_PATH       = 30;
const int NB_MAX_FINDHISTORY_FILTER  = 20;

const int MASK_ReplaceBySpc = 0x80;
const int MASK_TabSize = 0x7F;

struct Lang final {
	LangType _langID = L_TEXT;
	generic_string _langName;
	const TCHAR* _defaultExtList = nullptr;
	const TCHAR* _langKeyWordList[NB_LIST];
	const TCHAR* _pCommentLineSymbol = nullptr;
	const TCHAR* _pCommentStart = nullptr;
	const TCHAR* _pCommentEnd = nullptr;
	bool _isTabReplacedBySpace = false;
	int _tabSize = -1;
	Lang()
	{
		for(int i = 0; i < NB_LIST; _langKeyWordList[i] = NULL, ++i)
			;
	}
	Lang(LangType langID, const TCHAR * name) : _langID(langID), _langName(name ? name : TEXT(""))
	{
		for(int i = 0; i < NB_LIST; _langKeyWordList[i] = NULL, ++i);
	}
	~Lang() = default;
	void setDefaultExtList(const TCHAR * extLst) { _defaultExtList = extLst; }
	void setCommentLineSymbol(const TCHAR * commentLine) { _pCommentLineSymbol = commentLine; }
	void setCommentStart(const TCHAR * commentStart) { _pCommentStart = commentStart; }
	void setCommentEnd(const TCHAR * commentEnd) { _pCommentEnd = commentEnd; }
	void setTabInfo(int tabInfo)
	{
		if(tabInfo != -1 && tabInfo & MASK_TabSize) {
			_isTabReplacedBySpace = (tabInfo & MASK_ReplaceBySpc) != 0;
			_tabSize = tabInfo & MASK_TabSize;
		}
	}
	const TCHAR * getDefaultExtList() const { return _defaultExtList; }
	void setWords(const TCHAR * words, int index) { _langKeyWordList[index] = words; }
	const TCHAR * getWords(int index) const { return _langKeyWordList[index]; }
	LangType getLangID() const { return _langID; }
	const TCHAR * getLangName() const { return _langName.c_str(); }
	int getTabInfo() const { return (_tabSize == -1) ? -1 : ((_isTabReplacedBySpace ? 0x80 : 0x00) | _tabSize); }
};

class UserLangContainer final {
public:
	UserLangContainer() : _name(TEXT("new user define")), _ext(TEXT("")), _udlVersion(TEXT("")) 
	{
		for(int i = 0; i < SCE_USER_KWLIST_TOTAL; ++i) *_keywordLists[i] = '\0';
	}
	UserLangContainer(const TCHAR * name, const TCHAR * ext, bool isDarkModeTheme, const TCHAR * udlVer) :
		_name(name), _ext(ext), _isDarkModeTheme(isDarkModeTheme), _udlVersion(udlVer) 
	{
		for(int i = 0; i < SCE_USER_KWLIST_TOTAL; ++i) *_keywordLists[i] = '\0';
	}
	UserLangContainer & operator = (const UserLangContainer & ulc)
	{
		if(this != &ulc) {
			this->_name = ulc._name;
			this->_ext = ulc._ext;
			this->_isDarkModeTheme = ulc._isDarkModeTheme;
			this->_udlVersion = ulc._udlVersion;
			this->_isCaseIgnored = ulc._isCaseIgnored;
			this->_styles = ulc._styles;
			this->_allowFoldOfComments = ulc._allowFoldOfComments;
			this->_forcePureLC = ulc._forcePureLC;
			this->_decimalSeparator = ulc._decimalSeparator;
			this->_foldCompact = ulc._foldCompact;
			for(NppStyle & st : this->_styles) {
				if(st._bgColor == COLORREF(-1))
					st._bgColor = GetColorRef(SClrWhite); //white
				if(st._fgColor == COLORREF(-1))
					st._fgColor = GetColorRef(SClrBlack); //black
			}
			for(int i = 0; i < SCE_USER_KWLIST_TOTAL; ++i)
				wcscpy_s(this->_keywordLists[i], ulc._keywordLists[i]);
			for(int i = 0; i < SCE_USER_TOTAL_KEYWORD_GROUPS; ++i)
				_isPrefix[i] = ulc._isPrefix[i];
		}
		return *this;
	}
	const TCHAR * getName() { return _name.c_str(); }
	const TCHAR * getExtention() { return _ext.c_str(); }
	const TCHAR * getUdlVersion() { return _udlVersion.c_str(); }
private:
	StyleArray _styles;
	generic_string _name;
	generic_string _ext;
	generic_string _udlVersion;
	bool _isDarkModeTheme = false;
	TCHAR _keywordLists[SCE_USER_KWLIST_TOTAL][max_char];
	bool _isPrefix[SCE_USER_TOTAL_KEYWORD_GROUPS] = {false};
	bool _isCaseIgnored = false;
	bool _allowFoldOfComments = false;
	int _forcePureLC = PURE_LC_NONE;
	int _decimalSeparator = DECSEP_DOT;
	bool _foldCompact = false;

	// nakama zone
	friend class Notepad_plus;
	friend class ScintillaEditView;
	friend class NppParameters;
	friend class SharedParametersDialog;
	friend class FolderStyleDialog;
	friend class KeyWordsStyleDialog;
	friend class CommentStyleDialog;
	friend class SymbolsStyleDialog;
	friend class UserDefineDialog;
	friend class StylerDlg;
};

#define MAX_EXTERNAL_LEXER_NAME_LEN 16
#define MAX_EXTERNAL_LEXER_DESC_LEN 32

class ExternalLangContainer final {
public:
	TCHAR _name[MAX_EXTERNAL_LEXER_NAME_LEN];
	TCHAR _desc[MAX_EXTERNAL_LEXER_DESC_LEN];
	ExternalLangContainer(const TCHAR* name, const TCHAR* desc)
	{
		generic_strncpy(_name, name, MAX_EXTERNAL_LEXER_NAME_LEN);
		generic_strncpy(_desc, desc, MAX_EXTERNAL_LEXER_DESC_LEN);
	}
};

struct FindHistory final {
	enum searchMode {normal, extended, regExpr};
	enum transparencyMode {none, onLossingFocus, persistant};
	bool _isSearch2ButtonsMode = false;
	int _nbMaxFindHistoryPath    = 10;
	int _nbMaxFindHistoryFilter  = 10;
	int _nbMaxFindHistoryFind    = 10;
	int _nbMaxFindHistoryReplace = 10;
	std::vector<generic_string> _findHistoryPaths;
	std::vector<generic_string> _findHistoryFilters;
	std::vector<generic_string> _findHistoryFinds;
	std::vector<generic_string> _findHistoryReplaces;
	bool _isMatchWord = false;
	bool _isMatchCase = false;
	bool _isWrap = true;
	bool _isDirectionDown = true;
	bool _dotMatchesNewline = false;
	bool _isFifRecuisive = true;
	bool _isFifInHiddenFolder = false;
	bool _isFifProjectPanel_1 = false;
	bool _isFifProjectPanel_2 = false;
	bool _isFifProjectPanel_3 = false;
	searchMode _searchMode = normal;
	transparencyMode _transparencyMode = onLossingFocus;
	int _transparency = 150;
	bool _isFilterFollowDoc = false;
	bool _isFolderFollowDoc = false;
	// Allow regExpr backward search: this option is not present in UI, only to modify in config.xml
	bool _regexBackward4PowerUser = false;
};

class LocalizationSwitcher final {
	friend class NppParameters;
public:
	struct LocalizationDefinition {
		const wchar_t * _langName = nullptr;
		const wchar_t * _xmlFileName = nullptr;
	};
	bool addLanguageFromXml(const std::wstring& xmlFullPath);
	std::wstring getLangFromXmlFileName(const wchar_t * fn) const;
	std::wstring getXmlFilePathFromLangName(const wchar_t * langName) const;
	bool switchToLang(const wchar_t * lang2switch) const;
	size_t size() const
	{
		return _localizationList.size();
	}
	std::pair<std::wstring, std::wstring> getElementFromIndex(size_t index) const
	{
		if(index >= _localizationList.size())
			return std::pair<std::wstring, std::wstring>(std::wstring(), std::wstring());
		return _localizationList[index];
	}
	void setFileName(const char * fn)
	{
		if(fn)
			_fileName = fn;
	}
	std::string getFileName() const { return _fileName; }
private:
	std::vector< std::pair< std::wstring, std::wstring > > _localizationList;
	std::wstring _nativeLangPath;
	std::string _fileName;
};

class ThemeSwitcher final {
	friend class NppParameters;
public:
	void addThemeFromXml(const generic_string& xmlFullPath) 
	{
		_themeList.push_back(std::pair<generic_string, generic_string>(getThemeFromXmlFileName(xmlFullPath.c_str()), xmlFullPath));
	}
	void addDefaultThemeFromXml(const generic_string& xmlFullPath) 
	{
		_themeList.push_back(std::pair<generic_string, generic_string>(_defaultThemeLabel, xmlFullPath));
	}
	generic_string getThemeFromXmlFileName(const TCHAR * fn) const;
	generic_string getXmlFilePathFromThemeName(const TCHAR * themeName) const 
	{
		if(!themeName || themeName[0])
			return generic_string();
		generic_string themePath = _stylesXmlPath;
		return themePath;
	}
	bool themeNameExists(const TCHAR * themeName) 
	{
		for(size_t i = 0; i < _themeList.size(); ++i) {
			auto themeNameOnList = getElementFromIndex(i).first;
			if(sstreq(themeName, themeNameOnList.c_str()))
				return true;
		}
		return false;
	}
	size_t size() const { return _themeList.size(); }
	std::pair<generic_string, generic_string> & getElementFromIndex(size_t index)
	{
		assert(index < _themeList.size());
		return _themeList[index];
	}
	void setThemeDirPath(generic_string themeDirPath) { _themeDirPath = themeDirPath; }
	generic_string getThemeDirPath() const { return _themeDirPath; }
	generic_string getDefaultThemeLabel() const { return _defaultThemeLabel; }
	generic_string getSavePathFrom(const generic_string& path) const 
	{
		const auto iter = _themeStylerSavePath.find(path);
		return (iter == _themeStylerSavePath.end()) ? TEXT("") : iter->second;
	}
	void addThemeStylerSavePath(generic_string key, generic_string val) 
	{
		_themeStylerSavePath[key] = val;
	}
private:
	std::vector<std::pair<generic_string, generic_string> > _themeList;
	std::map<generic_string, generic_string> _themeStylerSavePath;
	generic_string _themeDirPath;
	const generic_string _defaultThemeLabel = TEXT("Default (stylers.xml)");
	generic_string _stylesXmlPath;
};

class PluginList final {
public:
	void add(generic_string fn, bool isInBL)
	{
		_list.push_back(std::pair<generic_string, bool>(fn, isInBL));
	}
private:
	std::vector<std::pair<generic_string, bool> >_list;
};

struct UdlXmlFileState final {
	TiXmlDocument* _udlXmlDoc = nullptr;
	bool _isDirty = false;
	std::pair<uchar, uchar> _indexRange;
	UdlXmlFileState(TiXmlDocument* doc, bool isDirty, std::pair<uchar, uchar> range) : _udlXmlDoc(doc), _isDirty(isDirty), _indexRange(range) 
	{
	}
};

const int NB_LANG = 100;
const int RECENTFILES_SHOWFULLPATH = -1;
const int RECENTFILES_SHOWONLYFILENAME = 0;

class NppParameters final {
private:
	static NppParameters* getInstancePointer() 
	{
		static NppParameters * instance = new NppParameters;
		return instance;
	}
public:
	static NppParameters& getInstance() { return *getInstancePointer(); }
	static LangType getLangIDFromStr(const TCHAR * langName);
	static generic_string getLocPathFromStr(const generic_string & localizationCode);
	bool load();
	bool reloadLang();
	bool reloadStylers(const TCHAR * stylePath = nullptr);
	void destroyInstance();
	generic_string getSettingsFolder();
	bool _isTaskListRBUTTONUP_Active = false;
	int L_END;
	NppGUI & getNppGUI() { return _nppGUI; }
	const TCHAR * getWordList(LangType langID, int typeIndex) const;
	Lang * getLangFromID(LangType langID) const;
	Lang * getLangFromIndex(size_t i) const;
	int getNbLang() const { return _nbLang; }
	LangType getLangFromExt(const TCHAR * ext);
	const TCHAR * getLangExtFromName(const TCHAR * langName) const;
	const TCHAR * getLangExtFromLangType(LangType langType) const;
	int getNbLRFile() const { return _nbRecentFile; }
	generic_string * getLRFile(int index) const { return _LRFileList[index]; }
	void setNbMaxRecentFile(int nb) { _nbMaxRecentFile = nb; }
	int getNbMaxRecentFile() const { return _nbMaxRecentFile; }
	void setPutRecentFileInSubMenu(bool doSubmenu) { _putRecentFileInSubMenu = doSubmenu; }
	bool putRecentFileInSubMenu() const { return _putRecentFileInSubMenu; }
	void setRecentFileCustomLength(int len) { _recentFileCustomLength = len; }
	int getRecentFileCustomLength() const { return _recentFileCustomLength; }
	const ScintillaViewParams& getSVP() const { return _svp; }
	bool writeRecentFileHistorySettings(int nbMaxFile = -1) const;
	bool writeHistory(const TCHAR * fullpath);
	bool writeProjectPanelsSettings() const;
	bool writeFileBrowserSettings(const std::vector<generic_string> & rootPath, const generic_string & latestSelectedItemPath) const;
	TiXmlNode * getChildElementByAttribut(TiXmlNode * pere, const TCHAR * childName, const TCHAR * attributName, const TCHAR * attributVal) const;
	bool writeScintillaParams();
	void createXmlTreeFromGUIParams();
	// return "" if saving file succeeds, otherwise return the new saved file path
	generic_string writeStyles(LexerStylerArray & lexersStylers, StyleArray & globalStylers); 
	bool insertTabInfo(const TCHAR * langName, int tabInfo);
	LexerStylerArray & getLStylerArray() { return _lexerStylerVect; }
	StyleArray & getGlobalStylers() { return _widgetStyleArray; }
	StyleArray & getMiscStylerArray() { return _widgetStyleArray; }
	GlobalOverride & getGlobalOverrideStyle() { return _nppGUI._globalOverride; }
	COLORREF getCurLineHilitingColour();
	void setCurLineHilitingColour(COLORREF colour2Set);
	void setFontList(HWND hWnd);
	bool isInFontList(const generic_string& fontName2Search) const;
	const std::vector<generic_string>& getFontList() const { return _fontlist; }
	HFONT getDefaultUIFont();
	int getNbUserLang() const { return _nbUserLang; }
	UserLangContainer & getULCFromIndex(size_t i) { return *_userLangArray[i]; }
	UserLangContainer * getULCFromName(const TCHAR * userLangName);
	int getNbExternalLang() const { return _nbExternalLang; }
	int getExternalLangIndexFromName(const TCHAR * externalLangName) const;
	ExternalLangContainer & getELCFromIndex(int i) { return *_externalLangArray[i]; }
	bool ExternalLangHasRoom() const { return _nbExternalLang < NB_MAX_EXTERNAL_LANG; }
	void getExternalLexerFromXmlTree(TiXmlDocument * doc);
	std::vector<TiXmlDocument *> * getExternalLexerDoc() { return &_pXmlExternalLexerDoc; }
	void writeDefaultUDL();
	void writeNonDefaultUDL();
	void writeNeed2SaveUDL();
	void writeShortcuts();
	void writeSession(const Session & session, const TCHAR * fileName = NULL);
	bool writeFindHistory();
	bool isExistingUserLangName(const TCHAR * newName) const;
	const TCHAR * getUserDefinedLangNameFromExt(TCHAR * ext, TCHAR * fullName) const;
	int addUserLangToEnd(const UserLangContainer & userLang, const TCHAR * newName);
	void removeUserLang(size_t index);
	bool isExistingExternalLangName(const TCHAR * newName) const;
	int addExternalLangToEnd(ExternalLangContainer * externalLang);
	TiXmlDocumentA * getNativeLangA() const { return _pXmlNativeLangDocA; }
	TiXmlDocument * getToolIcons() const { return _pXmlToolIconsDoc; }
	bool isTransparentAvailable() const { return (_transparentFuncAddr != NULL); }

	// 0 <= percent < 256
	// if (percent == 255) then opacq
	void SetTransparent(HWND hwnd, int percent);
	void removeTransparent(HWND hwnd);
	void setCmdlineParam(const CmdLineParamsDTO & cmdLineParams) { _cmdLineParams = cmdLineParams; }
	const CmdLineParamsDTO & getCmdLineParams() const { return _cmdLineParams; }
	const generic_string& getCmdLineString() const { return _cmdLineString; }
	void setCmdLineString(const generic_string& str) { _cmdLineString = str; }
	void setFileSaveDlgFilterIndex(int ln) { _fileSaveDlgFilterIndex = ln; };
	int getFileSaveDlgFilterIndex() const { return _fileSaveDlgFilterIndex; }
	bool isRemappingShortcut() const { return _shortcuts.size() != 0; }
	std::vector<CommandShortcut> & getUserShortcuts() { return _shortcuts; }
	std::vector<size_t> & getUserModifiedShortcuts() { return _customizedShortcuts; }
	void addUserModifiedIndex(size_t index);
	std::vector<MacroShortcut> & getMacroList() { return _macros; }
	std::vector<UserCommand> & getUserCommandList() { return _userCommands; }
	std::vector<PluginCmdShortcut> & getPluginCommandList() { return _pluginCommands; }
	std::vector<size_t> & getPluginModifiedKeyIndices() { return _pluginCustomizedCmds; }
	void addPluginModifiedIndex(size_t index);
	std::vector<ScintillaKeyMap> & getScintillaKeyList() { return _scintillaKeyCommands; }
	std::vector<int> & getScintillaModifiedKeyIndices() { return _scintillaModifiedKeyIndices; }
	void addScintillaModifiedIndex(int index);
	std::vector<MenuItemUnit> & getContextMenuItems() { return _contextMenuItems; }
	const Session & getSession() const { return _session; }
	bool hasCustomContextMenu() const { return !_contextMenuItems.empty(); }
	void setAccelerator(Accelerator * pAccel) { _pAccelerator = pAccel; }
	Accelerator * getAccelerator() { return _pAccelerator; }
	void setScintillaAccelerator(ScintillaAccelerator * pScintAccel) { _pScintAccelerator = pScintAccel; }
	ScintillaAccelerator * getScintillaAccelerator() { return _pScintAccelerator; }
	generic_string getNppPath() const { return _nppPath; }
	generic_string getContextMenuPath() const { return _contextMenuPath; }
	const TCHAR * getAppDataNppDir() const { return _appdataNppDir.c_str(); }
	const TCHAR * getPluginRootDir() const { return _pluginRootDir.c_str(); }
	const TCHAR * getPluginConfDir() const { return _pluginConfDir.c_str(); }
	const TCHAR * getUserPluginConfDir() const { return _userPluginConfDir.c_str(); }
	const TCHAR * getWorkingDir() const { return _currentDirectory.c_str(); }
	const TCHAR * getWorkSpaceFilePath(int i) const { return (i < 0 || i > 2) ? nullptr : _workSpaceFilePathes[i].c_str(); }
	const std::vector<generic_string> getFileBrowserRoots() const { return _fileBrowserRoot; }
	generic_string getFileBrowserSelectedItemPath() const { return _fileBrowserSelectedItemPath; }
	void setWorkSpaceFilePath(int i, const TCHAR * wsFile);
	void setWorkingDir(const TCHAR * newPath);
	void setStartWithLocFileName(const generic_string& locPath) { _startWithLocFileName = locPath; }
	void setFunctionListExportBoolean(bool doIt) { _doFunctionListExport = doIt; }
	bool doFunctionListExport() const { return _doFunctionListExport; }
	void setPrintAndExitBoolean(bool doIt) { _doPrintAndExit = doIt; }
	bool doPrintAndExit() const { return _doPrintAndExit; }
	bool loadSession(Session & session, const TCHAR * sessionFileName);
	int langTypeToCommandID(LangType lt) const;
	WNDPROC getEnableThemeDlgTexture() const { return _enableThemeDialogTextureFuncAddr; }

	struct FindDlgTabTitiles final {
		generic_string _find;
		generic_string _replace;
		generic_string _findInFiles;
		generic_string _findInProjects;
		generic_string _mark;
	};

	FindDlgTabTitiles & getFindDlgTabTitiles() { return _findDlgTabTitiles; }
	bool asNotepadStyle() const { return _asNotepadStyle; }
	bool reloadPluginCmds() { return getPluginCmdsFromXmlTree(); }
	bool getContextMenuFromXmlTree(HMENU mainMenuHadle, HMENU pluginsMenu);
	bool reloadContextMenuFromXmlTree(HMENU mainMenuHadle, HMENU pluginsMenu);
	winVer getWinVersion() const { return _winVersion; }
	generic_string getWinVersionStr() const;
	generic_string getWinVerBitStr() const;
	FindHistory & getFindHistory() { return _findHistory; }
	bool _isFindReplacing = false; // an on the fly variable for find/replace functions
	void safeWow64EnableWow64FsRedirection(BOOL Wow64FsEnableRedirection);
	LocalizationSwitcher & getLocalizationSwitcher() { return _localizationSwitcher; }
	ThemeSwitcher & getThemeSwitcher() { return _themeSwitcher; }
	std::vector<generic_string> & getBlackList() { return _blacklist; }
	bool isInBlackList(const TCHAR * fn) const;
	PluginList & getPluginList() { return _pluginList; }
	bool importUDLFromFile(const generic_string& sourceFile);
	bool exportUDLToFile(size_t langIndex2export, const generic_string& fileName2save);
	NativeLangSpeaker* getNativeLangSpeaker() { return _pNativeLangSpeaker; }
	void setNativeLangSpeaker(NativeLangSpeaker * nls) { _pNativeLangSpeaker = nls; }
	bool isLocal() const { return _isLocal; }
	void saveConfig_xml();
	generic_string getUserPath() const { return _userPath; }
	generic_string getUserDefineLangFolderPath() const { return _userDefineLangsFolderPath; }
	generic_string getUserDefineLangPath() const { return _userDefineLangPath; }
	bool writeSettingsFilesOnCloudForThe1stTime(const generic_string & cloudSettingsPath);
	void setCloudChoice(const TCHAR * pathChoice);
	void removeCloudChoice();
	bool isCloudPathChanged() const;
	int archType() const { return ARCH_TYPE; }
	COLORREF getCurrentDefaultBgColor() const { return _currentDefaultBgColor; }
	COLORREF getCurrentDefaultFgColor() const { return _currentDefaultFgColor; }
	void setCurrentDefaultBgColor(COLORREF c) { _currentDefaultBgColor = c; }
	void setCurrentDefaultFgColor(COLORREF c) { _currentDefaultFgColor = c; }
	void setCmdSettingsDir(const generic_string& settingsDir) { _cmdSettingsDir = settingsDir; }
	void setTitleBarAdd(const generic_string& titleAdd) { _titleBarAdditional = titleAdd; }
	const generic_string& getTitleBarAdd() const { return _titleBarAdditional; }
	DPIManager _dpiManager;

	generic_string static getSpecialFolderLocation(int folderKind);
	void setUdlXmlDirtyFromIndex(size_t i);
	void setUdlXmlDirtyFromXmlDoc(const TiXmlDocument* xmlDoc);
	void removeIndexFromXmlUdls(size_t i);
private:
	NppParameters();
	~NppParameters();
	// No copy ctor and assignment
	NppParameters(const NppParameters&) = delete;
	NppParameters& operator = (const NppParameters&) = delete;
	// No move ctor and assignment
	NppParameters(NppParameters&&) = delete;
	NppParameters & operator = (NppParameters&&) = delete;

	TiXmlDocument * _pXmlDoc = nullptr;
	TiXmlDocument * _pXmlUserDoc = nullptr;
	TiXmlDocument * _pXmlUserStylerDoc = nullptr;
	TiXmlDocument * _pXmlUserLangDoc = nullptr;
	std::vector<UdlXmlFileState> _pXmlUserLangsDoc;
	TiXmlDocument * _pXmlToolIconsDoc = nullptr;
	TiXmlDocument * _pXmlShortcutDoc = nullptr;
	TiXmlDocument * _pXmlBlacklistDoc = nullptr;
	TiXmlDocumentA * _pXmlNativeLangDocA = nullptr;
	TiXmlDocumentA * _pXmlContextMenuDocA = nullptr;
	std::vector<TiXmlDocument *> _pXmlExternalLexerDoc;
	NppGUI _nppGUI;
	ScintillaViewParams _svp;
	Lang* _langList[NB_LANG] = { nullptr };
	int _nbLang = 0;
	// Recent File History
	generic_string* _LRFileList[NB_MAX_LRF_FILE] = { nullptr };
	int _nbRecentFile = 0;
	int _nbMaxRecentFile = 10;
	int _recentFileCustomLength = RECENTFILES_SHOWFULLPATH; //	<0: Full File Path Name
	//	=0: Only File Name
	//	>0: Custom Entry Length
	FindHistory _findHistory;
	UserLangContainer* _userLangArray[NB_MAX_USER_LANG] = { nullptr };
	uchar _nbUserLang = 0; // won't be exceeded to 255;
	generic_string _userDefineLangsFolderPath;
	generic_string _userDefineLangPath;
	ExternalLangContainer* _externalLangArray[NB_MAX_EXTERNAL_LANG] = { nullptr };
	int _nbExternalLang = 0;
	CmdLineParamsDTO _cmdLineParams;
	generic_string _cmdLineString;
	int _fileSaveDlgFilterIndex = -1;
	// All Styles (colours & fonts)
	LexerStylerArray _lexerStylerVect;
	StyleArray _widgetStyleArray;
	std::vector<generic_string> _fontlist;
	std::vector<generic_string> _blacklist;
	PluginList _pluginList;
	HMODULE _hUXTheme = nullptr;
	WNDPROC _transparentFuncAddr = nullptr;
	WNDPROC _enableThemeDialogTextureFuncAddr = nullptr;
	bool _putRecentFileInSubMenu = false;
	bool _isLocal = false;
	bool _isx64 = false; // by default 32-bit
	generic_string _cmdSettingsDir;
	generic_string _titleBarAdditional;
public:
	void setShortcutDirty() { _isAnyShortcutModified = true; }
	void setAdminMode(bool isAdmin) { _isAdminMode = isAdmin; }
	bool isAdmin() const { return _isAdminMode; }
	bool regexBackward4PowerUser() const { return _findHistory._regexBackward4PowerUser; }
	bool isSelectFgColorEnabled() const { return _isSelectFgColorEnabled; }
private:
	bool _isAnyShortcutModified = false;
	std::vector<CommandShortcut> _shortcuts;                        //main menu shortuts. Static size
	std::vector<size_t> _customizedShortcuts;                       //altered main menu shortcuts. Indices static.
	                                                                // Needed when saving alterations
	std::vector<MacroShortcut> _macros; // macro shortcuts, dynamic size, defined on loading macros and adding/deleting them
	std::vector<UserCommand> _userCommands; //run shortcuts, dynamic size, defined on loading run commands and adding/deleting them
	std::vector<PluginCmdShortcut> _pluginCommands; //plugin commands, dynamic size, defined on loading plugins
	std::vector<size_t> _pluginCustomizedCmds; // plugincommands that have been altered. Indices determined after loading ALL plugins. Needed
		// when saving alterations
	std::vector<ScintillaKeyMap> _scintillaKeyCommands; // scintilla keycommands. Static size
	std::vector<int> _scintillaModifiedKeyIndices; // modified scintilla keys. Indices static, determined by
		// searching for commandId. Needed when saving alterations

	LocalizationSwitcher _localizationSwitcher;
	generic_string _startWithLocFileName;
	ThemeSwitcher _themeSwitcher;
	//vector<generic_string> _noMenuCmdNames;
	std::vector<MenuItemUnit> _contextMenuItems;
	Session _session;
	generic_string _shortcutsPath;
	generic_string _contextMenuPath;
	generic_string _sessionPath;
	generic_string _nppPath;
	generic_string _userPath;
	generic_string _stylerPath;
	generic_string _appdataNppDir; // sentinel of the absence of "doLocalConf.xml" : (_appdataNppDir == TEXT(""))?"doLocalConf.xml present":"doLocalConf.xml absent"
	generic_string _pluginRootDir; // plugins root where all the plugins are installed
	generic_string _pluginConfDir; // plugins config dir where the plugin list is installed
	generic_string _userPluginConfDir; // plugins config dir for per user where the plugin parameters are saved / loaded
	generic_string _currentDirectory;
	generic_string _workSpaceFilePathes[3];
	std::vector<generic_string> _fileBrowserRoot;
	generic_string _fileBrowserSelectedItemPath;
	Accelerator* _pAccelerator = nullptr;
	ScintillaAccelerator* _pScintAccelerator = nullptr;
	FindDlgTabTitiles _findDlgTabTitiles;
	winVer _winVersion = WV_UNKNOWN;
	PlatformArch _platForm = PF_UNKNOWN;
	NativeLangSpeaker * _pNativeLangSpeaker = nullptr;
	COLORREF _currentDefaultBgColor = GetColorRef(SClrWhite); //RGB(0xFF, 0xFF, 0xFF);
	COLORREF _currentDefaultFgColor = GetColorRef(SClrBlack); //RGB(0x00, 0x00, 0x00);
	generic_string _initialCloudChoice;
	generic_string _wingupFullPath;
	generic_string _wingupParams;
	generic_string _wingupDir;
	bool _doFunctionListExport = false;
	bool _doPrintAndExit = false;
	bool _asNotepadStyle = false;
	bool _isElevationRequired = false;
	bool _isAdminMode = false;
	bool _isSelectFgColorEnabled = false;
public:
	generic_string getWingupFullPath() const { return _wingupFullPath; }
	generic_string getWingupParams() const { return _wingupParams; }
	generic_string getWingupDir() const { return _wingupDir; }
	bool shouldDoUAC() const { return _isElevationRequired; }
	void setWingupFullPath(const generic_string& val2set) { _wingupFullPath = val2set; }
	void setWingupParams(const generic_string& val2set) { _wingupParams = val2set; }
	void setWingupDir(const generic_string& val2set) { _wingupDir = val2set; }
	void setElevationRequired(bool val2set) { _isElevationRequired = val2set; }
private:
	void getLangKeywordsFromXmlTree();
	bool getUserParametersFromXmlTree();
	bool getUserStylersFromXmlTree();
	std::pair<uchar, uchar> addUserDefineLangsFromXmlTree(TiXmlDocument * tixmldoc);
	bool getShortcutsFromXmlTree();
	bool getMacrosFromXmlTree();
	bool getUserCmdsFromXmlTree();
	bool getPluginCmdsFromXmlTree();
	bool getScintKeysFromXmlTree();
	bool getSessionFromXmlTree(TiXmlDocument * pSessionDoc, Session& session);
	bool getBlackListFromXmlTree();
	void feedGUIParameters(TiXmlNode * node);
	void feedKeyWordsParameters(TiXmlNode * node);
	void feedFileListParameters(TiXmlNode * node);
	void feedScintillaParam(TiXmlNode * node);
	void feedDockingManager(TiXmlNode * node);
	void duplicateDockingManager(TiXmlNode * dockMngNode, TiXmlElement* dockMngElmt2Clone);
	void feedFindHistoryParameters(TiXmlNode * node);
	void feedProjectPanelsParameters(TiXmlNode * node);
	void feedFileBrowserParameters(TiXmlNode * node);
	bool feedStylerArray(TiXmlNode * node);
	std::pair<uchar, uchar> feedUserLang(TiXmlNode * node);
	void feedUserStyles(TiXmlNode * node);
	void feedUserKeywordList(TiXmlNode * node);
	void feedUserSettings(TiXmlNode * node);
	void FASTCALL feedShortcut(TiXmlNode * node);
	void feedMacros(TiXmlNode * node);
	void feedUserCmds(TiXmlNode * node);
	void feedPluginCustomizedCmds(TiXmlNode * node);
	void feedScintKeys(TiXmlNode * node);
	bool feedBlacklist(TiXmlNode * node);
	void getActions(TiXmlNode * node, Macro & macro);
	bool getShortcuts(TiXmlNode * node, Shortcut & sc);
	void writeStyle2Element(const NppStyle & style2Write, NppStyle & style2Sync, TiXmlElement * element);
	void insertUserLang2Tree(TiXmlNode * node, UserLangContainer * userLang);
	void insertCmd(TiXmlNode * cmdRoot, const CommandShortcut & cmd);
	void insertMacro(TiXmlNode * macrosRoot, const MacroShortcut & macro);
	void insertUserCmd(TiXmlNode * userCmdRoot, const UserCommand & userCmd);
	void insertScintKey(TiXmlNode * scintKeyRoot, const ScintillaKeyMap & scintKeyMap);
	void insertPluginCmd(TiXmlNode * pluginCmdRoot, const PluginCmdShortcut & pluginCmd);
	TiXmlElement * insertGUIConfigBoolNode(TiXmlNode * r2w, const TCHAR * name, bool bVal);
	void insertDockingParamNode(TiXmlNode * GUIRoot);
	void writeExcludedLangList(TiXmlElement * element);
	void writePrintSetting(TiXmlElement * element);
	void initMenuKeys();            //initialise menu keys and scintilla keys. Other keys are initialized on their own
	void initScintillaKeys();       //these functions have to be called first before any modifications are loaded
	int getCmdIdFromMenuEntryItemName(HMENU mainMenuHadle, const generic_string& menuEntryName, const generic_string& menuItemName); // return -1 if not found
	int getPluginCmdIdFromMenuEntryItemName(HMENU pluginsMenu, const generic_string& pluginName, const generic_string& pluginCmdName); // return -1 if not found
	winVer getWindowsVersion();
};
//
#include "Utf8.h"
#include "Utf8_16.h"
//
//#include "sha-256.h"
//void calc_sha_256(uint8_t hash[32], const void *input, size_t len);
//
//#include "Buffer.h"
typedef Buffer * BufferID;       //each buffer has unique ID by which it can be retrieved
#define BUFFER_INVALID  reinterpret_cast<BufferID>(0)

typedef sptr_t Document;

enum DocFileStatus {
	DOC_REGULAR    = 0x01, // should not be combined with anything
	DOC_UNNAMED    = 0x02, // not saved (new ##)
	DOC_DELETED    = 0x04, // doesn't exist in environment anymore, but not DOC_UNNAMED
	DOC_MODIFIED   = 0x08, // File in environment has changed
	DOC_NEEDRELOAD = 0x10  // File is modified & needed to be reload (by log monitoring)
};

enum BufferStatusInfo {
	BufferChangeLanguage    = 0x001,  // Language was altered
	BufferChangeDirty               = 0x002,  // Buffer has changed dirty state
	BufferChangeFormat              = 0x004,  // EOL type was changed
	BufferChangeUnicode             = 0x008,  // Unicode type was changed
	BufferChangeReadonly    = 0x010,  // Readonly state was changed, can be both file and user
	BufferChangeStatus              = 0x020,  // Filesystem Status has changed
	BufferChangeTimestamp   = 0x040,  // Timestamp was changed
	BufferChangeFilename    = 0x080,  // Filename was changed
	BufferChangeRecentTag   = 0x100,  // Recent tag has changed
	BufferChangeLexing              = 0x200,  // Document needs lexing
	BufferChangeMask                = 0x3FF   // Mask: covers all changes
};

enum SavingStatus {
	SaveOK             = 0,
	SaveOpenFailed     = 1,
	SaveWrittingFailed = 2
};

const TCHAR UNTITLED_STR[] = TEXT("new ");

//File manager class maintains all buffers
class FileManager final {
public:
	void init(Notepad_plus* pNotepadPlus, ScintillaEditView* pscratchTilla);
	//void activateBuffer(int index);
	void checkFilesystemChanges(bool bCheckOnlyCurrentBuffer);
	size_t getNbBuffers() const { return _nbBufs; };
	size_t getNbDirtyBuffers() const;
	int getBufferIndexByID(BufferID id);
	Buffer * getBufferByIndex(size_t index);
	Buffer * getBufferByID(BufferID id) { return static_cast<Buffer*>(id); }
	void beNotifiedOfBufferChange(Buffer * theBuf, int mask);
	void closeBuffer(BufferID, ScintillaEditView * identifer);              //called by Notepad++
	void addBufferReference(BufferID id, ScintillaEditView * identifer);    //called by Scintilla etc indirectly
	//ID == BUFFER_INVALID on failure. If Doc == NULL, a new file is created, otherwise data is loaded in given document
	BufferID loadFile(const TCHAR * filename, Document doc = NULL, int encoding = -1, const TCHAR * backupFileName = NULL, FILETIME fileNameTimestamp = {}); 
	BufferID newEmptyDocument();
	//create Buffer from existing Scintilla, used from new Scintillas. If dontIncrease = true, then the new document
	// number isnt increased afterwards.
	//usefull for temporary but neccesary docs
	//If dontRef = false, then no extra reference is added for the doc. Its the responsibility of the caller to do
	// so
	BufferID bufferFromDocument(Document doc,  bool dontIncrease = false, bool dontRef = false);
	BufferID getBufferFromName(const TCHAR * name);
	BufferID getBufferFromDocument(Document doc);
	void setLoadedBufferEncodingAndEol(Buffer* buf, const Utf8_16_Read& UnicodeConvertor, int encoding, EolType bkformat);
	bool reloadBuffer(BufferID id);
	bool reloadBufferDeferred(BufferID id);
	SavingStatus saveBuffer(BufferID id, const TCHAR* filename, bool isCopy = false);
	bool backupCurrentBuffer();
	bool deleteBufferBackup(BufferID id);
	bool deleteFile(BufferID id);
	bool moveFile(BufferID id, const TCHAR * newFilename);
	bool createEmptyFile(const TCHAR * path);
	static FileManager& getInstance() 
	{
		static FileManager instance;
		return instance;
	}
	int getFileNameFromBuffer(BufferID id, TCHAR * fn2copy);
	int docLength(Buffer * buffer) const;
	size_t nextUntitledNewNumber() const;
private:
	struct LoadedFileFormat {
		LoadedFileFormat() = default;
		LangType _language = L_TEXT;
		int _encoding = 0;
		EolType _eolFormat = EolType::osdefault;
	};
	FileManager() = default;
	~FileManager();
	// No copy ctor and assignment
	FileManager(const FileManager&) = delete;
	FileManager& operator = (const FileManager&) = delete;
	// No move ctor and assignment
	FileManager(FileManager&&) = delete;
	FileManager& operator = (FileManager&&) = delete;
	int detectCodepage(char * buf, size_t len);
	bool loadFileData(Document doc, const TCHAR* filename, char * buffer, Utf8_16_Read* UnicodeConvertor, LoadedFileFormat& fileFormat);
	LangType detectLanguageFromTextBegining(const uchar * data, size_t dataLen);
private:
	Notepad_plus* _pNotepadPlus = nullptr;
	ScintillaEditView* _pscratchTilla = nullptr;
	Document _scratchDocDefault;
	std::vector<Buffer*> _buffers;
	BufferID _nextBufferID = 0;
	size_t _nbBufs = 0;
};

#define MainFileManager FileManager::getInstance()

class Buffer final {
	friend class FileManager;
public:
	//Loading a document:
	//constructor with ID.
	//Set a reference (pointer to a container mostly, like DocTabView or ScintillaEditView)
	//Set the position manually if needed
	//Load the document into Scintilla/add to TabBar
	//The entire lifetime if the buffer, the Document has reference count of _atleast_ one
	//Destructor makes sure its purged
	Buffer(FileManager * pManager, BufferID id, Document doc, DocFileStatus type, const TCHAR * fileName);
	// this method 1. copies the file name
	//             2. determinates the language from the ext of file name
	//             3. gets the last modified time
	void setFileName(const TCHAR * fn, LangType defaultLang = L_TEXT);
	const TCHAR * getFullPathName() const { return _fullPathName.c_str(); }
	const TCHAR * getFileName() const { return _fileName; }
	BufferID getID() const { return _id; }
	void increaseRecentTag() 
	{
		_recentTag = ++_recentTagCtr;
		doNotify(BufferChangeRecentTag);
	}
	long getRecentTag() const { return _recentTag; }
	bool checkFileState();
	bool isDirty() const { return _isDirty; }
	bool isReadOnly() const { return (_isUserReadOnly || _isFileReadOnly); };
	bool isUntitled() const { return (_currentStatus == DOC_UNNAMED); }
	bool getFileReadOnly() const { return _isFileReadOnly; }
	void setFileReadOnly(bool ro) 
	{
		_isFileReadOnly = ro;
		doNotify(BufferChangeReadonly);
	}
	bool getUserReadOnly() const { return _isUserReadOnly; }
	void setUserReadOnly(bool ro) 
	{
		_isUserReadOnly = ro;
		doNotify(BufferChangeReadonly);
	}
	EolType getEolFormat() const { return _eolFormat; }
	void setEolFormat(EolType format) 
	{
		_eolFormat = format;
		doNotify(BufferChangeFormat);
	}
	LangType getLangType() const { return _lang; }
	void setLangType(LangType lang, const TCHAR * userLangName = TEXT(""));
	UniMode getUnicodeMode() const { return _unicodeMode; }
	void setUnicodeMode(UniMode mode);
	int getEncoding() const { return _encoding; }
	void setEncoding(int encoding);
	DocFileStatus getStatus() const { return _currentStatus; }
	Document getDocument() { return _doc; }
	void setDirty(bool dirty);
	void setPosition(const Position & pos, ScintillaEditView * identifier);
	Position & getPosition(ScintillaEditView * identifier);
	void setHeaderLineState(const std::vector<size_t> & folds, ScintillaEditView * identifier);
	const std::vector<size_t> & getHeaderLineState(const ScintillaEditView * identifier) const;
	bool isUserDefineLangExt() const { return (_userLangExt[0] != '\0'); }
	const TCHAR * getUserDefineLangName() const { return _userLangExt.c_str(); }
	const TCHAR * getCommentLineSymbol() const
	{
		Lang * l = getCurrentLang();
		return l ? l->_pCommentLineSymbol : 0;
	}
	const TCHAR * getCommentStart() const
	{
		Lang * l = getCurrentLang();
		return l ? l->_pCommentStart : 0;
	}
	const TCHAR * getCommentEnd() const
	{
		Lang * l = getCurrentLang();
		return l ? l->_pCommentEnd : 0;
	}
	bool getNeedsLexing() const { return _needLexer; }
	void setNeedsLexing(bool lex)
	{
		_needLexer = lex;
		doNotify(BufferChangeLexing);
	}
	//these two return reference count after operation
	int addReference(ScintillaEditView * identifier); //if ID not registered, creates a new Position for that ID and new foldstate
	int removeReference(ScintillaEditView * identifier); //reduces reference. If zero, Document is purged
	void setHideLineChanged(bool isHide, int location);
	void setDeferredReload();
	bool getNeedReload() { return _needReloading; }
	void setNeedReload(bool reload) { _needReloading = reload; }
	int docLength() const
	{
		assert(_pManager != nullptr);
		return _pManager->docLength(_id);
	}
	int64_t getFileLength() const; // return file length. -1 if file is not existing.
	enum fileTimeType { 
		ft_created, 
		ft_modified, 
		ft_accessed 
	};
	generic_string getFileTime(fileTimeType ftt) const;
	Lang * getCurrentLang() const;
	bool isModified() const { return _isModified; }
	void setModifiedStatus(bool isModified) { _isModified = isModified; }
	generic_string getBackupFileName() const { return _backupFileName; }
	void setBackupFileName(const generic_string& fileName) { _backupFileName = fileName; }
	FILETIME getLastModifiedTimestamp() const { return _timeStamp; }
	bool isLoadedDirty() const { return _isLoadedDirty; }
	void setLoadedDirty(bool val) { _isLoadedDirty = val; }
	void startMonitoring() 
	{
		_isMonitoringOn = true;
		_eventHandle = ::CreateEvent(nullptr, TRUE, FALSE, nullptr);
	}
	HANDLE getMonitoringEvent() const { return _eventHandle; }
	void stopMonitoring() 
	{
		_isMonitoringOn = false;
		::SetEvent(_eventHandle);
		::CloseHandle(_eventHandle);
	}
	bool isMonitoringOn() const { return _isMonitoringOn; };
	void updateTimeStamp();
	void reload();
	void setMapPosition(const MapPosition & mapPosition) { _mapPosition = mapPosition; };
	MapPosition getMapPosition() const { return _mapPosition; };
	void langHasBeenSetFromMenu() { _hasLangBeenSetFromMenu = true; };
private:
	int indexOfReference(const ScintillaEditView * identifier) const;
	void setStatus(DocFileStatus status) 
	{
		_currentStatus = status;
		doNotify(BufferChangeStatus);
	}
	void doNotify(int mask);
	Buffer(const Buffer&) = delete;
	Buffer& operator =(const Buffer&) = delete;
private:
	FileManager * _pManager = nullptr;
	bool _canNotify = false;
	int _references = 0; // if no references file inaccessible, can be closed
	BufferID _id = nullptr;

	//document properties
	Document _doc;  //invariable
	LangType _lang = L_TEXT;
	generic_string _userLangExt; // it's useful if only (_lang == L_USER)
	bool _isDirty = false;
	EolType _eolFormat = EolType::osdefault;
	UniMode _unicodeMode = uniUTF8;
	int _encoding = -1;
	bool _isUserReadOnly = false;
	bool _needLexer = false; // new buffers do not need lexing, Scintilla takes care of that
	//these properties have to be duplicated because of multiple references
	//All the vectors must have the same size at all times
	std::vector<ScintillaEditView *> _referees; // Instances of ScintillaEditView which contain this buffer
	std::vector<Position> _positions;
	std::vector<std::vector<size_t> > _foldStates;

	//Environment properties
	DocFileStatus _currentStatus = DOC_REGULAR;
	FILETIME _timeStamp = {}; // 0 if it's a new doc
	bool _isFileReadOnly = false;
	generic_string _fullPathName;
	TCHAR * _fileName = nullptr; // points to filename part in _fullPathName
	bool _needReloading = false; // True if Buffer needs to be reloaded on activation
	long _recentTag = -1;
	static long _recentTagCtr;
	// For backup system
	generic_string _backupFileName;
	bool _isModified = false;
	bool _isLoadedDirty = false; // it's the indicator for finding buffer's initial state
	// For the monitoring
	HANDLE _eventHandle = nullptr;
	bool _isMonitoringOn = false;
	bool _hasLangBeenSetFromMenu = false;
	MapPosition _mapPosition;
	std::mutex _reloadFromDiskRequestGuard;
};
//
//#include "TabBar.h"
#ifndef _WIN32_IE
	#define _WIN32_IE       0x0600
#endif //_WIN32_IE

//Notification message
#define TCN_TABDROPPED (TCN_FIRST - 10)
#define TCN_TABDROPPEDOUTSIDE (TCN_FIRST - 11)
#define TCN_TABDELETE (TCN_FIRST - 12)
#define TCN_MOUSEHOVERING (TCN_FIRST - 13)
#define TCN_MOUSELEAVING (TCN_FIRST - 14)
#define TCN_MOUSEHOVERSWITCHING (TCN_FIRST - 15)

#define WM_TABSETSTYLE  (WM_APP + 0x024)

const int marge = 8;
const int nbCtrlMax = 10;

const TCHAR TABBAR_ACTIVEFOCUSEDINDCATOR[64] = TEXT("Active tab focused indicator");
const TCHAR TABBAR_ACTIVEUNFOCUSEDINDCATOR[64] = TEXT("Active tab unfocused indicator");
const TCHAR TABBAR_ACTIVETEXT[64] = TEXT("Active tab text");
const TCHAR TABBAR_INACTIVETEXT[64] = TEXT("Inactive tabs");

struct TBHDR {
	NMHDR _hdr;
	int _tabOrigin;
};

class TabBar : public Window {
public:
	TabBar() = default;
	virtual ~TabBar() = default;
	virtual void destroy();
	virtual void init(HINSTANCE hInst, HWND hwnd, bool isVertical = false, bool isMultiLine = false);
	virtual void reSizeTo(RECT & rc2Ajust);
	int insertAtEnd(const TCHAR * subTabName);
	void activateAt(int index) const;
	void getCurrentTitle(TCHAR * title, int titleLen);
	int32_t getCurrentTabIndex() const { return static_cast<int32_t>(SendMessage(_hSelf, TCM_GETCURSEL, 0, 0)); }
	int32_t getItemCount() const { return static_cast<int32_t>(::SendMessage(_hSelf, TCM_GETITEMCOUNT, 0, 0)); }
	void deletItemAt(size_t index);
	void deletAllItem();
	void setImageList(HIMAGELIST himl);
	size_t nbItem() const { return _nbItem; }
	void setFont(const TCHAR * fontName, int fontSize);
	void setVertical(bool b) { _isVertical = b; }
	void setMultiLine(bool b) { _isMultiLine = b; }
protected:
	size_t _nbItem = 0;
	bool _hasImgLst = false;
	HFONT _hFont = nullptr;
	HFONT _hLargeFont = nullptr;
	HFONT _hVerticalFont = nullptr;
	HFONT _hVerticalLargeFont = nullptr;
	int _ctrlID = 0;
	bool _isVertical = false;
	bool _isMultiLine = false;

	long getRowCount() const { return long(::SendMessage(_hSelf, TCM_GETROWCOUNT, 0, 0)); }
};

struct CloseButtonZone {
	CloseButtonZone();
	bool isHit(int x, int y, const RECT & tabRect, bool isVertical) const;
	RECT getButtonRectFrom(const RECT & tabRect, bool isVertical) const;

	int _width = 0;
	int _height = 0;
};

class TabBarPlus : public TabBar {
public:
	TabBarPlus() = default;
	enum tabColourIndex {
		activeText, 
		activeFocusedTop, 
		activeUnfocusedTop, 
		inactiveText, 
		inactiveBg
	};
	static void doDragNDrop(bool justDoIt) { _doDragNDrop = justDoIt; }
	virtual void init(HINSTANCE hInst, HWND hwnd, bool isVertical = false, bool isMultiLine = false);
	virtual void destroy();
	static bool doDragNDropOrNot() { return _doDragNDrop; }
	int getSrcTabIndex() const { return _nSrcTab; }
	int getTabDraggedIndex() const { return _nTabDragged; }
	POINT getDraggingPoint() const { return _draggingPoint; }
	void resetDraggingPoint() 
	{
		_draggingPoint.x = 0;
		_draggingPoint.y = 0;
	}
	static void doOwnerDrawTab();
	static void doVertical();
	static void doMultiLine();
	static bool isOwnerDrawTab() { return true; }
	static bool drawTopBar() { return _drawTopBar; }
	static bool drawInactiveTab() { return _drawInactiveTab; }
	static bool drawTabCloseButton() { return _drawTabCloseButton; }
	static bool isDbClk2Close() { return _isDbClk2Close; }
	static bool isVertical() { return _isCtrlVertical; }
	static bool isMultiLine() { return _isCtrlMultiLine; }
	static void setDrawTopBar(bool b) 
	{
		_drawTopBar = b;
		doOwnerDrawTab();
	}
	static void setDrawInactiveTab(bool b) 
	{
		_drawInactiveTab = b;
		doOwnerDrawTab();
	}
	static void setDrawTabCloseButton(bool b) 
	{
		_drawTabCloseButton = b;
		doOwnerDrawTab();
	}
	static void setDbClk2Close(bool b) { _isDbClk2Close = b; }
	static void setVertical(bool b) 
	{
		_isCtrlVertical = b;
		doVertical();
	}
	static void setMultiLine(bool b) 
	{
		_isCtrlMultiLine = b;
		doMultiLine();
	}
	static void setColour(COLORREF colour2Set, tabColourIndex i);
protected:
	// it's the boss to decide if we do the drag N drop
	static bool _doDragNDrop;
	// drag N drop members
	bool _mightBeDragging = false;
	int _dragCount = 0;
	bool _isDragging = false;
	bool _isDraggingInside = false;
	int _nSrcTab = -1;
	int _nTabDragged = -1;
	int _previousTabSwapped = -1;
	POINT _draggingPoint; // coordinate of Screen
	WNDPROC _tabBarDefaultProc = nullptr;
	RECT _currentHoverTabRect;
	int _currentHoverTabItem = -1; // -1 : no mouse on any tab
	CloseButtonZone _closeButtonZone;
	bool _isCloseHover = false;
	int _whichCloseClickDown = -1;
	bool _lmbdHit = false; // Left Mouse Button Down Hit
	HWND _tooltips = nullptr;

	LRESULT runProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK TabBarPlus_Proc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam) 
	{
		return (((TabBarPlus*)(::GetWindowLongPtr(hwnd, GWLP_USERDATA)))->runProc(hwnd, Message, wParam, lParam));
	}
	void setActiveTab(int tabIndex);
	void exchangeTabItemData(int oldTab, int newTab);
	void exchangeItemData(POINT point);

	// it's the boss to decide if we do the ownerDraw style tab
	static bool _drawInactiveTab;
	static bool _drawTopBar;
	static bool _drawTabCloseButton;
	static bool _isDbClk2Close;
	static bool _isCtrlVertical;
	static bool _isCtrlMultiLine;
	static COLORREF _activeTextColour;
	static COLORREF _activeTopBarFocusedColour;
	static COLORREF _activeTopBarUnfocusedColour;
	static COLORREF _inactiveTextColour;
	static COLORREF _inactiveBgColour;
	static int _nbCtrl;
	static HWND _hwndArray[nbCtrlMax];

	void drawItem(DRAWITEMSTRUCT * pDrawItemStruct, bool isDarkMode = false);
	void draggingCursor(POINT screenPoint);
	int getTabIndexAt(const POINT & p) { return getTabIndexAt(p.x, p.y); }
	int32_t getTabIndexAt(int x, int y);
	bool isPointInParentZone(POINT screenPoint) const;
	void notify(int notifyCode, int tabIndex);
	void trackMouseEvent(DWORD event2check);
};
//
//#include "ControlsTab.h"
struct DlgInfo {
	Window * _dlg;
	generic_string _name;
	generic_string _internalName;
	DlgInfo(Window * dlg, const TCHAR * name, const TCHAR * internalName = TEXT("")) : _dlg(dlg), _name(name), _internalName(internalName) 
	{
	}
};

typedef std::vector<DlgInfo> WindowVector;

class ControlsTab final : public TabBar {
public:
	ControlsTab() = default;
	virtual ~ControlsTab() = default;
	virtual void init(HINSTANCE hInst, HWND hwnd, bool isVertical = false, bool isMultiLine = false)
	{
		_isVertical = isVertical;
		TabBar::init(hInst, hwnd, false, isMultiLine);
	}
	void createTabs(WindowVector & winVector);
	void destroy()
	{
		TabBar::destroy();
	}
	virtual void reSizeTo(RECT & rc);
	void activateWindowAt(int index);
	void clickedUpdate()
	{
		int indexClicked = int(::SendMessage(_hSelf, TCM_GETCURSEL, 0, 0));
		activateWindowAt(indexClicked);
	}
	void renameTab(size_t index, const TCHAR * newName);
	bool renameTab(const TCHAR * internalName, const TCHAR * newName);
private:
	WindowVector * _pWinVector = nullptr;
	int _current = 0;
};
//
//#include "ColourPicker.h"

#define CPN_COLOURPICKED (BN_CLICKED)

class ColourPicker : public Window {
public:
	ColourPicker() = default;
	~ColourPicker() = default;
	virtual void init(HINSTANCE hInst, HWND parent);
	virtual void destroy();
	void setColour(COLORREF c) { _currentColour = c; }
	COLORREF getColour() const { return _currentColour; }
	bool isEnabled() { return _isEnabled; }
	void setEnabled(bool enabled) { _isEnabled = enabled; }
private:
	COLORREF _currentColour = RGB(0xFF, 0x00, 0x00);
	WNDPROC _buttonDefaultProc = nullptr;
	ColourPopup * _pColourPopup = nullptr;
	bool _isEnabled = true;
	static LRESULT CALLBACK staticWinProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam) 
	{
		return (((ColourPicker*)(::GetWindowLongPtr(hwnd, GWLP_USERDATA)))->runProc(Message, wParam, lParam));
	}
	LRESULT runProc(UINT Message, WPARAM wParam, LPARAM lParam);
	void drawForeground(HDC hDC);
	void drawBackground(HDC hDC);
};
//
//#include "URLCtrl.h"
class URLCtrl : public Window {
public:
	void create(HWND itemHandle, const TCHAR * link, COLORREF linkColor = RGB(0, 0, 255));
	void create(HWND itemHandle, int cmd, HWND msgDest = NULL);
	void destroy();
private:
	void action();
protected:
	generic_string _URL;
	HFONT _hfUnderlined = nullptr;
	HCURSOR _hCursor = nullptr;
	HWND _msgDest = nullptr;
	ulong _cmdID = 0;
	WNDPROC _oldproc = nullptr;
	COLORREF _linkColor = RGB(0xFF, 0xFF, 0xFF);
	COLORREF _visitedColor = RGB(0xFF, 0xFF, 0xFF);
	bool _clicking = false;
	static LRESULT CALLBACK URLCtrlProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
	{
		return ((URLCtrl*)(::GetWindowLongPtr(hwnd, GWLP_USERDATA)))->runProc(hwnd, Message, wParam, lParam);
	}
	LRESULT runProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
};
//
//#include "UserDefineDialog.h"
#define WL_LEN_MAX 1024
#define BOLD_MASK     1
#define ITALIC_MASK   2
const bool DOCK = true;
const bool UNDOCK = false;

class GlobalMappers {
public:
	std::unordered_map<generic_string, int> keywordIdMapper;
	std::unordered_map<int, generic_string> keywordNameMapper;
	std::unordered_map<generic_string, int> styleIdMapper;
	std::unordered_map<int, generic_string> styleNameMapper;
	std::unordered_map<generic_string, int> temp;
	std::unordered_map<generic_string, int>::iterator iter;
	std::unordered_map<int, int> nestingMapper;
	std::unordered_map<int, int> dialogMapper;
	std::unordered_map<int, std::string> setLexerMapper;
	GlobalMappers(); // only default constructor is needed
};

GlobalMappers & globalMappper();

class SharedParametersDialog : public StaticDialog {
	friend class StylerDlg;
public:
	SharedParametersDialog() = default;
	virtual void updateDlg() = 0;
protected:
	//Shared data
	static UserLangContainer * _pUserLang;
	static ScintillaEditView * _pScintilla;
	INT_PTR CALLBACK run_dlgProc(UINT Message, WPARAM wParam, LPARAM lParam);
	bool setPropertyByCheck(HWND hwnd, WPARAM id, bool & bool2set);
	virtual void setKeywords2List(int ctrlID) = 0;
};

class FolderStyleDialog : public SharedParametersDialog {
public:
	FolderStyleDialog() = default;
	void updateDlg();
protected:
	INT_PTR CALLBACK run_dlgProc(UINT Message, WPARAM wParam, LPARAM lParam);
	void setKeywords2List(int ctrlID);
private:
	void retrieve(TCHAR * dest, const TCHAR * toRetrieve, TCHAR * prefix) const;
	URLCtrl _pageLink;
};

class KeyWordsStyleDialog : public SharedParametersDialog {
public:
	KeyWordsStyleDialog() = default;
	void updateDlg();
protected:
	INT_PTR CALLBACK run_dlgProc(UINT Message, WPARAM wParam, LPARAM lParam);
	void setKeywords2List(int id);
};

class CommentStyleDialog : public SharedParametersDialog {
public:
	CommentStyleDialog() = default;
	void updateDlg();
protected:
	INT_PTR CALLBACK run_dlgProc(UINT Message, WPARAM wParam, LPARAM lParam);
	void setKeywords2List(int id);
private:
	void retrieve(TCHAR * dest, const TCHAR * toRetrieve, TCHAR * prefix) const;
};

class SymbolsStyleDialog : public SharedParametersDialog {
public:
	SymbolsStyleDialog() = default;
	void updateDlg();
protected:
	INT_PTR CALLBACK run_dlgProc(UINT Message, WPARAM wParam, LPARAM lParam);
	void setKeywords2List(int id);
private:
	void retrieve(TCHAR * dest, const TCHAR * toRetrieve, TCHAR * prefix) const;
};

class UserDefineDialog : public SharedParametersDialog {
	friend class ScintillaEditView;
public:
	UserDefineDialog();
	~UserDefineDialog();
	void init(HINSTANCE hInst, HWND hPere, ScintillaEditView * pSev);
	void setScintilla(ScintillaEditView * pScinView) 
	{
		_pScintilla = pScinView;
	}
	virtual void create(int dialogID, bool isRTL = false, bool msgDestParent = true) 
	{
		StaticDialog::create(dialogID, isRTL, msgDestParent);
	}
	void destroy() 
	{
		// A Ajouter les fils...
	}
	int getWidth() const { return _dlgPos.right; }
	int getHeight() const { return _dlgPos.bottom; }
	void doDialog(bool willBeShown = true, bool isRTL = false);
	virtual void reSizeTo(RECT & rc); // should NEVER be const !!!
	void reloadLangCombo();
	void changeStyle();
	bool isDocked() const { return _status == DOCK; }
	void setDockStatus(bool isDocked) { _status = isDocked; }
	HWND getFolderHandle() const { return _folderStyleDlg.getHSelf(); }
	HWND getKeywordsHandle() const { return _keyWordsStyleDlg.getHSelf(); }
	HWND getCommentHandle() const { return _commentStyleDlg.getHSelf(); }
	HWND getSymbolHandle() const { return _symbolsStyleDlg.getHSelf(); }
	void setTabName(int index, const TCHAR * name2set) { _ctrlTab.renameTab(index, name2set); }
protected:
	virtual INT_PTR CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
private:
	ControlsTab _ctrlTab;
	WindowVector _wVector;
	UserLangContainer * _pCurrentUserLang = nullptr;
	FolderStyleDialog _folderStyleDlg;
	KeyWordsStyleDialog _keyWordsStyleDlg;
	CommentStyleDialog _commentStyleDlg;
	SymbolsStyleDialog _symbolsStyleDlg;
	bool _status = UNDOCK;
	RECT _dlgPos = { 0 };
	int _currentHight = 0;
	int _yScrollPos = 0;
	int _prevHightVal = 0;
	void getActualPosSize();
	void restorePosSize() { reSizeTo(_dlgPos); }
	void enableLangAndControlsBy(size_t index);
protected:
	void setKeywords2List(int)
	{
	}
	void updateDlg();
};

class StringDlg : public StaticDialog {
public:
	StringDlg() = default;
	void init(HINSTANCE hInst, HWND parent, const TCHAR * title, const TCHAR * staticName, const TCHAR * text2Set,
	    int txtLen = 0, const TCHAR* restrictedChars = nullptr, bool bGotoCenter = false);
	INT_PTR doDialog();
	virtual void destroy() 
	{
	}
protected:
	INT_PTR CALLBACK run_dlgProc(UINT Message, WPARAM wParam, LPARAM);
	// Custom proc to subclass edit control
	LRESULT static CALLBACK customEditProc(HWND hEdit, UINT msg, WPARAM wParam, LPARAM lParam);
	bool isAllowed(const generic_string& txt);
	void HandlePaste(HWND hEdit);
private:
	generic_string _title;
	generic_string _textValue;
	generic_string _static;
	generic_string _restrictedChars;
	int _txtLen = 0;
	bool _shouldGotoCenter = false;
	WNDPROC _oldEditProc = nullptr;
};

class StylerDlg {
public:
	StylerDlg(HINSTANCE hInst, HWND parent, int stylerIndex = 0, int enabledNesters = -1);
	~StylerDlg();
	long doDialog();
	static INT_PTR CALLBACK dlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
private:
	HINSTANCE _hInst = nullptr;
	HWND _parent = nullptr;
	int _stylerIndex = 0;
	int _enabledNesters = 0;
	ColourPicker * _pFgColour = nullptr;
	ColourPicker * _pBgColour = nullptr;
	NppStyle _initialStyle;
	void move2CtrlRight(HWND hwndDlg, int ctrlID, HWND handle2Move, int handle2MoveWidth, int handle2MoveHeight);
};
//
#include "Docking.h"
//#include "ScintillaEditView.h"
#ifndef WM_MOUSEWHEEL
	#define WM_MOUSEWHEEL 0x020A
#endif //WM_MOUSEWHEEL
#ifndef WM_MOUSEHWHEEL
	#define WM_MOUSEHWHEEL 0x020E
#endif //WM_MOUSEHWHEEL
#ifndef WM_APPCOMMAND
	#define WM_APPCOMMAND                   0x0319
	#define APPCOMMAND_BROWSER_BACKWARD       1
	#define APPCOMMAND_BROWSER_FORWARD        2
	#define FAPPCOMMAND_MASK  0xF000
	#define GET_APPCOMMAND_LPARAM(lParam) ((short)(HIWORD(lParam) & ~FAPPCOMMAND_MASK))
#endif //WM_APPCOMMAND

class NppParameters;

#define NB_WORD_LIST 4
#define WORD_LIST_LEN 256

typedef sptr_t (* SCINTILLA_FUNC) (void *, uint, uptr_t, sptr_t);
typedef void * SCINTILLA_PTR;

#define WM_DOCK_USERDEFINE_DLG      (SCINTILLA_USER + 1)
#define WM_UNDOCK_USERDEFINE_DLG    (SCINTILLA_USER + 2)
#define WM_CLOSE_USERDEFINE_DLG     (SCINTILLA_USER + 3)
#define WM_REMOVE_USERLANG          (SCINTILLA_USER + 4)
#define WM_RENAME_USERLANG          (SCINTILLA_USER + 5)
#define WM_REPLACEALL_INOPENEDDOC   (SCINTILLA_USER + 6)
#define WM_FINDALL_INOPENEDDOC      (SCINTILLA_USER + 7)
#define WM_DOOPEN                   (SCINTILLA_USER + 8)
#define WM_FINDINFILES              (SCINTILLA_USER + 9)
#define WM_REPLACEINFILES           (SCINTILLA_USER + 10)
#define WM_FINDALL_INCURRENTDOC     (SCINTILLA_USER + 11)
#define WM_FRSAVE_INT               (SCINTILLA_USER + 12)
#define WM_FRSAVE_STR               (SCINTILLA_USER + 13)
#define WM_FINDALL_INCURRENTFINDER  (SCINTILLA_USER + 14)
#define WM_FINDINPROJECTS           (SCINTILLA_USER + 15)
#define WM_REPLACEINPROJECTS        (SCINTILLA_USER + 16)

const int NB_FOLDER_STATE = 7;

// Codepage
const int CP_CHINESE_TRADITIONAL = 950;
const int CP_CHINESE_SIMPLIFIED = 936;
const int CP_JAPANESE = 932;
const int CP_KOREAN = 949;
const int CP_GREEK = 1253;

//wordList
#define LIST_NONE 0
#define LIST_0 1
#define LIST_1 2
#define LIST_2 4
#define LIST_3 8
#define LIST_4 16
#define LIST_5 32
#define LIST_6 64
#define LIST_7 128
#define LIST_8 256

const bool fold_uncollapse = true;
const bool fold_collapse = false;
#define MAX_FOLD_COLLAPSE_LEVEL 8

enum TextCase : UCHAR {
	UPPERCASE,
	LOWERCASE,
	TITLECASE_FORCE,
	TITLECASE_BLEND,
	SENTENCECASE_FORCE,
	SENTENCECASE_BLEND,
	INVERTCASE,
	RANDOMCASE
};

const UCHAR MASK_FORMAT = 0x03;
const UCHAR MASK_ZERO_LEADING = 0x04;
const UCHAR BASE_10 = 0x00; // Dec
const UCHAR BASE_16 = 0x01; // Hex
const UCHAR BASE_08 = 0x02; // Oct
const UCHAR BASE_02 = 0x03; // Bin

const int MARK_BOOKMARK = 24;
const int MARK_HIDELINESBEGIN = 23;
const int MARK_HIDELINESEND = 22;
const int MARK_HIDELINESUNDERLINE = 21;
//const int MARK_LINEMODIFIEDUNSAVED = 20;
//const int MARK_LINEMODIFIEDSAVED = 19;
// 24 - 16 reserved for Notepad++ internal used
// 15 - 0  are free to use for plugins

int getNbDigits(int aNum, int base);
HMODULE loadSciLexerDll();

TCHAR * int2str(TCHAR * str, int strLen, int number, int base, int nbChiffre, bool isZeroLeading);

typedef LRESULT (WINAPI *CallWindowProcFunc)(WNDPROC, HWND, UINT, WPARAM, LPARAM);

const bool L2R = true;
const bool R2L = false;

struct ColumnModeInfo {
	int _selLpos = 0;
	int _selRpos = 0;
	int _order = -1; // 0 based index
	bool _direction = L2R; // L2R or R2L
	int _nbVirtualCaretSpc = 0;
	int _nbVirtualAnchorSpc = 0;

	ColumnModeInfo(int lPos, int rPos, int order, bool dir = L2R, int vAnchorNbSpc = 0, int vCaretNbSpc = 0)
		: _selLpos(lPos), _selRpos(rPos), _order(order), _direction(dir), _nbVirtualAnchorSpc(vAnchorNbSpc), _nbVirtualCaretSpc(
			vCaretNbSpc){
	};

	bool isValid() const {
		return (_order >= 0 && _selLpos >= 0 && _selRpos >= 0 && _selLpos <= _selRpos);
	};
};

//
// SortClass for vector<ColumnModeInfo>
// sort in _order : increased order
struct SortInSelectOrder {
	bool operator()(ColumnModeInfo & l, ColumnModeInfo & r) { return (l._order < r._order); }
};

//
// SortClass for vector<ColumnModeInfo>
// sort in _selLpos : increased order
struct SortInPositionOrder {
	bool operator()(ColumnModeInfo & l, ColumnModeInfo & r) { return (l._selLpos < r._selLpos); }
};

typedef std::vector<ColumnModeInfo> ColumnModeInfos;

struct LanguageName {
	const TCHAR * lexerName = nullptr;
	const TCHAR * shortName = nullptr;
	const TCHAR * longName = nullptr;
	LangType LangID = L_TEXT;
	int lexerID = 0;
};

#define URL_INDIC 8

class ScintillaEditView : public Window {
	friend class Finder;
public:
	ScintillaEditView();
	virtual ~ScintillaEditView();
	virtual void destroy();
	virtual void init(HINSTANCE hInst, HWND hPere);
	LRESULT execute(UINT Msg, WPARAM wParam = 0, LPARAM lParam = 0) const;
	void activateBuffer(BufferID buffer);
	void getCurrentFoldStates(std::vector<size_t> & lineStateVector);
	void syncFoldStateWith(const std::vector<size_t> & lineStateVectorNew);
	void getText(char * dest, size_t start, size_t end) const;
	void getGenericText(TCHAR * dest, size_t destlen, size_t start, size_t end) const;
	void getGenericText(TCHAR * dest, size_t deslen, int start, int end, int * mstart, int * mend) const;
	generic_string getGenericTextAsString(size_t start, size_t end) const;
	void insertGenericTextFrom(size_t position, const TCHAR * text2insert) const;
	void replaceSelWith(const char * replaceText);
	int getSelectedTextCount() 
	{
		Sci_CharacterRange range = getSelection();
		return (range.cpMax - range.cpMin);
	}
	void getVisibleStartAndEndPosition(int * startPos, int * endPos);
	char * getWordFromRange(char * txt, int size, int pos1, int pos2);
	char * getSelectedText(char * txt, int size, bool expand = true);
	char * getWordOnCaretPos(char * txt, int size);
	TCHAR * getGenericWordOnCaretPos(TCHAR * txt, int size);
	TCHAR * getGenericSelectedText(TCHAR * txt, int size, bool expand = true);
	int searchInTarget(const TCHAR * Text2Find, size_t lenOfText2Find, size_t fromPos, size_t toPos) const;
	void appandGenericText(const TCHAR * text2Append) const;
	void addGenericText(const TCHAR * text2Append) const;
	void addGenericText(const TCHAR * text2Append, long * mstart, long * mend) const;
	int replaceTarget(const TCHAR * str2replace, int fromTargetPos = -1, int toTargetPos = -1) const;
	int replaceTargetRegExMode(const TCHAR * re, int fromTargetPos = -1, int toTargetPos = -1) const;
	void showAutoComletion(size_t lenEntered, const TCHAR * list);
	void showCallTip(int startPos, const TCHAR * def);
	generic_string getLine(size_t lineNumber);
	void getLine(size_t lineNumber, TCHAR * line, int lineBufferLen);
	void addText(size_t length, const char * buf);
	void insertNewLineAboveCurrentLine();
	void insertNewLineBelowCurrentLine();
	void saveCurrentPos();
	void restoreCurrentPosPreStep();
	void restoreCurrentPosPostStep();
	void beginOrEndSelect();
	bool beginEndSelectedIsStarted() const { return _beginSelectPosition != -1; }
	int getCurrentDocLen() const { return int(execute(SCI_GETLENGTH)); }
	Sci_CharacterRange getSelection() const 
	{
		Sci_CharacterRange crange;
		crange.cpMin = long(execute(SCI_GETSELECTIONSTART));
		crange.cpMax = long(execute(SCI_GETSELECTIONEND));
		return crange;
	}
	void getWordToCurrentPos(TCHAR * str, int strLen) const 
	{
		auto caretPos = execute(SCI_GETCURRENTPOS);
		auto startPos = execute(SCI_WORDSTARTPOSITION, caretPos, true);
		str[0] = '\0';
		if((caretPos - startPos) < strLen)
			getGenericText(str, strLen, startPos, caretPos);
	}
	void doUserDefineDlg(bool willBeShown = true, bool isRTL = false) { _userDefineDlg.doDialog(willBeShown, isRTL); }
	static UserDefineDialog * getUserDefineDlg() { return &_userDefineDlg; }
	void setCaretColorWidth(int color, int width = 1) const 
	{
		execute(SCI_SETCARETFORE, color);
		execute(SCI_SETCARETWIDTH, width);
	}
	void beSwitched() { _userDefineDlg.setScintilla(this); }

	//Marge member and method
	static const int _SC_MARGE_LINENUMBER;
	static const int _SC_MARGE_SYBOLE;
	static const int _SC_MARGE_FOLDER;
	//static const int _SC_MARGE_MODIFMARKER;
	static LanguageName langNames[L_EXTERNAL+1];

	void showMargin(int whichMarge, bool willBeShowed = true);
	bool hasMarginShowed(int witchMarge) { return (execute(SCI_GETMARGINWIDTHN, witchMarge, 0) != 0); }
	void updateBeginEndSelectPosition(bool is_insert, size_t position, size_t length);
	void marginClick(Sci_Position position, int modifiers);
	void setMakerStyle(folderStyle style);
	void setWrapMode(lineWrapMethod meth);
	void showWSAndTab(bool willBeShowed = true);
	void showEOL(bool willBeShowed = true) { execute(SCI_SETVIEWEOL, willBeShowed); }
	bool isEolVisible() { return (execute(SCI_GETVIEWEOL) != 0); }
	void showInvisibleChars(bool willBeShowed = true) 
	{
		showWSAndTab(willBeShowed);
		showEOL(willBeShowed);
	}
	bool isInvisibleCharsShown() { return (execute(SCI_GETVIEWWS) != 0); }
	void showIndentGuideLine(bool willBeShowed = true);
	bool isShownIndentGuide() const { return (execute(SCI_GETINDENTATIONGUIDES) != 0); }
	void wrap(bool willBeWrapped = true) { execute(SCI_SETWRAPMODE, willBeWrapped); }
	bool isWrap() const { return (execute(SCI_GETWRAPMODE) == SC_WRAP_WORD); }

	bool isWrapSymbolVisible() const {
		return (execute(SCI_GETWRAPVISUALFLAGS) != SC_WRAPVISUALFLAG_NONE);
	};
	void showWrapSymbol(bool willBeShown = true) 
	{
		execute(SCI_SETWRAPVISUALFLAGSLOCATION, SC_WRAPVISUALFLAGLOC_DEFAULT);
		execute(SCI_SETWRAPVISUALFLAGS, willBeShown ? SC_WRAPVISUALFLAG_END : SC_WRAPVISUALFLAG_NONE);
	}
	size_t getCurrentLineNumber() const { return static_cast<size_t>(execute(SCI_LINEFROMPOSITION, execute(SCI_GETCURRENTPOS))); }
	int32_t lastZeroBasedLineNumber() const 
	{
		auto endPos = execute(SCI_GETLENGTH);
		return static_cast<int32_t>(execute(SCI_LINEFROMPOSITION, endPos));
	}
	long getCurrentXOffset() const { return long(execute(SCI_GETXOFFSET)); }
	void setCurrentXOffset(long xOffset) { execute(SCI_SETXOFFSET, xOffset); }
	void scroll(int column, int line) { execute(SCI_LINESCROLL, column, line); }
	long getCurrentPointX() const { return long (execute(SCI_POINTXFROMPOSITION, 0, execute(SCI_GETCURRENTPOS))); }
	long getCurrentPointY() const { return long (execute(SCI_POINTYFROMPOSITION, 0, execute(SCI_GETCURRENTPOS))); }
	long getTextHeight() const { return long(execute(SCI_TEXTHEIGHT)); }
	int getTextZoneWidth() const;
	void gotoLine(int line)
	{
		if(line < execute(SCI_GETLINECOUNT))
			execute(SCI_GOTOLINE, line);
	}
	long getCurrentColumnNumber() const { return long(execute(SCI_GETCOLUMN, execute(SCI_GETCURRENTPOS))); }
	std::pair<int, int> getSelectedCharsAndLinesCount(int maxSelectionsForLineCount = -1) const;
	int getUnicodeSelectedLength() const;
	long getLineLength(int line) const { return long(execute(SCI_GETLINEENDPOSITION, line) - execute(SCI_POSITIONFROMLINE, line)); }
	long getLineIndent(int line) const { return long(execute(SCI_GETLINEINDENTATION, line)); }
	void setLineIndent(int line, int indent) const;
	void updateLineNumbersMargin(bool forcedToHide);
	void updateLineNumberWidth();
	void setCurrentLineHiLiting(bool isHiliting, COLORREF bgColor) const 
	{
		execute(SCI_SETCARETLINEVISIBLE, isHiliting);
		if(!isHiliting)
			return;
		execute(SCI_SETCARETLINEBACK, bgColor);
	}
	bool isCurrentLineHiLiting() const { return (execute(SCI_GETCARETLINEVISIBLE) != 0); }
	void performGlobalStyles();
	void expand(size_t& line, bool doExpand, bool force = false, int visLevels = 0, int level = -1);
	std::pair<int, int> getSelectionLinesRange(int selectionNumber = -1) const;
	void currentLinesUp() const;
	void currentLinesDown() const;
	void changeCase(__inout wchar_t * const strWToConvert, const int & nbChars, const TextCase & caseToConvert) const;
	void convertSelectedTextTo(const TextCase & caseToConvert);
	void setMultiSelections(const ColumnModeInfos & cmi);
	void convertSelectedTextToLowerCase();
	void convertSelectedTextToUpperCase();
	void convertSelectedTextToNewerCase(const TextCase & caseToConvert);
	bool isFoldIndentationBased() const;
	void collapseFoldIndentationBased(int level2Collapse, bool mode);
	void collapse(int level2Collapse, bool mode);
	void foldAll(bool mode);
	void fold(size_t line, bool mode);
	bool isFolded(size_t line) { return (execute(SCI_GETFOLDEXPANDED, line) != 0); }
	void foldCurrentPos(bool mode);
	int getCodepage() const { return _codepage; }
	ColumnModeInfos getColumnModeSelectInfo();
	void columnReplace(ColumnModeInfos & cmi, const TCHAR * str);
	void columnReplace(ColumnModeInfos & cmi, int initial, int incr, int repeat, UCHAR format);
	void foldChanged(size_t line, int levelNow, int levelPrev);
	void FASTCALL clearIndicator(int indicatorNumber);
	bool getIndicatorRange(int indicatorNumber, int * from = NULL, int * to = NULL, int * cur = NULL);
	void bufferUpdated(Buffer * buffer, int mask);
	BufferID getCurrentBufferID() { return _currentBufferID; }
	Buffer * getCurrentBuffer() { return _currentBuffer; }
	void setCurrentBuffer(Buffer * buf2set) { _currentBuffer = buf2set; }
	void styleChange();
	void hideLines();
	bool markerMarginClick(int lineNumber); //true if it did something
	void notifyMarkers(Buffer * buf, bool isHide, int location, bool del);
	void runMarkers(bool doHide, size_t searchStart, bool endOfDoc, bool doDelete);
	bool isSelecting() const;
	bool isPythonStyleIndentation(LangType typeDoc) const 
	{
		return oneof13(typeDoc, L_PYTHON, L_COFFEESCRIPT, L_HASKELL, L_C, L_CPP, L_OBJC, L_CS, L_JAVA, L_PHP, L_JS, L_JAVASCRIPT, L_MAKEFILE, L_ASN1);
	}
	void defineDocType(LangType typeDoc);   //setup stylers for active document
	void addCustomWordChars();
	void restoreDefaultWordChars();
	void setWordChars();
	void mouseWheel(WPARAM wParam, LPARAM lParam) { scintillaNew_Proc(_hSelf, WM_MOUSEWHEEL, wParam, lParam); }
	void setHotspotStyle(NppStyle & styleToSet);
	void setTabSettings(Lang * lang);
	bool isWrapRestoreNeeded() const { return _wrapRestoreNeeded; }
	void setWrapRestoreNeeded(bool isWrapRestoredNeeded) { _wrapRestoreNeeded = isWrapRestoredNeeded; }
	bool isCJK() const { return oneof4(_codepage, CP_CHINESE_TRADITIONAL, CP_CHINESE_SIMPLIFIED, CP_JAPANESE, CP_KOREAN); }
	void scrollPosToCenter(size_t pos);
	generic_string getEOLString();
	void setBorderEdge(bool doWithBorderEdge);
	void sortLines(size_t fromLine, size_t toLine, ISorter * pSort);
	void changeTextDirection(bool isRTL);
	bool isTextDirectionRTL() const;
	void setPositionRestoreNeeded(bool val) { _positionRestoreNeeded = val; }
	void markedTextToClipboard(int indiStyle, bool doAll = false);
	void removeAnyDuplicateLines();
protected:
	static bool _SciInit;
	static int _refCount;
	static UserDefineDialog _userDefineDlg;
	static const int _markersArray[][NB_FOLDER_STATE];

	static LRESULT CALLBACK scintillaStatic_Proc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
	LRESULT scintillaNew_Proc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
	SCINTILLA_FUNC _pScintillaFunc = nullptr;
	SCINTILLA_PTR _pScintillaPtr = nullptr;
	static WNDPROC _scintillaDefaultProc;
	CallWindowProcFunc _callWindowProc = nullptr;
	BufferID attachDefaultDoc();

	//Store the current buffer so it can be retrieved later
	BufferID _currentBufferID = nullptr;
	Buffer * _currentBuffer = nullptr;
	int _codepage = CP_ACP;
	bool _wrapRestoreNeeded = false;
	bool _positionRestoreNeeded = false;
	uint32_t _restorePositionRetryCount = 0;
	typedef std::unordered_map<int, NppStyle> StyleMap;
	typedef std::unordered_map<BufferID, StyleMap*> BufferStyleMap;
	BufferStyleMap _hotspotStyles;
	long long _beginSelectPosition = -1;
	static std::string _defaultCharList;

//Lexers and Styling
	void restyleBuffer();
	const char * getCompleteKeywordList(std::basic_string<char> & kwl, LangType langType, int keywordIndex);
	void setKeywords(LangType langType, const char * keywords, int index);
	void setLexer(int lexerID, LangType langType, int whichList);
	void makeStyle(LangType langType, const TCHAR ** keywordArray = NULL);
	void setStyle(NppStyle styleToSet);                        //NOT by reference	(style edited)
	void setSpecialStyle(const NppStyle & styleToSet); //by reference
	void setSpecialIndicator(const NppStyle & styleToSet) { execute(SCI_INDICSETFORE, styleToSet._styleID, styleToSet._bgColor); }
	//Complex lexers (same lexer, different language)
	void setXmlLexer(LangType type);
	void setCppLexer(LangType type);
	void setJsLexer();
	void setTclLexer();
	void setObjCLexer(LangType type);
	void setUserLexer(const TCHAR * userLangName = NULL);
	void setExternalLexer(LangType typeDoc);
	void setEmbeddedJSLexer();
	void setEmbeddedPhpLexer();
	void setEmbeddedAspLexer();
	void setJsonLexer();
	void setTypeScriptLexer();
	//Simple lexers
	void setCssLexer() { setLexer(SCLEX_CSS, L_CSS, LIST_0 | LIST_1 | LIST_4 | LIST_6); }
	void setLuaLexer() { setLexer(SCLEX_LUA, L_LUA, LIST_0 | LIST_1 | LIST_2 | LIST_3); }
	void setMakefileLexer() 
	{
		execute(SCI_SETLEXER, SCLEX_MAKEFILE);
		makeStyle(L_MAKEFILE);
	}
	void setIniLexer() 
	{
		execute(SCI_SETLEXER, SCLEX_PROPERTIES);
		execute(SCI_STYLESETEOLFILLED, SCE_PROPS_SECTION, true);
		makeStyle(L_INI);
		execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("fold"), reinterpret_cast<LPARAM>("1"));
		execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("fold.compact"), reinterpret_cast<LPARAM>("0"));
	}
	void setSqlLexer() 
	{
		const bool kbBackSlash = NppParameters::getInstance().getNppGUI()._backSlashIsEscapeCharacterForSql;
		setLexer(SCLEX_SQL, L_SQL, LIST_0 | LIST_1 | LIST_4);
		execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("sql.backslash.escapes"), reinterpret_cast<LPARAM>(kbBackSlash ? "1" : "0"));
	}
	void setBashLexer() { setLexer(SCLEX_BASH, L_BASH, LIST_0); }
	void setVBLexer() { setLexer(SCLEX_VB, L_VB, LIST_0); }
	void setPascalLexer() 
	{
		setLexer(SCLEX_PASCAL, L_PASCAL, LIST_0);
		execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("fold.preprocessor"), reinterpret_cast<LPARAM>("1"));
	}
	void setPerlLexer() { setLexer(SCLEX_PERL, L_PERL, LIST_0); }
	void setPythonLexer() 
	{
		setLexer(SCLEX_PYTHON, L_PYTHON, LIST_0 | LIST_1);
		execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("fold.quotes.python"), reinterpret_cast<LPARAM>("1"));
	}
	void setBatchLexer() { setLexer(SCLEX_BATCH, L_BATCH, LIST_0); }
	void setTeXLexer() 
	{
		for(int i = 0; i < 4; ++i)
			execute(SCI_SETKEYWORDS, i, reinterpret_cast<LPARAM>(TEXT("")));
		setLexer(SCLEX_TEX, L_TEX, 0);
	}
	void setNsisLexer() { setLexer(SCLEX_NSIS, L_NSIS, LIST_0 | LIST_1 | LIST_2 | LIST_3); }
	void setFortranLexer() { setLexer(SCLEX_FORTRAN, L_FORTRAN, LIST_0 | LIST_1 | LIST_2); }
	void setFortran77Lexer() { setLexer(SCLEX_F77, L_FORTRAN_77, LIST_0 | LIST_1 | LIST_2); }
	void setLispLexer() { setLexer(SCLEX_LISP, L_LISP, LIST_0 | LIST_1); }
	void setSchemeLexer() { setLexer(SCLEX_LISP, L_SCHEME, LIST_0 | LIST_1); }
	void setAsmLexer() { setLexer(SCLEX_ASM, L_ASM, LIST_0 | LIST_1 | LIST_2 | LIST_3 | LIST_4 | LIST_5); }
	void setDiffLexer() { setLexer(SCLEX_DIFF, L_DIFF, LIST_NONE); }
	void setPropsLexer() { setLexer(SCLEX_PROPERTIES, L_PROPS, LIST_NONE); }
	void setPostscriptLexer() { setLexer(SCLEX_PS, L_PS, LIST_0 | LIST_1 | LIST_2 | LIST_3); }
	void setRubyLexer()
	{
		setLexer(SCLEX_RUBY, L_RUBY, LIST_0);
		execute(SCI_STYLESETEOLFILLED, SCE_RB_POD, true);
	}
	void setSmalltalkLexer() { setLexer(SCLEX_SMALLTALK, L_SMALLTALK, LIST_0); }
	void setVhdlLexer(){ setLexer(SCLEX_VHDL, L_VHDL, LIST_0 | LIST_1 | LIST_2 | LIST_3 | LIST_4 | LIST_5 | LIST_6); }
	void setKixLexer() { setLexer(SCLEX_KIX, L_KIX, LIST_0 | LIST_1 | LIST_2); }
	void setAutoItLexer()
	{
		setLexer(SCLEX_AU3, L_AU3, LIST_0 | LIST_1 | LIST_2 | LIST_3 | LIST_4 | LIST_5 | LIST_6);
		execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("fold.preprocessor"), reinterpret_cast<LPARAM>("1"));
	}
	void setCamlLexer() { setLexer(SCLEX_CAML, L_CAML, LIST_0 | LIST_1 | LIST_2); }
	void setAdaLexer() { setLexer(SCLEX_ADA, L_ADA, LIST_0); }
	void setVerilogLexer()
	{
		setLexer(SCLEX_VERILOG, L_VERILOG, LIST_0 | LIST_1);
		execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("fold.preprocessor"), reinterpret_cast<LPARAM>("1"));
	}
	void setMatlabLexer() { setLexer(SCLEX_MATLAB, L_MATLAB, LIST_0); }
	void setHaskellLexer() { setLexer(SCLEX_HASKELL, L_HASKELL, LIST_0); }
	void setInnoLexer() { setLexer(SCLEX_INNOSETUP, L_INNO, LIST_0 | LIST_1 | LIST_2 | LIST_3 | LIST_4 | LIST_5); }
	void setCmakeLexer() { setLexer(SCLEX_CMAKE, L_CMAKE, LIST_0 | LIST_1 | LIST_2); }
	void setYamlLexer() { setLexer(SCLEX_YAML, L_YAML, LIST_0); }
	//--------------------
	void setCobolLexer() { setLexer(SCLEX_COBOL, L_COBOL, LIST_0 | LIST_1 | LIST_2); }
	void setGui4CliLexer() { setLexer(SCLEX_GUI4CLI, L_GUI4CLI, LIST_0 | LIST_1 | LIST_2 | LIST_3 | LIST_4); }
	void setDLexer() { setLexer(SCLEX_D, L_D, LIST_0 | LIST_1 | LIST_2 | LIST_3 | LIST_4 | LIST_5 | LIST_6); }
	void setPowerShellLexer() { setLexer(SCLEX_POWERSHELL, L_POWERSHELL, LIST_0 | LIST_1 | LIST_2 | LIST_5); }
	void setRLexer() { setLexer(SCLEX_R, L_R, LIST_0 | LIST_1 | LIST_2); }
	void setCoffeeScriptLexer() { setLexer(SCLEX_COFFEESCRIPT, L_COFFEESCRIPT, LIST_0 | LIST_1 | LIST_2  | LIST_3); }
	void setBaanCLexer() 
	{
		setLexer(SCLEX_BAAN, L_BAANC, LIST_0 | LIST_1 | LIST_2 | LIST_3 | LIST_4 | LIST_5 | LIST_6 | LIST_7 | LIST_8);
		execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("lexer.baan.styling.within.preprocessor"), reinterpret_cast<LPARAM>("1"));
		execute(SCI_SETWORDCHARS, 0, reinterpret_cast<LPARAM>("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_$:"));
		execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("fold.preprocessor"), reinterpret_cast<LPARAM>("1"));
		execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("fold.baan.syntax.based"), reinterpret_cast<LPARAM>("1"));
		execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("fold.baan.keywords.based"), reinterpret_cast<LPARAM>("1"));
		execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("fold.baan.sections"), reinterpret_cast<LPARAM>("1"));
		execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("fold.baan.inner.level"), reinterpret_cast<LPARAM>("1"));
		execute(SCI_STYLESETEOLFILLED, SCE_BAAN_STRINGEOL, true);
	}
	void setSrecLexer() { setLexer(SCLEX_SREC, L_SREC, LIST_NONE); }
	void setIHexLexer() { setLexer(SCLEX_IHEX, L_IHEX, LIST_NONE); }
	void setTEHexLexer() { setLexer(SCLEX_TEHEX, L_TEHEX, LIST_NONE); }
	void setAsn1Lexer() { setLexer(SCLEX_ASN1, L_ASN1, LIST_0 | LIST_1 | LIST_2 | LIST_3); }
	void setAVSLexer() 
	{
		setLexer(SCLEX_AVS, L_AVS, LIST_0 | LIST_1 | LIST_2 | LIST_3 | LIST_4 | LIST_5);
		execute(SCI_SETWORDCHARS, 0, reinterpret_cast<LPARAM>("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_#"));
	}
	void setBlitzBasicLexer() { setLexer(SCLEX_BLITZBASIC, L_BLITZBASIC, LIST_0 | LIST_1 | LIST_2 | LIST_3); }
	void setPureBasicLexer() { setLexer(SCLEX_PUREBASIC, L_PUREBASIC, LIST_0 | LIST_1 | LIST_2 | LIST_3); }
	void setFreeBasicLexer() { setLexer(SCLEX_FREEBASIC, L_FREEBASIC, LIST_0 | LIST_1 | LIST_2 | LIST_3); }
	void setCsoundLexer() 
	{
		setLexer(SCLEX_CSOUND, L_CSOUND, LIST_0 | LIST_1 | LIST_2);
		execute(SCI_STYLESETEOLFILLED, SCE_CSOUND_STRINGEOL, true);
	}
	void setErlangLexer() { setLexer(SCLEX_ERLANG, L_ERLANG, LIST_0 | LIST_1 | LIST_2 | LIST_3 | LIST_4 | LIST_5); }
	void setESCRIPTLexer() { setLexer(SCLEX_ESCRIPT, L_ESCRIPT, LIST_0 | LIST_1 | LIST_2); }
	void setForthLexer() 
	{
		setLexer(SCLEX_FORTH, L_FORTH, LIST_0 | LIST_1 | LIST_2 | LIST_3 | LIST_4 | LIST_5);
		execute(SCI_SETWORDCHARS, 0, reinterpret_cast<LPARAM>("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789%-"));
	}
	void setLatexLexer() { setLexer(SCLEX_LATEX, L_LATEX, LIST_NONE); }
	void setMMIXALLexer() { setLexer(SCLEX_MMIXAL, L_MMIXAL, LIST_0 | LIST_1 | LIST_2); }
	void setNimrodLexer() { setLexer(SCLEX_NIMROD, L_NIM, LIST_0); }
	void setNncrontabLexer() 
	{
		setLexer(SCLEX_NNCRONTAB, L_NNCRONTAB, LIST_0 | LIST_1 | LIST_2);
		execute(SCI_SETWORDCHARS, 0, reinterpret_cast<LPARAM>("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789%-"));
	}
	void setOScriptLexer() 
	{
		setLexer(SCLEX_OSCRIPT, L_OSCRIPT, LIST_0 | LIST_1 | LIST_2 | LIST_3 | LIST_4 | LIST_5);
		execute(SCI_SETWORDCHARS, 0, reinterpret_cast<LPARAM>("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_$"));
	}
	void setREBOLLexer() 
	{
		setLexer(SCLEX_REBOL, L_REBOL, LIST_0 | LIST_1 | LIST_2 | LIST_3 | LIST_4 | LIST_5 | LIST_6);
		execute(SCI_SETWORDCHARS, 0, reinterpret_cast<LPARAM>("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789?!.’+-*&|=_~"));
	}
	void setRegistryLexer() { setLexer(SCLEX_REGISTRY, L_REGISTRY, LIST_NONE); }
	void setRustLexer() 
	{
		setLexer(SCLEX_RUST, L_RUST, LIST_0 | LIST_1 | LIST_2 | LIST_3 | LIST_4 | LIST_5 | LIST_6);
		execute(SCI_SETWORDCHARS, 0, reinterpret_cast<LPARAM>("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_#"));
	}
	void setSpiceLexer() { setLexer(SCLEX_SPICE, L_SPICE, LIST_0 | LIST_1 | LIST_2); }
	void setTxt2tagsLexer() { setLexer(SCLEX_TXT2TAGS, L_TXT2TAGS, LIST_NONE); }
	void setVisualPrologLexer() { setLexer(SCLEX_VISUALPROLOG, L_VISUALPROLOG, LIST_0 | LIST_1 | LIST_2 | LIST_3); }
	//--------------------
	void setSearchResultLexer() 
	{
		execute(SCI_STYLESETEOLFILLED, SCE_SEARCHRESULT_FILE_HEADER, true);
		execute(SCI_STYLESETEOLFILLED, SCE_SEARCHRESULT_SEARCH_HEADER, true);
		setLexer(SCLEX_SEARCHRESULT, L_SEARCHRESULT, 0);
	}
	bool isNeededFolderMarge(LangType typeDoc) const 
	{
		switch(typeDoc) {
			case L_ASCII:
			case L_BATCH:
			case L_TEXT:
			case L_MAKEFILE:
			case L_ASM:
			case L_HASKELL:
			case L_PROPS:
			case L_SMALLTALK:
			case L_KIX:
			case L_ADA:
			    return false;
			default:
			    return true;
		}
	}
//END: Lexers and Styling
	void defineMarker(int marker, int markerType, COLORREF fore, COLORREF back, COLORREF foreActive) 
	{
		execute(SCI_MARKERDEFINE, marker, markerType);
		execute(SCI_MARKERSETFORE, marker, fore);
		execute(SCI_MARKERSETBACK, marker, back);
		execute(SCI_MARKERSETBACKSELECTED, marker, foreActive);
	}
	int codepage2CharSet() const 
	{
		switch(_codepage) {
			case CP_CHINESE_TRADITIONAL: return SC_CHARSET_CHINESEBIG5;
			case CP_CHINESE_SIMPLIFIED: return SC_CHARSET_GB2312;
			case CP_KOREAN: return SC_CHARSET_HANGUL;
			case CP_JAPANESE: return SC_CHARSET_SHIFTJIS;
			case CP_GREEK: return SC_CHARSET_GREEK;
			default: return 0;
		}
	}
	std::pair<int, int> getWordRange();
	bool expandWordSelection();
	void getFoldColor(COLORREF& fgColor, COLORREF& bgColor, COLORREF& activeFgColor);
};
//
//#include "localization.h"
class FindReplaceDlg;
class PreferenceDlg;
class ShortcutMapper;
class UserDefineDialog;
class PluginsAdminDlg;

class MenuPosition {
public:
	int _x;
	int _y;
	int _z;
	char _id[64];
};

class NativeLangSpeaker {
public:
	NativeLangSpeaker() : _nativeLangA(NULL), _nativeLangEncoding(CP_ACP), _isRTL(false), _fileName(NULL)
	{
	}
	void init(TiXmlDocumentA * nativeLangDocRootA, bool loadIfEnglish = false);
	void changeConfigLang(HWND hDlg);
	void changeLangTabContextMenu(HMENU hCM);
	TiXmlNodeA * searchDlgNode(TiXmlNodeA * node, const char * dlgTagName);
	bool changeDlgLang(HWND hDlg, const char * dlgTagName, char * title = NULL, size_t titleMaxSize = 0);
	void changeLangTabDrapContextMenu(HMENU hCM);
	generic_string getSpecialMenuEntryName(const char * entryName) const;
	generic_string getNativeLangMenuString(int itemID) const;
	generic_string getShortcutNameString(int itemID) const;
	void changeMenuLang(HMENU menuHandle, generic_string & pluginsTrans, generic_string & windowTrans);
	void changeShortcutLang();
	void changeStyleCtrlsLang(HWND hDlg, int * idArray, const char ** translatedText);
	void changeUserDefineLang(UserDefineDialog * userDefineDlg);
	void changeUserDefineLangPopupDlg(HWND hDlg);
	void changeFindReplaceDlgLang(FindReplaceDlg & findReplaceDlg);
	void changePrefereceDlgLang(PreferenceDlg & preference);
	void changePluginsAdminDlgLang(PluginsAdminDlg & pluginsAdminDlg);
	bool getDoSaveOrNotStrings(generic_string& title, generic_string& msg);
	bool isRTL() const { return _isRTL; }
	const char * getFileName() const { return _fileName; }
	const TiXmlNodeA * getNativeLangA() { return _nativeLangA; }
	int getLangEncoding() const { return _nativeLangEncoding; }
	bool getMsgBoxLang(const char * msgBoxTagName, generic_string & title, generic_string & message);
	generic_string getShortcutMapperLangStr(const char * nodeName, const TCHAR * defaultStr) const;
	generic_string getProjectPanelLangMenuStr(const char * nodeName, int cmdID, const TCHAR * defaultStr) const;
	generic_string getFileBrowserLangMenuStr(int cmdID, const TCHAR * defaultStr) const;
	generic_string getAttrNameStr(const TCHAR * defaultStr, const char * nodeL1Name, const char * nodeL2Name, const char * nodeL3Name = "name") const;
	generic_string getLocalizedStrFromID(const char * strID, const generic_string& defaultString) const;
	int messageBox(const char * msgBoxTagName, HWND hWnd, const TCHAR * message, const TCHAR * title,
	    int msgBoxType, int intInfo = 0, const TCHAR * strInfo = NULL);
private:
	TiXmlNodeA * _nativeLangA;
	int _nativeLangEncoding;
	bool _isRTL;
	const char * _fileName;
};

MenuPosition & getMenuPosition(const char * id);
//
//#include "ColourPopup.h"
#define WM_PICKUP_COLOR  (COLOURPOPUP_USER + 1)
#define WM_PICKUP_CANCEL (COLOURPOPUP_USER + 2)

class ColourPopup : public Window {
public:
	ColourPopup() = default;
	explicit ColourPopup(COLORREF defaultColor) : _colour(defaultColor) 
	{
	}
	virtual ~ColourPopup() 
	{
	}
	bool isCreated() const { return (_hSelf != NULL); }
	void create(int dialogID);
	void doDialog(POINT p)
	{
		if(!isCreated())
			create(IDD_COLOUR_POPUP);
		::SetWindowPos(_hSelf, HWND_TOP, p.x, p.y, _rc.right - _rc.left, _rc.bottom - _rc.top, SWP_SHOWWINDOW);
	}
	virtual void destroy()
	{
		::DestroyWindow(_hSelf);
	}
	void setColour(COLORREF c)
	{
		_colour = c;
	}
	COLORREF getSelColour() const { return _colour; }
private:
	RECT _rc = {0};
	COLORREF _colour = RGB(0xFF, 0xFF, 0xFF);
	static INT_PTR CALLBACK dlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
	INT_PTR CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
};
//
//#include "columnEditor.h"
const bool activeText = true;
const bool activeNumeric = false;

class ColumnEditorDlg : public StaticDialog {
public:
	ColumnEditorDlg() = default;
	void init(HINSTANCE hInst, HWND hPere, ScintillaEditView ** ppEditView);
	virtual void create(int dialogID, bool isRTL = false, bool msgDestParent = true) 
	{
		StaticDialog::create(dialogID, isRTL, msgDestParent);
	}
	void doDialog(bool isRTL = false) 
	{
		if(!isCreated())
			create(IDD_COLUMNEDIT, isRTL);
		bool isTextMode = (BST_CHECKED == ::SendDlgItemMessage(_hSelf, IDC_COL_TEXT_RADIO, BM_GETCHECK, 0, 0));
		display();
		::SetFocus(::GetDlgItem(_hSelf, isTextMode ? IDC_COL_TEXT_EDIT : IDC_COL_INITNUM_EDIT));
	}
	virtual void display(bool toShow = true) const;
	void switchTo(bool toText);
	UCHAR getFormat();
protected:
	virtual INT_PTR CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
private:
	ScintillaEditView ** _ppEditView = nullptr;
};
//
//#include "ListView.h"
struct columnInfo {
	size_t _width;
	generic_string _label;
	columnInfo(const generic_string & label, size_t width) : _width(width), _label(label) 
	{
	}
};

class ListView : public Window {
public:
	ListView() = default;
	virtual ~ListView() = default;
	enum SortDirection {
		sortEncrease = 0,
		sortDecrease = 1
	};
	// addColumn() should be called before init()
	void addColumn(const columnInfo & column2Add) { _columnInfos.push_back(column2Add); }
	void setColumnText(size_t i, generic_string txt2Set); 
	// setStyleOption() should be called before init()
	void setStyleOption(int32_t extraStyle) { _extraStyle = extraStyle; }
	size_t findAlphabeticalOrderPos(const generic_string& string2search, SortDirection sortDir);
	void addLine(const std::vector<generic_string> & values2Add, LPARAM lParam = 0, int pos2insert = -1);
	size_t nbItem() const { return ListView_GetItemCount(_hSelf); }
	long getSelectedIndex() const { return ListView_GetSelectionMark(_hSelf); }
	void setSelection(int itemIndex) const;
	LPARAM getLParamFromIndex(int itemIndex) const;
	bool removeFromIndex(size_t i)  
	{
		return (i >= nbItem()) ? false : (ListView_DeleteItem(_hSelf, i) == TRUE);
	}
	std::vector<size_t> getCheckedIndexes() const;
	virtual void init(HINSTANCE hInst, HWND hwnd);
	virtual void destroy();
protected:
	WNDPROC _defaultProc = nullptr;
	int32_t _extraStyle = 0;
	std::vector<columnInfo> _columnInfos;
	LRESULT runProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK staticProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam) 
	{
		return (((ListView*)(::GetWindowLongPtr(hwnd, GWLP_USERDATA)))->runProc(hwnd, Message, wParam, lParam));
	}
};
//
//#include "DocTabView.h"
const int SAVED_IMG_INDEX = 0;
const int UNSAVED_IMG_INDEX = 1;
const int REDONLY_IMG_INDEX = 2;
const int MONITORING_IMG_INDEX = 3;

class DocTabView : public TabBarPlus {
public:
	DocTabView();
	virtual ~DocTabView();
	virtual void destroy();
	void init(HINSTANCE hInst, HWND parent, ScintillaEditView * pView, std::vector<IconList *> pIconListVector, uchar indexChoice);
	void changeIcons(uchar choice);
	void addBuffer(BufferID buffer);
	void closeBuffer(BufferID buffer);
	void bufferUpdated(Buffer * buffer, int mask);
	bool activateBuffer(BufferID buffer);
	BufferID activeBuffer();
	BufferID findBufferByName(const TCHAR * fullfilename);  //-1 if not found, something else otherwise
	int getIndexByBuffer(BufferID id);
	BufferID getBufferByIndex(size_t index);
	void setBuffer(size_t index, BufferID id);
	static bool setHideTabBarStatus(bool hideOrNot) 
	{
		bool temp = _hideTabBarStatus;
		_hideTabBarStatus = hideOrNot;
		return temp;
	}
	static bool getHideTabBarStatus() { return _hideTabBarStatus; }
	virtual void reSizeTo(RECT & rc);
	const ScintillaEditView* getScintillaEditView() const { return _pView; }
private:
	ScintillaEditView * _pView = nullptr;
	static bool _hideTabBarStatus;
	std::vector<IconList *> _pIconListVector;
	int _iconListIndexChoice = -1;
};
//
//#include "Splitter.h"
#define SV_HORIZONTAL           0x00000001
#define SV_VERTICAL                     0x00000002
#define SV_FIXED                        0x00000004
#define SV_ENABLERDBLCLK        0x00000008
#define SV_ENABLELDBLCLK        0x00000010
#define SV_RESIZEWTHPERCNT      0x00000020

#define WM_GETSPLITTER_X                (SPLITTER_USER + 1)
#define WM_GETSPLITTER_Y                (SPLITTER_USER + 2)
#define WM_DOPOPUPMENU                  (SPLITTER_USER + 3)
#define WM_RESIZE_CONTAINER             (SPLITTER_USER + 4)

const int HIEGHT_MINIMAL = 15;

enum class Arrow { left, up, right, down };
enum class WH { height, width };
enum class ZONE_TYPE { bottomRight, topLeft };
enum class SplitterMode : std::uint8_t{ DYNAMIC, LEFT_FIX, RIGHT_FIX };

class Splitter : public Window {
public:
	Splitter() = default;
	virtual ~Splitter() = default;
	virtual void destroy() override;
	void resizeSpliter(RECT * pRect = NULL);
	void init(HINSTANCE hInst, HWND hPere, int splitterSize, double iSplitRatio, DWORD dwFlags);
	void rotate();
	int getPhisicalSize() const { return _splitterSize; }
private:
	RECT _rect = {};
	double _splitPercent = 0.;
	int _splitterSize = 0;
	bool _isDraged = false;
	bool _isLeftButtonDown = false;
	DWORD _dwFlags = 0;
	bool _isFixed = false;
	static bool _isHorizontalRegistered;
	static bool _isVerticalRegistered;
	static bool _isHorizontalFixedRegistered;
	static bool _isVerticalFixedRegistered;
	RECT _clickZone2TL = { 0 };
	RECT _clickZone2BR = { 0 };
	static LRESULT CALLBACK staticWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT CALLBACK spliterWndProc(UINT uMsg, WPARAM wParam, LPARAM lParam);
	int  getClickZone(WH which);
	void adjustZoneToDraw(RECT & rc2def, ZONE_TYPE whichZone);
	void drawSplitter();
	bool isVertical() const { return (_dwFlags & SV_VERTICAL) != 0; }
	void paintArrow(HDC hdc, const RECT &rect, Arrow arrowDir);
	void gotoTopLeft();
	void gotoRightBottom();
	bool FASTCALL isInLeftTopZone(const POINT& p) const;
	bool FASTCALL isInRightBottomZone(const POINT & p) const;
	int  getSplitterFixPosX() const;
	int  getSplitterFixPosY() const;
};
//
//#include "SplitterContainer.h"
#define SPC_CLASS_NAME TEXT("splitterContainer")
#define ROTATION_LEFT 2000
#define ROTATION_RIGHT 2001

enum class DIRECTION {
	RIGHT,
	LEFT
};

class SplitterContainer : public Window {
public:
	virtual ~SplitterContainer() = default;
	void create(Window * pWin0, Window * pWin1, int splitterSize, SplitterMode mode = SplitterMode::DYNAMIC, int ratio = 50, bool _isVertical = true);
	void destroy();
	void reSizeTo(RECT & rc);
	virtual void display(bool toShow = true) const;
	virtual void redraw(bool forceUpdate = false) const;
	void setWin0(Window* pWin) { _pWin0 = pWin; }
	void setWin1(Window* pWin) { _pWin1 = pWin; }
	bool isVertical() const { return ((_dwSplitterStyle & SV_VERTICAL) != 0); }
private:
	Window* _pWin0 = nullptr; // left or top window
	Window* _pWin1 = nullptr; // right or bottom window
	Splitter _splitter;
	int _splitterSize = 0;
	int _ratio = 0;
	int _x = 0;
	int _y = 0;
	HMENU _hPopupMenu = NULL;
	DWORD _dwSplitterStyle = (SV_ENABLERDBLCLK | SV_ENABLELDBLCLK | SV_RESIZEWTHPERCNT);
	SplitterMode _splitterMode = SplitterMode::DYNAMIC;
	static bool _isRegistered;
	static LRESULT CALLBACK staticWinProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
	LRESULT runProc(UINT Message, WPARAM wParam, LPARAM lParam);
	void rotateTo(DIRECTION direction);
};
//
//#include "DockingDlgInterface.h"
class DockingDlgInterface : public StaticDialog { // Copyright (C) 2006 Jens Lorenz <jens.plugin.npp@gmx.de>
public:
	DockingDlgInterface() = default;
	explicit DockingDlgInterface(int dlgID) : _dlgID(dlgID) 
	{
	}
	virtual void init(HINSTANCE hInst, HWND parent);
	void create(tTbData* data, bool isRTL = false);
	virtual void updateDockingDlg() { ::SendMessage(_hParent, NPPM_DMMUPDATEDISPINFO, 0, reinterpret_cast<LPARAM>(_hSelf)); }
	virtual void destroy() 
	{
	}
	virtual void setBackgroundColor(COLORREF) 
	{
	}
	virtual void setForegroundColor(COLORREF) 
	{
	}
	virtual void display(bool toShow = true) const { ::SendMessage(_hParent, toShow ? NPPM_DMMSHOW : NPPM_DMMHIDE, 0, reinterpret_cast<LPARAM>(_hSelf)); }
	bool isClosed() const { return _isClosed; }
	void setClosed(bool toClose) { _isClosed = toClose; }
	const TCHAR * getPluginFileName() const { return _moduleName.c_str(); }
protected:
	virtual INT_PTR CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);

	int _dlgID = -1;
	bool _isFloating = true;
	int _iDockedPos = 0;
	generic_string _moduleName;
	generic_string _pluginName;
	bool _isClosed = false;
};
//
//#include "StatusBar.h"
#ifndef _WIN32_IE
	#define _WIN32_IE       0x0600
#endif //_WIN32_IE

class StatusBar final : public Window {
public:
	virtual ~StatusBar();
	void init(HINSTANCE hInst, HWND hPere, int nbParts);
	bool setPartWidth(int whichPart, int width);
	virtual void destroy() override;
	virtual void reSizeTo(const RECT& rc);
	int getHeight() const;
	bool setText(const TCHAR* str, int whichPart);
	bool setOwnerDrawText(const TCHAR* str);
	void adjustParts(int clientWidth);
private:
	virtual void init(HINSTANCE hInst, HWND hPere) override;
private:
	std::vector<int> _partWidthArray;
	int * _lpParts = nullptr;
	generic_string _lastSetText;
	StatusBarSubclassInfo* _pStatusBarInfo = nullptr;
};
//
//#include "FindReplaceDlg.h"
#define FIND_RECURSIVE 1
#define FIND_INHIDDENDIR 2

#define FINDREPLACE_MAXLENGTH 2048

enum DIALOG_TYPE {FIND_DLG, REPLACE_DLG, FINDINFILES_DLG, FINDINPROJECTS_DLG, MARK_DLG};

#define DIR_DOWN true
#define DIR_UP false

//#define FIND_REPLACE_STR_MAX 256

enum InWhat {ALL_OPEN_DOCS, FILES_IN_DIR, CURRENT_DOC, CURR_DOC_SELECTION, FILES_IN_PROJECTS};

struct FoundInfo {
	FoundInfo(int start, int end, size_t lineNumber, const TCHAR * fullPath) : _start(start), _end(end), _lineNumber(lineNumber), _fullPath(fullPath) 
	{
	}
	int _start;
	int _end;
	size_t _lineNumber;
	generic_string _fullPath;
};

struct TargetRange {
	int targetStart;
	int targetEnd;
};

enum SearchIncrementalType { NotIncremental, FirstIncremental, NextIncremental };
enum SearchType { FindNormal, FindExtended, FindRegex };
enum ProcessOperation { ProcessFindAll, ProcessReplaceAll, ProcessCountAll, ProcessMarkAll, ProcessMarkAll_2, ProcessMarkAll_IncSearch,
			ProcessMarkAllExt, ProcessFindInFinder };

struct FindOption {
	bool _isWholeWord = true;
	bool _isMatchCase = true;
	bool _isWrapAround = true;
	bool _whichDirection = DIR_DOWN;
	SearchIncrementalType _incrementalType = NotIncremental;
	SearchType _searchType = FindNormal;
	bool _doPurge = false;
	bool _doMarkLine = false;
	bool _isInSelection = false;
	generic_string _str2Search;
	generic_string _str4Replace;
	generic_string _filters;
	generic_string _directory;
	bool _isRecursive = true;
	bool _isInHiddenDir = false;
	bool _isProjectPanel_1 = false;
	bool _isProjectPanel_2 = false;
	bool _isProjectPanel_3 = false;
	bool _dotMatchesNewline = false;
	bool _isMatchLineNumber = true; // only for Find in Folder
};

//This class contains generic search functions as static functions for easy access
class Searching {
public:
	static int convertExtendedToString(const TCHAR * query, TCHAR * result, int length);
	static TargetRange t;
	static int FASTCALL buildSearchFlags(const FindOption * option);
	static void displaySectionCentered(int posStart, int posEnd, ScintillaEditView * pEditView, bool isDownwards = true);
private:
	static bool readBase(const TCHAR * str, int * value, int base, int size);
};

//Finder: Dockable window that contains search results
class Finder : public DockingDlgInterface {
	friend class FindReplaceDlg;
public:
	Finder() : DockingDlgInterface(IDD_FINDRESULT) 
	{
		_markingsStruct._length = 0;
		_markingsStruct._markings = NULL;
	}
	~Finder() 
	{
		_scintView.destroy();
	}
	void init(HINSTANCE hInst, HWND hPere, ScintillaEditView ** ppEditView) 
	{
		DockingDlgInterface::init(hInst, hPere);
		_ppEditView = ppEditView;
	}
	void addSearchLine(const TCHAR * searchName);
	void addFileNameTitle(const TCHAR * fileName);
	void addFileHitCount(int count);
	void addSearchHitCount(int count, int countSearched, bool isMatchLines, bool searchedEntireNotSelection);
	void add(FoundInfo fi, SearchResultMarking mi, const TCHAR* foundline);
	void setFinderStyle();
	void removeAll();
	void openAll();
	void wrapLongLinesToggle();
	void purgeToggle();
	void copy();
	void copyPathnames();
	void beginNewFilesSearch();
	void finishFilesSearch(int count, int searchedCount, bool isMatchLines, bool searchedEntireNotSelection);
	void gotoNextFoundResult(int direction);
	void gotoFoundLine();
	void deleteResult();
	std::vector<generic_string> getResultFilePaths() const;
	bool canFind(const TCHAR * fileName, size_t lineNumber) const;
	void setVolatiled(bool val) { _canBeVolatiled = val; }
	generic_string getHitsString(int count) const;
protected:
	virtual INT_PTR CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
	bool notify(SCNotification * notification);
private:
	enum { searchHeaderLevel = SC_FOLDLEVELBASE, fileHeaderLevel, resultLevel };

	ScintillaEditView ** _ppEditView = nullptr;
	std::vector<FoundInfo> _foundInfos1;
	std::vector<FoundInfo> _foundInfos2;
	std::vector<FoundInfo>* _pMainFoundInfos = &_foundInfos1;
	std::vector<SearchResultMarking> _markings1;
	std::vector<SearchResultMarking> _markings2;
	std::vector<SearchResultMarking>* _pMainMarkings = &_markings1;
	SearchResultMarkings _markingsStruct;
	ScintillaEditView _scintView;
	uint _nbFoundFiles = 0;
	int _lastFileHeaderPos = 0;
	int _lastSearchHeaderPos = 0;
	bool _canBeVolatiled = true;
	bool _longLinesAreWrapped = false;
	bool _purgeBeforeEverySearch = false;
	generic_string _prefixLineStr;
	void setFinderReadOnly(bool isReadOnly) { _scintView.execute(SCI_SETREADONLY, isReadOnly); }
	bool isLineActualSearchResult(const generic_string & s) const;
	generic_string & prepareStringForClipboard(generic_string & s) const;

	static FoundInfo EmptyFoundInfo;
	static SearchResultMarking EmptySearchResultMarking;
};

enum FindStatus { FSFound, FSNotFound, FSTopReached, FSEndReached, FSMessage, FSNoMessage};

enum FindNextType {
	FINDNEXTTYPE_FINDNEXT,
	FINDNEXTTYPE_REPLACENEXT,
	FINDNEXTTYPE_FINDNEXTFORREPLACE
};

struct FindReplaceInfo {
	const TCHAR * _txt2find = nullptr;
	const TCHAR * _txt2replace = nullptr;
	int _startRange = -1;
	int _endRange = -1;
};

struct FindersInfo {
	Finder * _pSourceFinder = nullptr;
	Finder * _pDestFinder = nullptr;
	const TCHAR * _pFileName = nullptr;
	FindOption _findOption;
};

class FindInFinderDlg : public StaticDialog {
public:
	void init(HINSTANCE hInst, HWND hPere) { Window::init(hInst, hPere); }
	void doDialog(Finder * launcher, bool isRTL = false);
	FindOption & getOption() { return _options; }

private:
	Finder  * _pFinder2Search = nullptr;
	FindOption _options;
	virtual INT_PTR CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
	void initFromOptions();
	void writeOptions();
};

class FindReplaceDlg : public StaticDialog {
	friend class FindIncrementDlg;
public:
	static FindOption _options;
	static FindOption* _env;
	FindReplaceDlg() 
	{
		_uniFileName = new char[(_fileNameLenMax + 3) * 2];
		_winVer = (NppParameters::getInstance()).getWinVersion();
		_env = &_options;
	}
	~FindReplaceDlg();
	void init(HINSTANCE hInst, HWND hPere, ScintillaEditView ** ppEditView) 
	{
		Window::init(hInst, hPere);
		if(!ppEditView)
			throw std::runtime_error("FindIncrementDlg::init : ppEditView is null.");
		_ppEditView = ppEditView;
	}
	virtual void create(int dialogID, bool isRTL = false, bool msgDestParent = true);
	void initOptionsFromDlg();
	void doDialog(DIALOG_TYPE whichType, bool isRTL = false, bool toShow = true);
	bool processFindNext(const TCHAR * txt2find, const FindOption * options = NULL, FindStatus * oFindStatus = NULL, FindNextType findNextType = FINDNEXTTYPE_FINDNEXT);
	bool processReplace(const TCHAR * txt2find, const TCHAR * txt2replace, const FindOption * options = NULL);
	int markAll(const TCHAR * txt2find, int styleID);
	int markAllInc(const FindOption * opt);
	int processAll(ProcessOperation op, const FindOption * opt, bool isEntire = false, const FindersInfo * pFindersInfo = nullptr, int colourStyleID = -1);
	int processRange(ProcessOperation op, FindReplaceInfo & findReplaceInfo, const FindersInfo * pFindersInfo, const FindOption * opt = nullptr,
	    int colourStyleID = -1, ScintillaEditView * view2Process = nullptr);
	void replaceAllInOpenedDocs();
	void findAllIn(InWhat op);
	void setSearchText(TCHAR * txt2find);
	void gotoNextFoundResult(int direction = 0) 
	{
		CALLPTRMEMB(_pFinder, gotoNextFoundResult(direction));
	}
	void putFindResult(int result) { _findAllResult = result; }
	const TCHAR * getDir2Search() const { return _env->_directory.c_str(); }
	void getPatterns(std::vector<generic_string> & patternVect);
	void getAndValidatePatterns(std::vector<generic_string> & patternVect);
	void launchFindInFilesDlg() { doDialog(FINDINFILES_DLG); }
	void launchFindInProjectsDlg() { doDialog(FINDINPROJECTS_DLG); }
	void setFindInFilesDirFilter(const TCHAR * dir, const TCHAR * filters);
	void setProjectCheckmarks(FindHistory * findHistory, int Msk);
	void enableProjectCheckmarks();
	generic_string getText2search() const { return _env->_str2Search; }
	const generic_string & getFilters() const { return _env->_filters; }
	const generic_string & getDirectory() const { return _env->_directory; }
	const FindOption & getCurrentOptions() const { return *_env; }
	bool isRecursive() const { return _env->_isRecursive; }
	bool isInHiddenDir() const { return _env->_isInHiddenDir; }
	bool isProjectPanel_1() const { return _env->_isProjectPanel_1; }
	bool isProjectPanel_2() const { return _env->_isProjectPanel_2; }
	bool isProjectPanel_3() const { return _env->_isProjectPanel_3; }
	void saveFindHistory();
	void changeTabName(DIALOG_TYPE index, const TCHAR * name2change);
	void beginNewFilesSearch();
	void finishFilesSearch(int count, int searchedCount, bool searchedEntireNotSelection);
	void focusOnFinder();
	HWND getHFindResults();
	void updateFinderScintilla();
	void execSavedCommand(int cmd, uptr_t intValue, const generic_string& stringValue);
	void clearMarks(const FindOption& opt);
	void setStatusbarMessage(const generic_string & msg, FindStatus staus, char const * pTooltipMsg = NULL);
	generic_string getScopeInfoForStatusBar(FindOption const * pFindOpt) const;
	Finder * createFinder();
	bool removeFinder(Finder * finder2remove);
	DIALOG_TYPE getCurrentStatus() const { return _currentStatus; }
protected:
	void resizeDialogElements(LONG newWidth);
	virtual INT_PTR CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
	static LONG_PTR originalFinderProc;
	static LONG_PTR originalComboEditProc;
	static LRESULT FAR PASCAL comboEditProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
	// Window procedure for the finder
	static LRESULT FAR PASCAL finderProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
	void combo2ExtendedMode(int comboID);
private:
	RECT _initialWindowRect = {0};
	LONG _deltaWidth = 0;
	LONG _initialClientWidth = 0;
	DIALOG_TYPE _currentStatus = FIND_DLG;
	RECT _findClosePos, _replaceClosePos, _findInFilesClosePos, _markClosePos;
	RECT _countInSelFramePos, _replaceInSelFramePos;
	RECT _countInSelCheckPos, _replaceInSelCheckPos;
	ScintillaEditView ** _ppEditView = nullptr;
	Finder  * _pFinder = nullptr;
	generic_string _findResTitle;
	std::vector<Finder *> _findersOfFinder;
	HWND _shiftTrickUpTip = nullptr;
	HWND _2ButtonsTip = nullptr;
	HWND _filterTip = nullptr;
	bool _isRTL = false;
	int _findAllResult;
	TCHAR _findAllResultStr[1024] = {'\0'};
	int _fileNameLenMax = 1024;
	char * _uniFileName = nullptr;
	TabBar _tab;
	winVer _winVer = winVer::WV_UNKNOWN;
	StatusBar _statusBar;
	FindStatus _statusbarFindStatus;
	generic_string _statusbarTooltipMsg;
	HWND _statusbarTooltipWnd = nullptr;
	HICON _statusbarTooltipIcon = nullptr;
	int _statusbarTooltipIconSize = 0;
	HFONT _hMonospaceFont = nullptr;
	std::map<int, bool> _controlEnableMap;

	void enableFindDlgItem(int dlgItemID, bool isEnable = true);
	void showFindDlgItem(int dlgItemID, bool isShow = true);
	void enableReplaceFunc(bool isEnable);
	void enableFindInFilesControls(bool isEnable, bool projectPanels);
	void enableFindInFilesFunc();
	void enableFindInProjectsFunc();
	void enableMarkAllControls(bool isEnable);
	void enableMarkFunc();
	void setDefaultButton(int nID) 
	{
		SendMessage(_hSelf, DM_SETDEFID, nID, 0L);
	}
	void gotoCorrectTab() 
	{
		auto currentIndex = _tab.getCurrentTabIndex();
		if(currentIndex != _currentStatus)
			_tab.activateAt(_currentStatus);
	}
	FindStatus getFindStatus() { return this->_statusbarFindStatus; }
	void updateCombos();
	void updateCombo(int comboID);
	void fillFindHistory();
	void fillComboHistory(int id, const std::vector<generic_string> & strings);
	int saveComboHistory(int id, int maxcount, std::vector<generic_string> & strings, bool saveEmpty);
	static const int FR_OP_FIND = 1;
	static const int FR_OP_REPLACE = 2;
	static const int FR_OP_FIF = 4;
	static const int FR_OP_GLOBAL = 8;
	static const int FR_OP_FIP = 16;
	void saveInMacro(size_t cmd, int cmdType);
	void drawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);
	bool replaceInFilesConfirmCheck(generic_string directory, generic_string fileTypes);
	bool replaceInProjectsConfirmCheck();
	bool replaceInOpenDocsConfirmCheck(void);
};

//FindIncrementDlg: incremental search dialog, docked in rebar
class FindIncrementDlg : public StaticDialog {
public:
	FindIncrementDlg() = default;
	void init(HINSTANCE hInst, HWND hPere, FindReplaceDlg * pFRDlg, bool isRTL = false);
	virtual void destroy();
	virtual void display(bool toShow = true) const;
	void setSearchText(const TCHAR* txt2find, bool) 
	{
		::SendDlgItemMessage(_hSelf, IDC_INCFINDTEXT, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(txt2find));
	}
	void setFindStatus(FindStatus iStatus, int nbCounted);
	FindStatus getFindStatus() const { return _findStatus; }
	void addToRebar(ReBar* rebar);
private:
	bool _isRTL = false;
	FindReplaceDlg * _pFRDlg = nullptr;
	FindStatus _findStatus = FSFound;
	ReBar* _pRebar = nullptr;
	REBARBANDINFO _rbBand;
	virtual INT_PTR CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
	void markSelectedTextInc(bool enable, FindOption * opt = NULL);
};

class Progress {
public:
	explicit Progress(HINSTANCE hInst);
	~Progress();
	// Disable copy construction and operator=
	Progress(const Progress&) = delete;
	const Progress& operator = (const Progress&) = delete;
	HWND open(HWND hCallerWnd, const TCHAR* header = NULL);
	void close();
	bool isCancelled() const;
	void setInfo(const TCHAR * info) const;
	void setPercent(unsigned percent, const TCHAR * fileName) const;
private:
	static DWORD WINAPI threadFunc(LPVOID data);
	static LRESULT APIENTRY wndProc(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam);
	int thread();
	int createProgressWindow();

	static const TCHAR cClassName[];
	static const TCHAR cDefaultHeader[];
	static const int cBackgroundColor;
	static const int cPBwidth;
	static const int cPBheight;
	static const int cBTNwidth;
	static const int cBTNheight;
	static volatile LONG refCount;

	HINSTANCE _hInst = nullptr;
	volatile HWND _hwnd = nullptr;
	HWND _hCallerWnd = nullptr;
	TCHAR _header[128] = {'\0'};
	HANDLE _hThread = nullptr;
	HANDLE _hActiveState = nullptr;
	HWND _hPText = nullptr;
	HWND _hPBar = nullptr;
	HWND _hBtn = nullptr;
};
//
//#include "AboutDlg.h"
class AboutDlg : public StaticDialog {
public:
	AboutDlg() = default;
	void doDialog();
	virtual void destroy();
protected:
	virtual INT_PTR CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
private:
	URLCtrl _emailLink;
	URLCtrl _pageLink;
};

class DebugInfoDlg : public StaticDialog {
public:
	DebugInfoDlg() = default;
	void init(HINSTANCE hInst, HWND parent, bool isAdmin, const generic_string& loadedPlugins);
	void doDialog();
	virtual void destroy() 
	{
		_copyToClipboardLink.destroy();
	}
protected:
	virtual INT_PTR CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
private:
	typedef const CHAR * (__cdecl * PWINEGETVERSION)();
	generic_string _debugInfoStr;
	bool _isAdmin = false;
	generic_string _loadedPlugins;
	URLCtrl _copyToClipboardLink;
};

class DoSaveOrNotBox : public StaticDialog {
public:
	DoSaveOrNotBox() = default;
	void init(HINSTANCE hInst, HWND parent, const TCHAR* fn, bool isMulti);
	void doDialog(bool isRTL = false);
	virtual void destroy() 
	{
	}
	int getClickedButtonId() const { return clickedButtonId; }
	void changeLang();
protected:
	virtual INT_PTR CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
private:
	int clickedButtonId = -1;
	generic_string _fn;
	bool _isMulti = false;
};

class DoSaveAllBox : public StaticDialog {
public:
	DoSaveAllBox() = default;
	void doDialog(bool isRTL = false);
	virtual void destroy() 
	{
	}
	int getClickedButtonId() const { return clickedButtonId; }
	void changeLang();
protected:
	virtual INT_PTR CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
private:
	int clickedButtonId = -1;
};
//
//#include "RunDlg.h"
#define CURRENTWORD_MAXLENGTH 2048

const TCHAR fullCurrentPath[] = TEXT("FULL_CURRENT_PATH");
const TCHAR currentDirectory[] = TEXT("CURRENT_DIRECTORY");
const TCHAR onlyFileName[] = TEXT("FILE_NAME");
const TCHAR fileNamePart[] = TEXT("NAME_PART");
const TCHAR fileExtPart[] = TEXT("EXT_PART");
const TCHAR currentWord[] = TEXT("CURRENT_WORD");
const TCHAR nppDir[] = TEXT("NPP_DIRECTORY");
const TCHAR nppFullFilePath[] = TEXT("NPP_FULL_FILE_PATH");
const TCHAR currentLine[] = TEXT("CURRENT_LINE");
const TCHAR currentColumn[] = TEXT("CURRENT_COLUMN");

int whichVar(TCHAR *str);
void expandNppEnvironmentStrs(const TCHAR *strSrc, TCHAR *stringDest, size_t strDestLen, HWND hWnd);

class Command {
public :
	Command() = default;
	explicit Command(const TCHAR *cmd) : _cmdLine(cmd){};
	explicit Command(const generic_string& cmd) : _cmdLine(cmd){};
	HINSTANCE run(HWND hWnd);
	HINSTANCE run(HWND hWnd, const TCHAR* cwd);
protected:
	generic_string _cmdLine;
private:
	void extractArgs(TCHAR *cmd2Exec, size_t cmd2ExecLen, TCHAR *args, size_t argsLen, const TCHAR *cmdEntier);
};

class RunDlg : public Command, public StaticDialog {
public :
	RunDlg() = default;
	void doDialog(bool isRTL = false);
    virtual void destroy() {};
protected :
	virtual INT_PTR CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
private:
	void addTextToCombo(const TCHAR *txt2Add) const;
	void removeTextFromCombo(const TCHAR *txt2Remove) const;
};
//
//#include "lastRecentFileList.h"
struct RecentItem {
	int _id = 0;
	generic_string _name;
	explicit RecentItem(const TCHAR * name) : _name(name) 
	{
	}
};

typedef std::deque<RecentItem> recentList;

class LastRecentFileList {
public:
	LastRecentFileList() 
	{
		_userMax = (NppParameters::getInstance()).getNbMaxRecentFile();
	}
	void initMenu(HMENU hMenu, int idBase, int posBase, Accelerator * accelerator, bool doSubMenu = false);
	void switchMode();
	void updateMenu();
	void add(const TCHAR * fn);
	void remove(const TCHAR * fn);
	void remove(size_t index);
	void clear();
	int getSize() const { return _size; }
	int getMaxNbLRF() const { return NB_MAX_LRF_FILE; }
	int getUserMaxNbLRF() const { return _userMax; }
	generic_string & getItem(int id);       //use menu id
	generic_string & getIndex(int index);   //use menu id
	generic_string getFirstItem() const { return (_lrfl.size() == 0) ? TEXT("") : _lrfl.front()._name; }
	void setUserMaxNbLRF(int size);
	void saveLRFL();
	void setLock(bool lock) { _locked = lock; }
	void setLangEncoding(int nativeLangEncoding) { _nativeLangEncoding = nativeLangEncoding; }
	bool isSubMenuMode() const { return (_hParentMenu != NULL); }
private:
	recentList _lrfl;
	Accelerator * _pAccelerator = nullptr;
	int _userMax = 0;
	int _size = 0;
	int _nativeLangEncoding = -1;

	// For the menu
	HMENU _hParentMenu = nullptr;
	HMENU _hMenu = nullptr;
	int _posBase = -1;
	int _idBase = -1;
	bool _idFreeArray[NB_MAX_LRF_FILE] = {false};
	bool _hasSeparators = false;
	bool _locked = false;

	int find(const TCHAR * fn);
	int popFirstAvailableID();
	void setAvailable(int id);
};
//
//#include "GoToLineDlg.h"
class GoToLineDlg : public StaticDialog {
public:
	GoToLineDlg() = default;
	void init(HINSTANCE hInst, HWND hPere, ScintillaEditView ** ppEditView);
	virtual void create(int dialogID, bool isRTL = false, bool msgDestParent = true);
	void doDialog(bool isRTL = false);
	virtual void display(bool toShow = true) const;
protected:
	enum mode {
		go2line, 
		go2offsset
	};
	mode _mode = go2line;
	virtual INT_PTR CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
private:
	ScintillaEditView ** _ppEditView = nullptr;
	void updateLinesNumbers() const;
	void cleanLineEdit() const { ::SetDlgItemText(_hSelf, ID_GOLINE_EDIT, TEXT("")); }
	int getLine() const;
};
//
//#include "FindCharsInRange.h"
class FindCharsInRangeDlg : public StaticDialog {
public:
	FindCharsInRangeDlg() = default;
	void init(HINSTANCE hInst, HWND hPere, ScintillaEditView ** ppEditView) 
	{
		Window::init(hInst, hPere);
		if(!ppEditView)
			throw std::runtime_error("FindCharsInRangeDlg::init : ppEditView is null.");
		_ppEditView = ppEditView;
	}
	virtual void create(int dialogID, bool isRTL = false, bool msgDestParent = true) { StaticDialog::create(dialogID, isRTL, msgDestParent); }
	void doDialog(bool isRTL = false) 
	{
		if(!isCreated())
			create(IDD_FINDCHARACTERS, isRTL);
		display();
	}
	virtual void display(bool toShow = true) const { Window::display(toShow); }

protected:
	virtual INT_PTR CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
private:
	ScintillaEditView ** _ppEditView = nullptr;
	bool findCharInRange(uchar beginRange, uchar endRange, int startPos, bool direction, bool wrap);
	bool getRangeFromUI(uchar & startRange, uchar & endRange);
	void getDirectionFromUI(bool & whichDirection, bool & isWrap);
};
//
//#include "WordStyleDlg.h"
#define WM_UPDATESCINTILLAS      (WORDSTYLE_USER + 1) //GlobalStyleDlg's msg 2 send 2 its parent
#define WM_UPDATEMAINMENUBITMAPS (WORDSTYLE_USER + 2)

enum fontStyleType {
	BOLD_STATUS, 
	ITALIC_STATUS, 
	UNDERLINE_STATUS
};

const bool C_FOREGROUND = false;
const bool C_BACKGROUND = true;

class ColourStaticTextHooker {
public:
	ColourStaticTextHooker() : _colour(RGB(0x00, 0x00, 0x00)) 
	{
	}
	COLORREF setColour(COLORREF colour2Set) 
	{
		COLORREF oldColour = _colour;
		_colour = colour2Set;
		return oldColour;
	}
	void hookOn(HWND staticHandle) 
	{
		::SetWindowLongPtr(staticHandle, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
		_oldProc = reinterpret_cast<WNDPROC>(::SetWindowLongPtr(staticHandle, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(staticProc)));
	}
private:
	COLORREF _colour = RGB(0xFF, 0xFF, 0xFF);
	WNDPROC _oldProc = nullptr;
	static LRESULT CALLBACK staticProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		ColourStaticTextHooker * pColourStaticTextHooker = reinterpret_cast<ColourStaticTextHooker *>(::GetWindowLongPtr(hwnd, GWLP_USERDATA));
		return pColourStaticTextHooker->colourStaticProc(hwnd, message, wParam, lParam);
	}
	LRESULT CALLBACK colourStaticProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
};

class WordStyleDlg : public StaticDialog {
public:
	WordStyleDlg() = default;
	void init(HINSTANCE hInst, HWND parent)     
	{
		Window::init(hInst, parent);
	}
	virtual void create(int dialogID, bool isRTL = false, bool msgDestParent = true);
	void doDialog(bool isRTL = false);
	void prepare2Cancel();
	virtual void redraw(bool forceUpdate = false) const;
	void restoreGlobalOverrideValues();
	void apply();
	void addLastThemeEntry();
	bool selectThemeByName(const TCHAR* themeName);
private:
	ColourPicker * _pFgColour = nullptr;
	ColourPicker * _pBgColour = nullptr;
	int _currentLexerIndex = 0;
	int _currentThemeIndex = 0;
	HWND _hCheckBold = nullptr;
	HWND _hCheckItalic = nullptr;
	HWND _hCheckUnderline = nullptr;
	HWND _hFontNameCombo = nullptr;
	HWND _hFontSizeCombo = nullptr;
	HWND _hSwitch2ThemeCombo = nullptr;
	HWND _hFgColourStaticText = nullptr;
	HWND _hBgColourStaticText = nullptr;
	HWND _hFontNameStaticText = nullptr;
	HWND _hFontSizeStaticText = nullptr;
	HWND _hStyleInfoStaticText = nullptr;
	LexerStylerArray _lsArray;
	StyleArray _globalStyles;
	generic_string _themeName;
	LexerStylerArray _styles2restored;
	StyleArray _gstyles2restored;
	GlobalOverride _gOverride2restored;
	bool _restoreInvalid = false;
	ColourStaticTextHooker _colourHooker;
	bool _isDirty = false;
	bool _isThemeDirty = false;
	bool _isShownGOCtrls = false;

	INT_PTR CALLBACK run_dlgProc(UINT Message, WPARAM wParam, LPARAM lParam);
	NppStyle & getCurrentStyler();
	int whichTabColourIndex();
	bool isDocumentMapStyle();
	void move2CtrlRight(int ctrlID, HWND handle2Move, int handle2MoveWidth, int handle2MoveHeight);
	void updateColour(bool which);
	void updateFontStyleStatus(fontStyleType whitchStyle);
	void updateExtension();
	void updateFontName();
	void updateFontSize();
	void updateUserKeywords();
	void switchToTheme();
	void updateThemeName(const generic_string& themeName);
	void loadLangListFromNppParam();
	void enableFg(bool isEnable);
	void enableBg(bool isEnable);
	void enableFontName(bool isEnable);
	void enableFontSize(bool isEnable);
	void enableFontStyle(bool isEnable);
	long notifyDataModified();
	void setStyleListFromLexer(int index);
	void setVisualFromStyleList();
	void updateGlobalOverrideCtrls();
	void showGlobalOverrideCtrls(bool show);
	void applyCurrentSelectedThemeAndUpdateUI();
};
//
//#include "trayIconControler.h"
#define ADD     NIM_ADD
#define REMOVE  NIM_DELETE

// code d'erreur
#define INCORRECT_OPERATION     1
#define OPERATION_INCOHERENT    2

class trayIconControler {
public:
	trayIconControler(HWND hwnd, UINT uID, UINT uCBMsg, HICON hicon, const TCHAR * tip);
	int doTrayIcon(DWORD op);
	bool isInTray() const { return _isIconShowed; };
private:
	NOTIFYICONDATA _nid;
	bool _isIconShowed = false;
};
//
#include "PluginInterface.h"
//#include "IDAllocator.h"
class IDAllocator {
public:
	IDAllocator(int start, int maximumID);
	/// Returns -1 if not enough available
	int allocate(int quantity);
	bool isInRange(int id) { return (id >= _start && id < _nextID); }
private:
	int _start = 0;
	int _nextID = 0;
	int _maximumID = 0;
};
//
//#include "PluginsManager.h"
typedef BOOL (__cdecl * PFUNCISUNICODE)();

struct PluginCommand {
	generic_string _pluginName;
	int _funcID = 0;
	PFUNCPLUGINCMD _pFunc = nullptr;
	PluginCommand(const TCHAR * pluginName, int funcID, PFUNCPLUGINCMD pFunc) : _funcID(funcID), _pFunc(pFunc), _pluginName(pluginName)
	{
	}
};

struct PluginInfo {
	PluginInfo() = default;
	~PluginInfo()
	{
		if(_pluginMenu)
			::DestroyMenu(_pluginMenu);
		if(_hLib)
			::FreeLibrary(_hLib);
	}
	HINSTANCE _hLib = nullptr;
	HMENU _pluginMenu = nullptr;
	PFUNCSETINFO _pFuncSetInfo = nullptr;
	PFUNCGETNAME _pFuncGetName = nullptr;
	PBENOTIFIED _pBeNotified = nullptr;
	PFUNCGETFUNCSARRAY _pFuncGetFuncsArray = nullptr;
	PMESSAGEPROC _pMessageProc = nullptr;
	PFUNCISUNICODE _pFuncIsUnicode = nullptr;
	FuncItem * _funcItems = nullptr;
	int _nbFuncItem = 0;
	generic_string _moduleName;
	generic_string _funcName;
};

struct LoadedDllInfo {
	generic_string _fullFilePath;
	generic_string _fileName;
	LoadedDllInfo(const generic_string & fullFilePath, const generic_string & fileName) : _fullFilePath(fullFilePath), _fileName(fileName) 
	{
	}
};

class PluginsManager {
	friend class PluginsAdminDlg;
public:
	PluginsManager();
	~PluginsManager();
	void init(const NppData & nppData);
	int loadPlugin(const TCHAR * pluginFilePath);
	bool loadPluginsV2(const TCHAR * dir = NULL);
	bool unloadPlugin(int index, HWND nppHandle);
	void runPluginCommand(size_t i);
	void runPluginCommand(const TCHAR * pluginName, int commandID);
	void addInMenuFromPMIndex(int i);
	HMENU setMenu(HMENU hMenu, const TCHAR * menuName, bool enablePluginAdmin = false);
	bool getShortcutByCmdID(int cmdID, ShortcutKey * sk);
	bool removeShortcutByCmdID(int cmdID);
	void notify(size_t indexPluginInfo, const SCNotification * notification); // to a plugin
	void notify(const SCNotification * notification); // broadcast
	void relayNppMessages(UINT Message, WPARAM wParam, LPARAM lParam);
	bool relayPluginMessages(UINT Message, WPARAM wParam, LPARAM lParam);
	HMENU getMenuHandle() const { return _hPluginsMenu; }
	void disable() { _isDisabled = true; }
	bool hasPlugins() { return (_pluginInfos.size()!= 0); }
	bool allocateCmdID(int numberRequired, int * start);
	bool inDynamicRange(int id) { return _dynamicIDAlloc.isInRange(id); }
	bool allocateMarker(int numberRequired, int * start);
	generic_string getLoadedPluginNames() const;
private:
	void pluginCrashAlert(const TCHAR * pluginName, const TCHAR * funcSignature);
	void pluginExceptionAlert(const TCHAR * pluginName, const std::exception& e);
	bool isInLoadedDlls(const TCHAR * fn) const;
	void addInLoadedDlls(const TCHAR * fullPath, const TCHAR * fn);

	NppData _nppData;
	HMENU _hPluginsMenu = NULL;
	std::vector<PluginInfo *> _pluginInfos;
	std::vector<PluginCommand> _pluginsCommands;
	std::vector<LoadedDllInfo> _loadedDlls;
	bool _isDisabled = false;
	IDAllocator _dynamicIDAlloc;
	IDAllocator _markerAlloc;
	bool _noMoreNotification = false;
};

#define EXT_LEXER_DECL __stdcall

// External Lexer function definitions...
typedef int (EXT_LEXER_DECL *GetLexerCountFn)();
typedef void (EXT_LEXER_DECL *GetLexerNameFn)(uint Index, char * name, int buflength);
typedef void (EXT_LEXER_DECL *GetLexerStatusTextFn)(uint Index, TCHAR * desc, int buflength);
//
//#include "regExtDlg.h"
const int extNameLen = 32;

class RegExtDlg : public StaticDialog {
public:
	RegExtDlg() = default;
	~RegExtDlg() = default;
	void doDialog(bool isRTL = false);
private:
	bool _isCustomize = false;
	INT_PTR CALLBACK run_dlgProc(UINT Message, WPARAM wParam, LPARAM lParam);
	void getRegisteredExts();
	void getDefSupportedExts();
	void addExt(TCHAR * ext);
	bool deleteExts(const TCHAR * ext2Delete);
	void writeNppPath();
	int getNbSubKey(HKEY hKey) const;
	int getNbSubValue(HKEY hKey) const;
};
//
//#include "preferenceDlg.h"
class MiscSubDlg : public StaticDialog {
public:
	MiscSubDlg() = default;
private:
	INT_PTR CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
};

class GeneralSubDlg : public StaticDialog {
public:
	GeneralSubDlg() = default;
	void setToolIconsFromStdToSmall();
	void disableTabbarAlternateIcons();
private:
	INT_PTR CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
};

class EditingSubDlg : public StaticDialog {
public:
	EditingSubDlg() = default;
private:
	INT_PTR CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
	void initScintParam();
};

class DarkModeSubDlg : public StaticDialog {
public:
	DarkModeSubDlg() = default;
private:
	ColourPicker* _pBackgroundColorPicker = nullptr;
	ColourPicker* _pSofterBackgroundColorPicker = nullptr;
	ColourPicker* _pHotBackgroundColorPicker = nullptr;
	ColourPicker* _pPureBackgroundColorPicker = nullptr;
	ColourPicker* _pErrorBackgroundColorPicker = nullptr;
	ColourPicker* _pTextColorPicker = nullptr;
	ColourPicker* _pDarkerTextColorPicker = nullptr;
	ColourPicker* _pDisabledTextColorPicker = nullptr;
	ColourPicker* _pEdgeColorPicker = nullptr;
	ColourPicker* _pLinkColorPicker = nullptr;
	INT_PTR CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
	void enableCustomizedColorCtrls(bool doEnable);
	void move2CtrlLeft(int ctrlID, HWND handle2Move, int handle2MoveWidth, int handle2MoveHeight);
};

class MarginsBorderEdgeSubDlg : public StaticDialog {
public:
	MarginsBorderEdgeSubDlg() = default;
private:
	INT_PTR CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
	void initScintParam();
};

struct LangID_Name {
	LangType _id = L_TEXT;
	generic_string _name;
	LangID_Name(LangType id, const generic_string& name) : _id(id), _name(name){
	};
};

class NewDocumentSubDlg : public StaticDialog {
public:
	NewDocumentSubDlg() = default;
private:
	void makeOpenAnsiAsUtf8(bool doIt);
	INT_PTR CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
};

class DefaultDirectorySubDlg : public StaticDialog {
public:
	DefaultDirectorySubDlg() = default;
private:
	INT_PTR CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
};

class RecentFilesHistorySubDlg : public StaticDialog {
public:
	RecentFilesHistorySubDlg() = default;
	virtual void destroy() 
	{
		_nbHistoryVal.destroy();
		_customLenVal.destroy();
	}
private:
	URLCtrl _nbHistoryVal;
	URLCtrl _customLenVal;
	void setCustomLen(int val);
	INT_PTR CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
};

class LanguageSubDlg : public StaticDialog {
public:
	LanguageSubDlg() = default;
	virtual void destroy() 
	{
		_tabSizeVal.destroy();
	}
private:
	LexerStylerArray _lsArray;
	URLCtrl _tabSizeVal;
	INT_PTR CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
	std::vector<LangMenuItem> _langList;
};

class HighlightingSubDlg : public StaticDialog {
public:
	HighlightingSubDlg() = default;
private:
	INT_PTR CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
};

class SearchingSubDlg : public StaticDialog {
public:
	SearchingSubDlg() = default;
private:
	INT_PTR CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
};

struct strCouple {
	generic_string _varDesc;
	generic_string _var;
	strCouple(const TCHAR * varDesc, const TCHAR * var) : _varDesc(varDesc), _var(var)
	{
	}
};

class PrintSubDlg : public StaticDialog {
public:
	PrintSubDlg() = default;
private:
	INT_PTR CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
	std::vector<strCouple> varList;
	int _focusedEditCtrl = 0;
};

class BackupSubDlg : public StaticDialog {
public:
	BackupSubDlg() = default;
private:
	void updateBackupGUI();
	INT_PTR CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
};

class AutoCompletionSubDlg : public StaticDialog {
public:
	AutoCompletionSubDlg() = default;
private:
	URLCtrl _nbCharVal;
	INT_PTR CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
};

class MultiInstanceSubDlg : public StaticDialog {
public:
	MultiInstanceSubDlg() = default;
private:
	const SYSTEMTIME _BTTF_time = {1985, 10, 6, 26, 16, 24, 42, 0};
	INT_PTR CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
};

class DelimiterSubDlg : public StaticDialog {
public:
	DelimiterSubDlg() = default;
	~DelimiterSubDlg() 
	{
		if(_tip)
			::DestroyWindow(_tip);
	}
private:
	POINT _singleLineModePoint, _multiLineModePoint;
	RECT _closerRect = { 0 };
	RECT _closerLabelRect = { 0 };
	HWND _tip = nullptr;

	INT_PTR CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
	void detectSpace(const char * text2Check, int & nbSp, int & nbTab) const;
	generic_string getWarningText(size_t nbSp, size_t nbTab) const;
	void setWarningIfNeed() const;
};

class CloudAndLinkSubDlg : public StaticDialog {
public:
	CloudAndLinkSubDlg() = default;
private:
	INT_PTR CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
};

class SearchEngineSubDlg : public StaticDialog {
public:
	SearchEngineSubDlg() = default;
private:
	INT_PTR CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
};

class PreferenceDlg : public StaticDialog {
	friend class NativeLangSpeaker;
	friend class Notepad_plus;
public:
	PreferenceDlg() = default;
	void init(HINSTANCE hInst, HWND parent)     
	{
		Window::init(hInst, parent);
	}
	void doDialog(bool isRTL = false) 
	{
		if(!isCreated()) {
			create(IDD_PREFERENCE_BOX, isRTL);
			goToCenter();
		}
		display();
	}
	bool renameDialogTitle(const TCHAR * internalName, const TCHAR * newName);
	int getListSelectedIndex() const 
	{
		return static_cast<int32_t>(::SendDlgItemMessage(_hSelf, IDC_LIST_DLGTITLE, LB_GETCURSEL, 0, 0));
	}
	void showDialogByName(const TCHAR * name) const;
	bool setListSelection(size_t currentSel) const;
	virtual void destroy();
private:
	INT_PTR CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
	void makeCategoryList();
	int32_t getIndexFromName(const TCHAR * name) const;
	void showDialogByIndex(size_t index) const;
	WindowVector _wVector;
	GeneralSubDlg _generalSubDlg;
	EditingSubDlg _editingSubDlg;
	DarkModeSubDlg _darkModeSubDlg;
	MarginsBorderEdgeSubDlg _marginsBorderEdgeSubDlg;
	MiscSubDlg _miscSubDlg;
	RegExtDlg _fileAssocDlg;
	LanguageSubDlg _languageSubDlg;
	HighlightingSubDlg _highlightingSubDlg;
	PrintSubDlg _printSubDlg;
	NewDocumentSubDlg _newDocumentSubDlg;
	DefaultDirectorySubDlg _defaultDirectorySubDlg;
	RecentFilesHistorySubDlg _recentFilesHistorySubDlg;
	BackupSubDlg _backupSubDlg;
	AutoCompletionSubDlg _autoCompletionSubDlg;
	MultiInstanceSubDlg _multiInstanceSubDlg;
	DelimiterSubDlg _delimiterSubDlg;
	CloudAndLinkSubDlg _cloudAndLinkSubDlg;
	SearchEngineSubDlg _searchEngineSubDlg;
	SearchingSubDlg _searchingSubDlg;
};
//
//#include "WinMgr.h"
//
// MSDN Magazine -- July 2001
// If this code works, it was written by Paul DiLascia.
// If not, I don't know who wrote it.
// Compiles with Visual C++ 6.0. Runs on Win 98 and probably Win 2000 too.
// Set tabsize = 3 in your editor.
// WinMgr.h -- Main header file for WinMgr library.
// Theo - Heavily modified to remove MFC dependencies. Replaced CWnd*/HWND, CRect/RECT, CSize/SIZE, CPoint/POINT
//
const SIZE SIZEZERO = {0, 0};
const SIZE SIZEMAX = {SHRT_MAX, SHRT_MAX};

inline SIZE GetSize(LONG w, LONG h) { SIZE sz = {w, h}; return sz; }
inline POINT GetPoint(LONG x, LONG y) { POINT pt = {x, y}; return pt; }
inline LONG RectWidth(const RECT& rc) { return rc.right - rc.left; }
inline LONG RectHeight(const RECT& rc) { return rc.bottom - rc.top; }
inline SIZE RectToSize(const RECT& rc) { return GetSize(RectWidth(rc), RectHeight(rc)); }

inline POINT RectToPoint(const RECT& rc) 
{
	POINT pt = {rc.left, rc.top};
	return pt; 
}

inline POINT SizeToPoint(SIZE sz) { return GetPoint(sz.cx, sz.cy); }

inline RECT &OffsetRect(RECT& rc, POINT pt) 
{
	rc.left += pt.x; rc.right += pt.x;
	rc.top += pt.y; rc.bottom += pt.y;
	return rc;
}

// handy functions to take the min or max of a SIZE
inline SIZE minsize(SIZE a, SIZE b) { return GetSize(MIN(a.cx, b.cx), MIN(a.cy, b.cy)); }
inline SIZE maxsize(SIZE a, SIZE b) { return GetSize(MAX(a.cx, b.cx), MAX(a.cy, b.cy)); }

//////////////////
// Size info about a rectangle/row/column
//
struct SIZEINFO {
	SIZE szAvail;		// total size avail (passed)
	SIZE szDesired;	// desired size: default=current
	SIZE szMin;			// minimum size: default=SIZEZERO
	SIZE szMax;			// maximum size: default=MAXSIZE
};

// types of rectangles:
#define	WRCT_END			0				// end of table
#define	WRCT_FIXED		0x0001		// height/width is fixed
#define	WRCT_PCT			0x0002		// height/width is percent of total
#define	WRCT_REST		0x0003		// height/width is whatever remains
#define	WRCT_TOFIT		0x0004		// height/width to fit contents
#define	WRCF_TYPEMASK	0x000F

// group flags
#define	WRCF_ROWGROUP	0x0010		// beginning of row group
#define	WRCF_COLGROUP	0x0020		// beginning of column group
#define	WRCF_ENDGROUP	0x00F0		// end of group
#define	WRCF_GROUPMASK	0x00F0

//////////////////
// This structure is used to hold a rectangle and describe its layout. Each
// WINRECT corresponds to a child rectangle/window. Each window that uses
// WinMgr provides a table (C array) of these to describe its layout.
//
class WINRECT {
protected:
	// pointers initialized by the window manager for easy traversing:
	WINRECT* next;			// next at this level
	WINRECT* prev;			// prev at this level
	// data
	RECT  rc;				// current rectangle position/size
	WORD  flags;			// flags (see above)
	UINT	nID;				// window ID if this WINRECT represents a window
	LONG	param;			// arg depends on type
public:
	WINRECT(WORD f, int id, LONG p);
	static WINRECT* InitMap(WINRECT* map, WINRECT* parent=NULL);
	WINRECT* Prev()			{ return prev; }
	WINRECT* Next()			{ return next; }
	WINRECT* Children() { return IsGroup() ? this+1 : NULL; }
	WINRECT* Parent();
	WORD GetFlags()			{ return flags; }
	WORD SetFlags(WORD f)	{ return flags=f; }
	LONG GetParam()			{ return param; }
	LONG SetParam(LONG p)	{ return param=p; }
	UINT GetID() { return nID; }
	UINT SetID(UINT id) { return nID=id; }
	RECT& GetRect()					{ return rc; }
	void SetRect(const RECT& r)	{ rc = r; }
	WORD Type() const			{ return flags & WRCF_TYPEMASK; }
	WORD GroupType() const	{ return flags & WRCF_GROUPMASK; }
	BOOL IsGroup() const		{ return GroupType() && GroupType()!=WRCF_ENDGROUP; }
	BOOL IsEndGroup() const { return flags==0 || flags==WRCF_ENDGROUP; }
	BOOL IsEnd() const		{ return flags==0; }
	BOOL IsWindow() const	{ return nID>0; }
	BOOL IsRowGroup()	const { return (flags & WRCF_GROUPMASK)==WRCF_ROWGROUP; }
	void SetHeight(LONG h)	{ rc.bottom = rc.top + h; }
	void SetWidth(LONG w)	{ rc.right = rc.left + w; }
	LONG GetHeightOrWidth(BOOL bHeight) const { return bHeight ? RectHeight(rc) : RectWidth(rc); }
	void SetHeightOrWidth(LONG horw, BOOL bHeight) { bHeight ? SetHeight(horw) : SetWidth(horw); }
	BOOL GetMargins(int& w, int& h);
	// For TOFIT types, param is the TOFIT size, if nonzero. Used in dialogs,
	// with CWinMgr::InitToFitSizeFromCurrent.
	BOOL HasToFitSize()			{ return param != 0; }
	SIZE GetToFitSize()			{ SIZE sz = {LOWORD(param),HIWORD(param)}; return sz; }
	void SetToFitSize(SIZE sz)	{ param = MAKELONG(sz.cx,sz.cy); }
};
//
// Below are all the macros to build your window map. 
//
// Begin/end window map. 'name' can be anything you want
#define BEGIN_WINDOW_MAP(name)	WINRECT name[] = {
#define END_WINDOW_MAP()			WINRECT(WRCT_END,-1,0) }; 

// Begin/end a group.
// The first entry in your map must be BEGINROWS or BEGINCOLS.
#define BEGINROWS(type,id,m)	WINRECT(WRCF_ROWGROUP|type,id,m),
#define BEGINCOLS(type,id,m)  WINRECT(WRCF_COLGROUP|type,id,m),
#define ENDGROUP()				WINRECT(WRCF_ENDGROUP,-1,0),

// This macros is used only with BEGINGROWS or BEGINCOLS to specify margins
#define RCMARGINS(w,h)			MAKELONG(w,h)

// Macros for primitive (non-group) entries.
// val applies to height for a row entry; width for a column entry.
#define RCFIXED(id,val)		WINRECT(WRCT_FIXED,id,val),
#define RCPERCENT(id,val)	WINRECT(WRCT_PCT,id,val),
#define RCREST(id)			WINRECT(WRCT_REST,id,0),
#define RCTOFIT(id)			WINRECT(WRCT_TOFIT,id,0),
#define RCSPACE(val)			RCFIXED(-1,val)
//
// Use this to iterate the entries in a group.
//
//	CWinGroupIterator it;
//	for(it=pGroup; it; it.Next()) {
//   WINRECT* wrc = it;
//   ..
// }
//
class CWinGroupIterator {
protected:
	WINRECT* pCur;	  // current entry
public:
	CWinGroupIterator() : pCur(NULL)
	{ 
	}
	CWinGroupIterator & operator = (WINRECT* pg) 
	{
		assert(pg->IsGroup()); // can only iterate a group!
		pCur = pg->Children();
		return *this;
	}
	operator WINRECT*()	{ return pCur; }
	WINRECT* pWINRECT()	{ return pCur; }
	WINRECT* Next() { return pCur = pCur ? pCur->Next() : NULL;}
};

extern const UINT WM_WINMGR; // Registered WinMgr message

// Notification struct, passed as LPARAM
struct NMWINMGR : public NMHDR {
	enum {								// notification codes:
		GET_SIZEINFO = 1,				// WinMgr is requesting size info
		SIZEBAR_MOVED					// user moved sizer bar
	};
	// each notification code has its own part of union
	union {
		SIZEINFO sizeinfo;	// used for GET_SIZEINFO
		struct {					// used for SIZEBAR_MOVED
			POINT ptMoved;		//  distance moved (x or y = zero)
		} sizebar;
	};
	BOOL processed;
	// ctor: initialize to zeroes
	NMWINMGR() { memzero(this, sizeof(NMWINMGR)); }
};
//
// Window manager. This class calculates all the sizes and positions of the
// rectangles in the window map.
//
class CWinMgr /*: public CObject*/ {
public:
	explicit CWinMgr(WINRECT* map);
	virtual ~CWinMgr();
	virtual void GetWindowPositions(HWND hWnd); // load map from window posns
	virtual void SetWindowPositions(HWND hWnd); // set window posns from map
	// get min/max/desired size of a rectangle
	virtual void OnGetSizeInfo(SIZEINFO& szi, WINRECT* pwrc, HWND hWnd=NULL);
	// calc layout using client area as total area
	void CalcLayout(HWND hWnd) 
	{
		assert(hWnd);
		RECT rcClient;
		GetClientRect(hWnd, &rcClient);
		CalcLayout(rcClient, hWnd);
	}
	// calc layout using cx, cy (for OnSize)
	void CalcLayout(int cx, int cy, HWND hWnd=NULL) 
	{
		RECT rc = {0,0,cx,cy};
		CalcLayout(rc, hWnd);
	}
	// calc layout using given rect as total area
	void CalcLayout(RECT rcTotal, HWND hWnd=NULL) 
	{
		assert(m_map);
		m_map->SetRect(rcTotal);
		CalcGroup(m_map, hWnd);
	}
	// Move rectangle vertically or horizontally. Used with sizer bars.
	void MoveRect(int nID, POINT ptMove, HWND pParentWnd) 
	{
		MoveRect(FindRect(nID), ptMove, pParentWnd);
	}
	void MoveRect(WINRECT* pwrcMove, POINT ptMove, HWND pParentWnd);
	RECT GetRect(UINT nID)						 { return FindRect(nID)->GetRect(); }
	void SetRect(UINT nID, const RECT& rc) { FindRect(nID)->SetRect(rc); }
	// get WINRECT corresponding to ID
	WINRECT* FindRect(int nID);
	// Calculate MINMAXINFO
	void GetMinMaxInfo(HWND hWnd, MINMAXINFO* lpMMI);
	void GetMinMaxInfo(HWND hWnd, SIZEINFO& szi);
	// set TOFIT size for all windows from current window sizes
	void InitToFitSizeFromCurrent(HWND hWnd);
protected:
	WINRECT*	m_map = nullptr;			// THE window map
	int  CountWindows();
	BOOL SendGetSizeInfo(SIZEINFO& szi, HWND hWnd, UINT nID);
	// you can override to do wierd stuff or fix bugs
	virtual void CalcGroup(WINRECT* group, HWND hWnd);
	virtual void AdjustSize(WINRECT* pEntry, BOOL bRow, int& hwRemaining, HWND hWnd);
	virtual void PositionRects(WINRECT* pGroup, const RECT& rcTotal,BOOL bRow);
private:
	CWinMgr() { assert(FALSE); } // no default constructor
};
//
//#include "SizeableDlg.h"
class SizeableDlg : public StaticDialog {
	typedef StaticDialog MyBaseClass;
public:
	explicit SizeableDlg(WINRECT* pWinMap);
protected:
	CWinMgr _winMgr;          // window manager
	virtual INT_PTR CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
	virtual BOOL onInitDialog();
	virtual void onSize(UINT nType, int cx, int cy);
	virtual void onGetMinMaxInfo(MINMAXINFO* lpMMI);
	virtual LRESULT onWinMgr(WPARAM wp, LPARAM lp);
};
//
//#include "WindowsDlg.h"

typedef enum {
	WDT_ACTIVATE = 1,
	WDT_SAVE = 2,
	WDT_CLOSE = 3,
	WDT_SORT = 4,
} WinDlgNotifyType;

struct NMWINDLG : public NMHDR {
	BOOL processed = FALSE;
	WinDlgNotifyType type = WDT_ACTIVATE;
	UINT curSel = 0;
	UINT nItems = 0;
	UINT * Items = 0;
	// ctor: initialize to zeroes
	NMWINDLG() 
	{
		memzero(this, sizeof(NMWINDLG));
	}
};

extern const UINT WDN_NOTIFY;

class WindowsDlg : public SizeableDlg {
	typedef SizeableDlg MyBaseClass;
public:
	WindowsDlg();
	int doDialog();
	virtual void init(HINSTANCE hInst, HWND parent, DocTabView * pTab);
	void doRefresh(bool invalidate = false);
protected:
	virtual INT_PTR CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
	virtual BOOL onInitDialog();
	virtual void onSize(UINT nType, int cx, int cy);
	virtual void onGetMinMaxInfo(MINMAXINFO* lpMMI);
	virtual LRESULT onWinMgr(WPARAM wp, LPARAM lp);
	virtual void destroy();
	void updateColumnNames();
	void fitColumnsToSize();
	void resetSelection();
	void doSave();
	void doClose();
	void doSortToTabs();
	void updateButtonState();
	void activateCurrent();
	void doColumnSort();
	void doCount();
	void putItemsToClipboard(bool isFullPath);
	Buffer* getBuffer(int index) const;
	static LONG_PTR originalListViewProc;
	static LRESULT CALLBACK listViewProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);

	HWND _hList = nullptr;
	static RECT _lastKnownLocation;
	SIZE _szMinButton = { 0 };
	SIZE _szMinListCtrl = { 0 };
	DocTabView * _pTab = nullptr;
	std::vector<int> _idxMap;
	int _currentColumn = -1;
	int _lastSort = -1;
	bool _reverseSort = false;
	ContextMenu _listMenu;
private:
	virtual void init(HINSTANCE hInst, HWND parent);
};

class WindowsMenu {
public:
	WindowsMenu();
	~WindowsMenu();
	void init(HINSTANCE hInst, HMENU hMainMenu, const TCHAR * translation);
	void initPopupMenu(HMENU hMenu, DocTabView * pTab);
private:
	HMENU _hMenu = nullptr;
};
//
//#include "RunMacroDlg.h"
#define RM_CANCEL -1
#define RM_RUN_MULTI 1
#define RM_RUN_EOF 2

class RunMacroDlg : public StaticDialog {
public:
	RunMacroDlg() = default;
	~RunMacroDlg() = default;
	void init(HINSTANCE hInst, HWND hPere /*, ScintillaEditView **ppEditView*/);
	void doDialog(bool isRTL = false);
	void initMacroList();
	int getMode() const { return _mode; }
	int getTimes() const { return _times; }
	int getMacro2Exec() const;
private:
	virtual INT_PTR CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
	void check(int);
	int _mode = RM_RUN_MULTI;
	int _times = 1;
	int _macroIndex = 0;
};
//
//#include "DockingCont.h"
// window styles
#define POPUP_STYLES            (WS_POPUP|WS_CLIPSIBLINGS|WS_CAPTION|WS_SYSMENU|WS_THICKFRAME|WS_MAXIMIZEBOX)
#define POPUP_EXSTYLES          (WS_EX_CONTROLPARENT|WS_EX_WINDOWEDGE|WS_EX_TOOLWINDOW)
#define CHILD_STYLES            (WS_CHILD)
#define CHILD_EXSTYLES          (0x00000000L)

enum eMousePos {
	posOutside,
	posCaption,
	posClose
};

// some fix modify values for GUI
#define HIGH_CAPTION       18
#define CAPTION_GAP         2
#define CLOSEBTN_POS_LEFT   3
#define CLOSEBTN_POS_TOP    3

class DockingCont : public StaticDialog { // Copyright (C) 2006 Jens Lorenz <jens.plugin.npp@gmx.de>
public:
	DockingCont();
	~DockingCont();
	HWND getTabWnd() const { return _hContTab; }
	HWND getCaptionWnd() const { return (_isFloating == false) ? _hCaption : _hSelf; }
	tTbData* createToolbar(tTbData data);
	void     removeToolbar(tTbData data);
	tTbData* findToolbarByWnd(HWND hClient);
	tTbData* findToolbarByName(TCHAR* pszName);
	void showToolbar(tTbData * pTbData, BOOL state);
	BOOL updateInfo(HWND hClient);
	void setActiveTb(tTbData* pTbData);
	void setActiveTb(int iItem);
	int getActiveTb();
	tTbData * getDataOfActiveTb();
	std::vector<tTbData *> getDataOfAllTb() { return _vTbData; }
	std::vector<tTbData *> getDataOfVisTb();
	bool isTbVis(tTbData* data);
	void doDialog(bool willBeShown = true, bool isFloating = false);
	bool isFloating() const { return _isFloating; }
	size_t getElementCnt() const { return _vTbData.size(); }
	// interface function for gripper
	BOOL startMovingFromTab() 
	{
		BOOL dragFromTabTemp = _dragFromTab;
		_dragFromTab = FALSE;
		return dragFromTabTemp;
	}
	void setCaptionTop(BOOL isTopCaption) 
	{
		_isTopCaption = (isTopCaption == CAPTION_TOP);
		onSize();
	}
	void focusClient();
	void SetActive(BOOL bState) 
	{
		_isActive = bState;
		updateCaption();
	}
	void setTabStyle(const BOOL & bDrawOgLine) 
	{
		_bDrawOgLine = bDrawOgLine;
		RedrawWindow(_hContTab, NULL, NULL, 0);
	}
	virtual void destroy() 
	{
		for(int iTb = static_cast<int>(_vTbData.size()); iTb > 0; iTb--) {
			delete _vTbData[iTb-1];
		}
		::DestroyWindow(_hSelf);
	}
protected:
	// Subclassing caption
	LRESULT runProcCaption(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK wndCaptionProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam) {
		return (((DockingCont*)(::GetWindowLongPtr(hwnd, GWLP_USERDATA)))->runProcCaption(hwnd, Message, wParam, lParam));
	};
	// Subclassing tab
	LRESULT runProcTab(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK wndTabProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam) {
		return (((DockingCont*)(::GetWindowLongPtr(hwnd, GWLP_USERDATA)))->runProcTab(hwnd, Message, wParam, lParam));
	};
	virtual INT_PTR CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
	// drawing functions
	void drawCaptionItem(DRAWITEMSTRUCT * pDrawItemStruct);
	void drawTabItem(DRAWITEMSTRUCT * pDrawItemStruct);
	void onSize();
	eMousePos isInRect(HWND hwnd, int x, int y); // functions for caption handling and drawing
	void doClose(BOOL closeAll); // handling of toolbars
	int  searchPosInTab(tTbData* pTbData); // return new item
	void selectTab(int iTab);
	int  hideToolbar(tTbData* pTbData, BOOL hideClient = TRUE);
	void viewToolbar(tTbData * pTbData);
	int  removeTab(tTbData* pTbData) { return hideToolbar(pTbData, FALSE); }
	bool updateCaption();
	LPARAM NotifyParent(UINT message);
private:
	// handles
	BOOL _isActive = FALSE;
	bool _isFloating = FALSE;
	HWND _hCaption = nullptr;
	HWND _hContTab = nullptr;
	HFONT _hFont = nullptr; // horizontal font for caption and tab
	// caption params
	BOOL _isTopCaption = FALSE;
	generic_string _pszCaption;
	BOOL _isMouseDown = FALSE;
	BOOL _isMouseClose = FALSE;
	BOOL _isMouseOver = FALSE;
	RECT _rcCaption = {0};
	BOOL _bDrawOgLine = FALSE; // tab style
	BOOL _dragFromTab = FALSE; // Important value for DlgMoving class
	WNDPROC _hDefaultCaptionProc = nullptr; // subclassing handle for caption
	WNDPROC _hDefaultTabProc = nullptr; // subclassing handle for tab
	// for moving and reordering
	UINT _prevItem = 0;
	BOOL _beginDrag = FALSE;
	BOOL _bTabTTHover = FALSE; // Is tooltip
	INT _iLastHovered = 0;
	BOOL _bCaptionTT = FALSE;
	BOOL _bCapTTHover = FALSE;
	eMousePos _hoverMPos = posOutside;
	int _captionHeightDynamic = HIGH_CAPTION;
	int _captionGapDynamic = CAPTION_GAP;
	int _closeButtonPosLeftDynamic = CLOSEBTN_POS_LEFT;
	int _closeButtonPosTopDynamic = CLOSEBTN_POS_TOP;
	int _closeButtonWidth = 12;
	int _closeButtonHeight = 12;
	std::vector<tTbData *> _vTbData; // data of added windows
};
//
//#include "DockingManager.h"
#define DSPC_CLASS_NAME TEXT("dockingManager")
#define CONT_MAP_MAX    50

class DockingManager : public Window { // Copyright (C) 2006 Jens Lorenz <jens.plugin.npp@gmx.de>
public:
	DockingManager();
	~DockingManager();
	void init(HINSTANCE hInst, HWND hWnd, Window ** ppWin);
	virtual void reSizeTo(RECT & rc);
	void setClientWnd(Window ** ppWin)
	{
		_ppWindow = ppWin;
		_ppMainWindow = ppWin;
	}
	void showFloatingContainers(bool show);
	void updateContainerInfo(HWND hClient);
	void createDockableDlg(tTbData data, int iCont = CONT_LEFT, bool isVisible = false);
	void setActiveTab(int iCont, int iItem);
	void showDockableDlg(HWND hDlg, BOOL view);
	void showDockableDlg(TCHAR* pszName, BOOL view);
	DockingCont * toggleActiveTb(DockingCont* pContSrc, UINT message, BOOL bNew = FALSE, LPRECT rcFloat = NULL);
	DockingCont * toggleVisTb(DockingCont* pContSrc, UINT message, LPRECT rcFloat = NULL);
	void toggleActiveTb(DockingCont* pContSrc, DockingCont* pContTgt);
	void toggleVisTb(DockingCont* pContSrc, DockingCont* pContTgt);
	// get number of container
	int  GetContainer(DockingCont* pCont);
	// get all container in vector
	std::vector<DockingCont*> & getContainerInfo() { return _vContainer; }
	// get dock data (sized areas)
	void getDockInfo(tDockMgr * pDockData) 
	{
		*pDockData = _dockData;
	}
	// setting styles of docking
	void FASTCALL setStyleCaption(BOOL captionOnTop);
	void FASTCALL setTabStyle(BOOL orangeLine);
	int getDockedContSize(int iCont);
	void setDockedContSize(int iCont, int iSize);
	virtual void destroy();
	void resize();
private:
	Window ** _ppWindow = nullptr;
	RECT _rcWork = { 0 };
	RECT _rect = { 0 };
	Window ** _ppMainWindow = nullptr;
	std::vector<HWND> _vImageList;
	HIMAGELIST _hImageList = nullptr;
	std::vector<DockingCont*> _vContainer;
	tDockMgr _dockData;
	static BOOL _isRegistered;
	BOOL _isInitialized = FALSE;
	int _iContMap[CONT_MAP_MAX] = { 0 };
	std::vector<DockingSplitter*> _vSplitter;

	static LRESULT CALLBACK staticWinProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
	LRESULT runProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
	void    toggleTb(DockingCont* pContSrc, DockingCont* pContTgt, tTbData TbData);
	// test if container exists
	BOOL ContExists(size_t iCont);
	int      FindEmptyContainer();
	LRESULT SendNotify(HWND hWnd, UINT message);
};
//
//#include "Processus.h"
enum progType {
	WIN32_PROG, 
	CONSOLE_PROG
};

class Process {
public:
	Process(const TCHAR * cmd, const TCHAR * args, const TCHAR * cDir);
	void run(bool isElevationRequired = false) const;
	ulong runSync(bool isElevationRequired = false) const;
protected:
	generic_string _command;
	generic_string _args;
	generic_string _curDir;
};
//
//#include "FunctionCallTip.h"
typedef std::vector<const TCHAR *> stringVec;

class FunctionCallTip { // Copyright (C) 2008 Harry Bruin <harrybharry@users.sourceforge.net>
	friend class AutoCompletion;
public:
	explicit FunctionCallTip(ScintillaEditView * pEditView) : _pEditView(pEditView) 
	{
	}
	~FunctionCallTip() /* cleanup(); */
	{
	}
	void setLanguageXML(TiXmlElement * pXmlKeyword); // set calltip keyword node
	bool updateCalltip(int ch, bool needShown = false); // Ch is character typed, or 0 if another event occured.
		// NeedShown is true if calltip should be attempted to displayed. Return true if calltip was made visible
	void showNextOverload(); // show next overlaoded parameters
	void showPrevOverload(); // show prev overlaoded parameters
	bool isVisible() 
	{
		return _pEditView ? _pEditView->execute(SCI_CALLTIPACTIVE) == TRUE : false;
	} // true if calltip visible
	void close(); // Close calltip if visible
private:
	ScintillaEditView * _pEditView = nullptr;       //Scintilla to display calltip in
	TiXmlElement * _pXmlKeyword = nullptr;  //current keyword node (first one)
	int _curPos = 0;                                        //cursor position
	int _startPos = 0;                                      //display start position
	TiXmlElement * _curFunction = nullptr;  //current function element
	//cache some XML values n stuff
	TCHAR * _funcName = nullptr;                            //name of function
	stringVec _retVals;                             //vector of overload return values/types
	std::vector<stringVec> _overloads;      //vector of overload params (=vector)
	stringVec _descriptions;                //vecotr of function descriptions
	size_t _currentNbOverloads = 0;         //current amount of overloads
	size_t _currentOverload = 0;                    //current chosen overload
	int _currentParam = 0;                          //current highlighted param

	TCHAR _start = '(';
	TCHAR _stop = ')';
	TCHAR _param = ',';
	TCHAR _terminal = ';';
	generic_string _additionalWordChar = TEXT("");
	bool _ignoreCase = true;
	bool _selfActivated = false;

	bool getCursorFunction(); // retrieve data about function at cursor. Returns true if a function was found. Calls loaddata if needed
	bool loadFunction(); // returns true if the function can be found
	void showCalltip(); // display calltip based on current variables
	void reset(); // reset all vars in case function is invalidated
	void cleanup(); // delete any leftovers
	bool isBasicWordChar(TCHAR ch) const { return ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9') || ch == '_'); };
	bool isAdditionalWordChar(TCHAR ch) const;
};
//
//#include "AutoCompletion.h"
const size_t tagMaxLen = 256;

struct MatchedCharInserted {
	MatchedCharInserted() = delete;
	char _c;
	int _pos;
	MatchedCharInserted(char c, int pos) : _c(c), _pos(pos) 
	{
	}
};

class InsertedMatchedChars {
public:
	void init(ScintillaEditView * pEditView) 
	{
		_pEditView = pEditView;
	}
	void removeInvalidElements(MatchedCharInserted mci);
	void add(MatchedCharInserted mci);
	bool isEmpty() const { return _insertedMatchedChars.size() == 0; }
	int search(char startChar, char endChar, int posToDetect);
private:
	std::vector<MatchedCharInserted> _insertedMatchedChars;
	ScintillaEditView * _pEditView = nullptr;
};

class AutoCompletion {
public:
	explicit AutoCompletion(ScintillaEditView * pEditView) : _pEditView(pEditView), _funcCalltip(pEditView) 
	{
		//Do not load any language yet
		_insertedMatchedChars.init(_pEditView);
	}
	~AutoCompletion()
	{
		delete _pXmlFile;
	}
	bool setLanguage(LangType language);
	//AutoComplete from the list
	bool showApiComplete();
	//WordCompletion from the current file
	bool showWordComplete(bool autoInsert); //autoInsert true if completion should fill in the word on a single match
	// AutoComplete from both the list and the current file
	bool showApiAndWordComplete();
	//Parameter display from the list
	bool showFunctionComplete();
	// Autocomplete from path.
	void showPathCompletion();
	void insertMatchedChars(int character, const MatchedPairConf & matchedPairConf);
	void update(int character);
	void callTipClick(size_t direction);
	void getCloseTag(char * closeTag, size_t closeTagLen, size_t caretPos, bool isHTML);
private:
	bool _funcCompletionActive = false;
	ScintillaEditView * _pEditView = nullptr;
	LangType _curLang = L_TEXT;
	TiXmlDocument * _pXmlFile = nullptr;
	TiXmlElement * _pXmlKeyword = nullptr;
	InsertedMatchedChars _insertedMatchedChars;
	bool _ignoreCase = true;
	std::vector<generic_string> _keyWordArray;
	generic_string _keyWords;
	size_t _keyWordMaxLen = 0;
	FunctionCallTip _funcCalltip;
	const TCHAR * getApiFileName();
	void getWordArray(std::vector<generic_string> & wordArray, TCHAR * beginChars, TCHAR * excludeChars);
};
//
//#include "SmartHighlighter.h"
class SmartHighlighter { // Copyright (C) 2008 Harry Bruin <harrybharry@users.sourceforge.net>
public:
	explicit SmartHighlighter(FindReplaceDlg * pFRDlg);
	void highlightView(ScintillaEditView * pHighlightView, ScintillaEditView * unfocusView);
	void highlightViewWithWord(ScintillaEditView * pHighlightView, const generic_string & word2Hilite);
private:
	FindReplaceDlg * _pFRDlg = nullptr;
};
//
//#include "ScintillaCtrls.h"
class ScintillaCtrls {
public:
	void init(HINSTANCE hInst, HWND hNpp) 
	{
		_hInst = hInst;
		_hParent = hNpp;
	}
	HWND createSintilla(HWND hParent);
	ScintillaEditView * getScintillaEditViewFrom(HWND handle2Find);
	//bool destroyScintilla(HWND handle2Destroy);
	void destroy();
private:
	std::vector<ScintillaEditView *> _scintVector;
	HINSTANCE _hInst = nullptr;
	HWND _hParent = nullptr;
	int getIndexFrom(HWND handle2Find);
};
//
//#include "md5Dlgs.h"
enum hashType {
	hash_md5, 
	hash_sha256
};

LRESULT run_textEditProc(WNDPROC oldEditProc, HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

class HashFromFilesDlg : public StaticDialog {
public:
	HashFromFilesDlg() = default;
	void doDialog(bool isRTL = false);
    virtual void destroy() {};
	void setHashType(hashType hashType2set);
protected :
	virtual INT_PTR CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
	hashType _ht = hash_md5;
	static LRESULT CALLBACK HashPathEditStaticProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK HashResultStaticProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
private:
	WNDPROC _oldHashPathEditProc = nullptr;
	WNDPROC _oldHashResultProc = nullptr;
};

class HashFromTextDlg : public StaticDialog {
public :
	HashFromTextDlg() = default;
	void doDialog(bool isRTL = false);
    virtual void destroy() {};
	void generateHash();
	void generateHashPerLine();
	void setHashType(hashType hashType2set);
protected :
	virtual INT_PTR CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
	hashType _ht = hash_md5;
	static LRESULT CALLBACK HashTextEditStaticProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK HashResultStaticProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
private:
	WNDPROC _oldHashTextEditProc = nullptr;
	WNDPROC _oldHashResultProc = nullptr;
};
//
//#include "lesDlgs.h"
const int DEFAULT_NB_NUMBER = 2;

class ValueDlg : public StaticDialog {
public:
	ValueDlg() = default;
	void init(HINSTANCE hInst, HWND parent, int valueToSet, const TCHAR * text);
	int doDialog(POINT p, bool isRTL = false);
	void setNBNumber(int nbNumber) 
	{
		if(nbNumber > 0)
			_nbNumber = nbNumber;
	}
	int reSizeValueBox();
	void destroy() 
	{
	}
protected:
	INT_PTR CALLBACK run_dlgProc(UINT Message, WPARAM wParam, LPARAM);
private:
	int _nbNumber = DEFAULT_NB_NUMBER;
	int _defaultValue = 0;
	generic_string _name;
	POINT _p = {0, 0};
};

// 0 : normal window
// 1 : fullscreen
// 2 : postit
// 4 : distractionFree
const int buttonStatus_nada = 0;            // 0000 0000
const int buttonStatus_fullscreen = 1;      // 0000 0001
const int buttonStatus_postit = 2;          // 0000 0010
const int buttonStatus_distractionFree = 4; // 0000 0100

class ButtonDlg : public StaticDialog {
public:
	ButtonDlg() = default;
	void init(HINSTANCE hInst, HWND parent)
	{
		Window::init(hInst, parent);
	}
	void doDialog(bool isRTL = false);
	void destroy() 
	{
	}
	int getButtonStatus() const { return _buttonStatus; }
	void setButtonStatus(int buttonStatus) { _buttonStatus = buttonStatus; }
	void display(bool toShow = true) const;
protected:
	INT_PTR CALLBACK run_dlgProc(UINT Message, WPARAM wParam, LPARAM);
	int _buttonStatus = buttonStatus_nada;
};
//
//#include "pluginsAdmin.h"
struct Version {
	ulong _major = 0;
	ulong _minor = 0;
	ulong _patch = 0;
	ulong _build = 0;
	Version() = default;
	Version(const generic_string& versionStr);
	void setVersionFrom(const generic_string& filePath);
	generic_string toString();
	bool isNumber(const generic_string& s) const
	{
		return !s.empty() && find_if(s.begin(), s.end(), [](_TCHAR c) { return !_istdigit(c); }) == s.end();
	}
	int compareTo(const Version& v2c) const;
	bool operator <(const Version& v2c) const { return compareTo(v2c) == -1; }
	bool operator >(const Version& v2c) const { return compareTo(v2c) == 1; }
	bool operator ==(const Version& v2c) const { return compareTo(v2c) == 0; }
	bool operator !=(const Version& v2c) const { return compareTo(v2c) != 0; }
};

struct PluginUpdateInfo {
	generic_string _fullFilePath; // only for the installed Plugin
	generic_string _folderName;   // plugin folder name - should be the same name with plugin and should be uniq among the plugins
	generic_string _displayName;  // plugin description name
	Version _version;
	generic_string _homepage;
	generic_string _sourceUrl;
	generic_string _description;
	generic_string _author;
	generic_string _id;           // Plugin package ID: SHA-256 hash
	generic_string _repository;
	bool _isVisible = true;       // if false then it should not be displayed
	generic_string describe();
	PluginUpdateInfo() = default;
	PluginUpdateInfo(const generic_string& fullFilePath, const generic_string& fileName);
};

struct NppCurrentStatus {
	bool _isAdminMode = false;         // can launch gitup en Admin mode directly
	bool _isInProgramFiles = true; // true: install/update/remove on "Program files" (ADMIN MODE)
		// false: install/update/remove on NPP_INST or install on %APPDATA%, update/remove on %APPDATA% & NPP_INST (NORMAL MODE)
	bool _isAppDataPluginsAllowed = false;  // true: install on %APPDATA%, update / remove on %APPDATA% & "Program files" or NPP_INST

	generic_string _nppInstallPath;
	generic_string _appdataPath;
	// it should determinate :
	// 1. deployment location : %ProgramFile%   %appdata%   %other%
	// 2. gitup launch mode:    ADM             ADM         NOMAL
	bool shouldLaunchInAdmMode() { return _isInProgramFiles; }
};

enum COLUMN_TYPE { 
	COLUMN_PLUGIN, 
	COLUMN_VERSION 
};

enum SORT_TYPE { 
	DISPLAY_NAME_ALPHABET_ENCREASE, 
	DISPLAY_NAME_ALPHABET_DECREASE 
};

struct SortDisplayNameDecrease final {
	bool operator()(PluginUpdateInfo* l, PluginUpdateInfo* r) { return (l->_displayName.compare(r->_displayName) <= 0); }
};

class PluginViewList {
public:
	PluginViewList() = default;
	~PluginViewList();
	void pushBack(PluginUpdateInfo* pi);
	HWND getViewHwnd() { return _ui.getHSelf(); }
	void displayView(bool doShow) const { _ui.display(doShow); }
	std::vector<size_t> getCheckedIndexes() const { return _ui.getCheckedIndexes(); }
	std::vector<PluginUpdateInfo*> fromUiIndexesToPluginInfos(const std::vector<size_t>&) const;
	long getSelectedIndex() const { return _ui.getSelectedIndex(); }
	void setSelection(int index) const { _ui.setSelection(index); }
	void initView(HINSTANCE hInst, HWND parent) { _ui.init(hInst, parent); }
	void addColumn(const columnInfo & column2Add) { _ui.addColumn(column2Add); }
	void reSizeView(RECT & rc) { _ui.reSizeTo(rc); }
	void setViewStyleOption(int32_t extraStyle) { _ui.setStyleOption(extraStyle); }
	size_t nbItem() const { return _ui.nbItem(); }
	PluginUpdateInfo* getPluginInfoFromUiIndex(size_t index) const 
	{
		return reinterpret_cast<PluginUpdateInfo*>(_ui.getLParamFromIndex(static_cast<int>(index)));
	}
	PluginUpdateInfo* findPluginInfoFromFolderName(const generic_string& folderName, int& index) const;
	bool removeFromListIndex(size_t index2remove);
	bool hideFromListIndex(size_t index2Hide);
	bool removeFromFolderName(const generic_string& folderName);
	bool removeFromUiIndex(size_t index2remove);
	bool hideFromPluginInfoPtr(PluginUpdateInfo* pluginInfo2hide);
	bool restore(const generic_string& folderName);
	bool removeFromPluginInfoPtr(PluginUpdateInfo* pluginInfo2hide);
	void changeColumnName(COLUMN_TYPE index, const TCHAR * name2change);
private:
	std::vector<PluginUpdateInfo*> _list;
	ListView _ui;
	SORT_TYPE _sortType = DISPLAY_NAME_ALPHABET_ENCREASE;
};

enum LIST_TYPE { 
	AVAILABLE_LIST, 
	UPDATES_LIST, 
	INSTALLED_LIST 
};

class PluginsAdminDlg final : public StaticDialog {
public:
	PluginsAdminDlg();
	~PluginsAdminDlg() = default;
	void init(HINSTANCE hInst, HWND parent)     
	{
		Window::init(hInst, parent);
	}
	virtual void create(int dialogID, bool isRTL = false, bool msgDestParent = true);
	void doDialog(bool isRTL = false);
	bool isValide();
	void switchDialog(int indexToSwitch);
	void setPluginsManager(PluginsManager * pluginsManager) { _pPluginsManager = pluginsManager; }
	bool updateListAndLoadFromJson();
	void setAdminMode(bool isAdm) { _nppCurrentStatus._isAdminMode = isAdm; }
	bool installPlugins();
	bool updatePlugins();
	bool removePlugins();
	void changeTabName(LIST_TYPE index, const TCHAR * name2change);
	void changeColumnName(COLUMN_TYPE index, const TCHAR * name2change);
	generic_string getPluginListVerStr() const;
protected:
	virtual INT_PTR CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
private:
	generic_string _updaterDir;
	generic_string _updaterFullPath;
	generic_string _pluginListFullPath;
	TabBar _tab;
	PluginViewList _availableList; // A permanent list, once it's loaded (no removal - only hide or show)
	PluginViewList _updateList;    // A dynamical list, items are removable
	PluginViewList _installedList; // A dynamical list, items are removable
	PluginsManager * _pPluginsManager = nullptr;
	NppCurrentStatus _nppCurrentStatus;
	void collectNppCurrentStatusInfos();
	bool searchInPlugins(bool isNextMode) const;
	const bool _inNames = true;
	const bool _inDescs = false;
	bool isFoundInAvailableListFromIndex(int index, const generic_string& str2search, bool inWhichPart) const;
	long searchFromCurrentSel(const generic_string& str2search, bool inWhichPart, bool isNextMode) const;
	long searchInNamesFromCurrentSel(const generic_string& str2search, bool isNextMode) const 
	{
		return searchFromCurrentSel(str2search, _inNames, isNextMode);
	}
	long searchInDescsFromCurrentSel(const generic_string& str2search, bool isNextMode) const 
	{
		return searchFromCurrentSel(str2search, _inDescs, isNextMode);
	}
	bool loadFromPluginInfos();
	bool checkUpdates();

	enum Operation {
		pa_install = 0,
		pa_update = 1,
		pa_remove = 2
	};

	bool exitToInstallRemovePlugins(Operation op, const std::vector<PluginUpdateInfo*>& puis);
};
//
//#include "documentSnapshot.h"
class DocumentPeeker : public StaticDialog {
public:
	DocumentPeeker() = default;
	void init(HINSTANCE hInst, HWND hPere) 
	{
		Window::init(hInst, hPere);
	}
	void doDialog(POINT p, Buffer * buf, ScintillaEditView & scintSource);
	void syncDisplay(Buffer * buf, ScintillaEditView & scintSource);
	void setParent(HWND parent2set) { _hParent = parent2set; }
	void scrollSnapshotWith(const MapPosition & mapPos, int textZoneWidth);
	void saveCurrentSnapshot(ScintillaEditView & editView);
protected:
	virtual INT_PTR CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
	void goTo(POINT p);
private:
	ScintillaEditView * _pPeekerView = nullptr;
};
//
//#include "Notepad_plus.h"
#define MENU    0x01
#define TOOLBAR 0x02

enum FileTransferMode {
	TransferClone           = 0x01,
	TransferMove            = 0x02
};

enum WindowStatus {     //bitwise mask
	WindowMainActive        = 0x01,
	WindowSubActive         = 0x02,
	WindowBothActive        = 0x03, //little helper shortcut
	WindowUserActive        = 0x04,
	WindowMask                      = 0x07
};

enum trimOp {
	lineHeader = 0,
	lineTail = 1,
	lineEol = 2
};

enum spaceTab {
	tab2Space = 0,
	space2TabLeading = 1,
	space2TabAll = 2
};

struct VisibleGUIConf final {
	DWORD_PTR _preStyle = (WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN);
	bool _isPostIt = false;
	bool _isFullScreen = false;
	bool _isDistractionFree = false;
	bool _isMenuShown = true; //Used by postit & fullscreen
	//used by postit
	bool _isTabbarShown = true;
	bool _isAlwaysOnTop = false;
	bool _isStatusbarShown = true;
	bool _was2ViewModeOn = false; // used by distractionFree
	WINDOWPLACEMENT _winPlace = {0}; // used by fullscreen
	std::vector<DockingCont*> _pVisibleDockingContainers;
};

struct QuoteParams {
	enum Speed { 
		slow = 0, 
		rapid, 
		speedOfLight 
	};
	QuoteParams() 
	{
	}
	QuoteParams(const wchar_t* quoter, Speed speed, bool shouldBeTrolling, int encoding, LangType lang, const wchar_t* quote) :
		_quoter(quoter), _speed(speed), _shouldBeTrolling(shouldBeTrolling), _encoding(encoding), _lang(lang), _quote(quote) 
	{
	}
	void reset() 
	{
		_quoter = nullptr;
		_speed = rapid;
		_shouldBeTrolling = false;
		_encoding = SC_CP_UTF8;
		_lang = L_TEXT;
		_quote = nullptr;
	}
	const wchar_t* _quoter = nullptr;
	Speed _speed = rapid;
	bool _shouldBeTrolling = false;
	int _encoding = SC_CP_UTF8;
	LangType _lang = L_TEXT;
	const wchar_t* _quote = nullptr;
};

class Notepad_plus final {
	friend class Notepad_plus_Window;
	friend class FileManager;
public:
	Notepad_plus();
	~Notepad_plus();
	LRESULT init(HWND hwnd);
	LRESULT process(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
	void killAllChildren();
	enum comment_mode {
		cm_comment, 
		cm_uncomment, 
		cm_toggle
	};
	void setTitle();
	void getTaskListInfo(TaskListInfo * tli);
	// For filtering the modeless Dialog message

	//! \name File Operations
	//@{
	//The doXXX functions apply to a single buffer and dont need to worry about views, with the excpetion of
	// doClose, since closing one view doesnt have to mean the document is gone
	BufferID doOpen(const generic_string& fileName, bool isRecursive = false, bool isReadOnly = false, int encoding = -1, const TCHAR * backupFileName = NULL, FILETIME fileNameTimestamp = {});
	bool doReload(BufferID id, bool alert = true);
	bool doSave(BufferID, const TCHAR * filename, bool isSaveCopy = false);
	void doClose(BufferID, int whichOne, bool doDeleteBackup = false);
	//bool doDelete(const TCHAR *fileName) const {return ::DeleteFile(fileName) != 0;};
	void fileOpen();
	void fileNew();
	bool fileReload();
	bool fileClose(BufferID id = BUFFER_INVALID, int curView = -1); //use curView to override view to close from
	bool fileCloseAll(bool doDeleteBackup, bool isSnapshotMode = false);
	bool fileCloseAllButCurrent();
	bool fileCloseAllGiven(const std::vector<int>& krvecBufferIndexes);
	bool fileCloseAllToLeft();
	bool fileCloseAllToRight();
	bool fileCloseAllUnchanged();
	bool fileSave(BufferID id = BUFFER_INVALID);
	bool fileSaveAllConfirm();
	bool fileSaveAll();
	bool fileSaveSpecific(const generic_string& fileNameToSave);
	bool fileSaveAs(BufferID id = BUFFER_INVALID, bool isSaveCopy = false);
	bool fileDelete(BufferID id = BUFFER_INVALID);
	bool fileRename(BufferID id = BUFFER_INVALID);
	bool switchToFile(BufferID buffer);                     //find buffer in active view then in other view.
	//@}
	bool isFileSession(const TCHAR * filename);
	bool isFileWorkspace(const TCHAR * filename);
	void filePrint(bool showDialog);
	void saveScintillasZoom();
	bool saveGUIParams();
	bool saveProjectPanelsParams();
	bool saveFileBrowserParam();
	void saveDockingParams();
	void saveUserDefineLangs();
	void saveShortcuts();
	void saveSession(const Session & session);
	void saveCurrentSession();
	void saveFindHistory();
	void getCurrentOpenedFiles(Session& session, bool includUntitledDoc = false);
	bool fileLoadSession(const TCHAR* fn = nullptr);
	const TCHAR * fileSaveSession(size_t nbFile, TCHAR ** fileNames, const TCHAR * sessionFile2save, bool includeFileBrowser = false);
	const TCHAR * fileSaveSession(size_t nbFile = 0, TCHAR** fileNames = nullptr);
	void changeToolBarIcons();
	bool doBlockComment(comment_mode currCommentMode);
	bool doStreamComment();
	bool undoStreamComment(bool tryBlockComment = true);
	bool addCurrentMacro();
	void macroPlayback(Macro);
	void loadLastSession();
	bool loadSession(Session & session, bool isSnapshotMode = false, bool shouldLoadFileBrowser = false);
	void prepareBufferChangedDialog(Buffer * buffer);
	void notifyBufferChanged(Buffer * buffer, int mask);
	bool findInFinderFiles(FindersInfo * findInFolderInfo);
	bool createFilelistForFiles(std::vector<generic_string> & fileNames);
	bool createFilelistForProjects(std::vector<generic_string> & fileNames);
	bool findInFiles();
	bool findInProjects();
	bool findInFilelist(std::vector<generic_string> & fileList);
	bool replaceInFiles();
	bool replaceInProjects();
	bool replaceInFilelist(std::vector<generic_string> & fileList);
	void setFindReplaceFolderFilter(const TCHAR * dir, const TCHAR * filters);
	std::vector<generic_string> addNppComponents(const TCHAR * destDir, const TCHAR * extFilterName, const TCHAR * extFilter);
	std::vector<generic_string> addNppPlugins(const TCHAR * extFilterName, const TCHAR * extFilter);
	int getHtmlXmlEncoding(const TCHAR * fileName) const;
	HACCEL getAccTable() const { return _accelerator.getAccTable(); }
	bool emergency(const generic_string& emergencySavedDir);
	Buffer* getCurrentBuffer() { return _pEditView->getCurrentBuffer(); }
	void launchDocumentBackupTask();
	int getQuoteIndexFrom(const wchar_t* quoter) const;
	void showQuoteFromIndex(int index) const;
	void showQuote(const QuoteParams* quote) const;
	generic_string getPluginListVerStr() const { return _pluginsAdminDlg.getPluginListVerStr(); }
	void minimizeDialogs();
	void restoreMinimizeDialogs();
	void refreshDarkMode(bool resetStyle = false);
private:
	Notepad_plus_Window* _pPublicInterface = nullptr;
	Window* _pMainWindow = nullptr;
	DockingManager _dockingManager;
	std::vector<int> _internalFuncIDs;
	AutoCompletion _autoCompleteMain;
	AutoCompletion _autoCompleteSub; // each Scintilla has its own autoComplete
	SmartHighlighter _smartHighlighter;
	NativeLangSpeaker _nativeLangSpeaker;
	DocTabView _mainDocTab;
	DocTabView _subDocTab;
	DocTabView* _pDocTab = nullptr;
	DocTabView* _pNonDocTab = nullptr;
	ScintillaEditView _subEditView;
	ScintillaEditView _mainEditView;
	ScintillaEditView _invisibleEditView; // for searches
	ScintillaEditView _fileEditView;      // for FileManager
	ScintillaEditView* _pEditView = nullptr;
	ScintillaEditView* _pNonEditView = nullptr;
	SplitterContainer* _pMainSplitter = nullptr;
	SplitterContainer _subSplitter;
	ContextMenu _tabPopupMenu;
	ContextMenu _tabPopupDropMenu;
	ContextMenu _fileSwitcherMultiFilePopupMenu;
	ToolBar _toolBar;
	IconList _docTabIconList;
	IconList _docTabIconListAlt;
	IconList _docTabIconListDarkMode;
	StatusBar _statusBar;
	ReBar _rebarTop;
	ReBar _rebarBottom;
	// Dialog
	FindReplaceDlg _findReplaceDlg;
	FindInFinderDlg _findInFinderDlg;
	FindIncrementDlg _incrementFindDlg;
	AboutDlg _aboutDlg;
	DebugInfoDlg _debugInfoDlg;
	RunDlg _runDlg;
	HashFromFilesDlg _md5FromFilesDlg;
	HashFromTextDlg _md5FromTextDlg;
	HashFromFilesDlg _sha2FromFilesDlg;
	HashFromTextDlg _sha2FromTextDlg;
	GoToLineDlg _goToLineDlg;
	ColumnEditorDlg _colEditorDlg;
	WordStyleDlg _configStyleDlg;
	PreferenceDlg _preference;
	FindCharsInRangeDlg _findCharsInRangeDlg;
	PluginsAdminDlg _pluginsAdminDlg;
	DocumentPeeker _documentPeeker;
	std::vector<HWND> _hModelessDlgs; // a handle list of all the Notepad++ dialogs
	LastRecentFileList _lastRecentFileList;
	WindowsMenu _windowsMenu;
	HMENU _mainMenuHandle = NULL;
	// For FullScreen/PostIt/DistractionFree features
	VisibleGUIConf _beforeSpecialView;
	void fullScreenToggle();
	void postItToggle();
	void distractionFreeToggle();

	// Keystroke macro recording and playback
	Macro _macro;
	RunMacroDlg _runMacroDlg;
	ShortcutMapper* _pShortcutMapper = nullptr; // For conflict detection when saving Macros or RunCommands
	Sci_CharacterRange _prevSelectedRange; //For Dynamic selection highlight
	//
	// Synchronized Scolling
	//
	struct SyncInfo final {
		int _line = 0;
		int _column = 0;
		bool _isSynScollV = false;
		bool _isSynScollH = false;
		bool doSync() const { return (_isSynScollV || _isSynScollH); }
	} _syncInfo;
	trayIconControler* _pTrayIco = nullptr;
	int _zoomOriginalValue = 0;
	Accelerator _accelerator;
	ScintillaAccelerator _scintaccelerator;
	PluginsManager _pluginsManager;
	ButtonDlg _restoreButton;
	bool _toReduceTabBar = false;
	bool _sysMenuEntering = false;
	bool _isAttemptingCloseOnQuit = false; // make sure we don't recursively call doClose when closing the last file with -quitOnEmpty
	bool _recordingMacro = false;
	bool _playingBackMacro = false;
	bool _recordingSaved = false;
	bool _linkTriggered = true; // For hotspot
	bool _isFolding = false;    // For hotspot
	bool _isUDDocked = false;
	bool _isFileOpening = false;
	bool _isAdministrator = false;
	bool _isEndingSessionButNotReady = false; // If Windows 10 update needs to restart
	UCHAR _mainWindowStatus = 0; //For 2 views and user dialog if docked
	uchar Reserve[3]; // @alignment
	int   _activeView = MAIN_VIEW;
	// and Notepad++ has one (some) dirty document(s)
	// and "Enable session snapshot and periodic backup" is not enabled
	// then WM_ENDSESSION is send with wParam == FALSE
	// in this case this boolean is set true, so Notepad++ will quit and its current session will be saved
	ScintillaCtrls _scintillaCtrls4Plugins;
	std::vector<std::pair<int, int> > _hideLinesMarks;
	StyleArray _hotspotStyles;
	AnsiCharPanel* _pAnsiCharPanel = nullptr;
	ClipboardHistoryPanel* _pClipboardHistoryPanel = nullptr;
	VerticalFileSwitcher* _pDocumentListPanel = nullptr;
	ProjectPanel* _pProjectPanel_1 = nullptr;
	ProjectPanel* _pProjectPanel_2 = nullptr;
	ProjectPanel* _pProjectPanel_3 = nullptr;
	FileBrowser* _pFileBrowser = nullptr;
	DocumentMap* _pDocMap = nullptr;
	FunctionListPanel* _pFuncList = nullptr;
	std::vector<HWND> _sysTrayHiddenHwnd;

	BOOL notify(SCNotification * notification);
	void command(int id);
//Document management
	//User dialog docking
	void dockUserDlg();
	void undockUserDlg();
	//View visibility
	void showView(int whichOne);
	bool viewVisible(int whichOne);
	void hideView(int whichOne);
	void hideCurrentView();
	bool bothActive() const { return (_mainWindowStatus & WindowBothActive) == WindowBothActive; }
	bool reloadLang();
	bool loadStyles();
	int currentView() const { return _activeView; }
	int otherView() const { return (_activeView == MAIN_VIEW ? SUB_VIEW : MAIN_VIEW); }
	int otherFromView(int whichOne) const { return (whichOne == MAIN_VIEW ? SUB_VIEW : MAIN_VIEW); }
	bool canHideView(int whichOne); //true if view can safely be hidden (no open docs etc)
	bool isEmpty(); // true if we have 1 view with 1 clean, untitled doc
	int switchEditViewTo(int gid);  //activate other view (set focus etc)
	void docGotoAnotherEditView(FileTransferMode mode);     //TransferMode
	void docOpenInNewInstance(FileTransferMode mode, int x = 0, int y = 0);
	void loadBufferIntoView(BufferID id, int whichOne, bool dontClose = false); //Doesnt _activate_ the buffer
	bool removeBufferFromView(BufferID id, int whichOne); //Activates alternative of possible, or creates clean document if not clean already
	bool activateBuffer(BufferID id, int whichOne);                 //activate buffer in that view if found
	void notifyBufferActivated(BufferID bufid, int view);
	void performPostReload(int whichOne);
//END: Document management
	int doSaveOrNot(const TCHAR * fn, bool isMulti = false);
	int doReloadOrNot(const TCHAR * fn, bool dirty);
	int doCloseOrNot(const TCHAR * fn);
	int doDeleteOrNot(const TCHAR * fn);
	int doSaveAll();
	void enableMenu(int cmdID, bool doEnable) const;
	void enableCommand(int cmdID, bool doEnable, int which) const;
	void checkClipboard();
	void checkDocState();
	void checkUndoState();
	void checkMacroState();
	void checkSyncState();
	void setupColorSampleBitmapsOnMainMenuItems();
	void dropFiles(HDROP hdrop);
	void checkModifiedDocument(bool bCheckOnlyCurrentBuffer);
	void getMainClientRect(RECT & rc) const;
	void staticCheckMenuAndTB() const;
	void dynamicCheckMenuAndTB() const;
	void enableConvertMenuItems(EolType f) const;
	void checkUnicodeMenuItems() const;
	generic_string getLangDesc(LangType langType, bool getName = false);
	void setLangStatus(LangType langType);
	void setDisplayFormat(EolType f);
	void setUniModeText();
	void checkLangsMenu(int id) const;
	void setLanguage(LangType langType);
	LangType menuID2LangType(int cmdID);
	BOOL processIncrFindAccel(MSG * msg) const;
	BOOL processFindAccel(MSG * msg) const;
	void checkMenuItem(int itemID, bool willBeChecked) const 
	{
		::CheckMenuItem(_mainMenuHandle, itemID, MF_BYCOMMAND | (willBeChecked ? MF_CHECKED : MF_UNCHECKED));
	}
	bool isConditionExprLine(int lineNumber);
	int findMachedBracePos(size_t startPos, size_t endPos, char targetSymbol, char matchedSymbol);
	void maintainIndentation(TCHAR ch);
	void addHotSpot(ScintillaEditView* view = NULL);
	void bookmarkAdd(int lineno) const;
	void bookmarkDelete(int lineno) const;
	bool bookmarkPresent(int lineno) const;
	void bookmarkToggle(int lineno) const;
	void bookmarkNext(bool forwardScan);
	void bookmarkClearAll() const { _pEditView->execute(SCI_MARKERDELETEALL, MARK_BOOKMARK); }
	void copyMarkedLines();
	void cutMarkedLines();
	void deleteMarkedLines(bool isMarked);
	void pasteToMarkedLines();
	void deleteMarkedline(int ln);
	void inverseMarks();
	void replaceMarkedline(int ln, const TCHAR * str);
	generic_string getMarkedLine(int ln);
	void findMatchingBracePos(int & braceAtCaret, int & braceOpposite);
	bool braceMatch();
	void activateNextDoc(bool direction);
	void activateDoc(size_t pos);
	void updateStatusBar();
	size_t getSelectedCharNumber(UniMode);
	size_t getCurrentDocCharCount(UniMode u);
	size_t getSelectedAreas();
	size_t getSelectedBytes();
	bool isFormatUnicode(UniMode);
	int getBOMSize(UniMode);
	void showAutoComp();
	void autoCompFromCurrentFile(bool autoInsert = true);
	void showFunctionComp();
	void showPathCompletion();
	//void changeStyleCtrlsLang(HWND hDlg, int *idArray, const char **translatedText);
	void setCodePageForInvisibleView(Buffer const* pBuffer);
	bool replaceInOpenedFiles();
	bool findInOpenedFiles();
	bool findInCurrentFile(bool isEntireDoc);
	void getMatchedFileNames(const TCHAR * dir, const std::vector<generic_string> & patterns, std::vector<generic_string> & fileNames, bool isRecursive, bool isInHiddenDir);
	void doSynScorll(HWND hW);
	void setWorkingDir(const TCHAR * dir);
	bool str2Cliboard(const generic_string & str2cpy);
	bool getIntegralDockingData(tTbData & dockData, int & iCont, bool & isVisible);
	int getLangFromMenuName(const TCHAR * langName);
	generic_string getLangFromMenu(const Buffer * buf);
	generic_string exts2Filters(const generic_string& exts, int maxExtsLen = -1) const; // maxExtsLen default value
		// -1 makes no limit of whole exts length
	int setFileOpenSaveDlgFilters(CustomFileDialog & fDlg, bool showAllExt, int langType = -1); // showAllExt should be true if it's
		// used for open file dialog - all set exts should be used for filtering files
	NppStyle * getStyleFromName(const TCHAR * styleName);
	bool dumpFiles(const TCHAR * outdir, const TCHAR * fileprefix = TEXT(""));      //helper func
	void drawTabbarColoursFromStylerArray();
	void drawDocumentMapColoursFromStylerArray();
	std::vector<generic_string> loadCommandlineParams(const TCHAR * commandLine, const CmdLineParams * pCmdParams) 
	{
		const CmdLineParamsDTO dto = CmdLineParamsDTO::FromCmdLineParams(*pCmdParams);
		return loadCommandlineParams(commandLine, &dto);
	}
	std::vector<generic_string> loadCommandlineParams(const TCHAR * commandLine, const CmdLineParamsDTO * pCmdParams);
	bool noOpenedDoc() const;
	bool goToPreviousIndicator(int indicID2Search, bool isWrap = true) const;
	bool goToNextIndicator(int indicID2Search, bool isWrap = true) const;
	int wordCount();
	void wsTabConvert(spaceTab whichWay);
	void doTrim(trimOp whichPart);
	void removeEmptyLine(bool isBlankContained);
	void removeDuplicateLines();
	void launchAnsiCharPanel();
	void launchClipboardHistoryPanel();
	void launchDocumentListPanel();
	void checkProjectMenuItem();
	void launchProjectPanel(int cmdID, ProjectPanel ** pProjPanel, int panelID);
	void launchDocMap();
	void launchFunctionList();
	void launchFileBrowser(const std::vector<generic_string> & folders, const generic_string& selectedItemPath, bool fromScratch = false);
	void showAllQuotes() const;
	static DWORD WINAPI threadTextPlayer(void * text2display);
	static DWORD WINAPI threadTextTroller(void * params);
	static int getRandomAction(int ranNum);
	static bool deleteBack(ScintillaEditView * pCurrentView, BufferID targetBufID);
	static bool deleteForward(ScintillaEditView * pCurrentView, BufferID targetBufID);
	static bool selectBack(ScintillaEditView * pCurrentView, BufferID targetBufID);

	static int getRandomNumber(int rangeMax = -1) 
	{
		const int randomNumber = rand();
		return (rangeMax == -1) ? randomNumber : (rand() % rangeMax);
	}
	static DWORD WINAPI backupDocument(void * params);
	static DWORD WINAPI monitorFileOnChange(void * params);
	struct MonitorInfo final {
		MonitorInfo(Buffer * buf, HWND nppHandle) : _buffer(buf), _nppHandle(nppHandle) 
		{
		}
		Buffer * _buffer = nullptr;
		HWND _nppHandle = nullptr;
	};
	void monitoringStartOrStopAndUpdateUI(Buffer* pBuf, bool isStarting);
	void updateCommandShortcuts();
};
//
//#include "BabyGrid.h"
//
// BABYGRID code is copyrighted (C) 20002 by David Hillard
// This code must retain this copyright message
// Printed BABYGRID message reference and tutorial available.
// email: mudcat@mis.net for more information.
//
#ifndef WM_MOUSEWHEEL
	#define WM_MOUSEWHEEL 0x020A
#endif //WM_MOUSEWHEEL

#define BGN_LBUTTONDOWN 0x0001
#define BGN_MOUSEMOVE   0x0002
#define BGN_OUTOFRANGE  0x0003
#define BGN_OWNERDRAW   0x0004
#define BGN_SELCHANGE   0x0005
#define BGN_ROWCHANGED  0x0006
#define BGN_COLCHANGED  0x0007
#define BGN_EDITBEGIN   0x0008
#define BGN_DELETECELL  0x0009
#define BGN_EDITEND     0x000A
#define BGN_F1          0x000B
#define BGN_F2          0x000C
#define BGN_F3          0x000D
#define BGN_F4          0x000E
#define BGN_F5          0x000F
#define BGN_F6          0x0010
#define BGN_F7          0x0011
#define BGN_F8          0x0012
#define BGN_F9          0x0013
#define BGN_F10         0x0014
#define BGN_F11         0x0015
#define BGN_F12         0x0016
#define BGN_GOTFOCUS    0x0017
#define BGN_LOSTFOCUS   0x0018
#define BGN_CELLCLICKED 0x0019
#define BGN_CELLDBCLICKED 0x001A
#define BGN_CELLRCLICKED 0x001B

#define BGM_PROTECTCELL BABYGRID_USER + 1
#define BGM_SETPROTECT  BABYGRID_USER + 2
#define BGM_SETCELLDATA BABYGRID_USER + 3
#define BGM_GETCELLDATA BABYGRID_USER + 4
#define BGM_CLEARGRID   BABYGRID_USER + 5
#define BGM_SETGRIDDIM  BABYGRID_USER + 6
#define BGM_DELETECELL  BABYGRID_USER + 7
#define BGM_SETCURSORPOS BABYGRID_USER + 8
#define BGM_AUTOROW     BABYGRID_USER + 9
#define BGM_GETOWNERDRAWITEM BABYGRID_USER + 10
#define BGM_SETCOLWIDTH BABYGRID_USER + 11
#define BGM_SETHEADERROWHEIGHT BABYGRID_USER + 12
#define BGM_GETTYPE     BABYGRID_USER + 13
#define BGM_GETPROTECTION BABYGRID_USER + 14
#define BGM_DRAWCURSOR  BABYGRID_USER + 15
#define BGM_SETROWHEIGHT BABYGRID_USER + 16
#define BGM_SETCURSORCOLOR BABYGRID_USER + 17
#define BGM_SETPROTECTCOLOR BABYGRID_USER + 18
#define BGM_SETUNPROTECTCOLOR BABYGRID_USER + 19
#define BGM_SETROWSNUMBERED BABYGRID_USER + 20
#define BGM_SETCOLSNUMBERED BABYGRID_USER + 21
#define BGM_SHOWHILIGHT BABYGRID_USER + 22
#define BGM_GETROWS BABYGRID_USER + 23
#define BGM_GETCOLS BABYGRID_USER + 24
#define BGM_NOTIFYROWCHANGED BABYGRID_USER + 25
#define BGM_NOTIFYCOLCHANGED BABYGRID_USER + 26
#define BGM_GETROW BABYGRID_USER + 27
#define BGM_GETCOL BABYGRID_USER + 28
#define BGM_PAINTGRID BABYGRID_USER + 29
#define BGM_GETCOLWIDTH BABYGRID_USER + 30
#define BGM_GETROWHEIGHT BABYGRID_USER + 31
#define BGM_GETHEADERROWHEIGHT BABYGRID_USER + 32
#define BGM_SETTITLEHEIGHT BABYGRID_USER + 33

#define BGM_SETHILIGHTCOLOR BABYGRID_USER + 34
#define BGM_SETHILIGHTTEXTCOLOR BABYGRID_USER + 35
#define BGM_SETEDITABLE BABYGRID_USER + 36
#define BGM_SETGRIDLINECOLOR BABYGRID_USER + 37
#define BGM_EXTENDLASTCOLUMN BABYGRID_USER + 38
#define BGM_SHOWINTEGRALROWS BABYGRID_USER + 39
#define BGM_SETELLIPSIS BABYGRID_USER + 40
#define BGM_SETCOLAUTOWIDTH BABYGRID_USER + 41
#define BGM_SETALLOWCOLRESIZE BABYGRID_USER + 42
#define BGM_SETTITLEFONT BABYGRID_USER + 43
#define BGM_SETHEADINGFONT BABYGRID_USER + 44
#define BGM_GETHOMEROW BABYGRID_USER + 45
#define BGM_SETLASTVIEW BABYGRID_USER + 46
#define BGM_SETINITIALCONTENT BABYGRID_USER + 47
#define BGM_SETHILIGHTCOLOR_NOFOCUS BABYGRID_USER + 48
#define BGM_SETHILIGHTCOLOR_PROTECT BABYGRID_USER + 49
#define BGM_SETHILIGHTCOLOR_PROTECT_NOFOCUS BABYGRID_USER + 50

struct _BGCELL {
	int row = 0;
	int col = 0;
};

//function forward declarations
ATOM RegisterGridClass(HINSTANCE);
LRESULT CALLBACK GridProc(HWND, UINT, WPARAM, LPARAM);
void SetCell(_BGCELL * cell, int row, int col);
//
//#include "BabyGridWrapper.h"
class BabyGridWrapper : public Window {
public:
	BabyGridWrapper() = default;
	~BabyGridWrapper() = default;
	virtual void init(HINSTANCE hInst, HWND parent, int16_t id);
	virtual void destroy() 
	{
		::DestroyWindow(_hSelf);
	}
	void setLineColNumber(size_t nbRow, size_t nbCol) { ::SendMessage(_hSelf, BGM_SETGRIDDIM, nbRow, nbCol); }
	void setCursorColour(COLORREF coulour) { ::SendMessage(_hSelf, BGM_SETCURSORCOLOR, coulour, 0); }
	void hideCursor() { setCursorColour(RGB(0, 0, 0)); }
	void setColsNumbered(bool isNumbered = true) { ::SendMessage(_hSelf, BGM_SETCOLSNUMBERED, isNumbered ? TRUE : FALSE, 0); }
	void setText(size_t row, size_t col, const TCHAR * text) 
	{
		_BGCELL cell;
		cell.row = int(row);
		cell.col = int(col);
		::SendMessage(_hSelf, BGM_SETCELLDATA, reinterpret_cast<WPARAM>(&cell), reinterpret_cast<LPARAM>(text));
	}
	void makeColAutoWidth(bool autoWidth = true) { ::SendMessage(_hSelf, BGM_SETCOLAUTOWIDTH, autoWidth ? TRUE : FALSE, 0); }
	int getSelectedRow() { return (int)::SendMessage(_hSelf, BGM_GETROW, 0, 0); }
	void deleteCell(int row, int col) 
	{
		_BGCELL cell;
		cell.row = row;
		cell.col = col;
		::SendMessage(_hSelf, BGM_DELETECELL, reinterpret_cast<WPARAM>(&cell), 0);
	}
	void setColWidth(uint col, uint width) { ::SendMessage(_hSelf, BGM_SETCOLWIDTH, col, width); }
	void clear() { ::SendMessage(_hSelf, BGM_CLEARGRID, 0, 0); }
	int getNumberRows() const { return (int)::SendMessage(_hSelf, BGM_GETROWS, 0, 0); }
	int getHomeRow() const { return (int)::SendMessage(_hSelf, BGM_GETHOMEROW, 0, 0); }
	void setLastView(const size_t homeRow, const size_t cursorRow) const { ::SendMessage(_hSelf, BGM_SETLASTVIEW, homeRow, cursorRow); }
	void updateView() const { ::SendMessage(_hSelf, WM_PAINT, 0, 0); }
	void setHighlightColorNoFocus(const COLORREF color) const { ::SendMessage(_hSelf, BGM_SETHILIGHTCOLOR_NOFOCUS, color, 0); }
	void setProtectColor(const COLORREF color) const { ::SendMessage(_hSelf, BGM_SETPROTECTCOLOR, color, 0); }
	void setHighlightColorProtect(const COLORREF color) const { ::SendMessage(_hSelf, BGM_SETHILIGHTCOLOR_PROTECT, color, 0); }
	void setHighlightColorProtectNoFocus(const COLORREF color) const { ::SendMessage(_hSelf, BGM_SETHILIGHTCOLOR_PROTECT_NOFOCUS, color, 0); }
	bool setMarker(const bool isMarker) const 
	{
		::SendMessage(_hSelf, BGM_SETPROTECT, isMarker, 0);
		return isMarker;
	}
	void setAutoRow(const bool isAutoRow) const { ::SendMessage(_hSelf, BGM_AUTOROW, isAutoRow, 0); }
	void setInitialContent(const bool isInitialContent) const { ::SendMessage(_hSelf, BGM_SETINITIALCONTENT, isInitialContent, 0); }
	void setHeaderFont(const HFONT & hFont) const { ::SendMessage(_hSelf, BGM_SETHEADINGFONT, reinterpret_cast<WPARAM>(hFont), 0); }
	void setRowFont(const HFONT & hFont) const { ::SendMessage(_hSelf, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), 0); }
	void setHeaderHeight(const size_t headerHeight) const { ::SendMessage(_hSelf, BGM_SETHEADERROWHEIGHT, headerHeight, 0); }
	void setRowHeight(const size_t rowHeight) const { ::SendMessage(_hSelf, BGM_SETROWHEIGHT, rowHeight, 0); }
private:
	static bool _isRegistered;
};
//
//#include "ShortcutMapper.h"
enum GridState {
	STATE_MENU, 
	STATE_MACRO, 
	STATE_USER, 
	STATE_PLUGIN, 
	STATE_SCINTILLA
};

class ShortcutMapper : public StaticDialog {
public:
	ShortcutMapper() : _currentState(STATE_MENU), StaticDialog() 
	{
		_shortcutFilter = TEXT("");
		_dialogInitDone = false;
	}
	~ShortcutMapper() = default;
	void init(HINSTANCE hInst, HWND parent, GridState initState = STATE_MENU) 
	{
		Window::init(hInst, parent);
		_currentState = initState;
	}
	void destroy() 
	{
	}
	void doDialog(bool isRTL = false);
	void getClientRect(RECT & rc) const;
	bool findKeyConflicts(__inout_opt generic_string * const keyConflictLocation, const KeyCombo & itemKeyCombo, const size_t & itemIndex) const;
	generic_string getTextFromCombo(HWND hCombo);
	bool isFilterValid(Shortcut);
	bool isFilterValid(PluginCmdShortcut sc);
protected:
	INT_PTR CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
private:
	BabyGridWrapper _babygrid;
	ContextMenu _rightClickMenu;
	GridState _currentState;
	HWND _hTabCtrl = nullptr;
	const static int _nbTab = 5;
	generic_string _tabNames[_nbTab];
	generic_string _shortcutFilter;
	std::vector<size_t> _shortcutIndex;
	//save/restore the last view
	std::vector<size_t> _lastHomeRow;
	std::vector<size_t> _lastCursorRow;
	generic_string _conflictInfoOk;
	generic_string _conflictInfoEditing;
	std::vector<HFONT> _hGridFonts;

	enum GridFonts : uint_fast8_t {
		GFONT_HEADER,
		GFONT_ROWS,
		MAX_GRID_FONTS
	};

	LONG _clientWidth = 0;
	LONG _clientHeight = 0;
	LONG _initClientWidth = 0;
	LONG _initClientHeight = 0;
	bool _dialogInitDone = false;

	void initTabs();
	void initBabyGrid();
	void fillOutBabyGrid();
	generic_string getTabString(size_t i) const;
	bool isConflict(const KeyCombo & lhs, const KeyCombo & rhs) const
	{
		return ((lhs._isCtrl  == rhs._isCtrl) && (lhs._isAlt == rhs._isAlt) && (lhs._isShift == rhs._isShift) && (lhs._key == rhs._key));
	}
};
//
//#include "EncodingMapper.h"
struct EncodingUnit {
	int _codePage = 0;
	const char * _aliasList = nullptr;
};

class EncodingMapper {
public:
	static EncodingMapper& getInstance() 
	{
		static EncodingMapper instance;
		return instance;
	}
	int getEncodingFromIndex(int index) const;
	int getIndexFromEncoding(int encoding) const;
	int getEncodingFromString(const char * encodingAlias) const;
private:
	EncodingMapper() = default;
	~EncodingMapper() = default;
	// No copy ctor and assignment
	EncodingMapper(const EncodingMapper&) = delete;
	EncodingMapper& operator = (const EncodingMapper&) = delete;
	// No move ctor and assignment
	EncodingMapper(EncodingMapper&&) = delete;
	EncodingMapper& operator = (EncodingMapper&&) = delete;
};
//
//#include "TreeView.h"
struct TreeStateNode {
	generic_string _label;
	generic_string _extraData;
	bool _isExpanded = false;
	bool _isSelected = false;
	std::vector<TreeStateNode> _children;
};

class TreeView : public Window {
public:
	TreeView() = default;
	virtual ~TreeView() = default;

	virtual void init(HINSTANCE hInst, HWND parent, int treeViewID);
	virtual void destroy();
	HTREEITEM addItem(const TCHAR * itemName, HTREEITEM hParentItem, int iImage, LPARAM lParam = NULL);
	bool setItemParam(HTREEITEM Item2Set, LPARAM param);
	LPARAM getItemParam(HTREEITEM Item2Get) const;
	generic_string getItemDisplayName(HTREEITEM Item2Set) const;
	HTREEITEM searchSubItemByName(const TCHAR * itemName, HTREEITEM hParentItem);
	void removeItem(HTREEITEM hTreeItem);
	void removeAllItems();
	bool renameItem(HTREEITEM Item2Set, const TCHAR * newName);
	void makeLabelEditable(bool toBeEnabled);
	HTREEITEM getChildFrom(HTREEITEM hTreeItem) const { return TreeView_GetChild(_hSelf, hTreeItem); }
	HTREEITEM getSelection() const { return TreeView_GetSelection(_hSelf); }
	bool selectItem(HTREEITEM hTreeItem2Select) const { return TreeView_SelectItem(_hSelf, hTreeItem2Select) == TRUE; }
	HTREEITEM getRoot() const { return TreeView_GetRoot(_hSelf); }
	HTREEITEM getParent(HTREEITEM hItem) const { return TreeView_GetParent(_hSelf, hItem); }
	HTREEITEM getNextSibling(HTREEITEM hItem) const { return TreeView_GetNextSibling(_hSelf, hItem); }
	HTREEITEM getPrevSibling(HTREEITEM hItem) const { return TreeView_GetPrevSibling(_hSelf, hItem); }
	void expand(HTREEITEM hItem) const { TreeView_Expand(_hSelf, hItem, TVE_EXPAND); }
	void fold(HTREEITEM hItem) const { TreeView_Expand(_hSelf, hItem, TVE_COLLAPSE); }
	void foldExpandRecursively(HTREEITEM hItem, bool isFold) const;
	void foldExpandAll(bool isFold) const;
	void foldAll() const { foldExpandAll(true); }
	void expandAll() const { foldExpandAll(false); }
	void toggleExpandCollapse(HTREEITEM hItem) const { TreeView_Expand(_hSelf, hItem, TVE_TOGGLE); }
	void setItemImage(HTREEITEM hTreeItem, int iImage, int iSelectedImage);
	// Drag and Drop operations
	void beginDrag(NMTREEVIEW* tv);
	void dragItem(HWND parentHandle, int x, int y);
	bool isDragging() const { return _isItemDragged; }
	bool dropItem();
	void addCanNotDropInList(int val2set) { _canNotDropInList.push_back(val2set); }
	void addCanNotDragOutList(int val2set) { _canNotDragOutList.push_back(val2set); }
	bool moveDown(HTREEITEM itemToMove);
	bool moveUp(HTREEITEM itemToMove);
	bool swapTreeViewItem(HTREEITEM itemGoDown, HTREEITEM itemGoUp);
	bool restoreFoldingStateFrom(const TreeStateNode & treeState2Compare, HTREEITEM treeviewNode);
	bool retrieveFoldingStateTo(TreeStateNode & treeState2Construct, HTREEITEM treeviewNode);
	bool searchLeafAndBuildTree(TreeView & tree2Build, const generic_string & text2Search, int index2Search);
	void sort(HTREEITEM hTreeItem, bool isRecusive);
	void customSorting(HTREEITEM hTreeItem, PFNTVCOMPARE sortingCallbackFunc, LPARAM lParam, bool isRecursive);
protected:
	WNDPROC _defaultProc = nullptr;
	LRESULT runProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK staticProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam) 
	{
		return (((TreeView*)(::GetWindowLongPtr(hwnd, GWLP_USERDATA)))->runProc(hwnd, Message, wParam, lParam));
	}
	void cleanSubEntries(HTREEITEM hTreeItem);
	void dupTree(HTREEITEM hTree2Dup, HTREEITEM hParentItem);
	bool searchLeafRecusivelyAndBuildTree(HTREEITEM tree2Build, const generic_string & text2Search, int index2Search, HTREEITEM tree2Search);
	// Drag and Drop operations
	HTREEITEM _draggedItem = nullptr;
	HIMAGELIST _draggedImageList = nullptr;
	bool _isItemDragged = false;
	std::vector<int> _canNotDragOutList;
	std::vector<int> _canNotDropInList;
	bool canBeDropped(HTREEITEM draggedItem, HTREEITEM targetItem);
	void moveTreeViewItem(HTREEITEM draggedItem, HTREEITEM targetItem);
	bool isParent(HTREEITEM targetItem, HTREEITEM draggedItem);
	bool isDescendant(HTREEITEM targetItem, HTREEITEM draggedItem);
	bool canDragOut(HTREEITEM targetItem);
	bool canDropIn(HTREEITEM targetItem);
};
//
//#include "fileBrowser.h"
#define FB_PANELTITLE         TEXT("Folder as Workspace")
#define FB_ADDROOT            TEXT("Add")
#define FB_REMOVEALLROOTS     TEXT("Remove All")
#define FB_REMOVEROOTFOLDER   TEXT("Remove")
#define FB_COPYPATH           TEXT("Copy path")
#define FB_COPYFILENAME       TEXT("Copy file name")
#define FB_FINDINFILES        TEXT("Find in Files...")
#define FB_EXPLORERHERE       TEXT("Explorer here")
#define FB_CMDHERE            TEXT("CMD here")
#define FB_OPENINNPP          TEXT("Open")
#define FB_SHELLEXECUTE       TEXT("Run by system")
#define FOLDERASWORKSPACE_NODE "FolderAsWorkspace"

class FileInfo final {
	friend class FileBrowser;
	friend class FolderInfo;
public:
	FileInfo() = delete; // constructor by default is forbidden
	FileInfo(const generic_string & name, FolderInfo * parent) : _name(name), _parent(parent) 
	{
	}
	generic_string getName() const { return _name; }
	void setName(generic_string name) { _name = name; }
private:
	FolderInfo * _parent = nullptr;
	generic_string _name;
};

class FolderInfo final {
	friend class FileBrowser;
	friend class FolderUpdater;
public:
	FolderInfo() = delete; // constructor by default is forbidden
	FolderInfo(const generic_string & name, FolderInfo * parent) : _name(name), _parent(parent) 
	{
	}
	void setRootPath(const generic_string& rootPath) { _rootPath = rootPath; }
	generic_string getRootPath() const { return _rootPath; }
	void setName(const generic_string& name) { _name = name; }
	generic_string getName() const { return _name; }
	void addFile(const generic_string& fn) { _files.push_back(FileInfo(fn, this)); }
	void addSubFolder(FolderInfo subDirectoryStructure) { _subFolders.push_back(subDirectoryStructure); }
	bool addToStructure(generic_string & fullpath, std::vector<generic_string> linarPathArray);
	bool removeFromStructure(std::vector<generic_string> linarPathArray);
	bool renameInStructure(std::vector<generic_string> linarPathArrayFrom, std::vector<generic_string> linarPathArrayTo);
private:
	std::vector<FolderInfo> _subFolders;
	std::vector<FileInfo> _files;
	FolderInfo* _parent = nullptr;
	generic_string _name;
	generic_string _rootPath; // set only for root folder; empty for normal folder
};

enum BrowserNodeType {
	browserNodeType_root = 0, 
	browserNodeType_folder = 2, 
	browserNodeType_file = 3
};

class FolderUpdater {
	friend class FileBrowser;
public:
	FolderUpdater(const FolderInfo& fi, FileBrowser * pFileBrowser) : _rootFolder(fi), _pFileBrowser(pFileBrowser) 
	{
	}
	~FolderUpdater() = default;
	void startWatcher();
	void stopWatcher();
private:
	FolderInfo _rootFolder;
	FileBrowser* _pFileBrowser = nullptr;
	HANDLE _watchThreadHandle = nullptr;
	HANDLE _EventHandle = nullptr;
	static DWORD WINAPI watching(void * param);
	static void processChange(DWORD dwAction, std::vector<generic_string> filesToChange, FolderUpdater* thisFolderUpdater);
};

struct SortingData4lParam {
	generic_string _rootPath; // Only for the root. It should be empty if it's not root
	generic_string _label;    // TreeView item label
	bool _isFolder = false;   // if it's not a folder, then it's a file
	SortingData4lParam(generic_string rootPath, generic_string label, bool isFolder) : _rootPath(rootPath), _label(label), _isFolder(isFolder) 
	{
	}
};

class FileBrowser : public DockingDlgInterface {
public:
	FileBrowser() : DockingDlgInterface(IDD_FILEBROWSER) 
	{
	}
	~FileBrowser();
	void init(HINSTANCE hInst, HWND hPere) { DockingDlgInterface::init(hInst, hPere); }
	virtual void display(bool toShow = true) const { DockingDlgInterface::display(toShow); }
	void setParent(HWND parent2set) { _hParent = parent2set; }
	virtual void setBackgroundColor(COLORREF bgColour) { TreeView_SetBkColor(_treeView.getHSelf(), bgColour); }
	virtual void setForegroundColor(COLORREF fgColour) { TreeView_SetTextColor(_treeView.getHSelf(), fgColour); }
	generic_string getNodePath(HTREEITEM node) const;
	generic_string getNodeName(HTREEITEM node) const;
	void addRootFolder(generic_string rootFolderPath);
	HTREEITEM getRootFromFullPath(const generic_string & rootPath) const;
	HTREEITEM findChildNodeFromName(HTREEITEM parent, const generic_string& label) const;
	HTREEITEM findInTree(const generic_string& rootPath, HTREEITEM node, std::vector<generic_string> linarPathArray) const;
	void deleteAllFromTree() { popupMenuCmd(IDM_FILEBROWSER_REMOVEALLROOTS); }
	bool renameInTree(const generic_string& rootPath, HTREEITEM node, const std::vector<generic_string>& linarPathArrayFrom, const generic_string & renameTo);
	std::vector<generic_string> getRoots() const;
	generic_string getSelectedItemPath() const;
	bool selectItemFromPath(const generic_string& itemPath) const;
protected:
	HWND _hToolbarMenu = nullptr;
	TreeView _treeView;
	HIMAGELIST _hImaLst = nullptr;
	HMENU _hGlobalMenu = NULL;
	HMENU _hRootMenu = NULL;
	HMENU _hFolderMenu = NULL;
	HMENU _hFileMenu = NULL;
	std::vector<FolderUpdater *> _folderUpdaters;
	generic_string _selectedNodeFullPath; // this member is used only for PostMessage call
	std::vector<SortingData4lParam*> sortingDataArray;
	generic_string _expandAllFolders = TEXT("Expand all folders");
	generic_string _collapseAllFolders = TEXT("Collapse all folders");
	generic_string _locateCurrentFile = TEXT("Locate current file");
	void initPopupMenus();
	void destroyMenus();
	BOOL setImageList(int root_open_id, int root_close_id, int open_node_id, int closed_node_id, int leaf_id);
	BrowserNodeType getNodeType(HTREEITEM hItem);
	void popupMenuCmd(int cmdID);
	bool selectCurrentEditingFile() const;

	struct FilesToChange {
		generic_string _commonPath; // Common path between all the files. _rootPath + _linarWithoutLastPathElement
		generic_string _rootPath;
		std::vector<generic_string> _linarWithoutLastPathElement;
		std::vector<generic_string> _files; // file/folder names
	};
	std::vector<FilesToChange> getFilesFromParam(LPARAM lParam) const;
	bool addToTree(FilesToChange & group, HTREEITEM node);
	bool deleteFromTree(FilesToChange & group);
	std::vector<HTREEITEM> findInTree(FilesToChange & group, HTREEITEM node) const;
	std::vector<HTREEITEM> findChildNodesFromNames(HTREEITEM parent, std::vector<generic_string> & labels) const;
	void removeNamesAlreadyInNode(HTREEITEM parent, std::vector<generic_string> & labels) const;
	virtual INT_PTR CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
	void notified(LPNMHDR notification);
	void showContextMenu(int x, int y);
	void openSelectFile();
	void getDirectoryStructure(const TCHAR * dir, const std::vector<generic_string> & patterns, FolderInfo & directoryStructure,
	    bool isRecursive, bool isInHiddenDir);
	HTREEITEM createFolderItemsFromDirStruct(HTREEITEM hParentItem, const FolderInfo & directoryStructure);
	static int CALLBACK categorySortFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);
};
//
//#include "documentMap.h"
#define DOCUMENTMAP_SCROLL        (WM_USER + 1)
#define DOCUMENTMAP_MOUSECLICKED  (WM_USER + 2)
#define DOCUMENTMAP_MOUSEWHEEL    (WM_USER + 3)

const TCHAR VIEWZONE_DOCUMENTMAP[64] = TEXT("Document map");
const bool moveDown = true;
const bool moveUp = false;

enum moveMode {
	perLine,
	perPage
};

class ViewZoneDlg : public StaticDialog {
public:
	ViewZoneDlg() : StaticDialog(), _viewZoneCanvas(NULL), _canvasDefaultProc(nullptr), _higherY(0), _lowerY(0) 
	{
	}
	enum class ViewZoneColorIndex {
		focus,
		frost
	};
	void doDialog();
	virtual void destroy() 
	{
	}
	void drawZone(long hY, long lY);
	int getViewerHeight() const { return (_lowerY - _higherY); }
	int getCurrentCenterPosY() const { return (_lowerY - _higherY)/2 + _higherY; }
	static void setColour(COLORREF colour2Set, ViewZoneColorIndex i);
protected:
	virtual INT_PTR CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK canvasStaticProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
	LRESULT CALLBACK canvas_runProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
	static COLORREF _focus;
	static COLORREF _frost;
	void drawPreviewZone(DRAWITEMSTRUCT * pdis);
private:
	HWND _viewZoneCanvas = nullptr;
	WNDPROC _canvasDefaultProc = nullptr;
	long _higherY = 0;
	long _lowerY = 0;
};

class DocumentMap : public DockingDlgInterface {
public:
	DocumentMap();
	void create(tTbData * data, bool isRTL = false);
	void init(HINSTANCE hInst, HWND hPere, ScintillaEditView ** ppEditView);
	virtual void display(bool toShow = true) const;
	virtual void redraw(bool forceUpdate = false) const;
	void setParent(HWND parent2set) { _hParent = parent2set; }
	void vzDlgDisplay(bool toShow = true) { _vzDlg.display(toShow); }
	void reloadMap();
	void showInMapTemporarily(Buffer * buf2show, ScintillaEditView * fromEditView);
	void wrapMap(const ScintillaEditView * editView = nullptr);
	void initWrapMap();
	void scrollMap();
	void scrollMap(bool direction, moveMode whichMode);
	void scrollMapWith(const MapPosition & mapPos);
	void doMove();
	void fold(size_t line, bool foldOrNot);
	void foldAll(bool mode);
	void setSyntaxHiliting();
	void changeTextDirection(bool isRTL);
	bool isTemporarilyShowing() const { return _isTemporarilyShowing; };
	void setTemporarilyShowing(bool tempShowing) { _isTemporarilyShowing = tempShowing; }
protected:
	virtual INT_PTR CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
	bool needToRecomputeWith(const ScintillaEditView * editView = nullptr);
private:
	ScintillaEditView** _ppEditView = nullptr;
	ScintillaEditView* _pMapView = nullptr;
	ViewZoneDlg _vzDlg;
	HWND _hwndScintilla;
	bool _isTemporarilyShowing = false;
	// for needToRecomputeWith function
	int _displayZoom = -1;
	int _displayWidth = 0;
	generic_string id4dockingCont = DM_NOFOCUSWHILECLICKINGCAPTION;
};
//
//#include "ReadDirectoryChanges.h"
// @sobolev #define _CRT_SECURE_NO_DEPRECATE
//#include "targetver.h"
// Including SDKDDKVer.h defines the highest available Windows platform.
// If you wish to build your application for a previous Windows platform, include WinSDKVer.h and
// set the _WIN32_WINNT macro to the platform you wish to support before including SDKDDKVer.h.
#include <sdkddkver.h>
//
#ifndef VC_EXTRALEAN
	#define VC_EXTRALEAN            // Exclude rarely-used stuff from Windows headers
#endif

using namespace std;

//#include "ThreadSafeQueue.h"
template <typename C> class CThreadSafeQueue : protected std::list<C> { // Copyright (c) 2010 James E Beveridge
protected:
	using Base = std::list<C>;
public:
	CThreadSafeQueue()
	{
		m_hEvent = ::CreateEvent(NULL/*no security attributes*/, FALSE/*auto reset*/, FALSE/*non-signalled*/, NULL/*anonymous*/);
	}
	~CThreadSafeQueue()
	{
		::CloseHandle(m_hEvent);
		m_hEvent = NULL;
	}
	void push(C& c)
	{
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			Base::push_back(c);
		}
		::SetEvent(m_hEvent);
	}
	bool pop(C& c)
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		if(Base::empty()) {
			return false;
		}
		c = Base::front();
		Base::pop_front();
		return true;
	}
	HANDLE GetWaitHandle() { return m_hEvent; }
protected:
	HANDLE m_hEvent = nullptr;
	std::mutex m_mutex;
};
//
typedef pair<DWORD, std::wstring> TDirectoryChangeNotification;

namespace ReadDirectoryChangesPrivate { // Copyright (c) 2010 James E Beveridge
	class CReadChangesServer;
}
//
/// <summary>
/// Track changes to filesystem directories and report them
/// to the caller via a thread-safe queue.
/// </summary>
/// <remarks>
/// <para>
/// This sample code is based on my blog entry titled, "Understanding ReadDirectoryChangesW"
///	http://qualapps.blogspot.com/2010/05/understanding-readdirectorychangesw.html
/// </para><para>
/// All functions in CReadDirectoryChangesServer run in
/// the context of the calling thread.
/// </para>
/// <example><code>
///     CReadDirectoryChanges changes;
///     changes.AddDirectory(_T("C:\\"), false, dwNotificationFlags);
///
///		const HANDLE handles[] = { hStopEvent, changes.GetWaitHandle() };
///
///		while(!bTerminate) {
///			::MsgWaitForMultipleObjectsEx(_countof(handles), handles, INFINITE, QS_ALLINPUT,
///				MWMO_INPUTAVAILABLE | MWMO_ALERTABLE);
///			switch (rc) {
///			case WAIT_OBJECT_0 + 0:
///				bTerminate = true;
///				break;
///			case WAIT_OBJECT_0 + 1:
///				// We've received a notification in the queue.
///				{
///					DWORD dwAction;
///					std::wstring wstrFilename;
///					while (changes.Pop(dwAction, wstrFilename))
///						wprintf(L"%s %s\n", ExplainAction(dwAction), wstrFilename);
///				}
///				break;
///			case WAIT_OBJECT_0 + _countof(handles):
///				// Get and dispatch message
///				break;
///			case WAIT_IO_COMPLETION:
///				// APC complete.No action needed.
///				break;
///			}
///		}
/// </code></example>
/// </remarks>
class CReadDirectoryChanges { // Copyright (c) 2010 James E Beveridge
public:
	CReadDirectoryChanges();
	~CReadDirectoryChanges();
	void Init();
	void Terminate();
	/// <summary>
	/// Add a new directory to be monitored.
	/// </summary>
	/// <param name="wszDirectory">Directory to monitor.</param>
	/// <param name="bWatchSubtree">True to also monitor subdirectories.</param>
	/// <param name="dwNotifyFilter">The types of file system events to monitor, such as
	// FILE_NOTIFY_CHANGE_ATTRIBUTES.</param>
	/// <param name="dwBufferSize">The size of the buffer used for overlapped I/O.</param>
	/// <remarks>
	/// <para>
	/// This function will make an APC call to the worker thread to issue a new
	/// ReadDirectoryChangesW call for the given directory with the given flags.
	/// </para>
	/// </remarks>
	void AddDirectory(LPCTSTR wszDirectory, BOOL bWatchSubtree, DWORD dwNotifyFilter, DWORD dwBufferSize = 16384);
	/// <summary>
	/// Return a handle for the Win32 Wait... functions that will be
	/// signaled when there is a queue entry.
	/// </summary>
	HANDLE GetWaitHandle() { return m_Notifications.GetWaitHandle(); }
	bool Pop(DWORD& dwAction, std::wstring& wstrFilename);
	// "Push" is for usage by ReadChangesRequest.  Not intended for external usage.
	void Push(DWORD dwAction, std::wstring& wstrFilename);
	uint GetThreadId() { return m_dwThreadId; }
protected:
	ReadDirectoryChangesPrivate::CReadChangesServer* m_pServer = nullptr;
	HANDLE m_hThread = nullptr;
	uint m_dwThreadId = 0;
	CThreadSafeQueue<TDirectoryChangeNotification> m_Notifications;
};
//
//#include "functionParser.h"

struct foundInfo final {
	generic_string _data;
	generic_string _data2;
	int _pos = -1;
	int _pos2 = -1;
};

class FunctionParser {
	friend class FunctionParsersManager;
public:
	FunctionParser(const TCHAR * id,
	    const TCHAR * displayName,
	    const TCHAR * commentExpr,
	    const generic_string& functionExpr,
	    const std::vector<generic_string>& functionNameExprArray,
	    const std::vector<generic_string>& classNameExprArray) :
		_id(id), _displayName(displayName), _commentExpr(commentExpr ? commentExpr : TEXT("")), _functionExpr(functionExpr),
		_functionNameExprArray(functionNameExprArray), _classNameExprArray(classNameExprArray)
	{
	}
	virtual void parse(std::vector<foundInfo> & foundInfos, size_t begin, size_t end,
	    ScintillaEditView ** ppEditView, generic_string classStructName = TEXT("")) = 0;
	void funcParse(std::vector<foundInfo> & foundInfos, size_t begin, size_t end, ScintillaEditView ** ppEditView,
	    generic_string classStructName = TEXT(""), const std::vector< std::pair<int, int> > * commentZones = NULL);
	bool isInZones(int pos2Test, const std::vector< std::pair<int, int> > & zones);
	virtual ~FunctionParser() = default;
protected:
	generic_string _id;
	generic_string _displayName;
	generic_string _commentExpr;
	generic_string _functionExpr;
	std::vector<generic_string> _functionNameExprArray;
	std::vector<generic_string> _classNameExprArray;
	void getCommentZones(std::vector< std::pair<int, int> > & commentZone, size_t begin, size_t end, ScintillaEditView ** ppEditView);
	void getInvertZones(std::vector< std::pair<int, int> > & destZones,
	    std::vector< std::pair<int, int> > & sourceZones, size_t begin, size_t end);
	generic_string parseSubLevel(size_t begin, size_t end, std::vector< generic_string > dataToSearch,
	    int & foundPos, ScintillaEditView ** ppEditView);
};

class FunctionZoneParser : public FunctionParser {
public:
	FunctionZoneParser() = delete;
	FunctionZoneParser(const TCHAR * id, const TCHAR * displayName, const TCHAR * commentExpr, const generic_string& rangeExpr,
	    const generic_string& openSymbole, const generic_string& closeSymbole, const std::vector<generic_string>& classNameExprArray,
	    const generic_string& functionExpr, const std::vector<generic_string>& functionNameExprArray) :
		FunctionParser(id, displayName, commentExpr, functionExpr, functionNameExprArray, classNameExprArray),
		_rangeExpr(rangeExpr), _openSymbole(openSymbole), _closeSymbole(closeSymbole) 
	{
	}
	void parse(std::vector<foundInfo> & foundInfos, size_t begin, size_t end, ScintillaEditView ** ppEditView, generic_string classStructName = TEXT(""));
protected:
	void classParse(std::vector<foundInfo> & foundInfos, std::vector< std::pair<int, int> > & scannedZones, const std::vector< std::pair<int, int> > & commentZones,
	    size_t begin, size_t end, ScintillaEditView ** ppEditView, generic_string classStructName = TEXT(""));
private:
	generic_string _rangeExpr;
	generic_string _openSymbole;
	generic_string _closeSymbole;
	size_t getBodyClosePos(size_t begin, const TCHAR * bodyOpenSymbol, const TCHAR * bodyCloseSymbol, const std::vector< std::pair<int,
	    int> > & commentZones, ScintillaEditView ** ppEditView);
};

class FunctionUnitParser : public FunctionParser {
public:
	FunctionUnitParser(const TCHAR * id, const TCHAR * displayName, const TCHAR * commentExpr,
	    const generic_string& mainExpr, const std::vector<generic_string>& functionNameExprArray,
	    const std::vector<generic_string>& classNameExprArray) : FunctionParser(id, displayName, commentExpr, mainExpr, functionNameExprArray, classNameExprArray)
	{
	}
	void parse(std::vector<foundInfo> & foundInfos, size_t begin, size_t end, ScintillaEditView ** ppEditView, generic_string classStructName = TEXT(""));
};

class FunctionMixParser : public FunctionZoneParser {
public:
	FunctionMixParser(const TCHAR * id,
	    const TCHAR * displayName,
	    const TCHAR * commentExpr,
	    const generic_string& rangeExpr,
	    const generic_string& openSymbole,
	    const generic_string& closeSymbole,
	    const std::vector<generic_string>& classNameExprArray,
	    const generic_string& functionExpr,
	    const std::vector<generic_string>& functionNameExprArray,
	    FunctionUnitParser * funcUnitPaser) :
		FunctionZoneParser(id, displayName, commentExpr, rangeExpr, openSymbole, closeSymbole,
		    classNameExprArray, functionExpr, functionNameExprArray), _funcUnitPaser(funcUnitPaser){
	};
	~FunctionMixParser()
	{
		delete _funcUnitPaser;
	}
	void parse(std::vector<foundInfo> & foundInfos, size_t begin, size_t end, ScintillaEditView ** ppEditView, generic_string classStructName = TEXT(""));
private:
	FunctionUnitParser* _funcUnitPaser = nullptr;
};

struct AssociationInfo final {
	int _id;
	int _langID;
	generic_string _ext;
	generic_string _userDefinedLangName;
	AssociationInfo(int id, int langID, const TCHAR * ext, const TCHAR * userDefinedLangName);
};

const int nbMaxUserDefined = 25;

struct ParserInfo {
	generic_string _id; // xml parser rule file name - if empty, then we use default name. Mandatory if _userDefinedLangName is not empty
	FunctionParser* _parser = nullptr;
	generic_string _userDefinedLangName;
	ParserInfo() 
	{
	}
	ParserInfo(const generic_string& id) : _id(id) 
	{
	}
	ParserInfo(const generic_string& id, const generic_string& userDefinedLangName) : _id(id),
		_userDefinedLangName(userDefinedLangName) 
	{
	}
	~ParserInfo() 
	{
		delete _parser;
	}
};

class FunctionParsersManager final {
public:
	~FunctionParsersManager();
	bool init(const generic_string& xmlPath, const generic_string& xmlInstalledPath, ScintillaEditView ** ppEditView);
	bool parse(std::vector<foundInfo> & foundInfos, const AssociationInfo & assoInfo);
private:
	ScintillaEditView ** _ppEditView = nullptr;
	generic_string _xmlDirPath; // The 1st place to load function list files. Usually it's "%APPDATA%\Notepad++\functionList\"
	generic_string _xmlDirInstalledPath; // Where Notepad++ is installed. The 2nd place to load function list files.
		// Usually it's "%PROGRAMFILES%\Notepad++\functionList\"
	ParserInfo* _parsers[L_EXTERNAL + nbMaxUserDefined] = {nullptr};
	int _currentUDIndex = L_EXTERNAL;
	bool getOverrideMapFromXmlTree(generic_string & xmlDirPath);
	bool loadFuncListFromXmlTree(generic_string & xmlDirPath, LangType lType, const generic_string& overrideId, int udlIndex = -1);
	bool getZonePaserParameters(TiXmlNode * classRangeParser,
	    generic_string &mainExprStr,
	    generic_string &openSymboleStr,
	    generic_string &closeSymboleStr,
	    std::vector<generic_string> &classNameExprArray,
	    generic_string &functionExprStr,
	    std::vector<generic_string> &functionNameExprArray);
	bool getUnitPaserParameters(TiXmlNode * functionParser,
	    generic_string &mainExprStr,
	    std::vector<generic_string> &functionNameExprArray,
	    std::vector<generic_string> &classNameExprArray);
	FunctionParser * getParser(const AssociationInfo & assoInfo);
};
//
//#include "Gripper.h"
// For the following #define see the comments at drawRectangle() definition. (jg)
#define USE_LOCKWINDOWUPDATE

// Used by getRectAndStyle() to draw the drag rectangle
static const WORD DotPattern[] = { 0x00aa, 0x0055, 0x00aa, 0x0055, 0x00aa, 0x0055, 0x00aa, 0x0055 };

class Gripper { // Copyright (C) 2006 Jens Lorenz <jens.plugin.npp@gmx.de>
public:
	Gripper();
	~Gripper();
	void init(HINSTANCE hInst, HWND hParent);
	void startGrip(DockingCont* pCont, DockingManager* pDockMgr);
protected:
	void create();
	static LRESULT CALLBACK staticWinProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
	LRESULT runProc(UINT Message, WPARAM wParam, LPARAM lParam);
	void onMove();
	void onButtonUp();
	void doTabReordering(POINT pt);
	void drawRectangle(const POINT* pPt);
	void getMousePoints(POINT* pt, POINT* ptPrev);
	void getMovingRect(POINT pt, RECT * rc);
	DockingCont * contHitTest(POINT pt);
	DockingCont * workHitTest(POINT pt, RECT * rcCont = NULL);
	void initTabInformation();
	void CalcRectToScreen(HWND hWnd, RECT * rc);
	void CalcRectToClient(HWND hWnd, RECT * rc);
	void ShrinkRcToSize(RECT * rc);
	void DoCalcGripperRect(RECT* rc, RECT rcCorr, POINT pt);
private:
	HINSTANCE _hInst; // Handle
	HWND _hParent;
	HWND _hSelf;
	tDockMgr _dockData; // data of container
	DockingManager * _pDockMgr;
	DockingCont * _pCont;
	POINT _ptOffset; // mouse offset in moving rectangle
	POINT _ptOld; // remembers old mouse point
	BOOL _bPtOldValid;
	RECT _rcPrev; // remember last drawn rectangle (jg)
	// for sorting tabs
	HWND _hTab;
	HWND _hTabSource;
	BOOL _startMovingFromTab;
	int _iItem;
	RECT _rcItem;
	TCITEM _tcItem;
	HDC _hdc;
	HBITMAP _hbm;
	HBRUSH _hbrush;
	static BOOL _isRegistered; // is class registered
};
//
//#include "CustomFileDialog.h"
//
// Customizable file dialog.
// It allows adding custom controls like checkbox to the dialog.
// This class loosely follows the interface of the FileDialog class.
// However, the implementation is different.
//
class CustomFileDialog {
public:
	explicit CustomFileDialog(HWND hwnd);
	~CustomFileDialog();
	void setTitle(const TCHAR* title);
	void setExtFilter(const TCHAR* text, const TCHAR* ext);
	void setExtFilter(const TCHAR* text, std::initializer_list<const TCHAR*> exts);
	void setDefExt(const TCHAR* ext);
	void setDefFileName(const TCHAR * fn);
	void setFolder(const TCHAR* folder);
	void setCheckbox(const TCHAR* text, bool isActive = true);
	void setExtIndex(int extTypeIndex);
	void enableFileTypeCheckbox(const generic_string& text, bool value);
	bool getFileTypeCheckboxValue() const;
	// Empty string is not a valid file name and may signal that the dialog was canceled.
	generic_string doSaveDlg();
	generic_string pickFolder();
	generic_string doOpenSingleFileDlg();
	std::vector<generic_string> doOpenMultiFilesDlg();
	bool getCheckboxState() const;
	bool isReadOnly() const;
private:
	class Impl;
	std::unique_ptr<Impl> _impl;
};
//
//#include "Notepad_plus_Window.h"
const int splitterSize = 8;

class Notepad_plus_Window : public Window {
public:
	static const TCHAR * P_CommandArgHelp;
	void init(HINSTANCE, HWND, const TCHAR * cmdLine, CmdLineParams * cmdLineParams);
	bool isDlgsMsg(MSG * msg) const;
	HACCEL getAccTable() const { return _notepad_plus_plus_core.getAccTable(); };
	bool emergency(const generic_string& emergencySavedDir) { return _notepad_plus_plus_core.emergency(emergencySavedDir); };
	bool isPrelaunch() const { return _isPrelaunch; };
	void setIsPrelaunch(bool val) { _isPrelaunch = val; };
	generic_string getPluginListVerStr() const { return _notepad_plus_plus_core.getPluginListVerStr(); };
	virtual void destroy() 
	{
		if(_hIconAbsent)
			::DestroyIcon(_hIconAbsent);
		::DestroyWindow(_hSelf);
	};
	static const TCHAR * getClassName() { return _className; };
	HICON getAbsentIcoHandle() { return _hIconAbsent; };
	static HWND gNppHWND;   //static handle to Notepad++ window, NULL if non-existant
private:
	Notepad_plus _notepad_plus_plus_core;
	static LRESULT CALLBACK Notepad_plus_Proc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
	LRESULT runProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
	static const TCHAR _className[32];
	bool _isPrelaunch = false;
	bool _disablePluginsManager = false;
	QuoteParams _quoteParams; // keep the availability of quote parameters for thread using
	std::wstring _userQuote; // keep the availability of this string for thread using
	HICON _hIconAbsent = nullptr;
};
//
//#include "Printer.h"
struct NPP_RangeToFormat {
	HDC hdc = nullptr;
	HDC hdcTarget = nullptr;
	RECT rc = { 0 };
	RECT rcPage = { 0 };
	Sci_CharacterRange chrg = { 0 };
};

class Printer {
public:
	Printer() = default;
	void init(HINSTANCE hInst, HWND hwnd, ScintillaEditView * pSEView, bool showDialog, int startPos, int endPos, bool isRTL = false);
	size_t doPrint();
	size_t doPrint(bool justDoIt);
private:
	PRINTDLG _pdlg;
	ScintillaEditView * _pSEView = nullptr;
	size_t _startPos = 0;
	size_t _endPos = 0;
	size_t _nbPageTotal = 0;
	bool _isRTL = false;
};
//
//#include "FileNameStringSplitter.h"
typedef std::vector<generic_string> stringVector;

class FileNameStringSplitter {
public:
	FileNameStringSplitter(const TCHAR * fileNameStr);
	const stringVector& getFileNames() const { return _fileNames; }
	const TCHAR * getFileName(size_t index) const { return (index >= _fileNames.size()) ? NULL : _fileNames[index].c_str(); }
	int size() const { return int(_fileNames.size()); }
private:
	stringVector _fileNames;
};
//
//#include "TaskList.h"
#ifndef WM_MOUSEWHEEL
	#define WM_MOUSEWHEEL 0x020A
#endif //WM_MOUSEWHEEL

class TaskList : public Window {
public:
	TaskList();
	virtual ~TaskList() = default;
	void init(HINSTANCE hInst, HWND hwnd, HIMAGELIST hImaLst, int nbItem, int index2set);
	virtual void destroy();
	void setFont(const TCHAR * fontName, int fontSize);
	RECT adjustSize();
	int getCurrentIndex() const { return _currentIndex; }
	int updateCurrentIndex();
	HIMAGELIST getImgLst() const { return ListView_GetImageList(_hSelf, LVSIL_SMALL); }
	HFONT GetFontSelected() { return _hFontSelected; }
protected:
	WNDPROC _defaultProc = nullptr;
	LRESULT runProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK staticProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam) 
	{
		return (((TaskList*)(::GetWindowLongPtr(hwnd, GWLP_USERDATA)))->runProc(hwnd, Message, wParam, lParam));
	}
	HFONT _hFont = nullptr;
	HFONT _hFontSelected = nullptr;
	int _nbItem = 0;
	int _currentIndex = 0;
	RECT _rc = { 0 };
};
//
//#include "xmlMatchedTagsHighlighter.h"
class XmlMatchedTagsHighlighter {
public:
	explicit XmlMatchedTagsHighlighter(ScintillaEditView * pEditView) : _pEditView(pEditView)
	{
	}
	void tagMatch(bool doHiliteAttr);
private:
	ScintillaEditView * _pEditView;

	struct XmlMatchedTagsPos {
		int tagOpenStart;
		int tagNameEnd;
		int tagOpenEnd;
		int tagCloseStart;
		int tagCloseEnd;
	};
	struct FindResult {
		int start;
		int end;
		bool success;
	};
	bool getXmlMatchedTagsPos(XmlMatchedTagsPos & tagsPos);
	// Allowed whitespace characters in XML
	bool isWhitespace(int ch) const { return ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n'; }
	FindResult findText(const char * text, int start, int end, int flags = 0);
	FindResult findOpenTag(const std::string & tagName, int start, int end);
	FindResult findCloseTag(const std::string & tagName, int start, int end);
	int findCloseAngle(int startPosition, int endPosition);
	std::vector< std::pair<int, int> > getAttributesPos(int start, int end);
};
//
//#include "asciiListView.h"
class AsciiListView : public ListView {
public:
	void setValues(int codepage = 0);
	void resetValues(int codepage);
	generic_string getAscii(uchar value);
	generic_string getHtmlName(uchar value);
	int getHtmlNumber(uchar value);
private:
	int _codepage = -1;
};
//
//#include "ansiCharPanel.h"
class AnsiCharPanel : public DockingDlgInterface {
public:
	AnsiCharPanel() : DockingDlgInterface(IDD_ANSIASCII_PANEL) 
	{
	}
	void init(HINSTANCE hInst, HWND hPere, ScintillaEditView ** ppEditView) 
	{
		DockingDlgInterface::init(hInst, hPere);
		_ppEditView = ppEditView;
	}
	void setParent(HWND parent2set) { _hParent = parent2set; }
	void switchEncoding();
	void insertChar(uchar char2insert) const;
	void insertString(LPWSTR string2insert) const;
	virtual void setBackgroundColor(int bgColour) const;
	virtual void setForegroundColor(int fgColour) const;
protected:
	virtual INT_PTR CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
private:
	ScintillaEditView ** _ppEditView = nullptr;
	AsciiListView _listView;
};
//
//#include "clipboardHistoryPanel.h"
typedef std::vector<uchar> ClipboardData;

class ByteArray {
public:
	ByteArray() = default;
	explicit ByteArray(ClipboardData cd);
	~ByteArray() 
	{
		delete [] _pBytes;
	}
	const uchar * getPointer() const { return _pBytes; }
	size_t getLength() const { return _length; }
protected:
	uchar * _pBytes = nullptr;
	size_t _length = 0;
};

class StringArray : public ByteArray {
public:
	StringArray(ClipboardData cd, size_t maxLen);
};

class ClipboardHistoryPanel : public DockingDlgInterface {
public:
	ClipboardHistoryPanel() : DockingDlgInterface(IDD_CLIPBOARDHISTORY_PANEL), _ppEditView(NULL), _hwndNextCbViewer(NULL),
		_lbBgColor(-1), _lbFgColor(-1) 
	{
	}
	void init(HINSTANCE hInst, HWND hPere, ScintillaEditView ** ppEditView) 
	{
		DockingDlgInterface::init(hInst, hPere);
		_ppEditView = ppEditView;
	}
	void setParent(HWND parent2set)
	{
		_hParent = parent2set;
	}
	ClipboardData getClipboadData();
	void addToClipboadHistory(ClipboardData cbd);
	int getClipboardDataIndex(ClipboardData cbd);
	virtual void setBackgroundColor(COLORREF bgColour) 
	{
		_lbBgColor = bgColour;
	}
	virtual void setForegroundColor(COLORREF fgColour) 
	{
		_lbFgColor = fgColour;
	}
	void drawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);
protected:
	virtual INT_PTR CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
private:
	ScintillaEditView ** _ppEditView = nullptr;
	std::vector<ClipboardData> _clipboardDataVector;
	HWND _hwndNextCbViewer = nullptr;
	int _lbBgColor = -1;
	int _lbFgColor = -1;
};
//
//#include "TaskListDlg.h"
#define TASKLIST_USER    (WM_USER + 8000)
#define WM_GETTASKLISTINFO (TASKLIST_USER + 01)

struct TaskLstFnStatus {
	int _iView = -1;
	int _docIndex = 0;
	generic_string _fn;
	int _status = 0;
	void * _bufID = nullptr;
	TaskLstFnStatus(const generic_string& str, int status) : _fn(str), _status(status)
	{
	}
	TaskLstFnStatus(int iView, int docIndex, generic_string str, int status, void * bufID) :
		_iView(iView), _docIndex(docIndex), _fn(str), _status(status), _bufID(bufID) 
	{
	}
};

struct TaskListInfo {
	std::vector<TaskLstFnStatus> _tlfsLst;
	int _currentIndex = -1;
};

//static HWND hWndServer = NULL;
static HHOOK hook = NULL;
static winVer windowsVersion = WV_UNKNOWN;

static LRESULT CALLBACK hookProc(int nCode, WPARAM wParam, LPARAM lParam);

class TaskListDlg : public StaticDialog {
public:
	TaskListDlg() : StaticDialog() 
	{
		_instanceCount++;
	}
	void init(HINSTANCE hInst, HWND parent, HIMAGELIST hImgLst, bool dir) 
	{
		Window::init(hInst, parent);
		_hImalist = hImgLst;
		_initDir = dir;
	}
	int doDialog(bool isRTL = false);
	virtual void destroy() 
	{
	}
protected:
	INT_PTR CALLBACK run_dlgProc(UINT Message, WPARAM wParam, LPARAM lParam);
private:
	TaskList _taskList;
	TaskListInfo _taskListInfo;
	HIMAGELIST _hImalist = nullptr;
	bool _initDir = false;
	HHOOK _hHooker = nullptr;
	void drawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);
public:
	static int _instanceCount;
};
//
//#include "VerticalFileSwitcherListView.h"
typedef Buffer * BufferID;      //each buffer has unique ID by which it can be retrieved

#define SORT_DIRECTION_NONE     -1
#define SORT_DIRECTION_UP     0
#define SORT_DIRECTION_DOWN   1

#define FS_ROOTNODE "DocList"
#define FS_CLMNNAME "ColumnName"
#define FS_CLMNEXT  "ColumnExt"
#define FS_CLMNPATH "ColumnPath"

struct SwitcherFileInfo {
	BufferID _bufID = 0;
	int _iView = 0;
	SwitcherFileInfo() = delete;
	SwitcherFileInfo(BufferID buf, int view) : _bufID(buf), _iView(view)
	{
	}
};

class VerticalFileSwitcherListView : public Window {
public:
	VerticalFileSwitcherListView() = default;
	virtual ~VerticalFileSwitcherListView() = default;
	virtual void init(HINSTANCE hInst, HWND parent, HIMAGELIST hImaLst);
	virtual void destroy();
	void initList();
	BufferID getBufferInfoFromIndex(int index, int & view) const;
	void setBgColour(int i) { ListView_SetItemState(_hSelf, i, LVIS_SELECTED|LVIS_FOCUSED, 0xFF); }
	int newItem(BufferID bufferID, int iView);
	int closeItem(BufferID bufferID, int iView);
	void activateItem(BufferID bufferID, int iView);
	void setItemIconStatus(BufferID bufferID);
	generic_string getFullFilePath(size_t i) const;
	void insertColumn(const TCHAR * name, int width, int index);
	void resizeColumns(int totalWidth);
	void deleteColumn(size_t i) { ListView_DeleteColumn(_hSelf, i); };
	int nbSelectedFiles() const { return static_cast<int32_t>(SendMessage(_hSelf, LVM_GETSELECTEDCOUNT, 0, 0)); };
	std::vector<SwitcherFileInfo> getSelectedFiles(bool reverse = false) const;
	void reload();
	void setBackgroundColor(COLORREF bgColour);
	void setForegroundColor(COLORREF fgColour);
protected:
	HIMAGELIST _hImaLst = nullptr;
	WNDPROC _defaultProc = nullptr;
	LRESULT runProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK staticProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam) 
	{
		return (((VerticalFileSwitcherListView*)(::GetWindowLongPtr(hwnd, GWLP_USERDATA)))->runProc(hwnd, Message, wParam, lParam));
	}

	int find(BufferID bufferID, int iView) const;
	int add(BufferID bufferID, int iView);
	void remove(int index);
	void removeAll();
};
//
//#include "VerticalFileSwitcher.h"
struct sortCompareData {
	HWND hListView = nullptr;
	int columnIndex = 0;
	int sortDirection = 0;
};

class VerticalFileSwitcher : public DockingDlgInterface {
public:
	VerticalFileSwitcher() : DockingDlgInterface(IDD_DOCLIST) 
	{
	}
	void init(HINSTANCE hInst, HWND hPere, HIMAGELIST hImaLst) 
	{
		DockingDlgInterface::init(hInst, hPere);
		_hImaLst = hImaLst;
	}
	virtual void display(bool toShow = true) const 
	{
		DockingDlgInterface::display(toShow);
	}
	void setParent(HWND parent2set)
	{
		_hParent = parent2set;
	}
	//Activate document in scintilla by using the internal index
	void activateDoc(TaskLstFnStatus * tlfs) const;
	int newItem(BufferID bufferID, int iView) { return _fileListView.newItem(bufferID, iView); }
	int closeItem(BufferID bufferID, int iView)
	{
		return _fileListView.closeItem(bufferID, iView);
	}
	void activateItem(BufferID bufferID, int iView) 
	{
		_fileListView.activateItem(bufferID, iView);
	}
	void setItemIconStatus(BufferID bufferID) 
	{
		_fileListView.setItemIconStatus(bufferID);
	}
	generic_string getFullFilePath(size_t i) const { return _fileListView.getFullFilePath(i); }
	int setHeaderOrder(int columnIndex);
	void updateHeaderArrow();
	int nbSelectedFiles() const { return _fileListView.nbSelectedFiles(); }
	std::vector<SwitcherFileInfo> getSelectedFiles(bool reverse = false) const { return _fileListView.getSelectedFiles(reverse); }
	void startColumnSort();
	void reload()
	{
		_fileListView.reload();
		startColumnSort();
	}
	void updateTabOrder()
	{
		if(_lastSortingDirection == SORT_DIRECTION_NONE) {
			_fileListView.reload();
		}
	}
	virtual void setBackgroundColor(COLORREF bgColour) 
	{
		_fileListView.setBackgroundColor(bgColour);
	}
	virtual void setForegroundColor(COLORREF fgColour) 
	{
		_fileListView.setForegroundColor(fgColour);
	}
protected:
	HMENU _hGlobalMenu = NULL;
	virtual INT_PTR CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
	void initPopupMenus();
	void popupMenuCmd(int cmdID);
private:
	bool colHeaderRClick = false;
	int _lastSortingColumn = 0;
	int _lastSortingDirection = SORT_DIRECTION_NONE;
	VerticalFileSwitcherListView _fileListView;
	HIMAGELIST _hImaLst = nullptr;
};
//
//#include "ProjectPanel.h"
enum NodeType {
	nodeType_root = 0, 
	nodeType_project = 1, 
	nodeType_folder = 2, 
	nodeType_file = 3
};

class ProjectPanel : public DockingDlgInterface {
public:
	ProjectPanel() : DockingDlgInterface(IDD_PROJECTPANEL) 
	{
	}
	~ProjectPanel();
	void init(HINSTANCE hInst, HWND hPere, int panelID) 
	{
		DockingDlgInterface::init(hInst, hPere);
		_panelID = panelID;
	}
	virtual void display(bool toShow = true) const { DockingDlgInterface::display(toShow); }
	void setParent(HWND parent2set) { _hParent = parent2set; }
	void setPanelTitle(generic_string title)  { _panelTitle = title; }
	const TCHAR * getPanelTitle() const { return _panelTitle.c_str(); }
	void newWorkSpace();
	bool saveWorkspaceRequest();
	bool openWorkSpace(const TCHAR * projectFileName, bool force = false);
	bool saveWorkSpace();
	bool saveWorkSpaceAs(bool saveCopyAs);
	void setWorkSpaceFilePath(const TCHAR * projectFileName) { _workSpaceFilePath = projectFileName; }
	const TCHAR * getWorkSpaceFilePath() const { return _workSpaceFilePath.c_str(); }
	bool isDirty() const { return _isDirty; }
	bool checkIfNeedSave();
	virtual void setBackgroundColor(COLORREF bgColour);
	virtual void setForegroundColor(COLORREF fgColour);
	bool enumWorkSpaceFiles(HTREEITEM tvFrom, const std::vector<generic_string> & patterns, std::vector<generic_string> & fileNames);
protected:
	TreeView _treeView;
	HIMAGELIST _hImaLst = nullptr;
	HWND _hToolbarMenu = nullptr;
	HMENU _hWorkSpaceMenu = nullptr;
	HMENU _hProjectMenu = nullptr;
	HMENU _hFolderMenu = nullptr;
	HMENU _hFileMenu = nullptr;
	generic_string _panelTitle;
	generic_string _workSpaceFilePath;
	generic_string _selDirOfFilesFromDirDlg;
	bool _isDirty = false;
	int _panelID = 0;

	void initMenus();
	void destroyMenus();
	BOOL setImageList(int root_clean_id, int root_dirty_id, int project_id, int open_node_id, int closed_node_id, int leaf_id, int ivalid_leaf_id);
	void addFiles(HTREEITEM hTreeItem);
	void addFilesFromDirectory(HTREEITEM hTreeItem);
	void recursiveAddFilesFrom(const TCHAR * folderPath, HTREEITEM hTreeItem);
	HTREEITEM addFolder(HTREEITEM hTreeItem, const TCHAR * folderName);
	bool writeWorkSpace(const TCHAR * projectFileName = NULL);
	generic_string getRelativePath(const generic_string & fn, const TCHAR * workSpaceFileName);
	void buildProjectXml(TiXmlNode * root, HTREEITEM hItem, const TCHAR* fn2write);
	NodeType getNodeType(HTREEITEM hItem);
	void setWorkSpaceDirty(bool isDirty);
	void popupMenuCmd(int cmdID);
	POINT getMenuDisplayPoint(int iButton);
	virtual INT_PTR CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
	bool buildTreeFrom(TiXmlNode * projectRoot, HTREEITEM hParentItem);
	void notified(LPNMHDR notification);
	void showContextMenu(int x, int y);
	void showContextMenuFromMenuKey(HTREEITEM selectedItem, int x, int y);
	HMENU getMenuHandler(HTREEITEM selectedItem);
	generic_string getAbsoluteFilePath(const TCHAR * relativePath);
	void openSelectFile();
	void setFileExtFilter(CustomFileDialog & fDlg);
	std::vector<generic_string*> fullPathStrs;
};

class FileRelocalizerDlg : public StaticDialog {
public:
	FileRelocalizerDlg() = default;
	void init(HINSTANCE hInst, HWND parent) 
	{
		Window::init(hInst, parent);
	}
	int doDialog(const TCHAR * fn, bool isRTL = false);
	virtual void destroy() 
	{
	}
	generic_string getFullFilePath() const { return _fullFilePath; }
protected:
	virtual INT_PTR CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
private:
	generic_string _fullFilePath;
};
//
//#include "functionListPanel.h"

class ScintillaEditView;
/*
1. global function + object + methods: Tree view of 2 levels - only the leaf contains the position info
root
|
|---leaf
|
|---node
|    |
|    |---leaf
|    |
|    |---leaf
|
|---node
     |
     |---leaf

2. each rule associates with file kind. For example, c_def (for *.c), cpp_def (for *.cpp) cpp_header (for *h) java_def (for *.java)...etc.
*/
struct SearchParameters {
	generic_string _text2Find;
	bool _doSort = false;
	bool hasParams() const { return (_text2Find != TEXT("") || _doSort); }
};

struct TreeParams {
	TreeStateNode _treeState;
	SearchParameters _searchParameters;
};

class FunctionListPanel : public DockingDlgInterface {
public:
	FunctionListPanel(): DockingDlgInterface(IDD_FUNCLIST_PANEL), _pTreeView(&_treeView) {};
	~FunctionListPanel();
	void init(HINSTANCE hInst, HWND hPere, ScintillaEditView **ppEditView);
    virtual void display(bool toShow = true) const;
	virtual void setBackgroundColor(COLORREF bgColour);
	virtual void setForegroundColor(COLORREF fgColour);
    void setParent(HWND parent2set);
	// functionalities
	static int CALLBACK categorySortFunc(LPARAM lParam1, LPARAM lParam2, LPARAM /*lParamSort*/);
	void sortOrUnsort();
	void reload();
	void markEntry();
	bool serialize(const generic_string & outputFilename = TEXT(""));
	void addEntry(const TCHAR *node, const TCHAR *displayText, size_t pos);
	void removeAllEntries();
	void searchFuncAndSwitchView();
protected:
	virtual INT_PTR CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
private:
	HWND _hToolbarMenu = nullptr;
	HWND _hSearchEdit = nullptr;
	TreeView *_pTreeView = nullptr;
	TreeView _treeView;
	TreeView _treeViewSearchResult;
	SCROLLINFO si;
	long _findLine = -1;
	long _findEndLine = -1;
	HTREEITEM _findItem = nullptr;
	generic_string _sortTipStr = TEXT("Sort");
	generic_string _reloadTipStr = TEXT("Reload");
	std::vector<foundInfo> _foundFuncInfos;
	std::vector<generic_string*> posStrs;
	ScintillaEditView **_ppEditView = nullptr;
	FunctionParsersManager _funcParserMgr;
	std::vector< std::pair<int, int> > _skipZones;
	std::vector<TreeParams> _treeParams;
	HIMAGELIST _hTreeViewImaLst = nullptr;

	generic_string parseSubLevel(size_t begin, size_t end, std::vector< generic_string > dataToSearch, int & foundPos);
	size_t getBodyClosePos(size_t begin, const TCHAR *bodyOpenSymbol, const TCHAR *bodyCloseSymbol);
	void notified(LPNMHDR notification);
	void addInStateArray(TreeStateNode tree2Update, const TCHAR *searchText, bool isSorted);
	TreeParams* getFromStateArray(generic_string fullFilePath);
	BOOL setTreeViewImageList(int root_id, int node_id, int leaf_id);
	bool openSelection(const TreeView &treeView);
	bool shouldSort();
	void setSort(bool isEnabled);
	void findMarkEntry(HTREEITEM htItem, LONG line);
};
//
//#include "DockingSplitter.h"
#define DMS_VERTICAL   0x00000001
#define DMS_HORIZONTAL 0x00000002

class DockingSplitter : public Window { // Copyright (C) 2006 Jens Lorenz <jens.plugin.npp@gmx.de>
public:
	DockingSplitter() = default;
	~DockingSplitter() = default;
	virtual void destroy() 
	{
	}
public:
	void init(HINSTANCE hInst, HWND hWnd, HWND hMessage, UINT flags);
protected:
	static LRESULT CALLBACK staticWinProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
	LRESULT runProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
private:
	HWND _hMessage = nullptr;
	BOOL _isLeftButtonDown = FALSE;
	POINT _ptOldPos = {0, 0};
	UINT _flags = 0;
	static BOOL _isVertReg;
	static BOOL _isHoriReg;
};
//
//#include "ToolTip.h"
class ToolTip : public Window {
public:
	ToolTip() = default;
	void destroy();
// Attributes
public:
// Implementation
public:
	virtual void init(HINSTANCE hInst, HWND hParent);
	void Show(RECT rectTitle, const TCHAR* pszTitleText, int iXOff = 0, int iWidthOff = 0);
protected:
	WNDPROC _defaultProc = nullptr;
	BOOL _bTrackMouse = FALSE;
	TOOLINFO _ti;
	static LRESULT CALLBACK staticWinProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
	LRESULT runProc(UINT Message, WPARAM wParam, LPARAM lParam);
};
//
//#include "ReadDirectoryChangesPrivate.h"
//
//	This sample code is for my blog entry titled, "Understanding ReadDirectoryChangesW"
//	http://qualapps.blogspot.com/2010/05/understanding-readdirectorychangesw.html
//	See ReadMe.txt for overview information.
//
namespace ReadDirectoryChangesPrivate { // Copyright (c) 2010 James E Beveridge
	class CReadChangesServer;
	//
	// All functions in CReadChangesRequest run in the context of the worker thread.
	// One instance of this object is created for each call to AddDirectory().
	//
	class CReadChangesRequest {
	public:
		CReadChangesRequest(CReadChangesServer* pServer, LPCTSTR sz, BOOL b, DWORD dw, DWORD size);
		~CReadChangesRequest();
		bool OpenDirectory();
		void BeginRead();
		// The dwSize is the actual number of bytes sent to the APC.
		void BackupBuffer(DWORD dwSize)
		{
			// We could just swap back and forth between the two
			// buffers, but this code is easier to understand and debug.
			memcpy(&m_BackupBuffer[0], &m_Buffer[0], dwSize);
		}
		void ProcessNotification();
		void RequestTermination()
		{
			::CancelIo(m_hDirectory);
			::CloseHandle(m_hDirectory);
			m_hDirectory = nullptr;
		}
		CReadChangesServer* m_pServer;
	protected:
		static VOID CALLBACK NotificationCompletion(DWORD dwErrorCode/*completion code*/, DWORD dwNumberOfBytesTransfered/*number of bytes transferred*/, LPOVERLAPPED lpOverlapped/* I/O information buffer */);
		// Parameters from the caller for ReadDirectoryChangesW().
		DWORD m_dwFilterFlags = 0;
		BOOL m_bIncludeChildren = FALSE;
		std::wstring m_wstrDirectory;
		HANDLE m_hDirectory = nullptr; // Result of calling CreateFile().
		OVERLAPPED m_Overlapped; // Required parameter for ReadDirectoryChangesW().
		// Data buffer for the request.
		// Since the memory is allocated by malloc, it will always
		// be aligned as required by ReadDirectoryChangesW().
		vector<BYTE> m_Buffer;
		// Double buffer strategy so that we can issue a new read
		// request before we process the current buffer.
		vector<BYTE> m_BackupBuffer;
	};
	//
	// All functions in CReadChangesServer run in the context of the worker thread.
	// One instance of this object is allocated for each instance of CReadDirectoryChanges.
	// This class is responsible for thread startup, orderly thread shutdown, and shimming
	// the various C++ member functions with C-style Win32 functions.
	//
	class CReadChangesServer {
	public:
		explicit CReadChangesServer(CReadDirectoryChanges* pParent) : m_bTerminate(false), m_nOutstandingRequests(0), m_pBase(pParent)
		{
		}
		static uint WINAPI ThreadStartProc(LPVOID arg)
		{
			CReadChangesServer* pServer = static_cast<CReadChangesServer*>(arg);
			pServer->Run();
			return 0;
		}
		// Called by QueueUserAPC to start orderly shutdown.
		static void CALLBACK TerminateProc(ULONG_PTR arg)
		{
			CReadChangesServer* pServer = reinterpret_cast<CReadChangesServer*>(arg);
			pServer->RequestTermination();
		}
		// Called by QueueUserAPC to add another directory.
		static void CALLBACK AddDirectoryProc(ULONG_PTR arg)
		{
			CReadChangesRequest* pRequest = reinterpret_cast<CReadChangesRequest*>(arg);
			pRequest->m_pServer->AddDirectory(pRequest);
		}
		CReadDirectoryChanges* m_pBase = nullptr;
		volatile DWORD m_nOutstandingRequests;
	protected:
		void Run()
		{
			while(m_nOutstandingRequests || !m_bTerminate) {
				::SleepEx(INFINITE, true);
			}
		}
		void AddDirectory(CReadChangesRequest* pBlock)
		{
			if(pBlock->OpenDirectory()) {
				::InterlockedIncrement(&pBlock->m_pServer->m_nOutstandingRequests);
				m_pBlocks.push_back(pBlock);
				pBlock->BeginRead();
			}
			else
				delete pBlock;
		}
		void RequestTermination()
		{
			m_bTerminate = true;
			for(DWORD i = 0; i<m_pBlocks.size(); ++i) {
				// Each Request object will delete itself.
				m_pBlocks[i]->RequestTermination();
			}
			m_pBlocks.clear();
		}
		vector<CReadChangesRequest*> m_pBlocks;
		bool m_bTerminate = false;
	};
}
//
//#include "ReadFileChanges.h"
#ifndef VC_EXTRALEAN
	#define VC_EXTRALEAN            // Exclude rarely-used stuff from Windows headers
#endif

class CReadFileChanges {
public:
	CReadFileChanges();
	~CReadFileChanges();
	void AddFile(LPCTSTR szDirectory, DWORD dwNotifyFilter);
	BOOL DetectChanges();
	void Terminate();
private:
	LPCTSTR _szFile = nullptr;
	DWORD _dwNotifyFilter = 0;
	WIN32_FILE_ATTRIBUTE_DATA _lastFileInfo = { 0 };
};
//
//#include "verifySignedfile.h"
//
//#define VerifySignedLibrary_DISABLE_REVOCATION_CHECK "Dont check certificat revocation"
// 
// Verifies an Authenticde DLL signature and ownership
// 
// Parameters:
//   @param filepath        path to the DLL file to examine
//   @param cert_display_name if specified, the signing certificate display name to compare to. Ignored if set to "", (weak comparison)
//   @param cert_subject    if specified, the full signing certificate subject name. Ignored if set to "" (strong comparison)
//   @param cert_key_id_hex if specified, the signing certificate key id (fingerprint), Ignored if set to "" (very strong comparison)
// @return true if the verification was positive, false if it was negative of encountered some error
// Dependencies:
//   This function uses 3 APIs: WinTrust, CryptoAPI, SENS API
//   It requires to link on : wintrust.lib, crypt32.lib (or crypt64.lib depending on the compilation target) and sensapi.lib
//   Those functions are available on Windows starting with Windows-XP
// Limitations:
//   Certificate revocation checking requires an access to Internet.
//   The functions checks for connectivity and will disable revocation checking if the machine is offline or if Microsoft
//   connectivity checking site is not reachable (supposely implying we are on an airgapped network).
//   Depending on Windows version, this test will be instantaneous (Windows 8 and up) or may take a few seconds.
//   This behaviour can be disabled by setting a define at compilation time.
//   If macro VerifySignedLibrary_DISABLE_REVOCATION_CHECK is defined, the revocation
//   state of the certificates will *not* be checked.
// 
enum SecurityMode { 
	sm_certif = 0, 
	sm_sha256 = 1 
};

enum NppModule { 
	nm_scilexer = 0, 
	nm_gup = 1, 
	nm_pluginList = 2 
};

class SecurityGard final {
public:
	SecurityGard();
	bool checkModule(const std::wstring& filePath, NppModule module2check);
private:
	// SHA256
	static SecurityMode _securityMode;
	std::vector<std::wstring> _scilexerSha256;
	std::vector<std::wstring> _gupSha256;
	std::vector<std::wstring> _pluginListSha256;
	bool checkSha256(const std::wstring& filePath, NppModule module2check);
	// Code signing certificate
	std::wstring _signer_display_name = TEXT("Notepad++");
	std::wstring _signer_subject = TEXT("C=FR, S=Ile-de-France, L=Saint Cloud, O=\"Notepad++\", CN=\"Notepad++\"");
	std::wstring _signer_key_id = TEXT("ED255D9151912E40DF048A56288E969A8D0DAFA3");
	bool _doCheckRevocation = false;
	bool _doCheckChainOfTrust = false;
	bool verifySignedLibrary(const std::wstring& filepath, NppModule module2check);
};
//
//#include "Win32Exception.h"
//
// This code was retrieved from
// http://www.thunderguy.com/semicolon/2002/08/15/visual-c-exception-handling/3/ (Visual C++ exception handling)
// By Bennett
// Formatting Slightly modified for N++
//
typedef const void * ExceptionAddress; // OK on Win32 platform

class Win32Exception : public std::exception {
public:
	static void installHandler();
	static void removeHandler();
	virtual const char * what()  const throw() { return _event; }
	ExceptionAddress where() const { return _location; }
	uint code()  const { return _code; }
	EXCEPTION_POINTERS * info()  const { return _info; }
protected:
	explicit Win32Exception(EXCEPTION_POINTERS * info); // Constructor only accessible by exception handler
	static void translate(unsigned code, EXCEPTION_POINTERS * info);
private:
	const char * _event = nullptr;
	ExceptionAddress _location;
	uint _code = 0;
	EXCEPTION_POINTERS * _info = nullptr;
};

class Win32AccessViolation : public Win32Exception {
public:
	bool isWrite() const { return _isWrite; }
	ExceptionAddress badAddress() const { return _badAddress; }
private:
	explicit Win32AccessViolation(EXCEPTION_POINTERS * info);
	bool _isWrite = false;
	ExceptionAddress _badAddress;
	friend void Win32Exception::translate(unsigned code, EXCEPTION_POINTERS* info);
};
//
//#include "keys.h"
//
// See winuser.h
// Altered list to support VK_0-9 and VK_A-Z
//
#define VK_NULL           0x00
#define VK_CANCEL         0x03
#define VK_BACK           0x08
#define VK_TAB            0x09
#define VK_CLEAR          0x0C
#define VK_RETURN         0x0D
#define VK_SHIFT          0x10
#define VK_CONTROL        0x11
#define VK_MENU           0x12
#define VK_PAUSE          0x13
#define VK_CAPITAL        0x14
#define VK_KANA           0x15
#define VK_HANGUL         0x15
#define VK_JUNJA          0x17
#define VK_FINAL          0x18
#define VK_HANJA          0x19
#define VK_KANJI          0x19
#define VK_ESCAPE         0x1B
#define VK_CONVERT        0x1C
#define VK_NONCONVERT     0x1D
#define VK_ACCEPT         0x1E
#define VK_MODECHANGE     0x1F
#define VK_SPACE          0x20
#define VK_PRIOR          0x21
#define VK_NEXT           0x22
#define VK_END            0x23
#define VK_HOME           0x24
#define VK_LEFT           0x25
#define VK_UP             0x26
#define VK_RIGHT          0x27
#define VK_DOWN           0x28
#define VK_SELECT         0x29
#define VK_PRINT          0x2A
#define VK_EXECUTE        0x2B
#define VK_SNAPSHOT       0x2C
#define VK_INSERT         0x2D
#define VK_DELETE         0x2E
#define VK_HELP           0x2F
#define VK_0              0x30
#define VK_1              0x31
#define VK_2              0x32
#define VK_3              0x33
#define VK_4              0x34
#define VK_5              0x35
#define VK_6              0x36
#define VK_7              0x37
#define VK_8              0x38
#define VK_9              0x39
#define VK_A              0x41
#define VK_B              0x42
#define VK_C              0x43
#define VK_D              0x44
#define VK_E              0x45
#define VK_F              0x46
#define VK_G              0x47
#define VK_H              0x48
#define VK_I              0x49
#define VK_J              0x4A
#define VK_K              0x4B
#define VK_L              0x4C
#define VK_M              0x4D
#define VK_N              0x4E
#define VK_O              0x4F
#define VK_P              0x50
#define VK_Q              0x51
#define VK_R              0x52
#define VK_S              0x53
#define VK_T              0x54
#define VK_U              0x55
#define VK_V              0x56
#define VK_W              0x57
#define VK_X              0x58
#define VK_Y              0x59
#define VK_Z              0x5A
#define VK_LWIN           0x5B
#define VK_RWIN           0x5C
#define VK_APPS           0x5D
#define VK_SLEEP          0x5F
#define VK_NUMPAD0        0x60
#define VK_NUMPAD1        0x61
#define VK_NUMPAD2        0x62
#define VK_NUMPAD3        0x63
#define VK_NUMPAD4        0x64
#define VK_NUMPAD5        0x65
#define VK_NUMPAD6        0x66
#define VK_NUMPAD7        0x67
#define VK_NUMPAD8        0x68
#define VK_NUMPAD9        0x69
#define VK_MULTIPLY       0x6A
#define VK_ADD            0x6B
#define VK_SEPARATOR      0x6C
#define VK_SUBTRACT       0x6D
#define VK_DECIMAL        0x6E
#define VK_DIVIDE         0x6F
#define VK_F1             0x70
#define VK_F2             0x71
#define VK_F3             0x72
#define VK_F4             0x73
#define VK_F5             0x74
#define VK_F6             0x75
#define VK_F7             0x76
#define VK_F8             0x77
#define VK_F9             0x78
#define VK_F10            0x79
#define VK_F11            0x7A
#define VK_F12            0x7B
#define VK_F13            0x7C
#define VK_F14            0x7D
#define VK_F15            0x7E
#define VK_F16            0x7F
#define VK_F17            0x80
#define VK_F18            0x81
#define VK_F19            0x82
#define VK_F20            0x83
#define VK_F21            0x84
#define VK_F22            0x85
#define VK_F23            0x86
#define VK_F24            0x87
#define VK_NUMLOCK        0x90
#define VK_SCROLL         0x91
#define VK_OEM_1          0xBA   // ';:' for US
#define VK_OEM_PLUS       0xBB   // '+' any country
#define VK_OEM_COMMA      0xBC   // ',' any country
#define VK_OEM_MINUS      0xBD   // '-' any country
#define VK_OEM_PERIOD     0xBE   // '.' any country
#define VK_OEM_2          0xBF   // '/?' for US
#define VK_OEM_3          0xC0   // '`~' for US
#define VK_OEM_4          0xDB  //  '[{' for US
#define VK_OEM_5          0xDC  //  '\|' for US
#define VK_OEM_6          0xDD  //  ']}' for US
#define VK_OEM_7          0xDE  //  ''"' for US
#define VK_OEM_8          0xDF
#define VK_OEM_102        0xE2  //  "<>" or "\|" on RT 102-key kbd.
#define VK_OEM_RESET      0xE9
#define VK_OEM_JUMP       0xEA
#define VK_OEM_PA1        0xEB
#define VK_OEM_PA2        0xEC
#define VK_OEM_PA3        0xED
#define VK_OEM_WSCTRL     0xEE
#define VK_OEM_CUSEL      0xEF
#define VK_OEM_ATTN       0xF0
#define VK_OEM_FINISH     0xF1
#define VK_OEM_COPY       0xF2
#define VK_OEM_AUTO       0xF3
#define VK_OEM_ENLW       0xF4
#define VK_OEM_BACKTAB    0xF5
#define VK_ATTN           0xF6
#define VK_CRSEL          0xF7
#define VK_EXSEL          0xF8
#define VK_EREOF          0xF9
#define VK_PLAY           0xFA
#define VK_ZOOM           0xFB
#define VK_NONAME         0xFC
#define VK_PA1            0xFD
#define VK_OEM_CLEAR      0xFE
//
// (empty) #include "localizationString.h"
//#include "DarkMode.h"
extern bool g_darkModeSupported;
extern bool g_darkModeEnabled;

bool ShouldAppsUseDarkMode();
bool AllowDarkModeForWindow(HWND hWnd, bool allow);
bool IsHighContrast();
void RefreshTitleBarThemeColor(HWND hWnd);
void SetTitleBarThemeColor(HWND hWnd, BOOL dark);
bool IsColorSchemeChangeMessage(LPARAM lParam);
bool IsColorSchemeChangeMessage(UINT message, LPARAM lParam);
void AllowDarkModeForApp(bool allow);
void EnableDarkScrollBarForWindowAndChildren(HWND hwnd);
void InitDarkMode();
void SetDarkMode(bool useDarkMode, bool fixDarkScrollbar);
//
//#include "UAHMenuBar.h"
// MIT license, see LICENSE Copyright(c) 2021 adzm / Adam D. Walling
//
// processes messages related to UAH / custom menubar drawing.
// return true if handled, false to continue with normal processing in your wndproc
bool UAHDarkModeWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam, LRESULT* lr);

// window messages related to menu bar drawing
#define WM_UAHDESTROYWINDOW    0x0090	// handled by DefWindowProc
#define WM_UAHDRAWMENU         0x0091	// lParam is UAHMENU
#define WM_UAHDRAWMENUITEM     0x0092	// lParam is UAHDRAWMENUITEM
#define WM_UAHINITMENU         0x0093	// handled by DefWindowProc
#define WM_UAHMEASUREMENUITEM  0x0094	// lParam is UAHMEASUREMENUITEM
#define WM_UAHNCPAINTMENUPOPUP 0x0095	// handled by DefWindowProc

// describes the sizes of the menu bar or menu item
typedef union tagUAHMENUITEMMETRICS {
	// cx appears to be 14 / 0xE less than rcItem's width!
	// cy 0x14 seems stable, i wonder if it is 4 less than rcItem's height which is always 24 atm
	struct {
		DWORD cx;
		DWORD cy;
	} rgsizeBar[2];
	struct {
		DWORD cx;
		DWORD cy;
	} rgsizePopup[4];
} UAHMENUITEMMETRICS;

// not really used in our case but part of the other structures
typedef struct tagUAHMENUPOPUPMETRICS {
	DWORD rgcx[4];
	DWORD fUpdateMaxWidths : 2; // from kernel symbols, padded to full dword
} UAHMENUPOPUPMETRICS;

// hmenu is the main window menu; hdc is the context to draw in
typedef struct tagUAHMENU {
	HMENU hmenu;
	HDC hdc;
	DWORD dwFlags; // no idea what these mean, in my testing it's either 0x00000a00 or sometimes 0x00000a10
} UAHMENU;

// menu items are always referred to by iPosition here
typedef struct tagUAHMENUITEM {
	int iPosition; // 0-based position of menu item in menubar
	UAHMENUITEMMETRICS umim;
	UAHMENUPOPUPMETRICS umpm;
} UAHMENUITEM;

// the DRAWITEMSTRUCT contains the states of the menu items, as well as
// the position index of the item in the menu, which is duplicated in
// the UAHMENUITEM's iPosition as well
typedef struct UAHDRAWMENUITEM {
	DRAWITEMSTRUCT dis; // itemID looks uninitialized
	UAHMENU um;
	UAHMENUITEM umi;
} UAHDRAWMENUITEM;

// the MEASUREITEMSTRUCT is intended to be filled with the size of the item
// height appears to be ignored, but width can be modified
typedef struct tagUAHMEASUREMENUITEM {
	MEASUREITEMSTRUCT mis;
	UAHMENU um;
	UAHMENUITEM umi;
} UAHMEASUREMENUITEM;
//
//#include "Sorters.h"
//
// Base interface for line sorting.
//
class ISorter {
private:
	bool _isDescending = true;
	size_t _fromColumn = 0;
	size_t _toColumn = 0;
protected:
	bool isDescending() const { return _isDescending; }
	generic_string getSortKey(const generic_string& input);
	bool isSortingSpecificColumns() const { return _toColumn != 0; }
public:
	ISorter(bool isDescending, size_t fromColumn, size_t toColumn) : _isDescending(isDescending), _fromColumn(fromColumn), _toColumn(toColumn)
	{
		assert(_fromColumn <= _toColumn);
	}
	virtual ~ISorter() 
	{
	}
	virtual std::vector<generic_string> sort(std::vector<generic_string> lines) = 0;
};

// Implementation of lexicographic sorting of lines.
class LexicographicSorter : public ISorter {
public:
	LexicographicSorter(bool isDescending, size_t fromColumn, size_t toColumn) : ISorter(isDescending, fromColumn, toColumn) 
	{
	}
	std::vector<generic_string> sort(std::vector<generic_string> lines) override;
};

// Implementation of lexicographic sorting of lines, ignoring character casing
class LexicographicCaseInsensitiveSorter : public ISorter {
public:
	LexicographicCaseInsensitiveSorter(bool isDescending, size_t fromColumn, size_t toColumn) : ISorter(isDescending, fromColumn, toColumn) 
	{
	}
	std::vector<generic_string> sort(std::vector<generic_string> lines) override;
};

// Treat consecutive numerals as one number
// Otherwise it is a lexicographic sort
class NaturalSorter : public ISorter {
public:
	NaturalSorter(bool isDescending, size_t fromColumn, size_t toColumn) : ISorter(isDescending, fromColumn, toColumn) 
	{
	}
	std::vector<generic_string> sort(std::vector<generic_string> lines) override;
};

class ReverseSorter : public ISorter {
public:
	ReverseSorter(bool isDescending, size_t fromColumn, size_t toColumn) : ISorter(isDescending, fromColumn, toColumn) 
	{
	}
	std::vector<generic_string> sort(std::vector<generic_string> lines) override
	{
		std::reverse(lines.begin(), lines.end());
		return lines;
	}
};

class RandomSorter : public ISorter {
public:
	uint   seed;
	RandomSorter(bool isDescending, size_t fromColumn, size_t toColumn) : ISorter(isDescending, fromColumn, toColumn)
	{
		seed = static_cast<unsigned>(time(NULL));
	}
	std::vector<generic_string> sort(std::vector<generic_string> lines) override
	{
		std::shuffle(lines.begin(), lines.end(), std::default_random_engine(seed));
		return lines;
	}
};
//
#endif // } __NPP_INTERNAL_H