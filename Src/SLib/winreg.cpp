// WINREG.CPP
// Copyright (c) A.Sobolev 2003, 2005, 2007, 2008, 2010, 2013, 2014, 2016, 2017, 2018, 2019, 2020, 2021, 2023
// @codepage UTF-8
//
#include <slib-internal.h>
#pragma hdrstop
#include <shlwapi.h>

/*static*/SVerT SDynLibrary::GetVersion(const char * pFileName)
{
	//DWORD GetDllVersion(LPCTSTR lpszDllName)
	SVerT result;
	// For security purposes, LoadLibrary should be provided with a
	// fully-qualified path to the DLL. The lpszDllName variable should be
	// tested to ensure that it is a fully qualified path before it is used. 
	SDynLibrary dl(pFileName);
	if(dl.IsValid()) {
		DLLGETVERSIONPROC dll_get_ver_proc = reinterpret_cast<DLLGETVERSIONPROC>(dl.GetProcAddr("DllGetVersion"));
		if(dll_get_ver_proc) {
			DLLVERSIONINFO dvi;
			INITWINAPISTRUCT(dvi);
			HRESULT hr = dll_get_ver_proc(&dvi);
			if(SUCCEEDED(hr)) {
				result.Set(dvi.dwMajorVersion, dvi.dwMinorVersion, 0);
			}
		}
	}
	return result;
}

SDynLibrary::SDynLibrary(const char * pFileName) : H(0)
{
	if(pFileName)
		Load(pFileName);
}

SDynLibrary::~SDynLibrary()
{
	if(H)
		::FreeLibrary(H);
}

int FASTCALL SDynLibrary::Load(const char * pFileName)
{
	if(H) {
		::FreeLibrary(H);
		H = 0;
	}
	H = ::LoadLibrary(SUcSwitch(pFileName));
	if(H)
		return 1;
	else {
		SLS.SetError(SLERR_DLLLOADFAULT, pFileName);
		return 0;
	}
}

bool SDynLibrary::IsValid() const
{
	if(H)
		return true;
	else {
		if(SLS.GetTLA().LastErr != SLERR_DLLLOADFAULT)
			SLS.SetError(SLERR_DLLLOADFAULT, "");
		return false;
	}
}

FARPROC FASTCALL SDynLibrary::GetProcAddr(const char * pProcName)
{
	FARPROC proc = 0;
	if(H) {
		proc = ::GetProcAddress(H, pProcName);
		if(!proc)
			SLS.SetOsError(pProcName);
	}
	return proc;
}

FARPROC STDCALL SDynLibrary::GetProcAddr(const char * pProcName, int unicodeSuffix)
{
	FARPROC proc = 0;
	if(H) {
		if(unicodeSuffix) {
			SString temp_buf(pProcName);
#ifdef UNICODE
			temp_buf.CatChar('W');
#else
			temp_buf.CatChar('A');
#endif
			proc = ::GetProcAddress(H, temp_buf);
		}
		else
			proc = ::GetProcAddress(H, pProcName);
		if(!proc)
			SLS.SetOsError(pProcName);
	}
	return proc;
}
//
//
//
WinRegValue::WinRegValue(size_t bufSize) : Type(0), DataSize(0)
{
	SBaseBuffer::Init();
	Alloc(bufSize);
}

WinRegValue::~WinRegValue()
{
	SBaseBuffer::Destroy();
}

int WinRegValue::Alloc(size_t bufSize)
{
	// @v11.8.7 {
	int    ok = SBaseBuffer::Alloc(bufSize);
	DataSize = MIN(SBaseBuffer::Size, DataSize);
	return ok;
	// } @v11.8.7 
	/* @v11.8.7
	if(bufSize || P_Buf) {
		P_Buf = SAlloc::R(P_Buf, bufSize);
		if(bufSize && P_Buf == 0) {
			BufSize = DataSize = 0;
			return 0;
		}
	}
	BufSize = bufSize;
	DataSize = MIN(BufSize, DataSize);
	return 1;
	*/
}

