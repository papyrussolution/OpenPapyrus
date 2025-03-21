// UserInputUtils.cpp

#include <7z-internal.h>
#pragma hdrstop
#include <7z-ifcs.h>

static const char kYes = 'y';
static const char kNo = 'n';
static const char kYesAll = 'a';
static const char kNoAll = 's';
static const char kAutoRenameAll = 'u';
static const char kQuit = 'q';
static const char * const kFirstQuestionMessage = "? ";
static const char * const kHelpQuestionMessage = "(Y)es / (N)o / (A)lways / (S)kip all / A(u)to rename all / (Q)uit? ";

// return true if pressed Quite;

NUserAnswerMode::EEnum ScanUserYesNoAllQuit(CStdOutStream * outStream)
{
	if(outStream)
		*outStream << kFirstQuestionMessage;
	for(;;) {
		if(outStream) {
			*outStream << kHelpQuestionMessage;
			outStream->Flush();
		}
		AString scannedString;
		if(!g_StdIn.ScanAStringUntilNewLine(scannedString))
			return NUserAnswerMode::kError;
		else if(g_StdIn.Error())
			return NUserAnswerMode::kError;
		else {
			scannedString.Trim();
			if(scannedString.IsEmpty() && g_StdIn.Eof())
				return NUserAnswerMode::kEof;
			else if(scannedString.Len() == 1) {
				switch(::MyCharLower_Ascii(scannedString[0])) {
					case kYes: return NUserAnswerMode::kYes;
					case kNo: return NUserAnswerMode::kNo;
					case kYesAll: return NUserAnswerMode::kYesAll;
					case kNoAll: return NUserAnswerMode::kNoAll;
					case kAutoRenameAll: return NUserAnswerMode::kAutoRenameAll;
					case kQuit: return NUserAnswerMode::kQuit;
				}
			}
		}
	}
}

#ifdef _WIN32
	#ifndef UNDER_CE
		#define MY_DISABLE_ECHO
	#endif
#endif

static bool GetPassword(CStdOutStream * outStream, UString &psw)
{
	if(outStream) {
		*outStream << "\nEnter password"
      #ifdef MY_DISABLE_ECHO
		" (will not be echoed)"
      #endif
		":";
		outStream->Flush();
	}
  #ifdef MY_DISABLE_ECHO
	HANDLE console = GetStdHandle(STD_INPUT_HANDLE);
	bool   wasChanged = false;
	DWORD  mode = 0;
	if(!oneof2(console, INVALID_HANDLE_VALUE, 0))
		if(GetConsoleMode(console, &mode))
			wasChanged = (SetConsoleMode(console, mode & ~ENABLE_ECHO_INPUT) != 0);
	bool res = g_StdIn.ScanUStringUntilNewLine(psw);
	if(wasChanged)
		SetConsoleMode(console, mode);
  #else
	bool res = g_StdIn.ScanUStringUntilNewLine(psw);
  #endif
	if(outStream) {
		*outStream << endl;
		outStream->Flush();
	}
	return res;
}

HRESULT GetPassword_HRESULT(CStdOutStream * outStream, UString &psw)
{
	if(!GetPassword(outStream, psw))
		return E_INVALIDARG;
	else if(g_StdIn.Error())
		return E_FAIL;
	else if(g_StdIn.Eof() && psw.IsEmpty())
		return E_ABORT;
	else
		return S_OK;
}

