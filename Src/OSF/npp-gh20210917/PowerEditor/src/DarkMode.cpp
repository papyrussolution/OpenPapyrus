// DARKMODE.CPP
//
#include <npp-internal.h>
#pragma hdrstop
//
//#include "IatHook.h"
//
// Code from https://github.com/stevemk14ebr/PolyHook_2_0/blob/master/sources/IatHook.cpp
// which is licensed under the MIT License. See PolyHook_2_0-LICENSE for more information.
//
template <typename T, typename T1, typename T2> constexpr T RVA2VA(T1 base, T2 rva)
{
	return reinterpret_cast<T>(reinterpret_cast<ULONG_PTR>(base) + rva);
}

template <typename T> constexpr T DataDirectoryFromModuleBase(void * moduleBase, size_t entryID)
{
	auto dosHdr = reinterpret_cast<PIMAGE_DOS_HEADER>(moduleBase);
	auto ntHdr = RVA2VA<PIMAGE_NT_HEADERS>(moduleBase, dosHdr->e_lfanew);
	auto dataDir = ntHdr->OptionalHeader.DataDirectory;
	return RVA2VA<T>(moduleBase, dataDir[entryID].VirtualAddress);
}

/*inline*/static PIMAGE_THUNK_DATA FindAddressByName(void * moduleBase, PIMAGE_THUNK_DATA impName, PIMAGE_THUNK_DATA impAddr, const char * funcName)
{
	for(; impName->u1.Ordinal; ++impName, ++impAddr) {
		if(IMAGE_SNAP_BY_ORDINAL(impName->u1.Ordinal))
			continue;
		auto import = RVA2VA<PIMAGE_IMPORT_BY_NAME>(moduleBase, impName->u1.AddressOfData);
		if(!sstreq(reinterpret_cast<const char *>(import->Name), funcName))
			continue;
		return impAddr;
	}
	return nullptr;
}

/*inline*/static PIMAGE_THUNK_DATA FindAddressByOrdinal(void * moduleBase, PIMAGE_THUNK_DATA impName, PIMAGE_THUNK_DATA impAddr, uint16_t ordinal)
{
	UNREFERENCED_PARAMETER(moduleBase);
	for(; impName->u1.Ordinal; ++impName, ++impAddr) {
		if(IMAGE_SNAP_BY_ORDINAL(impName->u1.Ordinal) && IMAGE_ORDINAL(impName->u1.Ordinal) == ordinal)
			return impAddr;
	}
	return nullptr;
}

inline PIMAGE_THUNK_DATA FindIatThunkInModule(void * moduleBase, const char * dllName, const char * funcName)
{
	auto imports = DataDirectoryFromModuleBase<PIMAGE_IMPORT_DESCRIPTOR>(moduleBase, IMAGE_DIRECTORY_ENTRY_IMPORT);
	for(; imports->Name; ++imports) {
		if(_stricmp(RVA2VA<LPCSTR>(moduleBase, imports->Name), dllName) != 0)
			continue;
		auto origThunk = RVA2VA<PIMAGE_THUNK_DATA>(moduleBase, imports->OriginalFirstThunk);
		auto thunk = RVA2VA<PIMAGE_THUNK_DATA>(moduleBase, imports->FirstThunk);
		return FindAddressByName(moduleBase, origThunk, thunk, funcName);
	}
	return nullptr;
}

inline PIMAGE_THUNK_DATA FindDelayLoadThunkInModule(void * moduleBase, const char * dllName, const char * funcName)
{
	auto imports = DataDirectoryFromModuleBase<PIMAGE_DELAYLOAD_DESCRIPTOR>(moduleBase, IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT);
	for(; imports->DllNameRVA; ++imports) {
		if(_stricmp(RVA2VA<LPCSTR>(moduleBase, imports->DllNameRVA), dllName) != 0)
			continue;
		auto impName = RVA2VA<PIMAGE_THUNK_DATA>(moduleBase, imports->ImportNameTableRVA);
		auto impAddr = RVA2VA<PIMAGE_THUNK_DATA>(moduleBase, imports->ImportAddressTableRVA);
		return FindAddressByName(moduleBase, impName, impAddr, funcName);
	}
	return nullptr;
}