uint32 WinRegValue::GetDWord() const
{
	if(oneof3(Type, REG_DWORD, REG_DWORD_LITTLE_ENDIAN, REG_DWORD_BIG_ENDIAN))
		if(P_Buf && DataSize >= sizeof(DWORD))
			return static_cast<uint32>(*reinterpret_cast<const DWORD *>(P_Buf));
	return 0;
}

const void * WinRegValue::GetBinary(size_t * pDataLen) const
{
	ASSIGN_PTR(pDataLen, DataSize);
	return P_Buf;
}

int WinRegValue::GetStringUtf8(SString & rBuf) const
{
	int    ok = 0;
	rBuf.Z();
	if(Type == REG_SZ) {
		rBuf = SUcSwitch(reinterpret_cast<const TCHAR *>(P_Buf));
		ok = 1;
	}
	return ok;
}

int WinRegValue::PutDWord(uint32 val)
{
	Type = REG_DWORD;
	if(Alloc(sizeof(DWORD))) {
		*reinterpret_cast<DWORD *>(P_Buf) = val;
		DataSize = sizeof(DWORD);
		return 1;
	}
	else
		return 0;
}

int WinRegValue::PutBinary(const void * pBuf, size_t dataSize)
{
	Type = REG_BINARY;
	if(Alloc(dataSize)) {
		memcpy(P_Buf, pBuf, dataSize);
		DataSize = dataSize;
		return 1;
	}
	else
		return 0;
}

int WinRegValue::PutStringUtf8(const char * pStr)
{
	int    ok = 1;
	Type = REG_SZ;
	SString temp_buf(pStr);
	THROW(temp_buf.IsLegalUtf8()); // @todo @err
	{
		SStringU temp_buf_u;
		temp_buf_u.CopyFromUtf8(temp_buf);
		const size_t len = (temp_buf_u.Len() + 1) * sizeof(wchar_t);
		THROW(Alloc(len));
		memcpy(P_Buf, temp_buf_u.ucptr(), len);
		DataSize = len;
	}
	CATCHZOK
	return ok;
}
//
//
//
WinRegKey::WinRegKey() : Key(0), KeyIsOuter(false)
{
}

WinRegKey::WinRegKey(HKEY key, const char * pSubKey, int readOnly) : Key(0), KeyIsOuter(false)
{
	Open(key, pSubKey, readOnly);
}

WinRegKey::WinRegKey(const char * pKey, int readOnly) : Key(0), KeyIsOuter(false)
{
	Open(pKey, readOnly);
}

WinRegKey::WinRegKey(HKEY key) : Key(key), KeyIsOuter(true)
{
}

WinRegKey::~WinRegKey()
{
	Close();
}

int WinRegKey::Delete(HKEY key, const char * pSubKey)
{
	return (SHDeleteKey(key, SUcSwitch(pSubKey)) == ERROR_SUCCESS) ? 1 : SLS.SetOsError(pSubKey); // @unicodeproblem
}

int WinRegKey::DeleteValue(HKEY key, const char * pSubKey, const char * pValue)
{
	return (SHDeleteValue(key, SUcSwitch(pSubKey), SUcSwitch(pValue)) == ERROR_SUCCESS) ? 1 : SLS.SetOsError(pValue); // @unicodeproblem
}

int WinRegKey::Open(HKEY key, const char * pSubKey, int readOnly, int onlyOpen)
{
	LONG   r = 0;
	DWORD  dispos = 0;
	Close();
	if(onlyOpen)
		r = RegOpenKeyEx(key, SUcSwitch(pSubKey), 0, readOnly ? KEY_READ : (KEY_READ|KEY_WRITE), &Key); // @unicodeproblem
	else
		r = RegCreateKeyEx(key, SUcSwitch(pSubKey), 0, 0, REG_OPTION_NON_VOLATILE, readOnly ? KEY_READ : (KEY_READ|KEY_WRITE), NULL, &Key, &dispos); // @unicodeproblem
	return (r == ERROR_SUCCESS) ? 1 : SLS.SetOsError(pSubKey);
}

