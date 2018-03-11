// LoadCodecs.cpp

/*
   EXTERNAL_CODECS
   ---------------
   CCodecs::Load() tries to detect the directory with plugins.
   It stops the checking, if it can find any of the following items:
    - 7z.dll file
    - "Formats" subdir
    - "Codecs"  subdir
   The order of check:
    1) directory of client executable
    2) WIN32: directory for REGISTRY item [HKEY_*\Software\7-Zip\Path**]
       The order for HKEY_* : Path** :
         - HKEY_CURRENT_USER  : PathXX
         - HKEY_LOCAL_MACHINE : PathXX
         - HKEY_CURRENT_USER  : Path
         - HKEY_LOCAL_MACHINE : Path
       PathXX is Path32 in 32-bit code
       PathXX is Path64 in 64-bit code


   EXPORT_CODECS
   -------------
   if(EXTERNAL_CODECS) is defined, then the code exports internal
   codecs of client from CCodecs object to external plugins.
   7-Zip doesn't use that feature. 7-Zip uses the scheme:
    - client application without internal plugins.
    - 7z.dll module contains all (or almost all) plugins.
      7z.dll can use codecs from another plugins, if required.
 */
#include <7z-internal.h>
#pragma hdrstop
#include <7z-ifcs.h>

using namespace NWindows;

#ifdef EXTERNAL_CODECS
	// #define EXPORT_CODECS
#endif
#ifdef NEW_FOLDER_INTERFACE
	extern HINSTANCE g_hInstance;
	//#include <Windows/ResourceString.h>
	static const UINT kIconTypesResId = 100;
#endif
#ifdef EXTERNAL_CODECS
//#include <Windows/FileFind.h>
//#include <Windows/DLL.h>
#ifdef _WIN32
	//#include <Windows/FileName.h>
	#include <Windows/Registry.h>
#endif

using namespace NFile;

#define kCodecsFolderName FTEXT("Codecs")
#define kFormatsFolderName FTEXT("Formats")

static CFSTR const kMainDll =
    // #ifdef _WIN32
    FTEXT("7z.dll");
// #else
// FTEXT("7z.so");
// #endif

#ifdef _WIN32

static LPCTSTR const kRegistryPath = TEXT("Software") TEXT(STRING_PATH_SEPARATOR) TEXT("7-zip");
static LPCWSTR const kProgramPathValue = L"Path";
static LPCWSTR const kProgramPath2Value = L"Path"
  #ifdef _WIN64
    L"64";
  #else
    L"32";
  #endif

static bool ReadPathFromRegistry(HKEY baseKey, LPCWSTR value, FString &path)
{
	NRegistry::CKey key;
	if(key.Open(baseKey, kRegistryPath, KEY_READ) == ERROR_SUCCESS) {
		UString pathU;
		if(key.QueryValue(value, pathU) == ERROR_SUCCESS) {
			path = us2fs(pathU);
			NName::NormalizeDirPathPrefix(path);
			return NFind::DoesFileExist(path + kMainDll);
		}
	}
	return false;
}

#endif // _WIN32

#endif // EXTERNAL_CODECS

static const  uint kNumArcsMax = 64;
static uint   g_NumArcs = 0;
static const  CArcInfo * g_Arcs[kNumArcsMax];

void RegisterArc(const CArcInfo * arcInfo) throw()
{
	if(g_NumArcs < kNumArcsMax) {
		g_Arcs[g_NumArcs] = arcInfo;
		g_NumArcs++;
	}
}

static void SplitString(const UString &srcString, UStringVector &destStrings)
{
	destStrings.Clear();
	UString s;
	uint len = srcString.Len();
	if(len == 0)
		return;
	for(uint i = 0; i < len; i++) {
		wchar_t c = srcString[i];
		if(c == L' ') {
			if(!s.IsEmpty()) {
				destStrings.Add(s);
				s.Empty();
			}
		}
		else
			s += c;
	}
	if(!s.IsEmpty())
		destStrings.Add(s);
}

CArcInfoEx::CArcInfoEx() : Flags(0), CreateInArchive(NULL), IsArcFunc(NULL)
#ifndef _SFX
, CreateOutArchive(NULL), UpdateEnabled(false), NewInterface(false)/*, Version(0)*/, SignatureOffset(0)
#endif
#ifdef EXTERNAL_CODECS
, LibIndex(-1)
#endif
{
}

