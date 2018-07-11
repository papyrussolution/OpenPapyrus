// VIKI_VISION.CPP
// Библиотека для работы с дисплеем покупателя VikiVision
//

#include <slib.h>
#include <CustomDisplay.h>

#define STRLEN			20 // Число символов в строке

BOOL APIENTRY DllMain(HANDLE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
    return true; 
}

class VikiVsnEquip : public CustomDisplayEquip {
public:
	int Connect(); // new
	int GetConfig(char * pBuf, size_t bufSize);
	int PrintLine();
};

static VikiVsnEquip * P_VikiVsnDisp = 0;

EXPORT int /*STDAPICALLTYPE*/ RunCommand(const char * pCmd, const char * pInputData, char * pOutputData, size_t outSize)
{
	int    ok = 0;
	THROW((pCmd != NULL) && (pOutputData != NULL) && (outSize != NULL));
	if(sstreqi_ascii(pCmd, "INIT"))
		THROWERR(Init(), CUSTDISP_NOTINITED)
	else if(sstreqi_ascii(pCmd, "RELEASE"))
		THROW(Release())
	else if(P_VikiVsnDisp) {
		ok = P_VikiVsnDisp->RunOneCommand(pCmd, pInputData, pOutputData, outSize);
		if(ok == 0) {
			if(sstreqi_ascii(pCmd, "GETCONFIG"))
				ok = P_VikiVsnDisp->GetConfig(pOutputData, outSize);
			else if(sstreqi_ascii(pCmd, "PUTLINE"))
				THROW(P_VikiVsnDisp->PrintLine() == 0)
			// new {
			else if(sstreqi_ascii(pCmd, "CONNECT")) {
				THROWERR(P_VikiVsnDisp->Connect(), CUSTDISP_NOTCONNECTED);
			}
			// } new
		}
	}
	CATCH
		ok = 1;
		_itoa(ErrorCode, pOutputData, 10);
		P_VikiVsnDisp->LastError = ErrorCode;
		ErrorCode = 0;
	ENDCATCH;
	return ok;
}

int Init() 
{
	SETIFZ(P_VikiVsnDisp, new VikiVsnEquip);
	return 1;
}

int Release() 
{	
	ZDELETE(P_VikiVsnDisp);
	return 1;
}

int VikiVsnEquip::GetConfig(char * pBuf, size_t bufSize)
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

int VikiVsnEquip::PrintLine() 
{
	int ok = 0;
	if(VerTab == VERT_UPP || VerTab == VERT_DOWN) {
		THROWERR(CommPort.PutChr(0x0b), CUSTDISP_PRINTTOPORT);
		if(VerTab == VERT_DOWN) {
			THROWERR(CommPort.PutChr(0x0a), CUSTDISP_PRINTTOPORT);
			THROWERR(CommPort.PutChr(0x0d), CUSTDISP_PRINTTOPORT); // Возврат корретки
		}
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
int VikiVsnEquip::Connect() { return CommPort.InitPort(Port, 0, 0); }
// } new