inline PIMAGE_THUNK_DATA FindDelayLoadThunkInModule(void * moduleBase, const char * dllName, uint16_t ordinal)
{
	auto imports = DataDirectoryFromModuleBase<PIMAGE_DELAYLOAD_DESCRIPTOR>(moduleBase, IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT);
	for(; imports->DllNameRVA; ++imports) {
		if(_stricmp(RVA2VA<LPCSTR>(moduleBase, imports->DllNameRVA), dllName) != 0)
			continue;
		auto impName = RVA2VA<PIMAGE_THUNK_DATA>(moduleBase, imports->ImportNameTableRVA);
		auto impAddr = RVA2VA<PIMAGE_THUNK_DATA>(moduleBase, imports->ImportAddressTableRVA);
		return FindAddressByOrdinal(moduleBase, impName, impAddr, ordinal);
	}
	return nullptr;
}

#if defined(__GNUC__) && __GNUC__ > 8
	#define WINAPI_LAMBDA_RETURN(return_t) ->return_t WINAPI
#elif defined(__GNUC__)
	#define WINAPI_LAMBDA_RETURN(return_t) WINAPI->return_t
#else
	#define WINAPI_LAMBDA_RETURN(return_t) ->return_t
#endif

enum IMMERSIVE_HC_CACHE_MODE {
	IHCM_USE_CACHED_VALUE,
	IHCM_REFRESH
};

// 1903 18362
enum class PreferredAppMode {
	Default,
	AllowDark,
	ForceDark,
	ForceLight,
	Max
};

enum WINDOWCOMPOSITIONATTRIB {
	WCA_UNDEFINED = 0,
	WCA_NCRENDERING_ENABLED = 1,
	WCA_NCRENDERING_POLICY = 2,
	WCA_TRANSITIONS_FORCEDISABLED = 3,
	WCA_ALLOW_NCPAINT = 4,
	WCA_CAPTION_BUTTON_BOUNDS = 5,
	WCA_NONCLIENT_RTL_LAYOUT = 6,
	WCA_FORCE_ICONIC_REPRESENTATION = 7,
	WCA_EXTENDED_FRAME_BOUNDS = 8,
	WCA_HAS_ICONIC_BITMAP = 9,
	WCA_THEME_ATTRIBUTES = 10,
	WCA_NCRENDERING_EXILED = 11,
	WCA_NCADORNMENTINFO = 12,
	WCA_EXCLUDED_FROM_LIVEPREVIEW = 13,
	WCA_VIDEO_OVERLAY_ACTIVE = 14,
	WCA_FORCE_ACTIVEWINDOW_APPEARANCE = 15,
	WCA_DISALLOW_PEEK = 16,
	WCA_CLOAK = 17,
	WCA_CLOAKED = 18,
	WCA_ACCENT_POLICY = 19,
	WCA_FREEZE_REPRESENTATION = 20,
	WCA_EVER_UNCLOAKED = 21,
	WCA_VISUAL_OWNER = 22,
	WCA_HOLOGRAPHIC = 23,
	WCA_EXCLUDED_FROM_DDA = 24,
	WCA_PASSIVEUPDATEMODE = 25,
	WCA_USEDARKMODECOLORS = 26,
	WCA_LAST = 27
};

struct WINDOWCOMPOSITIONATTRIBDATA {
	WINDOWCOMPOSITIONATTRIB Attrib;
	PVOID pvData;
	SIZE_T cbData;
};