bool CArcInfoEx::Flags_KeepName() const { return (Flags & NArcInfoFlags::kKeepName) != 0; }
bool CArcInfoEx::Flags_FindSignature() const { return (Flags & NArcInfoFlags::kFindSignature) != 0; }
bool CArcInfoEx::Flags_AltStreams() const { return (Flags & NArcInfoFlags::kAltStreams) != 0; }
bool CArcInfoEx::Flags_NtSecure() const { return (Flags & NArcInfoFlags::kNtSecure) != 0; }
bool CArcInfoEx::Flags_SymLinks() const { return (Flags & NArcInfoFlags::kSymLinks) != 0; }
bool CArcInfoEx::Flags_HardLinks() const { return (Flags & NArcInfoFlags::kHardLinks) != 0; }
bool CArcInfoEx::Flags_UseGlobalOffset() const { return (Flags & NArcInfoFlags::kUseGlobalOffset) != 0; }
bool CArcInfoEx::Flags_StartOpen() const { return (Flags & NArcInfoFlags::kStartOpen) != 0; }
bool CArcInfoEx::Flags_BackwardOpen() const { return (Flags & NArcInfoFlags::kBackwardOpen) != 0; }
bool CArcInfoEx::Flags_PreArc() const { return (Flags & NArcInfoFlags::kPreArc) != 0; }
bool CArcInfoEx::Flags_PureStartOpen() const { return (Flags & NArcInfoFlags::kPureStartOpen) != 0; }
UString CArcInfoEx::GetMainExt() const { return Exts.IsEmpty() ? UString() : Exts[0].Ext; }

int CArcInfoEx::FindExtension(const UString &ext) const
{
	FOR_VECTOR(i, Exts) {
		if(ext.IsEqualTo_NoCase(Exts[i].Ext))
			return i;
	}
	return -1;
}

void CArcInfoEx::AddExts(const UString &ext, const UString &addExt)
{
	UStringVector exts, addExts;
	SplitString(ext, exts);
	SplitString(addExt, addExts);
	FOR_VECTOR(i, exts) {
		CArcExtInfo extInfo;
		extInfo.Ext = exts[i];
		if(i < addExts.Size()) {
			extInfo.AddExt = addExts[i];
			if(extInfo.AddExt == L"*")
				extInfo.AddExt.Empty();
		}
		Exts.Add(extInfo);
	}
}

#ifndef _SFX
	static bool ParseSignatures(const Byte * data, uint size, CObjectVector <CByteBuffer> &signatures)
	{
		signatures.Clear();
		while(size > 0) {
			uint len = *data++;
			size--;
			if(len > size)
				return false;
			signatures.AddNew().CopyFrom(data, len);
			data += len;
			size -= len;
		}
		return true;
	}
#endif // _SFX

CCodecs::CCodecs() :
#ifdef EXTERNAL_CODECS
	NeedSetLibCodecs(true),
#endif
	CaseSensitiveChange(false), CaseSensitive(false)
{
}

CCodecs::~CCodecs()
{
	// OutputDebugStringA("~CCodecs");
}

const wchar_t * FASTCALL CCodecs::GetFormatNamePtr(int formatIndex) const { return formatIndex < 0 ? L"#" : (const wchar_t*)Formats[formatIndex].Name; }

HRESULT CCodecs::CreateInArchive(unsigned formatIndex, CMyComPtr<IInArchive> &archive) const
{
	const CArcInfoEx &ai = Formats[formatIndex];
#ifdef EXTERNAL_CODECS
	if(ai.LibIndex < 0)
#endif
	{
		COM_TRY_BEGIN
			archive = ai.CreateInArchive();
		return S_OK;
		COM_TRY_END
	}
#ifdef EXTERNAL_CODECS
	return CreateArchiveHandler(ai, false, (void**)&archive);
#endif
}

#ifndef _SFX
	HRESULT CCodecs::CreateOutArchive(unsigned formatIndex, CMyComPtr<IOutArchive> &archive) const
	{
		const CArcInfoEx &ai = Formats[formatIndex];
#ifdef EXTERNAL_CODECS
		if(ai.LibIndex < 0)
#endif
		{
			COM_TRY_BEGIN
				archive = ai.CreateOutArchive();
			return S_OK;
			COM_TRY_END
		}
#ifdef EXTERNAL_CODECS
		return CreateArchiveHandler(ai, true, (void**)&archive);
#endif
	}

	int CCodecs::FindOutFormatFromName(const UString &name) const
	{
		FOR_VECTOR(i, Formats) {
			const CArcInfoEx & arc = Formats[i];
			if(arc.UpdateEnabled) {
				if(arc.Name.IsEqualTo_NoCase(name))
					return i;
			}
		}
		return -1;
	}
#endif // _SFX

#ifdef EXTERNAL_CODECS

