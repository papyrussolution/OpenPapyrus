// WINREG.CPP
// Copyright (c) A.Sobolev 2003, 2005, 2007, 2008, 2010, 2013, 2014, 2016
//
#include <slib.h>
#include <tv.h>
#pragma hdrstop
#include <shlwapi.h>
//
// @v8.3.3 Удалено условие #ifdef __WIN32__
//
//
SLAPI SDynLibrary::SDynLibrary(const char * pFileName)
{
	H = 0;
	if(pFileName)
		Load(pFileName);
}

SLAPI SDynLibrary::~SDynLibrary()
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
	H = ::LoadLibrary(pFileName); // @unicodeproblem
	if(H)
		return 1;
	else {
		SLS.SetError(SLERR_DLLLOADFAULT, pFileName);
		return 0;
	}
}

int SLAPI SDynLibrary::IsValid() const
{
	if(H)
		return 1;
	else {
		if(SLS.GetTLA().LastErr != SLERR_DLLLOADFAULT)
			SLS.SetError(SLERR_DLLLOADFAULT, "");
		return 0;
	}
}

FARPROC SLAPI SDynLibrary::GetProcAddr(const char * pProcName, int unicodeSuffix)
{
	FARPROC proc = 0;
	if(H) {
		if(unicodeSuffix) {
			SString temp_buf = pProcName;
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
SLAPI WinRegValue::WinRegValue(size_t bufSize)
{
	Type = 0;
	P_Buf = 0;
	BufSize = DataSize = 0;
	Alloc(bufSize);
}

SLAPI WinRegValue::~WinRegValue()
{
	SAlloc::F(P_Buf);
}

int SLAPI WinRegValue::Alloc(size_t bufSize)
{
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
}

uint32 SLAPI WinRegValue::GetDWord() const
{
	if(oneof3(Type, REG_DWORD, REG_DWORD_LITTLE_ENDIAN, REG_DWORD_BIG_ENDIAN))
		if(P_Buf && DataSize >= sizeof(DWORD))
			return (uint32)*(DWORD *)P_Buf;
	return 0;
}

const void * SLAPI WinRegValue::GetBinary(size_t * pDataLen) const
{
	ASSIGN_PTR(pDataLen, DataSize);
	return P_Buf;
}

const char * SLAPI WinRegValue::GetString() const
{
	return (Type == REG_SZ) ? (const char *)P_Buf : 0;
}

int SLAPI WinRegValue::PutDWord(uint32 val)
{
	Type = REG_DWORD;
	if(Alloc(sizeof(DWORD))) {
		*(DWORD *)P_Buf = val;
		DataSize = sizeof(DWORD);
		return 1;
	}
	else
		return 0;
}

int SLAPI WinRegValue::PutBinary(const void * pBuf, size_t dataSize)
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

int SLAPI WinRegValue::PutString(const char * pStr)
{
	Type = REG_SZ;
	size_t len = pStr ? strlen(pStr)+1 : 0;
	if(Alloc(len)) {
		memcpy(P_Buf, pStr, len);
		DataSize = len;
		return 1;
	}
	else
		return 0;
}
//
//
//
SLAPI WinRegKey::WinRegKey()
{
	Key = 0;
}

SLAPI WinRegKey::WinRegKey(HKEY key, const char * pSubKey, int readOnly)
{
	Key = 0;
	Open(key, pSubKey, readOnly);
}

SLAPI WinRegKey::~WinRegKey()
{
	Close();
}

int SLAPI WinRegKey::Delete(HKEY key, const char * pSubKey)
{
	return (SHDeleteKey(key, pSubKey) == ERROR_SUCCESS) ? 1 : SLS.SetOsError(pSubKey); // @unicodeproblem
}

int SLAPI WinRegKey::DeleteValue(HKEY key, const char * pSubKey, const char * pValue)
{
	return (SHDeleteValue(key, pSubKey, pValue) == ERROR_SUCCESS) ? 1 : SLS.SetOsError(pValue); // @unicodeproblem
}

int SLAPI WinRegKey::Open(HKEY key, const char * pSubKey, int readOnly, int onlyOpen)
{
	LONG   r = 0;
	DWORD  dispos = 0;
	Close();
	if(onlyOpen)
		r = RegOpenKeyEx(key, pSubKey, 0, readOnly ? KEY_READ : (KEY_READ | KEY_WRITE), &Key); // @unicodeproblem
	else
		r = RegCreateKeyEx(key, pSubKey, 0, 0, REG_OPTION_NON_VOLATILE,
			readOnly ? KEY_READ : (KEY_READ | KEY_WRITE), NULL, &Key, &dispos); // @unicodeproblem
	return (r == ERROR_SUCCESS) ? 1 : SLS.SetOsError(pSubKey);
}

int SLAPI WinRegKey::Close()
{
	if(Key) {
		RegCloseKey(Key);
		Key = 0;
	}
	return 1;
}

int SLAPI WinRegKey::GetDWord(const char * pParam, uint32 * pVal)
{
	int    ok = 1;
	if(Key) {
		DWORD  type = 0;
		DWORD  val  = 0;
		DWORD  size = sizeof(val);
		LONG   r = RegQueryValueEx(Key, pParam, 0, &type, (LPBYTE)&val, &size); // @unicodeproblem
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

int SLAPI WinRegKey::GetString(const char * pParam, char * pBuf, size_t bufLen)
{
	if(Key == 0)
		return 0;
	DWORD type = 0;
	DWORD size = bufLen;
	LONG  r = RegQueryValueEx(Key, pParam, 0, &type, (LPBYTE)pBuf, &size); // @unicodeproblem
	return oneof2(r, ERROR_SUCCESS, ERROR_MORE_DATA) ? 1 : SLS.SetOsError(pParam);
}

int SLAPI WinRegKey::GetString(const char * pParam, SString & rBuf)
{
	int    ok = 1;
	rBuf = 0;
	if(Key == 0)
		ok = 0;
	else {
		DWORD type = 0;
		STempBuffer temp_buf(1024);
		DWORD size = temp_buf.GetSize();
		LONG  r = ERROR_MORE_DATA;
		while(r == ERROR_MORE_DATA) {
			temp_buf.Alloc(size);
			size = temp_buf.GetSize();
			r = RegQueryValueEx(Key, pParam, 0, &type, (LPBYTE)(char *)temp_buf, &size); // @unicodeproblem
		}
		if(r == ERROR_SUCCESS)
			rBuf.CatN(temp_buf, size);
		else
			ok = SLS.SetOsError(pParam);
	}
	return ok;
}

int SLAPI WinRegKey::GetBinary(const char * pParam, void * pBuf, size_t bufLen)
{
	if(Key == 0)
		return 0;
	DWORD type = 0;
	DWORD size = bufLen;
	LONG  r = RegQueryValueEx(Key, pParam, 0, &type, (LPBYTE)pBuf, &size); // @unicodeproblem
	return oneof2(r, ERROR_SUCCESS, ERROR_MORE_DATA) ? 1 : SLS.SetOsError(pParam);
}

int SLAPI WinRegKey::GetRecSize(const char * pParam, size_t * pRecSize)
{
	if(Key == 0)
		return 0;
	DWORD type = 0;
	DWORD size = 0;
	LONG  r = RegQueryValueEx(Key, pParam, 0, &type, 0, &size); // @unicodeproblem
	if(r == ERROR_SUCCESS || r == ERROR_MORE_DATA) {
		ASSIGN_PTR(pRecSize, size);
		return 1;
	}
	else
		return SLS.SetOsError(pParam);
}

int SLAPI WinRegKey::PutDWord(const char * pParam, uint32 val)
{
	if(Key == 0)
		return 0;
	LONG   r = RegSetValueEx(Key, pParam, 0, REG_DWORD, (LPBYTE)&val, sizeof(val)); // @unicodeproblem
	return (r == ERROR_SUCCESS) ? 1 : SLS.SetOsError(pParam);
}

int SLAPI WinRegKey::PutString(const char * pParam, const char * pBuf)
{
	if(Key == 0)
		return 0;
	LONG   r = RegSetValueEx(Key, pParam, 0, REG_SZ, (LPBYTE)pBuf, strlen(pBuf) + 1); // @unicodeproblem
	return (r == ERROR_SUCCESS) ? 1 : SLS.SetOsError(pParam);
}

int SLAPI WinRegKey::PutBinary(const char * pParam, const void * pBuf, size_t bufLen)
{
	if(Key == 0)
		return 0;
	LONG   r = RegSetValueEx(Key, pParam, 0, REG_BINARY, (LPBYTE)pBuf, bufLen); // @unicodeproblem
	return (r == ERROR_SUCCESS) ? 1 : SLS.SetOsError(pParam);
}

int SLAPI WinRegKey::PutValue(const char * pParam, const WinRegValue * pVal)
{
	if(Key == 0 || !pVal->GetType())
		return 0;
	LONG   r = RegSetValueEx(Key, pParam, 0, pVal->GetType(), (LPBYTE)pVal->P_Buf, pVal->DataSize); // @unicodeproblem
	return (r == ERROR_SUCCESS) ? 1 : SLS.SetOsError(pParam);
}

int SLAPI WinRegKey::EnumValues(uint * pIdx, SString * pParam, WinRegValue * pVal)
{
	const size_t init_name_len = 256;

	if(Key == 0)
		return 0;
	DWORD  idx = *pIdx;
	DWORD  data_len = 2048;
	DWORD  name_len = init_name_len;
	char   name[init_name_len];
	if(!pVal->Alloc(data_len))
		return 0;
	DWORD  typ = pVal->Type;
	LONG   r = RegEnumValue(Key, idx, name, &name_len, 0, &typ, (BYTE *)pVal->P_Buf, &data_len); // @unicodeproblem
	pVal->Type = typ;
	if(r == ERROR_SUCCESS) {
		pParam->CopyFrom(name);
		pVal->DataSize = data_len;
		*pIdx = idx+1;
		return 1;
	}
	else
		return SLS.SetOsError();
}

int SLAPI WinRegKey::EnumKeys(uint * pIdx, SString & rKey)
{
	int    ok = 0;
	if(Key && pIdx) {
		const size_t init_name_len = 256;
		DWORD  idx = *pIdx;
		DWORD  data_len = 2048;
		DWORD  name_len = init_name_len;
		FILETIME last_tm;
		char   name[init_name_len];
		LONG   r = RegEnumKeyEx(Key, idx, name, &name_len, NULL, NULL, NULL, &last_tm); // @unicodeproblem
		if(r == ERROR_SUCCESS) {
			rKey.CopyFrom(name);
			*pIdx = idx + 1;
			ok = 1;
		}
	}
	return ok;
}