int WinRegKey::Open(const char * pKey, int readOnly, int onlyOpen)
{
	int    ok = 0;
	if(!isempty(pKey)) {
		SString temp_buf;
		SString key_buf(pKey);
		StringSet ss_key;
		key_buf.Tokenize("/\\", ss_key);
		if(ss_key.getCount()) {
			uint ssp = 0;
			if(ss_key.get(&ssp, temp_buf)) {
				HKEY root_key = 0;
				if(temp_buf.IsEqiAscii("HKEY_CLASSES_ROOT") || temp_buf.IsEqiAscii("CLASSES_ROOT")) {
					root_key = HKEY_CLASSES_ROOT;
				}
				else if(temp_buf.IsEqiAscii("HKEY_CURRENT_CONFIG") || temp_buf.IsEqiAscii("CURRENT_CONFIG")) {
					root_key = HKEY_CURRENT_CONFIG;
				}
				else if(temp_buf.IsEqiAscii("HKEY_CURRENT_USER") || temp_buf.IsEqiAscii("CURRENT_USER")) {
					root_key = HKEY_CURRENT_USER;
				}
				else if(temp_buf.IsEqiAscii("HKEY_LOCAL_MACHINE") || temp_buf.IsEqiAscii("LOCAL_MACHINE") || temp_buf.IsEqiAscii("MACHINE")) {
					root_key = HKEY_LOCAL_MACHINE;
				}
				else if(temp_buf.IsEqiAscii("HKEY_USERS") || temp_buf.IsEqiAscii("USERS")) {
					root_key = HKEY_USERS;
				}
				if(root_key) {
					key_buf.Z();
					while(ss_key.get(&ssp, temp_buf)) {
						key_buf.CatDivIfNotEmpty('\\', 0).Cat(temp_buf);
					}
					ok = Open(root_key, key_buf, readOnly, onlyOpen);
				}
			}
		}
	}
	return ok;
}

void WinRegKey::Close()
{
	if(Key) {
		if(!KeyIsOuter)
			RegCloseKey(Key);
		Key = 0;
		KeyIsOuter = false;
	}
}

int WinRegKey::GetDWord(const char * pParam, uint32 * pVal)
{
	int    ok = 1;
	if(Key) {
		DWORD  type = 0;
		DWORD  val  = 0;
		DWORD  size = sizeof(val);
		LONG   r = RegQueryValueEx(Key, SUcSwitch(pParam), 0, &type, reinterpret_cast<LPBYTE>(&val), &size); // @unicodeproblem
		if(r == ERROR_SUCCESS && type == REG_DWORD) {
			ASSIGN_PTR(pVal, val);
		}
		else {
			ASSIGN_PTR(pVal, 0);
			ok = SLS.SetOsError(pParam);
		}
	}
	else
		ok = 0;
	return ok;
}

/*int WinRegKey::GetString(const char * pParam, char * pBuf, size_t bufLen)
{
	if(Key == 0)
		return 0;
	DWORD type = 0;
	DWORD size = static_cast<DWORD>(bufLen);
	LONG  r = RegQueryValueEx(Key, SUcSwitch(pParam), 0, &type, (LPBYTE)(pBuf), &size); // @unicodeproblem
	return oneof2(r, ERROR_SUCCESS, ERROR_MORE_DATA) ? 1 : SLS.SetOsError(pParam);
}*/