static FString GetBaseFolderPrefixFromRegistry()
{
	FString moduleFolderPrefix = NDLL::GetModuleDirPrefix();
  #ifdef _WIN32
	if(!NFind::DoesFileExist(moduleFolderPrefix + kMainDll) &&
	    !NFind::DoesDirExist(moduleFolderPrefix + kCodecsFolderName) &&
	    !NFind::DoesDirExist(moduleFolderPrefix + kFormatsFolderName)) {
		FString path;
		if(ReadPathFromRegistry(HKEY_CURRENT_USER,  kProgramPath2Value, path)) return path;
		if(ReadPathFromRegistry(HKEY_LOCAL_MACHINE, kProgramPath2Value, path)) return path;
		if(ReadPathFromRegistry(HKEY_CURRENT_USER,  kProgramPathValue,  path)) return path;
		if(ReadPathFromRegistry(HKEY_LOCAL_MACHINE, kProgramPathValue,  path)) return path;
	}
  #endif
	return moduleFolderPrefix;
}

static HRESULT GetCoderClass(Func_GetMethodProperty getMethodProperty, uint32 index, PROPID propId, CLSID &clsId, bool &isAssigned)
{
	NCOM::CPropVariant prop;
	isAssigned = false;
	RINOK(getMethodProperty(index, propId, &prop));
	if(prop.vt == VT_BSTR) {
		if(::SysStringByteLen(prop.bstrVal) != sizeof(GUID))
			return E_FAIL;
		isAssigned = true;
		clsId = *(const GUID*)prop.bstrVal;
	}
	else if(prop.vt != VT_EMPTY)
		return E_FAIL;
	return S_OK;
}

HRESULT CCodecs::LoadCodecs()
{
	CCodecLib &lib = Libs.Back();
	lib.CreateDecoder = (Func_CreateDecoder)lib.Lib.GetProc("CreateDecoder");
	lib.CreateEncoder = (Func_CreateEncoder)lib.Lib.GetProc("CreateEncoder");
	lib.GetMethodProperty = (Func_GetMethodProperty)lib.Lib.GetProc("GetMethodProperty");

	if(lib.GetMethodProperty) {
		uint32 numMethods = 1;
		Func_GetNumberOfMethods getNumberOfMethods = (Func_GetNumberOfMethods)lib.Lib.GetProc("GetNumberOfMethods");
		if(getNumberOfMethods) {
			RINOK(getNumberOfMethods(&numMethods));
		}
		for(uint32 i = 0; i < numMethods; i++) {
			CDllCodecInfo info;
			info.LibIndex = Libs.Size() - 1;
			info.CodecIndex = i;
			RINOK(GetCoderClass(lib.GetMethodProperty, i, NMethodPropID::kEncoder, info.Encoder, info.EncoderIsAssigned));
			RINOK(GetCoderClass(lib.GetMethodProperty, i, NMethodPropID::kDecoder, info.Decoder, info.DecoderIsAssigned));
			Codecs.Add(info);
		}
	}
	Func_GetHashers getHashers = (Func_GetHashers)lib.Lib.GetProc("GetHashers");
	if(getHashers) {
		RINOK(getHashers(&lib.ComHashers));
		if(lib.ComHashers) {
			uint32 numMethods = lib.ComHashers->GetNumHashers();
			for(uint32 i = 0; i < numMethods; i++) {
				CDllHasherInfo info;
				info.LibIndex = Libs.Size() - 1;
				info.HasherIndex = i;
				Hashers.Add(info);
			}
		}
	}
	return S_OK;
}

static HRESULT GetProp(Func_GetHandlerProperty getProp, Func_GetHandlerProperty2 getProp2, uint32 index, PROPID propID, NCOM::CPropVariant &prop)
{
	return getProp2 ? getProp2(index, propID, &prop) : getProp(propID, &prop);
}

static HRESULT GetProp_Bool(Func_GetHandlerProperty getProp, Func_GetHandlerProperty2 getProp2, uint32 index, PROPID propID, bool &res)
{
	res = false;
	NCOM::CPropVariant prop;
	RINOK(GetProp(getProp, getProp2, index, propID, prop));
	if(prop.vt == VT_BOOL)
		res = VARIANT_BOOLToBool(prop.boolVal);
	else if(prop.vt != VT_EMPTY)
		return E_FAIL;
	return S_OK;
}

static HRESULT GetProp_UInt32(Func_GetHandlerProperty getProp, Func_GetHandlerProperty2 getProp2, uint32 index, PROPID propID, uint32 &res, bool &defined)
{
	res = 0;
	defined = false;
	NCOM::CPropVariant prop;
	RINOK(GetProp(getProp, getProp2, index, propID, prop));
	if(prop.vt == VT_UI4) {
		res = prop.ulVal;
		defined = true;
	}
	else if(prop.vt != VT_EMPTY)
		return E_FAIL;
	return S_OK;
}