using fnRtlGetNtVersionNumbers = void (WINAPI*)(LPDWORD major, LPDWORD minor, LPDWORD build);
using fnSetWindowCompositionAttribute = BOOL(WINAPI *)(HWND hWnd, WINDOWCOMPOSITIONATTRIBDATA*);
// 1809 17763
using fnShouldAppsUseDarkMode = bool (WINAPI*)();  // ordinal 132
using fnAllowDarkModeForWindow = bool (WINAPI*)(HWND hWnd, bool allow);  // ordinal 133
using fnAllowDarkModeForApp = bool (WINAPI*)(bool allow);  // ordinal 135, in 1809
using fnFlushMenuThemes = void (WINAPI*)();  // ordinal 136
using fnRefreshImmersiveColorPolicyState = void (WINAPI*)();  // ordinal 104
using fnIsDarkModeAllowedForWindow = bool (WINAPI*)(HWND hWnd);  // ordinal 137
using fnGetIsImmersiveColorUsingHighContrast = bool (WINAPI*)(IMMERSIVE_HC_CACHE_MODE mode);  // ordinal 106
using fnOpenNcThemeData = HTHEME(WINAPI *)(HWND hWnd, LPCWSTR pszClassList); // ordinal 49
// 1903 18362
using fnShouldSystemUseDarkMode = bool (WINAPI*)();  // ordinal 138
using fnSetPreferredAppMode = PreferredAppMode(WINAPI *)(PreferredAppMode appMode);  // ordinal 135, in 1903
using fnIsDarkModeAllowedForApp = bool (WINAPI*)();  // ordinal 139

fnSetWindowCompositionAttribute _SetWindowCompositionAttribute = nullptr;
fnShouldAppsUseDarkMode _ShouldAppsUseDarkMode = nullptr;
fnAllowDarkModeForWindow _AllowDarkModeForWindow = nullptr;
fnAllowDarkModeForApp _AllowDarkModeForApp = nullptr;
fnFlushMenuThemes _FlushMenuThemes = nullptr;
fnRefreshImmersiveColorPolicyState _RefreshImmersiveColorPolicyState = nullptr;
fnIsDarkModeAllowedForWindow _IsDarkModeAllowedForWindow = nullptr;
fnGetIsImmersiveColorUsingHighContrast _GetIsImmersiveColorUsingHighContrast = nullptr;
fnOpenNcThemeData _OpenNcThemeData = nullptr;
// 1903 18362
//fnShouldSystemUseDarkMode _ShouldSystemUseDarkMode = nullptr;
fnSetPreferredAppMode _SetPreferredAppMode = nullptr;

bool g_darkModeSupported = false;
bool g_darkModeEnabled = false;
DWORD g_buildNumber = 0;

bool ShouldAppsUseDarkMode()
{
	return _ShouldAppsUseDarkMode ? _ShouldAppsUseDarkMode() : false;
}

bool AllowDarkModeForWindow(HWND hWnd, bool allow)
{
	return (g_darkModeSupported && _AllowDarkModeForWindow) ? _AllowDarkModeForWindow(hWnd, allow) : false;
}

bool IsHighContrast()
{
	HIGHCONTRASTW highContrast = { sizeof(highContrast) };
	if(SystemParametersInfoW(SPI_GETHIGHCONTRAST, sizeof(highContrast), &highContrast, FALSE))
		return highContrast.dwFlags & HCF_HIGHCONTRASTON;
	return false;
}

void SetTitleBarThemeColor(HWND hWnd, BOOL dark)
{
	if(g_buildNumber < 18362)
		SetPropW(hWnd, L"UseImmersiveDarkModeColors", reinterpret_cast<HANDLE>(static_cast<INT_PTR>(dark)));
	else if(_SetWindowCompositionAttribute) {
		WINDOWCOMPOSITIONATTRIBDATA data = { WCA_USEDARKMODECOLORS, &dark, sizeof(dark) };
		_SetWindowCompositionAttribute(hWnd, &data);
	}
}