int WinRegKey::GetString(const char * pParam, SString & rBuf)
{
	int    ok = 1;
	rBuf.Z();
	if(Key == 0)
		ok = 0;
	else {
		DWORD type = 0;
		STempBuffer temp_buf(1024);
		DWORD size = static_cast<DWORD>(temp_buf.GetSize());
		for(LONG  r = ERROR_MORE_DATA; ok && r == ERROR_MORE_DATA;) {
			temp_buf.Alloc(size);
			size = static_cast<DWORD>(temp_buf.GetSize());
			r = RegQueryValueEx(Key, SUcSwitch(pParam), 0, &type, static_cast<LPBYTE>(temp_buf.vptr()), &size/*bytes*/);
			if(oneof2(r, ERROR_SUCCESS, ERROR_MORE_DATA))
				rBuf.CatN(SUcSwitch(static_cast<const TCHAR *>(temp_buf.vcptr())), size / sizeof(TCHAR));
			else
				ok = SLS.SetOsError(pParam);
		}
	}
	return ok;
}

int WinRegKey::GetStringU(const char * pParam, SStringU & rBuf)
{
	int    ok = 1;
	rBuf.Z();
	if(Key == 0)
		ok = 0;
	else {
		DWORD type = 0;
		SStringU temp_buf_u;
		STempBuffer temp_buf(1024);
		DWORD size = static_cast<DWORD>(temp_buf.GetSize());
		SString & r_param_buf = SLS.AcquireRvlStr();
		SStringU & r_param_buf_u = SLS.AcquireRvlStrU();
		(r_param_buf = pParam).CopyToUnicode(r_param_buf_u);
		for(LONG r = ERROR_MORE_DATA; ok && r == ERROR_MORE_DATA;) {
			temp_buf.Alloc(size);
			size = static_cast<DWORD>(temp_buf.GetSize());
			r = RegQueryValueExW(Key, r_param_buf_u, 0, &type, static_cast<LPBYTE>(temp_buf.vptr()), &size/*bytes*/);
			if(oneof2(r, ERROR_SUCCESS, ERROR_MORE_DATA)) {
				rBuf.CatN(static_cast<const wchar_t *>(temp_buf.vcptr()), size/sizeof(wchar_t));
			}
			else
				ok = SLS.SetOsError(pParam);
		}
	}
	return ok;
}