static HRESULT GetProp_String(Func_GetHandlerProperty getProp, Func_GetHandlerProperty2 getProp2, uint32 index, PROPID propID, UString &res)
{
	res.Empty();
	NCOM::CPropVariant prop;
	RINOK(GetProp(getProp, getProp2, index, propID, prop));
	if(prop.vt == VT_BSTR)
		res.SetFromBstr(prop.bstrVal);
	else if(prop.vt != VT_EMPTY)
		return E_FAIL;
	return S_OK;
}

static HRESULT GetProp_RawData(Func_GetHandlerProperty getProp, Func_GetHandlerProperty2 getProp2, uint32 index, PROPID propID, CByteBuffer &bb)
{
	bb.Free();
	NCOM::CPropVariant prop;
	RINOK(GetProp(getProp, getProp2, index, propID, prop));
	if(prop.vt == VT_BSTR) {
		UINT len = ::SysStringByteLen(prop.bstrVal);
		bb.CopyFrom((const Byte*)prop.bstrVal, len);
	}
	else if(prop.vt != VT_EMPTY)
		return E_FAIL;
	return S_OK;
}

static const uint32 kArcFlagsPars[] = {
	NArchive::NHandlerPropID::kKeepName, NArcInfoFlags::kKeepName,
	NArchive::NHandlerPropID::kAltStreams, NArcInfoFlags::kAltStreams,
	NArchive::NHandlerPropID::kNtSecure, NArcInfoFlags::kNtSecure
};

HRESULT CCodecs::LoadFormats()
{
	const NDLL::CLibrary &lib = Libs.Back().Lib;
	Func_GetHandlerProperty getProp = NULL;
	Func_GetHandlerProperty2 getProp2 = (Func_GetHandlerProperty2)lib.GetProc("GetHandlerProperty2");
	Func_GetIsArc getIsArc = (Func_GetIsArc)lib.GetProc("GetIsArc");
	uint32 numFormats = 1;
	if(getProp2) {
		Func_GetNumberOfFormats getNumberOfFormats = (Func_GetNumberOfFormats)lib.GetProc("GetNumberOfFormats");
		if(getNumberOfFormats) {
			RINOK(getNumberOfFormats(&numFormats));
		}
	}
	else {
		getProp = (Func_GetHandlerProperty)lib.GetProc("GetHandlerProperty");
		if(!getProp)
			return S_OK;
	}
	for(uint32 i = 0; i < numFormats; i++) {
		CArcInfoEx item;
		item.LibIndex = Libs.Size() - 1;
		item.FormatIndex = i;
		RINOK(GetProp_String(getProp, getProp2, i, NArchive::NHandlerPropID::kName, item.Name));
		{
			NCOM::CPropVariant prop;
			if(GetProp(getProp, getProp2, i, NArchive::NHandlerPropID::kClassID, prop) != S_OK)
				continue;
			if(prop.vt != VT_BSTR)
				continue;
			if(::SysStringByteLen(prop.bstrVal) != sizeof(GUID))
				return E_FAIL;
			item.ClassID = *(const GUID*)prop.bstrVal;
			prop.Clear();
		}
		UString ext, addExt;
		RINOK(GetProp_String(getProp, getProp2, i, NArchive::NHandlerPropID::kExtension, ext));
		RINOK(GetProp_String(getProp, getProp2, i, NArchive::NHandlerPropID::kAddExtension, addExt));
		item.AddExts(ext, addExt);
		GetProp_Bool(getProp, getProp2, i, NArchive::NHandlerPropID::kUpdate, item.UpdateEnabled);
		bool flags_Defined = false;
		RINOK(GetProp_UInt32(getProp, getProp2, i, NArchive::NHandlerPropID::kFlags, item.Flags, flags_Defined));
		item.NewInterface = flags_Defined;
		if(!flags_Defined) { // && item.UpdateEnabled
			// support for DLL version before 9.31:
			for(uint j = 0; j < ARRAY_SIZE(kArcFlagsPars); j += 2) {
				bool val = false;
				GetProp_Bool(getProp, getProp2, i, kArcFlagsPars[j], val);
				if(val)
					item.Flags |= kArcFlagsPars[j + 1];
			}
		}
		CByteBuffer sig;
		RINOK(GetProp_RawData(getProp, getProp2, i, NArchive::NHandlerPropID::kSignature, sig));
		if(sig.Size() != 0)
			item.Signatures.Add(sig);
		else {
			RINOK(GetProp_RawData(getProp, getProp2, i, NArchive::NHandlerPropID::kMultiSignature, sig));
			ParseSignatures(sig, (uint)sig.Size(), item.Signatures);
		}
		bool signatureOffset_Defined;
		RINOK(GetProp_UInt32(getProp, getProp2, i, NArchive::NHandlerPropID::kSignatureOffset, item.SignatureOffset, signatureOffset_Defined));
		// bool version_Defined;
		// RINOK(GetProp_UInt32(getProp, getProp2, i, NArchive::NHandlerPropID::kVersion, item.Version,
		// version_Defined));
		if(getIsArc)
			getIsArc(i, &item.IsArcFunc);
		Formats.Add(item);
	}
	return S_OK;
}