void RefreshTitleBarThemeColor(HWND hWnd)
{
	BOOL dark = FALSE;
	if(_IsDarkModeAllowedForWindow && _ShouldAppsUseDarkMode) {
		if(_IsDarkModeAllowedForWindow(hWnd) && _ShouldAppsUseDarkMode() && !IsHighContrast()) {
			dark = TRUE;
		}
	}

	SetTitleBarThemeColor(hWnd, dark);
}

bool IsColorSchemeChangeMessage(LPARAM lParam)
{
	bool is = false;
	if(lParam && (0 == lstrcmpi(reinterpret_cast<LPCWCH>(lParam), L"ImmersiveColorSet")) && _RefreshImmersiveColorPolicyState) {
		_RefreshImmersiveColorPolicyState();
		is = true;
	}
	if(_GetIsImmersiveColorUsingHighContrast)
		_GetIsImmersiveColorUsingHighContrast(IHCM_REFRESH);
	return is;
}

bool IsColorSchemeChangeMessage(UINT message, LPARAM lParam)
{
	return (message == WM_SETTINGCHANGE) ? IsColorSchemeChangeMessage(lParam) : false;
}

void AllowDarkModeForApp(bool allow)
{
	if(_AllowDarkModeForApp)
		_AllowDarkModeForApp(allow);
	else if(_SetPreferredAppMode)
		_SetPreferredAppMode(allow ? PreferredAppMode::ForceDark : PreferredAppMode::Default);
}

void FlushMenuThemes()
{
	if(_FlushMenuThemes)
		_FlushMenuThemes();
}

// limit dark scroll bar to specific windows and their children

static std::unordered_set<HWND> g_darkScrollBarWindows;
static std::mutex g_darkScrollBarMutex;

void EnableDarkScrollBarForWindowAndChildren(HWND hwnd)
{
	std::lock_guard<std::mutex> lock(g_darkScrollBarMutex);
	g_darkScrollBarWindows.insert(hwnd);
}

bool IsWindowOrParentUsingDarkScrollBar(HWND hwnd)
{
	HWND hwndRoot = GetAncestor(hwnd, GA_ROOT);
	std::lock_guard<std::mutex> lock(g_darkScrollBarMutex);
	if(g_darkScrollBarWindows.count(hwnd))
		return true;
	else if(hwnd != hwndRoot && g_darkScrollBarWindows.count(hwndRoot))
		return true;
	else
		return false;
}