int WinRegKey::GetBinary(const char * pParam, SBuffer & rBuf)
{
	rBuf.Z();
	int    ok = 1;
	size_t rec_size = 0;
	THROW(!!Key);
	if(GetRecSize(pParam, &rec_size) > 0) {
		if(rec_size > 0) {
			STempBuffer tbuf(rec_size);
			THROW(tbuf.IsValid());
			DWORD type = 0;
			DWORD size = (DWORD)rec_size;
			LONG  r = RegQueryValueEx(Key, SUcSwitch(pParam), 0, &type, reinterpret_cast<LPBYTE>(tbuf.vptr()), &size);
			if(r == ERROR_SUCCESS) {
				rBuf.Write(tbuf.cptr(), size);
			}
			else 
				ok = SLS.SetOsError(pParam);
			//return oneof2(r, ERROR_SUCCESS, ERROR_MORE_DATA) ? 1 : SLS.SetOsError(pParam);
		}
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

int WinRegKey::GetBinary(const char * pParam, void * pBuf, size_t bufLen)
{
	if(Key == 0)
		return 0;
	DWORD type = 0;
	DWORD size = (DWORD)bufLen;
	LONG  r = RegQueryValueEx(Key, SUcSwitch(pParam), 0, &type, static_cast<LPBYTE>(pBuf), &size);
	return oneof2(r, ERROR_SUCCESS, ERROR_MORE_DATA) ? 1 : SLS.SetOsError(pParam);
}

int WinRegKey::GetRecSize(const char * pParam, size_t * pRecSize)
{
	if(Key == 0)
		return 0;
	DWORD type = 0;
	DWORD size = 0;
	LONG  r = RegQueryValueEx(Key, SUcSwitch(pParam), 0, &type, 0, &size);
	if(oneof2(r, ERROR_SUCCESS, ERROR_MORE_DATA)) {
		ASSIGN_PTR(pRecSize, size);
		return 1;
	}
	else if(r == ERROR_FILE_NOT_FOUND) {
		ASSIGN_PTR(pRecSize, 0);
		return -1;
	}
	else
		return SLS.SetOsError(pParam);
}

int WinRegKey::PutDWord(const char * pParam, uint32 val)
{
	if(Key == 0)
		return 0;
	LONG   r = RegSetValueEx(Key, SUcSwitch(pParam), 0, REG_DWORD, reinterpret_cast<LPBYTE>(&val), sizeof(val));
	return (r == ERROR_SUCCESS) ? 1 : SLS.SetOsError(pParam);
}

int WinRegKey::PutString(const char * pParam, const char * pBuf)
{
	if(Key == 0)
		return 0;
	const  TCHAR * p_buf_to_store = SUcSwitch(pBuf);
	DWORD  size_to_store = static_cast<DWORD>((sstrlen(pBuf) + 1) * sizeof(TCHAR));
	LONG   r = RegSetValueEx(Key, SUcSwitch(pParam), 0, REG_SZ, reinterpret_cast<const BYTE *>(p_buf_to_store), size_to_store); // @v10.4.5 
	// @v10.4.5 LONG   r = RegSetValueEx(Key, SUcSwitch(pParam), 0, REG_SZ, (LPBYTE)pBuf, (DWORD)(sstrlen(pBuf) + 1));
	return (r == ERROR_SUCCESS) ? 1 : SLS.SetOsError(pParam);
}

int WinRegKey::PutBinary(const char * pParam, const void * pBuf, size_t bufLen)
{
	if(Key == 0)
		return 0;
	LONG   r = RegSetValueEx(Key, SUcSwitch(pParam), 0, REG_BINARY, static_cast<const uint8 *>(pBuf), (DWORD)bufLen);
	return (r == ERROR_SUCCESS) ? 1 : SLS.SetOsError(pParam);
}

int WinRegKey::PutValue(const char * pParam, const WinRegValue * pVal)
{
	if(Key == 0 || !pVal->GetType())
		return 0;
	LONG   r = RegSetValueEx(Key, SUcSwitch(pParam), 0, pVal->GetType(), reinterpret_cast<const uint8 *>(pVal->P_Buf), (DWORD)pVal->DataSize);
	return (r == ERROR_SUCCESS) ? 1 : SLS.SetOsError(pParam);
}

int WinRegKey::PutEnumeratedStrings(const StringSet & rSs, StrAssocArray * pResult)
{
	int    ok = 1;
	SString temp_buf;
	SString param_buf;
	WinRegValue value;
	StrAssocArray ex_list;
	CALLPTRMEMB(pResult, Z());
	THROW(Key);
	{
		for(uint i = 0; EnumValues(&i, &param_buf, &value) > 0;) {
			if(param_buf.IsDigit()) {
				const long p = param_buf.ToLong();
				value.GetStringUtf8(temp_buf);
				if(temp_buf.NotEmptyS()) {
					ex_list.Add(p, temp_buf, 1);
					CALLPTRMEMB(pResult, Add(p, temp_buf, 1));
				}
			}
		}
	}
	{
		long _max_id = 0;
		ex_list.GetMaxID(&_max_id);
		for(uint ssp = 0; rSs.get(&ssp, temp_buf);) {
			if(!ex_list.SearchByTextNc(temp_buf, 0)) {
				value.PutStringUtf8(temp_buf);
				param_buf.Z().Cat(++_max_id);
				THROW(PutValue(param_buf, &value));
				CALLPTRMEMB(pResult, Add(_max_id, temp_buf, 1));
			}
		}
	}
	CATCHZOK
	return ok;
}

int WinRegKey::EnumValues(uint * pIdx, SString * pParam, WinRegValue * pVal)
{
	CALLPTRMEMB(pParam, Z()); // @v10.3.11
	const size_t init_name_len = 256;
	if(Key == 0)
		return 0;
	DWORD  idx = *pIdx;
	DWORD  data_len = 2048;
	DWORD  name_len = init_name_len;
	TCHAR  name[init_name_len];
	if(!pVal->Alloc(data_len))
		return 0;
	DWORD  typ = pVal->Type;
	LONG   r = RegEnumValue(Key, idx, name, &name_len, 0, &typ, reinterpret_cast<BYTE *>(pVal->P_Buf), &data_len);
	pVal->Type = typ;
	if(r == ERROR_SUCCESS) {
		ASSIGN_PTR(pParam, SUcSwitch(name));
		pVal->DataSize = data_len;
		*pIdx = idx+1;
		return 1;
	}
	else
		return SLS.SetOsError();
}

bool WinRegKey::EnumKeys(uint * pIdx, SString & rKey)
{
	bool   ok = false;
	if(Key && pIdx) {
		const  size_t init_name_len = 256;
		DWORD  idx = *pIdx;
		DWORD  data_len = 2048;
		DWORD  name_len = init_name_len;
		FILETIME last_tm;
		TCHAR  name[init_name_len];
		LONG   r = RegEnumKeyEx(Key, idx, name, &name_len, NULL, NULL, NULL, &last_tm);
		if(r == ERROR_SUCCESS) {
			rKey.CopyFrom(SUcSwitch(name));
			*pIdx = idx + 1;
			ok = true;
		}
	}
	return ok;
}

int WinRegKey::Save(const char * pFileName)
{
	int    ok = 1;
	/*
		LSTATUS RegSaveKeyA([in] HKEY hKey, [in] LPCSTR lpFile, [in, optional] const LPSECURITY_ATTRIBUTES lpSecurityAttributes);
	*/ 
	THROW(!isempty(pFileName)); // @todo @err
	THROW(IsValid());
	if(fileExists(pFileName)) {
		// Функция RegSaveKeyW выдаст ошибку если файл с таким именем уже существует
		SFile::Remove(pFileName);
	}
	{
		SPtrHandle _token = SlProcess::OpenCurrentAccessToken(/*TOKEN_ALL_ACCESS*/TOKEN_READ|TOKEN_WRITE|TOKEN_EXECUTE);
		SlProcess::CheckAndEnableAccesTokenPrivilege(_token, SE_BACKUP_NAME);
	}
	{
		SStringU & r_file_name_u = SLS.AcquireRvlStrU();
		SString & r_temp_buf = SLS.AcquireRvlStr();
		THROW((r_temp_buf = pFileName).Strip().CopyToUnicode(r_file_name_u));
		const LSTATUS r = RegSaveKeyW(Key, r_file_name_u, 0);
		if(r != ERROR_SUCCESS) {
			SLS.SetError(SLERR_WINDOWS);
		}
	}
	CATCHZOK
	return ok;
}

int WinRegKey::Restore(const char * pFileName)
{
	int    ok = 1;
	/*
		LSTATUS RegRestoreKeyA([in] HKEY hKey, [in] LPCSTR lpFile, [in] DWORD  dwFlags);
	*/
	THROW(!isempty(pFileName)); // @todo @err
	THROW(IsValid());
	THROW(fileExists(pFileName));
	{
		SStringU & r_file_name_u = SLS.AcquireRvlStrU();
		SString & r_temp_buf = SLS.AcquireRvlStr();
		THROW((r_temp_buf = pFileName).Strip().CopyToUnicode(r_file_name_u));
		const LSTATUS r = RegRestoreKeyW(Key, r_file_name_u, REG_FORCE_RESTORE);
		if(r != ERROR_SUCCESS) {
			SLS.SetError(SLERR_WINDOWS);
		}
	}
	CATCHZOK
	return ok;
}