#ifdef _7ZIP_LARGE_PAGES
	extern "C" {
		extern SIZE_T g_LargePageSize;
	}
#endif

HRESULT CCodecs::LoadDll(const FString &dllPath, bool needCheckDll, bool * loadedOK)
{
	ASSIGN_PTR(loadedOK, false);
	if(needCheckDll) {
		NDLL::CLibrary lib;
		if(!lib.LoadEx(dllPath, LOAD_LIBRARY_AS_DATAFILE))
			return S_OK;
	}
	Libs.AddNew();
	CCodecLib &lib = Libs.Back();
	lib.Path = dllPath;
	bool used = false;
	HRESULT res = S_OK;
	if(lib.Lib.Load(dllPath)) {
		ASSIGN_PTR(loadedOK, true);
    #ifdef NEW_FOLDER_INTERFACE
		lib.LoadIcons();
    #endif
    #ifdef _7ZIP_LARGE_PAGES
		if(g_LargePageSize != 0) {
			Func_SetLargePageMode setLargePageMode = (Func_SetLargePageMode)lib.Lib.GetProc("SetLargePageMode");
			if(setLargePageMode)
				setLargePageMode();
		}
    #endif
		if(CaseSensitiveChange) {
			Func_SetCaseSensitive setCaseSensitive = (Func_SetCaseSensitive)lib.Lib.GetProc("SetCaseSensitive");
			if(setCaseSensitive)
				setCaseSensitive(CaseSensitive ? 1 : 0);
		}
		lib.CreateObject = (Func_CreateObject)lib.Lib.GetProc("CreateObject");
		{
			uint   startSize = Codecs.Size() + Hashers.Size();
			res = LoadCodecs();
			used = (startSize != Codecs.Size() + Hashers.Size());
			if(res == S_OK && lib.CreateObject) {
				startSize = Formats.Size();
				res = LoadFormats();
				if(startSize != Formats.Size())
					used = true;
			}
		}
	}
	if(!used)
		Libs.DeleteBack();
	return res;
}

HRESULT CCodecs::LoadDllsFromFolder(const FString &folderPrefix)
{
	NFile::NFind::CEnumerator enumerator;
	enumerator.SetDirPrefix(folderPrefix);
	NFile::NFind::CFileInfo fi;
	while(enumerator.Next(fi)) {
		if(!fi.IsDir()) {
			RINOK(LoadDll(folderPrefix + fi.Name, true));
		}
	}
	return S_OK;
}

void CCodecs::CloseLibs()
{
	// OutputDebugStringA("~CloseLibs start");
	/*
	   WIN32: FreeLibrary() (CLibrary::Free()) function doesn't work as expected,
	   if it's called from another FreeLibrary() call.
	   So we need to call FreeLibrary() before global destructors.

	   Also we free global links from DLLs to object of this module before CLibrary::Free() call.
	 */
	FOR_VECTOR(i, Libs) {
		const CCodecLib &lib = Libs[i];
		if(lib.SetCodecs)
			lib.SetCodecs(NULL);
	}
	// OutputDebugStringA("~CloseLibs after SetCodecs");
	Libs.Clear();
	// OutputDebugStringA("~CloseLibs end");
}

#endif // EXTERNAL_CODECS

