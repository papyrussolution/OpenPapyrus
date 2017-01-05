// CUSTDISP_EPSON.CPP
// Библиотека для работы с дисплеем покупателя VFD-Epson
//

#include <slib.h>
#include <CustomDisplay.h>

#define STRLEN		20 // Число символов в строке

class VfdEquip : public CustomDisplayEquip {
public:
	int Connect(); // new
	int GetConfig(char * pBuf, size_t bufSize);
	int PrintLine();
};

static VfdEquip * P_VfdDisp = 0;

BOOL APIENTRY DllMain(HANDLE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
    return true; 
}

EXPORT int /*STDAPICALLTYPE*/ RunCommand(const char * pCmd, const char * pInputData, char * pOutputData, size_t outSize)
{
	int    ok = 0;
	THROW((pCmd != NULL) && (pOutputData != NULL) && (outSize != NULL));
	if(sstreqi_ascii(pCmd, "INIT"))
		THROWERR(Init(), CUSTDISP_NOTINITED)
	else if(sstreqi_ascii(pCmd, "RELEASE"))
		THROW(Release())
	else if(P_VfdDisp) {
		ok = P_VfdDisp->RunOneCommand(pCmd, pInputData, pOutputData, outSize);
		if(ok == 0) {
			if(sstreqi_ascii(pCmd, "GETCONFIG"))
				ok = P_VfdDisp->GetConfig(pOutputData, outSize);
			else if(sstreqi_ascii(pCmd, "PUTLINE"))
				THROW(P_VfdDisp->PrintLine() == 0)
			// new {
			else if(sstreqi_ascii(pCmd, "CONNECT")) {
				THROWERR(P_VfdDisp->Connect(), CUSTDISP_NOTCONNECTED);
			}
			// } new
		}
	}
	CATCH
		ok = 1;
		_itoa(ErrorCode, pOutputData, 10);
		P_VfdDisp->LastError = ErrorCode;
		ErrorCode = 0;
	ENDCATCH;
	return ok;
}

int Init() 
{
	SETIFZ(P_VfdDisp, new VfdEquip);
	return 1;
}

int Release() 
{	
	ZDELETE(P_VfdDisp);
	return 1;
}

int VfdEquip::GetConfig(char * pBuf, size_t bufSize)
{
	int ok = 0;
	SString str;
	str.CatEq("STRLEN", (long)STRLEN);
	if(str.Len() + 1 > bufSize)
		ok = 2;
	else
		memcpy(pBuf, str, str.Len() + 1);
	return ok;
}

int VfdEquip::PrintLine() 
{
	int ok = 0;
	if(VerTab == VERT_UPP || VerTab == VERT_DOWN) {
		THROWERR(CommPort.PutChr(0x0b), CUSTDISP_PRINTTOPORT);
		if(VerTab == VERT_DOWN)
			THROWERR(CommPort.PutChr(0x0a), CUSTDISP_PRINTTOPORT);
	}
	if(Align == LEFT)
		Text.Align(STRLEN, ADJ_LEFT);
	else if(Align == CENTER)
		Text.Align(STRLEN, ADJ_CENTER);
	else if(Align == RIGHT)
		Text.Align(STRLEN, ADJ_RIGHT);
	Text.ToUpper();
	for(uint i = 0; i < Text.Len(); i++)
		THROWERR(CommPort.PutChr(Text.C(i)), CUSTDISP_PRINTTOPORT);
	CATCH
		ok = 1;
	ENDCATCH;
	ClearParams();
	return ok;
}

// new {
int VfdEquip::Connect() 
{
	int ok = 1;
	SString control_symbol;
	THROW(CommPort.InitPort(Port));
	(control_symbol = 0).CatChar(0x1B).CatChar(0x74).CatChar(6); // Установим кодовую таблицу CP866
	for(size_t index = 0; index < control_symbol.Len(); index++)
		THROW(CommPort.PutChr(control_symbol.C(index)));
	CATCH
		ok = 0;
	ENDCATCH;
	return ok;
}
// } new
