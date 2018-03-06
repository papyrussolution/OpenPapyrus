// OpenCallbackConsole.cpp

#include <7z-internal.h>
#pragma hdrstop
#include <7z-ifcs.h>

static HRESULT CheckBreak2()
{
	return NConsoleClose::TestBreakSignal() ? E_ABORT : S_OK;
}

COpenCallbackConsole::COpenCallbackConsole() : _totalFilesDefined(false),
	/*_totalBytesDefined(false),*/ _totalBytes(0), MultiArcMode(false)
    #ifndef _NO_CRYPTO
	, PasswordIsDefined(false)
	// , PasswordWasAsked(false)
    #endif
{
}

void COpenCallbackConsole::ClosePercents()
{
	if(NeedPercents())
		_percent.ClosePrint(true);
}

void COpenCallbackConsole::Init(CStdOutStream * outStream, CStdOutStream * errorStream, CStdOutStream * percentStream)
{
	_so = outStream;
	_se = errorStream;
	_percent._so = percentStream;
}

HRESULT COpenCallbackConsole::Open_CheckBreak()
{
	return CheckBreak2();
}

HRESULT COpenCallbackConsole::Open_SetTotal(const uint64 * files, const uint64 * bytes)
{
	if(!MultiArcMode && NeedPercents()) {
		if(files) {
			_totalFilesDefined = true;
			// _totalFiles = *files;
			_percent.Total = *files;
		}
		else
			_totalFilesDefined = false;
		if(bytes) {
			// _totalBytesDefined = true;
			_totalBytes = *bytes;
			if(!files)
				_percent.Total = *bytes;
		}
		else {
			// _totalBytesDefined = false;
			if(!files)
				_percent.Total = _totalBytes;
		}
	}
	return CheckBreak2();
}

HRESULT COpenCallbackConsole::Open_SetCompleted(const uint64 * files, const uint64 * bytes)
{
	if(!MultiArcMode && NeedPercents()) {
		if(files) {
			_percent.Files = *files;
			if(_totalFilesDefined)
				_percent.Completed = *files;
		}
		if(bytes) {
			if(!_totalFilesDefined)
				_percent.Completed = *bytes;
		}
		_percent.Print();
	}
	return CheckBreak2();
}

HRESULT COpenCallbackConsole::Open_Finished()
{
	ClosePercents();
	return S_OK;
}

#ifndef _NO_CRYPTO

HRESULT COpenCallbackConsole::Open_CryptoGetTextPassword(BSTR * password)
{
	*password = NULL;
	RINOK(CheckBreak2());
	if(!PasswordIsDefined) {
		ClosePercents();
		RINOK(GetPassword_HRESULT(_so, Password));
		PasswordIsDefined = true;
	}
	return StringToBstr(Password, password);
}
/*
   HRESULT COpenCallbackConsole::Open_GetPasswordIfAny(bool &passwordIsDefined, UString &password)
   {
   passwordIsDefined = PasswordIsDefined;
   password = Password;
   return S_OK;
   }
   bool COpenCallbackConsole::Open_WasPasswordAsked() { return PasswordWasAsked; }
   void COpenCallbackConsole::Open_Clear_PasswordWasAsked_Flag () { PasswordWasAsked = false; }
 */

#endif