HRESULT CCodecs::Load()
{
  #ifdef NEW_FOLDER_INTERFACE
	InternalIcons.LoadIcons(g_hInstance);
  #endif
	Formats.Clear();
  #ifdef EXTERNAL_CODECS
	MainDll_ErrorPath.Empty();
	Codecs.Clear();
	Hashers.Clear();
  #endif
	for(uint32 i = 0; i < g_NumArcs; i++) {
		const CArcInfo &arc = *g_Arcs[i];
		CArcInfoEx item;
		item.Name = arc.Name;
		item.CreateInArchive = arc.CreateInArchive;
		item.IsArcFunc = arc.IsArc;
		item.Flags = arc.Flags;
		{
			UString e, ae;
			if(arc.Ext)
				e = arc.Ext;
			if(arc.AddExt)
				ae = arc.AddExt;
			item.AddExts(e, ae);
		}
    #ifndef _SFX
		item.CreateOutArchive = arc.CreateOutArchive;
		item.UpdateEnabled = (arc.CreateOutArchive != NULL);
		item.SignatureOffset = arc.SignatureOffset;
		// item.Version = MY_VER_MIX;
		item.NewInterface = true;
		if(arc.IsMultiSignature())
			ParseSignatures(arc.Signature, arc.SignatureSize, item.Signatures);
		else
			item.Signatures.AddNew().CopyFrom(arc.Signature, arc.SignatureSize);
    #endif
		Formats.Add(item);
	}

  #ifdef EXTERNAL_CODECS
	const FString baseFolder = GetBaseFolderPrefixFromRegistry();
	{
		bool loadedOK;
		RINOK(LoadDll(baseFolder + kMainDll, false, &loadedOK));
		if(!loadedOK)
			MainDll_ErrorPath = kMainDll;
	}
	RINOK(LoadDllsFromFolder(baseFolder + kCodecsFolderName FSTRING_PATH_SEPARATOR));
	RINOK(LoadDllsFromFolder(baseFolder + kFormatsFolderName FSTRING_PATH_SEPARATOR));

	NeedSetLibCodecs = true;

	if(Libs.Size() == 0)
		NeedSetLibCodecs = false;
	else if(Libs.Size() == 1) {
		// we don't need to set ISetCompressCodecsInfo, if all arcs and codecs are in one external module.
    #ifndef EXPORT_CODECS
		if(g_NumArcs == 0)
			NeedSetLibCodecs = false;
    #endif
	}

	if(NeedSetLibCodecs) {
		/* 15.00: now we call global function in DLL: SetCompressCodecsInfo(c)
		   old versions called only ISetCompressCodecsInfo::SetCompressCodecsInfo(c) for each archive handler */
		FOR_VECTOR(i, Libs) {
			CCodecLib &lib = Libs[i];
			lib.SetCodecs = (Func_SetCodecs)lib.Lib.GetProc("SetCodecs");
			if(lib.SetCodecs) {
				RINOK(lib.SetCodecs(this));
			}
		}
	}
  #endif
	return S_OK;
}

#ifndef _SFX
	int CCodecs::FindFormatForArchiveName(const UString &arcPath) const
	{
		int dotPos = arcPath.ReverseFind_Dot();
		if(dotPos > arcPath.ReverseFind_PathSepar()) {
			const UString ext = arcPath.Ptr(dotPos + 1);
			if(!ext.IsEmpty()) {
				if(!ext.IsEqualTo_Ascii_NoCase("exe")) {
					FOR_VECTOR(i, Formats) {
						const CArcInfoEx & arc = Formats[i];
						/*
						if(!arc.UpdateEnabled)
						continue;
						*/
						if(arc.FindExtension(ext) >= 0)
							return i;
					}
				}
			}
		}
		return -1;
	}

	int CCodecs::FindFormatForExtension(const UString &ext) const
	{
		if(!ext.IsEmpty()) {
			FOR_VECTOR(i, Formats) {
				if(Formats[i].FindExtension(ext) >= 0)
					return i;
			}
		}
		return -1;
	}

	int CCodecs::FindFormatForArchiveType(const UString &arcType) const
	{
		FOR_VECTOR(i, Formats) {
			if(Formats[i].Name.IsEqualTo_NoCase(arcType))
				return i;
		}
		return -1;
	}

	bool CCodecs::FindFormatForArchiveType(const UString &arcType, CIntVector &formatIndices) const
	{
		formatIndices.Clear();
		for(uint pos = 0; pos < arcType.Len(); ) {
			int pos2 = arcType.Find(L'.', pos);
			if(pos2 < 0)
				pos2 = arcType.Len();
			const UString name = arcType.Mid(pos, pos2 - pos);
			if(name.IsEmpty())
				return false;
			int index = FindFormatForArchiveType(name);
			if(index < 0 && name != L"*") {
				formatIndices.Clear();
				return false;
			}
			formatIndices.Add(index);
			pos = pos2 + 1;
		}
		return true;
	}
#endif // _SFX

#ifdef NEW_FOLDER_INTERFACE
	void CCodecIcons::LoadIcons(HMODULE m)
	{
		UString iconTypes;
		MyLoadString(m, kIconTypesResId, iconTypes);
		UStringVector pairs;
		SplitString(iconTypes, pairs);
		FOR_VECTOR(i, pairs) {
			const UString &s = pairs[i];
			int pos = s.Find(L':');
			CIconPair iconPair;
			iconPair.IconIndex = -1;
			if(pos < 0)
				pos = s.Len();
			else {
				UString num = s.Ptr(pos + 1);
				if(!num.IsEmpty()) {
					const wchar_t * end;
					iconPair.IconIndex = ConvertStringToUInt32(num, &end);
					if(*end != 0)
						continue;
				}
			}
			iconPair.Ext = s.Left(pos);
			IconPairs.Add(iconPair);
		}
	}

	bool CCodecIcons::FindIconIndex(const UString & ext, int & iconIndex) const
	{
		iconIndex = -1;
		FOR_VECTOR(i, IconPairs) {
			const CIconPair &pair = IconPairs[i];
			if(ext.IsEqualTo_NoCase(pair.Ext)) {
				iconIndex = pair.IconIndex;
				return true;
			}
		}
		return false;
	}
