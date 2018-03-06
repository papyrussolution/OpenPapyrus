// ArchiveOpenCallback.cpp

#include <7z-internal.h>
#pragma hdrstop
#include <7z-ifcs.h>

using namespace NWindows;

COpenCallbackImp::COpenCallbackImp() : Callback(NULL), _subArchiveMode(false) 
{
}

void COpenCallbackImp::Init(const FString &folderPrefix, const FString &fileName)
{
	_folderPrefix = folderPrefix;
	if(!_fileInfo.Find(_folderPrefix + fileName))
		throw 20121118;
	FileNames.Clear();
	FileNames_WasUsed.Clear();
	FileSizes.Clear();
	_subArchiveMode = false;
	// TotalSize = 0;
	PasswordWasAsked = false;
}

bool FASTCALL COpenCallbackImp::SetSecondFileInfo(CFSTR newName)
{
	return _fileInfo.Find(newName) && !_fileInfo.IsDir();
}

STDMETHODIMP COpenCallbackImp::SetTotal(const uint64 * files, const uint64 * bytes)
{
	COM_TRY_BEGIN
	if(ReOpenCallback)
		return ReOpenCallback->SetTotal(files, bytes);
	if(!Callback)
		return S_OK;
	return Callback->Open_SetTotal(files, bytes);
	COM_TRY_END
}

STDMETHODIMP COpenCallbackImp::SetCompleted(const uint64 * files, const uint64 * bytes)
{
	COM_TRY_BEGIN
	if(ReOpenCallback)
		return ReOpenCallback->SetCompleted(files, bytes);
	if(!Callback)
		return S_OK;
	return Callback->Open_SetCompleted(files, bytes);
	COM_TRY_END
}

STDMETHODIMP COpenCallbackImp::GetProperty(PROPID propID, PROPVARIANT * value)
{
	COM_TRY_BEGIN
	NCOM::CPropVariant prop;
	if(_subArchiveMode)
		switch(propID) {
			case kpidName: prop = _subArchiveName; break;
			    // case kpidSize:  prop = _subArchiveSize; break; // we don't use it now
		}
	else
		switch(propID) {
			case kpidName:  prop = _fileInfo.Name; break;
			case kpidIsDir:  prop = _fileInfo.IsDir(); break;
			case kpidSize:  prop = _fileInfo.Size; break;
			case kpidAttrib:  prop = (uint32)_fileInfo.Attrib; break;
			case kpidCTime:  prop = _fileInfo.CTime; break;
			case kpidATime:  prop = _fileInfo.ATime; break;
			case kpidMTime:  prop = _fileInfo.MTime; break;
		}
	prop.Detach(value);
	return S_OK;
	COM_TRY_END
}

struct CInFileStreamVol : public CInFileStream {
	int FileNameIndex;
	COpenCallbackImp * OpenCallbackImp;
	CMyComPtr<IArchiveOpenCallback> OpenCallbackRef;
	~CInFileStreamVol()
	{
		if(OpenCallbackRef)
			OpenCallbackImp->FileNames_WasUsed[FileNameIndex] = false;
	}
};

// from ArchiveExtractCallback.cpp
bool IsSafePath(const UString &path);

STDMETHODIMP COpenCallbackImp::GetStream(const wchar_t * name, IInStream ** inStream)
{
	COM_TRY_BEGIN
	* inStream = NULL;
	if(_subArchiveMode)
		return S_FALSE;
	if(Callback) {
		RINOK(Callback->Open_CheckBreak());
	}

	UString name2 = name;

  #ifndef _SFX

  #ifdef _WIN32
	name2.Replace(L'/', WCHAR_PATH_SEPARATOR);
  #endif

	// if(!allowAbsVolPaths)
	if(!IsSafePath(name2))
		return S_FALSE;

  #endif

	FString fullPath;
	if(!NFile::NName::GetFullPath(_folderPrefix, us2fs(name2), fullPath))
		return S_FALSE;
	if(!_fileInfo.Find(fullPath))
		return S_FALSE;
	if(_fileInfo.IsDir())
		return S_FALSE;
	CInFileStreamVol * inFile = new CInFileStreamVol;
	CMyComPtr<IInStream> inStreamTemp = inFile;
	if(!inFile->Open(fullPath)) {
		DWORD lastError = ::GetLastError();
		if(lastError == 0)
			return E_FAIL;
		return HRESULT_FROM_WIN32(lastError);
	}

	FileSizes.Add(_fileInfo.Size);
	FileNames.Add(name2);
	inFile->FileNameIndex = FileNames_WasUsed.Add(true);
	inFile->OpenCallbackImp = this;
	inFile->OpenCallbackRef = this;
	// TotalSize += _fileInfo.Size;
	*inStream = inStreamTemp.Detach();
	return S_OK;
	COM_TRY_END
}

#ifndef _NO_CRYPTO
	STDMETHODIMP COpenCallbackImp::CryptoGetTextPassword(BSTR * password)
	{
		COM_TRY_BEGIN
		if(ReOpenCallback) {
			CMyComPtr<ICryptoGetTextPassword> getTextPassword;
			ReOpenCallback.QueryInterface(IID_ICryptoGetTextPassword, &getTextPassword);
			if(getTextPassword)
				return getTextPassword->CryptoGetTextPassword(password);
		}
		if(!Callback)
			return E_NOTIMPL;
		PasswordWasAsked = true;
		return Callback->Open_CryptoGetTextPassword(password);
		COM_TRY_END
	}
#endif