void FixDarkScrollBar()
{
	HMODULE hComctl = LoadLibraryEx(L"comctl32.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
	if(hComctl) {
		auto addr = FindDelayLoadThunkInModule(hComctl, "uxtheme.dll", 49); // OpenNcThemeData
		if(addr) {
			DWORD oldProtect;
			if(VirtualProtect(addr, sizeof(IMAGE_THUNK_DATA), PAGE_READWRITE, &oldProtect) && _OpenNcThemeData) {
				auto MyOpenThemeData = [] (HWND hWnd, LPCWSTR classList)WINAPI_LAMBDA_RETURN(HTHEME) {
					if(wcscmp(classList, L"ScrollBar") == 0) {
						if(IsWindowOrParentUsingDarkScrollBar(hWnd)) {
							hWnd = nullptr;
							classList = L"Explorer::ScrollBar";
						}
					}
					return _OpenNcThemeData(hWnd, classList);
				};
				addr->u1.Function = reinterpret_cast<ULONG_PTR>(static_cast<fnOpenNcThemeData>(MyOpenThemeData));
				VirtualProtect(addr, sizeof(IMAGE_THUNK_DATA), oldProtect, &oldProtect);
			}
		}
	}
}

constexpr bool CheckBuildNumber(DWORD buildNumber)
{
	return (buildNumber == 17763 || // 1809
	       buildNumber == 18362 ||  // 1903
	       buildNumber == 18363 ||  // 1909
	       buildNumber == 19041 ||  // 2004
	       buildNumber == 19042 ||  // 20H2
	       buildNumber == 19043 ||  // 21H1
	       buildNumber >= 22000);   // Windows 11 insider builds
}

void InitDarkMode()
{
	fnRtlGetNtVersionNumbers RtlGetNtVersionNumbers = nullptr;
	HMODULE hNtdllModule = GetModuleHandle(L"ntdll.dll");
	if(hNtdllModule) {
		RtlGetNtVersionNumbers = reinterpret_cast<fnRtlGetNtVersionNumbers>(GetProcAddress(hNtdllModule, "RtlGetNtVersionNumbers"));
	}
	if(RtlGetNtVersionNumbers) {
		DWORD major, minor;
		RtlGetNtVersionNumbers(&major, &minor, &g_buildNumber);
		g_buildNumber &= ~0xF0000000;
		if(major == 10 && minor == 0 && CheckBuildNumber(g_buildNumber)) {
			HMODULE hUxtheme = LoadLibraryEx(L"uxtheme.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
			if(hUxtheme) {
				_OpenNcThemeData = reinterpret_cast<fnOpenNcThemeData>(GetProcAddress(hUxtheme, MAKEINTRESOURCEA(49)));
				_RefreshImmersiveColorPolicyState = reinterpret_cast<fnRefreshImmersiveColorPolicyState>(GetProcAddress(hUxtheme, MAKEINTRESOURCEA(104)));
				_GetIsImmersiveColorUsingHighContrast = reinterpret_cast<fnGetIsImmersiveColorUsingHighContrast>(GetProcAddress(hUxtheme, MAKEINTRESOURCEA(106)));
				_ShouldAppsUseDarkMode = reinterpret_cast<fnShouldAppsUseDarkMode>(GetProcAddress(hUxtheme, MAKEINTRESOURCEA(132)));
				_AllowDarkModeForWindow = reinterpret_cast<fnAllowDarkModeForWindow>(GetProcAddress(hUxtheme, MAKEINTRESOURCEA(133)));
				auto ord135 = GetProcAddress(hUxtheme, MAKEINTRESOURCEA(135));
				if(g_buildNumber < 18362)
					_AllowDarkModeForApp = reinterpret_cast<fnAllowDarkModeForApp>(ord135);
				else
					_SetPreferredAppMode = reinterpret_cast<fnSetPreferredAppMode>(ord135);
				_FlushMenuThemes = reinterpret_cast<fnFlushMenuThemes>(GetProcAddress(hUxtheme, MAKEINTRESOURCEA(136)));
				_IsDarkModeAllowedForWindow = reinterpret_cast<fnIsDarkModeAllowedForWindow>(GetProcAddress(hUxtheme, MAKEINTRESOURCEA(137)));
				HMODULE hUser32Module = GetModuleHandleW(L"user32.dll");
				if(hUser32Module) {
					_SetWindowCompositionAttribute = reinterpret_cast<fnSetWindowCompositionAttribute>(GetProcAddress(hUser32Module, "SetWindowCompositionAttribute"));
				}
				if(_OpenNcThemeData && _RefreshImmersiveColorPolicyState && _ShouldAppsUseDarkMode &&
				    _AllowDarkModeForWindow && (_AllowDarkModeForApp || _SetPreferredAppMode) &&
				    _FlushMenuThemes && _IsDarkModeAllowedForWindow) {
					g_darkModeSupported = true;
				}
			}
		}
	}
}

void SetDarkMode(bool useDark, bool fixDarkScrollbar)
{
	if(g_darkModeSupported) {
		AllowDarkModeForApp(useDark);
		//_RefreshImmersiveColorPolicyState();
		FlushMenuThemes();
		if(fixDarkScrollbar) {
			FixDarkScrollBar();
		}
		g_darkModeEnabled = ShouldAppsUseDarkMode() && !IsHighContrast();
	}
}