#endif // NEW_FOLDER_INTERFACE
#ifdef EXTERNAL_CODECS

// #define EXPORT_CODECS
#ifdef EXPORT_CODECS
	extern uint    g_NumCodecs;
	STDAPI CreateDecoder(uint32 index, const GUID * iid, void ** outObject);
	STDAPI CreateEncoder(uint32 index, const GUID * iid, void ** outObject);
	STDAPI GetMethodProperty(uint32 codecIndex, PROPID propID, PROPVARIANT * value);
	#define NUM_EXPORT_CODECS g_NumCodecs

	extern uint    g_NumHashers;
	STDAPI CreateHasher(uint32 index, IHasher ** hasher);
	STDAPI GetHasherProp(uint32 codecIndex, PROPID propID, PROPVARIANT * value);
	#define NUM_EXPORT_HASHERS g_NumHashers
#else // EXPORT_CODECS
	#define NUM_EXPORT_CODECS 0
	#define NUM_EXPORT_HASHERS 0
#endif // EXPORT_CODECS

STDMETHODIMP CCodecs::GetNumMethods(uint32 * numMethods)
{
	*numMethods = NUM_EXPORT_CODECS
    #ifdef EXTERNAL_CODECS
	    + Codecs.Size()
    #endif
	;
	return S_OK;
}

STDMETHODIMP CCodecs::GetProperty(uint32 index, PROPID propID, PROPVARIANT * value)
{
  #ifdef EXPORT_CODECS
	if(index < g_NumCodecs)
		return GetMethodProperty(index, propID, value);
  #endif
  #ifdef EXTERNAL_CODECS
	const CDllCodecInfo &ci = Codecs[index - NUM_EXPORT_CODECS];
	if(oneof2(propID, NMethodPropID::kDecoderIsAssigned, NMethodPropID::kEncoderIsAssigned)) {
		NCOM::CPropVariant prop;
		prop = (bool)((propID == NMethodPropID::kDecoderIsAssigned) ? ci.DecoderIsAssigned : ci.EncoderIsAssigned);
		prop.Detach(value);
		return S_OK;
	}
	const CCodecLib &lib = Libs[ci.LibIndex];
	return lib.GetMethodProperty(ci.CodecIndex, propID, value);
  #else
	return E_FAIL;
  #endif
}

STDMETHODIMP CCodecs::CreateDecoder(uint32 index, const GUID * iid, void ** coder)
{
  #ifdef EXPORT_CODECS
	if(index < g_NumCodecs)
		return CreateDecoder(index, iid, coder);
  #endif
  #ifdef EXTERNAL_CODECS
	const CDllCodecInfo &ci = Codecs[index - NUM_EXPORT_CODECS];
	if(ci.DecoderIsAssigned) {
		const CCodecLib &lib = Libs[ci.LibIndex];
		if(lib.CreateDecoder)
			return lib.CreateDecoder(ci.CodecIndex, iid, (void**)coder);
		if(lib.CreateObject)
			return lib.CreateObject(&ci.Decoder, iid, (void**)coder);
	}
	return S_OK;
  #else
	return E_FAIL;
  #endif
}

STDMETHODIMP CCodecs::CreateEncoder(uint32 index, const GUID * iid, void ** coder)
{
  #ifdef EXPORT_CODECS
	if(index < g_NumCodecs)
		return CreateEncoder(index, iid, coder);
  #endif
  #ifdef EXTERNAL_CODECS
	const CDllCodecInfo &ci = Codecs[index - NUM_EXPORT_CODECS];
	if(ci.EncoderIsAssigned) {
		const CCodecLib &lib = Libs[ci.LibIndex];
		if(lib.CreateEncoder)
			return lib.CreateEncoder(ci.CodecIndex, iid, (void**)coder);
		if(lib.CreateObject)
			return lib.CreateObject(&ci.Encoder, iid, (void**)coder);
	}
	return S_OK;
  #else
	return E_FAIL;
  #endif
}

STDMETHODIMP_(uint32) CCodecs::GetNumHashers()
{
	return NUM_EXPORT_HASHERS
    #ifdef EXTERNAL_CODECS
	       + Hashers.Size()
    #endif
	;
}

STDMETHODIMP CCodecs::GetHasherProp(uint32 index, PROPID propID, PROPVARIANT * value)
{
  #ifdef EXPORT_CODECS
	if(index < g_NumHashers)
		return ::GetHasherProp(index, propID, value);
  #endif
  #ifdef EXTERNAL_CODECS
	const CDllHasherInfo &ci = Hashers[index - NUM_EXPORT_HASHERS];
	return Libs[ci.LibIndex].ComHashers->GetHasherProp(ci.HasherIndex, propID, value);
  #else
	return E_FAIL;
  #endif
}

STDMETHODIMP CCodecs::CreateHasher(uint32 index, IHasher ** hasher)
{
  #ifdef EXPORT_CODECS
	if(index < g_NumHashers)
		return CreateHasher(index, hasher);
  #endif
  #ifdef EXTERNAL_CODECS
	const CDllHasherInfo &ci = Hashers[index - NUM_EXPORT_HASHERS];
	return Libs[ci.LibIndex].ComHashers->CreateHasher(ci.HasherIndex, hasher);
  #else
	return E_FAIL;
  #endif
}

int CCodecs::GetCodec_LibIndex(uint32 index) const
{
  #ifdef EXPORT_CODECS
	if(index < g_NumCodecs)
		return -1;
  #endif
  #ifdef EXTERNAL_CODECS
	const CDllCodecInfo &ci = Codecs[index - NUM_EXPORT_CODECS];
	return ci.LibIndex;
  #else
	return -1;
  #endif
}

int CCodecs::GetHasherLibIndex(uint32 index)
{
  #ifdef EXPORT_CODECS
	if(index < g_NumHashers)
		return -1;
  #endif
  #ifdef EXTERNAL_CODECS
	const CDllHasherInfo &ci = Hashers[index - NUM_EXPORT_HASHERS];
	return ci.LibIndex;
  #else
	return -1;
  #endif
}

bool CCodecs::GetCodec_DecoderIsAssigned(uint32 index) const
{
  #ifdef EXPORT_CODECS
	if(index < g_NumCodecs) {
		NCOM::CPropVariant prop;
		if(GetProperty(index, NMethodPropID::kDecoderIsAssigned, &prop) == S_OK) {
			if(prop.vt == VT_BOOL)
				return VARIANT_BOOLToBool(prop.boolVal);
		}
		return false;
	}
  #endif
  #ifdef EXTERNAL_CODECS
	return Codecs[index - NUM_EXPORT_CODECS].DecoderIsAssigned;
  #else
	return false;
  #endif
}

bool CCodecs::GetCodec_EncoderIsAssigned(uint32 index) const
{
  #ifdef EXPORT_CODECS
	if(index < g_NumCodecs) {
		NCOM::CPropVariant prop;
		if(GetProperty(index, NMethodPropID::kEncoderIsAssigned, &prop) == S_OK) {
			if(prop.vt == VT_BOOL)
				return VARIANT_BOOLToBool(prop.boolVal);
		}
		return false;
	}
  #endif
  #ifdef EXTERNAL_CODECS
	return Codecs[index - NUM_EXPORT_CODECS].EncoderIsAssigned;
  #else
	return false;
  #endif
}

uint32 CCodecs::GetCodec_NumStreams(uint32 index)
{
	NCOM::CPropVariant prop;
	RINOK(GetProperty(index, NMethodPropID::kPackStreams, &prop));
	if(prop.vt == VT_UI4)
		return (uint32)prop.ulVal;
	else if(prop.vt == VT_EMPTY)
		return 1;
	else
		return 0;
}

HRESULT CCodecs::GetCodec_Id(uint32 index, uint64 &id)
{
	NCOM::CPropVariant prop;
	RINOK(GetProperty(index, NMethodPropID::kID, &prop));
	if(prop.vt != VT_UI8)
		return E_INVALIDARG;
	id = prop.uhVal.QuadPart;
	return S_OK;
}

AString CCodecs::GetCodec_Name(uint32 index)
{
	AString s;
	NCOM::CPropVariant prop;
	if(GetProperty(index, NMethodPropID::kName, &prop) == S_OK)
		if(prop.vt == VT_BSTR)
			s.SetFromWStr_if_Ascii(prop.bstrVal);
	return s;
}

uint64 CCodecs::GetHasherId(uint32 index)
{
	NCOM::CPropVariant prop;
	if(GetHasherProp(index, NMethodPropID::kID, &prop) != S_OK)
		return 0;
	else if(prop.vt != VT_UI8)
		return 0;
	else
		return prop.uhVal.QuadPart;
}

AString CCodecs::GetHasherName(uint32 index)
{
	AString s;
	NCOM::CPropVariant prop;
	if(GetHasherProp(index, NMethodPropID::kName, &prop) == S_OK)
		if(prop.vt == VT_BSTR)
			s.SetFromWStr_if_Ascii(prop.bstrVal);
	return s;
}

uint32 CCodecs::GetHasherDigestSize(uint32 index)
{
	NCOM::CPropVariant prop;
	RINOK(GetHasherProp(index, NMethodPropID::kDigestSize, &prop));
	return (prop.vt != VT_UI4) ? 0 : prop.ulVal;
}

#endif // EXTERNAL_CODECS
